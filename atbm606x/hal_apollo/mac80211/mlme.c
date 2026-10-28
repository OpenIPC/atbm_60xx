/*
 * BSS client mode implementation
 * Copyright 2003-2008, Jouni Malinen <j@w1.fi>
 * Copyright 2004, Instant802 Networks, Inc.
 * Copyright 2005, Devicescape Software, Inc.
 * Copyright 2006-2007	Jiri Benc <jbenc@suse.cz>
 * Copyright 2007, Michael Wu <flamingice@sourmilk.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/if_ether.h>
#include <linux/skbuff.h>
#include <linux/if_arp.h>
#include <linux/etherdevice.h>
#include <linux/moduleparam.h>
#include <linux/rtnetlink.h>
#include <linux/crc32.h>
#include <linux/slab.h>
#include <linux/export.h>
#include <net/atbm_mac80211.h>
#include <asm/unaligned.h>
#include <net/sch_generic.h>
#include <linux/kthread.h>



#include "ieee80211_i.h"
#include "driver-ops.h"
#include "rate.h"
#include "twt.h"
#define IEEE80211_AUTH_TIMEOUT (HZ / 2)
#define IEEE80211_AUTH_MAX_TRIES 3
#define IEEE80211_ASSOC_TIMEOUT (HZ / 2)
#define IEEE80211_ASSOC_MAX_TRIES 1
#define IEEE80211_CONNECT_TIMEOUT	(HZ/2)
#define IEEE80211_FLUSH_STA_TIMEOUT (HZ/2)
#define WK_CONNECT_TRIES_MAX (5)
static int authen_fail = 0;
module_param(authen_fail, int, 0644);
static int assoc_fail = 0;
module_param(assoc_fail, int, 0644);
static int sta_flush = 0;
module_param(sta_flush, int, 0644);
static int om_20M = 0;
module_param(om_20M, int, 0644);

static int uapsd = 0;
module_param(uapsd, int, 0644);
static int om = 0;
module_param(om, int, 0644);
static int ul_mu_disable = 1;
module_param(ul_mu_disable, int, 0644);
static int ul_mu_data_diable = 0;
module_param(ul_mu_data_diable, int, 0644);
static int enable_vht = 1;
module_param(enable_vht, int, 0644);

static int vht_action = 0;
module_param(vht_action, int, 0644);

#ifdef  CONFIG_SUPPORT_80M_ADAPTER
extern int atbm_80M_adapter ;
#endif  //#ifdef  CONFIG_SUPPORT_80M_ADAPTER

#ifdef CONFIG_ATBM_SUPPORT_SAE
enum sae_authen_state{
	SAE_AUTHEN_TX_COMMIT,
	SAE_AUTHEN_TX_CONFIRM,
	SAE_AUTHEN_RX_COMMIT,
	SAE_AUTHEN_RX_CONFIRM,
};
enum sae_action
{
    SAE_CONTINUE,
    SAE_SKIP,
    SAE_FAIL,
    SAE_DONE,
};
#endif
enum conn_process_state{
    CONN_PROCESS__CONTINUE,
    CONN_PROCESS__FINISH,
    CONN_PROCESS__SKIP,
    CONN_PROCESS__ERR,
};
bool atbm_internal_cmd_scan_triger(struct ieee80211_sub_if_data *sdata,struct ieee80211_internal_scan_request *req);
static void ieee80211_mgd_deauth_rx(struct ieee80211_sub_if_data *sdata,struct atbm_ieee80211_mgmt *mgmt,size_t len);
static void ieee80211_mgd_disassoc_rx(struct ieee80211_sub_if_data *sdata,struct atbm_ieee80211_mgmt *mgmt,size_t len);
static void  ieee80211_mgd_assoc_timeout(struct ieee80211_sub_if_data *sdata,struct ieee80211_mgd_assoc_req *req);
static void ieee80211_auth_challenge(struct ieee80211_sub_if_data *sdata,struct atbm_ieee80211_mgmt *mgmt,size_t len);
#ifdef CONFIG_ATBM_SUPPORT_SAE
static enum sae_action ieee80211_sae_authen_state(struct ieee80211_sub_if_data *sdata,enum sae_authen_state state);
static enum sae_authen_state ieee80211_sae_trans_state(u16 trans,bool tx);
#endif
static void ieee80211_mgd_auth_rx_authen(struct ieee80211_sub_if_data *sdata,struct atbm_ieee80211_mgmt *mgmt, size_t len);
static void ieee80211_rx_mgmt_assoc_resp(struct ieee80211_sub_if_data *sdata,struct atbm_ieee80211_mgmt *mgmt,size_t len,bool reassoc);
static bool ieee80211_assoc_success(struct ieee80211_sub_if_data *sdata,struct atbm_ieee80211_mgmt* mgmt, size_t len);
static void ieee80211_mgd_assoc_rx_resp(struct ieee80211_sub_if_data *sdata,struct cfg80211_bss *bss,struct atbm_ieee80211_mgmt* mgmt,size_t len);
static enum conn_process_state ieee80211_conn_state_process(struct ieee80211_sub_if_data *sdata,void *cookie);
static enum conn_process_state __ieee80211_conn_state_process(struct ieee80211_sub_if_data *sdata ,void *cookie);
#if defined(CONFIG_PM)
static void ieee80211_sta_connection_lost(struct ieee80211_sub_if_data* sdata,
	u8* bssid, u8 reason);
#endif

#ifdef CONFIG_ATBM_VHT
extern void ieee80211_vht_cap_ie_to_sta_vht_cap(struct ieee80211_sub_if_data *sdata,
				    struct ieee80211_supported_band *sband,
				    const struct atbm_ieee80211_vht_cap *vht_cap_ie,
				    struct sta_info *sta);

extern u8 *ieee80211_ie_build_vht_cap(u8 *pos, struct atbm_ieee80211_sta_vht_cap *vht_cap,u32 cap);
#endif

/*
 * Beacon loss timeout is calculated as N frames times the
 * advertised beacon interval.  This may need to be somewhat
 * higher than what hardware might detect to account for
 * delays in the host processing frames. But since we also
 * probe on beacon miss before declaring the connection lost
 * default to what we want.
 */
#define IEEE80211_BEACON_LOSS_COUNT 7

 /*
  * Time the connection can be idle before we probe
  * it to see if we can still talk to the AP.
  */
#define IEEE80211_CONNECTION_IDLE_TIME (30 * HZ)
/*
 * Weight given to the latest Beacon frame when calculating average signal
 * strength for Beacon frames received in the current BSS. This must be
 * between 1 and 15.
 */
#define IEEE80211_SIGNAL_AVE_WEIGHT 3

 /*
  * How many Beacon frames need to have been used in average signal strength
  * before starting to indicate signal change events.
  */
#define IEEE80211_SIGNAL_AVE_MIN_COUNT 4

  /*
   * All cfg80211 functions have to be called outside a locked
   * section so that they can acquire a lock themselves... This
   * is much simpler than queuing up things in cfg80211, but we
   * do need some indirection for that here.
   */
enum rx_mgmt_action
{
	/* no action required */
	RX_MGMT_NONE,

	/* caller must call cfg80211_send_deauth() */
	RX_MGMT_CFG80211_DEAUTH,

	/* caller must call cfg80211_send_disassoc() */
	RX_MGMT_CFG80211_DISASSOC,
};
static inline void ASSERT_MGD_MTX(struct ieee80211_if_managed* ifmgd)
{
	lockdep_assert_held(&ifmgd->mtx);
}
static void run_again(struct ieee80211_if_managed *ifmgd,
			     unsigned long timeout)
{
	ASSERT_MGD_MTX(ifmgd);

	if (!atbm_timer_pending(&ifmgd->timer) ||
	    time_before(timeout, ifmgd->timer.expires))
		atbm_mod_timer(&ifmgd->timer, timeout);
}
static void ieee80211_conn_running_hold(struct ieee80211_conn *conn)
{
    BUG_ON(conn->running == true);
    conn->running = true;
}
static void ieee80211_conn_running_release(struct ieee80211_conn *conn)
{
    conn->running = false;
}
static void ieee80211_sta_timer(unsigned long data)
{
	struct ieee80211_sub_if_data *sdata =
		(struct ieee80211_sub_if_data *) data;
	struct ieee80211_local *local = sdata->local;

    if(ieee80211_sdata_running(sdata)){
	    ieee80211_queue_work(&local->hw, &sdata->work);
    }
}
static const char * ieee80211_conn_state_string(struct ieee80211_conn *conn)
{
    switch(conn->state){
    case IEEE80211_CONN_IDLE :                 return "IDLE";
    case IEEE80211_CONN_CHAN_ASYNC:            return "CHAN_ASYNC";
    case IEEE80211_CONN_JOIN :                 return "JOIN";
    case IEEE80211_CONN_AUTHENTICATE_NEXT:     return "AUTH_NEXT";
    case IEEE80211_CONN_AUTHENTICATING:        return "AUTHENTICATING";
    case IEEE80211_CONN_AUTHEN_FAILED_TIMEOUT: return "AUTH_FAIL";
    case IEEE80211_CONN_ASSOCIATE_NEXT:        return "ASSOC_NEXT";
    case IEEE80211_CONN_ASSOCIATING:           return "ASSOCIATING";
    case IEEE80211_CONN_ASSOC_FAILED:          return "ASSOC_FAIL";
    case IEEE80211_CONN_ASSOC_FAILED_TIMEOUT:  return "ASSOC_TIMEOUT";
    case IEEE80211_CONN_ABANDON:               return "ABANDON";
    case IEEE80211_CONN_PRE_ABANDON:           return "PRE_ABANDON";
    case IEEE80211_CONN_CONNECTED:             return "CONN";
    default:                                   return "UNKOWN";
    }
}
static void ieee80211_conn_mark_idle(struct ieee80211_sub_if_data *sdata)
{
    struct ieee80211_if_managed* ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;

    atbm_del_timer_sync(&ifmgd->timer);

    conn->state = IEEE80211_CONN_IDLE;
    conn->work = NULL;
    ifmgd->sae_state = IEEE80211_SAE_NOTHING;
    ifmgd->sae_state_bit = 0;
}
static void ieee80211_sdata_free_auth_data(struct ieee80211_sub_if_data *sdata,struct ieee80211_mgd_auth_req *req)
{
    struct ieee80211_if_managed* ifmgd = &sdata->u.mgd;

    atbm_printk_err("%s:(%p),(%p)\n",__func__,req,req->bss);
    ieee80211_atbm_release_bss(sdata->local->hw.wiphy,req->bss);
    ifmgd->conn.auth_req = NULL;
    atbm_kfree(req);
}
static void ieee80211_sdata_free_assoc_data(struct ieee80211_sub_if_data *sdata,struct ieee80211_mgd_assoc_req *req)
{
    struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;

    ieee80211_atbm_release_bss(sdata->local->hw.wiphy,req->bss);
    ifmgd->conn.assoc_req = NULL;
    atbm_kfree(req);
}
static void ieee80211_sdata_mgd_lock(struct ieee80211_sub_if_data *sdata,void *cookie)
{
    struct ieee80211_if_managed* ifmgd = &sdata->u.mgd;

    if(cookie == NULL){
        mutex_lock(&sdata->wdev.mtx);
    }
    
    mutex_lock(&ifmgd->mtx);
}
static void ieee80211_sdata_mgd_unlock(struct ieee80211_sub_if_data *sdata,void *cookie)
{
    struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;

    mutex_unlock(&ifmgd->mtx);

    if(cookie == NULL){
        mutex_unlock(&sdata->wdev.mtx);
    }
}
/* utils */


#ifdef CONFIG_ATBM_SUPPORT_REKEY
void ieee80211_rekey_set_data_enable(struct ieee80211_sub_if_data *sdata,u8 *kck,u8 *kek)
{
	atbm_printk_err("KEK[%d][%d][%d][%d]\n",kek[0],kek[1],kek[2],kek[3]);
	atbm_printk_err("KCK[%d][%d][%d][%d]\n",kck[0],kck[1],kck[2],kck[3]);
	
	sdata->vif.rekey_set = 1;
	memcpy(sdata->vif.kck,kck,16);
	memcpy(sdata->vif.kek,kek,16);
}

void ieee80211_rekey_set_data_disable(struct ieee80211_sub_if_data *sdata)
{
	sdata->vif.rekey_set = 0;
	memset(sdata->vif.kck,0,16);
	memset(sdata->vif.kek,0,16);
}
#endif

/*
 * We can have multiple work items (and connection probing)
 * scheduling this timer, but we need to take care to only
 * reschedule it when it should fire _earlier_ than it was
 * asked for before, or if it's not pending right now. This
 * function ensures that. Note that it then is required to
 * run this function for all timeouts after the first one
 * has happened -- the work that runs from this timer will
 * do that.
 */
static int ecw2cw(int ecw)
{
	return (1 << ecw) - 1;
}

/*
 * ieee80211_enable_ht should be called only after the operating band
 * has been determined as ht configuration depends on the hw's
 * HT abilities for a specific band.
 */
static u32 ieee80211_enable_ht(struct ieee80211_sub_if_data* sdata,
	struct atbm_ieee80211_ht_info* hti,
	const u8* bssid, u16 ap_ht_cap_flags,
	bool beacon_htcap_ie)
{
	struct ieee80211_local* local = sdata->local;
	struct ieee80211_channel_state* chan_state = ieee80211_get_channel_state(local, sdata);
	struct ieee80211_supported_band* sband;
	struct sta_info* sta;
	u32 changed = 0;
	int hti_cfreq;
	u16 ht_opmode;
	bool enable_ht = true;
	enum nl80211_channel_type prev_chantype;
	enum nl80211_channel_type channel_type = NL80211_CHAN_NO_HT;

	sband = local->hw.wiphy->bands[chan_state->conf.channel->band];

	prev_chantype = sdata->vif.bss_conf.channel_type;

	/* HT is not supported */
	if (!sband->ht_cap.ht_supported)
		enable_ht = false;

	if (enable_ht) {
		hti_cfreq = ieee80211_channel_to_frequency(hti->control_chan, sband->band);
		/* check that channel matches the right operating channel */
		if (channel_center_freq(chan_state->conf.channel) != hti_cfreq) {
			/* Some APs mess this up, evidently.
			 * Netgear WNDR3700 sometimes reports 4 higher than
			 * the actual channel, for instance.
			 */
			atbm_printk_err(
				"%s: Wrong control channel in association"
				" response: configured center-freq: %d"
				" hti-cfreq: %d  hti->control_chan: %d"
				" band: %d.  Disabling HT.\n",
				sdata->name,
				channel_center_freq(chan_state->conf.channel),
				hti_cfreq, hti->control_chan,
				sband->band);
			enable_ht = false;
		}
	}

	if (enable_ht) {
		channel_type = NL80211_CHAN_HT20;

		if (!(ap_ht_cap_flags & IEEE80211_HT_CAP_40MHZ_INTOLERANT) &&
			(sband->ht_cap.cap & IEEE80211_HT_CAP_SUP_WIDTH_20_40) &&
			(hti->ht_param & IEEE80211_HT_PARAM_CHAN_WIDTH_ANY)) {
			switch (hti->ht_param & IEEE80211_HT_PARAM_CHA_SEC_OFFSET) {
			case IEEE80211_HT_PARAM_CHA_SEC_ABOVE:
				if (!(chan_state->conf.channel->flags &
					IEEE80211_CHAN_NO_HT40PLUS))
					channel_type = NL80211_CHAN_HT40PLUS;
				break;
			case IEEE80211_HT_PARAM_CHA_SEC_BELOW:
				if (!(chan_state->conf.channel->flags &
					IEEE80211_CHAN_NO_HT40MINUS))
					channel_type = NL80211_CHAN_HT40MINUS;
				break;
			}
		}
	}

	if (chan_state->tmp_channel)
		chan_state->tmp_channel_type = channel_type;

	if (!ieee80211_set_channel_type(local, sdata, channel_type)) {
		/* can only fail due to HT40+/- mismatch */
		channel_type = NL80211_CHAN_HT20;
		WARN_ON(!ieee80211_set_channel_type(local, sdata, channel_type));
	}

	if (beacon_htcap_ie && (prev_chantype != channel_type)) {
		/*
		 * Whenever the AP announces the HT mode change that can be
		 * 40MHz intolerant or etc., it would be safer to stop tx
		 * queues before doing hw config to avoid buffer overflow.
		 */
		ieee80211_stop_queues_by_reason(&sdata->local->hw,
			IEEE80211_QUEUE_STOP_REASON_CHTYPE_CHANGE);

		/* flush out all packets */
		synchronize_net();

		drv_flush(local, sdata, false);
	}

	/* channel_type change automatically detected */
	ieee80211_hw_config(local, 0);

	if (prev_chantype != channel_type) {
		rcu_read_lock();
		sta = sta_info_get(sdata, bssid);
		if (sta) {
			rate_control_rate_update(local, sband, sta,
				IEEE80211_RC_HT_CHANGED,
				channel_type);
			ieee80211_ht_cap_to_sta_channel_type(sta);
		}
		rcu_read_unlock();

		if (beacon_htcap_ie)
			ieee80211_wake_queues_by_reason(&sdata->local->hw,
				IEEE80211_QUEUE_STOP_REASON_CHTYPE_CHANGE);
	}

	ht_opmode = le16_to_cpu(hti->operation_mode);

	/* if bss configuration changed store the new one */
	if (sdata->ht_opmode_valid != enable_ht ||
		sdata->vif.bss_conf.ht_operation_mode != ht_opmode ||
		prev_chantype != channel_type) {
		changed |= BSS_CHANGED_HT;
		if(prev_chantype != channel_type)
			changed |= BSS_CHANGED_HT_CHANNEL_TYPE;
		sdata->vif.bss_conf.ht_operation_mode = ht_opmode;
		sdata->ht_opmode_valid = enable_ht;
	}

	return changed;
}

/* frame sending functions */

static void ieee80211_send_deauth_disassoc(struct ieee80211_sub_if_data* sdata,
	const u8* bssid, u16 stype, u16 reason,
	bool send_frame)
{
	struct ieee80211_local* local = sdata->local;
	struct ieee80211_if_managed* ifmgd = &sdata->u.mgd;
	struct sk_buff* skb;
	struct atbm_ieee80211_mgmt* mgmt;

	skb = atbm_dev_alloc_skb(local->hw.extra_tx_headroom + sizeof(*mgmt));
	if (!skb)
		return;

	atbm_skb_reserve(skb, local->hw.extra_tx_headroom);

	mgmt = (struct atbm_ieee80211_mgmt*)atbm_skb_put(skb, 24);
	memset(mgmt, 0, 24);
	memcpy(mgmt->da, bssid, ETH_ALEN);
	memcpy(mgmt->sa, sdata->vif.addr, ETH_ALEN);
	memcpy(mgmt->bssid, bssid, ETH_ALEN);
	mgmt->frame_control = cpu_to_le16(IEEE80211_FTYPE_MGMT | stype);
	atbm_skb_put(skb, 2);
	/* u.deauth.reason_code == u.disassoc.reason_code */
	mgmt->u.deauth.reason_code = cpu_to_le16(reason);

    if(stype == IEEE80211_STYPE_DEAUTH){
        ieee80211_mgd_deauth_rx(sdata,mgmt,skb->len);
    }else {
        ieee80211_mgd_disassoc_rx(sdata,mgmt,skb->len);
    }

	if (!(ifmgd->flags & IEEE80211_STA_MFP_ENABLED))
		IEEE80211_SKB_CB(skb)->flags |= IEEE80211_TX_INTFL_DONT_ENCRYPT;

	if (send_frame){
		ieee80211_tx_skb(sdata, skb);
		drv_flush(local,sdata,false);
	}else
		atbm_kfree_skb(skb);
}

#ifdef CONFIG_ATBM_4ADDR
static void ieee80211_send_4addr_nullfunc(struct ieee80211_local* local,
	struct ieee80211_sub_if_data* sdata)
{
	struct sk_buff* skb;
	struct ieee80211_hdr* nullfunc;
	__le16 fc;

	if (WARN_ON(sdata->vif.type != NL80211_IFTYPE_STATION))
		return;

	skb = atbm_dev_alloc_skb(local->hw.extra_tx_headroom + 30);
	if (!skb)
		return;

	atbm_skb_reserve(skb, local->hw.extra_tx_headroom);

	nullfunc = (struct ieee80211_hdr*)atbm_skb_put(skb, 30);
	memset(nullfunc, 0, 30);
	fc = cpu_to_le16(IEEE80211_FTYPE_DATA | IEEE80211_STYPE_NULLFUNC |
		IEEE80211_FCTL_FROMDS | IEEE80211_FCTL_TODS);
	nullfunc->frame_control = fc;
	memcpy(nullfunc->addr1, sdata->u.mgd.bssid, ETH_ALEN);
	memcpy(nullfunc->addr2, sdata->vif.addr, ETH_ALEN);
	memcpy(nullfunc->addr3, sdata->u.mgd.bssid, ETH_ALEN);
	memcpy(nullfunc->addr4, sdata->vif.addr, ETH_ALEN);

	IEEE80211_SKB_CB(skb)->flags |= IEEE80211_TX_INTFL_DONT_ENCRYPT;
	ieee80211_tx_skb(sdata, skb);
}
#endif
/* spectrum management related things */
#ifdef CONFIG_ATBM_SUPPORT_CSA
#ifdef CONFIG_ATBM_SUPPORT_CHANSWITCH_TEST
unsigned char *ieee80211_sta_swbeacon_add(struct ieee80211_sub_if_data *sdata,struct sk_buff *skb)
{
	struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
	
	if(ifmgd->sw_ie.count && (atbm_skb_headroom(skb) >= 5)){				
		u8 *data = skb->data;
		size_t len = skb_headlen(skb);
		skb->data -= 5;
		memmove(skb->data, data, len);
		atbm_skb_set_tail_pointer(skb, len);
		/*
		*set sw ie
		*/
		data = atbm_skb_put(skb,5);
		data[0] = ATBM_WLAN_EID_CHANNEL_SWITCH;
		data[1] = 3;
		data[2] = ifmgd->sw_ie.mode;
		data[3] = ifmgd->sw_ie.new_ch_num;
		data[4] = ifmgd->sw_ie.count -- ;

		atbm_printk_err("%s:%d\n",__func__,skb->len);
	}

	return skb->data;
}
#endif
void ieee80211_sta_process_chanswitch(struct ieee80211_sub_if_data *sdata,
					  struct atbm_ieee80211_channel_sw_packed_ie *sw_packed_ie,
					  struct ieee80211_bss *bss,
					  u64 timestamp)
{
	struct ieee80211_channel_state *chan_state = ieee80211_get_channel_state(sdata->local, sdata);
	struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
	enum ieee80211_band new_band = IEEE80211_NUM_BANDS;
	struct ieee80211_csa_request csa;
	ASSERT_MGD_MTX(ifmgd);

	if (!ifmgd->associated)
		return;
	
	/* Disregard subsequent beacons if we are already running a timer
	   processing a CSA */
	if(sdata->csa_state != IEEE80211_CSA_MLME_STATE_IDLE)
		return;
	
	if(sw_packed_ie->ex_chan_sw_ie){
		
		if(ieee80211_opclass_to_band(sw_packed_ie->ex_chan_sw_ie->new_operaring_class,&new_band) == false){
			return;
		}
		csa.ie.new_ch_num = sw_packed_ie->ex_chan_sw_ie->new_ch_num;
		csa.ie.mode = sw_packed_ie->ex_chan_sw_ie->mode;
		csa.ie.count = sw_packed_ie->ex_chan_sw_ie->count;
	}
	else if(sw_packed_ie->chan_sw_ie){
		new_band =  ifmgd->associated->channel->band;
		csa.ie.new_ch_num = sw_packed_ie->chan_sw_ie->new_ch_num;
		csa.ie.mode = sw_packed_ie->chan_sw_ie->mode;
		csa.ie.count = sw_packed_ie->chan_sw_ie->count;
	}else {
		/**
		*sec_chan_ie must be together with the sw_elem or ex_sw_elem
		*/
		return ;
	}
	
	csa.chantype  = chan_state->conf.channel_type;
	csa.chan = ieee80211_get_channel(sdata->local->hw.wiphy, 
				ieee80211_channel_to_frequency(csa.ie.new_ch_num,new_band));
	
	if (!csa.chan || csa.chan->flags & IEEE80211_CHAN_DISABLED)
		return;

	if(sw_packed_ie->sec_chan_offs_ie){		
		switch(sw_packed_ie->sec_chan_offs_ie->sec_chan_offs)
		{
			case IEEE80211_HT_PARAM_CHA_SEC_NONE:
				csa.chantype = NL80211_CHAN_NO_HT;
				break;
			case IEEE80211_HT_PARAM_CHA_SEC_ABOVE:
				csa.chantype = NL80211_CHAN_HT40PLUS;
				break;
			case IEEE80211_HT_PARAM_CHA_SEC_BELOW:
				csa.chantype = NL80211_CHAN_HT40MINUS;
				break;
			default:
				break;
		}
	}

	if((csa.chan == chan_state->oper_channel) && 
	   (csa.chantype == chan_state->conf.channel_type)){
	   atbm_printk_err("sta csa no change\n");

	   return;
	}
	
	csa.mactime = timestamp;
#ifdef ATBM_USE_FASTLINK
	sdata->is_fast_associated = true;
	sdata->fast_chan_switch = csa.ie.new_ch_num;
#endif	   
	ieee80211_start_special_csa_work(sdata,&csa);
}
#endif
static void ieee80211_handle_pwr_constr(struct ieee80211_sub_if_data *sdata,
					u16 capab_info, u8 *pwr_constr_elem,
					u8 pwr_constr_elem_len)
{
	struct ieee80211_channel_state *chan_state = ieee80211_get_channel_state(sdata->local, sdata);

	if (!(capab_info & WLAN_CAPABILITY_SPECTRUM_MGMT))
		return;

	/* Power constraint IE length should be 1 octet */
	if (pwr_constr_elem_len != 1)
		return;

	if ((*pwr_constr_elem <= chan_state->conf.channel->max_power) &&
	    (*pwr_constr_elem != sdata->local->power_constr_level)) {
		sdata->local->power_constr_level = *pwr_constr_elem;
		ieee80211_hw_config(sdata->local, 0);
	}
}
/* powersave */
static void ieee80211_enable_ps(struct ieee80211_local *local,
				struct ieee80211_sub_if_data *sdata)
{
	struct ieee80211_bss_conf *conf = &sdata->vif.bss_conf;

	/*
	 * If we are scanning right now then the parameters will
	 * take effect when scan finishes.
	 */
	if (local->scanning) /* XXX: investigate this codepath */
		return;
	conf->ps_enabled = true;
	ieee80211_bss_info_change_notify(sdata, BSS_CHANGED_PS);
}

static void ieee80211_change_ps(struct ieee80211_local *local)
{
	struct ieee80211_sub_if_data *sdata;

	/* XXX: This needs to be verified */
	list_for_each_entry(sdata, &local->interfaces, list) {
		atbm_printk_debug("%s:ps_allowed(%d)(%d)\n",sdata->name,sdata->ps_allowed,sdata->vif.bss_conf.ps_enabled);
		if (sdata->ps_allowed) {
			atbm_printk_debug("%s:ps enable\n",sdata->name);
			ieee80211_enable_ps(local, sdata);
		} else if (sdata->vif.bss_conf.ps_enabled) {
			sdata->vif.bss_conf.ps_enabled = false;
			atbm_printk_debug("%s:ps disable\n",sdata->name);
			ieee80211_bss_info_change_notify(sdata, BSS_CHANGED_PS);
		}
	}
}

static bool ieee80211_powersave_allowed(struct ieee80211_sub_if_data *sdata)
{
	struct ieee80211_if_managed *mgd = &sdata->u.mgd;
	struct sta_info *sta = NULL;
	bool authorized = false;
	
#ifdef CONFIG_ATBM_SMPS
	if (!mgd->powersave)
		return false;
#endif
	if (mgd->powersave_enable == false)
		return false;
	if (mgd->broken_ap)
		return false;

	if (!mgd->associated)
		return false;

	if (!mgd->associated->beacon_ies)
		return false;

	if (mgd->flags & (IEEE80211_STA_BEACON_POLL |
			  IEEE80211_STA_CONNECTION_POLL))
		return false;

#ifdef CONFIG_ATBM_STA_DYNAMIC_PS
	if(sdata->traffic.current_tx_tp + sdata->traffic.current_rx_tp >= IEEE80211_KEEP_WAKEUP_TP_PER_SECOND){
		atbm_printk_debug("%s:keep wakeup\n",__func__);
		return false;
	}
#endif

	rcu_read_lock();
	sta = sta_info_get(sdata, mgd->bssid);
	if (sta)
		authorized = test_sta_flag(sta, WLAN_STA_AUTHORIZED);
	rcu_read_unlock();

	return authorized;
}

/* need to hold RTNL or interface lock */
void ieee80211_recalc_ps_vif(struct ieee80211_local *local, s32 latency)
{
	struct ieee80211_sub_if_data *sdata;
	struct ieee80211_bss_conf *conf;
	int timeout;
	s32 beaconint_us;
	bool awake = false;
	
	list_for_each_entry(sdata, &local->interfaces, list) {
		if (!ieee80211_sdata_running(sdata))
			continue;
		if(sdata->vif.type == NL80211_IFTYPE_STATION)
			continue;

		awake = true;
	}
	
	list_for_each_entry(sdata, &local->interfaces, list) {
		if (!ieee80211_sdata_running(sdata))
			continue;
		
		if (sdata->vif.type != NL80211_IFTYPE_STATION)
			continue;
		
		if(awake == true){
			sdata->ps_allowed = false;
			continue;
		}
		
		if (!ieee80211_powersave_allowed(sdata)) {
			sdata->ps_allowed = false;
			continue;
		}

		conf = &sdata->vif.bss_conf;
		beaconint_us = ieee80211_tu_to_usec(
					sdata->vif.bss_conf.beacon_int);

		timeout = (latency > 0 && latency <beaconint_us) ? latency:beaconint_us;
		
		sdata->dynamic_ps_user_timeout = timeout;
		conf->dynamic_ps_timeout	   = timeout;
		
		atbm_printk_debug("%s:beaconint_us(%x)\n",__func__,beaconint_us);
		{
			struct ieee80211_bss *bss;
			int maxslp = 1;
			u8 dtimper;

			bss = (void *)sdata->u.mgd.associated->priv;
			dtimper = bss->dtim_period;

			/* If the TIM IE is invalid, pretend the value is 1 */
			if (!dtimper)
				dtimper = 1;
			else if (dtimper > 1)
				maxslp = min_t(int, dtimper,
							latency / beaconint_us);

			sdata->vif.bss_conf.max_sleep_period = maxslp;
			sdata->vif.bss_conf.ps_dtim_period = dtimper;
			sdata->ps_allowed = true;
		}
	}
	ieee80211_change_ps(local);
}
void ieee80211_recalc_ps(struct ieee80211_local *local, s32 latency)
{	
	ieee80211_recalc_ps_vif(local,latency);
}
#ifdef CONFIG_ATBM_SUPPORT_P2P
#define MAX_P2P_NOA_DESC 4
/* TODO: check if not defined already */
struct noa_desc
{
	u8 count;
	__le32 duration;
	__le32 interval;
	__le32 start;
} __packed;

struct noa_attr
{
	u8 index;
	u8 oppPsCTWindow;
	struct noa_desc dsc[MAX_P2P_NOA_DESC];
} __packed;

struct p2p_attr
{
	u8 type;
	__le16 len;
	u8 data[0];
} __packed;

static void ieee80211_sta_p2p_noa_check(struct ieee80211_local* local,
	struct ieee80211_sub_if_data* sdata,
	u8* p2p_ie, size_t p2p_ie_len)
{
	struct ieee80211_bss_conf* conf = &sdata->vif.bss_conf;
	struct cfg80211_p2p_ps p2p_ps = { 0 };
	struct p2p_attr* attr;
	struct noa_attr* noa_attr = NULL;
	u8* ptr;
	u16 len = 0, noa_len = 0;
	int i;
	size_t left;
	size_t elen;

	if (!p2p_ie)
		goto out;
	ptr = p2p_ie;
	left = p2p_ie_len;

	while (left >= 2) {

		elen = *++ptr;
		ptr++;
		left -= 2;
		if (elen > left)
			break;
		ptr += 4;

		/* Find Noa attr */
		for (i = 0; i < elen - 4;) {
			attr = (struct p2p_attr*)&ptr[i];
			len = __le32_to_cpu(attr->len);

			switch (attr->type) {
			case 0x0C:
				noa_attr = (struct noa_attr*)&attr->data[0];
				noa_len = len;
				break;
			default:
				break;
			}
			if (noa_attr)
				break;
			i = i + len + 3;
		}

		left -= elen;
		ptr += elen - 4;

		if (!noa_attr)
			/* parse next P2P IE if any left */
			continue;

		/* Get NOA settings */
		p2p_ps.opp_ps = !!(noa_attr->oppPsCTWindow & BIT(7));
		p2p_ps.ctwindow = (noa_attr->oppPsCTWindow & (~BIT(7)));

		if (noa_len >= (sizeof(struct noa_desc) + 2)) {
			/* currently FW API supports only one descriptor */
			p2p_ps.count = noa_attr->dsc[0].count;
			p2p_ps.start =
				__le32_to_cpu(noa_attr->dsc[0].start);
			p2p_ps.duration =
				__le32_to_cpu(noa_attr->dsc[0].duration);
			p2p_ps.interval =
				__le32_to_cpu(noa_attr->dsc[0].interval);
		}
		/* do not continue if one descriptor found */
		break;
	}

out:
	/* Notify driver if change is required */
	if (memcmp(&conf->p2p_ps, &p2p_ps, sizeof(p2p_ps))) {
		/* do not change legacy ps settings */
		p2p_ps.legacy_ps = conf->p2p_ps.legacy_ps;
		conf->p2p_ps = p2p_ps;
		atbm_printk_debug("ieee80211_sta_p2p_noa_check\n");
		if (local->hw.flags & IEEE80211_HW_SUPPORTS_P2P_PS)
			ieee80211_bss_info_change_notify(sdata,
				BSS_CHANGED_P2P_PS);
	}
}

/* P2P */
static void ieee80211_sta_p2p_params(struct ieee80211_local* local,
	struct ieee80211_sub_if_data* sdata,
	u8* p2p_ie, size_t p2p_ie_len)
{
	struct ieee80211_if_managed* ifmgd = &sdata->u.mgd;

	if (!p2p_ie) {
		if (ifmgd->p2p_last_ie_len) {
			memset(ifmgd->p2p_last_ie, 0x00,
				sizeof(ifmgd->p2p_last_ie));
			ifmgd->p2p_last_ie_len = 0;
			ieee80211_sta_p2p_noa_check(local, sdata, p2p_ie,
				p2p_ie_len);
			return;
		}
	}

	if (p2p_ie_len != ifmgd->p2p_last_ie_len ||
		memcmp(p2p_ie, ifmgd->p2p_last_ie, p2p_ie_len)) {
		/* BSS_CHANGED_P2P_PS */
		ieee80211_sta_p2p_noa_check(local, sdata, p2p_ie,
			p2p_ie_len);
	}

	memcpy(ifmgd->p2p_last_ie, p2p_ie, p2p_ie_len);
	ifmgd->p2p_last_ie_len = p2p_ie_len;
}
#endif
/* MLME */
static bool ieee80211_sta_wmm_params(struct ieee80211_local* local,
	struct ieee80211_sub_if_data* sdata,
	u8* wmm_param, size_t wmm_param_len,
	const struct atbm_ieee80211_mu_edca_param_set* mu_edca)
{
	struct atbm_ieee80211_tx_queue_params params[IEEE80211_NUM_ACS];
	struct ieee80211_if_managed* ifmgd = &sdata->u.mgd;
	size_t left;
	int count, mu_edca_count, ac;
	const u8* pos;
	u8 uapsd_queues = 0;

	if (!local->ops->conf_tx)
		return false;

	if (local->hw.queues < IEEE80211_NUM_ACS)
		return false;

	if (!wmm_param)
		return false;

	if (wmm_param_len < 8 || wmm_param[5] /* version */ != 1)
		return false;

	if ((ifmgd->flags & IEEE80211_STA_UAPSD_ENABLED) && uapsd)
		uapsd_queues = sdata->uapsd_queues;

	count = wmm_param[6] & 0x0f;
	/* -1 is the initial value of ifmgd->mu_edca_last_param_set.
	 * if mu_edca was preset before and now it disappeared tell
	 * the driver about it.
	 */
	mu_edca_count = mu_edca ? mu_edca->mu_qos_info & 0x0f : -1;
	if (count == ifmgd->wmm_last_param_set &&
		mu_edca_count == ifmgd->mu_edca_last_param_set)
		return false;
	ifmgd->wmm_last_param_set = count;

	ifmgd->mu_edca_last_param_set = mu_edca_count;

	pos = wmm_param + 8;
	left = wmm_param_len - 8;

	memset(&params, 0, sizeof(params));

	sdata->wmm_acm = 0;
	for (; left >= 4; left -= 4, pos += 4) {
		int aci = (pos[0] >> 5) & 0x03;
		int acm = (pos[0] >> 4) & 0x01;
		bool uapsd = false;
		// int queue;

		switch (aci) {
		case 1: /* AC_BK */
			// ac = IEEE80211_AC_BK;
			ac = 3;
			if (acm)
				sdata->wmm_acm |= BIT(1) | BIT(2); /* BK/- */
			if (uapsd_queues & IEEE80211_WMM_IE_STA_QOSINFO_AC_BK)
				uapsd = true;

			params[ac].mu_edca = !!mu_edca;
			if (mu_edca)
				params[ac].mu_edca_param_rec = mu_edca->ac_bk;
			break;
		case 2: /* AC_VI */
			// ac = IEEE80211_AC_VI;
			ac = 1;
			if (acm)
				sdata->wmm_acm |= BIT(4) | BIT(5); /* CL/VI */
			if (uapsd_queues & IEEE80211_WMM_IE_STA_QOSINFO_AC_VI)
				uapsd = true;

			params[ac].mu_edca = !!mu_edca;
			if (mu_edca)
				params[ac].mu_edca_param_rec = mu_edca->ac_vi;
			break;
		case 3: /* AC_VO */
			ac = 0;
			if (acm)
				sdata->wmm_acm |= BIT(6) | BIT(7); /* VO/NC */
			if (uapsd_queues & IEEE80211_WMM_IE_STA_QOSINFO_AC_VO)
				uapsd = true;

			params[ac].mu_edca = !!mu_edca;
			if (mu_edca)
				params[ac].mu_edca_param_rec = mu_edca->ac_vo;
			break;
		case 0: /* AC_BE */
		default:
			ac = 2;
			if (acm)
				sdata->wmm_acm |= BIT(0) | BIT(3); /* BE/EE */
			if (uapsd_queues & IEEE80211_WMM_IE_STA_QOSINFO_AC_BE)
				uapsd = true;

			params[ac].mu_edca = !!mu_edca;
			if (mu_edca)
				params[ac].mu_edca_param_rec = mu_edca->ac_be;
			break;
		}

		params[ac].aifs = pos[0] & 0x0f;

		if (params[ac].aifs < 2) {
			atbm_printk_warn("%s AP has invalid WMM params (AIFSN=%d for ACI %d), will use 2\n",
				(sdata)->name, params[ac].aifs, aci);
			params[ac].aifs = 2;
		}
		params[ac].cw_max = ecw2cw((pos[1] & 0xf0) >> 4);
		params[ac].cw_min = ecw2cw(pos[1] & 0x0f);
		params[ac].txop = get_unaligned_le16(pos + 2);
		params[ac].acm = acm;
		params[ac].uapsd = uapsd;

		if (params[ac].cw_min == 0 ||
			params[ac].cw_min > params[ac].cw_max) {
			atbm_printk_warn("%s AP has invalid WMM params (CWmin/max=%d/%d for ACI %d), using defaults\n",
				(sdata)->name, params[ac].cw_min, params[ac].cw_max, aci);
			return false;
		}
		// ieee80211_regulatory_limit_wmm_params(sdata, &params[ac], ac);
	}

	/* WMM specification requires all 4 ACIs. */
	for (ac = 0; ac < IEEE80211_NUM_ACS; ac++) {
		if (params[ac].cw_min == 0) {
			atbm_printk_warn("%s AP has invalid WMM params (missing AC %d), using defaults\n", (sdata)->name,
				ac);
			return false;
		}
	}

	for (ac = 0; ac < IEEE80211_NUM_ACS; ac++) {
		atbm_printk_err("%s  WMM AC=%d acm=%d aifs=%d cWmin=%d cWmax=%d txop=%d uapsd=%d\n", (sdata)->name,
			ac, params[ac].acm,
			params[ac].aifs, params[ac].cw_min, params[ac].cw_max,
			params[ac].txop, params[ac].uapsd);
		sdata->tx_conf[ac] = params[ac];
		if (/*!ifmgd->tx_tspec[ac].downgraded &&*/
			drv_conf_tx(local, sdata, ac, &params[ac]))
			atbm_printk_err("%s  failed to set TX queue parameters for AC %d\n", (sdata)->name,
				ac);
	}

	/* enable WMM or activate new settings */
	sdata->vif.bss_conf.qos = true;
	sdata->vif.bss_conf.uapsd = uapsd ? true:false;
	// ieee80211_bss_info_change_notify(sdata, BSS_CHANGED_QOS);

	return true;
}

static u32 ieee80211_handle_bss_capability(struct ieee80211_sub_if_data* sdata,
	u16 capab, bool erp_valid, u8 erp)
{
	struct ieee80211_channel_state* chan_state = ieee80211_get_channel_state(sdata->local, sdata);
	struct ieee80211_bss_conf* bss_conf = &sdata->vif.bss_conf;
	u32 changed = 0;
	bool use_protection;
	bool use_short_preamble;
	bool use_short_slot;

	if (erp_valid) {
		use_protection = (erp & WLAN_ERP_USE_PROTECTION) != 0;
		use_short_preamble = (erp & WLAN_ERP_BARKER_PREAMBLE) == 0;
	}
	else {
		use_protection = false;
		use_short_preamble = !!(capab & WLAN_CAPABILITY_SHORT_PREAMBLE);
	}

	use_short_slot = !!(capab & WLAN_CAPABILITY_SHORT_SLOT_TIME);
	if (chan_state->conf.channel->band == IEEE80211_BAND_5GHZ)
		use_short_slot = true;

	if (use_protection != bss_conf->use_cts_prot) {
		bss_conf->use_cts_prot = use_protection;
		changed |= BSS_CHANGED_ERP_CTS_PROT;
	}

	if (use_short_preamble != bss_conf->use_short_preamble) {
		bss_conf->use_short_preamble = use_short_preamble;
		changed |= BSS_CHANGED_ERP_PREAMBLE;
	}

	if (use_short_slot != bss_conf->use_short_slot) {
		bss_conf->use_short_slot = use_short_slot;
		changed |= BSS_CHANGED_ERP_SLOT;
	}

	return changed;
}
static void ieee80211_mgd_enable_netq(struct ieee80211_sub_if_data *sdata,size_t waitting_max,long sleep)
{
	u8 wait = 0;

	/*
	*500ms is long enough to wait the txq ready
	*/
	do{
		
		if(!qdisc_tx_is_noop(sdata->dev)){
			break;
		}

		if(wait >= waitting_max){
			break;
		}

		if(netif_carrier_ok(sdata->dev) == 0){
			break;
		}
		wait ++;
		atbm_printk_err("[%s] Waiting Txq Ready[%d]\n",sdata->name,wait);
		schedule_timeout_interruptible(msecs_to_jiffies(sleep));
	}while(1);
	
	synchronize_rcu();
}
static enum work_done_result ieee80211_netq_ready_done(struct ieee80211_work *wk,
						  struct sk_buff *skb)
{
	atbm_printk_err("netq done");
	if(IEEE80211_WORK_NETQ == wk->type)
		ieee80211_mgd_enable_netq(wk->sdata,10,50);
	return WORK_DONE_DESTROY;
}

bool ieee80211_wk_netq_ready(struct ieee80211_sub_if_data *sdata)
{
	struct ieee80211_work *wk;
	struct ieee80211_channel_state *chan_state = ieee80211_get_channel_state(sdata->local, sdata);

	if(!qdisc_tx_is_noop(sdata->dev)){
		return true;
	}
	
	wk = atbm_kzalloc(sizeof(struct ieee80211_work), GFP_KERNEL);

	if(wk){
		
		wk->type = IEEE80211_WORK_NETQ;
		wk->chan_type = chan_state->conf.channel_type;
		wk->chan      = chan_state->conf.channel;
		wk->chan_mode = CHAN_MODE_FIXED;
		wk->sdata     = sdata;
		wk->done      = ieee80211_netq_ready_done;
		wk->start     = NULL;
		wk->rx        = NULL;
		wk->filter_fc = 0;
		memcpy(wk->filter_bssid,sdata->u.mgd.bssid,6);
		memcpy(wk->filter_sa,sdata->u.mgd.bssid,6);
		
		ieee80211_add_work(wk);
		ieee80211_work_purge(sdata,sdata->u.mgd.bssid,IEEE80211_WORK_NETQ,true);
		
		if(!qdisc_tx_is_noop(sdata->dev)){
			return true;
		}

		return false;
	} 

	return true;	
}

static void ieee80211_set_associated(struct ieee80211_sub_if_data* sdata,
	struct cfg80211_bss* cbss,
	u32 bss_info_changed)
{
	struct ieee80211_bss* bss = (void*)cbss->priv;
	struct ieee80211_local* local = sdata->local;
	struct ieee80211_bss_conf* bss_conf = &sdata->vif.bss_conf;
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)) | (CONFIG_CPTCFG_CFG80211))
	const struct cfg80211_bss_ies* ies;
#endif
	bss_info_changed |= BSS_CHANGED_ASSOC;
	/* set timing information */
	bss_conf->beacon_int = cbss->beacon_interval;
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)) || (CONFIG_CPTCFG_CFG80211))
	rcu_read_lock();
	ies = rcu_dereference(cbss->ies);
	bss_conf->timestamp = ies->tsf;
	rcu_read_unlock();
#else
	bss_conf->timestamp = cbss->tsf;
#endif

	//atbm_wifi_status_set(1);

	bss_info_changed |= BSS_CHANGED_BEACON_INT;
	bss_info_changed |= ieee80211_handle_bss_capability(sdata, cbss->capability, bss->has_erp_value, bss->erp_value);

	sdata->u.mgd.associated = cbss;
	memcpy(sdata->u.mgd.bssid, cbss->bssid, ETH_ALEN);

	sdata->u.mgd.flags |= IEEE80211_STA_RESET_SIGNAL_AVE;

	/* just to be sure */
	sdata->u.mgd.flags &= ~(IEEE80211_STA_CONNECTION_POLL |
		IEEE80211_STA_BEACON_POLL);
	if (local->hw.flags & IEEE80211_HW_NEED_DTIM_PERIOD)
		bss_conf->dtim_period = bss->dtim_period;
	else
		bss_conf->dtim_period = 0;

	bss_conf->assoc = 1;
	bss_conf->uapsd = uapsd ? true:false;
	/*
	 * For now just always ask the driver to update the basic rateset
	 * when we have associated, we aren't checking whether it actually
	 * changed or not.
	 */
	bss_info_changed |= BSS_CHANGED_BASIC_RATES;

	/* And the BSSID changed - we're associated now */
	bss_info_changed |= BSS_CHANGED_BSSID;

	/* Tell the driver to monitor connection quality (if supported) */
	if ((local->hw.flags & IEEE80211_HW_SUPPORTS_CQM_RSSI) &&
		bss_conf->cqm_rssi_thold)
		bss_info_changed |= BSS_CHANGED_CQM;

	/* Enable ARP filtering */
	if (bss_conf->arp_filter_enabled != sdata->arp_filter_state) {
		bss_conf->arp_filter_enabled = sdata->arp_filter_state;
		bss_info_changed |= BSS_CHANGED_ARP_FILTER;
	}

#ifdef IPV6_FILTERING
	/* Enable NDP filtering */
	if (bss_conf->ndp_filter_enabled != sdata->ndp_filter_state) {
		bss_conf->ndp_filter_enabled = sdata->ndp_filter_state;
		bss_info_changed |= BSS_CHANGED_NDP_FILTER;
	}
#endif /*IPV6_FILTERING*/

	ieee80211_bss_info_change_notify(sdata, bss_info_changed);
    /*
     * recal dcm
     */
    ieee80211_recalc_dcm(local);

	mutex_lock(&local->iflist_mtx);
	ieee80211_recalc_ps(local, -1);
#ifdef CONFIG_ATBM_SMPS
	ieee80211_recalc_smps(local);
#endif
	mutex_unlock(&local->iflist_mtx);
	netif_carrier_on(sdata->dev);
	ieee80211_mgd_enable_netq(sdata,1,10);
	netif_tx_start_all_queues(sdata->dev);
	ieee80211_medium_traffic_start(sdata);
}

static void ieee80211_set_disassoc(struct ieee80211_sub_if_data* sdata,
	bool remove_sta, bool tx)
{
	struct ieee80211_if_managed* ifmgd = &sdata->u.mgd;
	struct ieee80211_local* local = sdata->local;
	struct sta_info* sta;
	u32 changed = 0, config_changed = 0;
	u8 bssid[ETH_ALEN];

	ASSERT_MGD_MTX(ifmgd);

	if (WARN_ON(!ifmgd->associated))
		return;

    atbm_del_timer_sync(&ifmgd->timer);

//	atbm_wifi_status_set(0);

	memcpy(bssid, ifmgd->associated->bssid, ETH_ALEN);
	
#ifdef CONFIG_ATBM_SUPPORT_CSA
	ieee80211_csa_cancel(sdata);
#endif

	ieee80211_work_purge(sdata,bssid,IEEE80211_WORK_MAX,true);

    if(WARN_ON(ifmgd->conn.auth_req)){
        ieee80211_sdata_free_auth_data(sdata,ifmgd->conn.auth_req);
    }
    if(WARN_ON(ifmgd->conn.assoc_req)){
        ieee80211_sdata_free_assoc_data(sdata,ifmgd->conn.assoc_req);
    }

    ieee80211_conn_mark_idle(sdata);
	ifmgd->associated = NULL;
	ifmgd->b_notransmit_bssid = false;

	synchronize_rcu();

	memset(ifmgd->bssid, 0, ETH_ALEN);

	/*
	 * we need to commit the associated = NULL change because the
	 * scan code uses that to determine whether this iface should
	 * go to/wake up from powersave or not -- and could otherwise
	 * wake the queues erroneously.
	 */
	smp_mb();

	/*
	 * Thus, we can only afterwards stop the queues -- to account
	 * for the case where another CPU is finishing a scan at this
	 * time -- we don't want the scan code to enable queues.
	 */

	netif_tx_stop_all_queues(sdata->dev);
	netif_carrier_off(sdata->dev);
#ifdef CONFIG_ATBM_SUPPORT_REKEY
	ieee80211_rekey_set_data_disable(sdata);
#endif
	ieee80211_medium_traffic_concle(sdata);
	mutex_lock(&local->sta_mtx);
	sta = sta_info_get(sdata, bssid);
	if (sta) {
		set_sta_flag(sta, WLAN_STA_BLOCK_BA);
		ieee80211_sta_tear_down_BA_sessions(sta, tx);
		ieee80211_twt_sta_deinit(sta);
	}
	mutex_unlock(&local->sta_mtx);

	changed |= ieee80211_reset_erp_info(sdata);

	changed |= BSS_CHANGED_ASSOC;
	sdata->vif.bss_conf.assoc = false;

	ieee80211_set_wmm_default(sdata);

	/* channel(_type) changes are handled by ieee80211_hw_config */
	WARN_ON(!ieee80211_set_channel_type(local, sdata, NL80211_CHAN_NO_HT));

	/* on the next assoc, re-program HT parameters */
	sdata->ht_opmode_valid = false;

	local->power_constr_level = 0;
	if (sdata->vif.bss_conf.ps_enabled) {
		sdata->vif.bss_conf.ps_enabled = false;
		ieee80211_bss_info_change_notify(sdata, BSS_CHANGED_PS);
	}
	sdata->ps_allowed = false;

	ieee80211_hw_config(local, config_changed);
	/* Disable ARP filtering */
	if (sdata->vif.bss_conf.arp_filter_enabled) {
		sdata->vif.bss_conf.arp_filter_enabled = false;
		changed |= BSS_CHANGED_ARP_FILTER;
	}

#ifdef IPV6_FILTERING
	/* Disable NDP filtering */
	if (sdata->vif.bss_conf.ndp_filter_enabled) {
		sdata->vif.bss_conf.ndp_filter_enabled = false;
		changed |= BSS_CHANGED_NDP_FILTER;
	}
#endif /*IPV6_FILTERING*/

	/* The BSSID (not really interesting) and HT changed */
	changed |= BSS_CHANGED_BSSID | BSS_CHANGED_HT;
	ieee80211_bss_info_change_notify(sdata, changed);

	/* remove AP and TDLS peers */
	if (remove_sta)
		sta_info_flush(local, sdata);

    ieee80211_recalc_dcm(local);
#ifdef CONFIG_MAC80211_BRIDGE
	ieee80211_brigde_flush(sdata);
#endif // CONFIG_MAC80211_BRIDGE
}
void __ieee80211_connection_loss(struct ieee80211_sub_if_data* sdata,void *cookie)
{
	struct ieee80211_if_managed* ifmgd = &sdata->u.mgd;
	struct ieee80211_local* local = sdata->local;
	u8 bssid[ETH_ALEN];

    ieee80211_sdata_mgd_lock(sdata,cookie);
	if (!ifmgd->associated) {
        ieee80211_sdata_mgd_unlock(sdata,cookie);
		return;
	}

	memcpy(bssid, ifmgd->associated->bssid, ETH_ALEN);

	atbm_printk_always("<WARNING>%s:AP %pM lost.....\n",
		sdata->name, bssid);

	ieee80211_set_disassoc(sdata, false, true);
	/*
	 * must be outside lock due to cfg80211,
	 * but that's not a problem.
	 */
	ieee80211_send_deauth_disassoc(sdata, bssid,
		IEEE80211_STYPE_DEAUTH,
		WLAN_REASON_DISASSOC_DUE_TO_INACTIVITY,
		true);

	sta_info_flush(sdata->local, sdata);	

	ieee80211_work_purge(sdata,bssid,IEEE80211_WORK_MAX,false);

	mutex_lock(&local->mtx);	
	ieee80211_recalc_idle(local);
	mutex_unlock(&local->mtx);


    ieee80211_sdata_mgd_unlock(sdata,cookie);
}

void ieee80211_beacon_connection_loss_work(struct atbm_work_struct* work)
{
	struct ieee80211_sub_if_data* sdata =
		container_of(work, struct ieee80211_sub_if_data,
			u.mgd.beacon_connection_loss_work);

	if (sdata->local->adaptive_started == true) {
		atbm_printk_debug("adaptive_started! ieee80211_beacon_connection_loss_work stop");
		return;
	}

	__ieee80211_connection_loss(sdata,NULL);
}
void ieee80211_connection_loss(struct ieee80211_vif* vif)
{
	struct ieee80211_sub_if_data* sdata = vif_to_sdata(vif);
	struct ieee80211_hw* hw = &sdata->local->hw;

	trace_api_connection_loss(sdata);

	WARN_ON(!(hw->flags & IEEE80211_HW_CONNECTION_MONITOR));
	ieee80211_queue_work(hw, &sdata->u.mgd.beacon_connection_loss_work);
}
// EXPORT_SYMBOL(ieee80211_connection_loss);

static void ieee80211_rx_mgmt_deauth(struct ieee80211_sub_if_data* sdata,
	struct atbm_ieee80211_mgmt* mgmt, size_t len)
{
	struct ieee80211_if_managed* ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;
	const u8* bssid = NULL;
	u16 reason_code;
#ifdef CONFIG_ATBM_SUPPORT_P2P
    u16 pre_reason;
#endif
    bool abort = false;

	if (len < 24 + 2)
		return;

	ASSERT_MGD_MTX(ifmgd);

	reason_code = le16_to_cpu(mgmt->u.deauth.reason_code);
    atbm_printk_always("%s: deauthenticated from %pM (Reason: %u),conn_state[%s]\n",
			sdata->name, bssid, reason_code,ieee80211_conn_state_string(conn));

	if(ifmgd->associated&&(atbm_compare_ether_addr(ifmgd->associated->bssid, mgmt->bssid)==0)){
        WARN_ON(conn->state != IEEE80211_CONN_CONNECTED);
        WARN_ON(conn->work != NULL);
        WARN_ON(conn->auth_req);
        WARN_ON(conn->assoc_req);
        WARN_ON(conn->assoc_req);
		ieee80211_set_disassoc(sdata,true,false);
        goto exit;
	}

#ifdef CONFIG_ATBM_SUPPORT_P2P
	/*
	*some phone has a bug !!!!!!
	*when after p2p provisioning wps process,go must send a deauthen with reason
	*code 23 but some phone sends deauthen code 3.
	*/
	pre_reason = le16_to_cpu(mgmt->u.deauth.reason_code);
	if((sdata->vif.p2p == true)&&(pre_reason != 23)){
		mgmt->u.deauth.reason_code = cpu_to_le16(23);
		atbm_printk_debug( "%s(%s):IEEE80211_STYPE_DEAUTH,reason(%d),pre_reason(%d)\n",
		__func__,sdata->name,le16_to_cpu(mgmt->u.deauth.reason_code),pre_reason);
	}
#endif

    if(conn->auth_req &&(atbm_compare_ether_addr(conn->auth_req->bssid, mgmt->bssid)==0)){
        atbm_printk_err("%s:auth req abort\n",__func__);
        ieee80211_sdata_free_auth_data(sdata,conn->auth_req);
        abort = true;
    }

    if(conn->assoc_req && (atbm_compare_ether_addr(conn->assoc_req->bssid, mgmt->bssid)==0)){
        atbm_printk_err("%s:assoc req abort\n",__func__);
        ieee80211_sdata_free_assoc_data(sdata,conn->assoc_req);
        abort = true;
    }

    if(abort == true){
        sta_info_destroy_addr(sdata,mgmt->bssid);
	    ieee80211_work_purge(sdata,mgmt->bssid,IEEE80211_WORK_MAX,true);
        ieee80211_conn_mark_idle(sdata);
    }
exit:	
    ieee80211_mgd_deauth_rx(sdata,mgmt,len);

	mutex_lock(&sdata->local->mtx);
	ieee80211_recalc_idle(sdata->local);
	mutex_unlock(&sdata->local->mtx);
}

static void ieee80211_rx_mgmt_auth(struct ieee80211_sub_if_data *sdata,
        struct atbm_ieee80211_mgmt *mgmt,size_t len)
{
    struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;
    struct ieee80211_mgd_auth_req *auth_req = conn->auth_req;
	u16 auth_alg, auth_transaction, status_code;

    if(len < 24 + 6){
        goto exit;
    }

    if(authen_fail){
        goto exit;
    }
    if(auth_req == NULL){
        atbm_printk_err("%s:authen idle\n",__func__);
        goto exit;
    }

    atbm_printk_always("%s:conn state[%s][%d]\n",__func__,ieee80211_conn_state_string(conn),auth_req->done);
#if 0
    if(conn->state != IEEE80211_CONN_AUTHENTICATING){
        goto exit;
    }
#endif

    if(memcmp(auth_req->bssid,mgmt->bssid,ETH_ALEN)){
        atbm_printk_err("%s:bssid mismatch[%pM],[%pM]\n",__func__,auth_req->bssid,mgmt->bssid);
        goto exit;
    }

    if(auth_req->done){
        atbm_printk_err("%s:auth has been done ,ignor the comming auth\n",sdata->name);
        goto exit;
    }

    auth_alg = le16_to_cpu(mgmt->u.auth.auth_alg);
	auth_transaction = le16_to_cpu(mgmt->u.auth.auth_transaction);
	status_code = le16_to_cpu(mgmt->u.auth.status_code);

	if(auth_alg != auth_req->algorithm){
		atbm_printk_mgmt("%s:[%pM] auth_alg mismatch(%d)(%d)\n",sdata->name,mgmt->sa,auth_alg,auth_req->algorithm);
        goto exit;
	}

    if((auth_alg != ATBM_WLAN_AUTH_SAE && auth_transaction != auth_req->transaction) ||
       (auth_alg == ATBM_WLAN_AUTH_SAE && (auth_transaction < auth_req->transaction || auth_transaction > 2))
      ){
        atbm_printk_err("%s:[%pM] authen transaction err[%d][%d]\n",sdata->name,mgmt->bssid,auth_transaction,auth_req->transaction);
        goto exit;
    }

	switch (auth_req->algorithm) {
	case ATBM_WLAN_AUTH_OPEN:
	case ATBM_WLAN_AUTH_LEAP:
	case ATBM_WLAN_AUTH_FT:
	case ATBM_WLAN_AUTH_SHARED_KEY:
		if (status_code != WLAN_STATUS_SUCCESS) {
			atbm_printk_err("%s: %pM denied authentication (status %d)\n",sdata->name, mgmt->sa, status_code);
#ifdef CONFIG_MAC80211_ATBM_ROAMING_CHANGES
			sdata->queues_locked = 0;
#endif
			if(status_code == WLAN_STATUS_NOT_SUPPORTED_AUTH_ALG){
				atbm_printk_err("%s:auhen log err(%d)(%d)\n",__func__,auth_alg,auth_req->algorithm);
				/*
				*triger wpa_supplicant to retransmit an authen frame with the rignt algorithm
				*/
				mgmt->u.auth.auth_alg = cpu_to_le16(auth_req->algorithm);
			}
            auth_req->done = 1;
            goto done;
		}
		if (auth_req->transaction != 4 && auth_req->algorithm == ATBM_WLAN_AUTH_SHARED_KEY) {
			ieee80211_auth_challenge(sdata, mgmt, len);
			/* need another frame */
            goto exit;
		}
        auth_req->done = 1;
        auth_req->waiting = 1;
        goto recv;
#ifdef CONFIG_ATBM_SUPPORT_SAE
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 8, 0)) || defined(CONFIG_CFG80211_COMPAT_MODIFIED)
	case ATBM_WLAN_AUTH_SAE:
		if (status_code != WLAN_STATUS_SUCCESS) {
			if (auth_alg == ATBM_WLAN_AUTH_SAE &&
			    (status_code == ATBM_WLAN_STATUS_ANTI_CLOG_REQUIRED ||
			     (auth_transaction == 1 &&
			      (status_code == ATBM_WLAN_STATUS_SAE_HASH_TO_ELEMENT ||
			       status_code == ATBM_WLAN_STATUS_SAE_PK)))) {
			    atbm_printk_err("%pM sae authentication (status %d)\n",mgmt->sa, status_code);
			}else {
				
                auth_req->done = 1;

				atbm_printk_err("%pM denied authentication (status %d)\n",mgmt->sa, status_code);
                goto done;
			}
		}
		
		if(ieee80211_sae_authen_state(sdata,ieee80211_sae_trans_state(auth_transaction,false)) != SAE_CONTINUE){
            goto exit;
		}

        if(ifmgd->sae_state == IEEE80211_SAE_ACCEPTED){
            auth_req->done = 1;
        }

        auth_req->waiting = 1;
        goto recv;
#endif
#endif
	default:
		WARN_ON(1);
		return;
	}
exit:
   atbm_printk_always("%s: authenticated\n",sdata->name);
   return;
recv:
    ieee80211_mgd_auth_rx_authen(sdata,mgmt,len);
    goto exit;
done:
	ieee80211_work_purge(sdata,auth_req->bssid,IEEE80211_WORK_CHAN_ASYNC,true);
   	sta_info_destroy_addr(sdata,auth_req->bssid);
    ieee80211_sdata_free_auth_data(sdata,auth_req);
    ieee80211_conn_mark_idle(sdata);
    goto recv;
}
static void ieee80211_rx_mgmt_assoc_resp(struct ieee80211_sub_if_data *sdata,
        struct atbm_ieee80211_mgmt *mgmt,size_t len,bool reassoc)
{
    struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;
    struct ieee80211_mgd_assoc_req *assoc_req = conn->assoc_req;
#ifdef ATBM_USE_FASTLINK
	struct ieee80211_local *local = sdata->local;
#endif
	u16 capab_info, status_code, aid;
	struct ieee802_atbm_11_elems elems;
	u8 *pos;
    struct cfg80211_bss *bss;
	/*
	 * AssocResp and ReassocResp have identical structure, so process both
	 * of them in this function.
	 */

    atbm_printk_always("%s:conn state[%s]\n",__func__,ieee80211_conn_state_string(conn));

	if (len < 24 + 6)
        return;
	if (assoc_fail)
        return; 

    if(conn->assoc_req == NULL){
        atbm_printk_err("%s:assoc req is null \n",__func__);
        return;
    }
    if(conn->state != IEEE80211_CONN_ASSOCIATING){
        return;
    }

    if(memcmp(conn->bss->bssid,mgmt->bssid,ETH_ALEN)){
        return;
    }

    capab_info  = le16_to_cpu(mgmt->u.assoc_resp.capab_info);
	status_code = le16_to_cpu(mgmt->u.assoc_resp.status_code);
	aid = le16_to_cpu(mgmt->u.assoc_resp.aid);

	atbm_printk_mgmt("%s: RX %sssocResp from %pM (capab=0x%x "
	       "status=%d aid=%d)\n",
	       sdata->name, reassoc ? "Rea" : "A", mgmt->sa,
	       capab_info, status_code, (u16)(aid & ~(BIT(15) | BIT(14))));

	pos = mgmt->u.assoc_resp.variable;
	ieee802_11_parse_elems(pos, len - (pos - (u8 *) mgmt), false, &elems, mgmt->bssid, NULL);

	if (status_code == WLAN_STATUS_ASSOC_REJECTED_TEMPORARILY &&
	    elems.timeout_int && elems.timeout_int_len == 5 &&
	    elems.timeout_int[0] == WLAN_TIMEOUT_ASSOC_COMEBACK) {
		u32 tu, ms;
		tu = get_unaligned_le32(elems.timeout_int + 1);
		ms = tu * 1024 / 1000;
		atbm_printk_err( "%s: %pM rejected association temporarily; "
		       "comeback duration %u TU (%u ms)\n",
		       sdata->name, mgmt->sa, tu, ms);

		ifmgd->timeout = jiffies + msecs_to_jiffies(ms);
	    conn->state = IEEE80211_CONN_ASSOCIATE_NEXT;	
        run_again(ifmgd,ifmgd->timeout);
        return;
	}

//#ifdef ATBM_USE_FASTLINK
//	local->is_associated = true;
//	memcpy(local->associated_mac,mgmt->sa, ETH_ALEN);
//#endif
#ifdef CONFIG_MAC80211_ATBM_ROAMING_CHANGES
	sdata->queues_locked = 0;
#endif
    bss = assoc_req->bss; 

	if (status_code != WLAN_STATUS_SUCCESS){
     	sta_info_destroy_addr(sdata,conn->assoc_req->bssid);
        ieee80211_conn_mark_idle(sdata);
		atbm_printk_err( "%s: %pM denied association (code=%d)\n",sdata->name, mgmt->sa, status_code);
    }else if(ieee80211_assoc_success(sdata,mgmt,len) == true){
		atbm_printk_always( "%s: associated\n", sdata->name);
        conn->state = IEEE80211_CONN_CONNECTED;
    }else {
     	sta_info_destroy_addr(sdata,conn->assoc_req->bssid);
        ieee80211_conn_mark_idle(sdata);
    }
    
	ieee80211_work_purge(sdata,bss->bssid,IEEE80211_WORK_CHAN_ASYNC,true);

    if(WARN_ON(conn->work != NULL)){
        conn->work = NULL;
    }

	mutex_lock(&sdata->local->mtx);
	ieee80211_recalc_idle(sdata->local);
	mutex_unlock(&sdata->local->mtx);

    ieee80211_sdata_free_assoc_data(sdata,conn->assoc_req);

    ieee80211_mgd_assoc_rx_resp(sdata,bss,mgmt,len);

#ifdef ATBM_USE_FASTLINK
		if (status_code == WLAN_STATUS_SUCCESS) {
			memcpy(sdata->associated_mac,mgmt->sa, ETH_ALEN);
			sdata->is_fast_associated = true;
		}
#endif

    atbm_printk_always("%s:rx assoc resp\n",__func__);

}
static void ieee80211_rx_mgmt_disassoc(struct ieee80211_sub_if_data* sdata,
	struct atbm_ieee80211_mgmt* mgmt, size_t len)
{
	struct ieee80211_if_managed* ifmgd = &sdata->u.mgd;
	u16 reason_code;

	if (len < 24 + 2)
		return;

	ASSERT_MGD_MTX(ifmgd);

	if (WARN_ON(!ifmgd->associated))
		return;

	if (WARN_ON(memcmp(ifmgd->associated->bssid, mgmt->sa, ETH_ALEN)))
		return;

	reason_code = le16_to_cpu(mgmt->u.disassoc.reason_code);

	atbm_printk_always("%s: disassociated from %pM (Reason: %u),conn_state[%s]\n",
		sdata->name, mgmt->sa, reason_code,ieee80211_conn_state_string(&ifmgd->conn));

	ieee80211_set_disassoc(sdata, true, false);

    ieee80211_mgd_disassoc_rx(sdata,mgmt,len);

	mutex_lock(&sdata->local->mtx);
	ieee80211_recalc_idle(sdata->local);
	mutex_unlock(&sdata->local->mtx);
}
#ifdef CONFIG_ATBM_HE
static bool ieee80211_twt_req_supported(const struct sta_info* sta,
	const struct ieee802_atbm_11_elems* elems)
{
	if (elems->ext_capab_len < 10)
		return false;

	if (!(elems->ext_capab[9] & ATBM_WLAN_EXT_CAPA10_TWT_RESPONDER_SUPPORT))
		return false;

	return sta->sta.he_cap.he_cap_elem.mac_cap_info[0] &
		ATBM_IEEE80211_HE_MAC_CAP0_TWT_RES;
}

static int ieee80211_recalc_twt_req(struct ieee80211_sub_if_data* sdata,
	struct sta_info* sta,
	struct ieee802_atbm_11_elems* elems)
{
	bool twt = ieee80211_twt_req_supported(sta, elems);

	if (sdata->vif.bss_conf.twt_requester != twt) {
		sdata->vif.bss_conf.twt_requester = twt;
		return BSS_CHANGED_TWT;
	}
	return 0;
}

static bool ieee80211_twt_bcast_support(struct ieee80211_sub_if_data* sdata,
	struct ieee80211_bss_conf* bss_conf,
	struct ieee80211_supported_band* sband,
	struct sta_info* sta)
{
	const struct atbm_ieee80211_sta_he_cap* own_he_cap =
		atbm_ieee80211_get_he_iftype_cap(&sdata->local->hw,sband,
			ieee80211_vif_type_p2p(&sdata->vif),sdata);
	return bss_conf->he_support &&
		(sta->sta.he_cap.he_cap_elem.mac_cap_info[2] &
			ATBM_IEEE80211_HE_MAC_CAP2_BCAST_TWT) &&
		own_he_cap &&
		(own_he_cap->he_cap_elem.mac_cap_info[2] &
			ATBM_IEEE80211_HE_MAC_CAP2_BCAST_TWT);
}
#endif //#ifdef CONFIG_ATBM_HE

const struct atbm_element*
atbm_cfg80211_find_elem_match(u8 eid, const u8* ies, unsigned int len,
	const u8* match, unsigned int match_len,
	unsigned int match_offset)
{
	const struct atbm_element* elem;

	atbm_for_each_element_id(elem, eid, ies, len)
	{
		if (elem->datalen >= match_offset + match_len &&
			!memcmp(elem->data + match_offset, match, match_len))
			return elem;
	}

	return NULL;
}

/**
 * cfg80211_find_ie_match - match information element and byte array in data
 *
 * @eid: element ID
 * @ies: data consisting of IEs
 * @len: length of data
 * @match: byte array to match
 * @match_len: number of bytes in the match array
 * @match_offset: offset in the IE where the byte array should match.
 *	If match_len is zero, this must also be set to zero.
 *	Otherwise this must be set to 2 or more, because the first
 *	byte is the element id, which is already compared to eid, and
 *	the second byte is the IE length.
 *
 * Return: %NULL if the element ID could not be found or if
 * the element is invalid (claims to be longer than the given
 * data) or if the byte array doesn't match, or a pointer to the first
 * byte of the requested element, that is the byte containing the
 * element ID.
 *
 * Note: There are no checks on the element length other than
 * having to fit into the given data and being large enough for the
 * byte array to match.
 */
static inline const u8*
atbm_cfg80211_find_ie_match(u8 eid, const u8* ies, unsigned int len,
	const u8* match, unsigned int match_len,
	unsigned int match_offset)
{
	/* match_offset can't be smaller than 2, unless match_len is
	 * zero, in which case match_offset must be zero as well.
	 */
	if (WARN_ON((match_len && match_offset < 2) ||
		(!match_len && match_offset)))
		return NULL;

	return (void*)atbm_cfg80211_find_elem_match(eid, ies, len,
		match, match_len,
		match_offset ? match_offset - 2 : 0);
}

/**
 * cfg80211_find_ext_ie - find information element with EID Extension in data
 *
 * @ext_eid: element ID Extension
 * @ies: data consisting of IEs
 * @len: length of data
 *
 * Return: %NULL if the extended element ID could not be found or if
 * the element is invalid (claims to be longer than the given
 * data), or a pointer to the first byte of the requested
 * element, that is the byte containing the element ID.
 *
 * Note: There are no checks on the element length other than
 * having to fit into the given data.
 */
static inline const u8* atbm_cfg80211_find_ext_ie(u8 ext_eid, const u8* ies, int len)
{
	return atbm_cfg80211_find_ie_match(ATBM_WLAN_EID_EXTENSION, ies, len, &ext_eid, 1, 2);
}
static bool ieee80211_assoc_success(struct ieee80211_sub_if_data *sdata,
	struct atbm_ieee80211_mgmt* mgmt, size_t len)
{
	struct ieee80211_if_managed* ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;
    struct ieee80211_mgd_assoc_req *assoc_req = conn->assoc_req;
	struct ieee80211_local* local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct ieee80211_channel_state* chan_state = ieee80211_get_channel_state(local, sdata);
	struct ieee80211_supported_band* sband;
	struct sta_info* sta;
	struct cfg80211_bss* cbss = assoc_req->bss;
	u8* pos;
	u32 rates, basic_rates;
	u16 capab_info, aid;
	struct ieee802_atbm_11_elems elems;
	struct ieee80211_bss_conf* bss_conf = &sdata->vif.bss_conf;
	u32 changed = 0;
	int i, j, err;
	bool have_higher_than_11mbit = false;
	u16 ap_ht_cap_flags;
	int min_rate = INT_MAX, min_rate_index = -1;

	/* AssocResp and ReassocResp have identical structure */


	pos = mgmt->u.assoc_resp.variable;
	aid = le16_to_cpu(mgmt->u.assoc_resp.aid);
	capab_info = le16_to_cpu(mgmt->u.assoc_resp.capab_info);

	ieee802_11_parse_elems(pos, len - (pos - (u8*)mgmt), false, &elems, mgmt->bssid, cbss->bssid);

#ifdef CONFIG_ATBM_HE
	if (elems.aid_resp)
		aid = le16_to_cpu(elems.aid_resp->aid);
#endif // CONFIG_ATBM_HE
	/*
	 * The 5 MSB of the AID field are reserved
	 * (802.11-2016 9.4.1.8 AID field)
	 */
	aid &= 0x7ff;

	ifmgd->broken_ap = false;

	if (aid == 0 || aid > IEEE80211_MAX_AID) {
		atbm_printk_err("%s: invalid AID value %d\n", sdata->name, aid);
		aid = 0;
		ifmgd->broken_ap = true;
	}

	if (!elems.supp_rates) {
		atbm_printk_err("%s: no SuppRates element in AssocResp\n", sdata->name);
	//	return false;
	}

	ifmgd->aid = aid;

	mutex_lock(&sdata->local->sta_mtx);
	/*
	 * station info was already allocated and inserted before
	 * the association and should be available to us
	 */
	sta = sta_info_get_rx(sdata, cbss->bssid);
	if (WARN_ON(!sta)) {
		mutex_unlock(&sdata->local->sta_mtx);
		return false;
	}

	set_sta_flag(sta, WLAN_STA_AUTH);
	set_sta_flag(sta, WLAN_STA_ASSOC);
	set_sta_flag(sta, WLAN_STA_ASSOC_AP);
	if (!(ifmgd->flags & IEEE80211_STA_CONTROL_PORT))
		set_sta_flag(sta, WLAN_STA_AUTHORIZED);

	rates = 0;
	basic_rates = 0;
	sband = local->hw.wiphy->bands[assoc_req->chan->band];

	for (i = 0; i < elems.supp_rates_len; i++) {
		int rate = (elems.supp_rates[i] & 0x7f) * 5;
		bool is_basic = !!(elems.supp_rates[i] & 0x80);

		if (rate > 110)
			have_higher_than_11mbit = true;

		for (j = 0; j < sband->n_bitrates; j++) {
			if(ieee80211_rate_unmap(&sband->bitrates[j]))
					continue;
			if (sband->bitrates[j].bitrate == rate) {
				rates |= BIT(j);
				if (is_basic)
					basic_rates |= BIT(j);
				if (rate < min_rate) {
					min_rate = rate;
					min_rate_index = j;
				}
				break;
			}
		}
	}

	for (i = 0; i < elems.ext_supp_rates_len; i++) {
		int rate = (elems.ext_supp_rates[i] & 0x7f) * 5;
		bool is_basic = !!(elems.ext_supp_rates[i] & 0x80);

		if (rate > 110)
			have_higher_than_11mbit = true;

		for (j = 0; j < sband->n_bitrates; j++) {
			if(ieee80211_rate_unmap(&sband->bitrates[j]))
					continue;
			if (sband->bitrates[j].bitrate == rate) {
				rates |= BIT(j);
				if (is_basic)
					basic_rates |= BIT(j);
				if (rate < min_rate) {
					min_rate = rate;
					min_rate_index = j;
				}
				break;
			}
		}
	}

	/*
	 * some buggy APs don't advertise basic_rates. use the lowest
	 * supported rate instead.
	 */
	if (unlikely(!basic_rates) && min_rate_index >= 0) {
		atbm_printk_err("%s: No basic rates in AssocResp. Using min supported rate instead.\n", sdata->name);
		basic_rates = BIT(min_rate_index);
	}
	sta->sta.supp_rates[assoc_req->chan->band] = rates;
    atbm_printk_err("%s:supp_rates[%x][%x]\n",__func__,rates,sta->sta.supp_rates[assoc_req->chan->band]);
	sdata->vif.bss_conf.basic_rates = basic_rates;
	sdata->vif.csa_active = false;

	/* cf. IEEE 802.11 9.2.12 */
	if (assoc_req->chan->band == IEEE80211_BAND_2GHZ &&
		have_higher_than_11mbit)
		sdata->flags |= IEEE80211_SDATA_OPERATING_GMODE;
	else
		sdata->flags &= ~IEEE80211_SDATA_OPERATING_GMODE;
	// printk("%s:sssht_cap_elem(%p),ifmgd->flags(%x),time(%ld) ms\n",__func__,elems.ht_cap_elem,ifmgd->flags & IEEE80211_STA_DISABLE_11N,jiffies_to_msecs(EndTime-StartTime));
	if (elems.ht_cap_elem && !(ifmgd->flags & IEEE80211_STA_DISABLE_11N))
		ieee80211_ht_cap_ie_to_sta_ht_cap(sband,
			elems.ht_cap_elem, &sta->sta.ht_cap);
#ifdef CONFIG_ATBM_VHT
	if((hw_priv != NULL) && (hw_priv->chip_version != OCEANUS_NO_WIFI6))
	{

	    if (elems.vht_cap_elem && !(ifmgd->flags & IEEE80211_STA_DISABLE_VHT)){
	        ieee80211_vht_cap_ie_to_sta_vht_cap(sdata,sband,elems.vht_cap_elem,sta);
	    }

	}
#endif
	// check this is wifi6 AP
	bss_conf->is_wifi6_ap = false;

	if (cbss->beacon_ies || cbss->proberesp_ies) {

		const u8* ht_cap_ie = NULL; //
#ifdef CONFIG_ATBM_HE
		const u8* he_cap_ie = NULL; // = cbss->beacon_ies?cbss->beacon_ies:cbss->proberesp_ies;

		// atbm_printk_err( "is_wifi6_ap++ beacon_ies %x\n",ies->len);
		// frame_hexdump("beaconie:\n",cbss->beacon_ies->data,int len);
#if (LINUX_VERSION_IS_LESS_AND_NOT_CPTCFG(3, 10, 0))
		if (cbss->len_proberesp_ies)
			he_cap_ie = atbm_cfg80211_find_ext_ie(ATBM_WLAN_EID_EXT_HE_CAPABILITY, cbss->proberesp_ies, cbss->len_proberesp_ies);
		else
			he_cap_ie = atbm_cfg80211_find_ext_ie(ATBM_WLAN_EID_EXT_HE_CAPABILITY, cbss->beacon_ies, cbss->len_beacon_ies);
#else //(LINUX_VERSION_IS_LESS_AND_NOT_CPTCFG(3,10,0))

		if (cbss->proberesp_ies && cbss->proberesp_ies->len)
			he_cap_ie = atbm_cfg80211_find_ext_ie(ATBM_WLAN_EID_EXT_HE_CAPABILITY, cbss->proberesp_ies->data, cbss->proberesp_ies->len);
		else if (cbss->beacon_ies && cbss->beacon_ies->len)
			he_cap_ie = atbm_cfg80211_find_ext_ie(ATBM_WLAN_EID_EXT_HE_CAPABILITY, cbss->beacon_ies->data, cbss->beacon_ies->len);

#endif //(LINUX_VERSION_IS_LESS_AND_NOT_CPTCFG(3,10,0))

		if (he_cap_ie) {
			bss_conf->is_wifi6_ap = true;
			atbm_printk_err("is_wifi6_ap++\n");
		}
		
#endif // CONFIG_ATBM_HE
		// add by wp 2022/7/6
		if (!elems.ht_cap_elem && !(ifmgd->flags & IEEE80211_STA_DISABLE_11N)) {
			// for cisco ap , assoc rsp frame not have ht_cap_elem,so we need to check in beacon or probe rsp
#if (LINUX_VERSION_IS_LESS_AND_NOT_CPTCFG(3, 10, 0))
			if (cbss->len_proberesp_ies)
				ht_cap_ie = cfg80211_find_ie(ATBM_WLAN_EID_HT_CAPABILITY, cbss->proberesp_ies, cbss->len_proberesp_ies);
			else
				ht_cap_ie = cfg80211_find_ie(ATBM_WLAN_EID_HT_CAPABILITY, cbss->beacon_ies, cbss->len_beacon_ies);
#else  //(LINUX_VERSION_IS_LESS_AND_NOT_CPTCFG(3,10,0))

			if (cbss->proberesp_ies && cbss->proberesp_ies->len)
				ht_cap_ie = cfg80211_find_ie(ATBM_WLAN_EID_HT_CAPABILITY, cbss->proberesp_ies->data, cbss->proberesp_ies->len);
			else if (cbss->beacon_ies && cbss->beacon_ies->len)
				ht_cap_ie = cfg80211_find_ie(ATBM_WLAN_EID_HT_CAPABILITY, cbss->beacon_ies->data, cbss->beacon_ies->len);
#endif //(LINUX_VERSION_IS_LESS_AND_NOT_CPTCFG(3,10,0))
			if (ht_cap_ie) {
				ieee80211_ht_cap_ie_to_sta_ht_cap(sband, (struct ieee80211_ht_cap *)ht_cap_ie, &sta->sta.ht_cap);
			}
		}
	}
	else {
		atbm_printk_err("is_wifi6_ap++ beacon_ies == NULL\n");
	}

#ifdef CONFIG_ATBM_HE
	if (elems.he_operation && !(ifmgd->flags & IEEE80211_STA_DISABLE_HE) &&
		elems.he_cap) {
		bss_conf->is_wifi6_ap = true;
		ieee80211_he_cap_ie_to_sta_he_cap(sdata, sband,
			elems.he_cap,
			elems.he_cap_len,
			elems.he_6ghz_capa,
			sta);

		bss_conf->he_support = sta->sta.he_cap.has_he;
#ifdef WLAN_RSNX_CAPA_PROTECTED_TWT
		if (elems.rsnx && elems.rsnx_len &&
			(elems.rsnx[0] & WLAN_RSNX_CAPA_PROTECTED_TWT) &&
			wiphy_ext_feature_isset(local->hw.wiphy,
				NL80211_EXT_FEATURE_PROTECTED_TWT))
			bss_conf->twt_protected = true;
		else
			bss_conf->twt_protected = false;
#endif // WLAN_RSNX_CAPA_PROTECTED_TWT
		changed |= ieee80211_recalc_twt_req(sdata, sta, &elems);
	}
	else {
		bss_conf->he_support = false;
		bss_conf->twt_requester = false;
		bss_conf->twt_protected = false;
	}

	bss_conf->twt_broadcast =
		ieee80211_twt_bcast_support(sdata, bss_conf, sband, sta);

	if (bss_conf->he_support) {
		bss_conf->he_bss_color.color =
			le32_get_bits(elems.he_operation->he_oper_params,
				ATBM_IEEE80211_HE_OPERATION_BSS_COLOR_MASK);
		bss_conf->he_bss_color.partial =
			le32_get_bits(elems.he_operation->he_oper_params,
				ATBM_IEEE80211_HE_OPERATION_PARTIAL_BSS_COLOR);
		bss_conf->he_bss_color.enabled =
			!le32_get_bits(elems.he_operation->he_oper_params,
				ATBM_IEEE80211_HE_OPERATION_BSS_COLOR_DISABLED);

		if (bss_conf->he_bss_color.enabled)
			changed |= BSS_CHANGED_HE_BSS_COLOR;

		bss_conf->htc_trig_based_pkt_ext =
			le32_get_bits(elems.he_operation->he_oper_params,
				ATBM_IEEE80211_HE_OPERATION_DFLT_PE_DURATION_MASK);
		bss_conf->frame_time_rts_th =
			le32_get_bits(elems.he_operation->he_oper_params,
				ATBM_IEEE80211_HE_OPERATION_RTS_THRESHOLD_MASK);

		bss_conf->uora_exists = !!elems.uora_element;
		if (elems.uora_element)
			bss_conf->uora_ocw_range = elems.uora_element[0];

		ieee80211_he_op_ie_to_bss_conf(&sdata->vif, elems.he_operation);
		ieee80211_he_spr_ie_to_bss_conf(&sdata->vif, elems.he_spr);

      sta->sta.he_cap.he_operation.he_oper_params = __le32_to_cpu(elems.he_operation->he_oper_params);  
		
		/* TODO: OPEN: what happens if BSS color disable is set? */
	}
	if (atbm_iee80211_bss_is_nontrans(cbss)) {
		bss_conf->nontransmitted = true;
		memcpy(bss_conf->transmitter_bssid, atbm_iee80211_bss_transmit_bssid(cbss), 6);
		bss_conf->bssid_indicator = atbm_iee80211_max_bssid_indicator(cbss);
		bss_conf->bssid_index = atbm_iee80211_bssid_index(cbss);
	}
#endif //#ifdef CONFIG_ATBM_HE
#ifdef CONFIG_SUPPORT_BSS_MAX_IDLE
	if (elems.max_idle_period_ie) {
		bss_conf->max_idle_period = le16_to_cpu(elems.max_idle_period_ie->max_idle_period);
		bss_conf->protected_keep_alive =!!(elems.max_idle_period_ie->idle_options & BIT(0));
	} else {
		bss_conf->max_idle_period = 0;
		bss_conf->protected_keep_alive = false;
	}
#endif
	ap_ht_cap_flags = sta->sta.ht_cap.cap;
	rate_control_rate_init(sta);
    atbm_printk_err("%s:supp_rates[%x][%x]\n",__func__,rates,sta->sta.supp_rates[assoc_req->chan->band]);
	if (ifmgd->flags & IEEE80211_STA_MFP_ENABLED)
		set_sta_flag(sta, WLAN_STA_MFP);

	if (elems.wmm_param){
		set_sta_flag(sta, WLAN_STA_WME);
		sta->sta.wme = true;
	}
	ieee80211_mgd_he_om_work(sta);
	ieee80211_mgd_vht_action_work(sta);
	/* sta_info_reinsert will also unlock the mutex lock */
	err = sta_info_reinsert(sta);
	//sta = NULL;
	if (err) {
		atbm_printk_err("%s: failed to insert STA entry for"
			" the AP (error %d)\n", sdata->name, err);
		return false;
	}

	/*
	 * Always handle WMM once after association regardless
	 * of the first value the AP uses. Setting -1 here has
	 * that effect because the AP values is an unsigned
	 * 4-bit value.
	 */
	ifmgd->wmm_last_param_set = -1;
	ifmgd->mu_edca_last_param_set = -1;

	if (ifmgd->flags & IEEE80211_STA_DISABLE_WMM) {
		ieee80211_set_wmm_default(sdata);
	}
	else if (!ieee80211_sta_wmm_params(local, sdata, elems.wmm_param,
		elems.wmm_param_len,
		elems.mu_edca_param_set)) {
		/* still enable QoS since we might have HT/VHT */
		ieee80211_set_wmm_default(sdata);
		/* set the disable-WMM flag in this case to disable
		 * tracking WMM parameter changes in the beacon if
		 * the parameters weren't actually valid. Doing so
		 * avoids changing parameters very strangely when
		 * the AP is going back and forth between valid and
		 * invalid parameters.
		 */
		ifmgd->flags |= IEEE80211_STA_DISABLE_WMM;
	}
	else {
		changed |= BSS_CHANGED_QOS;
	}
	chan_state->oper_channel = assoc_req->chan;

	if (elems.ht_info_elem && elems.wmm_param &&
		(sdata->local->hw.queues >= 4) &&
		!(ifmgd->flags & IEEE80211_STA_DISABLE_11N))
		changed |= ieee80211_enable_ht(sdata, elems.ht_info_elem,
			cbss->bssid, ap_ht_cap_flags,
			false);

	/* set AID and assoc capability,
	 * ieee80211_set_associated() will tell the driver */
	bss_conf->aid = aid;
	bss_conf->assoc_capability = capab_info;
	ieee80211_set_associated(sdata, cbss, changed);
#ifdef  CONFIG_SUPPORT_80M_ADAPTER
	atbm_vht_80M_adapter_vif_assoc(sta);
#endif  //#ifdef  CONFIG_SUPPORT_80M_ADAPTER
#ifdef CONFIG_ATBM_SUPPORT_P2P
	/*
	 *process p2p ie ,and set p2p power save params:CTWindows,and NoA;
	 */
	while (sdata->vif.p2p) {
		if (!(elems.p2p_ie && elems.p2p_ie_len)) {
			atbm_printk_err("is p2p station,but p2p go do not send p2p ie\n");
			break;
		}
		ieee80211_sta_p2p_params(local, sdata, elems.p2p_ie, elems.p2p_ie_len);

		break;
	}
#endif

#ifdef CONFIG_ATBM_4ADDR
	/*
	 * If we're using 4-addr mode, let the AP know that we're
	 * doing so, so that it can create the STA VLAN on its side
	 */
	if (ifmgd->use_4addr)
		ieee80211_send_4addr_nullfunc(local, sdata);
#endif

	return true;
}

#ifdef ATBM_USE_FASTLINK
void ieee80211_save_bss_to_file(struct atbm_ieee80211_mgmt* mgmt,size_t len,struct ieee80211_sub_if_data* sdata)
{
	u8 write_all[432] = {0};
	u8* ies;
	u8 *ssid_ie;
	u8 ssid_len;
	u32 ies_len;
	u32 save_len;
	int i;
	__le16 fc;
	bool presp,ishidden=true;
	fc = mgmt->frame_control;
	ies = mgmt->u.beacon.variable;
	presp = ieee80211_is_probe_resp(fc);
	if (presp) {
			ies = mgmt->u.probe_resp.variable;
	}else{
			ies = mgmt->u.beacon.variable;
	}

	if(len > 400)
		return;
	
	ies_len = len - (u32)(ies - (u8 *)mgmt);
	ssid_ie = (u8 *)atbm_ieee80211_find_ie(ATBM_WLAN_EID_SSID, ies, ies_len);
	ssid_len = ssid_ie[1];

	// Length 0 means beacon
	if(ssid_len == 0){
		//if is hidden bss,the cfg80211 will drop the beacon,so we change beacon to resp
		mgmt->frame_control  =  cpu_to_le16(IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_PROBE_RESP);
		save_len = len + sdata->associated_ssid_len;
		atbm_printk_err("zezer:save_len = %d\n",save_len);
		memcpy(write_all,&save_len,sizeof(save_len));
		memcpy(write_all+sizeof(len),mgmt,len);
		memmove(write_all+sizeof(save_len)+(ssid_ie - (u8 *)mgmt) +2 + sdata->associated_ssid_len,write_all+sizeof(save_len)+(ssid_ie - (u8 *)mgmt)+2, len - (u32)(ssid_ie - (u8 *)mgmt)-2);
		atbm_printk_err("ssid_ie-mgmt = %d copy_size = %d\n",(ssid_ie - (u8 *)mgmt),len - (u32)(ssid_ie - (u8 *)mgmt)-2);
		
		memcpy(write_all+sizeof(save_len)+(ssid_ie - (u8 *)mgmt)+2,sdata->associated_ssid,sdata->associated_ssid_len);
		write_all[sizeof(save_len)+(ssid_ie - (u8 *)mgmt)+1] = sdata->associated_ssid_len;
	}else{
		for(i=0;i<ssid_len;i++){
			if(ssid_ie[2+i] == 0){
				continue;
			}else{
				ishidden = false;
				break;
				}	
		}
		if(ishidden){
			atbm_printk_err("associated_ssid = %s",sdata->associated_ssid);
			memcpy(ssid_ie+2,sdata->associated_ssid,ssid_len);
		}		
		memcpy(write_all,&len,sizeof(len));
		memcpy(write_all+sizeof(len),mgmt,len);
		atbm_printk_err("zezer:fastlink save data len = %d \n",len);
	}
	struct file *fp;
	mm_segment_t fs;
	loff_t pos;
	size_t fast_len;
	if(memcmp(sdata->name,"p2p",3))
		fp =filp_open(ATBM_SAVE_BSS_ID_0,O_RDWR | O_CREAT,0777);
	else
		fp =filp_open(ATBM_SAVE_BSS_ID_1,O_RDWR | O_CREAT,0777);

	if (IS_ERR(fp)){
		atbm_printk_err("fast : %s: create file error \n",sdata->name);
	}else{
		fs = get_fs();
		set_fs(KERNEL_DS);
		pos=0;
		fast_len = *((size_t *)write_all);
		//kernel_write(fp,local->write_all, len+sizeof(len), &pos);
		kernel_write(fp,write_all,512,fp->f_pos);
		filp_close(fp,NULL);
		set_fs(fs);
	}
}
#endif

static void ieee80211_rx_bss_info(struct ieee80211_sub_if_data* sdata,
	struct atbm_ieee80211_mgmt* mgmt,
	size_t len,
	struct ieee80211_rx_status* rx_status,
	struct ieee802_atbm_11_elems* elems,
	bool beacon)
{
	struct ieee80211_local* local = sdata->local;
	int freq;
	struct ieee80211_bss* bss;
	struct ieee80211_channel* channel;
	bool need_ps = false;
	struct ieee80211_channel_state* chan_state = ieee80211_get_channel_state(local, sdata);

	if (sdata->u.mgd.associated) {
		bss = (void*)sdata->u.mgd.associated->priv;
		/* not previously set so we may need to recalc */
		need_ps = !bss->dtim_period;
	}

	if (elems->ds_params && elems->ds_params_len == 1)
		freq = ieee80211_channel_to_frequency(elems->ds_params[0], rx_status->band);
	else
		freq = rx_status->freq;

	channel = ieee80211_get_channel(local->hw.wiphy, freq);

	if (!channel || channel->flags & IEEE80211_CHAN_DISABLED)
		return;

	bss = ieee80211_bss_info_update(local, rx_status, mgmt, len, elems,
		channel, beacon, false, mgmt->bssid);
	if (bss)
		ieee80211_rx_bss_put(local, bss);


#ifdef ATBM_USE_FASTLINK
	if(sdata->is_fast_associated && !memcmp(mgmt->bssid, sdata->associated_mac, ETH_ALEN)){
		ieee80211_save_bss_to_file(mgmt,len,sdata);
		sdata->is_fast_associated = false;
	}
#endif

	if (!sdata->u.mgd.associated)
		return;

	if (need_ps) {
		mutex_lock(&local->iflist_mtx);
		ieee80211_recalc_ps(local, -1);
		mutex_unlock(&local->iflist_mtx);
	}
	// add by wp , AP channel change we need deauth
	if (/*(sdata->u.mgd.associated) && */ (elems->ch_switch_elem == NULL) &&
		(channel_hw_value(chan_state->conf.channel) != channel_hw_value(channel))) {
		atbm_printk_err("rx_bss_info channel change %d,deauth it [%pM][%pM][%pM][%x] seq=%d\n", channel_hw_value(channel),
			mgmt->da, mgmt->sa, mgmt->bssid, mgmt->frame_control, mgmt->seq_ctrl);
		if (elems->ssid && elems->ssid_len) {
			u8* ssid = atbm_kzalloc(elems->ssid_len + 1, GFP_KERNEL);

			if (ssid) {
				memcpy(ssid, elems->ssid, elems->ssid_len);
				ssid[elems->ssid_len] = 0;
				atbm_printk_err("%s:channel change ssid(%s)\n", __func__, ssid);
				atbm_kfree(ssid);
			}
		}
		ieee80211_connection_loss(&sdata->vif);
		return;
	}
}


static void ieee80211_rx_mgmt_probe_resp(struct ieee80211_sub_if_data* sdata,
	struct sk_buff* skb)
{
	struct atbm_ieee80211_mgmt* mgmt = (void*)skb->data;
	struct ieee80211_if_managed* ifmgd;
	struct ieee80211_rx_status* rx_status = (void*)skb->cb;
	size_t baselen, len = skb->len;
	struct ieee802_atbm_11_elems elems;

	ifmgd = &sdata->u.mgd;

	ASSERT_MGD_MTX(ifmgd);

	if (memcmp(mgmt->da, sdata->vif.addr, ETH_ALEN))
		return; /* ignore ProbeResp to foreign address */

	baselen = (u8*)mgmt->u.probe_resp.variable - (u8*)mgmt;
	if (baselen > len)
		return;

	ieee802_11_parse_elems(mgmt->u.probe_resp.variable, len - baselen, false,
		&elems, mgmt->bssid, NULL);

	ieee80211_rx_bss_info(sdata, mgmt, len, rx_status, &elems, false);
}

static bool ieee80211_rx_our_beacon(const u8* tx_bssid,
	struct ieee80211_if_managed* ifmgd)
{
	if (ether_addr_equal(tx_bssid, ifmgd->bssid))
		return true;
	if (!ifmgd->b_notransmit_bssid)
		return false;

	return ether_addr_equal(tx_bssid, ifmgd->transmitter_bssid);
}
/*
 * This is the canonical list of information elements we care about,
 * the filter code also gives us all changes to the Microsoft OUI
 * (00:50:F2) vendor IE which is used for WMM which we need to track.
 *
 * We implement beacon filtering in software since that means we can
 * avoid processing the frame here and in cfg80211, and userspace
 * will not be able to tell whether the hardware supports it or not.
 *
 * XXX: This list needs to be dynamic -- userspace needs to be able to
 *	add items it requires. It also needs to be able to tell us to
 *	look out for other vendor IEs.
 */
static const u64 care_about_ies =
(1ULL << ATBM_WLAN_EID_COUNTRY) |
(1ULL << ATBM_WLAN_EID_ERP_INFO) |
(1ULL << ATBM_WLAN_EID_CHANNEL_SWITCH) |
(1ULL << ATBM_WLAN_EID_PWR_CONSTRAINT) |
(1ULL << ATBM_WLAN_EID_HT_CAPABILITY) |
(1ULL << ATBM_WLAN_EID_HT_INFORMATION) | 
(1ULL << ATBM_WLAN_EID_DS_PARAMS);
/*just used in sta mode && associated to ap*/
static void ieee80211_rx_mgmt_beacon(struct ieee80211_sub_if_data* sdata,
	struct atbm_ieee80211_mgmt* mgmt,
	size_t len,
	struct ieee80211_rx_status* rx_status)
{
	struct ieee80211_if_managed* ifmgd = &sdata->u.mgd;
	struct ieee80211_bss_conf* bss_conf = &sdata->vif.bss_conf;
	size_t baselen;
	struct ieee802_atbm_11_elems elems;
	struct ieee80211_local* local = sdata->local;
	struct ieee80211_channel_state* chan_state = ieee80211_get_channel_state(local, sdata);
	u32 changed = 0;
	bool erp_valid;
	u8 erp_value = 0;
	u32 ncrc;
	u8* bssid;
	struct sta_info* sta;

	ASSERT_MGD_MTX(ifmgd);
	/* Process beacon from the current BSS */
	baselen = (u8*)mgmt->u.beacon.variable - (u8*)mgmt;
	
	if (baselen > len)
		return;

    if(ifmgd->conn.state != IEEE80211_CONN_CONNECTED){
        atbm_printk_mgmt("%s:state not connected[%d]\n",__func__,ifmgd->conn.state);
        return;
    }
	/*
	 * We might have received a number of frames, among them a
	 * disassoc frame and a beacon...
	 */
	if (!ifmgd->associated)
		return;

	bssid = ifmgd->associated->bssid;
	/*
	 * And in theory even frames from a different AP we were just
	 * associated to a split-second ago!
	 */

	if (!ieee80211_rx_our_beacon(mgmt->bssid, ifmgd)) {
		return;
	}

	/* Track average RSSI from the Beacon frames of the current AP */
	ifmgd->last_beacon_signal = rx_status->signal;
	if (ifmgd->flags & IEEE80211_STA_RESET_SIGNAL_AVE) {
		ifmgd->flags &= ~IEEE80211_STA_RESET_SIGNAL_AVE;
		ifmgd->ave_beacon_signal = rx_status->signal * 16;
		ifmgd->last_cqm_event_signal = 0;
		ifmgd->count_beacon_signal = 1;
		ifmgd->last_ave_beacon_signal = 0;
	}
	else {
		ifmgd->ave_beacon_signal =
			(IEEE80211_SIGNAL_AVE_WEIGHT * rx_status->signal * 16 +
				(16 - IEEE80211_SIGNAL_AVE_WEIGHT) * ifmgd->ave_beacon_signal) /
			16;
		ifmgd->count_beacon_signal++;
	}
	
	bss_conf->timestamp = mgmt->u.beacon.timestamp;
	bss_conf->timestamp_update = ktime_to_us(ktime_get_real());
	
	ncrc = crc32_be(0, (void*)&mgmt->u.beacon.beacon_int, 4);
	ncrc = ieee802_11_parse_elems_crc(mgmt->u.beacon.variable,
		len - baselen, false, &elems,
		care_about_ies, ncrc,
		mgmt->bssid, bssid);
	
	if(elems.wmm_param && elems.wmm_param_len){
		if (!(ifmgd->flags & IEEE80211_STA_DISABLE_WMM) &&
			ieee80211_sta_wmm_params(local, sdata, elems.wmm_param,
				elems.wmm_param_len,
				elems.mu_edca_param_set)) {
			changed |= BSS_CHANGED_QOS;
		}
	}
	if (ncrc != ifmgd->beacon_crc || !ifmgd->beacon_crc_valid) {
		ieee80211_rx_bss_info(sdata, mgmt, len, rx_status, &elems, true);
#ifdef CONFIG_ATBM_SUPPORT_P2P
		ieee80211_sta_p2p_params(local, sdata, elems.p2p_ie, elems.p2p_ie_len);
#endif
	}
	if(elems.tim && sdata->vif.bss_conf.dtim_period != elems.tim->dtim_period){
		atbm_printk_err("[%s]:dtim changed [%d]->[%d]\n",sdata->name,sdata->vif.bss_conf.dtim_period,elems.tim->dtim_period);
		ifmgd->beacon_crc_valid = false;
		changed |= BSS_CHANGED_STA_DTIM;
		sdata->vif.bss_conf.dtim_period = elems.tim->dtim_period;
	}
	if (bss_conf->he_support && elems.he_operation) {
		u16 frame_time_rts_th = le32_get_bits(elems.he_operation->he_oper_params,ATBM_IEEE80211_HE_OPERATION_RTS_THRESHOLD_MASK);
		
		if(bss_conf->frame_time_rts_th != frame_time_rts_th){
			changed |= BSS_CHANGED_ASSOC;
			atbm_printk_err("frame_time_rts_th(%d)(%d)\n",bss_conf->frame_time_rts_th,frame_time_rts_th);
			bss_conf->frame_time_rts_th = frame_time_rts_th;
		}
	}
	if (ncrc == ifmgd->beacon_crc && ifmgd->beacon_crc_valid){
		if(changed){
			ieee80211_bss_info_change_notify(sdata, changed);
		}
		return;
	}
	ifmgd->beacon_crc = ncrc;
	ifmgd->beacon_crc_valid = true;

	if (elems.erp_info && elems.erp_info_len >= 1) {
		erp_valid = true;
		erp_value = elems.erp_info[0];
	}
	else {
		erp_valid = false;
	}
#ifdef CONFIG_ATBM_SUPPORT_CSA
	{
		struct atbm_ieee80211_channel_sw_packed_ie sw_packed_ie = {
				.chan_sw_ie = (struct atbm_ieee80211_channel_sw_ie *)elems.ch_switch_elem,
				.ex_chan_sw_ie = (struct atbm_ieee80211_ext_chansw_ie *)elems.extended_ch_switch_elem,
				.sec_chan_offs_ie = (struct atbm_ieee80211_sec_chan_offs_ie *)elems.secondary_ch_elem,
				};
		ieee80211_sta_process_chanswitch(sdata,
								&sw_packed_ie,
								(void *)ifmgd->associated->priv,
								rx_status->mactime);
	}
#endif
	changed |= ieee80211_handle_bss_capability(sdata,
		le16_to_cpu(mgmt->u.beacon.capab_info),
		erp_valid, erp_value);

#ifdef CONFIG_ATBM_HE

	rcu_read_lock();

	sta = sta_info_get(sdata, bssid);
	if (WARN_ON(!sta)) {
		atbm_printk_err("%s() %d## ifname[%s] sdata->vif.addr:%pM MAC:%pM \n",
			__func__, __LINE__, sdata->name, sdata->vif.addr, bssid);
		rcu_read_unlock();
		return;
	}

	changed |= ieee80211_recalc_twt_req(sdata, sta, &elems);

	rcu_read_unlock();
	if (!(ifmgd->flags & IEEE80211_STA_DISABLE_HE) && sdata->vif.bss_conf.he_support && elems.he_operation) {
		if ((sdata->vif.bss_conf.he_bss_color.color != le32_get_bits(elems.he_operation->he_oper_params, ATBM_IEEE80211_HE_OPERATION_BSS_COLOR_MASK))
			|| (sdata->vif.bss_conf.he_bss_color.partial != le32_get_bits(elems.he_operation->he_oper_params, ATBM_IEEE80211_HE_OPERATION_PARTIAL_BSS_COLOR))
			|| (sdata->vif.bss_conf.he_bss_color.enabled != !le32_get_bits(elems.he_operation->he_oper_params, ATBM_IEEE80211_HE_OPERATION_BSS_COLOR_DISABLED))) {
			sdata->vif.bss_conf.he_bss_color.color = le32_get_bits(elems.he_operation->he_oper_params, ATBM_IEEE80211_HE_OPERATION_BSS_COLOR_MASK);
			sdata->vif.bss_conf.he_bss_color.partial = le32_get_bits(elems.he_operation->he_oper_params, ATBM_IEEE80211_HE_OPERATION_PARTIAL_BSS_COLOR);
			sdata->vif.bss_conf.he_bss_color.enabled = !le32_get_bits(elems.he_operation->he_oper_params, ATBM_IEEE80211_HE_OPERATION_BSS_COLOR_DISABLED);

			changed |= BSS_CHANGED_HE_BSS_COLOR;
			ieee80211_he_op_ie_to_bss_conf(&sdata->vif, elems.he_operation);
		}
		/* TODO: OPEN: what happens if BSS color disable is set? */
	}
#endif //#ifdef CONFIG_ATBM_HE

	if (elems.ht_cap_elem && elems.ht_info_elem && elems.wmm_param &&
		!(ifmgd->flags & IEEE80211_STA_DISABLE_11N)) {
		// struct sta_info *sta;
		struct ieee80211_supported_band* sband;
		u16 ap_ht_cap_flags;

		rcu_read_lock();

		sta = sta_info_get(sdata, bssid);
		if (WARN_ON(!sta)) {
			atbm_printk_err("%s() %d## ifname[%s] sdata->vif.addr:%pM MAC:%pM \n",
				__func__, __LINE__, sdata->name, sdata->vif.addr, bssid);
			rcu_read_unlock();
			return;
		}

		sband = local->hw.wiphy->bands[chan_state->conf.channel->band];

		ieee80211_ht_cap_ie_to_sta_ht_cap(sband,
			elems.ht_cap_elem, &sta->sta.ht_cap);

		ap_ht_cap_flags = sta->sta.ht_cap.cap;

		rcu_read_unlock();

		changed |= ieee80211_enable_ht(sdata, elems.ht_info_elem,
			bssid, ap_ht_cap_flags, true);
	}

	/* Note: country IE parsing is done for us by cfg80211 */
	if (elems.country_elem) {
		/* TODO: IBSS also needs this */
		if (elems.pwr_constr_elem)
			ieee80211_handle_pwr_constr(sdata,
				le16_to_cpu(mgmt->u.probe_resp.capab_info),
				elems.pwr_constr_elem,
				elems.pwr_constr_elem_len);
	}

	ieee80211_bss_info_change_notify(sdata, changed);
}
static void ieee80211_rx_mgmt_action(struct ieee80211_sub_if_data *sdata,struct sk_buff *skb)
{
    struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;
	struct atbm_ieee80211_mgmt* mgmt;
    struct atbm_ieee80211_channel_sw_packed_ie sw_packed_ie = {
	.chan_sw_ie = NULL,
	.ex_chan_sw_ie = NULL,
	.sec_chan_offs_ie = NULL,
	};
    int ie_len;
	struct ieee802_atbm_11_elems elems;
    u8 *ies;
	struct ieee80211_rx_status* rx_status;

	mgmt = (struct atbm_ieee80211_mgmt*)skb->data;
	rx_status = (struct ieee80211_rx_status*)skb->cb;

    if(conn->state != IEEE80211_CONN_CONNECTED){
        return;
    }
    if(WARN_ON(ifmgd->associated == NULL)){
        return;
    }

    if(atbm_compare_ether_addr(ifmgd->associated->bssid,mgmt->bssid) != 0){
        return;
    }

    switch (mgmt->u.action.category) {
#ifdef CONFIG_ATBM_SUPPORT_CSA
	case ATBM_WLAN_CATEGORY_SPECTRUM_MGMT:
	    if(ATBM_WLAN_ACTION_SPCT_CHL_SWITCH == mgmt->u.action.u.chan_switch.action_code){

            ie_len = skb->len - offsetof(struct atbm_ieee80211_mgmt,u.action.u.chan_switch.variable);

            if(ie_len < 0){
                break;
            }
            ies = mgmt->u.action.u.chan_switch.variable; 
	        ieee802_11_parse_elems(ies,ie_len, false, &elems, mgmt->bssid, NULL);

            if(elems.ch_switch_elem && elems.ch_switch_elem_len >= sizeof(struct atbm_ieee80211_channel_sw_ie)){
               sw_packed_ie.chan_sw_ie = (struct atbm_ieee80211_channel_sw_ie *)elems.ch_switch_elem; 
            }

            if(elems.secondary_ch_elem && elems.secondary_ch_elem_len >= sizeof(struct atbm_ieee80211_sec_chan_offs_ie)){
                sw_packed_ie.sec_chan_offs_ie = (struct atbm_ieee80211_sec_chan_offs_ie *)elems.secondary_ch_elem;
            }

			ieee80211_sta_process_chanswitch(sdata,&sw_packed_ie,
							(void *)ifmgd->associated->priv,
							rx_status->mactime);

            break;
        }
#ifdef CONFIG_ATBM_SPECTRUM_MGMT
		else if(mgmt->u.action.u.measurement.action_code == WLAN_ACTION_SPCT_MSR_REQ){
		    /*
		    *process measurement
		    */
		    ieee80211_process_measurement_req(sdata,mgmt,skb->len);
		}
#endif
	case ATBM_WLAN_CATEGORY_PUBLIC:
	//add 8.5.8.7 Extended Channel Switch Announcement frame format
		if(ATBM_WLAN_PUB_ACTION_EX_CHL_SW_ANNOUNCE == mgmt->u.action.u.ext_chan_switch.action_code)
		{
            ie_len = skb->len - offsetof(struct atbm_ieee80211_mgmt,u.action.u.ext_chan_switch.variable);

            if(ie_len < 0){
                break;
            }
			sw_packed_ie.ex_chan_sw_ie = &mgmt->u.action.u.ext_chan_switch.ext_sw_elem;
            ieee802_11_parse_elems(&mgmt->u.action.u.ext_chan_switch.variable[0],ie_len, false, &elems, mgmt->bssid, NULL);

            if(elems.ch_switch_elem && elems.ch_switch_elem_len >= sizeof(struct atbm_ieee80211_channel_sw_ie)){
               sw_packed_ie.chan_sw_ie = (struct atbm_ieee80211_channel_sw_ie *)elems.ch_switch_elem; 
            }

            if(elems.secondary_ch_elem && elems.secondary_ch_elem_len >= sizeof(struct atbm_ieee80211_sec_chan_offs_ie)){
                sw_packed_ie.sec_chan_offs_ie = (struct atbm_ieee80211_sec_chan_offs_ie *)elems.secondary_ch_elem;
            }
			ieee80211_sta_process_chanswitch(sdata,&sw_packed_ie,
							(void *)ifmgd->associated->priv,
							rx_status->mactime);
		}
		break;
#endif
	case ATBM_WLAN_CATEGORY_HT:
			//add 8.5.12.2 Notify Channel Width frame format
			if(
				WLAN_HT_ACTION_NOTIFY_CHANWIDTH == mgmt->u.action.u.notify_chan_width.action_code
			  )
			{
				break;	
			}

	default:
		break;

    }
}
void ieee80211_sta_rx_queued_mgmt(struct ieee80211_sub_if_data* sdata,
	struct sk_buff* skb)
{
	struct ieee80211_rx_status* rx_status;
	struct atbm_ieee80211_mgmt* mgmt;
	u16 fc;

	rx_status = (struct ieee80211_rx_status*)skb->cb;
	mgmt = (struct atbm_ieee80211_mgmt*)skb->data;
	fc = le16_to_cpu(mgmt->frame_control) & IEEE80211_FCTL_STYPE;

	if (!ieee80211_is_mgmt(mgmt->frame_control)){
		atbm_printk_err("%s:recv(%x) drop\n",sdata->name,mgmt->frame_control);
		return;
	}
	
    ieee80211_sdata_mgd_lock(sdata,NULL);

    ieee80211_conn_running_hold(&sdata->u.mgd.conn);

	switch (fc) {
	case IEEE80211_STYPE_BEACON:
#ifdef CONFIG_ATBM_SUPPORT_CSA
#ifdef CONFIG_ATBM_SUPPORT_CHANSWITCH_TEST
	    mgmt = (struct atbm_ieee80211_mgmt*)ieee80211_sta_swbeacon_add(sdata,skb);
#endif
#endif
        ieee80211_rx_mgmt_beacon(sdata, mgmt, skb->len,rx_status);
		break;
	case IEEE80211_STYPE_PROBE_RESP:
		ieee80211_rx_mgmt_probe_resp(sdata, skb);
		break;
    case IEEE80211_STYPE_AUTH:
        ieee80211_rx_mgmt_auth(sdata,mgmt,skb->len);
        break;
	case IEEE80211_STYPE_DEAUTH:
		ieee80211_rx_mgmt_deauth(sdata, mgmt, skb->len);
		break;
    case IEEE80211_STYPE_ASSOC_RESP:
        atbm_fallthrough;
    case IEEE80211_STYPE_REASSOC_RESP:
        ieee80211_rx_mgmt_assoc_resp(sdata,mgmt,skb->len,fc ==  IEEE80211_STYPE_REASSOC_RESP);
        break;
	case IEEE80211_STYPE_DISASSOC:
		ieee80211_rx_mgmt_disassoc(sdata, mgmt, skb->len);
		break;
	case IEEE80211_STYPE_ACTION:
        ieee80211_rx_mgmt_action(sdata,skb);
		break;
    default:
        break;
	}

#ifdef CONFIG_ATBM_STA_LISTEN
	if (rx_status->flag & RX_FLAG_STA_LISTEN) {
		ieee80211_sta_rx_queued_mgmt_special(sdata, skb);
	}
#endif
    ieee80211_conn_running_release(&sdata->u.mgd.conn);
    ieee80211_sdata_mgd_unlock(sdata,NULL);
    return;
}
#if defined(CONFIG_PM)
static void ieee80211_sta_connection_lost(struct ieee80211_sub_if_data* sdata,
	u8* bssid, u8 reason)
{
	struct ieee80211_local* local = sdata->local;
	struct ieee80211_if_managed* ifmgd = &sdata->u.mgd;

	ifmgd->flags &= ~(IEEE80211_STA_CONNECTION_POLL |
		IEEE80211_STA_BEACON_POLL);

	ieee80211_set_disassoc(sdata, true, true);
	mutex_lock(&local->mtx);
	ieee80211_recalc_idle(local);
	mutex_unlock(&local->mtx);
	/*
	 * must be outside lock due to cfg80211,
	 * but that's not a problem.
	 */
	ieee80211_send_deauth_disassoc(sdata, bssid,
		IEEE80211_STYPE_DEAUTH, reason,true);
}
#endif

void atbm_send_deauth_disossicate(struct ieee80211_sub_if_data *sdata,u8 *bssid)
{
#if 0
  ieee80211_sta_connection_lost(sdata,bssid,3);
#endif
}


#if defined(CONFIG_PM) || defined(ATBM_SUSPEND_REMOVE_INTERFACE) || defined(ATBM_SUPPORT_WOW)
void ieee80211_sta_quiesce(struct ieee80211_sub_if_data* sdata)
{
	struct ieee80211_if_managed* ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;

	/*
	 * we need to use atomic bitops for the running bits
	 * only because both timers might fire at the same
	 * time -- the code here is properly synchronised.
	 */
#ifdef CONFIG_ATBM_SMPS
	atbm_cancel_work_sync(&ifmgd->request_smps_work);
#endif
	atbm_cancel_work_sync(&ifmgd->beacon_connection_loss_work);

    ieee80211_sdata_mgd_lock(sdata,NULL);

    if(conn->assoc_req || conn->auth_req){
        if(conn->auth_req){
            conn->state = IEEE80211_CONN_AUTHEN_FAILED_TIMEOUT;
        }else {
            conn->state = IEEE80211_CONN_ASSOC_FAILED_TIMEOUT;
        }
        __ieee80211_conn_state_process(sdata,NULL);
    }

    ieee80211_sdata_mgd_unlock(sdata,NULL);
}
#endif

#if defined(CONFIG_PM)
void ieee80211_sta_restart(struct ieee80211_sub_if_data* sdata)
{
	struct ieee80211_if_managed* ifmgd = &sdata->u.mgd;
    
    ieee80211_sdata_mgd_lock(sdata,NULL);

	if (!ifmgd->associated)
		goto exit;

	if (sdata->flags & IEEE80211_SDATA_DISCONNECT_RESUME) {
		sdata->flags &= ~IEEE80211_SDATA_DISCONNECT_RESUME;
		if (ifmgd->associated) {
#ifdef CONFIG_MAC80211_ATBM_VERBOSE_DEBUG
			wiphy_debug(sdata->local->hw.wiphy,
				"%s: driver requested disconnect after resume.\n",
				sdata->name);
#endif
			ieee80211_sta_connection_lost(sdata,
				ifmgd->associated->bssid,
				WLAN_REASON_UNSPECIFIED);
			goto exit;
		}
	}
exit:
    ieee80211_sdata_mgd_unlock(sdata,NULL);
    return;
}
#endif

/* interface stop*/
void ieee80211_conn_purge(struct ieee80211_sub_if_data *sdata)
{
    struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;

    if(sdata->vif.type != NL80211_IFTYPE_STATION){
        return;
    }

    atbm_cancel_work_sync(&ifmgd->beacon_connection_loss_work);

	synchronize_rcu();

    ieee80211_sdata_mgd_lock(sdata,NULL);
    
    atbm_printk_always("%s:conn state[%s]\n",__func__,ieee80211_conn_state_string(conn));

    if(conn->state != IEEE80211_CONN_IDLE){

        if(conn->state == IEEE80211_CONN_ABANDON){
            /*
             * wait abandon,do not change the state;
             */
        }else if(conn->auth_req){
            conn->state = IEEE80211_CONN_AUTHEN_FAILED_TIMEOUT;
        }else if(conn->assoc_req){
            conn->state = IEEE80211_CONN_ASSOC_FAILED_TIMEOUT;
        }else {
            conn->state = IEEE80211_CONN_ABANDON;
        }
        __ieee80211_conn_state_process(sdata,NULL);
    }

    WARN_ON(conn->state     !=  IEEE80211_CONN_IDLE);
    WARN_ON(conn->auth_req  != NULL);
    WARN_ON(conn->assoc_req != NULL);

    atbm_del_timer_sync(&ifmgd->timer);
    ieee80211_sdata_mgd_unlock(sdata,NULL);

	atbm_flush_work(&sdata->work);
}
/* interface setup */
void ieee80211_sta_setup_sdata(struct ieee80211_sub_if_data* sdata)
{
	struct ieee80211_if_managed* ifmgd;

	ifmgd = &sdata->u.mgd;
	ATBM_INIT_WORK(&ifmgd->beacon_connection_loss_work,
		ieee80211_beacon_connection_loss_work);

    atbm_setup_timer(&ifmgd->timer,ieee80211_sta_timer,(unsigned long) sdata);

#ifdef CONFIG_ATBM_SMPS
	ATBM_INIT_WORK(&ifmgd->request_smps_work, ieee80211_request_smps_work);
    if (sdata->local->hw.flags & IEEE80211_HW_SUPPORTS_DYNAMIC_SMPS)
		ifmgd->req_smps = IEEE80211_SMPS_AUTOMATIC;
	else
		ifmgd->req_smps = IEEE80211_SMPS_OFF;
#endif
	/*
	setup_timer(&ifmgd->scan_delay_timer,ieee80211_scan_delay_timer,
			(unsigned long)sdata);
	*/

	ifmgd->flags = 0;

	mutex_init(&ifmgd->mtx);

	/* Disable UAPSD for sta by default */
	sdata->uapsd_queues = IEEE80211_DEFAULT_UAPSD_QUEUES;
#ifdef CONFIG_MAC80211_ATBM_ROAMING_CHANGES
	ifmgd->roaming = 0;
#endif // CONFIG_MAC80211_ATBM_ROAMING_CHANGES
}

/* scan finished notification */
void ieee80211_mlme_notify_scan_completed(struct ieee80211_local* local)
{
	struct ieee80211_sub_if_data* sdata;
	mutex_lock(&local->iflist_mtx);
	list_for_each_entry(sdata, &local->interfaces, list)
	{
		if (ieee80211_sdata_running(sdata) && (sdata->vif.type == NL80211_IFTYPE_STATION))
			ieee80211_queue_work(&sdata->local->hw, &sdata->work);
	}
	mutex_unlock(&local->iflist_mtx);
}
static void ieee80211_mgd_deauth_rx(struct ieee80211_sub_if_data *sdata,struct atbm_ieee80211_mgmt *mgmt,size_t len)
{
#if (LINUX_VERSION_IS_LESS_AND_NOT_CPTCFG(3, 11, 0))
	__cfg80211_send_deauth(sdata->dev, (u8*)mgmt, len);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	cfg80211_tx_mlme_mgmt(sdata->dev, (u8*)mgmt, len,1);
#else
#if CONFIG_CPTCFG_CFG80211
	cfg80211_tx_mlme_mgmt(sdata->dev, (u8*)mgmt, len);
#else
	cfg80211_tx_mlme_mgmt(sdata->dev, (u8*)mgmt, len);
#endif //	CONFIG_CPTCFG_CFG80211
#endif //(LINUX_VERSION_IS_LESS_AND_NOT_CPTCFG(3,11,0))

}
static void ieee80211_mgd_auth_rx_authen(struct ieee80211_sub_if_data *sdata,struct atbm_ieee80211_mgmt *mgmt, size_t len)
{
#if (LINUX_VERSION_IS_LESS_AND_NOT_CPTCFG(3, 11, 0))
    ieee80211_sdata_mgd_unlock(sdata,NULL);
	cfg80211_send_rx_auth(sdata->dev, (u8*)mgmt, len);
    ieee80211_sdata_mgd_lock(sdata,NULL);
#else
	cfg80211_rx_mlme_mgmt(sdata->dev, (u8*)mgmt, len);
#endif
}

static void ieee80211_mgd_auth_timeout(struct ieee80211_sub_if_data *sdata,struct ieee80211_mgd_auth_req *req)
{
    atbm_printk_err("%s:(%p)\n",__func__,req);
#if (LINUX_VERSION_IS_LESS_AND_NOT_CPTCFG(3, 11, 0))
    ieee80211_sdata_mgd_unlock(sdata,NULL);
	cfg80211_send_auth_timeout(sdata->dev, req->bssid);
	atbm_printk_err("ieee80211_mgd_auth_timeout to user space\n");
    ieee80211_sdata_mgd_lock(sdata,NULL);
#else
	cfg80211_auth_timeout(sdata->dev,req->bssid);
#endif

}
#ifdef CONFIG_ATBM_SUPPORT_SAE
static enum sae_authen_state ieee80211_sae_trans_state(u16 trans,bool tx)
{
	enum sae_authen_state state;

	if(tx == true){
		state = trans == 1 ? SAE_AUTHEN_TX_COMMIT : SAE_AUTHEN_TX_CONFIRM;
	}else {
		state = trans == 1 ? SAE_AUTHEN_RX_COMMIT : SAE_AUTHEN_RX_CONFIRM;
	}

	return state;
}

static enum sae_action ieee80211_sae_authen_state(struct ieee80211_sub_if_data *sdata,
        enum sae_authen_state state)
{
	struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
    enum sae_action action = SAE_CONTINUE;

    atbm_printk_always("%s:sae_state in[%d][%d][%x]\n",__func__,ifmgd->sae_state,state,ifmgd->sae_state_bit);

    switch(state){
    case SAE_AUTHEN_TX_COMMIT:
        ifmgd->sae_state = IEEE80211_SAE_COMMITTED;
        ifmgd->sae_state_bit = IEEE80211_LOCAL_COMMIT; 
        break;
    case SAE_AUTHEN_TX_CONFIRM:
        if((ifmgd->sae_state_bit & IEEE80211_LOCAL_COMMIT) == 0){
            action = SAE_SKIP;
            break;
        }
        ifmgd->sae_state_bit |= IEEE80211_LOCAL_CONFIRM;

        if(ifmgd->sae_state_bit & IEEE80211_PEAR_CONFIRM){
            ifmgd->sae_state = IEEE80211_SAE_ACCEPTED;
        }else {
            ifmgd->sae_state = IEEE80211_SAE_CONFIRMED;
        }
        break;
    case SAE_AUTHEN_RX_COMMIT:
        if(ifmgd->sae_state != IEEE80211_SAE_COMMITTED){
            action = SAE_SKIP;
            break;
        }
        ifmgd->sae_state_bit |= IEEE80211_PEAR_COMMIT;
        break;
    case SAE_AUTHEN_RX_CONFIRM:

        if((ifmgd->sae_state != IEEE80211_SAE_COMMITTED) && 
           (ifmgd->sae_state != IEEE80211_SAE_CONFIRMED)){
            action = SAE_SKIP;
            break;
        }

        if((ifmgd->sae_state_bit & IEEE80211_PEAR_COMMIT) == 0){
            action = SAE_SKIP;
            break;
        }

        ifmgd->sae_state_bit |= IEEE80211_PEAR_CONFIRM;

        if(ifmgd->sae_state == IEEE80211_SAE_CONFIRMED){
            ifmgd->sae_state = IEEE80211_SAE_ACCEPTED;
        }
        break;
    }
    atbm_printk_always("%s:sae_state out[%d][%d][%x]\n",__func__,ifmgd->sae_state,state,ifmgd->sae_state_bit);
    return action;
}
#endif
static int ieee80211_authenticate(struct ieee80211_sub_if_data *sdata)
{
    struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;
    struct ieee80211_mgd_auth_req *req = conn->auth_req;
    int ret = 0;
    u16 trans = 1;
	u16 status = 0;

    if(WARN_ON(req == NULL)){
        ret = -ENOLINK;
        goto err;
    }

    switch(req->algorithm){
    case ATBM_WLAN_AUTH_OPEN:
	case ATBM_WLAN_AUTH_LEAP:
	case ATBM_WLAN_AUTH_FT:
    case ATBM_WLAN_AUTH_SHARED_KEY:
		req->transaction = 2;	
        break;
#ifdef CONFIG_ATBM_SUPPORT_SAE
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 8, 0)) || defined(CONFIG_CFG80211_COMPAT_MODIFIED)
	case ATBM_WLAN_AUTH_SAE:
		trans  = req->sae_trans;
		status = req->sae_status;
		req->transaction = trans;

		switch(ieee80211_sae_authen_state(sdata,ieee80211_sae_trans_state(req->sae_trans,true))){
        case SAE_DONE:
		case SAE_CONTINUE:
            break;
		case SAE_SKIP:
			/*transmit authen*/
			goto exit;
        case SAE_FAIL:
            ret = -ENOENT;
            goto exit;
		}

        if(ifmgd->sae_state == IEEE80211_SAE_ACCEPTED){
            req->done = true;
        }
		break;
#endif
#endif
    default:
        break;
    }
    atbm_printk_err("%s: authenticate with %pM  (%d)(%d)\n",sdata->name, 
            req->bssid,trans,status);
    ieee80211_send_auth(sdata, trans,req->algorithm, status, 
            req->ie,req->ie_len,req->bssid, NULL, 0, 0);
exit:
    return ret;
err:
    goto exit; 
}
static void ieee80211_auth_challenge(struct ieee80211_sub_if_data *sdata,
				     struct atbm_ieee80211_mgmt *mgmt,
				     size_t len)
{
    struct ieee80211_mgd_auth_req *auth_req = sdata->u.mgd.conn.auth_req;
    u8 *pos;
	struct ieee802_atbm_11_elems elems;

    BUG_ON(auth_req == NULL);
	pos = mgmt->u.auth.variable;
	ieee802_11_parse_elems(pos, len - (pos - (u8 *) mgmt), false, &elems, mgmt->bssid, NULL);
	if (!elems.challenge)
		return;
	ieee80211_send_auth(sdata, 3,auth_req->algorithm, 0,
			    elems.challenge - 2, elems.challenge_len + 2,
			    auth_req->bssid, auth_req->key,
			    auth_req->key_len, auth_req->key_idx);
	auth_req->transaction = 4;

}

static int ieee80211_mgd_assoc_state_check(struct ieee80211_sub_if_data *sdata,struct ieee80211_mgd_assoc_req *req)
{
    struct ieee80211_if_managed* ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;

    atbm_printk_err("%s:state[%s][%p][%p]\n",__func__, ieee80211_conn_state_string(conn),conn->auth_req,conn->assoc_req);

    if(conn->assoc_req){
        return -EBUSY;
    }

    conn->tries = 0;

    switch(conn->state){
    case IEEE80211_CONN_IDLE:
        atbm_printk_err("%s:idle -> assoc ?\n",__func__);
        if(WARN_ON(conn->assoc_req))
            ieee80211_sdata_free_assoc_data(sdata,conn->assoc_req);
        if(WARN_ON(conn->auth_req))
            ieee80211_sdata_free_auth_data(sdata,conn->auth_req);
        conn->state = IEEE80211_CONN_CHAN_ASYNC;
        break;
    case IEEE80211_CONN_AUTHENTICATE_NEXT:
        WARN_ON(1);
        return -EBUSY;
    case IEEE80211_CONN_CHAN_ASYNC:
    case IEEE80211_CONN_JOIN:
    case IEEE80211_CONN_AUTHENTICATING:
        if(conn->auth_req){
            if(WARN_ON(memcmp(conn->auth_req->bssid,req->bssid,ETH_ALEN))){
                atbm_printk_err("%s:auth and assoc with diffrent bssid[%pM],[%pM]\n",__func__,
                        conn->auth_req->bssid,req->bssid);
                sta_info_destroy_addr(sdata,conn->auth_req->bssid);
                return -EBUSY;
            }
        }else {
            WARN_ON(1);
            return -EINVAL;
        }
        break;
    case IEEE80211_CONN_AUTHEN_FAILED_TIMEOUT:
        atbm_printk_err("%s: assoc too late\n",__func__);
       return -ENOTSUPP;
    case IEEE80211_CONN_ASSOCIATE_NEXT:
    case IEEE80211_CONN_ASSOCIATING:
    case IEEE80211_CONN_ASSOC_FAILED:
    case IEEE80211_CONN_ASSOC_FAILED_TIMEOUT:
       return -EBUSY;
    case IEEE80211_CONN_CONNECTED:
       ieee80211_set_disassoc(sdata,true,true);
       conn->state = IEEE80211_CONN_CHAN_ASYNC;
       break;
    default:
       return -EBUSY;
    }
    return 0;
}
static int ieee80211_mgd_auth_state_check(struct ieee80211_sub_if_data *sdata,struct ieee80211_mgd_auth_req *auth_req)
{
    struct ieee80211_if_managed* ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;
    int status = 0;

    atbm_printk_err("%s:state[%s][%p][%p] in\n",__func__,ieee80211_conn_state_string(conn),conn->auth_req,conn->assoc_req);

    conn->tries = 0;

    switch(conn->state){
    case IEEE80211_CONN_IDLE:
        ifmgd->conn.state = IEEE80211_CONN_CHAN_ASYNC;

        if(WARN_ON(conn->assoc_req)){
            ieee80211_sdata_free_assoc_data(sdata,conn->assoc_req);
        }
        break;
    case IEEE80211_CONN_CHAN_ASYNC:
    case IEEE80211_CONN_PRE_ABANDON:
    case IEEE80211_CONN_JOIN:
    case IEEE80211_CONN_AUTHENTICATE_NEXT:
        atbm_printk_err("%s:can not in this state\n",__func__);
        status = -EBUSY;
        break;
    case IEEE80211_CONN_AUTHENTICATING:
        WARN_ON(conn->assoc_req);

        if(conn->auth_req &&conn->auth_req->algorithm == ATBM_WLAN_AUTH_SAE){
            if(auth_req->algorithm != ATBM_WLAN_AUTH_SAE){
                atbm_printk_err("%s:algorithm from[%d]->[%d]\n",__func__,conn->auth_req->algorithm,auth_req->algorithm);
                status = -EBUSY;
                break;
            }
            if(memcmp(conn->auth_req->bssid,auth_req->bssid,ETH_ALEN) && (status == 0)){
                atbm_printk_err("%s:flush authen bssid[%p][%p],[%pM][%pM]\n",__func__,conn->auth_req->bss,
                    auth_req->bss,conn->auth_req->bssid,auth_req->bssid);
                status = -EBUSY;
                break;
            }
            ieee80211_sdata_free_auth_data(sdata,conn->auth_req);
        }else if(conn->auth_req){
            atbm_printk_err("%s:start new auth ?\n",__func__);
            status = -EBUSY;
        }else {
            WARN_ON(1);
            status = -EINVAL;
        }
        break;
    case IEEE80211_CONN_ABANDON:
    case IEEE80211_CONN_ASSOCIATE_NEXT:
    case IEEE80211_CONN_ASSOCIATING:
        WARN_ON(ifmgd->conn.auth_req);
        atbm_printk_err("%s:wait assoc ,return busy\n",__func__);
        status = -EBUSY;
        break;
    case IEEE80211_CONN_ASSOC_FAILED:
    case IEEE80211_CONN_ASSOC_FAILED_TIMEOUT:
    case IEEE80211_CONN_AUTHEN_FAILED_TIMEOUT:
        BUG_ON(1);
        break;
    case IEEE80211_CONN_CONNECTED:
        WARN_ON(conn->work != NULL);
        WARN_ON(ifmgd->associated == NULL);
        if(ifmgd->associated){
            /*
             * disconnect from current ap
             */
            ieee80211_set_disassoc(sdata,true,true);
            conn->state = IEEE80211_CONN_CHAN_ASYNC;
        }
        break;
    }

    atbm_printk_err("%s:state[%s][%p][%p] out\n",__func__,ieee80211_conn_state_string(conn),conn->auth_req,conn->assoc_req);
    return status;
}
int ieee80211_mgd_auth(struct ieee80211_sub_if_data *sdata,
		       struct cfg80211_auth_request *req)
{
	struct ieee80211_if_managed* ifmgd = &sdata->u.mgd;
    struct ieee80211_mgd_auth_req *auth_req = NULL;
    int ret = 0;
    size_t ie_len = 0;
	const u8 *ssid;
    /*lock ifmgd lock*/
    ieee80211_sdata_mgd_lock(sdata,req);

    if((sdata->vif.type != NL80211_IFTYPE_STATION ) && 
       (sdata->vif.type != NL80211_IFTYPE_P2P_CLIENT)){
        atbm_printk_err("%s:if type err(%d)\n",__func__,sdata->vif.type);
        ret = -ENOTSUPP;
        goto err;
    }

    ie_len = req->ie_len;
#ifdef CONFIG_ATBM_SUPPORT_SAE
#if (LINUX_VERSION_IS_GEQ_OR_CPTCFG_OR_COMPAT(4, 10, 0))
	ie_len += req->auth_data_len;
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0))
	ie_len += req->sae_data_len;
#endif
#endif
    auth_req = atbm_kzalloc(sizeof(struct ieee80211_mgd_auth_req)+ie_len,GFP_KERNEL);

    if(auth_req == NULL){
        ret = -ENOMEM;
        goto exit;
    }

    switch (req->auth_type) {
	case NL80211_AUTHTYPE_OPEN_SYSTEM:
        auth_req->algorithm = ATBM_WLAN_AUTH_OPEN; 
		break;
	case NL80211_AUTHTYPE_SHARED_KEY:
#ifdef CONFIG_ATBM_USE_SW_ENC
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 4, 0))
		if (IS_ERR(sdata->local->wep_tx_tfm)){
			ret = -EOPNOTSUPP;
            goto err;
        }
#endif
        auth_req->algorithm = ATBM_WLAN_AUTH_SHARED_KEY;
		break;
#else
		ret = -EOPNOTSUPP;
        goto err;
#endif
	case NL80211_AUTHTYPE_FT:
        auth_req->algorithm = ATBM_WLAN_AUTH_FT;
		break;
	case NL80211_AUTHTYPE_NETWORK_EAP:
        auth_req->algorithm = ATBM_WLAN_AUTH_LEAP;
		break;
#ifdef CONFIG_ATBM_SUPPORT_SAE
#if (LINUX_VERSION_IS_GEQ_OR_CPTCFG_OR_COMPAT(3, 8, 0))
	case NL80211_AUTHTYPE_SAE:
		auth_req->algorithm = ATBM_WLAN_AUTH_SAE;
		break;
#endif
#endif
	default:
		ret = -EOPNOTSUPP;
        goto err;
	}
#ifdef CONFIG_ATBM_SUPPORT_SAE
#if (LINUX_VERSION_IS_GEQ_OR_CPTCFG_OR_COMPAT(4, 10, 0))
	if (req->auth_data_len >= 4) {
		__le16* pos = (__le16*)req->auth_data;
		auth_req->sae_trans = le16_to_cpu(pos[0]);
		auth_req->sae_status = le16_to_cpu(pos[1]);
		memcpy(auth_req->ie, req->auth_data + 4,
			req->auth_data_len - 4);
		auth_req->ie_len += req->auth_data_len - 4;
	}
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0))
	if (req->sae_data_len >= 4) {
		//struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
		__le16* pos = (__le16*)req->sae_data;
		auth_req->sae_trans = le16_to_cpu(pos[0]);
		auth_req->sae_status = le16_to_cpu(pos[1]);
		memcpy(auth_req->ie, req->sae_data + 4,
			req->sae_data_len - 4);
		auth_req->ie_len += req->sae_data_len - 4;
	}
#endif
#endif    

	memcpy(auth_req->bssid, req->bss->bssid, ETH_ALEN);
#ifdef  CONFIG_SUPPORT_80M_ADAPTER
	atbm_vht_80M_adapter_vif_init(sdata,1,req->bss->bssid);
#endif  //#ifdef  CONFIG_SUPPORT_80M_ADAPTER

    if (req->ie && req->ie_len) {
		memcpy(&auth_req->ie[auth_req->ie_len], req->ie, req->ie_len);
		auth_req->ie_len += req->ie_len;
	}

	if (req->key && req->key_len) {
		auth_req->key_len = req->key_len;
		auth_req->key_idx = req->key_idx;
		memcpy(auth_req->key, req->key, req->key_len);
	}

	ssid = ieee80211_bss_get_ie(req->bss, ATBM_WLAN_EID_SSID);

	if (ssid && (IEEE80211_MAX_SSID_LEN >= ssid[1])) {
		memcpy(auth_req->ssid, ssid + 2, ssid[1]);
		auth_req->ssid_len = ssid[1];
	}
	else {
		atbm_printk_mgmt("%s:no ssid\n", __func__);
		auth_req->ssid_len = 0;
	}

	auth_req->privacy = req->bss->capability & WLAN_CAPABILITY_PRIVACY;
	auth_req->bss     = req->bss;
    auth_req->chan    = req->bss->channel;

    if(ieee80211_mgd_auth_state_check(sdata,auth_req)){
        atbm_printk_err("%s: authen state err\n",__func__);
        ret = -EBUSY;
        goto err;
    }
    BUG_ON(ifmgd->conn.auth_req);
    ifmgd->conn.auth_req = auth_req;
    ifmgd->conn.bss = req->bss;
    ieee80211_atbm_handle_bss(sdata->local->hw.wiphy,req->bss);

    ieee80211_conn_state_process(sdata,req);
exit:
    ieee80211_sdata_mgd_unlock(sdata,req);
    return ret;
err:
    if(auth_req)
        atbm_kfree(auth_req);
    goto exit;
}
/*
*0:upload
*1:not unload
:-1:not alloc
*/
static int ieee80211_mgd_sta_status(struct ieee80211_sub_if_data *sdata,u8 *bssid)
{
	struct sta_info *sta;
	int status = -1;
	
	rcu_read_lock();
	sta = sta_info_get_rx(sdata,bssid);

	if(sta){
		status = sta->uploaded == true ? 0 : 1;
	}else {
		status = -1;
	}
	rcu_read_unlock();

	return status;
}
int ieee80211_mgd_pre_sta(struct ieee80211_sub_if_data *sdata,struct cfg80211_bss *cbss)
{
	struct sta_info *sta;
	int err;

	if((err = ieee80211_mgd_sta_status(sdata,cbss->bssid)) >= 0){

		atbm_printk_err("sta has been insert(%d)\n",err);
		return err;
	}

	sta = sta_info_alloc(sdata, cbss->bssid, GFP_KERNEL);
	if (!sta)
		return -ENOMEM;
	/*
	*update basic rate for later use
	*/
	{
		struct ieee80211_bss *bss = (struct ieee80211_bss *)cbss->priv;
		int i = 0;
		int j = 0;
		struct ieee80211_supported_band* sband;
		u32 rates = 0;
		
		sband = sdata->local->hw.wiphy->bands[bss->band];
		
		for (i = 0; i < bss->supp_rates_len; i++) {
			int rate = (bss->supp_rates[i] & 0x7f) * 5;

			for (j = 0; j < sband->n_bitrates; j++) {
				if(ieee80211_rate_unmap(&sband->bitrates[j]))
					continue;
				if (sband->bitrates[j].bitrate == rate) {
					rates |= BIT(j);
					break;
				}
			}
		}
		sta->sta.supp_rates[bss->band] = rates;		
	}
	
	sta->dummy = true;
	
	err = sta_info_insert(sta);
	
	if (err) {
		atbm_printk_err( "%s: failed to insert Dummy STA(%d)\n", sdata->name, err);
		return err;
	}
	return ieee80211_mgd_sta_status(sdata,cbss->bssid); 
}
static void ieee80211_mgd_disassoc_rx(struct ieee80211_sub_if_data *sdata,struct atbm_ieee80211_mgmt *mgmt,size_t len)
{
#if (LINUX_VERSION_IS_LESS_AND_NOT_CPTCFG(3, 11, 0))
	__cfg80211_send_disassoc(sdata->dev, (u8*)mgmt, len);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	cfg80211_tx_mlme_mgmt(sdata->dev, (u8*)mgmt, len,1);
#else
#if CONFIG_CPTCFG_CFG80211
	cfg80211_tx_mlme_mgmt(sdata->dev, (u8*)mgmt, len);
#else
	cfg80211_tx_mlme_mgmt(sdata->dev, (u8*)mgmt, len);
#endif //	CONFIG_CPTCFG_CFG80211
#endif //(LINUX_VERSION_IS_LESS_AND_NOT_CPTCFG(3,11,0))

}
static void ieee80211_mgd_assoc_rx_resp(struct ieee80211_sub_if_data *sdata,struct cfg80211_bss *bss,struct atbm_ieee80211_mgmt* mgmt,size_t len)
{
	
#if (LINUX_VERSION_IS_LESS_AND_NOT_CPTCFG(3, 4, 0))
    ieee80211_sdata_mgd_unlock(sdata,NULL);
	cfg80211_send_rx_assoc(sdata->dev, (u8*)mgmt, len);
    ieee80211_sdata_mgd_lock(sdata,NULL);
#elif (LINUX_VERSION_IS_LESS_AND_NOT_CPTCFG(3, 11, 0))
    ieee80211_sdata_mgd_unlock(sdata,NULL);
	cfg80211_send_rx_assoc(sdata->dev,bss, (u8*)mgmt, len);
    ieee80211_sdata_mgd_lock(sdata,NULL);
#elif (LINUX_VERSION_IS_LESS_AND_NOT_CPTCFG(3, 18, 0))
	/*
	void cfg80211_rx_assoc_resp(struct net_device *dev, struct cfg80211_bss *bss,
					const u8 *buf, size_t len)
	*/
	cfg80211_rx_assoc_resp(sdata->dev,bss, (u8*)mgmt, len);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	
	struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
	struct cfg80211_rx_assoc_resp resp = {
		.uapsd_queues = -1,
	};
	resp.links[0].bss = bss;
	resp.buf = (u8 *)mgmt;
	resp.len = len;
//	resp.req_ies = ifmgd->assoc_req_ies;
//	resp.req_ies_len = ifmgd->assoc_req_ies_len;
	
	cfg80211_rx_assoc_resp(sdata->dev,&resp);

#elif (LINUX_VERSION_IS_GEQ_OR_CPTCFG(5, 0, 0))
	cfg80211_rx_assoc_resp(sdata->dev,bss, (u8*)mgmt, len, -1, NULL, 0);
#else
	cfg80211_rx_assoc_resp(sdata->dev,bss, (u8*)mgmt, len, -1);
#endif
}

static void  ieee80211_mgd_assoc_timeout(struct ieee80211_sub_if_data *sdata,struct ieee80211_mgd_assoc_req *req)
{
#if (LINUX_VERSION_IS_LESS_AND_NOT_CPTCFG(3, 11, 0))
    ieee80211_sdata_mgd_unlock(sdata,NULL);
	cfg80211_send_assoc_timeout(sdata->dev, req->bssid);
    ieee80211_sdata_mgd_lock(sdata,NULL);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	struct cfg80211_assoc_failure data={0};
	data.bss[0] = req->bss;
	data.timeout = 1;
    ieee80211_sdata_mgd_unlock(sdata,NULL);
	cfg80211_assoc_failure(sdata->dev,&data);
    ieee80211_sdata_mgd_lock(sdata,NULL);
#else
	cfg80211_assoc_timeout(sdata->dev,req->bss);
#endif

}

static int ieee80211_compatible_rates(const u8 *supp_rates, int supp_rates_len,
				      struct ieee80211_supported_band *sband,
				      u32 *rates)
{
	int i, j, count;
	*rates = 0;
	count = 0;
	for (i = 0; i < supp_rates_len; i++) {
		int rate = (supp_rates[i] & 0x7F) * 5;

		for (j = 0; j < sband->n_bitrates; j++)
			if (sband->bitrates[j].bitrate == rate) {
				*rates |= BIT(j);
				count++;
				break;
			}
	}

	return count;
}
static void ieee80211_add_ht_ie(struct sk_buff *skb, struct atbm_ieee80211_ht_info *ht_info,
				struct ieee80211_supported_band *sband,
				struct ieee80211_channel *channel,
				enum ieee80211_smps_mode smps,u32 ht_override)
{
	u8 *pos;
	u32 flags = (channel->flags & ~(ht_override)) | ht_override;
	u16 cap = sband->ht_cap.cap;
	__le16 tmp;

	if (!sband->ht_cap.ht_supported)
		return;

	if (!ht_info)
		return;

	/* determine capability flags */

	switch (ht_info->ht_param & IEEE80211_HT_PARAM_CHA_SEC_OFFSET) {
	case IEEE80211_HT_PARAM_CHA_SEC_ABOVE:
		if (flags & IEEE80211_CHAN_NO_HT40PLUS) {
			cap &= ~IEEE80211_HT_CAP_SUP_WIDTH_20_40;
			cap &= ~IEEE80211_HT_CAP_SGI_40;
		}
		break;
	case IEEE80211_HT_PARAM_CHA_SEC_BELOW:
		if (flags & IEEE80211_CHAN_NO_HT40MINUS) {
			cap &= ~IEEE80211_HT_CAP_SUP_WIDTH_20_40;
			cap &= ~IEEE80211_HT_CAP_SGI_40;
		}
		break;
	}

	/* set SM PS mode properly */
	cap &= ~IEEE80211_HT_CAP_SM_PS;
	switch (smps) {
	case IEEE80211_SMPS_AUTOMATIC:
	case IEEE80211_SMPS_NUM_MODES:
		WARN_ON(1);
		atbm_fallthrough;
	case IEEE80211_SMPS_OFF:
		cap |= WLAN_HT_CAP_SM_PS_DISABLED <<
			IEEE80211_HT_CAP_SM_PS_SHIFT;
		break;
	case IEEE80211_SMPS_STATIC:
		cap |= WLAN_HT_CAP_SM_PS_STATIC <<
			IEEE80211_HT_CAP_SM_PS_SHIFT;
		break;
	case IEEE80211_SMPS_DYNAMIC:
		cap |= WLAN_HT_CAP_SM_PS_DYNAMIC <<
			IEEE80211_HT_CAP_SM_PS_SHIFT;
		break;
	}

	/* reserve and fill IE */

	pos = atbm_skb_put(skb, sizeof(struct ieee80211_ht_cap) + 2);
	*pos++ = ATBM_WLAN_EID_HT_CAPABILITY;
	*pos++ = sizeof(struct ieee80211_ht_cap);
	memset(pos, 0, sizeof(struct ieee80211_ht_cap));

	/* capability flags */
	tmp = cpu_to_le16(cap);
	memcpy(pos, &tmp, sizeof(u16));
	pos += sizeof(u16);

	/* AMPDU parameters */
	*pos++ = sband->ht_cap.ampdu_factor |
		 (sband->ht_cap.ampdu_density <<
			IEEE80211_HT_AMPDU_PARM_DENSITY_SHIFT);

	/* MCS set */
	memcpy(pos, &sband->ht_cap.mcs, sizeof(sband->ht_cap.mcs));
	pos += sizeof(sband->ht_cap.mcs);

	/* extended capabilities */
	pos += sizeof(__le16);

	/* BF capabilities */
	pos += sizeof(__le32);

	/* antenna selection */
	pos += sizeof(u8);
}
#ifdef CONFIG_ATBM_HE
static void ieee80211_add_he_ie(struct ieee80211_sub_if_data *sdata,
				struct sk_buff *skb,
				struct ieee80211_supported_band *sband)
{
	u8 *pos;
	const struct atbm_ieee80211_sta_he_cap *he_cap = NULL;
	u8 he_cap_size;
	//bool reg_cap = false;
	he_cap = atbm_ieee80211_get_he_iftype_cap(&sdata->local->hw,sband,
				     ieee80211_vif_type_p2p(&sdata->vif),sdata);
     if (!he_cap)
		return;
	he_cap_size =
		2 + 1 + sizeof(he_cap->he_cap_elem) +
		atbm_ieee80211_he_mcs_nss_size(&he_cap->he_cap_elem) +
		atbm_ieee80211_he_ppe_size(he_cap->ppe_thres[0],
				      he_cap->he_cap_elem.phy_cap_info);
	pos = skb_put(skb, he_cap_size);
	ieee80211_ie_build_he_cap(pos, he_cap, pos + he_cap_size);
}
#endif  //CONFIG_ATBM_HE
/* This function determines vht capability flags for the association                                                                             * and builds the IE.
 * Note - the function may set the owner of the MU-MIMO capability
 */
#ifdef CONFIG_ATBM_VHT
static void ieee80211_add_vht_ie(struct ieee80211_sub_if_data *sdata,
                 struct sk_buff *skb,
                 struct ieee80211_supported_band *sband,
                 struct atbm_ieee80211_vht_cap *ap_vht_cap)
{
    //struct ieee80211_local *local = sdata->local;
    u8 *pos;
    u32 cap;
    struct atbm_ieee80211_sta_vht_cap vht_cap;
    const struct atbm_ieee80211_sta_vht_cap *support_cap;
    u32 mask, ap_bf_sts, our_bf_sts;

    support_cap = atbm_ieee80211_get_vht_cap(&sdata->local->hw,sband,sdata);

    if(support_cap == NULL){
        return;
    }

    memcpy(&vht_cap,support_cap, sizeof(vht_cap));

    cap = vht_cap.cap;
    if (!(ap_vht_cap->vht_cap_info &
            cpu_to_le32(ATBM_IEEE80211_VHT_CAP_SU_BEAMFORMER_CAPABLE)))
        cap &= ~(ATBM_IEEE80211_VHT_CAP_SU_BEAMFORMEE_CAPABLE |
             ATBM_IEEE80211_VHT_CAP_MU_BEAMFORMEE_CAPABLE);
    else if (!(ap_vht_cap->vht_cap_info &
            cpu_to_le32(ATBM_IEEE80211_VHT_CAP_MU_BEAMFORMER_CAPABLE)))
        cap &= ~ATBM_IEEE80211_VHT_CAP_MU_BEAMFORMEE_CAPABLE; 
    
    mask = ATBM_IEEE80211_VHT_CAP_BEAMFORMEE_STS_MASK;
    ap_bf_sts = le32_to_cpu(ap_vht_cap->vht_cap_info) & mask;
    our_bf_sts = cap & mask;

    if (ap_bf_sts < our_bf_sts) {
        cap &= ~mask;
        cap |= ap_bf_sts;
    }

    /* reserve and fill IE */
    pos = skb_put(skb, sizeof(struct atbm_ieee80211_vht_cap) + 2);
    ieee80211_ie_build_vht_cap(pos, &vht_cap, cap);
}
#endif
static void ieee80211_send_assoc(struct ieee80211_sub_if_data *sdata)
{   
    struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
    struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;
    struct ieee80211_mgd_assoc_req *assoc_req = conn->assoc_req;
	struct sk_buff *skb;
	struct atbm_ieee80211_mgmt *mgmt;
	u8 *pos, qos_info;
	size_t offset = 0, noffset;
	int i, count, rates_len, supp_rates_len;
	u16 capab;
	struct ieee80211_supported_band *sband;
	u32 rates = 0;
	u8 *index;

    sband = local->hw.wiphy->bands[assoc_req->chan->band];

	if (assoc_req->supp_rates_len) {
		/*
		 * Get all rates supported by the device and the AP as
		 * some APs don't like getting a superset of their rates
		 * in the association request (e.g. D-Link DAP 1353 in
		 * b-only mode)...
		 */
		rates_len = ieee80211_compatible_rates(assoc_req->supp_rates,assoc_req->supp_rates_len,sband, &rates);
	} else {
		/*
		 * In case AP not provide any supported rates information
		 * before association, we send information element(s) with
		 * all rates that we support.
		 */
		rates = ~0;
		rates_len = sband->n_bitrates;
	}

    skb = atbm_alloc_skb(local->hw.extra_tx_headroom +
			sizeof(*mgmt) + /* bit too much but doesn't matter */
			2 + assoc_req->ssid_len + /* SSID */
			4 + rates_len + /* (extended) rates */
			4 + /* power capability */
			2 + 2 * sband->n_channels + /* supported channels */
			2 + sizeof(struct ieee80211_ht_cap) + /* HT */
#ifdef CONFIG_ATBM_HE
			2 + 1 + sizeof(struct atbm_ieee80211_he_cap_elem) + /* HE */
				sizeof(struct atbm_ieee80211_he_mcs_nss_supp) +
				ATBM_IEEE80211_HE_PPE_THRES_MAX_LEN +
			2 + 1 + sizeof(struct atbm_ieee80211_he_6ghz_capa) +
#endif  //  CONFIG_ATBM_HE
#ifdef CONFIG_SUPPORT_BSS_MAX_IDLE
			2 + sizeof(struct atbm_ieee80211_bss_max_idle_period_ie) + 
#endif
			assoc_req->ie_len + /* extra IEs */ + 3 +
			9, /* WMM */
			GFP_KERNEL);
	if (!skb)
		return;

    atbm_skb_reserve(skb, local->hw.extra_tx_headroom);

	capab = WLAN_CAPABILITY_ESS;

	if (sband->band == IEEE80211_BAND_2GHZ) {
		if (!(local->hw.flags & IEEE80211_HW_2GHZ_SHORT_SLOT_INCAPABLE))
			capab |= WLAN_CAPABILITY_SHORT_SLOT_TIME;
		if (!(local->hw.flags & IEEE80211_HW_2GHZ_SHORT_PREAMBLE_INCAPABLE))
			capab |= WLAN_CAPABILITY_SHORT_PREAMBLE;
	}

	if (assoc_req->capability & WLAN_CAPABILITY_PRIVACY)
		capab |= WLAN_CAPABILITY_PRIVACY;

	if ((assoc_req->capability & WLAN_CAPABILITY_SPECTRUM_MGMT) &&(local->hw.flags & IEEE80211_HW_SPECTRUM_MGMT))
		capab |= WLAN_CAPABILITY_SPECTRUM_MGMT;
#ifdef WLAN_CAPABILITY_RADIO_MEASURE
	if(local->ext_wfa_test_flag == 1){
		if (assoc_req->capability & WLAN_CAPABILITY_RADIO_MEASURE)
			capab |= WLAN_CAPABILITY_RADIO_MEASURE;
	}
#endif
    mgmt = (struct atbm_ieee80211_mgmt *) atbm_skb_put(skb, 24);
	memset(mgmt, 0, 24);
	memcpy(mgmt->da, assoc_req->bssid, ETH_ALEN);
	memcpy(mgmt->sa, sdata->vif.addr, ETH_ALEN);
	memcpy(mgmt->bssid, assoc_req->bssid, ETH_ALEN);

	if(assoc_req->filter_fc == IEEE80211_STYPE_REASSOC_RESP){
		atbm_skb_put(skb, 10);
		mgmt->frame_control = cpu_to_le16(IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_REASSOC_REQ);
		mgmt->u.reassoc_req.capab_info = cpu_to_le16(capab);
		mgmt->u.reassoc_req.listen_interval =
				cpu_to_le16(sdata->vif.bss_conf.listen_interval);
		memcpy(mgmt->u.reassoc_req.current_ap, assoc_req->prev_bssid,ETH_ALEN);
	} else {
		atbm_skb_put(skb, 4);
		mgmt->frame_control = cpu_to_le16(IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_ASSOC_REQ);
		mgmt->u.assoc_req.capab_info = cpu_to_le16(capab);
		mgmt->u.assoc_req.listen_interval =
				cpu_to_le16(sdata->vif.bss_conf.listen_interval);
	}
    /* SSID */
	pos = atbm_skb_put(skb, 2 + assoc_req->ssid_len);
	*pos++ = ATBM_WLAN_EID_SSID;
	*pos++ = assoc_req->ssid_len;
	memcpy(pos, assoc_req->ssid,assoc_req->ssid_len);

	/* add all rates which were marked to be used above */
	supp_rates_len = rates_len;
	if (supp_rates_len > 8)
		supp_rates_len = 8;

	pos = atbm_skb_put(skb, supp_rates_len + 2);
	*pos++ = ATBM_WLAN_EID_SUPP_RATES;
	*pos++ = supp_rates_len;

	count = 0;
	for (i = 0; i < sband->n_bitrates; i++) {
		if (BIT(i) & rates) {
			int rate = sband->bitrates[i].bitrate;
			if(i<4){
				*pos++ = (u8) ((rate / 5)|0x80);
			}else{
				*pos++ = (u8) (rate / 5);
			}
			if (++count == 8)
				break;
		}
	}

	if (rates_len > count) {
		pos = atbm_skb_put(skb, rates_len - count + 2);
		*pos++ = ATBM_WLAN_EID_EXT_SUPP_RATES;
		*pos++ = rates_len - count;

		for (i++; i < sband->n_bitrates; i++) {
			if (BIT(i) & rates) {
				int rate = sband->bitrates[i].bitrate;
				*pos++ = (u8)(rate / 5);
			}
		}
	}
    if (capab & WLAN_CAPABILITY_SPECTRUM_MGMT) {
		/* 1. power capabilities */
		pos = atbm_skb_put(skb, 4);
		*pos++ = ATBM_WLAN_EID_PWR_CAPABILITY;
		*pos++ = 2;
		*pos++ = 0; /* min tx power */
		*pos++ = assoc_req->chan->max_power; /* max tx power */

		/* 2. supported channels */
		/* TODO: get this in reg domain format */
		pos = atbm_skb_put(skb, 2 * sband->n_channels + 2);
		*pos++ = ATBM_WLAN_EID_SUPPORTED_CHANNELS;
		*pos++ = 2 * sband->n_channels;
		for (i = 0; i < sband->n_channels; i++) {
			*pos++ = ieee80211_frequency_to_channel(
					channel_center_freq(&sband->channels[i]));
			*pos++ = 1; /* one channel in the subband*/
		}
	}
#ifdef CONFIG_SUPPORT_BSS_MAX_IDLE
	{
		struct atbm_ieee80211_bss_max_idle_period_ie *idle;
		
		pos = atbm_skb_put(skb, sizeof(struct atbm_ieee80211_bss_max_idle_period_ie) + 2);
		*pos++ = ATBM_WLAN_EID_BSS_MAX_IDLE_PERIOD;
		*pos++ = 3;
		idle = (struct atbm_ieee80211_bss_max_idle_period_ie *)pos;
		idle->max_idle_period = cpu_to_le16(0x0b);
		idle->idle_options = 1;
	}
#endif

	/* if present, add any custom IEs that go before HT */
	if (assoc_req->ie_len && assoc_req->ie) {
		static const u8 before_ht[] = {
			ATBM_WLAN_EID_SSID,
			ATBM_WLAN_EID_SUPP_RATES,
			ATBM_WLAN_EID_EXT_SUPP_RATES,
			ATBM_WLAN_EID_PWR_CAPABILITY,
			ATBM_WLAN_EID_SUPPORTED_CHANNELS,
			ATBM_WLAN_EID_RSN,
			ATBM_WLAN_EID_QOS_CAPA,
			ATBM_WLAN_EID_RRM_ENABLED_CAPABILITIES,
			ATBM_WLAN_EID_MOBILITY_DOMAIN,
			ATBM_WLAN_EID_SUPPORTED_REGULATORY_CLASSES,
		};
		noffset = atbm_ieee80211_ie_split(assoc_req->ie, assoc_req->ie_len,before_ht, ARRAY_SIZE(before_ht), offset);
		pos = atbm_skb_put(skb, noffset - offset);
		memcpy(pos, assoc_req->ie + offset, noffset - offset);
		offset = noffset;
	}
    if (assoc_req->use_11n && assoc_req->wmm_used &&
	    local->hw.queues >= 4){
		ieee80211_add_ht_ie(skb, &assoc_req->ht_info,sband, assoc_req->chan, assoc_req->smps,sdata->vif.p2p == true ? IEEE80211_CHAN_NO_HT40PLUS | IEEE80211_CHAN_NO_HT40MINUS:0);
	
#ifdef CONFIG_ATBM_HE
	   if (assoc_req->use_he){	
			atbm_printk_err("assoc.use_he %d %d++\n",assoc_req->use_he, assoc_req->wmm_used);
			ieee80211_add_he_ie(sdata, skb, sband);
	    }
#endif  //  CONFIG_ATBM_HE
#ifdef CONFIG_ATBM_VHT
	   if((hw_priv != NULL) && (hw_priv->chip_version != OCEANUS_NO_WIFI6))
	   {
       if(assoc_req->use_vht){
           ieee80211_add_vht_ie(sdata,skb,sband,&assoc_req->vht_cap);
           if(assoc_req->chan->band == IEEE80211_BAND_5GHZ){
               atbm_printk_err("%s:add chanel op\n",__func__);
               pos = atbm_skb_put(skb, 3);

		       *pos++ = ATBM_WLAN_EID_OP_MODE_NOTIFICATION;
               *pos++ = 1;
			   
			   //printk("----%s------>sdata->bSupport40M %d\n",__func__,sdata->bSupport40M);
               if(sdata->bSupport40M == 0){
                   *pos++ = 0;
               }else {
                   *pos++ = 1;
               }
           }
       }
       }
#endif
	}

	/* if present, add any custom non-vendor IEs that go after HT */
	if (assoc_req->ie_len && assoc_req->ie) {
		noffset = ieee80211_ie_split_vendor(assoc_req->ie, assoc_req->ie_len,offset);
		pos = atbm_skb_put(skb, noffset - offset);
		memcpy(pos, assoc_req->ie + offset, noffset - offset);
		offset = noffset;
	}
	if (assoc_req->wmm_used && local->hw.queues >= 4) {
		if (assoc_req->uapsd_used) {
			qos_info = sdata->uapsd_queues;
			qos_info |= (local->uapsd_max_sp_len << IEEE80211_WMM_IE_STA_QOSINFO_SP_SHIFT);
		} else {
			qos_info = 0;
		}
		pos = atbm_skb_put(skb, 9);
		*pos++ = ATBM_WLAN_EID_VENDOR_SPECIFIC;
		*pos++ = 7; /* len */
		*pos++ = 0x00; /* Microsoft OUI 00:50:F2 */
		*pos++ = 0x50;
		*pos++ = 0xf2;
		*pos++ = 2; /* WME */
		*pos++ = 0; /* WME info */
		*pos++ = 1; /* WME ver */
		*pos++ = qos_info;
	}
    /* add any remaining custom (i.e. vendor specific here) IEs */
	if (assoc_req->ie_len && assoc_req->ie) {
		noffset = assoc_req->ie_len;
		pos = atbm_skb_put(skb, noffset - offset);
		memcpy(pos, assoc_req->ie + offset, noffset - offset);
		pos+=noffset - offset;
	}

	//ADD EXT_CAP Multiple BSSID
	index = (u8 *)cfg80211_find_ie(ATBM_WLAN_EID_EXT_CAPABILITY, mgmt->u.assoc_req.variable, pos-mgmt->u.assoc_req.variable);
	if(index){
		index[4] |= BIT(6);	
	}
	IEEE80211_SKB_CB(skb)->flags |= IEEE80211_TX_INTFL_DONT_ENCRYPT;
	ieee80211_tx_skb(sdata, skb);

}
static enum work_done_result ieee80211_chan_async_work_done(struct ieee80211_work* wk,struct sk_buff* skb)
{
    struct ieee80211_sub_if_data *sdata = wk->sdata;

    atbm_printk_always("%s:[%d]\n",__func__,wk->chan_async.done);

    sdata->u.mgd.conn.work = NULL;

    if(wk->chan_async.done == false){
	    ieee80211_queue_work(&wk->sdata->local->hw, &wk->sdata->work);
    }

    return WORK_DONE_DESTROY; 
}
static enum work_action __must_check ieee80211_chan_async_work_start(struct ieee80211_work *wk)
{
    struct sk_buff *skb;
    atbm_printk_always("%s:[%d]\n",__func__,wk->chan_async.done);

    if(wk->chan_async.done == true){
        goto exit;
    }

    skb = atbm_alloc_skb(0,GFP_KERNEL);

    if(skb == NULL){
        wk->timeout = jiffies + msecs_to_jiffies(20);
        goto exit;
    }
    
    ieee80211_ht_cap_adjust_by_customer(&wk->sdata->local->hw,wk->sdata);

    skb->pkt_type = IEEE80211_SDATA_QUEUE_MLME;
    skb->cb[0]    = MLME_CHANNEL_ASYNC_NOTIFY;
    wk->chan_async.done = true;

	atbm_skb_queue_tail(&wk->sdata->skb_queue, skb);
	ieee80211_queue_work(&wk->sdata->local->hw, &wk->sdata->work);

exit:
    wk->timeout = jiffies + HZ;
    return WORK_ACT_NONE;
}
static enum conn_process_state ieee80211_conn_channel_async(struct ieee80211_sub_if_data *sdata,unsigned long *next_delay)
{
    struct ieee80211_work *wk;
    struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;

    if((conn->assoc_req == NULL) && (conn->auth_req == NULL)){
        conn->state = IEEE80211_CONN_PRE_ABANDON;
        goto exit;
    }
    
    if(conn->work && conn->work->chan == conn->bss->channel){
        if(conn->work->started == true){
            conn->state = IEEE80211_CONN_JOIN;
        }else {
            conn->tries ++;

            if(conn->tries > 5){
                conn->state = IEEE80211_CONN_JOIN;
                goto exit;
            }

            atbm_printk_always("%s:wait channel\n",__func__);
            *next_delay = 500;
        }
        goto exit;

    }else if(WARN_ON(conn->work)){
        ieee80211_work_purge(sdata,NULL,IEEE80211_WORK_MAX,true);
    }
    
    wk = atbm_kzalloc(sizeof(struct ieee80211_work),GFP_KERNEL);
    
    if(wk == NULL){
        conn->state = IEEE80211_CONN_PRE_ABANDON;
        goto exit;
    }
    
    memcpy(wk->filter_bssid, conn->bss->bssid, ETH_ALEN);
    memcpy(wk->filter_sa,conn->bss->bssid,ETH_ALEN);
    
    wk->type = IEEE80211_WORK_CHAN_ASYNC;
    wk->chan = conn->bss->channel;
    wk->chan_type = NL80211_CHAN_NO_HT;
    wk->chan_mode = CHAN_MODE_FIXED;
    wk->sdata  = sdata;
    wk->done   = ieee80211_chan_async_work_done;
    wk->start  = ieee80211_chan_async_work_start;
    wk->filter = NULL; 
    wk->rx     = NULL;
    
    conn->work = wk;
    
    ieee80211_add_work(wk);

    ieee80211_work_work(&sdata->local->work_work);

    if(conn->work == NULL){
        atbm_printk_err("%s:chan_async work failed\n",__func__);
        conn->tries = INT_MAX;
        conn->state = IEEE80211_CONN_JOIN;
    }else if(conn->work->started == true){
        conn->state = IEEE80211_CONN_JOIN;
    }else {
        *next_delay = 500;
    }
exit:
    return CONN_PROCESS__CONTINUE;
}
static enum conn_process_state ieee80211_conn_join(struct ieee80211_sub_if_data *sdata,unsigned long *next_delay)
{
    struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;
	struct ieee80211_local *local = sdata->local;
	//struct atbm_common *hw_priv=local->hw.priv;
 
	//atbm_printk_always("if_id-ieee80211_conn_join:%d\n", local->dummy_insert_failcnt);
    *next_delay = 0;

    if((conn->tries > 3) || (local->dummy_insert_failcnt >= 2)){
        atbm_printk_err("%s: authen timeout",__func__);
		local->dummy_insert_failcnt = 0;
		
        if(conn->auth_req){
            conn->state = IEEE80211_CONN_AUTHEN_FAILED_TIMEOUT;
            goto exit;
        }
        if(conn->assoc_req){
            conn->state = IEEE80211_CONN_ASSOC_FAILED_TIMEOUT;
            goto exit;
        }
        conn->state = IEEE80211_CONN_PRE_ABANDON;
        goto exit;
    }
    
    if((conn->auth_req == NULL) && (conn->assoc_req == NULL)){
        atbm_printk_err("%s: authen or assoc has been disabled\n",__func__);
        conn->state = IEEE80211_CONN_PRE_ABANDON;
        goto exit;
    }
    
    conn->tries ++;
    
    if(WARN_ON(ieee80211_mgd_pre_sta(sdata,conn->bss))){
        /*
         * wait 500ms,then retry
         */
        *next_delay = 500;
        goto exit;
    }
    
    if(conn->auth_req && (conn->auth_req->send == false)){
        conn->state = IEEE80211_CONN_AUTHENTICATE_NEXT;
        goto exit;
    }
    
    if(conn->assoc_req && (conn->assoc_req->send == false)){
        conn->state = IEEE80211_CONN_ASSOCIATE_NEXT;
        goto exit;
    }
    
    conn->state = IEEE80211_CONN_ASSOCIATING;
    *next_delay = 500;
exit:
    return CONN_PROCESS__CONTINUE;
}
static enum conn_process_state ieee80211_conn_auth_next(struct ieee80211_sub_if_data *sdata,unsigned long *next_delay)
{
    struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;
    struct ieee80211_mgd_auth_req *req = conn->auth_req;

    *next_delay = 0;
    /*
     * authen may has been cancled
     */
    if(WARN_ON(req == NULL)){
        conn->state = IEEE80211_CONN_AUTHEN_FAILED_TIMEOUT;
        goto exit; 
    }

    if(req->tries > 3){
        conn->state = IEEE80211_CONN_AUTHEN_FAILED_TIMEOUT;
        goto exit;
    }

    if(((req->waiting == true) || (req->done == true)) && (req->send == true)){
        atbm_printk_err("%s:waitting next:(%d),(%d)\n",__func__,req->waiting,req->done);
        if(conn->assoc_req == NULL){
            *next_delay = 500;
            conn->state = IEEE80211_CONN_AUTHENTICATING;
        }else {
            conn->state = IEEE80211_CONN_ASSOCIATE_NEXT;
        }
        goto exit;
    }
     /*
     * send authen
     */
    if(ieee80211_authenticate(sdata)){
        conn->state = IEEE80211_CONN_AUTHENTICATE_NEXT;
        *next_delay = 500;
    }else {
        req->send = true;
        if(conn->assoc_req){
            /*
             * send assoc 
             */
            WARN_ON(conn->assoc_req->send == true);
            conn->state = IEEE80211_CONN_ASSOCIATE_NEXT;
            goto exit;
        }
        conn->state = IEEE80211_CONN_AUTHENTICATING;
        *next_delay = 500;
    }
exit:
    return CONN_PROCESS__CONTINUE;
}
static enum conn_process_state ieee80211_conn_authenticating(struct ieee80211_sub_if_data *sdata,unsigned long *next_delay)
{
    struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;
    struct ieee80211_mgd_auth_req *auth_req = conn->auth_req;
    struct ieee80211_mgd_assoc_req *assoc_req = conn->assoc_req;

    if(assoc_req || (auth_req && auth_req->send == false)){
        conn->state = IEEE80211_CONN_CHAN_ASYNC;
        goto exit;
    }

    if(auth_req){ 
        auth_req->tries ++;

        if(auth_req->tries > 3){
            conn->state = IEEE80211_CONN_AUTHEN_FAILED_TIMEOUT;
        }else {
            conn->state = IEEE80211_CONN_AUTHENTICATE_NEXT;
        }
    }else {
        WARN_ON(1);
        conn->state = IEEE80211_CONN_AUTHEN_FAILED_TIMEOUT;
    }
exit:
   *next_delay = 0;
   return CONN_PROCESS__CONTINUE;
}
static enum conn_process_state ieee80211_conn_auth_timeout(struct ieee80211_sub_if_data *sdata,unsigned long *next_delay)
{    
    struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;

    if(conn->auth_req){
        atbm_printk_err("%s:[%pM],auth timeout\n",__func__,conn->auth_req->bssid);
        /*
   	    * Most likely AP is not in the range so remove the
   	    * bss struct for that AP.
   	    */
        if(conn->auth_req->done == 0){
            cfg80211_unlink_bss(sdata->local->hw.wiphy,conn->auth_req->bss);
        }
#ifdef CONFIG_MAC80211_ATBM_ROAMING_CHANGES
		sdata->queues_locked = 0;
#endif
		sta_info_destroy_addr(sdata,conn->auth_req->bssid);
        if(conn->assoc_req){
            conn->state = IEEE80211_CONN_ASSOC_FAILED_TIMEOUT;
            goto exit;
        }
    }
    conn->state = IEEE80211_CONN_PRE_ABANDON;
exit:
    *next_delay = 0;
    return CONN_PROCESS__CONTINUE;
}
static enum conn_process_state ieee80211_conn_assoc_next(struct ieee80211_sub_if_data *sdata,unsigned long *next_delay)
{
    struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;
    struct ieee80211_mgd_assoc_req *req = conn->assoc_req;
            
    if(conn->auth_req){
        ieee80211_sdata_free_auth_data(sdata,conn->auth_req);
    }

    if(req == NULL){
        atbm_printk_always("%s:assoc aband\n",__func__);
        conn->state = IEEE80211_CONN_ASSOC_FAILED_TIMEOUT;
        goto exit;
    }

    ieee80211_send_assoc(sdata);
    conn->state = IEEE80211_CONN_ASSOCIATING;
    req->send = true;

    *next_delay = 500;
exit:
    return CONN_PROCESS__CONTINUE;
}
static enum conn_process_state ieee80211_conn_associating(struct ieee80211_sub_if_data *sdata,unsigned long *next_delay)
{
    struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;
    struct ieee80211_mgd_assoc_req *req = conn->assoc_req;

    if(req == NULL){
        atbm_printk_always("%s:assoc aband\n",__func__);
        conn->state = IEEE80211_CONN_ASSOC_FAILED_TIMEOUT;
        goto exit;
    }

    req->tries ++;

    if(req->tries > 3){
        conn->state = IEEE80211_CONN_ASSOC_FAILED_TIMEOUT;
    }else {
        conn->state = IEEE80211_CONN_ASSOCIATE_NEXT;
    }

exit:
    *next_delay = 0;
    return CONN_PROCESS__CONTINUE;
}
static enum conn_process_state ieee80211_conn_assoc_timeout(struct ieee80211_sub_if_data *sdata,unsigned long *next_delay)
{
    struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;

    if(conn->assoc_req){
        atbm_printk_err("%s:[%pM],auth timeout\n",__func__,conn->assoc_req->bssid);
	    if (conn->assoc_req->bss)
	        cfg80211_unlink_bss(sdata->local->hw.wiphy,conn->assoc_req->bss);
	    sta_info_destroy_addr(sdata,conn->assoc_req->bssid);
    }

    conn->state = IEEE80211_CONN_PRE_ABANDON;

    *next_delay = 0;

    return CONN_PROCESS__CONTINUE;
}
static enum conn_process_state ieee80211_conn_abandon(struct ieee80211_sub_if_data *sdata,unsigned long *next_delay)
{
    struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;

    sta_info_flush(sdata->local,sdata);

    ieee80211_work_purge(sdata,NULL,IEEE80211_WORK_MAX,true);

    ieee80211_conn_mark_idle(sdata);

    atbm_del_timer_sync(&ifmgd->timer);

    if(conn->auth_req){
        struct ieee80211_mgd_auth_req *req = conn->auth_req;

        conn->auth_req = NULL;

        if(req->done == 0){
            ieee80211_mgd_auth_timeout(sdata,req);
        }
        ieee80211_sdata_free_auth_data(sdata,req);
    }

    if(conn->assoc_req){
        struct ieee80211_mgd_assoc_req *req = conn->assoc_req;
        conn->assoc_req = NULL;

        ieee80211_mgd_assoc_timeout(sdata,req);

        ieee80211_sdata_free_assoc_data(sdata,req);
    }

    /*
     * recalc idle state
     */
    mutex_lock(&sdata->local->mtx);
	ieee80211_recalc_idle(sdata->local);
	mutex_unlock(&sdata->local->mtx);

    *next_delay = 0;
    return CONN_PROCESS__FINISH;
}
static enum conn_process_state __ieee80211_conn_state_process(struct ieee80211_sub_if_data *sdata ,void *cookie)
{
    struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;
    unsigned long next_delay = 0;
    enum conn_process_state state = CONN_PROCESS__CONTINUE;

    atbm_del_timer_sync(&ifmgd->timer);

    ieee80211_conn_running_hold(conn);

    do{
        atbm_printk_always("%s:state[%s],auth[%p],assoc[%p],cookie[%p]\n",__func__,ieee80211_conn_state_string(conn),conn->auth_req,conn->assoc_req,cookie);

        switch(conn->state){
        case IEEE80211_CONN_IDLE:
            state = CONN_PROCESS__FINISH;
            break;
        case IEEE80211_CONN_CHAN_ASYNC:
            state = ieee80211_conn_channel_async(sdata,&next_delay);
            break;
        case IEEE80211_CONN_JOIN:
            state = ieee80211_conn_join(sdata,&next_delay);
            break;
        case IEEE80211_CONN_AUTHENTICATE_NEXT:
            state = ieee80211_conn_auth_next(sdata,&next_delay);
            break;
        case IEEE80211_CONN_AUTHENTICATING:
            state = ieee80211_conn_authenticating(sdata,&next_delay);
            break;
        case IEEE80211_CONN_AUTHEN_FAILED_TIMEOUT:
            state = ieee80211_conn_auth_timeout(sdata,&next_delay);
            break;
        case IEEE80211_CONN_ASSOCIATE_NEXT:
            state = ieee80211_conn_assoc_next(sdata,&next_delay);
            break;
        case IEEE80211_CONN_ASSOCIATING:
            state = ieee80211_conn_associating(sdata,&next_delay);
            break;
        case IEEE80211_CONN_ASSOC_FAILED:
            state = CONN_PROCESS__SKIP;
            break;
        case IEEE80211_CONN_ASSOC_FAILED_TIMEOUT:
            state = ieee80211_conn_assoc_timeout(sdata,&next_delay);
            break;
        case IEEE80211_CONN_PRE_ABANDON:
            conn->state = IEEE80211_CONN_ABANDON;
            if(cookie){
                next_delay = 10;
            }
            break;
        case IEEE80211_CONN_ABANDON:
            state = ieee80211_conn_abandon(sdata,&next_delay);
            break;
        case IEEE80211_CONN_CONNECTED:
            state = CONN_PROCESS__FINISH;
            break;
        }

    }while((next_delay == 0) && (state == CONN_PROCESS__CONTINUE));

    if(next_delay && (state == CONN_PROCESS__CONTINUE)){
        ifmgd->timeout = jiffies + msecs_to_jiffies(next_delay);
        run_again(ifmgd,ifmgd->timeout);
    }

    ieee80211_conn_running_release(conn);
    return state;
}
static enum conn_process_state ieee80211_conn_state_process(struct ieee80211_sub_if_data *sdata,void *cookie)
{
    struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;
    enum conn_process_state state = CONN_PROCESS__CONTINUE;

    /*
     * for compatible with th lower version of kernel
     */
    if(conn->running == false){
        state = __ieee80211_conn_state_process(sdata,cookie);
    }else {
        struct sk_buff *skb;

        skb = atbm_alloc_skb(0,GFP_KERNEL);

        if(skb){
            skb->pkt_type = IEEE80211_SDATA_QUEUE_MLME;
            skb->cb[0]    = MLME_REQ_PROCESS;

	        atbm_skb_queue_tail(&sdata->skb_queue,skb);
	        ieee80211_queue_work(&sdata->local->hw, &sdata->work);
        }else {        
            ifmgd->timeout = jiffies + msecs_to_jiffies(30);
            run_again(ifmgd,ifmgd->timeout);
        }
    }

    return state;
}
void ieee80211_sdata_queue_mlme_process(struct ieee80211_sub_if_data *sdata,struct sk_buff *skb)
{
    struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;

    ieee80211_sdata_mgd_lock(sdata,NULL);
    atbm_printk_always("%s:[%d]\n",__func__,skb->cb[0]);
    switch(skb->cb[0]){
    case MLME_CHANNEL_ASYNC_NOTIFY:
        if(conn->state != IEEE80211_CONN_CHAN_ASYNC){
            break;
        }
        atbm_fallthrough;
    case MLME_REQ_PROCESS:
        __ieee80211_conn_state_process(sdata,NULL);
        break;
    default:
        break;
    }
    ieee80211_sdata_mgd_unlock(sdata,NULL);
}
int ieee80211_mgd_assoc(struct ieee80211_sub_if_data* sdata,
	struct cfg80211_assoc_request* req)
{
	struct ieee80211_local* local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;

	struct ieee80211_if_managed* ifmgd = &sdata->u.mgd;
	struct ieee80211_bss* bss = (void*)req->bss->priv;
    struct ieee80211_mgd_assoc_req *assoc_req = NULL;
    unsigned int flags;
	const u8* ssid;
	const u8 *ht_information_ie;
	int i;
    int ret = 0;

    ieee80211_sdata_mgd_lock(sdata,req);

    if (ifmgd->associated) {
		if (!req->prev_bssid ||
			memcmp(req->prev_bssid, ifmgd->associated->bssid,
				ETH_ALEN)) {
			/*
			 * We are already associated and the request was not a
			 * reassociation request from the current BSS, so
			 * reject it.
			 */
			ret = -EALREADY;
            goto err;
		}

#ifdef CONFIG_MAC80211_ATBM_ROAMING_CHANGES
		ifmgd->roaming = 1;
#endif
		/* Trying to reassociate - clear previous association state */
		ieee80211_set_disassoc(sdata, true, false);
#ifdef CONFIG_MAC80211_ATBM_ROAMING_CHANGES
		ifmgd->roaming = 0;
#endif
	}

    assoc_req = atbm_kzalloc(sizeof(struct ieee80211_mgd_assoc_req)+req->ie_len,GFP_KERNEL);

    if(assoc_req == NULL){
        atbm_printk_err("%s:alloc assoc req err\n",__func__);
        ret = -ENOMEM;
        goto err;
    }

    flags = ifmgd->flags;
    flags &= ~(IEEE80211_STA_DISABLE_11N | IEEE80211_STA_DISABLE_HE | IEEE80211_STA_NULLFUNC_ACKED | IEEE80211_STA_DISABLE_VHT);

	ifmgd->beacon_crc_valid = false;

	for (i = 0; i < req->crypto.n_ciphers_pairwise; i++){
		if (req->crypto.ciphers_pairwise[i] == WLAN_CIPHER_SUITE_WEP40 ||
			req->crypto.ciphers_pairwise[i] == WLAN_CIPHER_SUITE_TKIP ||
			req->crypto.ciphers_pairwise[i] == WLAN_CIPHER_SUITE_WEP104) {
			flags |= IEEE80211_STA_DISABLE_11N | IEEE80211_STA_DISABLE_HE | IEEE80211_STA_DISABLE_VHT;
		}
    }
    if (req->ie && req->ie_len) {
		memcpy(assoc_req->ie, req->ie, req->ie_len);
		assoc_req->ie_len = req->ie_len;
	} else
		assoc_req->ie_len = 0;
    /*
     * copy ap ht_info used when send assoc req
     */
    ht_information_ie = ieee80211_bss_get_ie(req->bss, ATBM_WLAN_EID_HT_INFORMATION);
    if(ht_information_ie){
        if (ht_information_ie[1] < sizeof(struct atbm_ieee80211_ht_info)){
            flags |= IEEE80211_STA_DISABLE_11N;
        }else {
            memcpy(&assoc_req->ht_info,ht_information_ie+2,sizeof(struct atbm_ieee80211_ht_info));
        }
    }else {
        flags |= IEEE80211_STA_DISABLE_11N;
    }
    if (bss->wmm_used && bss->uapsd_supported &&
		(sdata->local->hw.flags & IEEE80211_HW_SUPPORTS_UAPSD)) {
		assoc_req->uapsd_used = true;
		flags |= IEEE80211_STA_UAPSD_ENABLED;
	} else {
		assoc_req->uapsd_used = false;
		flags &= ~IEEE80211_STA_UAPSD_ENABLED;
	}

    atbm_printk_err("%s:he_used(%d)\n",__func__,bss->he_used);
    if ((bss->he_used == false) || 
        (atbm_ieee80211_get_he_iftype_cap(&local->hw,local->hw.wiphy->bands[req->bss->channel->band],ieee80211_vif_type_p2p(&sdata->vif),sdata) == NULL)) {
		flags |= IEEE80211_STA_DISABLE_HE;
		 atbm_printk_err("%s:disable he\n",__func__);
	}
#ifdef CONFIG_ATBM_VHT
	if((hw_priv != NULL) && (hw_priv->chip_version != OCEANUS_NO_WIFI6))
	{
	    if((bss->vht_used == false) || (!atbm_ieee80211_vht_supported(&local->hw,local->hw.wiphy->bands[req->bss->channel->band])) || (enable_vht == 0)||((enable_vht == 1) && (hw_priv->channel->band == IEEE80211_BAND_2GHZ))){
	        atbm_printk_err("%s:bss->vht_used(%d),(%d),(%d)\n",__func__,bss->vht_used,atbm_ieee80211_vht_supported(&local->hw,local->hw.wiphy->bands[req->bss->channel->band]),enable_vht);
	        flags |= IEEE80211_STA_DISABLE_VHT;
	    }
	}
#endif
    if(!(flags & IEEE80211_STA_DISABLE_VHT)){
        const u8* vht_cap = ieee80211_bss_get_ie(req->bss,ATBM_WLAN_EID_VHT_CAPABILITY);

        atbm_printk_err("%s:vht [%p]\n",__func__,vht_cap);
        if(vht_cap){
            if(vht_cap[1] >= sizeof(struct atbm_ieee80211_vht_cap)){
                memcpy(&assoc_req->vht_cap,vht_cap + 2,sizeof(struct atbm_ieee80211_vht_cap));
            }else {
                flags |= IEEE80211_STA_DISABLE_VHT;
            }
        }else {
            flags |= IEEE80211_STA_DISABLE_VHT;
        }
    }
    assoc_req->chan   = req->bss->channel;
    assoc_req->use_he = !(flags & IEEE80211_STA_DISABLE_HE);
    assoc_req->use_vht = !(flags & IEEE80211_STA_DISABLE_VHT);
    assoc_req->bss = req->bss;
    assoc_req->use_11n    = !(flags & IEEE80211_STA_DISABLE_11N);
	assoc_req->capability = req->bss->capability;
	assoc_req->wmm_used   = bss->wmm_used && (local->hw.queues >= IEEE80211_NUM_ACS);
	assoc_req->supp_rates = bss->supp_rates;
	assoc_req->supp_rates_len = bss->supp_rates_len;

    atbm_printk_err("%s:[%pM],[%d][%d],[%d]\n",__func__,req->bss->bssid,assoc_req->use_11n,assoc_req->use_vht,assoc_req->use_he);
    memcpy(assoc_req->bssid, req->bss->bssid, ETH_ALEN);

    ssid = ieee80211_bss_get_ie(req->bss, ATBM_WLAN_EID_SSID);
    if(ssid && (IEEE80211_MAX_SSID_LEN >= ssid[1])){
	    memcpy(assoc_req->ssid, ssid + 2, ssid[1]);
	    assoc_req->ssid_len = ssid[1];
    }
	if (req->prev_bssid)
		memcpy(assoc_req->prev_bssid, req->prev_bssid, ETH_ALEN);

    if (!sdata->vif.p2p && !is_zero_ether_addr(assoc_req->prev_bssid)) {
		assoc_req->filter_fc = IEEE80211_STYPE_REASSOC_RESP;
	}else {
		assoc_req->filter_fc = IEEE80211_STYPE_ASSOC_RESP;
	}

    ret =  ieee80211_mgd_assoc_state_check(sdata,assoc_req);

    if(ret){
        goto err;
    }

    ifmgd->conn.assoc_req = assoc_req; 
    ifmgd->conn.bss = req->bss;
    ieee80211_atbm_handle_bss(sdata->local->hw.wiphy,req->bss);

#ifdef CONFIG_ATBM_SMPS
    if (ifmgd->req_smps == IEEE80211_SMPS_AUTOMATIC) {
		if (ifmgd->powersave)
			ifmgd->ap_smps = IEEE80211_SMPS_DYNAMIC;
		else
			ifmgd->ap_smps = IEEE80211_SMPS_OFF;
	} else{
		ifmgd->ap_smps = ifmgd->req_smps;
    }
	assoc_req->smps       = ifmgd->ap_smps;
#else
	assoc_req->smps       = IEEE80211_SMPS_OFF;
#endif
	if (req->use_mfp) {
		ifmgd->mfp = IEEE80211_MFP_REQUIRED;
		flags |= IEEE80211_STA_MFP_ENABLED;
	} else {
		ifmgd->mfp = IEEE80211_MFP_DISABLED;
		flags &= ~IEEE80211_STA_MFP_ENABLED;
	}

	if (req->crypto.control_port)
		flags |= IEEE80211_STA_CONTROL_PORT;
	else
		flags &= ~IEEE80211_STA_CONTROL_PORT;

	ifmgd->b_notransmit_bssid = atbm_iee80211_bss_is_nontrans(req->bss);

	if (ifmgd->b_notransmit_bssid) {
		memcpy(ifmgd->transmitter_bssid, atbm_iee80211_bss_transmit_bssid(req->bss), ETH_ALEN);
	}
	else {
		memcpy(ifmgd->transmitter_bssid, req->bss->bssid, ETH_ALEN);
	}

	sdata->control_port_protocol = req->crypto.control_port_ethertype;
	sdata->control_port_no_encrypt = req->crypto.control_port_no_encrypt;
    ifmgd->flags = flags;

    ieee80211_conn_state_process(sdata,req);
exit:
    ieee80211_sdata_mgd_unlock(sdata,req);
    return ret;
err:
    if(assoc_req){
        atbm_kfree(assoc_req);
    }
    goto exit;
}
int ieee80211_mgd_deauth(struct ieee80211_sub_if_data *sdata,
			 struct cfg80211_deauth_request *req,
			 void *cookie)
{
	struct ieee80211_local *local = sdata->local;
	struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;
	u8 bssid[ETH_ALEN];
    bool send_frame = true;

#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0)))
    send_frame = true;
#else
    send_frame = !req->local_state_change ? true:false;
#endif
    ieee80211_sdata_mgd_lock(sdata,req);

#if (LINUX_VERSION_IS_LESS_AND_NOT_CPTCFG(3, 4, 0))
	memcpy(bssid, req->bss->bssid, ETH_ALEN);
#else
	memcpy(bssid, req->bssid, ETH_ALEN);
#endif

    ieee80211_work_purge(sdata,&bssid[0],IEEE80211_WORK_MAX,true);

	if(ifmgd->associated&&(atbm_compare_ether_addr(ifmgd->associated->bssid, bssid)==0)){
        WARN_ON(conn->state != IEEE80211_CONN_CONNECTED);
        WARN_ON(conn->work != NULL);
        WARN_ON(conn->auth_req);
        WARN_ON(conn->assoc_req);
		ieee80211_set_disassoc(sdata, false, true);
	}else {
	
        if(conn->auth_req &&(atbm_compare_ether_addr(conn->auth_req->bssid, bssid)==0)){
            atbm_printk_err("%s:auth req abort\n",__func__);
            ieee80211_sdata_free_auth_data(sdata,conn->auth_req);
            ieee80211_conn_mark_idle(sdata);
        }

        if(conn->assoc_req && (atbm_compare_ether_addr(conn->assoc_req->bssid, bssid)==0)){
            atbm_printk_err("%s:assoc req abort\n",__func__);
            ieee80211_sdata_free_assoc_data(sdata,conn->assoc_req);
            ieee80211_conn_mark_idle(sdata);
        }

    }

	atbm_printk_always("%s: deauthenticating from %pM by local choice (reason=%d)\n",
		sdata->name, bssid, req->reason_code);

	ieee80211_send_deauth_disassoc(sdata, bssid, IEEE80211_STYPE_DEAUTH,
		req->reason_code,
		send_frame);

	sta_info_flush(local, sdata);

	mutex_lock(&local->mtx);
	ieee80211_recalc_idle(local);
	mutex_unlock(&local->mtx);

    ieee80211_sdata_mgd_unlock(sdata,req);
	return 0;
}

int ieee80211_mgd_disassoc(struct ieee80211_sub_if_data* sdata,
	struct cfg80211_disassoc_request* req,
	void* cookie)
{
	struct ieee80211_if_managed* ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;
	u8 bssid[ETH_ALEN];
    int ret = 0;

    ieee80211_sdata_mgd_lock(sdata,req);
	/*
	 * cfg80211 should catch this ... but it's racy since
	 * we can receive a disassoc frame, process it, hand it
	 * to cfg80211 while that's in a locked section already
	 * trying to tell us that the user wants to disconnect.
	 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	if (memcmp(ifmgd->associated->bssid, req->ap_addr, ETH_ALEN)) 
#else

	if (ifmgd->associated != req->bss) 
#endif
    {
        goto exit;
	}

	atbm_printk_mgmt("%s: disassociating from %pM by local choice (reason=%d)\n",
		sdata->name,ifmgd->associated->bssid, req->reason_code);
	
	memcpy(bssid, ifmgd->associated->bssid, ETH_ALEN);

	ieee80211_set_disassoc(sdata, false, true);

	ieee80211_send_deauth_disassoc(sdata, ifmgd->associated->bssid,
		IEEE80211_STYPE_DISASSOC, req->reason_code,!req->local_state_change);

    if(conn->auth_req &&(atbm_compare_ether_addr(conn->auth_req->bssid, bssid)==0)){
        atbm_printk_err("%s:auth req abort\n",__func__);
        ieee80211_sdata_free_auth_data(sdata,conn->auth_req);
    }

    if(conn->assoc_req && (atbm_compare_ether_addr(conn->assoc_req->bssid, bssid)==0)){
        atbm_printk_err("%s:assoc req abort\n",__func__);
        ieee80211_sdata_free_assoc_data(sdata,conn->assoc_req);
    }

	sta_info_flush(sdata->local, sdata);

	mutex_lock(&sdata->local->mtx);
	ieee80211_recalc_idle(sdata->local);
	mutex_unlock(&sdata->local->mtx);
exit:	
    ieee80211_sdata_mgd_unlock(sdata,req);
	return ret;
}
static void ieee80211_mgd_he_om(struct ieee80211_sub_if_data* sdata)
{
	struct ieee80211_htc_om htc;

	memset(&htc, 0, sizeof(struct ieee80211_htc_om));
	/*
	*set he A-Control
	*/
	htc.ht_control = 3;
	/*
	*om control
	*/
	htc.control_id = 1;
	/*
	*om fild
	*/
#ifdef  CONFIG_SUPPORT_80M_ADAPTER
	htc.channel_width = om_20M ? 0 :(ieee80211_chw_is_ht40(sdata->vif.bss_conf.channel_type)&&sdata->bSupport40M);
#else //CONFIG_SUPPORT_80M_ADAPTER
	htc.channel_width = om_20M ? 0 :ieee80211_chw_is_ht40(sdata->vif.bss_conf.channel_type);
#endif  //CONFIG_SUPPORT_80M_ADAPTER
	htc.ul_mu_disable = ul_mu_disable;
	htc.ul_mu_data_diable = ul_mu_data_diable;
	ieee80211_send_htc_qosnullfunc(&sdata->local->hw, &sdata->vif, *((u32*)&htc));
	atbm_printk_err("send htc qos null[%d]\n",ieee80211_chw_is_ht40(sdata->vif.bss_conf.channel_type));
}
static enum work_done_result ieee80211_om_work_done(struct ieee80211_work *wk,
						  struct sk_buff *skb)
{
	atbm_printk_err("om done\n");
	return WORK_DONE_DESTROY;
}

static enum work_action __must_check
ieee80211_wk_om_work_start(struct ieee80211_work *wk)
{
	
	wk->raw_work.tries ++;
	
	if(wk->raw_work.tries >= wk->dhcp.retry_max){
		
		return WORK_ACT_TIMEOUT;
	}

	ieee80211_mgd_he_om(wk->sdata);
	
	wk->timeout = jiffies + HZ/4;
	
	atbm_printk_always("om work(%d)\n",wk->raw_work.tries);
	return WORK_ACT_NONE;
}

void ieee80211_mgd_he_om_work(struct sta_info *sta)
{
	struct ieee80211_work *wk;
	struct ieee80211_sub_if_data *sdata = sta->sdata;
#ifdef  CONFIG_SUPPORT_80M_ADAPTER
	if((om == 0)&&(sdata->bSupport40M==1)){ 	
		return ;
	}
#else //CONFIG_SUPPORT_80M_ADAPTER	
	if(om == 0){		
		return ;
	}
#endif  //CONFIG_SUPPORT_80M_ADAPTER

	if((sdata->vif.type != NL80211_IFTYPE_STATION) || 
	   !test_sta_flag(sta, WLAN_STA_AUTHORIZED) || !test_sta_flag(sta, WLAN_STA_HE)){
	   return ;
	}
	
	wk = atbm_kzalloc(sizeof(struct ieee80211_work), GFP_ATOMIC);

	if(wk){
		struct ieee80211_channel_state *chan_state = ieee80211_get_channel_state(sdata->local, sdata);
		wk->type = IEEE80211_WORK_OM;
		wk->sdata     = sta->sdata;
		wk->done      = ieee80211_om_work_done;
		wk->start     = ieee80211_wk_om_work_start;
		wk->rx        = NULL;
		wk->chan      = chan_state->oper_channel;
		wk->chan_type = chan_state->_oper_channel_type;
		wk->chan_mode = CHAN_MODE_FIXED;
		wk->filter_fc = 0;
		wk->raw_work.tries = 0;
		wk->raw_work.retry_max = 5;
		
		memcpy(wk->filter_bssid,sdata->vif.addr,6);	
		memcpy(wk->filter_sa,sdata->vif.addr,6);	
		ieee80211_add_work(wk);
	} 
}
static enum work_done_result ieee80211_vht_action_done(struct ieee80211_work *wk,
						  struct sk_buff *skb)
{
	atbm_printk_err("vht action done\n");
	return WORK_DONE_DESTROY;
}
static void ieee80211_mgd_vht_action_send(struct ieee80211_sub_if_data* sdata)
{
    struct sk_buff *skb;
    struct atbm_ieee80211_mgmt *mgmt;

	skb = atbm_dev_alloc_skb(sizeof(*mgmt) + sdata->local->hw.extra_tx_headroom);

    if(skb == NULL){
        return;
    }

    atbm_skb_reserve(skb, sdata->local->hw.extra_tx_headroom);
	mgmt = (struct atbm_ieee80211_mgmt *) atbm_skb_put(skb, 24);
	memset(mgmt, 0, 24);
	memcpy(mgmt->da, sdata->u.mgd.bssid, ETH_ALEN);
	memcpy(mgmt->sa, sdata->vif.addr, ETH_ALEN);
	memcpy(mgmt->bssid, sdata->u.mgd.bssid, ETH_ALEN);

    mgmt->frame_control = cpu_to_le16(IEEE80211_FTYPE_MGMT|IEEE80211_STYPE_ACTION);
    atbm_skb_put(skb, 1 + sizeof(mgmt->u.action.u.vht_op_action));

    mgmt->u.action.category = ATBM_WLAN_CATEGORY_VHT;
    mgmt->u.action.u.vht_op_action.action_code = 2;
#ifndef ATBM_NOT_SUPPORT_40M_CHW
#ifdef  CONFIG_SUPPORT_80M_ADAPTER
    mgmt->u.action.u.vht_op_action.op_mode = om_20M?0:sdata->bSupport40M;
#else //CONFIG_SUPPORT_80M_ADAPTER
    mgmt->u.action.u.vht_op_action.op_mode = om_20M?0:1;
#endif  //CONFIG_SUPPORT_80M_ADAPTER
#else //ATBM_NOT_SUPPORT_40M_CHW
    mgmt->u.action.u.vht_op_action.op_mode = 0;
#endif  //ATBM_NOT_SUPPORT_40M_CHW
	ieee80211_tx_skb(sdata, skb);
}
static void ieee80211_mgd_ht_action_send(struct ieee80211_sub_if_data *sdata)
{
    struct sk_buff *skb;
    struct atbm_ieee80211_mgmt *mgmt;

	skb = atbm_dev_alloc_skb(sizeof(*mgmt) + sdata->local->hw.extra_tx_headroom);

    if(skb == NULL){
        return;
    }

    atbm_skb_reserve(skb, sdata->local->hw.extra_tx_headroom);
	mgmt = (struct atbm_ieee80211_mgmt *) atbm_skb_put(skb, 24);
	memset(mgmt, 0, 24);
	memcpy(mgmt->da, sdata->u.mgd.bssid, ETH_ALEN);
	memcpy(mgmt->sa, sdata->vif.addr, ETH_ALEN);
	memcpy(mgmt->bssid, sdata->u.mgd.bssid, ETH_ALEN);

    mgmt->frame_control = cpu_to_le16(IEEE80211_FTYPE_MGMT|IEEE80211_STYPE_ACTION);
    atbm_skb_put(skb, 1 + sizeof(mgmt->u.action.u.notify_chan_width));

    mgmt->u.action.category = ATBM_WLAN_CATEGORY_HT;
    mgmt->u.action.u.notify_chan_width.action_code = WLAN_HT_ACTION_NOTIFY_CHANWIDTH;
#ifndef ATBM_NOT_SUPPORT_40M_CHW
#ifdef  CONFIG_SUPPORT_80M_ADAPTER
		mgmt->u.action.u.notify_chan_width.chan_width = om_20M?0:sdata->bSupport40M;
#else //CONFIG_SUPPORT_80M_ADAPTER
		mgmt->u.action.u.notify_chan_width.chan_width = om_20M?0:1;
#endif  //CONFIG_SUPPORT_80M_ADAPTER

#else //ATBM_NOT_SUPPORT_40M_CHW
    mgmt->u.action.u.notify_chan_width.chan_width = 0;
#endif //ATBM_NOT_SUPPORT_40M_CHW
	ieee80211_tx_skb(sdata, skb);

}

static enum work_action __must_check
ieee80211_wk_vht_action_work_start(struct ieee80211_work *wk)
{
	
	wk->raw_work.tries ++;
	
	if(wk->raw_work.tries >= wk->raw_work.retry_max){
		
		return WORK_ACT_TIMEOUT;
	}

	ieee80211_mgd_vht_action_send(wk->sdata);
    ieee80211_mgd_ht_action_send(wk->sdata);
	wk->timeout = jiffies + HZ/4;
	
	atbm_printk_always("vht action work(%d)\n",wk->raw_work.tries);
	return WORK_ACT_NONE;
}

void ieee80211_mgd_vht_action_work(struct sta_info *sta)
{
    struct ieee80211_work *wk;
	struct ieee80211_sub_if_data *sdata = sta->sdata;
	
#ifdef  CONFIG_SUPPORT_80M_ADAPTER
	if((vht_action == 0)&&(sdata->bSupport40M==1)){		
		return ;
	}
#else //CONFIG_SUPPORT_80M_ADAPTER	
	if(vht_action == 0){		
		return ;
	}
#endif  //CONFIG_SUPPORT_80M_ADAPTER

	if( (sdata->vif.type != NL80211_IFTYPE_STATION) || 
	   !test_sta_flag(sta, WLAN_STA_AUTHORIZED) || !test_sta_flag(sta, WLAN_STA_VHT)){
	   return ;
	}

	wk = atbm_kzalloc(sizeof(struct ieee80211_work), GFP_ATOMIC);

    if(wk){
		struct ieee80211_channel_state *chan_state = ieee80211_get_channel_state(sdata->local, sdata);
		wk->type = IEEE80211_WORK_VHT_ACTION;
		wk->sdata     = sta->sdata;
		wk->done      = ieee80211_vht_action_done;
		wk->start     = ieee80211_wk_vht_action_work_start;
		wk->rx        = NULL;
		wk->chan      = chan_state->oper_channel;
		wk->chan_type = chan_state->_oper_channel_type;
		wk->chan_mode = CHAN_MODE_FIXED;
		wk->filter_fc = 0;
		wk->raw_work.tries = 0;
		wk->raw_work.retry_max = 5;
		
		memcpy(wk->filter_bssid,sdata->vif.addr,6);	
		memcpy(wk->filter_sa,sdata->vif.addr,6);	
		ieee80211_add_work(wk);
	} 

}
void ieee80211_sta_work(struct ieee80211_sub_if_data *sdata)
{
	struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
    struct ieee80211_conn *conn = &ifmgd->conn;

    ieee80211_sdata_mgd_lock(sdata,NULL);

    if(conn->auth_req || conn->assoc_req){

        if(time_is_after_jiffies(ifmgd->timeout) && 
           atbm_timer_pending(&ifmgd->timer)){
            run_again(ifmgd,ifmgd->timeout);
        }else {
            __ieee80211_conn_state_process(sdata,NULL);
        }
    }

    ieee80211_sdata_mgd_unlock(sdata,NULL);
    
}

/*****************************************************************************************************************************/
/*****************************atbm_vht_80M_adapter*************************************************************************/
/*****************************************************************************************************************************/
#ifdef  CONFIG_SUPPORT_80M_ADAPTER
#include "rc80211_hmac.h"
extern int chan_width;
void atbm_vht_80M_adapter_rx_check(struct atbmn_ieee80211_rx_status* hdr, struct sta_info* sta)
{

	struct ieee80211_sub_if_data* sdata = sta->sdata;
	RATE_CONTROL_RATE* rate_info = (RATE_CONTROL_RATE*)sta->rate_ctrl_priv;
	
	if((atbm_80M_adapter & ATBM_ADAPTER_HMAC) ==  0){
		return;
	}
	if(hdr->flag & RX_FLAG_MULTICAST){
		return;
	}
	
	// 80M is only send in 5G band
	if (hdr->band != IEEE80211_BAND_5GHZ){
		return;
	}	

	if ((sdata == NULL)
		|| sdata->bSupport40M_Fix 
		|| (!test_sta_flag(sta, WLAN_STA_40M_CH))
		|| (!sdata->bSupport40M)
		|| ((!test_sta_flag(sta, WLAN_STA_HE))&&(!test_sta_flag(sta, WLAN_STA_VHT)))
		|| ((sdata->vif.type != NL80211_IFTYPE_STATION) && (sdata->vif.type != NL80211_IFTYPE_P2P_CLIENT))) {

		//atbm_printk_always("80M_adapter--, %x, %x\n", (sdata == NULL),  (!test_sta_flag(sta, WLAN_STA_40M_CH)));
		if ((!sdata->bSupport40M) && time_after(jiffies, sdata->last_80M_adapter_time + 5 * HZ)) {
			atbm_printk_always("80M_adapter--:>b40M %d,rxflag %x,rx%d\n", sdata->bSupport40M, hdr->flag, hdr->rate_idx);
			sdata->last_80M_adapter_time = jiffies;
		}
		return;
	}
	if (rate_info == NULL) {
		return;
	}
	//printk("hdr->flag %x,FC%x,rate %d,rxcnt %d\n",hdr->flag,hdr->frame_ctrl,hdr->rate_idx, sdata->last_rx_num);
		
	if (hdr->flag & RX_FLAG_40MHZ) {
		if (hdr->flag & (RX_FLAG_VHT | RX_FLAG_HE)) {
			if (hdr->rate_idx >= 4) {
				sdata->highthrouputRateCnt++;
				//return;
			}
		}
		//vif->highthrouputRateCnt++;
		//return;
	}
	if (hdr->flag & (RX_FLAG_VHT | RX_FLAG_HE)) {
		if (hdr->rate_idx >= 4) {
			sdata->highthrouputRateCnt++;
			//return;
		}
	}

	if (hdr->flag & RX_FLAG_HT) {
		if (hdr->rate_idx >= 3) {
			sdata->highthrouputRateCnt++;
			//return;
		}
	}
	sdata->last_rx_num++;

	if (time_after(jiffies, sdata->last_80M_adapter_time + 1 * HZ)) {
		if (sdata->highthrouputRateCnt) {
			//if (time_after(jiffies, vif->last_80M_adapter_time + 1 * HZ)) {
				//vif->highthrouputRateCnt = 0;
				//vif->last_80M_adapter_time = jiffies;
			//}
			//return;
			atbm_printk_always("80M_adapter:RateCnt0,rxflag %x,rx%d\n", hdr->flag, hdr->rate_idx);
			if( (sdata->highthrouputRateCnt > 900)
					&& (rate_info->min_rssi > -50)
					&& ((sdata->last_rx_num)>1000)){
				sdata->bSupport40M_Fix = 1;
			}
		}
		else {

			if (((sdata->last_rx_num) > 50)
				|| ((rate_info->total_tx_counter - sdata->last_tx_num) > 50)) {

				u8 maxrate_id = rate_info->max_tp_rate_idx;
				bool b_highthroughput = 0;
				if (maxrate_id >= RATE_INDEX_VHT_MC0) {
					b_highthroughput = maxrate_id >= RATE_INDEX_VHT_MC5;
				}
				else if (maxrate_id >= RATE_INDEX_HE_MC0) {
					b_highthroughput = maxrate_id >= RATE_INDEX_HE_MC5;

				}
				else if (maxrate_id >= RATE_INDEX_N_6_5M) {
					b_highthroughput = maxrate_id >= RATE_INDEX_N_39M;
				}
				else {
				}
				if (b_highthroughput
					&& (sdata->highthrouputRateCnt == 0)
					&& (rate_info->min_rssi > -50)
					&& ((sdata->last_rx_num)>10)) {
					sdata->bSupport40M_Next = 0;
					atbm_printk_always("80M_adapter:om_20M txrate%d,rcnt%d,rxflag %x,rx%d\n", maxrate_id,sdata->last_rx_num, hdr->flag, hdr->rate_idx);
					//ieee80211_mgd_he_om_work(sta);
					//ieee80211_mgd_vht_action_work(sta);
					
			        if(ieee80211_sdata_running(sdata)){
			            ieee80211_connection_loss(&sdata->vif);
			        }
				}
				//sdata->last_rx_num = rate_info->received_num;
				//sdata->last_tx_num = rate_info->total_tx_counter;

			}
			//sdata->last_rx_num = rate_info->received_num;
			//sdata->last_tx_num = rate_info->total_tx_counter;
			atbm_printk_always("80M_adapter:HRateCnt%d,rxflag %x,rxrate%d,rx/txCnt %d:%d,TxRate%d\n",
																			sdata->highthrouputRateCnt, 
																			hdr->flag, hdr->rate_idx,
																			sdata->last_rx_num, 
																			(rate_info->total_tx_counter - sdata->last_tx_num), 
																			rate_info->max_tp_rate_idx);
		}

		sdata->last_rx_num = 0;
		sdata->last_tx_num = rate_info->total_tx_counter;

		sdata->highthrouputRateCnt = 0;
		sdata->last_80M_adapter_time = jiffies;
		
	}
}
void atbm_vht_80M_adapter_vif_assoc( struct sta_info* sta)
{
	struct ieee80211_sub_if_data* sdata = sta->sdata;

	if ((sdata->vif.type == NL80211_IFTYPE_STATION)||(sdata->vif.type == NL80211_IFTYPE_P2P_CLIENT)){
	
		RATE_CONTROL_RATE* rate_info = (RATE_CONTROL_RATE*)sta->rate_ctrl_priv;
		
		sdata->highthrouputRateCnt = 0;
		sdata->last_80M_adapter_time = jiffies;
	
		if (rate_info != NULL) {
			sdata->last_tx_num = rate_info->total_tx_counter;
		}
		else {
			sdata->last_tx_num = 0;
		}
		sdata->last_rx_num = 0;
	}
}
//only call in station mode
void atbm_vht_80M_adapter_vif_init(struct ieee80211_sub_if_data* sdata,bool isVHT,u8 *bssid)
{
	bool isSameBssid = memcmp(sdata->last_bssid,bssid,6);
	//if ((sdata->vif.type == NL80211_IFTYPE_STATION)||(sdata->vif.type == NL80211_IFTYPE_P2P_CLIENT)){
	sdata->highthrouputRateCnt = 0;
	sdata->bSupport40M_Fix = 0;
	sdata->last_80M_adapter_time = jiffies;
	if(isSameBssid !=0){
		sdata->bSupport40M = chan_width;
		sdata->bSupport40M_Next = chan_width;
		atbm_printk_always("80M_adapter:"ATBM_MACSTR "|" ATBM_MACSTR " used default %d\n",ATBM_MAC2STR(bssid),ATBM_MAC2STR(sdata->last_bssid),isSameBssid);
		memcpy(sdata->last_bssid, bssid, ETH_ALEN);
	}
	else{	
		sdata->bSupport40M = sdata->bSupport40M_Next;
		//used last  check  bSupport40M
		atbm_printk_always("80M_adapter:same "ATBM_MACSTR" use last\n",ATBM_MAC2STR(bssid));
	} 
	if(!isVHT){
		sdata->bSupport40M = chan_width;
	}
	//}
}

#endif  //#ifdef  CONFIG_SUPPORT_80M_ADAPTER

// EXPORT_SYMBOL(ieee80211_cqm_rssi_notify);
#ifdef CONFIG_ATBM_RADAR_DETECT
#if LINUX_VERSION_IS_GEQ_OR_CPTCFG(3, 9, 0)
void ieee80211_dfs_cac_timer_work(struct atbm_work_struct* work)
{
	struct atbm_delayed_work* delayed_work = atbm_to_delayed_work(work);
	struct ieee80211_sub_if_data* sdata =
		container_of(atbm_delayed_work, struct ieee80211_sub_if_data,
			dfs_cac_timer_work);

	mutex_lock(&sdata->local->mtx);
	sdata->radar_required = false;
	if (sdata->wdev.cac_started) {
		sdata->local->hw.conf.radar_enabled = 0;
		ieee80211_hw_config(sdata->local, IEEE80211_CONF_CHANGE_CHANNEL);
#if LINUX_VERSION_IS_GEQ_OR_CPTCFG(3, 14, 0)
		cfg80211_cac_event(sdata->dev, &sdata->local->dfs_cac_chan_def,
			NL80211_RADAR_CAC_FINISHED,
			GFP_KERNEL);
#else
		cfg80211_cac_event(sdata->dev, NL80211_RADAR_CAC_FINISHED,
			GFP_KERNEL);
#endif
	}
	mutex_unlock(&sdata->local->mtx);
}
void ieee80211_dfs_cac_abort(struct ieee80211_sub_if_data* sdata)
{
	atbm_cancel_delayed_work_sync(&sdata->dfs_cac_timer_work);
	sdata->radar_required = false;
	if (sdata->wdev.cac_started) {
		sdata->local->hw.conf.radar_enabled = 0;
		ieee80211_hw_config(sdata->local, IEEE80211_CONF_CHANGE_CHANNEL);
#if LINUX_VERSION_IS_GEQ_OR_CPTCFG(3, 14, 0)
		cfg80211_cac_event(sdata->dev, &sdata->local->dfs_cac_chan_def, NL80211_RADAR_CAC_ABORTED,
			GFP_KERNEL);
#else
		cfg80211_cac_event(sdata->dev, NL80211_RADAR_CAC_ABORTED,
			GFP_KERNEL);
#endif
	}
}
#endif
#endif
#ifdef CONFIG_ATBM_HE

/**
 * atbm_ieee80211_get_he_iftype_cap - return HE capabilities for an sband's iftype
 * @sband: the sband to search for the iftype on
 * @iftype: enum nl80211_iftype
 *
 * Return: pointer to the struct ieee80211_sta_he_cap, or NULL is none found
 */
const struct atbm_ieee80211_sta_he_cap *
atbm_ieee80211_get_he_iftype_cap(struct ieee80211_hw *hw,const struct ieee80211_supported_band *sband,
			    u8 iftype,struct ieee80211_sub_if_data *sdata)
{
    struct atbm_ieee80211_sta_he_cap *he_cap = NULL;

	if((sband->band == IEEE80211_BAND_2GHZ) && hw->he_cap_2g4){
		he_cap = hw->he_cap_2g4;
	}else if((sband->band == IEEE80211_BAND_5GHZ) && hw->he_cap_5g){
		he_cap = hw->he_cap_5g;
	}

    if(he_cap == NULL){
        return NULL;
    }

    if(beamforming_on){
        atbm_printk_mgmt(KERN_ALERT "beamforming on\n");
        he_cap->he_cap_elem.phy_cap_info[4] |= (ATBM_IEEE80211_HE_PHY_CAP4_SU_BEAMFORMEE |\
                    ATBM_IEEE80211_HE_PHY_CAP4_BEAMFORMEE_MAX_STS_UNDER_80MHZ_4); /*IEEE80211_HE_PHY_CAP4_SU_BEAMFORMEE |
                    IEEE80211_HE_PHY_CAP4_BEAMFORMEE_MAX_STS_UNDER_80MHZ_4*/
        he_cap->he_cap_elem.phy_cap_info[6] |= (ATBM_IEEE80211_HE_PHY_CAP6_TRIG_SU_BEAMFORMING_FB |\
                ATBM_IEEE80211_HE_PHY_CAP6_TRIG_MU_BEAMFORMING_PARTIAL_BW_FB|\
                ATBM_IEEE80211_HE_PHY_CAP6_TRIG_CQI_FB);
    }else{
        atbm_printk_mgmt(KERN_ALERT "beamforming off\n");
        he_cap->he_cap_elem.phy_cap_info[4] &= ~(ATBM_IEEE80211_HE_PHY_CAP4_SU_BEAMFORMEE |\
                    ATBM_IEEE80211_HE_PHY_CAP4_BEAMFORMEE_MAX_STS_UNDER_80MHZ_4); /*IEEE80211_HE_PHY_CAP4_SU_BEAMFORMEE |
                    IEEE80211_HE_PHY_CAP4_BEAMFORMEE_MAX_STS_UNDER_80MHZ_4*/
        he_cap->he_cap_elem.phy_cap_info[6] &= ~(ATBM_IEEE80211_HE_PHY_CAP6_TRIG_SU_BEAMFORMING_FB |\
                ATBM_IEEE80211_HE_PHY_CAP6_TRIG_MU_BEAMFORMING_PARTIAL_BW_FB|\
                ATBM_IEEE80211_HE_PHY_CAP6_TRIG_CQI_FB);
    }

    switch(he_rx_mcs){
    case 0:
        he_cap->he_mcs_nss_supp.rx_mcs_80 = cpu_to_le16(0xfffc);
        break;
    case 1:
        he_cap->he_mcs_nss_supp.rx_mcs_80 = cpu_to_le16(0xfffd);
        break;
    case 2:
        he_cap->he_mcs_nss_supp.rx_mcs_80 = cpu_to_le16(0xfffe);
        break;
    default:
        break;
    }

    switch(he_tx_mcs){
    case 0:
        he_cap->he_mcs_nss_supp.tx_mcs_80 = cpu_to_le16(0xfffc);
        break;
    case 1:
        he_cap->he_mcs_nss_supp.tx_mcs_80 = cpu_to_le16(0xfffd);
        break;
    case 2:
        he_cap->he_mcs_nss_supp.tx_mcs_80 = cpu_to_le16(0xfffe);
        break;
    default:
        break;
    }
	//printk("----%s------>sdata->bSupport40M %d\n",__func__,sdata->bSupport40M);

    switch(sdata->bSupport40M){
    case 0:
        if(sband->band == IEEE80211_BAND_2GHZ){
            he_cap->he_cap_elem.phy_cap_info[0] &= ~ATBM_IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_40MHZ_IN_2G;
        }else{ 
            he_cap->he_cap_elem.phy_cap_info[0] &= ~ATBM_IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_40MHZ_80MHZ_IN_5G;
        }
        break;
    case 1:
        if(sband->band == IEEE80211_BAND_2GHZ){
            he_cap->he_cap_elem.phy_cap_info[0] |= ATBM_IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_40MHZ_IN_2G;
        }else{ 
            he_cap->he_cap_elem.phy_cap_info[0] |= ATBM_IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_40MHZ_80MHZ_IN_5G;
        }
        break;
    default:
        break;
    }

    atbm_printk_init("%s:he_rx_mcs(%x),he_tx_mcs(%x),phy_cap_info[%x]\n",__func__,he_cap->he_mcs_nss_supp.rx_mcs_80,he_cap->he_mcs_nss_supp.tx_mcs_80,he_cap->he_cap_elem.phy_cap_info[0]);
	return he_cap;
}
#else //CONFIG_ATBM_HE
const struct atbm_ieee80211_sta_he_cap *
atbm_ieee80211_get_he_iftype_cap(struct ieee80211_hw *hw,const struct ieee80211_supported_band *sband,
			    u8 iftype,struct ieee80211_sub_if_data *sdata)
{
	return NULL;
}

#endif
#ifdef CONFIG_ATBM_VHT
const struct atbm_ieee80211_sta_vht_cap *
atbm_ieee80211_get_vht_cap(struct ieee80211_hw *hw,const struct ieee80211_supported_band *sband,struct ieee80211_sub_if_data *sdata)
{
	struct atbm_common *hw_priv = hw->priv;
    struct atbm_ieee80211_sta_vht_cap *vht_cap = NULL;
    if((hw_priv != NULL) && (hw_priv->chip_version != OCEANUS_NO_WIFI6))
	{
		if((sband->band == IEEE80211_BAND_2GHZ) && hw->vht_cap_2g4){
			vht_cap = hw->vht_cap_2g4;
		}else if((sband->band == IEEE80211_BAND_5GHZ) && hw->vht_cap_5g){
			vht_cap = hw->vht_cap_5g;
		}

		if(vht_cap == NULL){
			return NULL;
		}

		if(beamforming_on){
			vht_cap->cap |= (ATBM_IEEE80211_VHT_CAP_SU_BEAMFORMER_CAPABLE | ATBM_IEEE80211_VHT_CAP_SU_BEAMFORMEE_CAPABLE);
		}else {
			vht_cap->cap &= ~(ATBM_IEEE80211_VHT_CAP_SU_BEAMFORMER_CAPABLE | ATBM_IEEE80211_VHT_CAP_SU_BEAMFORMEE_CAPABLE);
		}
		
		//printk("----%s------>sdata->bSupport40M %d\n",__func__,sdata->bSupport40M);
		switch(sdata->bSupport40M){
		case 0:
			vht_cap->vht_mcs.rx_mcs_map = cpu_to_le16(ATBM_IEEE80211_VHT_MCS_SUPPORT_0_8 << 0 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 2 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 4 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 6 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 8 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 10 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 12 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 14);
			vht_cap->vht_mcs.tx_mcs_map = cpu_to_le16(ATBM_IEEE80211_VHT_MCS_SUPPORT_0_8 << 0 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 2 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 4 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 6 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 8 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 10 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 12 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 14);

			break;
		case 1:
			vht_cap->vht_mcs.rx_mcs_map = cpu_to_le16(ATBM_IEEE80211_VHT_MCS_SUPPORT_0_9 << 0 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 2 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 4 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 6 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 8 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 10 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 12 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 14);
			vht_cap->vht_mcs.tx_mcs_map = cpu_to_le16(ATBM_IEEE80211_VHT_MCS_SUPPORT_0_9 << 0 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 2 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 4 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 6 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 8 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 10 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 12 |
													  ATBM_IEEE80211_VHT_MCS_NOT_SUPPORTED << 14);
			break;
		default:
			break;
		}

	}
    return vht_cap;
}
#else
const struct atbm_ieee80211_sta_vht_cap *
atbm_ieee80211_get_vht_cap(struct ieee80211_hw *hw,const struct ieee80211_supported_band *sband,struct ieee80211_sub_if_data *sdata)
{
    return NULL;
}
#endif

// EXPORT_SYMBOL(ieee80211_get_operstate);
