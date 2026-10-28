#include <net/atbm_mac80211.h>
#include <linux/nl80211.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/skbuff.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/freezer.h>
#include <linux/inetdevice.h>
#include <net/net_namespace.h>

#include "ieee80211_i.h"
#include "driver-ops.h"

#define IEEE80211_BLE_SKB_HEADNEED	128
static void  ieee80211_ble_dump(const char *string,u8 *mem,size_t len)
{
#if 0
	size_t i = 0;
	atbm_printk_err("[%s]:\n",string);

	for(i = 0; i< len ; i++){
		if(!(i % 16)){
			atbm_printk_err("\n");
		}
		atbm_printk_err("[%x]",mem[i]);
	}
#endif	
}
static int ieee80211_ble_thread_wakeup(struct ieee80211_ble_thread *thread)
{

	void *bh;
	rcu_read_lock();
	if(test_and_set_bit(THREAD_ACTION_WAKEUP, &thread->flags) == 0){
		bh = rcu_dereference(thread->thread);
		if(bh){			
			wake_up_process((struct task_struct *)bh);
		}
	}
	rcu_read_unlock();
	return 0;
}

static int ieee80211_ble_thread_deinit(struct ieee80211_ble_thread *thread)
{
	void *bh;
	struct ieee80211_local *local = thread->local;
	struct ieee80211_ble_local *ble_local = &local->ble_local;
	
	set_bit(THREAD_ACTION_SHOULD_STOP,&thread->flags);
	spin_lock_bh(&ble_local->ble_spin_lock);
	bh = rcu_dereference(thread->thread);
	rcu_assign_pointer(thread->thread,NULL);
	spin_unlock_bh(&ble_local->ble_spin_lock);
	if (bh){
		synchronize_rcu();
		kthread_stop(bh);
	}

	return 0;
}

static int ieee80211_ble_kthread_should_stop(struct ieee80211_ble_thread *thread)
{
	if(!kthread_should_stop()){
		return 0;
	}
	
	set_bit(THREAD_ACTION_SHOULD_STOP,&thread->flags);
	if(test_bit(THREAD_ACTION_SHOULD_SUSPEND, &thread->flags)) {
		if (!test_and_set_bit(THREAD_ACTION_SUSPENED, &thread->flags))
			complete(&thread->suspended);
	}

	return 1;
}
static void ieee80211_ble_schedule_timeout(struct ieee80211_ble_thread *thread)
{
	signed long timeout = schedule_timeout(thread->wakeup_period);

	if (timeout == 0 && thread->period_handle){
		thread->period_handle(thread);
	}
}

static int ieee80211_ble_wait_action(struct ieee80211_ble_thread *thread)
{
	set_current_state(TASK_INTERRUPTIBLE);	
	while (!ieee80211_ble_kthread_should_stop(thread)) {
		if (test_and_clear_bit(THREAD_ACTION_WAKEUP,
				       &thread->flags)) {
			__set_current_state(TASK_RUNNING);
			return 0;
		}
		if (!ieee80211_ble_kthread_should_stop(thread))
			ieee80211_ble_schedule_timeout(thread);
		set_current_state(TASK_INTERRUPTIBLE);
		
	}
	__set_current_state(TASK_RUNNING);
	return -1;
}

static int ieee80211_ble_thread_process(void *val)
{
	struct ieee80211_ble_thread *thread = (struct ieee80211_ble_thread *)val;
	atbm_printk_init("[%s] start\n",thread->name);
	while(!ieee80211_ble_wait_action(thread)){
		thread->thread_fn(thread);
	}
	atbm_printk_init("[%s] stop\n",thread->name);
	return 0;
}

static int ieee80211_ble_thread_init(struct ieee80211_ble_thread *thread)
{	
	thread->thread = kthread_create(ieee80211_ble_thread_process,thread, thread->name);
	
	if (IS_ERR(thread->thread)){
		thread->thread = NULL;
		atbm_printk_err("sdio %s err\n",thread->name);
		return -1;
	}
	init_completion(&thread->suspended);
	return  0;
}
static int ieee80211_ble_xmit_thread(struct ieee80211_ble_thread *thread)
{
	struct sk_buff *skb;
	struct ieee80211_local *local = thread->local;
	struct ieee80211_ble_local *ble_local = &local->ble_local;

	while((skb  =  atbm_skb_dequeue(&ble_local->ble_xmit_queue))){
		
		/*
		*start tx
		*/
		BUG_ON(local->ops->do_ble_xmit == NULL);
		//printk("[ble xmit]:len [%d]\n",skb->len);
		ieee80211_ble_dump(__func__,skb->data,skb->len);
		local->ops->do_ble_xmit(&local->hw,skb->data,skb->len);
		/*
		*free skb
		*/
		atbm_dev_kfree_skb(skb);
	}

	return 0;
}
static int ieee80211_ble_xmit_init(struct ieee80211_local *local)
{
	struct ieee80211_ble_local *ble_local = &local->ble_local;
	struct ieee80211_ble_thread *thread = &ble_local->xmit_thread;
	
	atbm_skb_queue_head_init(&ble_local->ble_xmit_queue);

	thread->flags = 0;
	thread->name  = ieee80211_alloc_name(&local->hw,"ble_xmit");
	thread->period_handle = NULL;
	thread->thread_fn = ieee80211_ble_xmit_thread;
	thread->local = local;
	thread->wakeup_period = MAX_SCHEDULE_TIMEOUT;

	if(ieee80211_ble_thread_init(thread)){
		atbm_printk_err("ble_xmit thread err\n");
		return -1;
	}
	
	return 0;
}
void  ieee80211_ble_recv(struct ieee80211_hw *hw,struct sk_buff *skb)
{
	struct ieee80211_local *local = hw_to_local(hw);
	struct ieee80211_ble_local *ble_local = &local->ble_local;

	spin_lock_bh(&ble_local->ble_spin_lock);
	
	if(ble_local->ble_started == true){
		atbm_skb_queue_tail(&ble_local->ble_recv_queue,skb);
		ieee80211_ble_thread_wakeup(&ble_local->recv_thread);
	}else {
		atbm_dev_kfree_skb(skb);
	}
	
	spin_unlock_bh(&ble_local->ble_spin_lock);
}
static int ieee80211_ble_recv_thread(struct ieee80211_ble_thread *thread)
{
	struct sk_buff *skb;
	struct ieee80211_local *local = thread->local;
	struct ieee80211_ble_local *ble_local = &local->ble_local;
	int  (*ble_cb)(struct ieee80211_local* local, u8 * buff, size_t buff_size, enum ieee80211_ble_msg_type msg_type);
	
	mutex_lock(&ble_local->ble_mutex_lock);

	ble_cb = rcu_dereference(ble_local->ble_recv_callback);
	
	while((skb  =  atbm_skb_dequeue(&ble_local->ble_recv_queue))){
		struct ieee80211_ble_status *status = (struct ieee80211_ble_status *)&skb->cb[0];
		atbm_printk_debug("%s:ble(%d)(%d)\n",__func__,skb->len,status->msg_type);
		if(ble_cb) ble_cb(local,skb->data,skb->len,status->msg_type);

		atbm_dev_kfree_skb(skb);
	}
	
	mutex_unlock(&ble_local->ble_mutex_lock);

	return 0;
}
static int ieee80211_ble_recv_init(struct ieee80211_local *local)
{
	struct ieee80211_ble_local *ble_local = &local->ble_local;
	struct ieee80211_ble_thread *thread = &ble_local->recv_thread;
	
	atbm_skb_queue_head_init(&ble_local->ble_recv_queue);

	thread->flags = 0;
	thread->name  = ieee80211_alloc_name(&local->hw,"ble_recv");
	thread->period_handle = NULL;
	thread->thread_fn = ieee80211_ble_recv_thread;
	thread->local = local;
	thread->wakeup_period = MAX_SCHEDULE_TIMEOUT;

	if(ieee80211_ble_thread_init(thread)){
		atbm_printk_err("ble_recv thread err\n");
		return -1;
	}
	
	return 0;
}
static int ieee80211_ble_xmit_exit(struct ieee80211_local *local)
{
	struct ieee80211_ble_local *ble_local = &local->ble_local;
	struct ieee80211_ble_thread *thread = &ble_local->xmit_thread;
	
	ieee80211_ble_thread_deinit(thread);

	atbm_skb_queue_purge(&ble_local->ble_xmit_queue);
	return  0;
}

static int ieee80211_ble_recv_exit(struct ieee80211_local *local)
{
	struct ieee80211_ble_local *ble_local = &local->ble_local;
	struct ieee80211_ble_thread *thread = &ble_local->recv_thread;
	
	ieee80211_ble_thread_deinit(thread);

	atbm_skb_queue_purge(&ble_local->ble_recv_queue);
	return  0;
}


int ieee80211_ble_commb_start(struct ieee80211_local* local)
{
	struct ieee80211_ble_local *ble_local = &local->ble_local;

	atbm_printk_init("ble start\n");
//	if(ieee80211_ble_recv_init(local)){
//		goto fail_recv;
//	}

	if(ieee80211_ble_xmit_init(local)){
		goto fail_xmit;
	}
	/*
	*start sucess
	*/
	spin_lock_bh(&ble_local->ble_spin_lock);
	ble_local->ble_started = true;
	spin_unlock_bh(&ble_local->ble_spin_lock);
	return 0;
fail_xmit:
	ieee80211_ble_recv_exit(local);
fail_recv:
	atbm_printk_init("ble start err\n");
	return -1;

}

int ieee80211_ble_commb_stop(struct ieee80211_local* local)
{
	struct ieee80211_ble_local *ble_local = &local->ble_local;

	spin_lock_bh(&ble_local->ble_spin_lock);
	ble_local->ble_started = false;
	spin_unlock_bh(&ble_local->ble_spin_lock);
	
	synchronize_rcu();
	
	ieee80211_ble_xmit_exit(local);
//	ieee80211_ble_recv_exit(local);
	atbm_printk_init("ble stop\n");
	return 0;
}

int ieee80211_ble_commb_xmit(struct ieee80211_local* local, u8* xmit, size_t xmit_len)
{
	//struct ieee80211_local *local = ble_to_local(pble_dev);
	struct ieee80211_ble_buff *ble_buff;
	struct ieee80211_ble_local *ble_local = &local->ble_local;
	
	struct sk_buff *skb;
	ieee80211_ble_dump(__func__,xmit,xmit_len);
	ble_buff = container_of((void *)xmit, struct ieee80211_ble_buff, mem);

	skb = ble_buff->skb;

	BUG_ON((skb->data + IEEE80211_BLE_SKB_HEADNEED) != (u8*)ble_buff);
	
	atbm_skb_reserve(skb, IEEE80211_BLE_SKB_HEADNEED+sizeof(struct ieee80211_ble_buff));
	atbm_skb_put(skb,xmit_len);
	
	spin_lock_bh(&ble_local->ble_spin_lock);
	
	if(ble_local->ble_started == true){
		//printk("[%s]:len [%d]\n",__func__,skb->len);
		atbm_skb_queue_tail(&ble_local->ble_xmit_queue,skb);
		ieee80211_ble_thread_wakeup(&ble_local->xmit_thread);
	}else {
		atbm_dev_kfree_skb(skb);
	}
	
	spin_unlock_bh(&ble_local->ble_spin_lock);
	return 0;
}

int ieee80211_ble_commb_subscribe(struct ieee80211_local* local,
	int (*recv)(struct ieee80211_local* local, u8* recv, size_t recv_len, enum ieee80211_ble_msg_type msg_type))
{
	//struct ieee80211_local *local = ble_to_local(pble_dev);
	struct ieee80211_ble_local *ble_local = &local->ble_local;

	atbm_printk_init("ble subscribe\n");
	mutex_lock(&ble_local->ble_mutex_lock);
	rcu_assign_pointer(ble_local->ble_recv_callback,recv);
	mutex_unlock(&ble_local->ble_mutex_lock);
	
	return 0;
}
int ieee80211_ble_commb_unsubscribe(struct ieee80211_local* local)
{
	//struct ieee80211_local *local = ble_to_local(pble_dev);
	struct ieee80211_ble_local *ble_local = &local->ble_local;
	atbm_printk_init("ble unsubscribe\n");
	mutex_lock(&ble_local->ble_mutex_lock);
	rcu_assign_pointer(ble_local->ble_recv_callback,NULL);
	mutex_unlock(&ble_local->ble_mutex_lock);

	synchronize_rcu();
	return 0;
}
char *ieee80211_ble_commb_ble_alloc_xmit(size_t len)
{
	struct sk_buff *skb;
	struct  ieee80211_ble_buff *ble_buff;
	
	skb = atbm_dev_alloc_skb(len +  IEEE80211_BLE_SKB_HEADNEED + sizeof(struct  ieee80211_ble_buff));

	if(skb == NULL){
		return  NULL;
	}

	ble_buff = (struct  ieee80211_ble_buff *)(skb->data + IEEE80211_BLE_SKB_HEADNEED);
	ble_buff->skb = skb;

	return (char *)ble_buff->mem;
}


int ieee80211_ble_dev_int(struct ieee80211_local *local)
{
	struct ieee80211_ble_local *ble_local = &local->ble_local;
	
	ble_local->ble_recv_callback = 0;
	spin_lock_init(&ble_local->ble_spin_lock);
	mutex_init(&ble_local->ble_mutex_lock);
	return 0;
	
}
int ieee80211_ble_dev_register(struct ieee80211_local *local)
{	
	

	return 0;
}
void ieee80211_ble_dev_deregister(struct ieee80211_local *local)
{
	struct ieee80211_ble_local *ble_local = &local->ble_local;
	
	//platform_device_unregister(&local->ble_dev);
	mutex_destroy(&ble_local->ble_mutex_lock);
}
