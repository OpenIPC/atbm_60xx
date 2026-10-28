/*
 * Scan implementation for altobeam APOLLO mac80211 drivers
 *
 * Copyright (c) 2016, altobeam
 *
 * Based on:
 * Copyright (c) 2010, ST-Ericsson
 * Author: Dmitry Tarnyagin <dmitry.tarnyagin@stericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/sched.h>
#include "apollo.h"
#include "scan.h"
#include "sta.h"
#include "pm.h"
#include "bh.h"
#include "atbm_p2p.h"
//#ifdef CONFIG_WIRELESS_EXT
extern int etf_v2_scan_end(struct atbm_common *hw_priv, struct ieee80211_vif *vif );
extern void etf_v2_scan_rx(struct atbm_common *hw_priv,struct sk_buff *skb,u8 rssi );
//#endif

static int atbm_scan_start(struct atbm_vif *priv, struct wsm_scan *scan)
{
	int ret, i;
	int tmo = 9000;//reserve more time budget for extreme case, such as STA+AP+BLE
	struct atbm_common *hw_priv = ABwifi_vifpriv_to_hwpriv(priv);

	for (i = 0; i < scan->numOfChannels; ++i)
		tmo += scan->ch[i].maxChannelTime + 10;

	atomic_set(&hw_priv->scan.in_progress, 1);
	atomic_set(&hw_priv->recent_scan, 1);
#ifndef CONFIG_WAKELOCK
#ifdef CONFIG_PM
	atbm_pm_stay_awake(&hw_priv->pm_state, tmo * HZ / 1000);
#endif
#endif
	atbm_hw_priv_queue_delayed_work(hw_priv, &hw_priv->scan.timeout,
		tmo * HZ / 1000);

	hw_priv->scan.wait_complete = 1;

	ret = wsm_scan(hw_priv, scan, priv->if_id);
	
	if (unlikely(ret)) {
		hw_priv->scan.wait_complete = 0;
		atomic_set(&hw_priv->scan.in_progress, 0);
	    atomic_xchg(&hw_priv->recent_scan, 0);
		atbm_hw_cancel_delayed_work(&hw_priv->scan.timeout,false);
	}

	return ret;
}
int atbm_hw_scan(struct ieee80211_hw *hw,
		   struct ieee80211_vif *vif,
		   struct ieee80211_scan_req_wrap *req_wrap)
{
	struct atbm_common *hw_priv = hw->priv;
	struct atbm_vif *priv = ABwifi_get_vif_from_ieee80211(vif);
	struct wsm_template_frame frame = {
		.frame_type = WSM_FRAME_TYPE_PROBE_REQUEST,
	};
	int i;
	int ret = 0;
    struct wsm_scan scan = {
		.scanType = WSM_SCAN_TYPE_FOREGROUND,
		.scanFlags = 0, /* TODO:COMBO */
		//.scanFlags = WSM_SCAN_FLAG_SPLIT_METHOD, /* TODO:COMBO */
	};
	u32 ProbeRequestTime  = 10;
	u32 ChannelRemainTime = 20;
	u32 maxChannelTime;
	int scan_status = 0;
    struct ieee80211_channel *first;

	if(atbm_bh_is_term(hw_priv)){
		ret = -EOPNOTSUPP;
        goto exit;
	}

	if(atomic_read(&priv->enabled) == 0){
		atbm_printk_err("%s:disabled\n",__func__);
		ret = -EOPNOTSUPP;
        goto exit;
	}
	/* Scan when P2P_GO corrupt firmware MiniAP mode */
	if (priv->join_status == ATBM_APOLLO_JOIN_STATUS_AP)
	{
		ret = -EOPNOTSUPP;
        goto exit;
	}

	if (req_wrap->req->n_ssids == 1 && !req_wrap->req->ssids[0].ssid_len)
		req_wrap->req->n_ssids = 0;

	atbm_printk_debug("[SCAN] Scan request for %d SSIDs.\n",req_wrap->req->n_ssids);

	if (req_wrap->req->n_ssids > hw->wiphy->max_scan_ssids){
		ret = -EINVAL;
        goto exit;
	}

    if (req_wrap->req->n_channels > hw->max_scan_channels_support){
        ret = -EINVAL;
        goto exit;
    }

	frame.skb = ieee80211_probereq_get(hw, vif, NULL, 0,
		req_wrap->req->ie, req_wrap->req->ie_len,
		req_wrap->flags & IEEE80211_SCAN_REQ_NEED_BSSID ? req_wrap->bssid:NULL);

	if (!frame.skb){
        ret = -ENOMEM;
        goto exit;
    }

	/*
	*supplicant requres scan,we must stay awake.
	*/
	atbm_hold_suspend(hw_priv);

	down(&hw_priv->scan.lock);
	mutex_lock(&hw_priv->conf_mutex);

	BUG_ON(hw_priv->scan.req);
    hw_priv->scan.n_ssids = 0;
	hw_priv->scan.status = 0;
    hw_priv->scan.if_id = priv->if_id;
    hw_priv->scan.req = req_wrap->req;
    hw_priv->scan.cca = !!(req_wrap->flags & IEEE80211_SCAN_REQ_CCA);

	atbm_printk_scan("%s:if_id(%d)(%d)\n",__func__,priv->if_id,req_wrap->req->n_channels);
    /*
	*must make sure that there are no pkgs in the lmc.
	*/
	wsm_lock_tx_async(hw_priv);
	wsm_flush_tx(hw_priv);

	ret = wsm_set_template_frame(hw_priv, &frame,
			priv->if_id);
	if (ret) {
        goto err;
	}

	priv->tmpframe_probereq_set = 1;

    first = req_wrap->req->channels[0];

	if (priv->if_id)
		scan.scanFlags |= WSM_FLAG_MAC_INSTANCE_1;
	else
		scan.scanFlags &= ~WSM_FLAG_MAC_INSTANCE_1;

    scan.band = first->band;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 1, 0))
	if (hw_priv->scan.req->no_cck){
		scan.maxTransmitRate = WSM_TRANSMIT_RATE_6;
	}
	else
#endif
	{
		scan.maxTransmitRate = WSM_TRANSMIT_RATE_1;
	}
	
	if (priv->if_id&&priv->vif->p2p){
		scan.maxTransmitRate = WSM_TRANSMIT_RATE_6;
	}

	if(scan.band == IEEE80211_BAND_5GHZ){
		scan.maxTransmitRate = WSM_TRANSMIT_RATE_6;
	}
		/* TODO: Is it optimal? */
	scan.numOfProbeRequests = channel_is_no_ir(first) ? 0 : 3;
	
	if(req_wrap->flags & IEEE80211_SCAN_REQ_CCA)
		scan.numOfProbeRequests = 0;

    for (i = 0; i < req_wrap->req->n_ssids; ++i) {
		struct wsm_ssid *dst =
			&hw_priv->scan.ssids[hw_priv->scan.n_ssids];
		BUG_ON(req_wrap->req->ssids[i].ssid_len > sizeof(dst->ssid));
		memcpy(&dst->ssid[0], req_wrap->req->ssids[i].ssid,
			sizeof(dst->ssid));
		dst->length = req_wrap->req->ssids[i].ssid_len;
		++hw_priv->scan.n_ssids;
	}
	/*
	*passive scan
	*/
	if(req_wrap->flags & IEEE80211_SCAN_REQ_PASSIVE_SCAN)
		scan.numOfProbeRequests = 0;

	scan.numOfSSIDs =req_wrap->req->n_ssids;
	scan.ssids = &hw_priv->scan.ssids[0];
	scan.numOfChannels = req_wrap->req->n_channels;
	/* TODO: Is it optimal? */
	if(scan.numOfChannels == 3)
	{
		ProbeRequestTime = 10;
		ChannelRemainTime = 10;
	}
	scan.probeDelay = ProbeRequestTime;
	/* It is not stated in WSM specification, however
	 * FW team says that driver may not use FG scan
	 * when joined. */
	if (priv->join_status == ATBM_APOLLO_JOIN_STATUS_STA) {
		scan.scanType = WSM_SCAN_TYPE_BACKGROUND;
		scan.scanFlags = WSM_SCAN_FLAG_FORCE_BACKGROUND;
	}		
	if(req_wrap->flags & IEEE80211_SCAN_REQ_CCA){
		scan.scanFlags |= WSM_FLAG_BEST_CHANNEL_START;
	}

	scan.ch = atbm_kzalloc(sizeof(struct wsm_scan_ch[14]), GFP_KERNEL);
	if (!scan.ch) {
        ret = -ENOMEM;
		goto err;
	}
	maxChannelTime = (scan.numOfSSIDs * scan.numOfProbeRequests *
		ProbeRequestTime) + ChannelRemainTime;
	maxChannelTime = (maxChannelTime < 35) ? 35 : maxChannelTime;

#ifndef ATBM_USE_FASTLINK
	for (i = 0; i < scan.numOfChannels; ++i) {
		scan.ch[i].number = channel_hw_value(req_wrap->req->channels[i]);		
#else
	int chan_num[14]={1,6,11,2,7,3,8,4,9,5,10,12,13,14};
	for (i = 0; i < scan.numOfChannels; ++i) {
		scan.ch[i].number = channel_hw_value(req_wrap->req->channels[chan_num[i] - 1]);
#endif

        atbm_printk_err("%s: index[%d][%d]\n",__func__,i,scan.ch[i].number);
		if(channel_is_no_ir(req_wrap->req->channels[i])){
			scan.ch[i].minChannelTime = 120;
			scan.ch[i].maxChannelTime = 150;
		}
		else {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 1, 0))
				if (hw_priv->scan.req->no_cck){			
					scan.ch[i].minChannelTime = 35;
				}
				else
#endif //#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 1, 0))
				{	
					scan.ch[i].minChannelTime = 15;
				}
			    scan.ch[i].maxChannelTime = maxChannelTime;
			}
		if(req_wrap->flags & IEEE80211_SCAN_REQ_CCA){
			scan.ch[i].maxChannelTime = 500;
			if(req_wrap->flags & IEEE80211_SCAN_REQ_RADAR_DETECT){
				if(req_wrap->req->channels[i]->flags & IEEE80211_CHAN_RADAR)
					scan.ch[i].flags |= WSM_SCAN_REQ_RADAR_DETECT;
					scan.ch[i].maxChannelTime = 1000;
			}
		}
	}
    if(scan.numOfChannels == 1 && hw_priv->single_channel_scan_reduce_time){
		scan.ch[0].minChannelTime = 35;
		scan.ch[0].maxChannelTime = hw_priv->single_channel_scan_reduce_time;
		if(scan.ch[0].maxChannelTime <= 50){
            scan.numOfProbeRequests = 1;
        }
		atbm_printk_scan("scan single channel = %d \n",hw_priv->single_channel_scan_reduce_time);
	}
	/*
	*sometimes,t920 take a long time to scan,and
	*the usb recive process run too slowly.if that
	*happens,can triger some errors.
	*/
	//hw_priv->scan.status = atbm_scan_start(priv, &scan);
	atbm_printk_scan("scan start band(%d),(%d)\n",scan.band,scan.numOfChannels);
	scan_status = atbm_scan_start(priv, &scan);
	atbm_kfree(scan.ch);

	if (WARN_ON(scan_status)){
        ret = -EBUSY;
		goto err;
	}

	atbm_printk_scan("%s:scan, delay suspend\n",__func__);
	mutex_unlock(&hw_priv->conf_mutex);

exit:
	if (frame.skb)
		atbm_dev_kfree_skb(frame.skb);
    return ret;
err:
    hw_priv->scan.n_ssids = 0;
	hw_priv->scan.status = 0;
    hw_priv->scan.if_id = -1;
    hw_priv->scan.req = NULL;
    hw_priv->scan.cca = 0; 
    wsm_unlock_tx(hw_priv);
	mutex_unlock(&hw_priv->conf_mutex);
	up(&hw_priv->scan.lock);
	atbm_release_suspend(hw_priv);
	goto exit;
}
static void atbm_scan_complete(struct atbm_common *hw_priv, int if_id)
{
	atomic_xchg(&hw_priv->recent_scan, 0);

    hw_priv->scan.req = NULL;
    hw_priv->scan.cca = 0;
	wsm_unlock_tx(hw_priv);
	ieee80211_scan_completed(hw_priv->hw,
				 hw_priv->scan.status ? 1 : 0);
	up(&hw_priv->scan.lock);
	atbm_release_suspend(hw_priv);

}

void atbm_scan_complete_cb(struct atbm_common *hw_priv,
				struct wsm_scan_complete *arg)
{
	struct atbm_vif *priv = ABwifi_hwpriv_to_vifpriv(hw_priv,
					hw_priv->scan.if_id);	

	
	if (unlikely(!priv))
	{
		return;
	}

	if (unlikely(priv->mode == NL80211_IFTYPE_UNSPECIFIED)) {
		/* STA is stopped. */
		atbm_priv_vif_list_read_unlock(&priv->vif_lock);
		atbm_printk_err("[SCAN] mode err\n");
		return;
	}
	atbm_priv_vif_list_read_unlock(&priv->vif_lock);

	//printk("hw_priv->bStartTx %d\n",hw_priv->bStartTx);
	if(hw_priv->bStartTx)
	{
		atbm_hw_priv_queue_delayed_work(hw_priv,&hw_priv->scan.timeout, HZ/10);
		return;
	}

	atbm_printk_scan("hw_priv->scan.status %d\n",hw_priv->scan.status);

	if(hw_priv->scan.status == -ETIMEDOUT)
		atbm_printk_warn("Scan timeout already occured. Don't cancel work");
	if ((hw_priv->scan.status != -ETIMEDOUT) &&
		(atbm_hw_cancel_delayed_work(&hw_priv->scan.timeout,false/*can't set to true,because this function is call in bh, must not wait in bh */) > 0)) {
		hw_priv->scan.status = 1;
		if(hw_priv->scan.cca){
			struct ieee80211_internal_scan_notity notify;
			notify.cca.val = arg->busy_ratio;
			notify.cca.val_len = sizeof(arg->busy_ratio);
			notify.radar.val = arg->radar_exist;
			notify.radar.val_len = sizeof(arg->radar_exist);
			notify.success = true;
			ieee80211_scan_cca_notify(hw_priv->hw,&notify);
		}
#ifdef SIGMSTAR_SCAN_FEATURE
		else {
				int i;
				struct ieee80211_local *local = hw_to_local(hw_priv->hw);
				for(i=0;(i<CHANNEL_NUM)&&(i<14);i++){
				local->noise_floor[i] = arg->busy_ratio[i];
				}
		}
#endif //#ifdef SIGMSTAR_SCAN_FEATURE
		atbm_hw_priv_queue_delayed_work(hw_priv,
				&hw_priv->scan.timeout, 0);
	}
}
//#ifdef CONFIG_WIRELESS_EXT

void etf_scan_end_work(struct atbm_work_struct *work)
{
	struct atbm_common *hw_priv =
		container_of(work, struct atbm_common, etf_tx_end_work);
	
	struct atbm_vif *priv = __ABwifi_hwpriv_to_vifpriv(hw_priv,
					hw_priv->scan.if_id);
	
	etf_v2_scan_end(hw_priv,priv->vif);
}
//#endif  //CONFIG_WIRELESS_EXT
void atbm_scan_timeout(struct atbm_work_struct *work)
{
	struct atbm_common *hw_priv =
		container_of(work, struct atbm_common, scan.timeout.work);
//#ifdef CONFIG_WIRELESS_EXT
	if(hw_priv->bStartTx)
	{
		struct atbm_vif *priv = __ABwifi_hwpriv_to_vifpriv(hw_priv,
						hw_priv->scan.if_id);
		if(hw_priv->bStartTxWantCancel==0){
			
			wsm_start_scan_etf(hw_priv,priv->vif);
		}
		else {
			hw_priv->bStartTx = 0;
			hw_priv->bStartTxWantCancel  = 0;
			if(hw_priv->etf_test_v2){
				atbm_hw_priv_queue_work(hw_priv, &hw_priv->etf_tx_end_work);
			}
			//stop etf test
			//up(&hw_priv->scan.lock);
			//printk("atbm_scan_timeout bStartTx %d\n",hw_priv->bStartTx);
		}
		return;
	}
//#endif  //CONFIG_WIRELESS_EXT
	
	mutex_lock(&hw_priv->conf_mutex);
	if (likely(atomic_xchg(&hw_priv->scan.in_progress, 0))) {
		if (hw_priv->scan.status > 0)
			hw_priv->scan.status = 0;
		else if (!hw_priv->scan.status) {
			atbm_printk_warn("Timeout waiting for scan "
				"complete notification.\n");
			hw_priv->scan.status = -ETIMEDOUT;
			//hw_priv->scan.curr = hw_priv->scan.end;
			if(WARN_ON(wsm_stop_scan(hw_priv,
						hw_priv->scan.if_id ? 1 : 0)))
			{
				hw_priv->scan.wait_complete= 0;
				wsm_oper_unlock(hw_priv);
			}
		}
	    mutex_unlock(&hw_priv->conf_mutex);
		atbm_scan_complete(hw_priv, hw_priv->scan.if_id);
	    mutex_lock(&hw_priv->conf_mutex);
	}
	mutex_unlock(&hw_priv->conf_mutex);
}

void atbm_wait_scan_complete_sync(struct atbm_common *hw_priv)
{
	down(&hw_priv->scan.lock);
	mutex_lock(&hw_priv->conf_mutex);
	/*
	*here wait scan completed
	*/
	mutex_unlock(&hw_priv->conf_mutex);
	up(&hw_priv->scan.lock);
	atbm_printk_scan( "%s\n",__func__);
}
void atbm_cancel_hw_scan(struct ieee80211_hw *hw,struct ieee80211_vif *vif)
{
	struct atbm_common *hw_priv = hw->priv;
	struct atbm_vif *priv = ABwifi_get_vif_from_ieee80211(vif);

	atbm_printk_scan( "%s:[%d]\n",__func__,priv->if_id);
#ifdef ATBM_USE_FASTLINK
	if(atbm_hw_cancel_delayed_work(&hw_priv->scan.timeout,true) == true){
		atbm_scan_timeout(&hw_priv->scan.timeout.work);
	}
#endif
	atbm_wait_scan_complete_sync(hw_priv);
}
