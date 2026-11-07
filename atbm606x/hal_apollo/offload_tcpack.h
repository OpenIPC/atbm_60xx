#ifndef __ATBM_TCPACKOFFLOAD__H_
#define __ATBM_TCPACKOFFLOAD__H_




#define TCPACKOFFLOAD_MAX_SOCKET_ID  		32
#define TCPACKOFFLOAD_MAX_FRAMEBUF_ID  	TCPACKOFFLOAD_MAX_SOCKET_ID*2

#define TCPACKOFFLOAD_MAX_OFFLOAD_CNT	5
#define TCPACKOFFLOAD_AGE_TIME			3//ms
#define TCPACKOFFLOAD_MAX_ACK_LEN 		200
#define TCPACKOFFLOAD_MIN_WIN 			(128*1024)

/*
struct tcphdr {
	__be16	source;		//源端口
	__be16	dest;		//目的端口
	__be32	seq;			//数据段的起始序列号	
	__be32	ack_seq;		//确认序列号
#if defined(__LITTLE_ENDIAN_BITFIELD)   //小端                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
	__u16	res1:4,
		doff:4,			//协议头长度
		fin:1,			//断开标志
		syn:1,			
		rst:1,
		psh:1,
		ack:1,
		urg:1,
		ece:1,
		cwr:1;
#elif defined(__BIG_ENDIAN_BITFIELD)    //大端
	__u16	doff:4,
		res1:4,
		cwr:1,
		ece:1,
		urg:1,
		ack:1,
		psh:1,
		rst:1,
		syn:1,
		fin:1;
#else
#error	"Adjust your <asm/byteorder.h> defines"
#endif	
	__be16	window;		//窗口控制
	__sum16	check;		//阻塞控制
	__be16	urg_ptr;		

}
*/


#define TCP_FIN_FLAG 0x01U
#define TCP_SYN_FLAG 0x02U
#define TCP_RST_FLAG 0x04U
#define TCP_PSH_FLAG 0x08U
#define TCP_ACK_FLAG 0x10U
#define ATBM_TCP_HLEN 20
#define ATBM_TCP_OPT_LEN_TS 10

#define TCP_SEQ_LT(a,b)  (((s32)((s32)(a)-(s32)(b))) < 0)
#define TCP_SEQ_LEQ(a,b)  (((s32)((s32)(a)-(s32)(b))) <= 0)
#define TCP_SEQ_GT(a,b)  (((s32)((s32)(a)-(s32)(b))) > 0)
#define TCP_SEQ_GEQ(a,b)  (((s32)((s32)(a)-(s32)(b))) >= 0)

#define TCP_SEQ_BETWEEN(a,b,c) (TCP_SEQ_GEQ(a,b)&&TCP_SEQ_LEQ(a,c))
#define ATBM_SND_WND_SCALE(snd_scale, wnd) (((wnd) << (snd_scale))




#define ATBM_IPH_V(hdr)  ((hdr)->_v_hl >> 4)
#define ATBM_IPH_HL(hdr) ((hdr)->_v_hl & 0x0f)
#define ATBM_IPH_TOS(hdr) ((hdr)->_tos)
#define ATBM_IPH_LEN(hdr) ((hdr)->_len)
#define ATBM_IPH_ID(hdr) ((hdr)->_id)
#define ATBM_IPH_OFFSET(hdr) ((hdr)->_offset)
#define ATBM_IPH_TTL(hdr) ((hdr)->_ttl)
#define ATBM_IPH_PROTO(hdr) ((hdr)->_proto)
#define ATBM_IPH_CHKSUM(hdr) ((hdr)->_chksum)
#define ATBM_IP_PROTO_ICMP    1
#define ATBM_IP_PROTO_IGMP    2
#define ATBM_IP_PROTO_UDP     17
#define ATBM_IP_PROTO_UDPLITE 136
#define ATBM_IP_PROTO_TCP     6

struct atbm_tcpack_skbinfo {
	//struct list_head list;
	struct sk_buff *skb;

};

struct atbm_tcpack_info {
	u16 source;
	u16 dest;
	s32 saddr;
	s32 daddr;
	u32 seq;
};


struct atbm_sockt_info {
	bool vaild;
	bool psh_flag;
	bool throughput_flag;
	int index;
	int offload_cnt;
	int check_cnt;
	u32 psh_seq;
	spinlock_t lock;
	unsigned long last_time;
	unsigned long check_start_time;
	unsigned long timeout;
	struct atbm_timer_list timer;
	struct atbm_tcpack_skbinfo *framebuf;
	struct atbm_tcpack_info atbm_tcpack_info;
};

struct atbm_tcpack_offload_comm {
	/* 1 filter */
	atomic_t enable;
	int socket_info_cnt;
	//unsigned long last_time;
	//unsigned long timeout;
	/* lock for tcp ack alloc and free */
	spinlock_t lock;
	struct atbm_common *priv;
	struct atbm_sockt_info socket_info[TCPACKOFFLOAD_MAX_SOCKET_ID];
	struct atbm_tcpack_skbinfo framebuf[TCPACKOFFLOAD_MAX_FRAMEBUF_ID];
};
//
//return state
//
#define TCPACKF_JUST_SEND 0
#define TCPACKF_FILTER_SEND 1

void atbm_tcpack_offload_init(struct atbm_common *priv);
void atbm_tcpack_offload_deinit(struct atbm_common *priv);
int atbm_tcpack_offload_tx_check(struct atbm_common* hw_priv, struct sk_buff* skb);
void atbm_tcpack_update_when_rx(struct atbm_common* priv, struct sk_buff* skb);

#endif //__ATBM_TCPACKOFFLOAD__H_
