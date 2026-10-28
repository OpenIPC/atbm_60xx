/*
 * Copyright 2002-2005, Instant802 Networks, Inc.
 * Copyright 2005, Devicescape Software, Inc.
 * Copyright (c) 2006 Jiri Benc <jbenc@suse.cz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef IEEE80211_RATE_H
#define IEEE80211_RATE_H
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/types.h>
#include <linux/kref.h>
#include <net/atbm_mac80211.h>
#include "ieee80211_i.h"
#include "sta_info.h"
#include "../apollo.h"
#include "../internal_cmd.h"

struct rate_control_ref {
	struct ieee80211_local *local;
	struct atbm_rate_control_ops *ops;
	void *priv;
	struct kref kref;
};

void rate_control_get_rate(struct ieee80211_sub_if_data *sdata,
			   struct sta_info *sta,
			   struct ieee80211_tx_rate_control *txrc);
struct rate_control_ref *rate_control_get(struct rate_control_ref *ref);
void rate_control_put(struct rate_control_ref *ref);

/*****************************************************************************************************************************/
/*****************************atbm_vht_80M_adapter*************************************************************************/
/*****************************************************************************************************************************/

void atbm_vht_80M_adapter_rx_check(struct atbmn_ieee80211_rx_status* hdr, struct sta_info* sta);
void atbm_vht_80M_adapter_vif_init(struct ieee80211_sub_if_data* sdata,bool isVHT,u8 *bssid);
void atbm_vht_80M_adapter_vif_assoc( struct sta_info* sta);


#if 0
static inline void rate_control_tx_status(struct ieee80211_local *local,
					  struct ieee80211_supported_band *sband,
					  struct sta_info *sta,
					  struct sk_buff *skb)
{

/*
	struct rate_control_ref *ref = local->rate_ctrl;
	struct ieee80211_sta *ista = &sta->sta;
	struct atbm_ieee80211_sta *atbm_sta; 
	void *priv_sta = sta->rate_ctrl_priv;

	if (!ref)
		return;

		*/
 //not used function
	
  // atbm_sta = ( struct atbm_ieee80211_sta *)ista;

   //ref->ops->tx_status(ref->priv, sband, atbm_sta, priv_sta, skb);
	
}
#endif
static inline void rate_control_rx_status(struct ieee80211_local *local,
					  struct ieee80211_supported_band *sband,
					  struct sta_info *sta,
					  struct sk_buff *skb)
{
	struct rate_control_ref *ref = local->rate_ctrl;
	struct ieee80211_sta *ista = &sta->sta;
    struct atbm_ieee80211_sta *atbm_sta; 
	void *priv_sta = sta->rate_ctrl_priv;
    struct atbmn_ieee80211_rx_status * atbm_status;
    struct ieee80211_rx_status * status;

	if (!ref)
		return;
    
    status = IEEE80211_SKB_RXCB(skb);
    atbm_status = (struct atbmn_ieee80211_rx_status *)status;
	atbm_sta = ( struct atbm_ieee80211_sta *)ista;

	ref->ops->rx_status(ref->priv, priv_sta, atbm_status);
#ifdef  CONFIG_SUPPORT_80M_ADAPTER
	atbm_vht_80M_adapter_rx_check(atbm_status, sta);
#endif  //#ifdef  CONFIG_SUPPORT_80M_ADAPTER
}

static inline void rate_control_tx_hmac(struct ieee80211_local *local,
					  struct sta_info *sta)
{
	struct rate_control_ref *ref = local->rate_ctrl;
	struct ieee80211_sta *ista = &sta->sta;
	void *priv_sta = sta->rate_ctrl_priv;
	struct atbm_ieee80211_sta atbm_sta; 

	if (!ref)
		return;

	if(priv_sta == NULL)
	{
		return;
	}


	//atbm_sta = ( struct atbm_ieee80211_sta *)ista;
    
    atbm_sta.chip_category = ista->chip_category;
    atbm_sta.n_rates = ista->n_rates;
    atbm_sta.txs_retrys = ista->txs_retrys;

    atbm_sta.he_cap = &ista->he_cap;
    atbm_sta.ht_cap = &ista->ht_cap;
    atbm_sta.vht_cap = &ista->vht_cap;
    
	ref->ops->hmac_tx_status(ref->priv, &atbm_sta, priv_sta);
}

static inline void rate_control_rate_init(struct sta_info *sta)
{
	struct ieee80211_local *local = sta->sdata->local;
	struct ieee80211_channel_state *chan_state = ieee80211_get_channel_state(local, sta->sdata);
	struct rate_control_ref *ref = sta->rate_ctrl;
	struct ieee80211_sta *ista = &sta->sta;
	void *priv_sta = sta->rate_ctrl_priv;
	struct ieee80211_supported_band *sband;
    struct atbmwifi_band_info band_info;
	struct atbm_ieee80211_sta atbm_sta; 

	if (!ref)
		return;

	sband = local->hw.wiphy->bands[chan_state->conf.channel->band];

    band_info.center_freq = chan_state->conf.channel->center_freq;
    
    band_info.channel_type = (enum atbm_nl80211_channel_type)chan_state->_oper_channel_type;
    band_info.channel_type = sta->sdata->vif.bss_conf.channel_type;

    memcpy((u8*)(&atbm_sta.supp_rates[0]), (u8*)(&ista->supp_rates[0]),  IEEE80211_NUM_BANDS*sizeof(u32));
    atbm_sta.chip_category = ista->chip_category;
    atbm_sta.n_rates = ista->n_rates;
    atbm_sta.txs_retrys = ista->txs_retrys;
 
    atbm_sta.he_cap = &ista->he_cap;
    atbm_sta.ht_cap = &ista->ht_cap;
    atbm_sta.vht_cap = &ista->vht_cap;
      
    if(!test_sta_flag(sta,WLAN_STA_40M_CH))  //peer not support 40M
     {
        if((band_info.channel_type == ATBM_NL80211_CHAN_HT40PLUS)||(band_info.channel_type == ATBM_NL80211_CHAN_HT40MINUS))  
         {
              band_info.channel_type = ATBM_NL80211_CHAN_HT20;
         }
     }

	atbm_printk_rc(" rate_control_rate_init chip_category:%d\n", ista->chip_category);

	ref->ops->rate_init(ref->priv, &band_info, &atbm_sta, priv_sta);
}

static inline void rate_control_rate_update(struct ieee80211_local *local,
				    struct ieee80211_supported_band *sband,
				    struct sta_info *sta, u32 changed,
				    enum nl80211_channel_type oper_chan_type)
{
	struct rate_control_ref *ref = local->rate_ctrl;
	struct ieee80211_sta *ista = &sta->sta;
	struct atbm_ieee80211_sta atbm_sta; 
	void *priv_sta = sta->rate_ctrl_priv;
    enum atbm_nl80211_channel_type atbm_oper_chan_type = (enum atbm_nl80211_channel_type)oper_chan_type;
    struct atbmwifi_band_info band_info;  
    struct ieee80211_channel_state *chan_state = ieee80211_get_channel_state(local, sta->sdata);


    /*Porting part: copy to support rates and capability to atbm_sta*/
	//atbm_sta = ( struct atbm_ieee80211_sta *)ista;
    memcpy((u8*)(&atbm_sta.supp_rates[0]), (u8*)(&ista->supp_rates[0]),  IEEE80211_NUM_BANDS*sizeof(u32));
    
   // memcpy((u8*)(atbm_sta.ht_cap), (u8*)(ista->ht_cap),  sizeof(struct ieee80211_sta_ht_cap));
    //memcpy((u8*)(atbm_sta.he_cap), (u8*)(ista->he_cap),  sizeof(struct atbm_ieee80211_sta_he_cap ));
   // memcpy((u8*)(atbm_sta.vht_cap), (u8*)(ista->vht_cap),  sizeof(struct ieee80211_sta_vht_cap));
   
    atbm_sta.ht_cap =  (struct ieee80211_sta_ht_cap*)&ista->ht_cap;
    atbm_sta.vht_cap = (struct atbm_ieee80211_sta_vht_cap*)&ista->vht_cap;
    atbm_sta.he_cap =  (struct atbm_ieee80211_sta_he_cap*)&ista->he_cap;
   
    atbm_sta.chip_category = ista->chip_category;
    
    band_info.center_freq = chan_state->conf.channel->center_freq;;    
    band_info.channel_type = oper_chan_type; //for tx used.
    if(!test_sta_flag(sta,WLAN_STA_40M_CH))  //peer not support 40M
    {
       if((band_info.channel_type == ATBM_NL80211_CHAN_HT40PLUS)||(band_info.channel_type == ATBM_NL80211_CHAN_HT40MINUS))  
        {
             band_info.channel_type = ATBM_NL80211_CHAN_HT20;
        }
    }
   /*end of porting part*/
    	atbm_printk_rc(" rate_control_rate_update hip_category:%d\n",  ista->chip_category);
	if (ref && ref->ops->rate_update)
		ref->ops->rate_update(ref->priv, &band_info, &atbm_sta,
				      priv_sta, changed, atbm_oper_chan_type);
}

static inline void *rate_control_alloc_sta(struct rate_control_ref *ref,
					   struct ieee80211_sta *sta,
					   gfp_t gfp)
{
    struct atbm_ieee80211_sta atbm_sta;  

    void *p;
	/*
	struct atbm_sta_priv *sta_priv =
			(struct atbm_sta_priv *)&sta->drv_priv; //ap interface
	struct atbm_vif *priv_vif = (struct atbm_vif *)sta_priv->priv;    
    struct atbm_common *hw_priv = (struct atbm_common *)priv_vif->hw_priv;
*/
	
	atbm_printk_rc(" rate_control_alloc_sta chip_category:%d\n", sta->chip_category);
   // atbm_sta = (struct atbm_ieee80211_sta *)sta;  
	//atbm_sta->chip_category = hw_priv->chip_cat;
	
    //atbm_printk_rc(" rate_control_alloc_sta atbm_sta:0x%x\n", atbm_sta);
	p = ref->ops->alloc_sta(ref->priv, &atbm_sta, gfp);
    /*pass txs_retrys and n_rates to sta*/
	sta->txs_retrys = atbm_sta.txs_retrys;
	sta->n_rates =  atbm_sta.n_rates;

	
	//atbm_printk_rc(" rate_control_alloc_sta atbm_sta:0x%x txs_retrys:0x%x 0x%x\n", atbm_sta, atbm_sta.txs_retrys, sta->txs_retrys);
	return p;
}

static inline void rate_control_free_sta(struct sta_info *sta)
{
	struct rate_control_ref *ref = sta->rate_ctrl;
	//struct ieee80211_sta *ista = &sta->sta;
	void *priv_sta = sta->rate_ctrl_priv;

	ref->ops->free_sta(ref->priv, priv_sta);
	sta->rate_ctrl_priv = NULL;
}

static inline void rate_control_add_sta_debugfs(struct sta_info *sta)
{
#ifdef CONFIG_MAC80211_ATBM_DEBUGFS
	struct rate_control_ref *ref = sta->rate_ctrl;
	if (ref && sta->debugfs.dir && ref->ops->add_sta_debugfs)
		ref->ops->add_sta_debugfs(ref->priv, sta->rate_ctrl_priv,
					  sta->debugfs.dir);
#endif
}

static inline void rate_control_remove_sta_debugfs(struct sta_info *sta)
{
#ifdef CONFIG_MAC80211_ATBM_DEBUGFS
	struct rate_control_ref *ref = sta->rate_ctrl;
	if (ref && ref->ops->remove_sta_debugfs)
		ref->ops->remove_sta_debugfs(ref->priv, sta->rate_ctrl_priv);
#endif
}

/* Get a reference to the rate control algorithm. If `name' is NULL, get the
 * first available algorithm. */
int atbm_ieee80211_init_rate_ctrl_alg(struct ieee80211_local *local,
				 const char *name);
void rate_control_deinitialize(struct ieee80211_local *local);
int  atbm_rc80211_hmac_init(void);
void atbm_rc80211_hmac_exit(void);
int hmac_get_txrx_status_wh(struct sta_info* sta_info, struct TX_RX_Statistics_S* data);

/* Rate control algorithms */
#endif /* IEEE80211_RATE_H */
