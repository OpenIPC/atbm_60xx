
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <net/atbm_mac80211.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/udp.h>
#include <net/ip.h>
#include <linux/bitops.h>
//#include <uapi/linux/if_ether.h>
//#include <uapi/linux/tcp.h>
//#include <uapi/linux/ip.h>
//#include <uapi/linux/in.h>
#include <linux/moduleparam.h>
#include <net/tcp.h>
#include "apollo.h"
#include "wsm.h"
#include "bh.h"
#include "ap.h"
#include "debug.h"

#ifdef CONFIG_TCP_ACK_OFFLOAD
int atbm_tcpack_update_cache(struct atbm_tcpack_skbinfo* new_msgbuf,
	struct atbm_tcpack_offload_comm* offload_priv,
	struct atbm_sockt_info* socket_info,
	struct atbm_tcpack_info* ack_now,
	bool is_newfilter);
int atbm_tcpack_alloc_socketid(struct atbm_tcpack_offload_comm* offload_priv);
int atbm_tcpack_find_socketid(struct atbm_tcpack_offload_comm* offload_priv,struct atbm_tcpack_info* atbm_tcpack_info);
void atbm_tcpack_free_skbinfo(struct atbm_common* hw_priv, struct atbm_tcpack_skbinfo* info);
void __atbm_tx(struct ieee80211_hw* dev, struct sk_buff* skb);

/*
enable /disenable ack offload function
*/
int offload_ack = 0;
module_param(offload_ack, int, 0);
int offload_ack_num = 5;
module_param(offload_ack_num, int, 0);
int offload_ack_to = 5;
module_param(offload_ack_to, int, 0);

/* return:
 *      0, msg buf freed by the real driver
 *      others, skb need free by the caller,remember not use msg->skb!
 */
int atbm_tcpack_tx(struct atbm_common* hw_priv, struct atbm_tcpack_skbinfo* msg)
{
	struct sk_buff *skb=msg->skb;
	
	atbm_tcpack_free_skbinfo(hw_priv,msg);

	__atbm_tx(hw_priv->hw, skb);
	return 0;
}

struct atbm_tcpack_skbinfo *atbm_tcpack_alloc_skbinfo(struct atbm_common* hw_priv,struct sk_buff* skb)
{
	int index = 0;
	struct atbm_tcpack_skbinfo *pframebuf;
 	struct atbm_tcpack_offload_comm *offload_priv = &hw_priv->offload_priv;
	
	spin_lock_bh(&offload_priv->lock);
	for (index = 0;(index < TCPACKOFFLOAD_MAX_FRAMEBUF_ID); index++) {
		pframebuf = &offload_priv->framebuf[index];
		if(pframebuf->skb==NULL){
			pframebuf->skb = skb;
			spin_unlock_bh(&offload_priv->lock);
			return pframebuf;
		}
	}
	spin_unlock_bh(&offload_priv->lock);
	printk("<WARNING>atbm_tcpack_alloc fail\n");
	return NULL;
}
	
void atbm_tcpack_free_skbinfo(struct atbm_common *hw_priv,
			    struct atbm_tcpack_skbinfo *info)
{
	//struct atbm_sockt_info *socket_info;
	struct atbm_tcpack_offload_comm *offload_priv = &hw_priv->offload_priv;

	if (!atomic_read(&offload_priv->enable))
		return;
	
	spin_lock_bh(&offload_priv->lock);
	info->skb = NULL;	
	spin_unlock_bh(&offload_priv->lock);
}
				
void atbm_tcpack_drop_skb(struct atbm_common *hw_priv,
					    struct atbm_tcpack_skbinfo *info)
{
	
 	struct atbm_tcpack_offload_comm *offload_priv = &hw_priv->offload_priv;
	struct sk_buff *skb;
	
	spin_lock_bh(&offload_priv->lock);
	if (info->skb){
		skb = info->skb;
		info->skb = NULL;
		spin_unlock_bh(&offload_priv->lock);
		dev_kfree_skb_any(skb);
		return;
	}
	spin_unlock_bh(&offload_priv->lock);
}

static void atbm_tcpack_timeout(unsigned long data)
{
	struct atbm_sockt_info *socket_info = (struct atbm_sockt_info *)data;
	struct atbm_tcpack_skbinfo *info;
	struct atbm_tcpack_offload_comm *offload_priv = NULL;

	offload_priv = container_of(socket_info, struct atbm_tcpack_offload_comm,socket_info[socket_info->index]);

	spin_lock_bh(&socket_info->lock);
	info = socket_info->framebuf;
	if (socket_info->vaild && info) {
		socket_info->framebuf = NULL;
		socket_info->offload_cnt = 0;
		spin_unlock_bh(&socket_info->lock);
		atbm_tcpack_tx(offload_priv->priv, info);//send skb
		return;
	}
	spin_unlock_bh(&socket_info->lock);
	
}

int atbm_tcpack_is_quickack(unsigned char *buf, struct atbm_tcpack_info *tcpack_info)
{
	int ip_hdr_len;
	struct ethhdr *ethhdr;
	struct atbm_iphdr *iphdr;
	struct atbm_tcphdr *tcphdr;

	ethhdr = (struct ethhdr *)buf;
	if (ethhdr->h_proto != htons(ETH_P_IP))
		return 0;
	iphdr = (struct atbm_iphdr *)(ethhdr + 1);
	if (iphdr->version != 4 || iphdr->protocol != IPPROTO_TCP)
		return 0;
	ip_hdr_len = iphdr->ihl * 4;
	tcphdr = (struct atbm_tcphdr*)((u8*)(iphdr)+ip_hdr_len);

	if (tcphdr->ack)
		return 0;

	if (tcphdr->psh) {
		tcpack_info->source = tcphdr->dest;
		tcpack_info->dest = tcphdr->source;
		tcpack_info->saddr = iphdr->daddr;
		tcpack_info->daddr = iphdr->saddr;
		tcpack_info->seq = ntohl(tcphdr->seq);
		return 1;
	}

	return 0;
}
/*
 return 0, just send
*/

int atbm_tcpack_checkopt(struct atbm_tcphdr *tcphdr, int tcp_tot_len)
{
	int drop = TCPACKF_FILTER_SEND;
	int hdrlen_bytes = tcphdr->doff * 4;
	/* Move the payload pointer in the pbuf so that it points to the
	   TCP data instead of the TCP header. */
	u16 tcphdr_optlen = hdrlen_bytes - ATBM_TCP_HLEN;

	if (tcp_tot_len > hdrlen_bytes) {
		drop = TCPACKF_JUST_SEND;
	}
	else {
		//if have tcp option ,not drop frame
		if (tcphdr_optlen)
			drop = TCPACKF_JUST_SEND;
	}

	return drop;
}


int atbm_tcpack_check(struct atbm_tcpack_offload_comm *offload_priv, struct atbm_tcpack_skbinfo* framebuf)
{
	int ret;
	int ip_hdr_len;
	int tcp_tot_len;
	struct ethhdr *ethhdr;
	struct atbm_iphdr *iphdr;
	struct atbm_tcphdr *tcphdr;
	struct atbm_tcpack_info atbm_tcpack_info;
	unsigned char* buf = framebuf->skb->data;
	int index;

	ethhdr =(struct ethhdr *)buf;
	if (ethhdr->h_proto != htons(ETH_P_IP))
		return TCPACKF_JUST_SEND;

	iphdr = (struct atbm_iphdr *)(ethhdr + 1);
	if (iphdr->version != 4 || iphdr->protocol != IPPROTO_TCP)
		return TCPACKF_JUST_SEND;

	ip_hdr_len = iphdr->ihl * 4;
	tcphdr = (struct atbm_tcphdr *)((u8 *)(iphdr) + ip_hdr_len);
	
	if (!(tcphdr->ack))
		return TCPACKF_JUST_SEND;

	tcp_tot_len = ntohs(iphdr->tot_len) - ip_hdr_len;// tcp total len
	ret = atbm_tcpack_checkopt(tcphdr, tcp_tot_len);


	atbm_tcpack_info.saddr = iphdr->saddr;
	atbm_tcpack_info.daddr = iphdr->daddr;
	atbm_tcpack_info.source = tcphdr->source;
	atbm_tcpack_info.dest = tcphdr->dest;
	atbm_tcpack_info.seq = ntohl(tcphdr->ack_seq);

	index = atbm_tcpack_find_socketid(offload_priv, &atbm_tcpack_info);
	if ((index >= 0)) {
		struct atbm_sockt_info* socket_info = &offload_priv->socket_info[index];
		static unsigned long time_printf = 0;
		if (socket_info->check_cnt == 0) {
			socket_info->check_start_time = jiffies;
			socket_info->check_cnt++;
			socket_info->throughput_flag = 0;
		}
		else {
			if (socket_info->check_cnt > 100) {
				if (!time_after(jiffies, socket_info->check_start_time + msecs_to_jiffies(200))) {
					socket_info->throughput_flag = 1;
				}
				else {
					socket_info->throughput_flag = 0;
				}
				socket_info->check_start_time = jiffies;
				socket_info->check_cnt = 0;
			}
			socket_info->check_cnt++;
		}
		if ((ntohs(tcphdr->window) < TCPACKOFFLOAD_MIN_WIN) && !socket_info->throughput_flag){
			ret = TCPACKF_JUST_SEND;
		}
		if (time_after(jiffies, time_printf + 1*HZ)){
			time_printf = jiffies;
			printk("window %dKB\n",ntohs(tcphdr->window) /1000);
		}
		ret = atbm_tcpack_update_cache(framebuf, offload_priv, socket_info,&atbm_tcpack_info, ret == TCPACKF_FILTER_SEND);
	}
	else {
		index = atbm_tcpack_alloc_socketid(offload_priv);
		if (index >= 0) {
			struct atbm_tcpack_info* p_atbm_tcpack_info_tmp = &offload_priv->socket_info[index].atbm_tcpack_info;
			spin_lock_bh(&offload_priv->socket_info[index].lock);
			offload_priv->socket_info[index].vaild = 1;
			offload_priv->socket_info[index].psh_flag = 0;
			offload_priv->socket_info[index].throughput_flag = 0;
			offload_priv->socket_info[index].last_time = jiffies;
			offload_priv->socket_info[index].offload_cnt = 0;
			offload_priv->socket_info[index].check_cnt = 0;
			
			memcpy(p_atbm_tcpack_info_tmp, &atbm_tcpack_info, sizeof(atbm_tcpack_info));
			spin_unlock_bh(&offload_priv->socket_info[index].lock);
		}
		ret = TCPACKF_JUST_SEND;
	}
	return ret;
}

int atbm_tcpack_find_socketid(struct atbm_tcpack_offload_comm *offload_priv,
				struct atbm_tcpack_info *atbm_tcpack_info)
{
	int i, index=-1;
	struct atbm_sockt_info *socket_info;
	struct atbm_tcpack_info *ack_info;
	
	spin_lock_bh(&offload_priv->lock);

	for (i = 0; (i < TCPACKOFFLOAD_MAX_SOCKET_ID); i++) {
		socket_info = &offload_priv->socket_info[i];

		ack_info = &socket_info->atbm_tcpack_info;
		if (socket_info->vaild &&
			ack_info->source == atbm_tcpack_info->source &&
			ack_info->dest == atbm_tcpack_info->dest &&
			ack_info->saddr == atbm_tcpack_info->saddr &&
			ack_info->daddr == atbm_tcpack_info->daddr){
			socket_info->last_time = jiffies;
			index = i;
			break;
		}
	}	
	
	spin_unlock_bh(&offload_priv->lock);


	return index;
}



int atbm_tcpack_alloc_socketid(struct atbm_tcpack_offload_comm *offload_priv)
{
	int i, index = -1;
	struct atbm_sockt_info* socket_info;
	//unsigned start;
	struct atbm_tcpack_info* ack;

	spin_lock_bh(&offload_priv->lock);
	//if (offload_priv->socket_info_cnt == TCPACKOFFLOAD_MAX_SOCKET_ID) {
	//	spin_unlock_bh(&offload_priv->lock);
	//	return -1;
	//}

	for (i = 0; (i < TCPACKOFFLOAD_MAX_SOCKET_ID); i++) {
		socket_info = &offload_priv->socket_info[i];
		if (!socket_info->vaild) {
			socket_info->vaild = 1;
			socket_info->last_time = jiffies;
			ack = &socket_info->atbm_tcpack_info;
			ack->source = 0;
			offload_priv->socket_info_cnt++;
			index = i;
			break;
		}
	}
	//if no free socket , find the oldest one to use
	if (index < 0) {
		for (i = 0; (i < TCPACKOFFLOAD_MAX_SOCKET_ID); i++) {
			socket_info = &offload_priv->socket_info[i];
			if (time_after(jiffies, socket_info->last_time + 10*HZ)){
				ack = &socket_info->atbm_tcpack_info;
				ack->dest = 0;
				ack->source = 0;
				socket_info->last_time = jiffies;
				index = i;
				break;
			}
		}
	}
	spin_unlock_bh(&offload_priv->lock);

	return index;
}


/* return val: 0 for not handle tx, 1 for handle tx */
int atbm_tcpack_update_cache(struct atbm_tcpack_skbinfo *new_msgbuf,
			  struct atbm_tcpack_offload_comm *offload_priv,
			  struct atbm_sockt_info *socket_info,
			  struct atbm_tcpack_info *ack_now,
			  bool is_newfilter)
{
	bool is_quick_ack = 0;
	struct atbm_tcpack_info *ack;
	int ret = TCPACKF_JUST_SEND;
	struct atbm_tcpack_skbinfo *p_drop_skbinfo = NULL;
	

	spin_lock_bh(&socket_info->lock);

	socket_info->last_time = jiffies;
	ack = &socket_info->atbm_tcpack_info;
	//if the ack_now  send  is newer than the seq ?
	//we need drop old ack
	if (TCP_SEQ_LEQ(ack->seq, ack_now->seq) || (ack->dest == 0)) {
		if (time_after(jiffies, socket_info->last_time + msecs_to_jiffies(1))) {
			is_quick_ack = 1;
		}
		//drop old frame
		if (socket_info->framebuf) {
			p_drop_skbinfo = socket_info->framebuf;
			socket_info->framebuf = NULL;
		}
		//if need push ack, so we must not buffer ack
		//if now ack is newer than old clear psh_flag
		
		if (socket_info->psh_flag ) {
			if(TCP_SEQ_LT(ack_now->seq,socket_info->psh_seq)){
			}
			else {
				socket_info->psh_flag = 0;
				socket_info->offload_cnt = 0;
			}
			is_quick_ack = 1;
		}
		else if (is_newfilter) {
			socket_info->offload_cnt++;
		}

		ack->seq = ack_now->seq;

		if (is_quick_ack || (socket_info->offload_cnt >= offload_ack_num) || !is_newfilter) {
			socket_info->offload_cnt = 0;
			atbm_del_timer(&socket_info->timer);
			ret = TCPACKF_JUST_SEND;
		}
		else {
			ret = TCPACKF_FILTER_SEND;
			socket_info->framebuf = new_msgbuf;
			if (!atbm_timer_pending(&socket_info->timer))
				atbm_mod_timer(&socket_info->timer, (jiffies + socket_info->timeout));
		}

	}
	else {
		ret = TCPACKF_JUST_SEND;
	}	
_exit:
	spin_unlock_bh(&socket_info->lock);



	if (p_drop_skbinfo){
		atbm_tcpack_drop_skb(offload_priv->priv, p_drop_skbinfo);// drop skb
	}

	return ret;

}

void atbm_tcpack_update_when_rx(struct atbm_common *priv,struct sk_buff* skb)
{
	int index;
	struct atbm_tcpack_info atbm_tcpack_info;
	struct atbm_sockt_info *socket_info;
	struct atbm_tcpack_offload_comm *offload_priv = &priv->offload_priv;
	u8 * buf = skb->data;

	if (!atomic_read(&offload_priv->enable))
		return;

	if (cpu_to_le16(skb->len) > TCPACKOFFLOAD_MAX_ACK_LEN)
		return;
	
	if (!atbm_tcpack_is_quickack(buf, &atbm_tcpack_info))
		return;

	index = atbm_tcpack_find_socketid(offload_priv, &atbm_tcpack_info);
	if (index >= 0) {
		socket_info = offload_priv->socket_info + index;
		spin_lock_bh(&socket_info->lock);
		socket_info->psh_flag = 1;
		socket_info->psh_seq = atbm_tcpack_info.seq;
		spin_unlock_bh(&socket_info->lock);
	}
}

/* return 
0 for not offload, 
1 for offload 
*/
int atbm_tcpack_offload_tx_check(struct atbm_common *hw_priv, struct sk_buff* skb)
{
	int drop;
	struct atbm_tcpack_offload_comm* offload_priv = &hw_priv->offload_priv;
	struct atbm_tcpack_skbinfo *framebuf = NULL;

	if(!offload_ack){
		return 0;
	}

	if (!atomic_read(&offload_priv->enable))
		return 0;
	

	if (cpu_to_le16(skb->len) > TCPACKOFFLOAD_MAX_ACK_LEN)
		return 0;

	framebuf = atbm_tcpack_alloc_skbinfo(hw_priv,skb);
	if(framebuf == NULL){
		return 0;		
	}

	drop = atbm_tcpack_check(offload_priv,framebuf);

	if(drop == TCPACKF_JUST_SEND){
		atbm_tcpack_free_skbinfo(hw_priv,framebuf);
		return 0;
	}
	
	return drop == TCPACKF_FILTER_SEND;
}

//struct atbm_common 
void atbm_tcpack_offload_init(struct atbm_common *priv)
{
	int i;
	struct atbm_sockt_info* socket_info;
	struct atbm_tcpack_offload_comm* offload_priv;
	
	if(!offload_ack){
		return;
	}
	offload_priv = &priv->offload_priv;

	memset(offload_priv, 0, sizeof(struct atbm_tcpack_offload_comm));
	offload_priv->priv = priv;
	spin_lock_init(&offload_priv->lock);

	for (i = 0; i < TCPACKOFFLOAD_MAX_SOCKET_ID; i++) {
		socket_info = &offload_priv->socket_info[i];
		socket_info->index = i;
		socket_info->vaild = 0;
		spin_lock_init(&socket_info->lock);
		socket_info->last_time = jiffies;
		socket_info->timeout = msecs_to_jiffies(offload_ack_to);
		atbm_setup_timer(&socket_info->timer,
			atbm_tcpack_timeout,
			(unsigned long)socket_info);
	}
	for (i = 0; i < TCPACKOFFLOAD_MAX_FRAMEBUF_ID; i++) {
		offload_priv->framebuf[i].skb = NULL;
	}	

	atomic_set(&offload_priv->enable, 1);
}

void atbm_tcpack_offload_deinit(struct atbm_common *priv)
{
	int i;
	struct atbm_tcpack_offload_comm *offload_priv;
	struct atbm_tcpack_skbinfo *p_drop_skbinfo = NULL;
	if(!offload_ack){
		return;
	}
	offload_priv = &priv->offload_priv;

	atomic_set(&offload_priv->enable, 0);

	for (i = 0; i < TCPACKOFFLOAD_MAX_SOCKET_ID; i++) {
		p_drop_skbinfo = NULL;

		spin_lock_bh(&offload_priv->socket_info[i].lock);
		atbm_del_timer(&offload_priv->socket_info[i].timer);
		p_drop_skbinfo = offload_priv->socket_info[i].framebuf;
		offload_priv->socket_info[i].framebuf = NULL;
		spin_unlock_bh(&offload_priv->socket_info[i].lock);

		if (p_drop_skbinfo)
			atbm_tcpack_drop_skb(priv, p_drop_skbinfo);//drop skb
	}
}
#endif  //#ifdef CONFIG_TCP_ACK_OFFLOAD

