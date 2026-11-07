/*
 * Scan interface for altobeam APOLLO mac80211 drivers
 * *
 * Copyright (c) 2016, altobeam
 * Author:
 *
 *Based on apollo code
 * Copyright (c) 2010, ST-Ericsson
 * Author: Dmitry Tarnyagin <dmitry.tarnyagin@stericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef SCAN_H_INCLUDED
#define SCAN_H_INCLUDED

#include <linux/semaphore.h>
#include "wsm.h"

/* external */ struct sk_buff;
/* external */ struct cfg80211_scan_request;
/* external */ struct ieee80211_channel;
/* external */ struct ieee80211_hw;
/* external */ struct atbm_work_struct;

struct atbm_scan {
	struct semaphore lock;
//	struct atbm_work_struct work;
	struct atbm_delayed_work timeout;
	struct cfg80211_scan_request *req;
//	struct ieee80211_scan_req_wrap *req_wrap;
//	struct ieee80211_channel **begin;
//	struct ieee80211_channel **curr;
//	struct ieee80211_channel **end;
	struct wsm_ssid ssids[WSM_SCAN_MAX_NUM_OF_SSIDS];
//	int output_power;
	int n_ssids;
	int status;
	atomic_t in_progress;
	/* Direct probe requests workaround */
	u8 if_id;
	int wait_complete;
	u8 cca;
	//u8 passive;
};

int atbm_hw_scan(struct ieee80211_hw *hw,
			struct ieee80211_vif *vif,
			struct ieee80211_scan_req_wrap *req_wrap);
void atbm_scan_timeout(struct atbm_work_struct *work);
void etf_scan_end_work(struct atbm_work_struct *work);
void atbm_scan_complete_cb(struct atbm_common *priv,
				struct wsm_scan_complete *arg);
/* ******************************************************************** */
/* Raw probe requests TX workaround					*/

void atbm_cancel_hw_scan(struct ieee80211_hw *hw,struct ieee80211_vif *vif);
void atbm_wait_scan_complete_sync(struct atbm_common *hw_priv);

#endif
