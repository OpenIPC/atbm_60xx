/*
 * Copyright 2002-2005, Instant802 Networks, Inc.
 * Copyright 2005-2006, Devicescape Software, Inc.
 * Copyright (c) 2006 Jiri Benc <jbenc@suse.cz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/rtnetlink.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <net/atbm_mac80211.h>
#include <net/cfg80211.h>
#include "rate.h"
#include "ieee80211_i.h"
#include "debugfs.h"
#include "../apollo.h"
#include <net/netlink.h>
#include "rc80211_hmac.h"

#define CONFIG_COMPAT_MAC80211_RC_DEFAULT "rc_hmac"
struct rate_control_alg {
	struct list_head list;
	struct atbm_rate_control_ops *ops;
};

static LIST_HEAD(rate_ctrl_algs);
static DEFINE_MUTEX(rate_ctrl_mutex);

static char *ieee80211_default_rc_algo = CONFIG_COMPAT_MAC80211_RC_DEFAULT;
module_param(ieee80211_default_rc_algo, charp, 0644);
MODULE_PARM_DESC(ieee80211_default_rc_algo,
		 "Default rate control algorithm for mac80211 to use");

int ieee80211_rate_control_register(struct atbm_rate_control_ops *ops)
{
	struct rate_control_alg *alg;

	if (!ops->name)
		return -EINVAL;
	atbm_printk_rc( "ieee80211_rate_control_register %s \n", ops->name);

	mutex_lock(&rate_ctrl_mutex);
	list_for_each_entry(alg, &rate_ctrl_algs, list) {
		if (!strcmp(alg->ops->name, ops->name)) {
			/* don't register an algorithm twice */
			WARN_ON(1);
			mutex_unlock(&rate_ctrl_mutex);
			return -EALREADY;
		}
	}

	alg = atbm_kzalloc(sizeof(*alg), GFP_KERNEL);
	if (alg == NULL) {
		mutex_unlock(&rate_ctrl_mutex);
		return -ENOMEM;
	}
	alg->ops = ops;
	atbm_printk_rc( " ieee80211_rate_control_register %s \n", ops->name);

	list_add_tail(&alg->list, &rate_ctrl_algs);
	mutex_unlock(&rate_ctrl_mutex);

	return 0;
}
//EXPORT_SYMBOL(ieee80211_rate_control_register);

void ieee80211_rate_control_unregister(struct atbm_rate_control_ops *ops)
{
	struct rate_control_alg *alg;
	atbm_printk_rc( "ieee80211_rate_control_unregister %s \n", ops->name);

	mutex_lock(&rate_ctrl_mutex);
	list_for_each_entry(alg, &rate_ctrl_algs, list) {
		if (alg->ops == ops) {
			list_del(&alg->list);
			atbm_kfree(alg);
			break;
		}
	}
	mutex_unlock(&rate_ctrl_mutex);
}
//EXPORT_SYMBOL(ieee80211_rate_control_unregister);

/*port part*/

int atbm_ieee80211_rate_control_register(struct atbm_rate_control_ops *ops)
{
   return ieee80211_rate_control_register(ops);
}

void atbm_ieee80211_rate_control_unregister(struct atbm_rate_control_ops *ops)
{
    ieee80211_rate_control_unregister(ops);
}



static struct atbm_rate_control_ops *
ieee80211_try_rate_control_ops_get(const char *name)
{
	struct rate_control_alg *alg;
	struct atbm_rate_control_ops *ops = NULL;

	if (!name)
		return NULL;

	mutex_lock(&rate_ctrl_mutex);
	list_for_each_entry(alg, &rate_ctrl_algs, list) {
		if (!strcmp(alg->ops->name, name))
			if (try_module_get(alg->ops->module)) {
				ops = alg->ops;
				break;
			}
	}
	mutex_unlock(&rate_ctrl_mutex);
	return ops;
}

/* Get the rate control algorithm. */
static struct atbm_rate_control_ops *
ieee80211_rate_control_ops_get(const char *name)
{
	struct atbm_rate_control_ops *ops;
	const char *alg_name;
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 2, 0))
	kparam_block_sysfs_write(ieee80211_default_rc_algo);
	#else
	kernel_param_lock(THIS_MODULE);
	#endif
	if (!name)
		alg_name = ieee80211_default_rc_algo;
	else
		alg_name = name;

	ops = ieee80211_try_rate_control_ops_get(alg_name);
	if (!ops) {
		request_module("rc80211_%s", alg_name);
		ops = ieee80211_try_rate_control_ops_get(alg_name);
	}
	if (!ops && name)
		/* try default if specific alg requested but not found */
		ops = ieee80211_try_rate_control_ops_get(ieee80211_default_rc_algo);

	/* try built-in one if specific alg requested but not found */
	if (!ops && strlen(CONFIG_COMPAT_MAC80211_RC_DEFAULT))
		ops = ieee80211_try_rate_control_ops_get(CONFIG_COMPAT_MAC80211_RC_DEFAULT);
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 2, 0))
	kparam_unblock_sysfs_write(ieee80211_default_rc_algo);
	#else
	kernel_param_unlock(THIS_MODULE);
	#endif

	return ops;
}

static void ieee80211_rate_control_ops_put(struct atbm_rate_control_ops *ops)
{
	module_put(ops->module);
}

#ifdef CONFIG_MAC80211_ATBM_DEBUGFS
static ssize_t rcname_read(struct file *file, char __user *userbuf,
			   size_t count, loff_t *ppos)
{
	struct rate_control_ref *ref = file->private_data;
	int len = strlen(ref->ops->name);

	return simple_read_from_buffer(userbuf, count, ppos,
				       ref->ops->name, len);
}

static const struct file_operations rcname_ops = {
	.read = rcname_read,
	.open = mac80211_open_file_generic,
	.llseek = default_llseek,
};
#endif

static struct rate_control_ref *rate_control_alloc(const char *name,
					    struct ieee80211_local *local)
{
	struct dentry *debugfsdir = NULL;
	struct rate_control_ref *ref;
	struct atbm_common *hw_priv  = (struct atbm_common *) local->hw.priv;

	ref = atbm_kmalloc(sizeof(struct rate_control_ref), GFP_KERNEL);
	if (!ref)
		goto fail_ref;
	kref_init(&ref->kref);
	ref->local = local;
	ref->ops = ieee80211_rate_control_ops_get(name);
	if (!ref->ops)
		goto fail_ops;
	atbm_printk_rc( "rate_control_alloc %s \n", name);

#ifdef CONFIG_MAC80211_ATBM_DEBUGFS
	debugfsdir = debugfs_create_dir("rc", local->hw.wiphy->debugfsdir);
	local->debugfs.rcdir = debugfsdir;
	debugfs_create_file("name", 0400, debugfsdir, ref, &rcname_ops);
#endif

	ref->priv = ref->ops->alloc(hw_priv->chip_cat);
	atbm_printk_rc( " rate_control_alloc\n");

	if (!ref->priv)
		goto fail_priv;
	return ref;

fail_priv:
	ieee80211_rate_control_ops_put(ref->ops);
fail_ops:
	atbm_kfree(ref);
fail_ref:
	return NULL;
}

static void rate_control_release(struct kref *kref)
{
	struct rate_control_ref *ctrl_ref;

	ctrl_ref = container_of(kref, struct rate_control_ref, kref);
	ctrl_ref->ops->free(ctrl_ref->priv);

#ifdef CONFIG_MAC80211_ATBM_DEBUGFS
	debugfs_remove_recursive(ctrl_ref->local->debugfs.rcdir);
	ctrl_ref->local->debugfs.rcdir = NULL;
#endif

	ieee80211_rate_control_ops_put(ctrl_ref->ops);
	atbm_kfree(ctrl_ref);
}
static inline s8
rate_lowest_non_cck_index(struct ieee80211_supported_band *sband,
			  struct ieee80211_sta *sta)
{
	int i;

	for (i = 0; i < sband->n_bitrates; i++) {
		struct ieee80211_rate *srate = &sband->bitrates[i];
		if ((srate->bitrate == 10) || (srate->bitrate == 20) ||
		    (srate->bitrate == 55) || (srate->bitrate == 110))
			continue;

		if (rate_supported(sta, sband->band, i))
			return sband->bitrates[i].hw_value;
	}

	/* No matching rate found */
	return 0;
}


bool rate_control_send_low(struct ieee80211_sta *sta,
			   void *priv_sta,
			   struct ieee80211_tx_rate_control *txrc)
{
	struct ieee80211_tx_info *info = txrc->info;
	struct ieee80211_supported_band *sband = txrc->sband;
    s8 idx;
   
	if (txrc->txrc_in.lower) 
    {
		if ((sband->band != IEEE80211_BAND_2GHZ) ||
		    !(info->flags & IEEE80211_TX_CTL_NO_CCK_RATE))
			idx = rate_lowest_index(txrc->sband, sta);
		else
			idx =
				rate_lowest_non_cck_index(txrc->sband, sta);
        //set rate   
        info->control.txrc_out.tx_max_rate = idx;        
		return true;       
	}
    
	return false;   //not use lowest rate.
}

bool atbm_rate_control_send_low(struct ieee80211_sta *sta,
			   void *priv_sta,
			   struct ieee80211_tx_rate_control *txrc)
{

    struct ieee80211_tx_info *info = txrc->info;
    
	struct ieee80211_supported_band *sband = txrc->sband;
   // struct ieee80211_sta *sta = (struct ieee80211_sta *)atbm_sta;
    s8 idx;
 //  atbm_printk_rc( "x atbm_rate_control_send_low \n");
	if (txrc->txrc_in.lower) 
    {
      atbm_printk_rc( " atbm_rate_control_send_low sband->band:%d\n", sband->band);
		if ((sband->band != IEEE80211_BAND_2GHZ) ||
		    !(info->flags & IEEE80211_TX_CTL_NO_CCK_RATE))
			idx = rate_lowest_index(sband, sta);
		else
			idx =
				rate_lowest_non_cck_index(sband, sta);
        //set rate   
        txrc->info->control.txrc_out.tx_max_rate = idx;        
		return true;       
	}
    
	return false;   //not use lowest rate.

}




/*linux modem driver private API*/			   
int hmac_get_txrx_status_wh(struct sta_info* sta_info, struct TX_RX_Statistics_S* data)
{
	RATE_CONTROL_RATE* rc_sta_info = (RATE_CONTROL_RATE*)sta_info->rate_ctrl_priv;
	
	data->rx_rate_avg = (unsigned short)rc_sta_info->rx_rate_avg;
	data->received_num = rc_sta_info->received_num;
	data->sum_success_cnt = rc_sta_info->total_tx_counter - rc_sta_info->total_tx_fail_counter;
	data->transmit_total_cnt = rc_sta_info->total_tx_counter;

	return 0;
}
               
//EXPORT_SYMBOL(rate_control_send_low);

void rate_control_get_rate(struct ieee80211_sub_if_data *sdata,
			   struct sta_info *sta,
			   struct ieee80211_tx_rate_control *txrc)
{
	struct rate_control_ref *ref = sdata->local->rate_ctrl;
	void *priv_sta = NULL;
    struct atbm_ieee80211_tx_rate_control atbm_txrc;
	struct ieee80211_sta *ista = NULL;
	//struct atbm_ieee80211_sta atbm_sta;  
    
	if (sta) {
		ista = &sta->sta;
		priv_sta = sta->rate_ctrl_priv;
	}else
		{
		     atbm_printk_err( " get_rate \n");
		}
	//atbm_sta = (struct atbm_ieee80211_sta *)sta;  

    /*copy from ieee80211_tx_rate_control to atbm_ieee80211_tx_rate_control  */
  //  atbm_txrc.manual       = txrc->manual;
  //  atbm_txrc.manual_rate  = txrc->manual_rate;
  //  atbm_txrc.max_rate_idx = txrc->max_rate_idx;
  //  atbm_txrc.lower        = txrc->lower;
 //    atbm_printk_rc( "x get_rate pre1\n");
	 	
     atbm_txrc.tx_rc_in = (struct atbm_tx_rc_input *)&txrc->txrc_in;
     atbm_txrc.info.control = (struct atbm_tx_rc_output *) &txrc->info->control.txrc_out;
	  //  atbm_printk_rc( "x get_rate pre2\n");
    //atbm_txrc.priv         = txrc->sband;
    //check low rate.
    if(atbm_rate_control_send_low(ista,  priv_sta,txrc))
    {        
         txrc->txrc_in.max_rate_idx = txrc->info->control.txrc_out.tx_max_rate;
         atbm_txrc.tx_rc_in->lower = 1;
         atbm_txrc.tx_rc_in->max_rate_idx = txrc->txrc_in.max_rate_idx;
         atbm_printk_err( "x get_rate low max_rate_idx:%d\n", atbm_txrc.tx_rc_in->max_rate_idx );
    }
    else
    {
           atbm_txrc.tx_rc_in->lower = 0;
    }

   // atbm_sta->he_cap = &ista->he_cap;
  //  atbm_sta->ht_cap = &ista->ht_cap;
  //  atbm_sta->vht_cap = &ista->vht_cap;
   // atbm_printk_rc( "x get_rate\n");
	ref->ops->get_rate(ref->priv, priv_sta, &atbm_txrc);     

   /*copy to ieee80211_tx_rate_control*/    
    /*
    txrc->info->control.tx_rate_sets     =  atbm_txrc.info->control.tx_rate_sets;
    txrc->info->control.tx_max_rate      =  atbm_txrc.info->control.tx_max_rate;
    txrc->info->control.tx_rc_flag       =  atbm_txrc.info->control.tx_rc_flag;
    txrc->info->control.force_policyid   =  atbm_txrc.info->control.force_policyid;      
    */
    txrc->info->tx_update_rate           =  atbm_txrc.info.tx_update_rate;
    txrc->info->sample_pkt_flag          =  atbm_txrc.info.sample_pkt_flag;
    
}

struct rate_control_ref *rate_control_get(struct rate_control_ref *ref)
{
	kref_get(&ref->kref);
	return ref;
}

void rate_control_put(struct rate_control_ref *ref)
{
	kref_put(&ref->kref, rate_control_release);
}
int atbm_ieee80211_init_rate_ctrl_alg(struct ieee80211_local *local,
				 const char *name)
{
	struct rate_control_ref *ref, *old;

	ASSERT_RTNL();

	if (local->open_count)
		return -EBUSY;

	if (local->hw.flags & IEEE80211_HW_HAS_RATE_CONTROL) {
		if (WARN_ON(!local->ops->set_rts_threshold))
			return -EINVAL;
		return 0;
	}

	atbm_printk_rc( " atbm_ieee80211_init_rate_ctrl_alg\n");
	
	ref = rate_control_alloc(name, local);
	if (!ref) {
		atbm_printk_warn("Failed to select rate control algorithm,%s\n",ref->ops->name);
		atbm_printk_rc( " atbm_ieee80211_init_rate_ctrl_alg fail\n");
	
		return -ENOENT;
	}

	old = local->rate_ctrl;
	local->rate_ctrl = ref;

	//atbm_printk_rc( " atbm_ieee80211_init_rate_ctrl_alg %s local addr:0x%x ref:0x%x \n ", ref->ops->name, local, local->rate_ctrl);

	
	if (old) {
		rate_control_put(old);
		sta_info_flush(local, NULL);
	}

	wiphy_debug(local->hw.wiphy, "Selected rate control algorithm '%s'\n",
		    ref->ops->name);
	return 0;
}

void rate_control_deinitialize(struct ieee80211_local *local)
{
	struct rate_control_ref *ref;

	ref = local->rate_ctrl;

	if (!ref)
		return;

	local->rate_ctrl = NULL;
	rate_control_put(ref);
}

#if 1
struct txrx_statistics_s
{
	short rx_rate_avg;
	int received_num;
	int sum_success_cnt;
	int transmmit_total_cnt;
};

static struct txrx_statistics_s txrx_statues_empty[16 + 2];

void atbm_get_rx_rate_average(struct ieee80211_rx_status* hdr, struct atbm_common* hw_priv)
{
	static int rx_rate_avg_vir[5] = {0};
	static int rx_rate_avg_vir_index = 0;
	int i, rate_sum = 0;
	struct ieee80211_supported_band* sband;

	sband = hw_priv->hw->wiphy->bands[hdr->band];
	rx_rate_avg_vir[rx_rate_avg_vir_index] = sband->bitrates[hdr->rate_idx].bitrate;
	if (++rx_rate_avg_vir_index >= 5) {
		rx_rate_avg_vir_index = 0;
	}

	for (i = 0; i < 5; i++) {
		rate_sum += rx_rate_avg_vir[i];
	}

	
	txrx_statues_empty[hdr->aid].rx_rate_avg = rate_sum / 5;
	txrx_statues_empty[hdr->aid].received_num++;
	
	return;
}

static void atbm_get_tx_status(int sta_id, struct atbm_vif* priv)
{
	struct sta_info* stainfo;
	struct ieee80211_sta* sta;
	RATE_CONTROL_RATE* rate_info;

	sta = rcu_dereference(priv->linked_sta[sta_id]);
	stainfo = container_of(sta, struct sta_info, sta);
	rate_info = (RATE_CONTROL_RATE*)stainfo->rate_ctrl_priv;
	
	txrx_statues_empty[sta_id].sum_success_cnt = rate_info->total_tx_counter - rate_info->total_tx_fail_counter;
	txrx_statues_empty[sta_id].transmmit_total_cnt = rate_info->total_tx_counter;
	
	return;
}

int atbm_get_tx_rx_average(int sta_id, struct atbm_vif* priv, void* txrx_stat)
{
	struct txrx_statistics_s* p_txrx_rate = (struct txrx_statistics_s*)txrx_stat;

	atbm_printk_err("get txrx average :%d\n", sta_id);
	atbm_get_tx_status(sta_id, priv);
	p_txrx_rate->rx_rate_avg = txrx_statues_empty[sta_id].rx_rate_avg;
	p_txrx_rate->received_num = txrx_statues_empty[sta_id].received_num;
	p_txrx_rate->sum_success_cnt = txrx_statues_empty[sta_id].sum_success_cnt;
	p_txrx_rate->transmmit_total_cnt = txrx_statues_empty[sta_id].transmmit_total_cnt;

	return 0;
}

void atbm_clear_txrx_average(int sta_id)
{
	if (sta_id < 17) {
		txrx_statues_empty[sta_id].rx_rate_avg = 0;
		txrx_statues_empty[sta_id].received_num = 0;
		txrx_statues_empty[sta_id].sum_success_cnt = 0;
		txrx_statues_empty[sta_id].transmmit_total_cnt = 0;
	}

	return;
}
#endif