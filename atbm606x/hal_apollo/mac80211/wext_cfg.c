/*
 * Copyright 2018-, altobeam.inc
 */

#include <net/atbm_mac80211.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/skbuff.h>
#include <linux/etherdevice.h>
#include <linux/if_arp.h>
#include <linux/rtnetlink.h>
#include <linux/bitmap.h>
#include <linux/inetdevice.h>
#include <net/net_namespace.h>
#include <net/cfg80211.h>
#ifdef CONFIG_WIRELESS_EXT
#include <net/iw_handler.h>
#endif
#ifdef IPV6_FILTERING
#include <net/if_inet6.h>
#include <net/addrconf.h>
#endif /*IPV6_FILTERING*/

#include "ieee80211_i.h"
#include "driver-ops.h"
#include "rate.h"
#include "mesh.h"
#include "wep.h"
#include "cfg.h"
#include "debugfs.h"
#include "../sbus.h"
#include "atbm_common.h"
#include "../apollo.h"
#include "../smartconfig.h"
#include "../wsm.h"
#include "../internal_cmd.h"
#include "../sta.h"
#include "rc80211_hmac.h"
#include "../dev_ioctl.h"
#include "twt.h"
#ifdef CONFIG_ATBM_SUPPORT_BLUEZ
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>
#endif


//#define CONFIG_JUAN_MISC

#define ATBM_WEXT_PROCESS_PARAMS(_start_pos,_length,_val,_process,_err_code,_exit_lable,_status)		\
do{													\
	char const *pos_next = NULL;					\
	pos_next = memchr(_start_pos,ATBM_SPACE,_length);	\
	if(pos_next == NULL) pos_next = memchr(_start_pos,ATBM_TAIL,_length);	\
	if(_process(_start_pos,pos_next - _start_pos,&_val) == _err_code){	\
		atbm_printk_err("%s:line(%d) err\n",__func__,__LINE__);			\
		_status = -EINVAL;											\
		goto _exit_lable;												\
	}															\
	pos_next++;													\
	_length -= (pos_next-_start_pos);										\
	_start_pos = pos_next;												\
}while(0)

#define FREQ_CNT	(10)

#define TARGET_FREQOFFSET_HZ  (7000)



#define HW_CHIP_VERSION_AthenaB (0x24)
#define HW_CHIP_VERSION_AresB (0x49)
#define N_BIT_TO_SIGNED_32BIT(v,n)	(s32)(((v) & BIT(n-1))?((v)|0xffffffff<<n):(v))
#define SIGNED_32BIT_TO_N_BIT(g,n) (g &= (BIT(n)-1))






struct rxstatus{
	u32 GainImb;
	u32 PhaseImb;
	u32 Cfo;
	u32 evm;
	u32  RSSI;
	u32 probcnt;
};

typedef struct PAS_MONITOR {
	u32 rxSuccessUnicastDataFrameCnt[2];
	u32 rxSuccessUnicastCtrlFrameCnt[2];
	u32 rxSuccessUnicastMgmFrameCnt[2];
	u32 rxSuccessMulticastMgmFrameCnt;
	u32 rxSuccessMulticastDataFrameCnt;
	u32 rxSuccessBeaconCnt;
	u32 rxSuccessOtherMgmFrameCnt;
	u32 rxSuccessOtherDataFrameCnt;
	u32 rxSuccessUnicastOther;
	u32 rxSuccessMulticastOther;
	u32 rxSuccessUnicastDataFrameByte[2];
	u32 rxU32NRGAINIMB;
	u32 rxU32NRPHASEIMB;
    u16          Cfo;
    u16			evm;
	u8           Rssi;
	u8           Rcpi;
	short           atbm_probCnt;
	//u32 rxSuccessCnt[2];
}PAS_MONITOR ;


u32 chipversion = 0;

u8 ETF_bStartTx = 0;
u8 ETF_bStartRx = 0;
char ch_and_type[20] = {0};
int g_RxMode = 0;


u8 CodeStart = 0;
u8 CodeEnd = 0;

u8 ucWriteEfuseFlag = 0;

int Atbm_Test_Success = 0;
int atbm_test_rx_cnt = 0;
int txevm_total = 0;

struct efuse_headr efuse_data_etf;
struct rxstatus_signed gRxs_s;
struct etf_test_config etf_config;
extern int g_CertSRRCFlag;
extern int rate_txpower_cfg[RATE_CFG_TXPOWER_ARRY_MAX];
extern int rate_txpower_cfg_5g[RATE_CFG_TXPOWER_ARRY_MAX];

extern const s8 MCS_LUT_20M[OFDM_20M_INDEX_MAX];
extern u8 MCS_LUT_20M_Modify[OFDM_20M_INDEX_MAX]; 
extern s8 MCS_LUT_20M_delta_mode[OFDM_20M_INDEX_MAX]; 						
extern s8 MCS_LUT_20M_delta_bw[OFDM_20M_INDEX_MAX];

extern const s8 MCS_LUT_40M[OFDM_40M_INDEX_MAX]; 
extern s8 MCS_LUT_40M_Modify[OFDM_40M_INDEX_MAX];
extern s8 MCS_LUT_40M_delta_mode[OFDM_40M_INDEX_MAX]; 
extern s8 MCS_LUT_40M_delta_bw[OFDM_40M_INDEX_MAX]; 
							
extern const char *rate_name[]; 

//-----------cronus
extern const s8 MCS_LUT_20M_Cronus[CRONUS_OFDM_INDEX_MAX];
extern u8 MCS_LUT_20M_Modify_Cronus[CRONUS_OFDM_INDEX_MAX]; 
extern s8 MCS_LUT_20M_delta_mode_Cronus[CRONUS_OFDM_INDEX_MAX]; 						
extern s8 MCS_LUT_20M_delta_bw_Cronus[CRONUS_OFDM_INDEX_MAX];

extern const s8 MCS_LUT_40M_Cronus[CRONUS_OFDM_INDEX_MAX]; 
extern s8 MCS_LUT_40M_Modify_Cronus[CRONUS_OFDM_INDEX_MAX];
extern s8 MCS_LUT_40M_delta_mode_Cronus[CRONUS_OFDM_INDEX_MAX]; 
extern s8 MCS_LUT_40M_delta_bw_Cronus[CRONUS_OFDM_INDEX_MAX]; 
							
extern const char *rate_name_Cronus[]; 


extern int WiFiMode;
extern int OFDMMode;
extern int ChBW;
extern int RateIndex;
extern int g_PacketInterval;
extern int g_DutyCycle;
extern u16 g_ch_idle_ratio;
extern int atbm_Get_MCS_LUT_Offset_Index(int WiFiMode,int OFDMMode,int ChBW,int RateIndex);
extern u8 atbm_DCXOCodeRead(struct atbm_common *hw_priv);
extern int atbm_DCXOCodeWrite(struct atbm_common *hw_priv,int data);
extern unsigned int HW_READ_REG_BIT(unsigned int addr,int endbit,int startbit);
extern void HW_WRITE_REG_BIT(unsigned int addr,unsigned int endBit,unsigned int startBit,unsigned int data );
extern int wsm_start_tx_v2(struct atbm_common *hw_priv, struct ieee80211_vif *vif );
extern int atbm_get_cfo_allow_flag(void);

#ifdef CONFIG_ATBM_PRODUCT_TEST_USE_GOLDEN_LED
extern int wsm_send_result(struct atbm_common *hw_priv, struct ieee80211_vif *vif );
#endif
extern u32 atbm_get_chip_crystal_type(struct atbm_common *hw_priv);
#define RATE_CONTROL_UNICAST 1

#define	arraysize(a)	sizeof(a)/sizeof(a[0])
extern int atbm_direct_write_reg_32(struct atbm_common *hw_priv, u32 addr, u32 val);
extern int atbm_direct_read_reg_32(struct atbm_common *hw_priv, u32 addr, u32 *val);
extern int atbm_usb_write_bit(struct atbm_common *hw_priv,u32 addr,u8 endBit,u8 startBit,u32 data );

#ifdef CONFIG_WIRELESS_EXT

static int atbm_ioctl_stop_rx(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext);
static int atbm_ioctl_stop_tx(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext);

extern void atbm_set_freq(struct ieee80211_local *local);
extern void atbm_set_tx_power(struct atbm_common *hw_priv, int txpw);
extern int atbm_str2mac(char *dst_mac, char *src_str);
extern int atbm_tool_rts_threshold;
extern int atbm_tool_shortGi;
extern int atbm_tesmode_reply(struct wiphy *wiphy,const void *data, int len);
extern void atbm_set_shortGI(u32 shortgi);
extern int wsm_start_tx(struct atbm_common *hw_priv, struct ieee80211_vif *vif);
extern int wsm_stop_tx(struct atbm_common *hw_priv);
static int atbm_ioctl_stop_tx(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext);
static int atbm_ioctl_stop_rx(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext);
int atbm_ioctl_etf_result_get(struct net_device *dev, struct iw_request_info *info, union iwreq_data  *wrqu, char *extra);
extern int wsm_get_efuse_status(struct atbm_common *hw_priv, struct ieee80211_vif *vif );


u32 atbm_get_chip_crystal_type(struct atbm_common *hw_priv);

#ifdef CONFIG_ATBM_IWPRIV_USELESS
static int Test_FreqOffset(struct atbm_common *hw_priv, u32 *dcxo, int *pfreqErrorHz, struct rxstatus_signed *rxs_s, int channel);
#endif
/*
static const u32 band_table[21] = {10, 20, 55, 110, 60, 90, 120,
								180, 240, 360, 480, 540, 65, 130,
								195, 260, 390, 520, 585, 650, 320};

*/

struct iw_handler_def atbm_handlers_def;


typedef struct iwripv_common_cmd{
	const char *cmd;
	const char cmd_len;
	int (*handler)(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext);
	const char *uage;
}iwripv_common_cmd_t;


static int atbm_ioctl_common_help(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_get_rssi(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);

static int atbm_ioctl_get_Private_256BITSEFUSE(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_set_Private_256BITSEFUSE(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext);



static int atbm_ioctl_get_channel_idle(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_set_efuse(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext);
#ifdef CONFIG_ATBM_SUPPORT_BLUEZ
static int atbm_ioctl_set_ble_pub_mac(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext);
#endif
static int atbm_ioctl_get_efuse_first(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext);
static int atbm_ioctl_get_efuse(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext);
static int atbm_ioctl_get_efuse_free_space(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext);
static int atbm_ioctl_get_efuse_all_data(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext);



#ifdef CONFIG_ATBM_IWPRIV_USELESS
static int atbm_ioctl_freqoffset(struct net_device *dev, struct iw_request_info *info, union iwreq_data  *wrqu, char *extra);
static int atbm_ioctl_channel_test_start(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);

#endif/*CONFIG_ATBM_IWPRIV_USELESS*/


static int atbm_ioctl_send_singleTone(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext);
static int atbm_ioctl_set_duty_ratio(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext);

#ifdef CONFIG_ATBM_SUPPORT_AP_CONFIG
static int atbm_ioctl_set_ap_conf(struct net_device *dev, struct iw_request_info *info,union iwreq_data *wrqu, char *ext);
#endif/*CONFIG_ATBM_SUPPORT_AP_CONFIG*/

#ifdef CONFIG_ATBM_MONITOR_SPECIAL_MAC
static int atbm_ioctl_rx_monitor_mac(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_rx_monitor_mac_status(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
#endif/*CONFIG_ATBM_MONITOR_SPECIAL_MAC*/

#ifdef CONFIG_IEEE80211_SPECIAL_FILTER
static int atbm_ioctl_rx_filter_frame(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_rx_filter_ie(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_rx_filter_clear(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_rx_filter_show(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
#endif/*CONFIG_IEEE80211_SPECIAL_FILTER*/
#ifdef HIDDEN_SSID
extern int atbm_upload_beacon_private(struct atbm_vif *priv);
static int atbm_ioctl_set_hidden_ssid(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
#endif/*HIDDEN_SSID*/

static int atbm_ioctl_get_Last_mac(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_get_First_mac(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_get_Tjroom(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_set_calibrate_flag(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext);
static int atbm_ioctl_set_calibrate_TPC(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_set_calibrate_deltagain(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_set_all_efuse(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext);
static int atbm_ioctl_get_dcxo(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_set_dcxo(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_associate_sta_status(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
#ifdef ATBM_PRIVATE_IE
static int atbm_ioctl_ie_ipc_clear_insert_data(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_ie_insert_vendor_ie(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext);
#endif/*ATBM_PRIVATE_IE*/
static int atbm_set_power_save_mode(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext);
static int atbm_ioctl_gpio_config(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_gpio_output(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_edca_params(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_get_edca_params(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_get_txposer_status(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_set_powerTarget(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_get_powerTarget(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_set_txpower(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_get_txpower(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_set_ladder_txpower(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_set_rate_txpower_mode(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_get_txrx_info(struct net_device *dev, struct iw_request_info *info,  union iwreq_data  *wrqu, char *extra);
static int atbm_ioctl_ctrl_use_deltagain_and_tpc(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_set_power_threshold(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_set_reduce_tx_rx_power_consumption(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);

extern void atbm_get_delta_gain(char *srcData,int *allgain);
static int atbm_ioctl_set_txpwr_by_file(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_set_rate_txpwr_by_file(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_set_management_frame_rate(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_set_rts_threshold(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);


static int atbm_ioctl_atbm_get_work_channel(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext);
//#ifdef CONFIG_CPTCFG_CFG80211_COUNTRY_CODE

static int atbm_ioctl_set_country_code(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext);
static int atbm_ioctl_get_country_code(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext);
//#endif
static int atbm_ioctl_get_rate(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra);
static int atbm_ioctl_get_cur_max_rate(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_get_rx_rate(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);

static int atbm_ioctl_get_cfg_txpower_by_file(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
int atbm_ioctl_get_gpio4(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
int atbm_ioctl_get_cfo_cali_data(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
extern void atbm_send_deauth_disossicate(struct ieee80211_sub_if_data *sdata,u8 *bssid);
int atbm_ioctl_send_deauth(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
int atbm_ioctl_send_vendor_ie(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
int atbm_ioctl_get_vendor_ie(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
int atbm_ioctl_set_listen_probe_req(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
#ifdef SDIO_BUS
//static int atbm_ioctl_device_hibernate(struct net_device *dev, struct iw_request_info *ifno, union iwreq_data *wrqu, char *ext);
#endif

#ifdef CONFIG_ATBM_AP_CHANNEL_CHANGE_EVENT
int atbm_ioctl_change_ap_channel(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
#endif
static int atbm_ioctl_get_cca_threshold(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_get_tx_parameter(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_get_rts_threshold(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_get_rts_duration(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_get_noise_level(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
#ifdef CONFIG_ATBM_SUPPORT_REKEY
static int atbm_ioctl_set_rekey(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
#endif
static int atbm_ioctl_get_snr(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_get_tx_packet_status(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);

static int atbm_ioctl_update_beacon_loss_conter(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);

#ifdef CONFIG_JUAN_MISC

static int atbm_ioctl_common_get_tim(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_common_set_tim(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
#endif

#ifdef CONFIG_IEEE80211_SEND_SPECIAL_MGMT
static int atbm_ioctl_send_probe_resp(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_stop_send_probe_resp(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);

static int atbm_ioctl_send_action(struct net_device *dev, struct iw_request_info *info,  union iwreq_data  *wrqu, char *extra);
static int atbm_ioctl_stop_send_action(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
#endif

#ifdef ATBM_SUPPORT_LSLP_PRIV_COMMUNICATION
static int atbm_ioctl_set_lightsleep_info(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_set_lightsleep_start(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
#endif

#ifdef SDIO_BUS
static int atbm_ioctl_device_hibernate(struct net_device *dev, struct iw_request_info *ifno, union iwreq_data *wrqu, char *ext);
#endif
static int atbm_ioctl_set_tx_rate(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
#ifdef CONFIG_ATBM_BLE_ADV_COEXIST
static int atbm_ioctl_ble_coexist_cmd(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_ble_set_adv_data_cmd(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
#endif
static int atbm_ioctl_fw_reset(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_set_packet_interval(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);

#ifdef WLAN_CAPABILITY_RADIO_MEASURE
static int atbm_ioctl_wfa_certificate(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
#endif
#ifdef CONFIG_ATBM_SUPPORT_GRO
static int atbm_ioctl_napi_stats(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
#endif
static int atbm_ioctl_set_ampdu(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_set_single_scan_time(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
#ifdef ATBM_USE_FASTLINK
static int atbm_ioctl_reset_fastlink(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
#endif

static int atbm_ioctl_get_wifi_state(struct net_device *dev, struct iw_request_info *info,  union iwreq_data  *wrqu, char *extra);
static int atbm_ioctl_read_reg_bit(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext);
static int atbm_ioctl_write_reg_bit(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext);
static int atbm_ioctl_get_pa_status(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_best_ch_scan2(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext);
static int atbm_ioctl_best_ch_scan3(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext);

static int atbm_ioctl_common_set_temp_dcxo_table(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_set_efuse_gain_trim(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_set_bus_awake(struct net_device* dev, struct iw_request_info* info, union iwreq_data* wrqu, char* ext);
//static int atbm_ioctl_gpio_output_getval(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
static int atbm_ioctl_2AntExtraIoSet(struct net_device *dev, struct iw_request_info *ifno, union iwreq_data *wrqu, char *ext);


static int atbm_ioctl_set_tx_om(struct net_device* dev, struct iw_request_info* info, union iwreq_data* wrqu, char* extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	union iwreq_data* wdata = (union iwreq_data*)wrqu;
	char* ptr = NULL;
	char const* pos = NULL;
	char const* pos_end = NULL;
	int ret = 0;
	int i = 0;
	int len = 0;
	int enable;

	if (!ieee80211_sdata_running(sdata)) {
		ret = -ENETDOWN;
		goto exit;
	}
	if(sdata->vif.type != NL80211_IFTYPE_STATION){
		ret = -ENETDOWN;
		goto exit;
	}
	ptr = (char*)atbm_kzalloc(wdata->data.length + 1, GFP_KERNEL);

	if (!ptr) {
		ret = -ENOMEM;
		goto exit;
	}

	if (copy_from_user(ptr, wdata->data.pointer, wdata->data.length)) {
		ret = -EINVAL;
		goto exit;
	}

	ptr[wrqu->data.length] = 0;

	for (i = 0;i < wrqu->data.length;i++) {
		if (ptr[i] == ',')
			ptr[i] = ATBM_SPACE;
	}

	pos = atbm_skip_space(ptr, wrqu->data.length + 1);

	if (pos == NULL) {
		ret = -EINVAL;
		goto exit;
	}

	pos = pos + strlen("om");

	pos = atbm_skip_space(pos, wrqu->data.length + 1 - (pos - ptr));

	if (pos == NULL) {
		ret = -EINVAL;
		goto exit;
	}

	len = wrqu->data.length + 1 - (pos - ptr);

	if (len <= 0) {
		ret = -EINVAL;
		goto exit;
	}

	pos_end = memchr(pos, ATBM_TAIL, len);

	if (pos_end != NULL)
		len = pos_end - pos + 1;

	/*
	*parase enable
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos, len, enable, atbm_accsii_to_int, false, exit, ret);

	if(enable != 0 && enable != 1){
		ret = -EINVAL;
		goto exit;
	}
	if (sdata->vif.bss_conf.is_wifi6_ap) {
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
		htc.channel_width = ieee80211_chw_is_ht40(sdata->vif.bss_conf.channel_type);
		htc.ul_mu_disable = enable;
		ieee80211_send_htc_qosnullfunc(&local->hw, &sdata->vif, *((u32*)&htc));
		atbm_printk_err("send htc qos null\n");
	}

exit:
	if(ptr)
		atbm_kfree(ptr);
	return ret;
}
static int atbm_ioctl_mac_reset(struct net_device* dev, struct iw_request_info* info, union iwreq_data* wrqu, char* extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	union iwreq_data* wdata = (union iwreq_data*)wrqu;
	char* ptr = NULL;
	char const* pos = NULL;
	char const* pos_end = NULL;
	int ret = 0;
	int i = 0;
	int len = 0;
	int enable;

	if (!ieee80211_sdata_running(sdata)) {
		ret = -ENETDOWN;
		goto exit;
	}
	if(sdata->vif.type != NL80211_IFTYPE_STATION){
		ret = -ENETDOWN;
		goto exit;
	}
	ptr = (char*)atbm_kzalloc(wdata->data.length + 1, GFP_KERNEL);

	if (!ptr) {
		ret = -ENOMEM;
		goto exit;
	}

	if (copy_from_user(ptr, wdata->data.pointer, wdata->data.length)) {
		ret = -EINVAL;
		goto exit;
	}

	ptr[wrqu->data.length] = 0;

	for (i = 0;i < wrqu->data.length;i++) {
		if (ptr[i] == ',')
			ptr[i] = ATBM_SPACE;
	}

	pos = atbm_skip_space(ptr, wrqu->data.length + 1);

	if (pos == NULL) {
		ret = -EINVAL;
		goto exit;
	}

	pos = pos + strlen("mac_reset");

	pos = atbm_skip_space(pos, wrqu->data.length + 1 - (pos - ptr));

	if (pos == NULL) {
		ret = -EINVAL;
		goto exit;
	}

	len = wrqu->data.length + 1 - (pos - ptr);

	if (len <= 0) {
		ret = -EINVAL;
		goto exit;
	}

	pos_end = memchr(pos, ATBM_TAIL, len);

	if (pos_end != NULL)
		len = pos_end - pos + 1;

	/*
	*parase enable
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos, len, enable, atbm_accsii_to_int, false, exit, ret);

	if(enable != 0 && enable != 1){
		ret = -EINVAL;
		goto exit;
	}
	wsm_write_mib(local->hw.priv,WSM_MIB_ID_MAC_RESET,&enable,1,0);
exit:
	if(ptr)
		atbm_kfree(ptr);
	return ret;
}
static int atbm_ioctl_uapsd(struct net_device* dev, struct iw_request_info* info, union iwreq_data* wrqu, char* extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
//	struct ieee80211_local *local = sdata->local;
	union iwreq_data* wdata = (union iwreq_data*)wrqu;
	char* ptr = NULL;
	char const* pos = NULL;
	char const* pos_end = NULL;
	int ret = 0;
	int i = 0;
	int len = 0;
	int uapsd;

	if (!ieee80211_sdata_running(sdata)) {
		ret = -ENETDOWN;
		goto exit;
	}
	
	ptr = (char*)atbm_kzalloc(wdata->data.length + 1, GFP_KERNEL);

	if (!ptr) {
		ret = -ENOMEM;
		goto exit;
	}

	if (copy_from_user(ptr, wdata->data.pointer, wdata->data.length)) {
		ret = -EINVAL;
		goto exit;
	}

	ptr[wrqu->data.length] = 0;

	for (i = 0;i < wrqu->data.length;i++) {
		if (ptr[i] == ',')
			ptr[i] = ATBM_SPACE;
	}

	pos = atbm_skip_space(ptr, wrqu->data.length + 1);

	if (pos == NULL) {
		ret = -EINVAL;
		goto exit;
	}

	pos = pos + strlen("uapsd");

	pos = atbm_skip_space(pos, wrqu->data.length + 1 - (pos - ptr));

	if (pos == NULL) {
		ret = -EINVAL;
		goto exit;
	}

	len = wrqu->data.length + 1 - (pos - ptr);

	if (len <= 0) {
		ret = -EINVAL;
		goto exit;
	}

	pos_end = memchr(pos, ATBM_TAIL, len);

	if (pos_end != NULL)
		len = pos_end - pos + 1;

	/*
	*parase enable
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos, len, uapsd, atbm_accsii_to_int, false, exit, ret);

	if(uapsd > 0xFF){
		ret = -EINVAL;
		goto exit;
	}
	
	atbm_printk_err("[%s]:uapsd[%x]\n",sdata->name,uapsd);
	sdata->uapsd_queues = uapsd;
	
exit:
	if(ptr)
		atbm_kfree(ptr);
	return ret;
}

static int atbm_ioctl_twt(struct net_device* dev, struct iw_request_info* info, union iwreq_data* wrqu, char* extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	union iwreq_data* wdata = (union iwreq_data*)wrqu;
	struct ieee80211_if_managed *ifmgd;
	char* ptr = NULL;
	char const* pos = NULL;
	char const* pos_end = NULL;
	int ret = 0;
	int i = 0;
	int len = 0;
	int enable;

	if (!ieee80211_sdata_running(sdata)) {
		ret = -ENETDOWN;
		goto exit;
	}
	
	if(sdata->vif.type != NL80211_IFTYPE_STATION){
		ret = -ENETDOWN;
		goto exit;
	}

	ptr = (char*)atbm_kzalloc(wdata->data.length + 1, GFP_KERNEL);

	if (!ptr) {
		ret = -ENOMEM;
		goto exit;
	}

	if (copy_from_user(ptr, wdata->data.pointer, wdata->data.length)) {
		ret = -EINVAL;
		goto exit;
	}

	ptr[wrqu->data.length] = 0;

	for (i = 0;i < wrqu->data.length;i++) {
		if (ptr[i] == ',')
			ptr[i] = ATBM_SPACE;
	}

	pos = atbm_skip_space(ptr, wrqu->data.length + 1);

	if (pos == NULL) {
		ret = -EINVAL;
		goto exit;
	}

	pos = pos + strlen("twt");

	pos = atbm_skip_space(pos, wrqu->data.length + 1 - (pos - ptr));

	if (pos == NULL) {
		ret = -EINVAL;
		goto exit;
	}

	len = wrqu->data.length + 1 - (pos - ptr);

	if (len <= 0) {
		ret = -EINVAL;
		goto exit;
	}

	pos_end = memchr(pos, ATBM_TAIL, len);

	if (pos_end != NULL)
		len = pos_end - pos + 1;

	/*
	*parase enable
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos, len, enable, atbm_accsii_to_int, false, exit, ret);

	if(enable != 0 && enable != 1){
		ret = -EINVAL;
		goto exit;
	}
	
	atbm_printk_err("[%s]:twt[%x]\n",sdata->name,enable);

	ifmgd = &sdata->u.mgd;

	mutex_lock(&ifmgd->mtx);
	mutex_lock(&local->sta_mtx);

	do{
		struct sta_info * sta;
		
		if(ifmgd->associated == NULL){
			ret = -ENETDOWN;
			break;
		}

		sta = sta_info_get(sdata,ifmgd->associated->bssid);

		if(sta == NULL){
			ret = -ENETDOWN;
			break;
		}

		if(enable == 0){
		
			ieee80211_twt_sta_teardown(sta);
			break;
		}

		if(enable){
			struct ieee80211_twt_request_params twt_request;
			
			memset(&twt_request,0,sizeof(twt_request));

			twt_request.setup_cmd = ATBM_TWT_SETUP_CMD_REQUEST;
			twt_request.trigger   = 1;
			twt_request.implicit  = 1;
			twt_request.flow_type = 0;
			twt_request.flow_id   = 1;
			twt_request.exp       = 10;
			twt_request.mantissa  = 512;
			if(ieee80211_twt_sta_request_work(sta,&twt_request) == true){
				ret = 0;
			}else {
				ret = -EINVAL;
			}
			break;
		}
	}while(0);
	
	mutex_unlock(&local->sta_mtx);
	mutex_unlock(&ifmgd->mtx);
exit:
	if(ptr)
		atbm_kfree(ptr);
	return ret;
}
#ifdef CONFIG_ATBM_SUPPORT_CSA
#ifdef CONFIG_ATBM_SUPPORT_CHANSWITCH_TEST
static int atbm_ioctl_sta_chswitch(struct net_device* dev, struct iw_request_info* info, union iwreq_data* wrqu, char* extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	union iwreq_data* wdata = (union iwreq_data*)wrqu;
	char* ptr = NULL;
	char const* pos = NULL;
	char const* pos_end = NULL;
	int ret = 0;
	int i = 0;
	int len = 0;
	int mode;
	int channel;
	int count;
	struct atbm_ieee80211_channel_sw_ie sw_ie;
	struct atbm_ieee80211_channel_sw_packed_ie sw_packed_ie;
	struct ieee80211_if_managed* ifmgd = &sdata->u.mgd;
	
	if (!ieee80211_sdata_running(sdata)) {
		ret = -ENETDOWN;
		goto exit;
	}
	
	if(sdata->vif.type != NL80211_IFTYPE_STATION){
		ret = -ENETDOWN;
		goto exit;
	}

	ptr = (char*)atbm_kzalloc(wdata->data.length + 1, GFP_KERNEL);

	if (!ptr) {
		ret = -ENOMEM;
		goto exit;
	}

	if (copy_from_user(ptr, wdata->data.pointer, wdata->data.length)) {
		ret = -EINVAL;
		goto exit;
	}

	
	ptr[wrqu->data.length] = 0;

	for (i = 0;i < wrqu->data.length;i++) {
		if (ptr[i] == ',')
			ptr[i] = ATBM_SPACE;
	}

	pos = atbm_skip_space(ptr, wrqu->data.length + 1);

	if (pos == NULL) {
		ret = -EINVAL;
		goto exit;
	}

	pos = pos + strlen("chswitch");

	pos = atbm_skip_space(pos, wrqu->data.length + 1 - (pos - ptr));

	if (pos == NULL) {
		ret = -EINVAL;
		goto exit;
	}

	len = wrqu->data.length + 1 - (pos - ptr);

	if (len <= 0) {
		ret = -EINVAL;
		goto exit;
	}

	pos_end = memchr(pos, ATBM_TAIL, len);

	if (pos_end != NULL)
		len = pos_end - pos + 1;

	/*
	*parase mode
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos, len, mode, atbm_accsii_to_int, false, exit, ret);

	if(mode != 0 && mode != 1){
		ret = -EINVAL;
		goto exit;
	}

	/*
	*parase channel
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos, len, channel, atbm_accsii_to_int, false, exit, ret);

	if((channel > 14) || (channel <= 0)){
		ret = -EINVAL;
		goto exit;
	}

	/*
	*parase count
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos, len, count, atbm_accsii_to_int, false, exit, ret);

	if(count > 255){
		ret = -EINVAL;
		goto exit;
	}

	mutex_lock(&ifmgd->mtx);

	if(ifmgd->associated){
		
		sw_ie.mode = mode;
		sw_ie.new_ch_num = channel;
		sw_ie.count = count;

		sw_packed_ie.chan_sw_ie       = &sw_ie;
		sw_packed_ie.ex_chan_sw_ie    = NULL;
		sw_packed_ie.sec_chan_offs_ie = NULL;

		ieee80211_sta_process_chanswitch(sdata,&sw_packed_ie,(void *)ifmgd->associated->priv,jiffies);
	}
	mutex_unlock(&ifmgd->mtx);
exit:
	if(ptr)
		atbm_kfree(ptr);
	return ret;
}

static int atbm_ioctl_sta_chswitch_beacon(struct net_device* dev, struct iw_request_info* info, union iwreq_data* wrqu, char* extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	union iwreq_data* wdata = (union iwreq_data*)wrqu;
	char* ptr = NULL;
	char const* pos = NULL;
	char const* pos_end = NULL;
	int ret = 0;
	int i = 0;
	int len = 0;
	int mode;
	int channel;
	int count;
	struct ieee80211_if_managed* ifmgd = &sdata->u.mgd;
	
	if (!ieee80211_sdata_running(sdata)) {
		ret = -ENETDOWN;
		goto exit;
	}
	
	if(sdata->vif.type != NL80211_IFTYPE_STATION){
		ret = -ENETDOWN;
		goto exit;
	}

	ptr = (char*)atbm_kzalloc(wdata->data.length + 1, GFP_KERNEL);

	if (!ptr) {
		ret = -ENOMEM;
		goto exit;
	}

	if (copy_from_user(ptr, wdata->data.pointer, wdata->data.length)) {
		ret = -EINVAL;
		goto exit;
	}

	
	ptr[wrqu->data.length] = 0;

	for (i = 0;i < wrqu->data.length;i++) {
		if (ptr[i] == ',')
			ptr[i] = ATBM_SPACE;
	}

	pos = atbm_skip_space(ptr, wrqu->data.length + 1);

	if (pos == NULL) {
		ret = -EINVAL;
		goto exit;
	}

	pos = pos + strlen("swbeacon");

	pos = atbm_skip_space(pos, wrqu->data.length + 1 - (pos - ptr));

	if (pos == NULL) {
		ret = -EINVAL;
		goto exit;
	}

	len = wrqu->data.length + 1 - (pos - ptr);

	if (len <= 0) {
		ret = -EINVAL;
		goto exit;
	}

	pos_end = memchr(pos, ATBM_TAIL, len);

	if (pos_end != NULL)
		len = pos_end - pos + 1;

	/*
	*parase mode
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos, len, mode, atbm_accsii_to_int, false, exit, ret);

	if(mode != 0 && mode != 1){
		ret = -EINVAL;
		goto exit;
	}

	/*
	*parase channel
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos, len, channel, atbm_accsii_to_int, false, exit, ret);

	if((channel > 14) || (channel <= 0)){
		ret = -EINVAL;
		goto exit;
	}

	/*
	*parase count
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos, len, count, atbm_accsii_to_int, false, exit, ret);

	if(count > 255){
		ret = -EINVAL;
		goto exit;
	}

	mutex_lock(&ifmgd->mtx);

	if(ifmgd->associated){
		
		ifmgd->sw_ie.mode = mode;
		ifmgd->sw_ie.new_ch_num = channel;
		ifmgd->sw_ie.count = count;
	}
	mutex_unlock(&ifmgd->mtx);
exit:
	if(ptr)
		atbm_kfree(ptr);
	return ret;
}
#endif
#endif

static int atbm_ioctl_read_reg_bit(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	//struct ieee80211_internal_ap_conf conf_req;
	//struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	char *extra = NULL;
	const char* pos;
	const char* pos_end;
	int ret = 0;
	int len = 0;
	u32 regAddr = 0;
	int endbit = 0;
	int startbit = 0;
	u32 regValue = 0;
	int i;

	extra = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL);

	if(extra == NULL){
		ret =  -ENOMEM;
		goto exit;
	}

	if((ret = copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))){
		return -EINVAL;
		goto exit;
	}
	
	extra[wrqu->data.length] = 0;
	
	for(i = 0;i<wrqu->data.length;i++){
		if(extra[i] == ',')
			extra[i] = ATBM_SPACE;
	}

	pos = atbm_skip_space(extra,wrqu->data.length+1);

	if(pos == NULL){
		ret = -EINVAL;
		goto exit;
	}

	pos = pos+strlen("rmem_bit");
	pos = atbm_skip_space(pos,wrqu->data.length+1-(pos-extra));

	if(pos == NULL){
		ret = -EINVAL;
		goto exit;
	}

	len = wrqu->data.length + 1 - (pos - extra);

	if(len <= 0){
		ret = -EINVAL;
		goto exit;
	}

	pos_end = memchr(pos,ATBM_TAIL,len);
	
	if(pos_end != NULL)
		len = pos_end - pos+1;

	// read reg addr
	ATBM_WEXT_PROCESS_PARAMS(pos,len,regAddr,atbm_accsii_to_hex_V2,false,exit,ret);

	// read end bit
	ATBM_WEXT_PROCESS_PARAMS(pos,len,endbit,atbm_accsii_to_int,false,exit,ret);

	// read start bit
	ATBM_WEXT_PROCESS_PARAMS(pos,len,startbit,atbm_accsii_to_int,false,exit,ret);

	if((endbit > 31) || (startbit < 0) || (startbit > endbit)){
		atbm_printk_err("Invalid endbit or startbit [%d:%d]\n", endbit, startbit);
		ret = -EINVAL;
		goto exit;
	}
	
	regValue = HW_READ_REG_BIT(regAddr, endbit, startbit);
	
	atbm_printk_always("%08X[%d:%d] %08X\n", regAddr, endbit, startbit, regValue);
exit:
	if(extra)
		atbm_kfree(extra);

	return ret;
}

static int atbm_ioctl_write_reg_bit(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	//struct ieee80211_internal_ap_conf conf_req;
	//struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	char *extra = NULL;
	const char* pos;
	const char* pos_end;
	int ret = 0;
	int len = 0;
	u32 regAddr = 0;
	int endbit = 0;
	int startbit = 0;
	u32 regValue = 0;
	int i;

	extra = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL);

	if(extra == NULL){
		ret =  -ENOMEM;
		goto exit;
	}

	if((ret = copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))){
		return -EINVAL;
		goto exit;
	}
	
	extra[wrqu->data.length] = 0;
	
	for(i = 0;i<wrqu->data.length;i++){
		if(extra[i] == ',')
			extra[i] = ATBM_SPACE;
	}

	pos = atbm_skip_space(extra,wrqu->data.length+1);

	if(pos == NULL){
		ret = -EINVAL;
		goto exit;
	}

	pos = pos+strlen("rmem_bit");
	pos = atbm_skip_space(pos,wrqu->data.length+1-(pos-extra));

	if(pos == NULL){
		ret = -EINVAL;
		goto exit;
	}

	len = wrqu->data.length + 1 - (pos - extra);

	if(len <= 0){
		ret = -EINVAL;
		goto exit;
	}

	pos_end = memchr(pos,ATBM_TAIL,len);
	
	if(pos_end != NULL)
		len = pos_end - pos+1;

	// read reg addr
	ATBM_WEXT_PROCESS_PARAMS(pos,len,regAddr,atbm_accsii_to_hex_V2,false,exit,ret);

	// read end bit
	ATBM_WEXT_PROCESS_PARAMS(pos,len,endbit,atbm_accsii_to_int,false,exit,ret);

	// read start bit
	ATBM_WEXT_PROCESS_PARAMS(pos,len,startbit,atbm_accsii_to_int,false,exit,ret);

	// read reg value
	ATBM_WEXT_PROCESS_PARAMS(pos,len,regValue,atbm_accsii_to_hex_V2,false,exit,ret);

	if((endbit > 31) || (startbit < 0) || (startbit > endbit)){
		atbm_printk_err("Invalid endbit or startbit [%d:%d]\n", endbit, startbit);
		ret = -EINVAL;
		goto exit;
	}
	
	HW_WRITE_REG_BIT(regAddr, endbit, startbit, regValue);
	
	atbm_printk_always("%08X[%d:%d] <= %08X\n", regAddr, endbit, startbit, regValue);
exit:
	if(extra)
		atbm_kfree(extra);

	return ret;
}


iwripv_common_cmd_t common_cmd[]={
	{"help",4,atbm_ioctl_common_help,"get help information"},
	{"get_rssi",8,atbm_ioctl_get_rssi,"iwpriv wlan0 common get_rssi [mac]"},
	{"stations_show",13,atbm_ioctl_associate_sta_status,"get sta mode support rate"},

#ifdef CONFIG_ATBM_IWPRIV_USELESS
	{"channel_test_start",18,atbm_ioctl_channel_test_start,"channel_test_start"},
#endif
	{"get_channel_idle",16,atbm_ioctl_get_channel_idle,"get channel idle value,lager be better"},
	{"set_efuse_dcxo",14,atbm_ioctl_set_efuse,"effect freq offsert"},
	{"setEfuse_dcxo",13,atbm_ioctl_set_efuse,"effect freq offsert"},
	{"set_efuse_deltagain",19,atbm_ioctl_set_efuse,"effect tx power"},
	{"setEfuse_deltagain",18,atbm_ioctl_set_efuse,"effect tx power"},
	{"set_efuse_5gdeltagain",21,atbm_ioctl_set_efuse,"effect tx power"},
	{"setEfuse_5gdeltagain",20,atbm_ioctl_set_efuse,"effect tx power"},
#ifdef CONFIG_ATBM_IWPRIV_USELESS
	{"set_efuse_tjroom",16,atbm_ioctl_set_efuse,"not allow change"},
	{"setEfuse_tjroom",18,atbm_ioctl_set_efuse,"not allow change"},
#endif
	{"get_tjroom",10,atbm_ioctl_get_Tjroom,"get efuse tjroom"},
	{"set_efuse_mac",13,atbm_ioctl_set_efuse,"change mac addr,The reload driver takes effect "},
	{"setEfuse_mac",12,atbm_ioctl_set_efuse,"change mac addr,The reload driver takes effect "},
	#ifdef CONFIG_ATBM_SUPPORT_BLUEZ
	{"setble_pub_mac",14,atbm_ioctl_set_ble_pub_mac,"change ble_mac addr "},
	#endif
	{"get_first_efuse",15,atbm_ioctl_get_efuse_first,"get first set efuse,Query whether changes have been made"},
	{"getefusefirst",13,atbm_ioctl_get_efuse_first,"get first set efuse,Query whether changes have been made"},
	{"get_efuse_free",14,atbm_ioctl_get_efuse_free_space,"Gets the size of Efuse remaining space"},
	{"EfusefreeSpace",14,atbm_ioctl_get_efuse_free_space,"Gets the size of Efuse remaining space"},
	{"getallEfuse",11,atbm_ioctl_get_efuse_all_data,"Gets all efuse data"},
	{"getEfuse",8,atbm_ioctl_get_efuse,"get efsue value"},
	{"get_efuse",9,atbm_ioctl_get_efuse,"get efsue value"},
#ifdef CONFIG_ATBM_IWPRIV_USELESS

	{"freqoffset",10,atbm_ioctl_freqoffset,"i don't know function"},
#endif
	{"singletone",10,atbm_ioctl_send_singleTone,"Transmit single carrier"},
	{"duty_ratio",10,atbm_ioctl_set_duty_ratio,"Modify the offer duty ratio in the ETF mode,only change 11n"},
#ifdef CONFIG_ATBM_SUPPORT_AP_CONFIG
	{"ap_conf",7,atbm_ioctl_set_ap_conf,"Fixed a channel scanning"},
#endif
#ifdef CONFIG_ATBM_MONITOR_SPECIAL_MAC
	{"monitor_mac",11,atbm_ioctl_rx_monitor_mac,"Add a MAC address to the hardware to be filtered"},
	{"stations_show",14,atbm_ioctl_rx_monitor_mac_status,"Is there a packet with the newly set MAC address for use with [monitor_mac]"},
#endif
#ifdef CONFIG_IEEE80211_SPECIAL_FILTER
	{"filter_frame",12,atbm_ioctl_rx_filter_frame,"Whitelist frame filtering,such as:beacon/probe"},
	{"filter_ie",9,atbm_ioctl_rx_filter_ie,"Whitelist frames filter certain IE fields"},
	{"filter_clear",12,atbm_ioctl_rx_filter_clear,"clear configuration with Whitelist frame filtering"},
	{"filter_show",11,atbm_ioctl_rx_filter_show,"show all configuration with Whitelist frame filtering"},
#endif
#ifdef HIDDEN_SSID
	{"ap_hide_ssid",12,atbm_ioctl_set_hidden_ssid,"set ap mode hidden ssid"},
#endif
	{"get_last_mac",12,atbm_ioctl_get_Last_mac,"get last set to efuse mac value"},
	{"get_first_mac",13,atbm_ioctl_get_First_mac,"get first set to efuse mac value"},
	
	{"set_cali_flag",13,atbm_ioctl_set_calibrate_flag,"The configuration items needed for the whole machine production and test"},
	{"set_deltagain",13,atbm_ioctl_set_calibrate_deltagain,"set deltagain after exe cmd set_cali_flag"},
	{"set_tpc",7,atbm_ioctl_set_calibrate_TPC,"set tpc after exe cmd set_cali_flag"},
	{"set_all_efuse",13,atbm_ioctl_set_all_efuse,"set all efuse,including:dcxo ,efuse,macaddr"},
	{"get_dcxo",8,atbm_ioctl_get_dcxo,"set dcxoafter exe cmd set_cali_flag"},
	{"set_dcxo",8,atbm_ioctl_set_dcxo,"set dcxo after exe cmd set_cali_flag"},
#ifdef ATBM_PRIVATE_IE
	{"ipc_reset",9,atbm_ioctl_ie_ipc_clear_insert_data,"clear ipc private ie"},
	{"vendor_ie",9,atbm_ioctl_ie_insert_vendor_ie,"insert vendor IE to (Probe Req)"},
#endif	

	
	{"getPrivateEfuse",15,atbm_ioctl_get_Private_256BITSEFUSE,"get private efuse space value"},
	{"setPrivateEfuse",15,atbm_ioctl_set_Private_256BITSEFUSE,"set private efuse space value"},



	{"power_save",10,atbm_set_power_save_mode,"0:normal mode 1:power save mode"},
	{"gpio_conf",9,atbm_ioctl_gpio_config,"Configure the GPIO state"},
	{"gpio_out",8,atbm_ioctl_gpio_output,"Set the GPIO output level"},
	{"edca_params",11,atbm_ioctl_edca_params,"set edca parameters"},
	{"get_edca_params",15,atbm_ioctl_get_edca_params,"get edca parameters"},
	{"txpower_status",14,atbm_ioctl_get_txposer_status,"get current txpower"},
#ifdef CONFIG_TXPOWER_DCXO_VALUE
	{"settxpower_byfile",17,atbm_ioctl_set_txpwr_by_file,"read "CONFIG_TXPOWER_DCXO_VALUE" to set efuse gain or dcxo"},
#else
	{"settxpower_byfile",17,atbm_ioctl_set_txpwr_by_file,"read /tmp/atbm_txpwer_dcxo_cfg.txt to set efuse gain or dcxo"},
#endif
#ifdef CONFIG_RATE_TXPOWER
	{"set_rate_power",14,atbm_ioctl_set_rate_txpwr_by_file,"read "CONFIG_RATE_TXPOWER" to set Power at different rates"},
#else
	{"set_rate_power",14,atbm_ioctl_set_rate_txpwr_by_file,"read /tmp/set_rate_power.txt to set Power at different rates"},
#endif
	{"get_txpower",11,atbm_ioctl_get_txpower,"get power by function cmd"},
	{"set_txpower",11,atbm_ioctl_set_txpower,"set power by function cmd"},
	{"set_ladder_txpower",18,atbm_ioctl_set_ladder_txpower,"set ladder txpower,0/3/15/63"},
	{"set_rate_txpower_mode",21,atbm_ioctl_set_rate_txpower_mode,"set_rate_txpower_mode"},
	{"get_power_target",16,atbm_ioctl_get_powerTarget,"get power target in current rate"},
	{"set_power_target",16,atbm_ioctl_set_powerTarget,"set power target in current rate"},
	{"get_cfg_txpower",15,atbm_ioctl_get_cfg_txpower_by_file,"read configured tx power by /tmp/atbm_txpwer_dcxo_cfg.txt and /tmp/set_rate_power.txt"},
	{"set_frame_rate",14,atbm_ioctl_set_management_frame_rate,"set manager frame rate 1Mbps"},
	{"get_maxrate",11,atbm_ioctl_get_cur_max_rate,"Gets the current primary shipping rate"},
	{"get_rx_rate",11,atbm_ioctl_get_rx_rate,"Gets the current primary shipping rate"},
	{"rts_threshold",13,atbm_ioctl_set_rts_threshold,"rts threshold"},
	{"atbm_get_work_channel",21,atbm_ioctl_atbm_get_work_channel,"get current work channel"},
//#ifdef CONFIG_CPTCFG_CFG80211_COUNTRY_CODE
	{"country_code",12,atbm_ioctl_set_country_code,"country_code US/CN/JP ===> channel 11/13/14"},
	{"get_country_code",16,atbm_ioctl_get_country_code,"get country_code US/CN/JP ===> channel 11/13/14"},
//#endif
#ifdef CONFIG_ATBM_GET_GPIO4
	{"get_gpio4",9,atbm_ioctl_get_gpio4,"get gpio4 value"},
#endif	
	{"get_cali_data",13,atbm_ioctl_get_cfo_cali_data,"get dcxo & cfo calibration value"},
	{"send_deauth",11,atbm_ioctl_send_deauth,"send brodcast deauth frame"},
	{"user_vendor_ie",14,atbm_ioctl_send_vendor_ie,"send special ie===>user_vendor_ie,ssid,psk"},
	{"get_user_vendor_ie",18,atbm_ioctl_get_vendor_ie,"get 6441 special data"},
	{"listen_probe_req",16,atbm_ioctl_set_listen_probe_req,"sta mode set listen probe req===>listen_probe_req,channel [channel,0~14]"},
#ifdef SDIO_BUS
//	{"hibernate_en",12,atbm_ioctl_device_hibernate,"set device hibernate or wake"},
#endif
#ifdef CONFIG_ATBM_AP_CHANNEL_CHANGE_EVENT
	
	{"change_chan",11,atbm_ioctl_change_ap_channel,"change_chan,chan,ht40,change ap channel,Must be provided by ATBM HOSTAPD��other version hostapd not support!"},
#endif
	{"get_cca_threshold",17,atbm_ioctl_get_cca_threshold,"get cca threshold"},
	{"get_tx_parameter",16,atbm_ioctl_get_tx_parameter,"get tx parameter such as tx lifetime setting"},
	{"get_rts_threshold",17,atbm_ioctl_get_rts_threshold,"get rts send packet lenth threshold"},
	{"get_rts_duration",16,atbm_ioctl_get_rts_duration,"get rts duration value"},
	{"get_noise_level",15,atbm_ioctl_get_noise_level,"To obtain the bottom noise, inig=60 is accurate"},
	{"get_snr",7,atbm_ioctl_get_snr,"The channel ratio is obtained, and the obtained value needs to be calculated by the formulaListed as follows:10*log10(4096/snr_val)"},
	{"fw_reset",8,atbm_ioctl_fw_reset,"fw reset,reload firmware,1:wifi6_bt_comb , 0: only support wifi6"},

#ifdef CONFIG_JUAN_MISC	
	{"get_tim",7,atbm_ioctl_common_get_tim,"get beacon tim "},
	{"set_tim",7,atbm_ioctl_common_set_tim,"set beacon tim,eg:set_tim,ena,hex_val,"},
#endif

#ifdef ATBM_SUPPORT_LSLP_PRIV_COMMUNICATION
	{"set_lightsleep_info",19,atbm_ioctl_set_lightsleep_info,"set lightsleep info,eg:set_lightsleep_info,ApMacAddress,WakeupGpio,WakeupVaule,KeepRxTimeMs,WakeupEnable"},
	{"set_lightsleep_start",20,atbm_ioctl_set_lightsleep_start,"set lightsleep start,eg:set_lightsleep_start"},
#endif

#ifdef SDIO_BUS
	{"hibernate_en",12,atbm_ioctl_device_hibernate,"set device hibernate or wake"},
#endif
    {"tx_rate",7,atbm_ioctl_set_tx_rate,"tx_rate"},
    
#ifdef CONFIG_ATBM_BLE_ADV_COEXIST
	{"ble_coexist",11,atbm_ioctl_ble_coexist_cmd,"ble coexist mode start/stop(0/1)"},
	{"ble_set_adv",11,atbm_ioctl_ble_set_adv_data_cmd,"ble set adv mac and data"},
#endif
#ifdef CONFIG_ATBM_SUPPORT_REKEY
	{"set_rekey",9,atbm_ioctl_set_rekey,"set_rekey data enable"},
#endif
#ifdef WLAN_CAPABILITY_RADIO_MEASURE
	{"wfa_certificate",15,atbm_ioctl_wfa_certificate,"wifi he wfa certificate test"}, 
#endif
#ifdef CONFIG_ATBM_SUPPORT_GRO
	{"napi_stats",10,atbm_ioctl_napi_stats,"napi stats"},
#endif
	{"om",2,atbm_ioctl_set_tx_om,"om"},
	{"set_packet_interval",19,atbm_ioctl_set_packet_interval,"set packet interval unit(us)"},
	{"mac_reset",9,atbm_ioctl_mac_reset,"mac_reset"},
	{"get_conn_state",14,atbm_ioctl_get_wifi_state,"sta mode get  connect state"},
	{"get_txrx_info",13,atbm_ioctl_get_txrx_info,"get txrx_info :rx_packet, tx packet,tx_suc_packet,rx_avg_rate "},
	{"uapsd",5,atbm_ioctl_uapsd,"uapsd"},
	{"twt",3,atbm_ioctl_twt,"twt"},
#ifdef CONFIG_ATBM_SUPPORT_CSA
#ifdef CONFIG_ATBM_SUPPORT_CHANSWITCH_TEST
	{"chswitch",8,atbm_ioctl_sta_chswitch,"chswitch"},
	{"swbeacon",8,atbm_ioctl_sta_chswitch_beacon,"swbeacon"},
#endif
#endif
	{"rmem_bit",8,atbm_ioctl_read_reg_bit,"rmem_bit : read register value by bit width "},
	{"wmem_bit",8,atbm_ioctl_write_reg_bit,"wmem_bit : write register value by bit width "},
#ifdef CONFIG_IEEE80211_SEND_SPECIAL_MGMT
	{"send_action",11,atbm_ioctl_send_action,"send private action , param1:action vendor ie"},
	{"stop_send_action",16,atbm_ioctl_stop_send_action,"stop_send_action"},
	{"send_probe_resp",15,atbm_ioctl_send_probe_resp,"until send probe resp,===>>send_probe_resp,ssid,psk"},
	{"stop_probe_resp",15,atbm_ioctl_stop_send_probe_resp,"stop send probe resp"},
#endif
#ifdef ATBM_USE_FASTLINK
	{"reset_fastlink",14,atbm_ioctl_reset_fastlink,"reset fastlink"},
#endif
	{"set_ampdu",9,atbm_ioctl_set_ampdu,"set_ampdu"},
	{"set_single_scan_time",20,atbm_ioctl_set_single_scan_time,"set single scan time"},
	{"get_pa_state",12,atbm_ioctl_get_pa_status,"get pa status"},
	{"best_ch_scan2",13,atbm_ioctl_best_ch_scan2,"start_chan,end_chan,special,ssid"},
	{"best_ch_scan3",13,atbm_ioctl_best_ch_scan3,"start_chan,end_chan,special"},
	{"get_tx_status",13,atbm_ioctl_get_tx_packet_status,"get tx packet & tx fail packet"},
	{"beacon_loss_cnt",15,atbm_ioctl_update_beacon_loss_conter,"sta mode set beacon loss conter"},
#ifdef CONFIG_TEMP_DCXO_TABLE
	{"set_temp_dcxo_table",19,atbm_ioctl_common_set_temp_dcxo_table,"read "CONFIG_TEMP_DCXO_TABLE" to set efuse gain or dcxo"},
#else
	{"set_temp_dcxo_table",19,atbm_ioctl_common_set_temp_dcxo_table,"read /tmp/temp_dcxo_table.txt to set efuse gain or dcxo"},
#endif
	{"efuse_gain_trim",15,atbm_ioctl_set_efuse_gain_trim,"efuse_gain_trim,low_gain,mid_gain,hight_gain,write_rom" },
	{"bus_awake",9,atbm_set_bus_awake,"0:wake,1:sleep" },

	{"ctrl_deltagain_tpc",18,atbm_ioctl_ctrl_use_deltagain_and_tpc,"0:not use deltagain and tpc;1:use deltagain and use tpc;2:not use deltagain and use tpc;3:use deltagain and not use tpc" },
	{"power_threshold",15,atbm_ioctl_set_power_threshold,"0:normal,1:set tx power threshold" },
	{"low_energy",10,atbm_ioctl_set_reduce_tx_rx_power_consumption,"0:normal,1:reduce tx rx power consumption" },
	//{"gpio_getval",11,atbm_ioctl_gpio_output_getval,"Get the GPIO value in output mode"},
	{"2ant_extraio_ctl",16,atbm_ioctl_2AntExtraIoSet,"Control the 2-ant extra IO value in output mode"},

	{ NULL,0,NULL,NULL }
};



static int atbm_ioctl_common_help(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	iwripv_common_cmd_t *cmd = common_cmd;
	atbm_printk_err("Usage: iwpriv [ifname] common [option] ?-?-\n");
	atbm_printk_err("	option			information");
	while(cmd->cmd){
		atbm_printk_err("	%-20s		%-s\n", cmd->cmd, cmd->uage);
		cmd++;
	}
	return 0;
}

#ifdef ATBM_SUPPORT_SMARTCONFIG
static int atbm_ioctl_smartconfig_start(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	u32 value;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	
	value = *extra - 0x30;
	
	atbm_smartconfig_start(hw_priv,value);

	return 0;
}


static int atbm_ioctl_smartconfig_stop(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	//u32 value;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;

	//value = *extra - 0x30;
	atbm_smartconfig_stop(hw_priv);
	return 0;
}

static int atbm_ioctl_smartconfig_start_v2(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	//u32 value;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *vif;
	int i = 0;	
	u32 enable = 1;
	
	//value = *extra - 0x30;
	atbm_for_each_vif(hw_priv,vif,i){
				if (vif != NULL)
				{
					WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_SMARTCONFIG_START,
						&enable, sizeof(enable), vif->if_id));
					break;
				}
			}
	atbm_smartconfig_start(hw_priv,enable);
	return 0;
}

static int atbm_ioctl_smartconfig_stop_v2(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	//u32 value;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *vif;
	int i = 0;	
	u32 enable = 0;

	//value = *extra - 0x30;
	atbm_for_each_vif(hw_priv,vif,i){
				if (vif != NULL)
				{
					WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_SMARTCONFIG_START,
						&enable, sizeof(enable), vif->if_id));
					break;
				}
			}
	atbm_smartconfig_start(hw_priv,enable);
	return 0;
}
#endif

static int atbm_ioctl_fwdbg(struct net_device *dev, struct iw_request_info *ifno, union iwreq_data *wrqu, char *ext)
{
	int i = 0;	
	int ret = 0;
	u8 ucDbgPrintOpenFlag = 0;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *vif;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	u8 ptr[2];
	
	if(copy_from_user(ptr, wdata->data.pointer, 1)){
		ret =  -EINVAL;
		goto exit;
	}
	
	ptr[1] = 0;
	
	ucDbgPrintOpenFlag = ptr[0]  - 0x30;
	atbm_printk_err("ALTM_GET_DBG_PRINT_TO_HOST\n");
	atbm_printk_err("%s dbgflag:%d\n", __func__, ucDbgPrintOpenFlag);
	atbm_for_each_vif(hw_priv,vif,i){
		if (vif != NULL)
		{
			WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_DBG_PRINT_TO_HOST,
				&ucDbgPrintOpenFlag, sizeof(ucDbgPrintOpenFlag), vif->if_id));
			break;
		}
	}

exit:

	return ret;
}
extern void atbm_adaptive_val_set(int status);
extern void atbm_wifi_cfo_set(int status);

int fwcmd_common_deal(struct ieee80211_sub_if_data *sdata,char *extra,int len)
{
	if((len > 12) && (!memcmp(extra,"set_adaptive",12))){
		if(extra[13] == '1'){
			sdata->local->adaptive_started = true;
			sdata->local->adaptive_started_time = jiffies;
			atbm_printk_err("adaptive_started!\n");
			atbm_adaptive_val_set(1);
		}else if(extra[13] == '0')	{
			sdata->local->adaptive_started = false;
			atbm_printk_err("adaptive_stoped!\n");
			atbm_adaptive_val_set(0);
		}	
	}else if((len > 3) && (!memcmp(extra,"cfo",3))){
		if(atbm_get_cfo_allow_flag() == 0){
			atbm_printk_err("not support auto cfo! \n");
			return -1;
		}
	
		if(extra[4] == '0'){
			atbm_wifi_cfo_set(0);
		}else{
			//extra[4] = '1';
			atbm_wifi_cfo_set(1);
		}
	}
	
	return 0;
}

static int atbm_ioctl_fwcmd(struct net_device *dev, struct iw_request_info *ifno, union iwreq_data *wrqu, char *ext)
{
	
	int i = 0;
	int ret = 0;
	char *extra = NULL;
	char substr[20] = {0};
	char *str;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	unsigned int channel_num = 0;
	unsigned int freq = 0;
	struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *vif;
	if(!(extra = atbm_kmalloc(wrqu->data.length+1,GFP_KERNEL)))
		return -ENOMEM;
	if(copy_from_user(extra, wrqu->data.pointer, wrqu->data.length)){
		atbm_kfree(extra);
		return -EINVAL;
	}

	if(wrqu->data.length <= 1){
		atbm_printk_err("invalid parameter!\n");
		atbm_printk_err("e.g:./iwpriv wlan0 fwcmd intr\n");
		atbm_kfree(extra);
		return -EINVAL;
	}

	for(i=0;i<wrqu->data.length;i++){
		if(extra[i] == ',')
			extra[i] =' ';
	}

	if(extra[0] == ' '){
		atbm_printk_err("invalid parameter!\n");
		atbm_kfree(extra);
		return -EINVAL;
	}
	//printk("exttra = %s length: %d channel_num: %d freq: %d\n", extra, wrqu->data.length, channel_num, freq);
	str = strstr(extra, "set_freq");
	if(NULL != str)  //set_freq
	{	
	    memcpy(substr, str, min(strlen(str), 16));		
        str = substr;
		atbm_CmdLine_GetToken(&str);
		atbm_CmdLine_GetInteger(&str, &channel_num);		
		atbm_CmdLine_GetInteger(&str, &freq);
		wifi_special_freq = freq;		
	   // printk("dbg exttra = %s length: %d channel_num: %d freq: %d\n", extra, wrqu->data.length, channel_num, freq);
	}
	
	if(fwcmd_common_deal(sdata,extra,wrqu->data.length) != 0){
		atbm_kfree(extra);
		return 0;
	}
	i = 0;
	atbm_for_each_vif(hw_priv,vif,i){
		if (vif != NULL)
		{
			atbm_printk_wext("exttra = %s\n", extra);
			WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_FW_CMD,
				extra, wrqu->data.length, vif->if_id));
			break;
		}
	}
	atbm_kfree(extra);
	return ret;
	
}


/*
iwpriv [interface] start_tx chan,mode,rate,bw,chOff,ldpc,len,[precom]
interface：wlan0 or p2p0 
channel: [1,14]
mode: 0:11b;1:11g:2:11n:3:11ac;4:11ax HE-SU;5:HE-ER_SU
rate:
	11b: [0,1,2,3] --> [1M, 2M, 5.5M, 11M]
	11g: [0,1,2,3,4,5,6,7] --> [6M, 9M, 12M, 18M, 24M, 36M, 48M, 54M]
	11n: [0,1,2,3,4,5,6,7] --> [mcs0, ..... mcs7]
	11ac: [0,9] --> [mcs0, mcs9]
	11ax: [0,11] --> [mcs0, mcs11]
	tips:HE-ER_SU only supported mcs0~mcs2
bw: 0:20M;1:40M;
chOff: 0:zero;
ldpc:0:BBC;1:LDPC
	tips:HE 20M mcs10/mcs11,40M mcs0-mcs11 ldpc must be 1
len      : packet len
precom   : 20M chan1 precompensation,[0, 1, 2]
		0: NTPCOMPRegsBW20M
		1: NTPCOMPRegsBW20M_arc8M
		2: NTPCOMPRegsBW20M_arc9M
		3: SRRC

*/

static int atbm_ioctl_start_tx(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	//struct atbm_vif *vif;
	struct ieee80211_sub_if_data *sdata_tmp = NULL;
	struct atbm_vif *priv;// = ABwifi_get_vif_from_ieee80211(&sdata->vif);
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	char *ptr = NULL;
	char const *pos = NULL;
	char const *pos_end = NULL;
	int ret = 0;
	int i   = 0;

	int channel = 0;
	int rateIdx = 0;
	//int rateMin = 0;
	//int rateMax = 0;
	int mode = 0;//0
	int bw = 0;
	int chOff = 0;
	int ldpc = 0;
	int packetLen = 1024;
	int precom = 0;
	int precom_temp = 0;

	int len = 0;
	char threshold_param[100] = {0};
	char *delim = "cfg:";
	char *pthreshold = NULL;
	//int etf_v2 = 0;
//	u8 ucDbgPrintOpenFlag = 1;
	
	
	//ETF_HE_TX_CONFIG_REQ  EtfConfig;
	start_tx_param_t tx_param;

	//atbm_printk_err("start tx join status [%d] \n",priv->join_status);
	
	list_for_each_entry(sdata_tmp, &local->interfaces, list){
	    if (sdata_tmp->vif.type == NL80211_IFTYPE_AP){
		 		atbm_printk_err("driver run ap mode! not allow start tx! \n");
	             return -EINVAL;
	    }
		priv = ABwifi_get_vif_from_ieee80211(&sdata_tmp->vif);
		if(priv->join_status == 4){
			atbm_printk_err("driver on  sta mode connect ap! not allow start tx! \n");
			return -EINVAL;
		}
    }
	
	
	
	ptr = (char *)atbm_kzalloc(wdata->data.length+1, GFP_KERNEL);
	
	if(!ptr){
		ret =  -ENOMEM;
		goto exit;
	}
	
	if(copy_from_user(ptr, wdata->data.pointer, wdata->data.length)){
		ret =  -EINVAL;
		goto exit;
	}
	
	ptr[wrqu->data.length] = 0;

	
	pthreshold = strstr(ptr, delim);
	if(pthreshold)
	{
		memcpy(threshold_param, pthreshold, strlen(pthreshold));
		memset(pthreshold, 0, strlen(pthreshold));
		atbm_printk_wext("**ptr:%s**\n", ptr);
		atbm_printk_wext("**threshold_param:%s**\n", threshold_param);
	}
	
	
	for(i = 0;i<wrqu->data.length;i++){
		if(ptr[i] == ',')
			ptr[i] = ATBM_SPACE;
	}
	
	pos = atbm_skip_space(ptr,wrqu->data.length+1);

	if(pos == NULL){
		ret = -EINVAL;
		goto exit;
	}

	pos = atbm_skip_space(pos,wrqu->data.length+1-(pos-ptr));

	if(pos == NULL){
		ret = -EINVAL;
		goto exit;
	}

	len = wrqu->data.length + 1 - (pos - ptr);

	if(len <= 0){
		ret = -EINVAL;
		goto exit;
	}

	pos_end = memchr(pos,ATBM_TAIL,len);
	
	if(pos_end != NULL)
		len = pos_end - pos+1;

	/*
	*parase channel
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos,len,channel,atbm_accsii_to_int,false,exit,ret);

	
	/*
	*parase mode
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos,len,mode,atbm_accsii_to_int,false,exit,ret);	
	
	

	/*
	*parase rateIdx
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos,len,rateIdx,atbm_accsii_to_int,false,exit,ret);	


	/*
	*parase bw,0:20M,1:40M;
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos,len,bw,atbm_accsii_to_int,false,exit,ret);	
	
	

	/*
	*parase chOff,0:ZERO
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos,len,chOff,atbm_accsii_to_int,false,exit,ret);	
	
	
	
	/*
	*parase LDPC,0:BBC;1:LDPC
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos,len,ldpc,atbm_accsii_to_int,false,exit,ret);	
	
	
	
	/*
	*parase packetLen
	*/
	//ATBM_WEXT_PROCESS_PARAMS(pos,len,packetLen,atbm_accsii_to_int,false,exit,ret);	

	if(len)
	{
		/*
		*parase precom
		*/
		ATBM_WEXT_PROCESS_PARAMS(pos,len,precom_temp,atbm_accsii_to_int,false,exit,ret);
		if(hw_priv->chip_id == HW_CHIP_VERSION_Cronus)
		{
			switch(precom_temp){
			case 1://FCC
			case 2:
				precom = precom_temp;
				if(bw != 0)
					precom = 0;	
				break;
			case 3://SRRC 10dBm 
			case 6://SRRC 3dBm
				precom = (precom_temp==3)?9:10;	
				break;
			case 4://FCC
			case 5://CE
				if(mode == 0)//DSSS
					precom = 8;
				else{//OFDM
					if(bw == 0){
						//if((bw == 0) && (channel != 1) && (channel != 11))
							//precom = 0;	
						if(channel==11){
							//bw = 1;
							precom = 3;
							//if(mode == 3)
								//ldpc = 1;
						}
						else
							precom = 6;
					}
					else{
						precom = (channel==9)?4:7;
						//if((bw == 1) && (channel != 3) && (channel != 9))
							//precom = 0;	
					}
				}
				break;
			default:
				precom = 0;
				break;
			}
		}
		else
		{
			precom = !!precom_temp;
		}
	}
	
	atbm_printk_err("%s:channel[%d],mode[%d],rateIdx[%d],bw[%d],chOff[%d],ldpc[%d],precom[%d]\n",sdata->name,
		channel,mode,rateIdx,bw,chOff,ldpc,precom);
	
	tx_param.channel = channel;
	tx_param.mode = mode;
	tx_param.rate_id = rateIdx;
	tx_param.bw = bw;
	tx_param.chOff = chOff;
	tx_param.pktlen = packetLen;
	tx_param.ldpc = ldpc;
	tx_param.precom = precom;
	
	if(pthreshold){
		memset(tx_param.threshold_param,0,sizeof(tx_param.threshold_param));
		memcpy(tx_param.threshold_param,threshold_param,sizeof(threshold_param));
	}
	ret = atbm_internal_start_tx(hw_priv,&tx_param);	
	if(ret < 0){
		goto exit;
	}


exit:
	if(ptr)
		atbm_kfree(ptr);

	return ret;
}


static int atbm_ioctl_stop_tx(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	//int i = 0;
	int ret = 0;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;


	ret = atbm_internal_stop_tx(hw_priv);

	return ret;
}

/*
	iwpriv wlan0 start_rx <channel>,<bw>,<chOff>
	channel: [1,14]
	bw: 0:20M;1:40M;
	chOff: 0:zero;1:10U;2:10L
	mode:0:ofdm;1:dsss
	tips:20M chOff = 0;40M chOff = 1 or chOff = 2;
*/
static int atbm_ioctl_start_rx(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	char *ptr = NULL;
	char const *pos = NULL;
	char const *pos_end = NULL;
	int i = 0;
	int ret = 0;
	int bw = 0;
	int chOff = 0;
	int channel = 0;
	int mode = 0;
	int len = 0;

	struct ieee80211_sub_if_data *sdata_tmp = NULL;
	struct atbm_vif *priv;

	start_rx_param_t rx_param;
	
	list_for_each_entry(sdata_tmp, &local->interfaces, list){
             if (sdata_tmp->vif.type == NL80211_IFTYPE_AP){
			 		atbm_printk_err("driver run ap mode! not allow start rx! \n");
                     return -EINVAL;
             }
			 priv = ABwifi_get_vif_from_ieee80211(&sdata_tmp->vif);
			if(priv->join_status == 4){
				atbm_printk_err("driver on  sta mode connect ap! not allow start tx! \n");
				return -EINVAL;
			}
    }

	

	if(wrqu->data.length <= 3){
		atbm_printk_err("need to input parameters\n");
		atbm_printk_err("e.g: ./iwpriv wlan0 start_rx 1,0\n");
		return -EINVAL;
	}

	ptr = (char *)atbm_kzalloc(wdata->data.length+1, GFP_KERNEL);
	
	if(!ptr){
		ret =  -ENOMEM;
		goto exit;
	}
	
	if(copy_from_user(ptr, wdata->data.pointer, wdata->data.length)){
		ret =  -EINVAL;
		goto exit;
	}
	
	ptr[wrqu->data.length] = 0;
	
	for(i = 0;i<wrqu->data.length;i++){
		if(ptr[i] == ',')
			ptr[i] = ATBM_SPACE;
	}
	
	pos = atbm_skip_space(ptr,wrqu->data.length+1);

	if(pos == NULL){
		ret = -EINVAL;
		goto exit;
	}

	pos = atbm_skip_space(pos,wrqu->data.length+1-(pos-ptr));

	if(pos == NULL){
		ret = -EINVAL;
		goto exit;
	}

	len = wrqu->data.length + 1 - (pos - ptr);

	if(len <= 0){
		ret = -EINVAL;
		goto exit;
	}

	pos_end = memchr(pos,ATBM_TAIL,len);
	
	if(pos_end != NULL)
		len = pos_end - pos+1;	

	/*
	*parase channel
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos,len,channel,atbm_accsii_to_int,false,exit,ret);
	
	
	/*
	*parase bw:0:20M;1:40M;2:RU242;3:RU106;
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos,len,bw,atbm_accsii_to_int,false,exit,ret);	
	

	/*
	*parase chOff:0:zero;1:10U;2:10L;
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos,len,chOff,atbm_accsii_to_int,false,exit,ret);	
	

	/*
	*parase mode:0:ofdm;1:dsss;2ofdm+dsss;
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos,len,mode,atbm_accsii_to_int,false,exit,ret);	


	atbm_printk_always("channel:[%d], bw:[%d], chOff[%d], mode[%d]\n", channel, bw, chOff, mode);

	rx_param.channel = channel;
	rx_param.bw = bw;
	rx_param.chOff = chOff;
	rx_param.mode = mode;
	g_RxMode = mode;
	ret = atbm_internal_start_rx(hw_priv,&rx_param);
	

exit:
	if(ptr)
		atbm_kfree(ptr);
	return ret;
}

/*
	iwpriv wlan0 start_rx chan,bw
	chan:[0,14]
	bw:0:20M;1:40M
*/
static int atbm_ioctl_stop_rx(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{

	int ret = 0;

	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	get_result_rx_data rx_data;
	memset(&rx_data,0,sizeof(get_result_rx_data));

	ret = atbm_internal_stop_rx(hw_priv,&rx_data);
	if((ret = copy_to_user(wrqu->data.pointer, &rx_data, sizeof(get_result_rx_data))) != 0){
		//atbm_printk_egrr("copy_to_user err! ");
		//return -EINVAL;
		atbm_printk_always("rxSuc:%d,Err:%d\n", rx_data.rxSuccess, rx_data.rxError);
	}
	return ret;
}

#ifdef CONFIG_ATBM_IWPRIV_USELESS
static int atbm_ioctl_get_rx_stats(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	int ret = 0;
	char *extra = NULL;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *vif;
	int i = 0;
	
	if(!(extra = atbm_kmalloc(16, GFP_KERNEL)))
		return -ENOMEM;

	if((ret = copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(extra);
		return -EINVAL;
	}

	atbm_for_each_vif(hw_priv,vif,i){
				if (vif != NULL)
				{
					WARN_ON(wsm_read_mib(hw_priv, WSM_MIB_ID_GET_ETF_RX_STATS,
						extra, 16, vif->if_id));
					break;
				}
			}

	if((ret = copy_to_user(wrqu->data.pointer, extra, 16)) != 0){
		atbm_kfree(extra);
		return -EINVAL;
	}
	atbm_kfree(extra);
	return ret;
}
#endif
static int atbm_ioctl_set_rts(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	int i  = 0, j= 0;
	int ret = 0;
	int value = 0;
	int value_packet_time = 0;
	char *extra = NULL;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *vif;
	if(wrqu->data.length <= 1){
		atbm_printk_err("need parameter,please try again!\n");
		return -EINVAL;
	}

	if(!(extra = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL)))
		return -EINVAL;
	if((ret = copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(extra);
		return -EINVAL;
	}
	if(!(memcmp(extra, "show", wrqu->data.length))){
		atbm_printk_err("get_rts_threshold = %d\n", atbm_tool_rts_threshold);
		atbm_kfree(extra);
		return ret;
	}
	else
	{
		for(i=0;((extra[i] != ',')&&(extra[i] != '\0'));i++){
			value = value * 10 + (extra[i] - 0x30);
		}
		if(extra[i] == ','){
			for(j = i+1;extra[j] != '\0';j++)
			{
				value_packet_time =  value_packet_time* 10 + (extra[j] - 0x30);
			}
		}
		else if(extra[i] == '\0')
		{
			value_packet_time = 1024 * 32;
			goto next;
		}
		if(value_packet_time < 0 || value_packet_time > 32735)
			value_packet_time = 1023 * 32;
next:
		if((value <0 ) || (value > 2000)){
			atbm_printk_err("invalid parameter!\n");
			value = 0;
		}
		value_packet_time = value_packet_time / 32;//iwpriv 是us�?lmac�?2us
		atbm_printk_wext("set_rtsthr is %d\n", value);
		atbm_for_each_vif(hw_priv,vif,i){
			if(vif != NULL)
			{
				__le32 value_ven[2];
				if (value != (u32) -1)
					value_ven[0] = __cpu_to_le32(value);
				else
					value_ven[0] = 0; /* disabled */
				value_ven[1] = __cpu_to_le32(value_packet_time);
				/* mutex_lock(&priv->conf_mutex); */
				ret = WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_DOT11_RTS_THRESHOLD,
					value_ven, sizeof(value_ven), vif->if_id));
				atbm_tool_rts_threshold = value_ven[0];
				break;
			}
		}
	}
	atbm_kfree(extra);
	return ret;
}

static int atbm_ioctl_ctl_gi(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	int ret = 0;
	int value = 0;
	char *extra = NULL;
	atbm_printk_wext("@@@@@@@ atbm_ioctl_ctl_gi()\n");

	if(!(extra = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		return -EINVAL;
	}
	if((ret = copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(extra);
		return -EINVAL;
	}

	if((wrqu->data.length <= 1) || (wrqu->data.length > 2)){
		atbm_kfree(extra);
		atbm_printk_err("invalid parameter,please try again!\n");
		return -EINVAL;
	}

	value = *extra - 0x30;
	if((value < 0 ) || (value > 1)){
		atbm_kfree(extra);
		atbm_printk_err("invalid parameter,parameter must be 1 or 0\n");
		return -EINVAL;
	}
	atbm_printk_wext("set_short_gi(%d)\n", value);
	atbm_set_shortGI(value);
	atbm_tool_shortGi = value;
	atbm_kfree(extra);
	return ret;
}

#if 0
static int atbm_ioctl_setmac(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	int index;
	u8 macAddr[6] = {0};
	u8 extraBuff[18] = {0};
	int ret = -EINVAL;
#define   isHex(value)  	(((value > '9') && (value < '0')) && \
							 ((value > 'f') && (value < 'a')) && \
							 ((value > 'F') && (value < 'A')))
	memcpy(extraBuff, extra, 17);
	extraBuff[17] = ':';
	for (index  = 0 ; index < 17; index+= 3)
	{
		if (isHex(extraBuff[index]))
		{
					
			atbm_printk_err("mac addr format error\n");
			return -EINVAL;
		}
		if (isHex(extraBuff[index + 1]))
		{
					
			atbm_printk_err( "mac addr format error\n");
			return -EINVAL;
		}
		
		if (extraBuff[index + 2] != ':')
		{
			atbm_printk_err("mac addr format error\n");
			return -EINVAL;
		}
		
	}

#undef isHex

	sscanf(extraBuff, "%x:%x:%x:%x:%x:%x", (int *)&macAddr[0], (int *)&macAddr[1], (int *)&macAddr[2], (int *)&macAddr[3], (int *)&macAddr[4], (int *)&macAddr[5]);

	if (local->ops != NULL)
	{
		ret = local->ops->set_mac_addr2efuse(&local->hw, macAddr);
	}

	return ret;
}
#endif
static int atbm_ioctl_get_Last_mac(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	//int index;
	u8 macAddr[6] = {0};
	//u8 extraBuff[18] = {0};
	int ret = -EINVAL;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
/*
	if(wrqu->data.length > 1){
		atbm_printk_err("command 'getmac' need not parameters\n");
		return ret;
	}
*/
	if ((ret = wsm_get_efuse_last_mac(hw_priv, &macAddr[0])) == 0){
		if(extra){
			sprintf(extra,"macAddr:%02x:%02x:%02x:%02x:%02x:%02x\n", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
			wdata->data.length = strlen(extra);
		}
		atbm_printk_err("macAddr:%02x:%02x:%02x:%02x:%02x:%02x\n", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
		//return ret;
	}
	else{
		if(extra){
			sprintf(extra,"read mac address failed\n");
			wdata->data.length = strlen(extra);
		}
		atbm_printk_err("read mac address failed\n");
		return ret;
	}
	//if(extra){
		//memcpy((u8 *)extra, (u8*)macAddr, 6);
	//	wdata->data.length = strlen(extra);
	//}
	return ret;
}

static int atbm_ioctl_get_First_mac(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	//int index;
	u8 macAddr[6] = {0};
	//u8 extraBuff[18] = {0};
	int ret = -EINVAL;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
/*
	if(wrqu->data.length > 1){
		atbm_printk_err("command 'getmac' need not parameters\n");
		return ret;
	}
*/
	if ((ret = wsm_get_efuse_first_mac(hw_priv, &macAddr[0])) == 0){
		if(extra){
			sprintf(extra,"macAddr:%02x:%02x:%02x:%02x:%02x:%02x\n", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
			wdata->data.length = strlen(extra);
		}
		atbm_printk_err("macAddr:%02x:%02x:%02x:%02x:%02x:%02x\n", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
		//return ret;
	}
	else{
		if(extra){
			sprintf(extra,"read mac address failed\n");
			wdata->data.length = strlen(extra);
		}
		atbm_printk_err("read mac address failed\n");
		return ret;
	}
	//if(extra){
		//memcpy((u8 *)extra, (u8*)macAddr, 6);
		//wdata->data.length = strlen(extra);
	//}
	return ret;
}

static int atbm_ioctl_getmac(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	//int index;
	//u8 macAddr[6] = {0};
	//u8 extraBuff[18] = {0};
	int ret = -EINVAL;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	//struct ieee80211_local *local = sdata->local;
	//struct atbm_common *hw_priv=local->hw.priv;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;

	if(wrqu->data.length > 1){
		atbm_printk_err("command 'getmac' need not parameters\n");
		return ret;
	}
#if 0	
	if ((ret = wsm_get_mac_address(hw_priv, &macAddr[0])) == 0){
		atbm_printk_err("macAddr:%02x:%02x:%02x:%02x:%02x:%02x\n", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
		//return ret;
	}
	else{
		atbm_printk_err("read mac address failed\n");
		return ret;
	}
#endif
	ret = 0;
	atbm_printk_err("ifname[%s],macAddr:[%pM] \n",sdata->name,sdata->vif.addr);
	if(copy_to_user(wdata->data.pointer, sdata->vif.addr, 6) != 0){
		ret = -EINVAL;
		//goto exit;
	}
	return ret;
}


#ifdef CONFIG_ATBM_START_WOL_TEST
static int atbm_ioctl_start_wol(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	int i;
	int ret = 0;
	int value = 0;
	char *extra = NULL;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *vif;

	if(!(extra = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		return -EINVAL;
	}
	if((ret = copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(extra);
		return -EINVAL;
	}

	if((wrqu->data.length <= 1) || (wrqu->data.length > 2)){
		atbm_printk_err("invalid parameter,please try again!\n");
		atbm_kfree(extra);
		return -EINVAL;
	}

	value = *extra - 0x30;
	if((value < 0 ) || (value > 1)){
		atbm_printk_err("invalid parameter,parameter must be 1 or 0\n");
		atbm_kfree(extra);
		return -EINVAL;
	}
	atbm_printk_wext("start wol func(%d)\n", value);
	atbm_for_each_vif(hw_priv,vif,i){
		if(vif != NULL)
		{
			__le32 val32;
			if (value != (u32) -1)
				val32 = __cpu_to_le32(value);
			else
				val32 = 0; /* disabled */
	
			ret = WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_START_WOL,
				&val32, sizeof(val32), vif->if_id));
			break;
		}
	}
	atbm_kfree(extra);
	return ret;

}
#endif
#define MAC2STR(a) (a)[0],(a)[1],(a)[2],(a)[3],(a)[4],(a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#ifdef ATBM_PRIVATE_IE
/*
*1, SSTAR_INSERT_USERDATA_CMD    	
*	ioctl(global->ioctl_sock, SSTAR_INSERT_USERDATA_CMD, &user_data)
*	
*   update special ie to beacon,probe response and probe request ,if possible 
*/
static int atbm_ioctl_ie_insert_user_data(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct ieee80211_sub_if_data *sdata_update;
	u8 atbm_oui[4]={0x41,0x54,0x42,0x4D};
	
	int ret = 0;
	char *special = NULL;
	int len = 0;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	
	if(!ieee80211_sdata_running(sdata)){
		ret = -ENETDOWN;
		goto exit;
	}

	len = wdata->data.length-1;

	if((len<0)||(len>255)){
		atbm_printk_err("ErrMsg: data too long\n");
		ret = -EINVAL;
		goto exit;
	}

	special = atbm_kzalloc(len+4, GFP_KERNEL);

	if(special == NULL){
		ret = -EINVAL;
		goto exit;
	}
	
	memcpy(special,atbm_oui,4);
	
	if(copy_from_user(special+4, wdata->data.pointer, len)){
		atbm_printk_err("[IE] copy_from_user fail\n");
		ret = -ENODATA;
		goto exit;
	}
	atbm_printk_always("[IE]:%s\n",special);
	list_for_each_entry(sdata_update, &local->interfaces, list){
		bool res = true;
		
		if(!ieee80211_sdata_running(sdata_update)){
			continue;
		}

		if(sdata_update->vif.type == NL80211_IFTYPE_STATION){
			res = ieee80211_ap_update_special_probe_request(sdata_update,special,len+4);
		}else if((sdata_update->vif.type == NL80211_IFTYPE_AP)&&
		         (rtnl_dereference(sdata_update->u.ap.beacon))){
		    res = ieee80211_ap_update_special_beacon(sdata_update,special,len+4);
			if(res == true){
				res = ieee80211_ap_update_special_probe_response(sdata_update,special,len+4);
			}
		}
		if(res == false){
			atbm_printk_err("[IE] insert failed\n");
			ret = -EOPNOTSUPP;
			goto exit;
		}
	}
exit:
	if(special)
		atbm_kfree(special);
	return ret;
}
static int atbm_ioctl_get_txrx_info(struct net_device *dev, struct iw_request_info *info,  union iwreq_data  *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
//	struct atbm_common *hw_priv=local->hw.priv;
	int ret = 0;
	atbm_wifi_txrx_info txrx_info;
	struct TX_RX_Statistics_S txrx_stat;
	char *results = NULL;
//	int sta_id;
//	struct atbm_vif *priv = (struct atbm_vif *)sdata->vif.drv_priv;
	int copy_len = 0;
	int total_len = 0;
	struct sta_info *sta;
	memset(&txrx_info,0,sizeof(atbm_wifi_txrx_info));
	
	//ptr_len = wdata->data.length - 1;
	/*
	*max is 513
	*/
	results = atbm_kzalloc(512,GFP_KERNEL);
	
	if(results == NULL){
		ret = -ENOMEM;
		goto exit;
	}	
	mutex_lock(&local->sta_mtx);
	list_for_each_entry_rcu(sta, &local->sta_list, list) {
		if((sta->sdata != sdata) || 
		   (sta->uploaded == false) || 
		   (sta->dead == true) ||
		   (test_sta_flag(sta, WLAN_STA_AUTHORIZED) == 0)){
			continue;
		}
		   
		ret = hmac_get_txrx_status_wh(sta, &txrx_stat);
		if(ret == 0){
			txrx_info.rx_packets = txrx_stat.received_num;
			txrx_info.tx_packets = txrx_stat.transmit_total_cnt;
			txrx_info.tx_suc_packets = txrx_stat.sum_success_cnt;
			txrx_info.rx_avg_rate = txrx_stat.rx_rate_avg;
		}
		
		copy_len = scnprintf(results + total_len,512-total_len,"mac[%pM],rssi[%d],rp[%ld],tp[%ld],tsp[%ld],rrate[%d]\n",
		sta->sta.addr,(s8)-atbm_ewma_read(&sta->avg_signal2),
		txrx_info.rx_packets,txrx_info.tx_packets,txrx_info.tx_suc_packets,txrx_info.rx_avg_rate);
		
		if(copy_len > 0)
			total_len += copy_len;
		else {
			break;
		}		
	}
	mutex_unlock(&local->sta_mtx);
	if(extra){
		memcpy(extra,results,total_len);	
		wrqu->data.length = total_len + 1;
	}

	

exit:

	if(results)
		atbm_kfree(results);
	return ret;
}

int hex2digit(int c)
{
	if('0'<=c && c<='9')
		return c-'0';
	if('A'<=c && c<='F')
		return c-('A'-10);
	if('a'<=c && c<='f')
		return c-('a'-10);
	return -1;
}

/*
*1, SSTAR_INSERT_USERDATA_CMD    	
*	ioctl(global->ioctl_sock, SSTAR_INSERT_USERDATA_CMD, &user_data)
*	
*   update special ie to beacon,probe response and probe request ,if possible 
*/
static int atbm_ioctl_ie_insert_vendor_ie(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct ieee80211_sub_if_data *sdata_update;
	u8 atbm_oui[4]={0x41,0x54,0x42,0x4D};
	
	int ret = 0;
	char *extra = NULL;
	char *ie_str =NULL;
	int ie_str_len;
	u8 vendor_ie[255];
	int i,j;
	
	if(!ieee80211_sdata_running(sdata)){
		ret = -ENETDOWN;
		goto exit;
	}

	extra = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL);

	if(extra == NULL){
		ret =  -ENOMEM;
		goto exit;
	}

	if((ret = copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))){
		ret = -EINVAL;
		goto exit;
	}
	
	extra[wrqu->data.length] = 0;
	
	for(i=0; i<wrqu->data.length; i++){
		if(extra[i] == ','){
			ie_str = &extra[i+1];
			break;
		}
	}
	
	ie_str_len = wrqu->data.length - i - 2;
	if((ie_str_len <= 0) || (ie_str_len > 251*2) || (ie_str_len%2)){
		atbm_printk_err("ErrMsg: vendor_ie_len err(%d)\n", ie_str_len);
		ret = -EINVAL;
		goto exit;
	}

	atbm_printk_always("VENDOR IE:%s,LEN:%d\n",ie_str,ie_str_len);

	for(i=0;i<ie_str_len;i++){
		ret = hex2digit(ie_str[i]);
		if(ret < 0){
			atbm_printk_err("ErrMsg: ie payload format err\n");
			ret = -EINVAL;
			goto exit;
		}
		ie_str[i] = ret;
	}
	
	memcpy(vendor_ie, atbm_oui, 4);
	for(i=0,j=0; j<ie_str_len/2; i+=2,j++){
		vendor_ie[j+4] = ie_str[i+1] | (ie_str[i]<<4);
	}

	list_for_each_entry(sdata_update, &local->interfaces, list){
		bool res = true;
		
		if(!ieee80211_sdata_running(sdata_update)){
			continue;
		}

		res = ieee80211_ap_update_vendor_probe_request(sdata_update,vendor_ie,ie_str_len/2+4);
#if 0
		if(sdata_update->vif.type == NL80211_IFTYPE_STATION){
			res = ieee80211_ap_update_special_probe_request(sdata_update,special,len+4);
		}else if((sdata_update->vif.type == NL80211_IFTYPE_AP)&&
		         (rtnl_dereference(sdata_update->u.ap.beacon))){
		    res = ieee80211_ap_update_special_beacon(sdata_update,special,len+4);
			if(res == true){
				res = ieee80211_ap_update_special_probe_response(sdata_update,special,len+4);
			}
		}
#endif
		if(res == false){
			atbm_printk_err("[IE] insert failed\n");
			ret = -EOPNOTSUPP;
			goto exit;
		}
	}	
exit:
	if(extra)
		atbm_kfree(extra);	
	return ret;
}


/*
*2,  SSTAR_GET_USERDATA_CMD    			
*	ioctl(global->ioctl_sock, SSTAR_INSERT_USERDATA_CMD, &user_data)
*	get special ie of the received beacon
*/
static bool atbm_handle_special_ie(struct ieee80211_hw *hw,struct atbm_internal_scan_results_req *req,struct ieee80211_internal_scan_sta *sta)
{
	u8 *special_ie = (u8*)req->priv;
	u8 *pos = NULL;
	u8 atbm_oui[4]={0x41,0x54,0x42,0x4D};//ATBM
	
	if(req->n_stas >= MAC_FILTER_NUM){
		return false;
	}

	pos = special_ie + (req->n_stas*USER_DATE_LEN);

	if(sta->ie&&sta->ie_len){
		if(memcmp(sta->ie,atbm_oui,4) == 0){
			if((sta->ie_len>4)){
				memcpy(pos,sta->ie+4,sta->ie_len-4);
				req->n_stas++;
			}
		}else {
			memcpy(pos,sta->ie,sta->ie_len);
			req->n_stas++;
		}
	}
	return true;
}
static int atbm_ioctl_ie_get_user_data(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	int ret = 0;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct atbm_internal_scan_results_req req;
	u8 *special_ie = NULL;
	
	if(!ieee80211_sdata_running(sdata)){
		ret = -ENETDOWN;
		goto exit;
	}

	special_ie = atbm_kzalloc(MAC_FILTER_NUM*USER_DATE_LEN, GFP_KERNEL);

	if(special_ie == NULL){
		ret = -EINVAL;
		goto exit;
	}

	req.flush = true;
	req.priv  = special_ie;
	req.result_handle  = atbm_handle_special_ie;
	req.n_stas = 0;
	ieee80211_scan_internal_req_results(local,&req);
	atbm_printk_always("ie:%s\n",special_ie);
	if(copy_to_user(wdata->data.pointer, special_ie, USER_DATE_LEN*MAC_FILTER_NUM) != 0){
		ret = -EINVAL;
		goto exit;
	}
	
exit:
	if(special_ie)
		atbm_kfree(special_ie);
	
	return ret;
}


/*
*3,  SSTAR_SEND_MSG_CMD       		     
*    ioctl(global->ioctl_sock, SSTAR_SEND_MSG_CMD, &Wifi_Send_Info_t)
*
*    triger scan
*/
static int atbm_ioctl_ie_send_msg(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	int ret = 0;
	int len = 0;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	Wifi_Send_Info_t *send_info = NULL;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_internal_scan_request internal_scan;
	u8 channel = 0;
	
	memset(&internal_scan,0,sizeof(struct ieee80211_internal_scan_request));

	if(!ieee80211_sdata_running(sdata)){
		ret = -ENETDOWN;
		goto exit;
	}

	if(sdata->vif.type != NL80211_IFTYPE_STATION){
		ret = -EOPNOTSUPP;
		goto exit;
	}

	len = wdata->data.length-1;

	if(len>0){
		
		send_info = (Wifi_Send_Info_t *)atbm_kzalloc(wdata->data.length+1, GFP_KERNEL);
		if(send_info == NULL){
			ret =  -ENOMEM;
			goto exit;
		}
		if(copy_from_user((void *)send_info, wdata->data.pointer, wdata->data.length)){
			ret =  -EINVAL;
			goto exit;
		}

		if(send_info->mac_filter){
			u8 i = 0;
			internal_scan.macs = atbm_kzalloc(MAC_FILTER_NUM*sizeof(struct ieee80211_internal_mac), GFP_KERNEL);

			if(internal_scan.macs == NULL){
				ret =  -EINVAL;
				goto exit;
			}

			for(i = 0;i<MAC_FILTER_NUM;i++){
				memcpy(internal_scan.macs[i].mac,send_info->Bssid[i],6);
			}
			internal_scan.n_macs = MAC_FILTER_NUM;
		}
		channel = send_info->channel;
	}else {
	}

	if(channel != 0){
		internal_scan.channels = &channel;
		internal_scan.n_channels = 1;
	}
    if(atbm_internal_cmd_scan_triger(sdata,&internal_scan) == false){
		ret =  -ENOMEM;
		goto exit;
    }
exit:
	if(send_info)
		atbm_kfree(send_info);
	if(internal_scan.macs)
		atbm_kfree(internal_scan.macs);
	return ret;
}
static bool atbm_handle_scan_sta(struct ieee80211_hw *hw,struct atbm_internal_scan_results_req *req,struct ieee80211_internal_scan_sta *sta)
{
	Wifi_Recv_Info_t *info     = (Wifi_Recv_Info_t *)req->priv;
	Wifi_Recv_Info_t *pos_info = NULL;
	u8 atbm_oui[4]={0x41,0x54,0x42,0x4D};//ATBM
	
	if(req->n_stas >= MAC_FILTER_NUM){
		return false;
	}
#ifndef SIGMASTAR_FILTER_MACADDR_ONLY
	atbm_printk_debug("%s:ssid[%s] ie_len(%d)\n",__func__,sta->ssid,sta->ie_len);
	if((sta->ie == NULL) || (sta->ie_len == 0))
		return true;
#endif
	if(sta->ie && sta->ie_len){	
		atbm_printk_debug("%s:ie[%s] ie_len(%d)\n",__func__,sta->ie,sta->ie_len);
		//if(memcmp(sta->ie,atbm_oui,4) || (sta->ie_len < 4))
		if(sta->ie_len == 0)
			return true;
	}
	
	pos_info = info+req->n_stas;
	req->n_stas ++;
	pos_info->channel = sta->channel;
	pos_info->Rssi = sta->signal;
	memcpy(pos_info->Bssid,sta->bssid,6);
	if(sta->ssid_len && sta->ssid)
		memcpy(pos_info->Ssid,sta->ssid,sta->ssid_len);
	if(sta->ie && sta->ie_len){
		if((sta->ie_len >= 4) && (memcmp(sta->ie,atbm_oui,4) == 0))
			memcpy(pos_info->User_data,sta->ie+4,sta->ie_len-4);
		else
			memcpy(pos_info->User_data,sta->ie,sta->ie_len);
	}
	return true;
}

/*
*4,  SSTAR_RECV_MSG_CMD       
*	 ioctl(global->ioctl_sock, SSTAR_RECV_MSG_CMD, &Wifi_Recv_Info_t)
*	 get the received beacon and probe response
*/
static int atbm_ioctl_ie_recv_msg(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	u8 *recv_info = NULL;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct atbm_internal_scan_results_req req;
	int ret = 0;

	req.flush = false;
	req.n_stas = 0;
	req.priv   = NULL;
	req.result_handle = NULL;
	
	if(!ieee80211_sdata_running(sdata)){
		ret = -ENETDOWN;
		goto exit;
	}

	recv_info = atbm_kzalloc(sizeof(Wifi_Recv_Info_t)*MAC_FILTER_NUM, GFP_KERNEL);

	if(recv_info == NULL){
		ret =  -ENOMEM;
		goto exit;
	}

	req.flush = true;
	req.n_stas = 0;
	req.priv = recv_info;
	req.result_handle = atbm_handle_scan_sta;

	ieee80211_scan_internal_req_results(sdata->local,&req);

	if(copy_to_user((void *)wdata->data.pointer, recv_info, MAC_FILTER_NUM*sizeof(Wifi_Recv_Info_t)) != 0){
		ret = -EINVAL;
	}
exit:
	if(recv_info)
		atbm_kfree(recv_info);
	return ret;
}
static int atbm_ioctl_ie_ipc_clear_insert_data(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
        int ret = 0;

        struct ieee80211_sub_if_data *sdata = NULL;

        if(dev == NULL){
                atbm_printk_debug("[IE] atbm_ioctl_ie_ipc_reset() dev NULL\n");
                return -1;
        }

        sdata = IEEE80211_DEV_TO_SUB_IF(dev);

        if(sdata == NULL){
                atbm_printk_err("[IE] atbm_ioctl_ie_ipc_reset() sdata NULL\n");
                return -1;
        }
		if(!ieee80211_sdata_running(sdata)){
			return -ENETDOWN;
		}
   

        atbm_printk_err("[IE] atbm_ioctl_ie_ipc_clear_insert_data()\n\n");
     	{
                
			struct ieee80211_local *local = sdata->local;
			struct ieee80211_sub_if_data *sdata_update;
			list_for_each_entry(sdata_update, &local->interfaces, list){
				bool res = true;
				
				if(!ieee80211_sdata_running(sdata_update)){
					continue;
				}

				if(sdata_update->vif.type == NL80211_IFTYPE_STATION){
					res = ieee80211_ap_update_special_probe_request(sdata_update,NULL,0);
				}else if((sdata_update->vif.type == NL80211_IFTYPE_AP)&&
				         (rtnl_dereference(sdata_update->u.ap.beacon))){
				    res = ieee80211_ap_update_special_beacon(sdata_update,NULL,0);
					if(res == true){
						res = ieee80211_ap_update_special_probe_response(sdata_update,NULL,0);
					}
				}
				if(res == false){
					ret = -EOPNOTSUPP;
					return ret;
				}
			}
				
        }



        return ret;
		
}

#if 0
static int atbm_ioctl_ie_test(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	int ret = 0;

	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	//struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	//struct atbm_vif *priv = ABwifi_hwpriv_to_vifpriv(hw_priv,0);
	Wifi_Send_Info_t send_info;
	char *ptr = NULL;
	unsigned short channel = 0;
	
	struct ieee80211_sub_if_data *sdata = NULL;
	struct atbm_vif *priv = NULL;
	
	if(dev == NULL){
		atbm_printk_err("[IE] atbm_ioctl_ie_test() dev NULL\n");
		return -1;
	}

	sdata = IEEE80211_DEV_TO_SUB_IF(dev);

	if(sdata == NULL){
		atbm_printk_err("[IE] atbm_ioctl_ie_test() sdata NULL\n");
		return -1;
	}
	

	priv = (struct atbm_vif *)sdata->vif.drv_priv;
	
	if(atomic_read(&priv->enabled)==0){
		atbm_printk_err("[IE] atbm_ioctl_ie_test() priv is disabled\n");
		return -1;
	}	

	atbm_printk_wext("\n[IE] atbm_ioctl_ie_test()\n\n");

	if(sdata->vif.type == NL80211_IFTYPE_STATION){
		atbm_printk_wext("[IE] STA Test Start\n");
		if(!(ptr = (char *)atbm_kzalloc(wdata->data.length+1, GFP_KERNEL))){
			return -ENOMEM;
		}
		if((ret = copy_from_user(ptr, wdata->data.pointer, wdata->data.length))){
			atbm_kfree(ptr);
			return -EINVAL;
		}

		frame_hexdump("Test", ptr, 10);
		
		//channel = my_atoi(ptr);
		//channel = 3;
		//channel = *(unsigned short *)ptr;
		//memcpy(&channel, ptr, sizeof(unsigned short));
		channel = str2channel(ptr);
		atbm_printk_wext("[IE] channel is %d\n", channel);
		
		memset(&send_info, 0, sizeof(Wifi_Send_Info_t));
		
		send_info.channel = channel;

		if(copy_to_user(wdata->data.pointer, (char *)&send_info, sizeof(Wifi_Send_Info_t)) != 0){
			atbm_kfree(ptr);
			return -EINVAL;
		}
		wdata->data.length = sizeof(Wifi_Send_Info_t) + 1;
		
		atbm_ioctl_ie_send_msg(dev, info, (void *)wrqu, extra);
		atbm_kfree(ptr);
	}else{
		atbm_printk_wext("[IE] AP Test Start\n");
	}

	return ret;
}
#endif
#endif

#ifdef ATBM_SUPPORT_LSLP_PRIV_COMMUNICATION
static int atbm_ioctl_set_lightsleep_info(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	int ret = 0;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct light_sleep_info lightSleepInfo;
	char *light_sleep_data = NULL;
	char *pos;
	int   index;
	unsigned int	word;
	
	if(!ieee80211_sdata_running(sdata)){
		ret = -ENETDOWN;
		goto exit;
	}

	light_sleep_data = atbm_kzalloc(wrqu->data.length+1, GFP_KERNEL);
	
	if(light_sleep_data == NULL){
		ret = -EINVAL;
		goto exit;
	}

	if(copy_from_user(light_sleep_data, wrqu->data.pointer, wrqu->data.length)){
		ret =  -EINVAL;
		goto exit;
	}
	//atbm_printk_err("light_sleep_data:%s\n", light_sleep_data);
	light_sleep_data[wrqu->data.length] = 0;
	pos = light_sleep_data;
	atbm_CmdLine_GetToken(&pos);
	for (index = 0; index < 6; index++)
	{
		if(atbm_CmdLine_GetHex(&pos, &word) < 0)
		{
			ret =  -EINVAL;
			goto exit;
		}
		lightSleepInfo.token[index] = (unsigned char)word;
	}
	for (index = 0; index < 6; index++)
	{
		if(atbm_CmdLine_GetHex(&pos, &word) < 0)
		{
			ret =  -EINVAL;
			goto exit;
		}
		lightSleepInfo.ap1MacAddress[index] = (unsigned char)word;
	}
	for (index = 0; index < 6; index++)
	{
		if(atbm_CmdLine_GetHex(&pos, &word) < 0)
		{
			ret =  -EINVAL;
			goto exit;
		}
		lightSleepInfo.ap2MacAddress[index] = (unsigned char)word;
	}
	for (index = 0; index < 6; index++)
	{
		if(atbm_CmdLine_GetHex(&pos, &word) < 0)
		{
			ret =  -EINVAL;
			goto exit;
		}
		lightSleepInfo.ap3MacAddress[index] = (unsigned char)word;
	}	
	for (index = 0; index < 6; index++)
	{
		if(atbm_CmdLine_GetHex(&pos, &word) < 0)
		{
			ret =  -EINVAL;
			goto exit;
		}
		lightSleepInfo.ap4MacAddress[index] = (unsigned char)word;
	}	
	
	if(atbm_CmdLine_GetInteger(&pos, &word) < 0)
	{
		ret =  -EINVAL;
		goto exit;
	}
	lightSleepInfo.notifyWakeupGpioNum	= (unsigned char)word;
	if(atbm_CmdLine_GetInteger(&pos, &word) < 0)
	{
		ret =  -EINVAL;
		goto exit;
	}
	lightSleepInfo.buttonStateAndWakeupVaule = (unsigned char)word;
	if(atbm_CmdLine_GetInteger(&pos, &word) < 0)
	{
		ret =  -EINVAL;
		goto exit;
	}
	lightSleepInfo.wakeupTypeGpioNum	= (unsigned char)word;
	if(atbm_CmdLine_GetInteger(&pos, &word) < 0)
	{
		ret =  -EINVAL;
		goto exit;
	}
	lightSleepInfo.lostCnt 	= (unsigned char)word;
	if(atbm_CmdLine_GetInteger(&pos, &word) < 0)
	{
		ret =  -EINVAL;
		goto exit;
	}	
	if(word < 30 || word > 200)
	{
		ret =  -EINVAL;
		goto exit;
	}
	lightSleepInfo.wakeTimeMs 			= (unsigned short)word;

	if(atbm_CmdLine_GetInteger(&pos, &word) < 0)
	{
		ret =  -EINVAL;
		goto exit;
	}
	lightSleepInfo.sleepTimeMs 			= word;
	if (lightSleepInfo.sleepTimeMs > 12000)
	{
		ret =  -EINVAL;
		goto exit;
	}
	if(atbm_CmdLine_GetInteger(&pos, &word) < 0)
	{
		ret =  -EINVAL;
		goto exit;
	}
	lightSleepInfo.channel 			        = word;

	ret = wsm_set_lightsleep_info(hw_priv,&lightSleepInfo,sizeof(struct light_sleep_info));


	if(ret == 0){
		sprintf(extra,"\nset_lightsleep_info success\n");
	}else{
		sprintf(extra,"\nset_lightsleep_info fail\n");
	}
	wrqu->data.length = strlen(extra);

exit:
	if(light_sleep_data)
		atbm_kfree(light_sleep_data);
	return ret;
	
}

static int atbm_ioctl_set_lightsleep_start(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	int ret = 0;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	char unused;

	if(!ieee80211_sdata_running(sdata) || (sdata->vif.type != NL80211_IFTYPE_STATION)){
		ret = -ENETDOWN;
		goto exit;
	}
	ret = wsm_set_lightsleep_start(hw_priv,&unused,sizeof(char));

	if(ret == 0){
		sprintf(extra,"\nset_lightsleep_start success\n");
	}else{
		sprintf(extra,"\nset_lightsleep_start fail\n");
	}
	wrqu->data.length = strlen(extra);

exit:
	return ret;

}

#endif


#ifdef SDIO_BUS

extern int atbm_device_hibernate(struct atbm_common *hw_priv);
extern int atbm_device_wakeup(struct atbm_common *hw_priv);
static int atbm_ioctl_device_hibernate(struct net_device *dev, struct iw_request_info *ifno, union iwreq_data *wrqu, char *ext)
{
	int i = 0;	
	int ret = 0;
	u8 hibernate = 0;
	char *extra = NULL;
	char *ptr = NULL;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;

	if(!(extra = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		return -EINVAL;
	}
	if((ret = copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(extra);
		ret = -EINVAL;
		goto exit;
	}
	extra[wrqu->data.length] = 0;

	atbm_printk_always("extra:%s\n",extra);

	if(wrqu->data.length < strlen("hibernate_en,")){
		atbm_printk_err("e.g: ./iwpriv wlan0 common sleep_en,<sleep_enable>\n");
		atbm_printk_err("e.g: ./iwpriv wlan0 common sleep_en,0\n");
		atbm_kfree(extra);
		ret = -EINVAL;
		goto exit;
	}

	for(i = 0;i<wrqu->data.length;i++){
		if(extra[i] == ',')
		{
			ptr = &extra[i+1];
			break;
		}
	}
	atbm_printk_always(" ptr:%s\n",ptr);
	hibernate = *ptr - 0x30;
	atbm_printk_always("%s set sdiodevice hibernate:%d\n", __func__, hibernate);
	if (hibernate){
		atbm_device_hibernate(hw_priv);
	}
	else {
		atbm_device_wakeup(hw_priv);
	}

exit:
	if(extra)
		atbm_kfree(extra);
	return ret;
}
#endif

static int atbm_ioctl_get_rssi(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct altm_wext_msg{
		int type;
		int value;
		int value_data;
		char externData[256];
	}msg;
	
	struct _atbm_wifi_info_{
		int wext_rssi;
		u8	wext_mac[ETH_ALEN];
	}atbm_wifi_info[ATBMWIFI_MAX_STA_IN_AP_MODE];

	int i = 0,j=0,len=0;	
	int ret = 0;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = NULL;
	struct ieee80211_local *local;
	struct atbm_vif *priv = NULL;
	struct atbm_common *hw_priv;	
	struct sta_info *sta;


	if(dev == NULL){
		atbm_printk_err("atbm_ioctl_get_rssi() dev NULL\n");
		return -1;
	}

	sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	if(sdata == NULL){
		atbm_printk_err("atbm_ioctl_get_rssi() sdata NULL\n");
		return -1;
	}

	local = sdata->local;
	if(local == NULL){
		atbm_printk_err("atbm_ioctl_get_rssi() local NULL\n");
		return -1;
	}
	
	mutex_lock(&local->iflist_mtx);

	priv = (struct atbm_vif *)sdata->vif.drv_priv;
	if(priv == NULL){
		atbm_printk_err("atbm_ioctl_get_rssi() priv NULL\n");
		return -1;
	}

	if(atomic_read(&priv->enabled)==0){
		atbm_printk_err("atbm_ioctl_get_rssi() priv is disabled\n");
		mutex_unlock(&local->iflist_mtx);
		return -1;
	}
	
	hw_priv = priv->hw_priv;
	if(hw_priv == NULL){
		atbm_printk_err("atbm_ioctl_get_rssi() hw_priv NULL\n");
		mutex_unlock(&local->iflist_mtx);
		return -1;
	}

	atbm_printk_wext("atbm_ioctl_get_rssi()\n");

	memset(&msg, 0, sizeof(msg));
	memset(atbm_wifi_info,0,sizeof(struct _atbm_wifi_info_)*ATBMWIFI_MAX_STA_IN_AP_MODE);

	rcu_read_lock();
		
	list_for_each_entry_rcu(sta, &local->sta_list, list) {

		if(sta != NULL){
			if (sta->sdata->vif.type == NL80211_IFTYPE_AP){
				atbm_printk_wext( "@@@ sta cnt %d, %zu\n", hw_priv->connected_sta_cnt, sizeof(atbm_wifi_info));
				
				atbm_wifi_info[i].wext_rssi = -atbm_ewma_read(&sta->avg_signal);
				memcpy(atbm_wifi_info[i].wext_mac, sta->sta.addr, ETH_ALEN);
				atbm_printk_err( "%d get sta: rssi %d, "MACSTR"\n", i, atbm_wifi_info[i].wext_rssi, MAC2STR(atbm_wifi_info[i].wext_mac));
				
				++i;
			}else{
				msg.value = -atbm_ewma_read(&sta->avg_signal);
				msg.value_data = -atbm_ewma_read(&sta->avg_signal_data);
				atbm_printk_err( "atbm_ioctl_get_rssi() rssi %d data_rssi %d\n", msg.value, msg.value_data);
				break;
			}
		}

	}

	rcu_read_unlock();

	memcpy((u8*)msg.externData, (u8*)&atbm_wifi_info[0], sizeof(atbm_wifi_info));

/*	if(copy_to_user((u8 *)wdata->data.pointer+userdata_len, (u8*)(&msg), sizeof(msg)) != 0)
//	if( != 0)	
	{
		mutex_unlock(&local->iflist_mtx);
		return -EINVAL;
	}
	*/
	if(extra){
		if(sdata->vif.type == NL80211_IFTYPE_AP){
			for(j=0;j<i;j++){
				sprintf(extra + len,"\nmac:"MACSTR",rssi=%d\n",MAC2STR(atbm_wifi_info[j].wext_mac),atbm_wifi_info[j].wext_rssi);
				len=strlen(extra);
			}
			//memcpy((u8 *)extra, (u8*)(&msg), sizeof(msg));
			wdata->data.length = strlen(extra);
		}else{
			sprintf(extra,"\nrssi=%d data_rssi=%d\n",msg.value,msg.value_data);
			wdata->data.length = strlen(extra);
		}
	}
	//atbm_printk_wext("atbm_ioctl_get_rssi() size %zu , \n",wdata->data.length);
	mutex_unlock(&local->iflist_mtx);

	return ret;
}



//extern unsigned int atbm_wifi_status_get(void);
#if 0
static int atbm_ioctl_get_wifi_state(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	int ret = 0;
	unsigned char *ptr = NULL;
	unsigned int wifi_status = 0;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = NULL;
	struct sta_info *sta;
	struct ieee80211_local *local = sdata->local;
	


	sdata = IEEE80211_DEV_TO_SUB_IF(dev);

	if(sdata == NULL){
		atbm_printk_err("[IE] atbm_ioctl_get_wifi_state() sdata NULL\n");
		return -1;
	}

	
	ptr = wdata->data.pointer;

	//printk("atbm_ioctl_get_wifi_state()\n");

	//ap mode
	if(sdata->vif.type != NL80211_IFTYPE_STATION){
		ret = -1;
		goto Error;
	}
	rcu_read_lock();
	list_for_each_entry_rcu(sta, &local->sta_list, list) {

		if(sta != NULL){
			if (sta->sdata->vif.type == NL80211_IFTYPE_STATION){
				wifi_status = test_sta_flag(sta, WLAN_STA_AUTHORIZED);
			}
				
		}

	}
	rcu_read_unlock();
	
	
//	wifi_status = atbm_wifi_status_get();
	
	atbm_printk_wext("%d\n", wifi_status);

	if(copy_to_user(ptr, (char *)&wifi_status, sizeof(unsigned int)) != 0){
	
		ret = -EINVAL;
	}

Error:	

	return ret;

}
#else
static int atbm_ioctl_get_wifi_state(struct net_device *dev, struct iw_request_info *info,  union iwreq_data  *wrqu, char *extra)
{

	unsigned int wifi_status = 0;

	struct ieee80211_sub_if_data *sdata = NULL;

	sdata = IEEE80211_DEV_TO_SUB_IF(dev);

	wifi_status = atbm_get_sta_wifi_connect_status(sdata);


	atbm_printk_err("%u,%s\n",wifi_status, wifi_status?"CONNECTED":"DISCONNECTED");
	sprintf(extra,"\nwifi_status=%lu\n",wifi_status);
	return 0;

}
#endif
static int atbm_ioctl_set_freq(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	int ret = 0;
	char *freq_info = NULL;
	char *pos;
	int len = 0;
	unsigned int channel_num;
	unsigned int freq;
	struct ieee80211_internal_set_freq_req req;

	if(!ieee80211_sdata_running(sdata)){
		ret = -ENETDOWN;
		goto exit;
	}

	len = wdata->data.length;

	if(len <= 0){
		ret = -EINVAL;
		goto exit;
	}

	freq_info = atbm_kzalloc(wdata->data.length+1, GFP_KERNEL);

	if(freq_info == NULL){
		ret = -EINVAL;
		goto exit;
	}

	if(copy_from_user(freq_info, wdata->data.pointer, wdata->data.length)){
		ret =  -EINVAL;
		goto exit;
	}

	pos = freq_info;
	
	atbm_CmdLine_GetInteger(&pos, &channel_num);
	atbm_CmdLine_GetInteger(&pos, &freq);
	if(freq == 0)
		req.set = false;
	else
		req.set = true;
	req.channel_num = (u16)channel_num;
	req.freq = freq;
	
	atbm_printk_wext("atbm: ch %d, freq %zu\n", req.channel_num, req.freq);

	if(atbm_internal_freq_set(&sdata->local->hw,&req) == false){
		ret =  -EINVAL;
	}

	if(ret == 0)
		atbm_set_freq(sdata->local);
exit:
	if(freq_info)
		atbm_kfree(freq_info);
	return ret;
}



static int atbm_ioctl_set_txpw(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	int ret = 0;
//	int len = 0;
	char *ptr = NULL, *free_pp = NULL;
	char cmd[32];
	unsigned int tx_pw;

	union iwreq_data *wdata = (union iwreq_data *)wrqu;

	struct ieee80211_sub_if_data *sdata = NULL;
	struct ieee80211_local *local = NULL;
	struct atbm_vif *priv = NULL;
	struct atbm_common *hw_priv= NULL;
	
	if(dev == NULL){
		atbm_printk_err("[IE] atbm_ioctl_set_txpw() dev NULL\n");
		return -1;
	}

	sdata = IEEE80211_DEV_TO_SUB_IF(dev);

	if(sdata == NULL){
		atbm_printk_err("[IE] atbm_ioctl_set_txpw() sdata NULL\n");
		return -1;
	}
	
	mutex_lock(&sdata->local->iflist_mtx);
	local = sdata->local;
	hw_priv=local->hw.priv;
	
	priv = (struct atbm_vif *)sdata->vif.drv_priv;
	
	if(atomic_read(&priv->enabled)==0){
		atbm_printk_err("[IE] atbm_ioctl_set_txpw() priv is disabled\n");
		mutex_unlock(&sdata->local->iflist_mtx);
		return -1;
	}

	atbm_printk_wext("atbm_ioctl_set_txpw()\n\n");

	if(!(ptr = (char *)atbm_kzalloc(wdata->data.length+1, GFP_KERNEL))){
		mutex_unlock(&sdata->local->iflist_mtx);
		return -ENOMEM;
	}
	if((ret = copy_from_user(ptr, wdata->data.pointer, wdata->data.length))){
		atbm_kfree(ptr);
		mutex_unlock(&sdata->local->iflist_mtx);
		return -EINVAL;
	}
	
	free_pp = ptr;

//	len = wdata->data.length-1;

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "set_txpower %s ", ptr);
	atbm_printk_wext("atbm: %s\n", cmd);
	ret = wsm_write_mib(hw_priv, WSM_MIB_ID_FW_CMD, cmd, strlen(cmd), priv->if_id);
	if(ret < 0)
		atbm_printk_err("atbm: txpw wsm write mib failed. \n");

	atbm_CmdLine_GetInteger(&ptr, &tx_pw);
	atbm_set_tx_power(hw_priv, (int)tx_pw);
	
	atbm_kfree(free_pp);
	mutex_unlock(&sdata->local->iflist_mtx);

	return ret;
}



static int atbm_ioctl_get_rate(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
//	atbm_printk_err("************************get_tp_rate*****************************\n");
	char mac_addr[6];

	int sta_id = 0;
	unsigned char ptr[30]={0};
	unsigned int rate_val = 0;
	unsigned int ptr_len = 0;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	//struct ieee80211_local *local = sdata->local;
	//struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *priv = (struct atbm_vif *)sdata->vif.drv_priv;
	
	
	struct sta_info* stainfo = NULL;
	
			
	ptr_len = wdata->data.length - 1;
	atbm_printk_wext("atbm_ioctl_get_rate()  %d\n",ptr_len);
	msleep(100);
	//ap mode
	if (sdata->vif.type == NL80211_IFTYPE_AP && (ptr_len == 17)) {
		struct ieee80211_sta* sta;
		//clear mac addr buffer
		memset(mac_addr, 0, 6);
		if(copy_from_user(ptr, wdata->data.pointer, ptr_len)){
			atbm_printk_wext("%s() copy userspace data err!!\n",__func__);
			return -EINVAL;
		}
		atbm_printk_wext("mac = %s \n",ptr);
#if 0		
		for(i=0;i<ptr_len;i++){
			 if(ptr[i] == ','){
				break;
			}
		
		}
#endif		
		//according to mac hex, find out sta link id
		atbm_str2mac(mac_addr, ptr);
		rcu_read_lock();
		//according to mac hex, find out sta link id
		sta = ieee80211_find_sta(&sdata->vif, mac_addr);
		if (sta){
			sta_id = sta->aid;
			stainfo = container_of(sta, struct sta_info, sta);
		}
		rcu_read_unlock();
		atbm_printk_wext("atbm_ioctl_get_rate() sta_id %d,stainfo=%x\n", sta_id, stainfo->last_rx_rate_idx);
//		wsm_write_mib(hw_priv, WSM_MIB_ID_GET_RATE, &sta_id, 1, priv->if_id);
	}
	else
	{
		rcu_read_lock();
		stainfo = sta_info_get(sdata, priv->bssid);
		rcu_read_unlock();
	}

	if(stainfo != NULL)
    {
   		 RATE_CONTROL_RATE *rate_info = (RATE_CONTROL_RATE*)stainfo->rate_ctrl_priv;
		 if(rate_info!= NULL){
    	 	rate_val = rate_info->average_throughput;
		//	atbm_printk_err("%pM,rate: %d bps\n",mac_addr,maxrate_id,rate_info->average_throughput);
		}else{
			atbm_printk_wext("rate_info == NULL \n");
			rate_val = 0;
		}
		
	}else{
		atbm_printk_wext("stainfo == NULL \n");
	}
	if (sdata->vif.type == NL80211_IFTYPE_AP && (ptr_len == 17)) 
		atbm_printk_err("%pM,rate: %d bps\n",mac_addr,rate_val*2000);
	else
		atbm_printk_err("rate: %d bps\n",rate_val*2000);

	//
	return 0;
}

static int atbm_ioctl_get_rx_rate(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{

	int i;
	char mac_addr[6];
	int sta_id = 0;
	unsigned char ptr[30]={0};
	unsigned char maxrate_id = 0;
	unsigned char dcm_used = 0;
	unsigned int ptr_len = 0,curr_rx_rate = 0;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct atbm_vif *priv = (struct atbm_vif *)sdata->vif.drv_priv;
	unsigned char support_40M_symbol = 0;

	
	struct sta_info* stainfo = NULL;

	
	ptr_len = wdata->data.length - 1;
	atbm_printk_wext("atbm_ioctl_get_rate()  %d\n",ptr_len);
	msleep(100);
	//ap mode
	if (sdata->vif.type == NL80211_IFTYPE_AP && (ptr_len == 29)) {
		struct ieee80211_sta* sta;
		//clear mac addr buffer
		memset(mac_addr, 0, 6);
		
		if(copy_from_user(ptr, wdata->data.pointer, ptr_len)){
			atbm_printk_wext("%s() copy userspace data err!!\n",__func__);
			return -EINVAL;
		}
		for(i=0;i<ptr_len;i++){
			 if(ptr[i] == ','){
				break;
			}
			
		}
		//convert mac string to mac hex format
		atbm_str2mac(mac_addr, &ptr[i+1]);
		rcu_read_lock();
		//according to mac hex, find out sta link id
		sta = ieee80211_find_sta(&sdata->vif, mac_addr);
		if (sta){
			sta_id = sta->aid;			
		    stainfo = container_of(sta, struct sta_info, sta);
			}
		rcu_read_unlock();
		atbm_printk_wext("atbm_ioctl_get_rate() sta_id %d\n", sta_id);
//		wsm_write_mib(hw_priv, WSM_MIB_ID_GET_RATE, &sta_id, 1, priv->if_id);
	}
	else{
			rcu_read_lock();
			stainfo = sta_info_get(sdata, priv->bssid);
			rcu_read_unlock();
	}

	if(stainfo != NULL)
    {
   		 RATE_CONTROL_RATE *rate_info = (RATE_CONTROL_RATE*)stainfo->rate_ctrl_priv;
		 if(rate_info!= NULL){
    	 	maxrate_id = rate_info->rx_rate_idx;
			if(rate_info->tx_rc_flags&ATBM_IEEE80211_TX_RC_HE_DCM)
			{
				dcm_used = 1;
			}
			if(rate_info->bw_20M_cnt <= 0)
			{
				support_40M_symbol = 1;
			}
			atbm_printk_always("id:%d,support_40M_symbol: %d\n", maxrate_id, support_40M_symbol);
		}
    

		curr_rx_rate = atbm_get_rate_from_rateid(maxrate_id,0,0)/50000;
		 
		atbm_printk_always("rx rate: %u/10 Mbits/s\n", curr_rx_rate);
		if(extra){	
			sprintf(extra,"\nrx rate:%u/10 Mbps\n",curr_rx_rate);
			wrqu->data.length = strlen(extra);
		}
	}else{
		if(extra){	
			sprintf(extra,"\nnot found sta\n");
			wrqu->data.length = strlen(extra);
		}
	}
	return 0;	
}

static int atbm_ioctl_get_cur_max_rate(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	int ret = 0,i;
	char mac_addr[6];
	int sta_id = 0;
	unsigned char ptr[30]={0};
	unsigned char maxrate_id = 0;
	unsigned char dcm_used = 0;
	unsigned int ptr_len = 0,curr_tx_rate;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	//struct ieee80211_local *local = sdata->local;
	//struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *priv = (struct atbm_vif *)sdata->vif.drv_priv;
	unsigned char support_40M_symbol = 0;

	
	struct sta_info* stainfo = NULL;
	
			
	ptr_len = wdata->data.length - 1;
	atbm_printk_wext("atbm_ioctl_get_rate()  %d\n",ptr_len);
	msleep(100);
	//ap mode
	if (sdata->vif.type == NL80211_IFTYPE_AP && (ptr_len == 29)) {
		struct ieee80211_sta* sta;
		//clear mac addr buffer
		memset(mac_addr, 0, 6);
		
		if(copy_from_user(ptr, wdata->data.pointer, ptr_len)){
			atbm_printk_wext("%s() copy userspace data err!!\n",__func__);
			return -EINVAL;
		}
		for(i=0;i<ptr_len;i++){
			 if(ptr[i] == ','){
				break;
			}
			
		}
		//convert mac string to mac hex format
		atbm_str2mac(mac_addr, &ptr[i+1]);
		rcu_read_lock();
		//according to mac hex, find out sta link id
		sta = ieee80211_find_sta(&sdata->vif, mac_addr);
		if (sta){
			sta_id = sta->aid;			
		    stainfo = container_of(sta, struct sta_info, sta);
			}
		rcu_read_unlock();
		atbm_printk_wext("atbm_ioctl_get_rate() sta_id %d\n", sta_id);
//		wsm_write_mib(hw_priv, WSM_MIB_ID_GET_RATE, &sta_id, 1, priv->if_id);
	}
	else{
			rcu_read_lock();
			stainfo = sta_info_get(sdata, priv->bssid);
			rcu_read_unlock();
	}
	
	//ret = wsm_read_mib(hw_priv, WSM_MIB_ID_GET_CUR_MAX_RATE, &maxrate_id, sizeof(unsigned char), priv->if_id);
    if(stainfo != NULL)
    {
   		 RATE_CONTROL_RATE *rate_info = (RATE_CONTROL_RATE*)stainfo->rate_ctrl_priv;
		 if(rate_info!= NULL){
    	 	maxrate_id = rate_info->max_tp_rate_idx;
			if(rate_info->tx_rc_flags&ATBM_IEEE80211_TX_RC_HE_DCM)
			{
				dcm_used = 1;
			}
			if(rate_info->bw_20M_cnt <= 0)
			{
				support_40M_symbol = 1;
			}
			atbm_printk_always("id:%d,max_tp_rate_idx: %d\n", maxrate_id, rate_info->max_tp_rate_idx);
		}
    }

	curr_tx_rate = atbm_get_rate_from_rateid(maxrate_id,0,0)/50000;

	atbm_printk_always("tx rate: %u/10 Mbits/s\n", curr_tx_rate);
	if(extra){	
		sprintf(extra,"\nsend_rate:%u/10 Mbps\n",curr_tx_rate);
		wrqu->data.length = strlen(extra);
	}
//}
	return ret;
}

#if 0
int atbm_ioctl_best_ch_start(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	int ret = 0;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_internal_channel_auto_select_req req;
	
	if(!ieee80211_sdata_running(sdata)){
		ret = -ENETDOWN;
		goto exit;
	}

	if(sdata->vif.type != NL80211_IFTYPE_STATION){
		ret = -EOPNOTSUPP;
		goto exit;
	}
	memset(&req,0,sizeof(struct ieee80211_internal_channel_auto_select_req));

	if(atbm_internal_channel_auto_select(sdata,&req) == false){
		ret = -EOPNOTSUPP;
	}
exit:
	return ret;
}
int atbm_ioctl_best_ch_scan_result(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	int ret = 0;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_internal_channel_auto_select_results results;
	Best_Channel_Scan_Result scan_result;
	int i = 0;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	
	if(!ieee80211_sdata_running(sdata)){
		ret = -ENETDOWN;
		goto exit;
	}

	if(sdata->vif.type != NL80211_IFTYPE_STATION){
		ret = -EOPNOTSUPP;
		goto exit;
	}
	
	memset(&scan_result,0,sizeof(Best_Channel_Scan_Result));
	memset(&results,0,sizeof(struct ieee80211_internal_channel_auto_select_results));
	results.version = 0;//use version 0
	
	if(atbm_internal_channel_auto_select_results(sdata,&results) == false){
		ret = -EINVAL;
		goto exit;
	}

	for(i = 0;i<CHANNEL_NUM;i++){
		scan_result.channel_ap_num[i] = results.n_aps[i];
	}

	for(i = 0;i<CHANNEL_NUM;i++){
		scan_result.busy_ratio[i] = results.busy_ratio[i];
	}

	scan_result.suggest_ch = results.susgest_channel;

	if(copy_to_user(wdata->data.pointer, &scan_result, sizeof(scan_result)) != 0)
		ret = -EINVAL;
exit:
	return ret;
}
#endif

/*
	support special channel channel 1~42 all channel SpecialFlag = 1	
	other 1~14 all channel 

*/
static int atbm_ioctl_best_ch_scan3(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct ieee80211_internal_channel_auto_select_results *results = NULL;
	struct BestChannelSelect{
		unsigned char start_channel;
		unsigned char end_channel;
		unsigned char SpecialFlag;
		Best_Channel_Scan_Result3 scan_result;
	};
	struct BestChannelSelect BestCh;
	int ret = 0;
	unsigned char *ptr = NULL;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;

	
	if(!ieee80211_sdata_running(sdata)){
		atbm_printk_err("sdata is not running! \n");
		ret = -ENETDOWN;
		goto exit;
	}

	memset(&BestCh,0,sizeof(BestCh));
	if(!(ptr = (char *)atbm_kzalloc(wdata->data.length+1, GFP_KERNEL)))
		return -ENOMEM;
	atbm_printk_warn("wdata->data.length = %d %d\n",wdata->data.length,(int)sizeof(struct BestChannelSelect));
	if((ret = copy_from_user(ptr, wdata->data.pointer, wdata->data.length))){
		atbm_kfree(ptr);
		return -EINVAL;
	}
	atbm_printk_err("ptr:%s \n",ptr);
	results = atbm_kmalloc(sizeof(struct ieee80211_internal_channel_auto_select_results),GFP_KERNEL);
	if(!results){
		ret = -ENETDOWN;
		goto exit;		
	}
	memset(results,0,sizeof(struct ieee80211_internal_channel_auto_select_results));

	sscanf(ptr, "best_ch_scan3,%d,%d,%d", (int*)&BestCh.start_channel, (int*)&BestCh.end_channel, (int*)&BestCh.SpecialFlag);
	
	atbm_printk_warn("start_channel = %d \n",BestCh.start_channel);
	atbm_printk_warn("end_channel = %d \n",BestCh.end_channel);
	atbm_printk_warn("SpecialFlag = %d \n",BestCh.SpecialFlag);

	if((BestCh.start_channel <= 0) || (BestCh.end_channel <= 0) || (BestCh.start_channel > BestCh.end_channel)){
		atbm_printk_warn("input parameters is not allows,start channel[%d] end channel[%d]  \n",
				BestCh.start_channel,BestCh.end_channel);
		ret = -EINVAL;
		goto exit;
	}

	memset(&BestCh.scan_result,0,sizeof(Best_Channel_Scan_Result));
	results->version = 0;//use version 0
	results->best_channel_flag = 3;
	

	results->apInfo_num = 0;

	/*
		start_channel
		end_channel
		2.4G & 5G
		if support 5G but not confine channel, default return 1~14 channel value , suggest_ch range 1~14
		
	*/
	
	//Determine channel validity
	if(atbm_dev_best_channel_scan_end_process(sdata,BestCh.start_channel,BestCh.end_channel,results,(Best_Channel_Scan_Result *)(&(BestCh.scan_result))) < 0){
		ret = -1;
		goto exit;
	}


	if(atbm_internal_channel_auto_select_results2(sdata,results,&BestCh.scan_result.chan[0], BestCh.scan_result.support_chan_num) == false){
		ret = -EINVAL;
		goto exit;
	}
	
	BestCh.scan_result.suggest_ch[0] = results->susgest_3_channel[0];
	BestCh.scan_result.suggest_ch[1] = results->susgest_3_channel[1];
	BestCh.scan_result.suggest_ch[2] = results->susgest_3_channel[2];
	BestCh.SpecialFlag = 0;
	if(ieee8011_channel_valid(&local->hw,36) == true){
		BestCh.SpecialFlag = 1;
		atbm_printk_err("BestCh->SpecialFlag = 1 \n");
	}

	atbm_printk_err("auto_select channel %d , %d ,%d \n",BestCh.scan_result.suggest_ch[0],BestCh.scan_result.suggest_ch[1],BestCh.scan_result.suggest_ch[2]);
	if(ext){
		memcpy(ext,&BestCh,sizeof(struct BestChannelSelect));

		wdata->data.length = sizeof(struct BestChannelSelect);
	}
	//if(copy_to_user(wdata->data.pointer, (char*)BestCh, sizeof(struct BestChannelSelect)) != 0)
	//	ret = -EINVAL;
	
	//wdata->data.length = sizeof(struct BestChannelSelect);
exit:

	if(ptr)
		atbm_kfree(ptr);
	if(results)
		atbm_kfree(ptr);
	return ret;
}

static int atbm_ioctl_best_ch_scan2(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct ieee80211_internal_channel_auto_select_results *results = NULL;
	int ret = 0;
	struct BestChannelSelect{
		unsigned char start_channel;
		unsigned char end_channel;
		unsigned char SpecialFlag;
		unsigned char ap_ssid[32];
		Best_Channel_Scan_Result2 scan_result;
	};
	struct BestChannelSelect *BestCh = NULL;
	char *ptr = NULL;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	
	if(!ieee80211_sdata_running(sdata)){
		atbm_printk_err("sdata is not running! \n");
		ret = -ENETDOWN;
		goto exit;
	}

	BestCh = atbm_kmalloc(sizeof(struct BestChannelSelect),GFP_KERNEL);
	if(!BestCh){
		ret = -ENETDOWN;
		goto exit;		
	}
	memset(BestCh,0,sizeof(struct BestChannelSelect));

	results = atbm_kmalloc(sizeof(struct ieee80211_internal_channel_auto_select_results),GFP_KERNEL);
	if(!results){
		ret = -ENETDOWN;
		goto exit;		
	}
	memset(results,0,sizeof(struct ieee80211_internal_channel_auto_select_results));

	if(!(ptr = (char *)atbm_kzalloc(wdata->data.length+1, GFP_KERNEL)))
		return -ENOMEM;
	atbm_printk_warn("wdata->data.length = %d %d\n",wdata->data.length,(int)sizeof(struct BestChannelSelect));
	if((ret = copy_from_user(ptr, wdata->data.pointer, wdata->data.length))){
		atbm_kfree(ptr);
		return -EINVAL;
	}
	atbm_printk_err("ptr:%s \n",ptr);
//	BestCh = (struct BestChannelSelect *)(ptr+14);

	sscanf(ptr, "best_ch_scan2,%d,%d,%d,%32s", (int*)&BestCh->start_channel, (int*)&BestCh->end_channel, (int*)&BestCh->SpecialFlag, BestCh->ap_ssid);
	
	atbm_printk_warn("start_channel = %d \n",BestCh->start_channel);
	atbm_printk_warn("end_channel = %d \n",BestCh->end_channel);
	atbm_printk_warn("SpecialFlag = %d \n",BestCh->SpecialFlag);

	if((BestCh->start_channel <= 0) || (BestCh->end_channel <= 0) || (BestCh->start_channel > BestCh->end_channel)){
		atbm_printk_warn("input parameters is not allows,start channel[%d] end channel[%d]  \n",
				BestCh->start_channel,BestCh->end_channel);
		ret = -EINVAL;
		goto exit;
	}

	results->version = 0;//use version 0
	results->best_channel_flag = 2;
	
	atbm_printk_err("TP_BEST_CHANNEL_ADD_FUNC!!BestCh->ap_ssid=%s \n",BestCh->ap_ssid);
	memcpy(results->ap_ssid,BestCh->ap_ssid,32);
	results->apInfo_num = 0;

	/*
		start_channel
		end_channel
		2.4G & 5G
		if support 5G but not confine channel, default return 1~14 channel value , suggest_ch range 1~14
		
	*/
	
	//Determine channel validity
	if(atbm_dev_best_channel_scan_end_process(sdata,BestCh->start_channel,BestCh->end_channel,results,(Best_Channel_Scan_Result *)(&(BestCh->scan_result))) < 0){
		ret = -1;
		goto exit;
	}


	if(atbm_internal_channel_auto_select_results(sdata,results,&BestCh->scan_result.chan[0], BestCh->scan_result.support_chan_num) == false){
		ret = -EINVAL;
		goto exit;
	}
	
	
	BestCh->scan_result.suggest_ch = results->susgest_channel;
	BestCh->SpecialFlag = 0;
	if(ieee8011_channel_valid(&local->hw,36) == true){
		BestCh->SpecialFlag = 1;
		atbm_printk_err("BestCh->SpecialFlag = 1 \n");
	}
	
	memcpy(BestCh->scan_result.apInfo,results->apInfo,sizeof(results->apInfo));

	atbm_printk_err("auto_select channel %d\n",BestCh->scan_result.suggest_ch);
	if(ext){
		memcpy(ext,BestCh,sizeof(struct BestChannelSelect));

		wdata->data.length = sizeof(struct BestChannelSelect);
	}
	//if(copy_to_user(wdata->data.pointer, (char*)BestCh, sizeof(struct BestChannelSelect)) != 0)
	//	ret = -EINVAL;
	
	//wdata->data.length = sizeof(struct BestChannelSelect);
exit:
	if(ptr)
		atbm_kfree(ptr);
	if(BestCh)
		atbm_kfree(BestCh);
	if(results)
		atbm_kfree(results);
	return ret;
}


static int atbm_ioctl_best_ch_scan(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_internal_channel_auto_select_results *results = NULL;
	u8 start_channel,end_channel;
	struct BestChannelSelect{
		unsigned char start_channel;
		unsigned char end_channel;
		unsigned char SpecialFlag;
		Best_Channel_Scan_Result scan_result;
	};
	struct BestChannelSelect *BestCh = NULL;
	int ret = 0;
	unsigned char *ptr = NULL;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;

	if(!ieee80211_sdata_running(sdata)){
		atbm_printk_err("sdata is not running! \n");
		ret = -ENETDOWN;
		goto exit;
	}

	WARN_ON(wdata->data.length < sizeof(struct BestChannelSelect));
	if(!(ptr = (char *)atbm_kzalloc(wdata->data.length+1, GFP_KERNEL)))
		return -ENOMEM;

	atbm_printk_warn("wdata->data.length = %d %d\n",wdata->data.length,(int)sizeof(struct BestChannelSelect));
	if((ret = copy_from_user(ptr, wdata->data.pointer, wdata->data.length))){
		atbm_kfree(ptr);
		return -EINVAL;
	}
	//BestCh = (struct BestChannelSelect *)ptr;
	BestCh = atbm_kzalloc(sizeof(struct BestChannelSelect), GFP_KERNEL);
    sscanf(ptr,"%d,%d,%d",&BestCh->start_channel,&BestCh->end_channel,&BestCh->SpecialFlag);

	results = atbm_kmalloc(sizeof(struct ieee80211_internal_channel_auto_select_results),GFP_KERNEL);
	if(!results){
		ret = -ENETDOWN;
		goto exit;		
	}
	memset(results,0,sizeof(struct ieee80211_internal_channel_auto_select_results));
	atbm_printk_warn("start_channel = %d \n",BestCh->start_channel);
	atbm_printk_warn("end_channel = %d \n",BestCh->end_channel);
	atbm_printk_warn("SpecialFlag = %d \n",BestCh->SpecialFlag);

	if((BestCh->start_channel <= 0) || (BestCh->end_channel <= 0) || (BestCh->start_channel > BestCh->end_channel)){
		atbm_printk_warn("input parameters is not allows,start channel[%d] end channel[%d]  \n",
				BestCh->start_channel,BestCh->end_channel);
		ret = -EINVAL;
		goto exit;
	}

	memset(&BestCh->scan_result,0,sizeof(Best_Channel_Scan_Result));
	results->version = 0;//use version 0
	results->best_channel_flag = 1;
	/*
		start_channel
		end_channel
		2.4G & 5G
		if support 5G but not confine channel, default return 1~14 channel value , suggest_ch range 1~14
		
	*/
	
	start_channel = BestCh->start_channel;
	end_channel = BestCh->end_channel;
	
	//Determine channel validity
	if(atbm_dev_best_channel_scan_end_process(sdata,BestCh->start_channel,BestCh->end_channel,results,&(BestCh->scan_result)) < 0){
		ret = -1;
		goto exit;
	}

	
	if(atbm_internal_channel_auto_select_results(sdata,results,&BestCh->scan_result.chan[0], BestCh->scan_result.support_chan_num) == false){
		ret = -EINVAL;
		goto exit;
	}

	BestCh->scan_result.suggest_ch = results->susgest_channel;
	if(copy_to_user(wdata->data.pointer, (char*)BestCh, sizeof(struct BestChannelSelect)) != 0)
		ret = -EINVAL;

	wdata->data.length = sizeof(struct BestChannelSelect);
exit:

	if(ptr)
		atbm_kfree(ptr);
	if(results)
		atbm_kfree(results);
	if(BestCh)
		atbm_kfree(BestCh);
	return ret;

}

static int atbm_ioctl_get_Private_256BITSEFUSE(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	u8 efuseBuff[32] = {0};
	//u8 extraBuff[18] = {0};
	int ret = -EINVAL;
	int i;
//	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;

	//if(wrqu->data.length > 1){
	//	printk("command 'getsigmstarefuse' need not parameters\n");
	//	return ret;
	//}

	if ((ret = wsm_get_SIGMSTAR_256BITSEFUSE(hw_priv, &efuseBuff[0], sizeof(efuseBuff))) == 0){
		
		atbm_printk_wext("Get Private efuse data:\n");
		for(i = 0; i < sizeof(efuseBuff); i++)
		{
			atbm_printk_always("%02x ", efuseBuff[i]);
		}
		atbm_printk_always("\n");
	}
	else{
		atbm_printk_err("read private efuse failed\n");
	}
	/*
	userdata_len = wdata->data.length;
	if(copy_to_user(wdata->data.pointer + userdata_len, efuseBuff, 32) != 0){
		ret = -EINVAL;
		atbm_printk_wext("copy to user failed.\n");
	}
	*/
	if(extra){	
		memcpy(extra,efuseBuff,32);	
		wrqu->data.length = 33;
	}
	
	return ret;	
}
static int atbm_ioctl_set_Private_256BITSEFUSE(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	int i;
	int ret = -EINVAL;
	char *extra = NULL;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	u8 efuseBuff[32 +1];
	int strheadLen, trueLen;
	atbm_printk_wext("####### efuse\n");
	if(!(extra = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		return -EINVAL;
	}
	if((ret = copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(extra);
		return -EINVAL;
	}
	atbm_printk_err("length=%d,data=%s\n", wrqu->data.length, extra);
	strheadLen=strlen("setSigmstarEfuse,");
	trueLen =((wrqu->data.length - strheadLen) > 32) ? 32:(wrqu->data.length - strheadLen);
	atbm_printk_err("trueLen=%d,strheadLen=%d\n", trueLen, strheadLen);

	memset(efuseBuff, 0, sizeof(efuseBuff));
	for(i =0; i < trueLen; i++)
	{
		efuseBuff[i] = extra[strheadLen + i];
	}
	
	if ((ret = wsm_set_SIGMSTAR_256BITSEFUSE(hw_priv, &efuseBuff[0], 32)) == 0)
	{
		atbm_printk_wext("Set Private efuse data:\n");
		for(i = 0; i < sizeof(efuseBuff); i++)
		{
			atbm_printk_wext("%02hhx ", efuseBuff[i]);
		}
		atbm_printk_wext("\n");
	}
	else{
		atbm_printk_err("write private efuse failed\n");
	}

	if(extra)
		atbm_kfree(extra);
	return ret;
}


#ifdef CONFIG_ATBM_IWPRIV_USELESS
static int atbm_ioctl_channel_test_start(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	u8 chTestStart = 0;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *priv = (struct atbm_vif *)sdata->vif.drv_priv;

	wsm_write_mib(hw_priv, WSM_MIB_ID_CHANNEL_TEST_START, &chTestStart, sizeof(u8), priv->if_id);
//	wsm_read_mib(hw_priv, WSM_MIB_ID_GET_CHANNEL_IDLE, NULL, sizeof(unsigned short), priv->if_id);

	return 0;
}
#endif
static int atbm_ioctl_get_channel_idle(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	int ret = 0;
	
//	unsigned char *ptr = NULL;
	unsigned short idle = 0;

	unsigned short tx_laten_l2 = 0;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *priv = (struct atbm_vif *)sdata->vif.drv_priv;


	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	
	//struct ieee80211_local *local = sdata->local;
	//struct atbm_common *hw_priv=local->hw.priv;
	//struct atbm_vif *priv = (struct atbm_vif *)sdata->vif.drv_priv;

	if(sdata->vif.type != NL80211_IFTYPE_AP &&  sdata->vif.type != NL80211_IFTYPE_STATION){
		atbm_printk_err("atbm_ioctl_get_channel_idle() ## please run ap or sta mode\n");
		return -EINVAL;
	}

	wsm_read_mib(hw_priv, WSM_MIB_ID_GET_TX_LATENCY, &tx_laten_l2, sizeof(unsigned short), priv->if_id);
//	ptr = wdata->data.pointer;

	idle = g_ch_idle_ratio;

	//wsm_read_mib(hw_priv, WSM_MIB_ID_GET_CHANNEL_IDLE, &idle, sizeof(unsigned short), priv->if_id);

	atbm_printk_err("current_idle:%d, laten:%d\n", idle, tx_laten_l2);
	//memcpy(extra, (char *)&idle, sizeof(unsigned short));
/*	
	if(copy_to_user(ptr + user_len, (char *)&idle, sizeof(unsigned short)) != 0)
		return -EINVAL;
*/
	if(extra){
		sprintf(extra,"current_idle:%u tx_laten_l2:%u\n", idle, tx_laten_l2);
		//memcpy((char *)extra, (char *)&idle,sizeof(unsigned short));
		wdata->data.length = strlen(extra);
	}
	
	return ret;
}
//#endif
static int atbm_ioctl_get_efuse(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	int i,num = 0,ret = -EINVAL;
	char *extra = NULL;
	char *type_val = NULL;
	char *pflag = NULL;
	int data_flag = 1;//default efuse data
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct efuse_headr efuse_data;

	memset(&efuse_data, 0, sizeof(efuse_data));
//	memset(&hw_priv->efuse,0, sizeof(struct efuse_headr));

	if(!(extra = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		return -EINVAL;
	}
	if((ret = copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(extra);
		return -EINVAL;
	}
	extra[wrqu->data.length] = 0;

	for(i=0;i<wrqu->data.length;i++){
		 if((extra[i] == ',') && (num == 0)){
			type_val= extra + i + 1;
			num++;
		}
		else if((extra[i] == ',') && (num == 1)){
			pflag = extra + i + 1;
			extra[i] = '\0';
			num++;
			data_flag = *pflag - '0';
			break;
		}
	}

	if ((ret = wsm_get_efuse_data(hw_priv, &efuse_data, sizeof(efuse_data))) == 0){	
		atbm_printk_init("Get efuse data is [%d,%d,%d,%d,%d,%d,%d,%d,%02x:%02x:%02x:%02x:%02x:%02x]\n",
				efuse_data.version,efuse_data.dcxo_trim,efuse_data.delta_gain1,efuse_data.delta_gain2,efuse_data.delta_gain3,
				efuse_data.Tj_room,efuse_data.topref_ctrl_bias_res_trim,efuse_data.PowerSupplySel,efuse_data.mac[0],efuse_data.mac[1],
				efuse_data.mac[2],efuse_data.mac[3],efuse_data.mac[4],efuse_data.mac[5]);
		if(hw_priv->chip_version == OCEANUS)
		{
			atbm_printk_init("5G deltagain [%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]\n", efuse_data.delta_gain1_5g,efuse_data.delta_gain2_5g,
			efuse_data.delta_gain3_5g,efuse_data.delta_gain4_5g,efuse_data.delta_gain5_5g,efuse_data.delta_gain6_5g,efuse_data.delta_gain7_5g,
			efuse_data.delta_gain8_5g,efuse_data.delta_gain9_5g,efuse_data.delta_gain10_5g);
			if(efuse_data.version == 3)
				atbm_printk_init("pdetK [%d,%d,%d]\n", efuse_data.pdetK1, efuse_data.pdetK2, efuse_data.pdetK3);
		}
		memcpy(&hw_priv->efuse, &efuse_data, sizeof(struct efuse_headr));
	}
	else{
		atbm_printk_err("read efuse failed\n");
	}

	//atbm_printk_always("type_val:%s,data_flag:%d\n", type_val, data_flag);
	if((data_flag == 0) && (!memcmp(type_val, "dcxo", 4)))
	{
		efuse_data.dcxo_trim = atbm_DCXOCodeRead(hw_priv);
		atbm_printk_always("[DCXO_TRIM_REG]efuse_data.dcxo_trim:%d\n", efuse_data.dcxo_trim);
	}
/*
	if(copy_to_user(wdata->data.pointer, &efuse_data, sizeof(efuse_data)) != 0){
		ret = -EINVAL;
		atbm_printk_wext("copy to user failed.\n");
	}
*/
	if(ext){
		sprintf(ext,"\nCurrentEfuse:\ndcxo:%d\ngain1:%d\ngain2:%d\ngain3:%d\nmac:"MACSTR"\n",
			efuse_data.dcxo_trim,efuse_data.delta_gain1,
			efuse_data.delta_gain2,efuse_data.delta_gain3,MAC2STR(efuse_data.mac));
		//memcpy(ext, &efuse_data,sizeof(efuse_data));
		wdata->data.length = strlen(ext);
	}

	atbm_kfree(extra);
	return ret;
}
static int atbm_ioctl_get_efuse_free_space(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{

	//int ret = -EINVAL;
	//char *extra = NULL;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	int efuse_remainbit = 0;
	efuse_remainbit = wsm_get_efuse_status(hw_priv, NULL);
	atbm_printk_err("efuse free space:[%d] bit \n", efuse_remainbit);
	
	if(ext){
		sprintf(ext,"\nefuse_free_space:%dbit\n",efuse_remainbit);
		//memcpy(ext, &efuse_remainbit,sizeof(efuse_remainbit));
		wdata->data.length = strlen(ext);
	}

	return 0;
}
static int atbm_ioctl_get_efuse_first(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	int ret = -EINVAL;
	//union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct efuse_headr efuse_data;

	memset(&efuse_data,0, sizeof(struct efuse_headr));
	if ((ret = wsm_get_efuse_first_data(hw_priv, (void *)&efuse_data, sizeof(struct efuse_headr))) == 0){	
		atbm_printk_always("efuse first data is [%d,%d,%d,%d,%d,%d,%d,%d,%02x:%02x:%02x:%02x:%02x:%02x]\n",
				efuse_data.version,efuse_data.dcxo_trim,efuse_data.delta_gain1,efuse_data.delta_gain2,efuse_data.delta_gain3,
				efuse_data.Tj_room,efuse_data.topref_ctrl_bias_res_trim,efuse_data.PowerSupplySel,efuse_data.mac[0],efuse_data.mac[1],
				efuse_data.mac[2],efuse_data.mac[3],efuse_data.mac[4],efuse_data.mac[5]);
		if(hw_priv->chip_version == OCEANUS)
		{
			atbm_printk_init("5G deltagain [%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]\n", efuse_data.delta_gain1_5g,efuse_data.delta_gain2_5g,
			efuse_data.delta_gain3_5g,efuse_data.delta_gain4_5g,efuse_data.delta_gain5_5g,efuse_data.delta_gain6_5g,efuse_data.delta_gain7_5g,
			efuse_data.delta_gain8_5g,efuse_data.delta_gain9_5g,efuse_data.delta_gain10_5g);
			if(efuse_data.version == 3)
				atbm_printk_init("pdetK [%d,%d,%d]\n", efuse_data.pdetK1, efuse_data.pdetK2, efuse_data.pdetK3);
		}
	}
	else{
		atbm_printk_err("read efuse failed\n");
	}
	if(ext){
			sprintf(ext,"\nFirstEfuse:\ndcxo:%d\ngain1:%d\ngain2:%d\ngain3:%d\nmac:"MACSTR"\n",efuse_data.dcxo_trim,efuse_data.delta_gain1,
				efuse_data.delta_gain2,efuse_data.delta_gain3,MAC2STR(efuse_data.mac));
			//memcpy(ext, &efuse_data,sizeof(efuse_data));
			wrqu->data.length = strlen(ext);
		}

	return ret;
}

static int atbm_ioctl_get_efuse_all_data(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	int ret = -EINVAL,i = 0;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	u8 buffer[128] = {0};

	memset(buffer,0, sizeof(buffer));
	if ((ret = wsm_get_efuse_all_data(hw_priv, (void *)&buffer, sizeof(buffer))) == 0){	
		for(i=0;i<109;i++)
			atbm_printk_err("%02x ",buffer[i]);
	}
	else{
		atbm_printk_err("read all efuse failed\n");
	}

	return ret;
}


static int atbm_ioctl_set_efuse(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	int i = 0;
	int ret = -EINVAL;
	char *extra = NULL;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	int strheadLen = 0;
	char *pRxData;
	int rxData;
	char cmd[50] = "";
	struct efuse_headr efuse_data;
	int writeEfuseFlag = 0;//
	int efuse_remainbit = 0;
	int set_dcxo_flag = 0, set_deltagain_flag = 0;
	int deltagainMax = 31;
	
	atbm_printk_wext("####### efuse\n");
	if(!(extra = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		return -EINVAL;
	}
	if((ret = copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(extra);
		return -EINVAL;
	}
	extra[wrqu->data.length] = 0;

	for(i=0;i<wrqu->data.length;i++)
	{
		if(extra[i] == ',')
		{
			strheadLen=i;
			memcpy(cmd, extra, i);
			break;
		}
	}

	atbm_printk_err("cmd:%s\n", cmd);
	
	atbm_printk_err("length=%d,data=%s,strheadLen=%d\n", wrqu->data.length, extra, strheadLen);
	if (strheadLen >= wrqu->data.length){
		atbm_kfree(extra);
		return -EINVAL;
	}

	memset(&efuse_data, 0, sizeof(efuse_data));
	if ((ret = wsm_get_efuse_data(hw_priv, &efuse_data, sizeof(efuse_data))) == 0){
		memcpy(&hw_priv->efuse, &efuse_data, sizeof(struct efuse_headr));
	}
	else{
		atbm_printk_err("read efuse failed\n");
	}

	
	if(hw_priv->chip_version == OCEANUS)
		deltagainMax = 63;
	

	pRxData = &extra[strheadLen];
	atbm_printk_err("cmd:%s\n", pRxData);

	efuse_remainbit = wsm_get_efuse_status(hw_priv, NULL);
	atbm_printk_err("##before write efuse_remainbit:%d##\n", efuse_remainbit);

	if(memcmp(cmd, "set_efuse_dcxo", 14) == 0 || memcmp(cmd, "setEfuse_dcxo", 13) == 0)
	{
		atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
		hw_priv->efuse.dcxo_trim = rxData;
		if(rxData < 0 || rxData > 127){
			atbm_printk_err("ERR!!!  efuse_dcxo range is 0~127,now input value is [%d] \n",rxData);
			return -EINVAL;
			goto efuse_err;
		}
		atbm_CmdLine_GetHex(&pRxData, &writeEfuseFlag);
		set_dcxo_flag = 1;
		atbm_printk_err("set efuse data is dcxo[%d]\n",hw_priv->efuse.dcxo_trim);
	}
	else if(memcmp(cmd, "set_efuse_deltagain", 19) == 0 || memcmp(cmd, "setEfuse_deltagain", 19) == 0)
	{
		atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
		hw_priv->efuse.delta_gain1 = rxData;
		if(rxData < 0 || rxData > deltagainMax){
			atbm_printk_err("ERR!!!  efuse delta_gain1 range is 0~31,now input value is [%d] \n",rxData);
			return -EINVAL;
			goto efuse_err;
		}
		//atbm_printk_err("%s %d\n", __func__, __LINE__);
		atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
		hw_priv->efuse.delta_gain2 = rxData;
		if(rxData < 0 || rxData > deltagainMax){
			atbm_printk_err("ERR!!!  efuse delta_gain2 range is 0~31,now input value is [%d] \n",rxData);
			return -EINVAL;
			goto efuse_err;
		}
		//atbm_printk_err("%s %d\n", __func__, __LINE__);
		atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
		hw_priv->efuse.delta_gain3 = rxData;
		if(rxData < 0 || rxData > deltagainMax){
			atbm_printk_err("ERR!!!  efuse delta_gain3 range is 0~31,now input value is [%d] \n",rxData);
			return -EINVAL;
			goto efuse_err;
		}

		atbm_CmdLine_GetHex(&pRxData, &writeEfuseFlag);
		set_deltagain_flag = 1;
		atbm_printk_err("set efuse data is delta_gain[%d,%d,%d]\n",
			hw_priv->efuse.delta_gain1,hw_priv->efuse.delta_gain2,hw_priv->efuse.delta_gain3);
	}
	else if(memcmp(cmd, "setEfuse_5gdeltagain", 20) == 0)
	{
		if(hw_priv->chip_version != OCEANUS){
			atbm_printk_err("Not support\n");
			return -EINVAL;
			goto efuse_err;
		}
		atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
		if((rxData < 0) || (rxData > deltagainMax)){
			atbm_printk_err("invalid delta_gain1_5g:%d\n", rxData);
			goto efuse_err;
		}
		hw_priv->efuse.delta_gain1_5g = rxData;
		atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
		if((rxData < 0) || (rxData > deltagainMax)){
			atbm_printk_err("invalid delta_gain2_5g:%d\n", rxData);
			goto efuse_err;
		}
		hw_priv->efuse.delta_gain2_5g = rxData;
		atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
		if((rxData < 0) || (rxData > deltagainMax)){
			atbm_printk_err("invalid delta_gain3_5g:%d\n", rxData);
			goto efuse_err;
		}
		hw_priv->efuse.delta_gain3_5g = rxData;
		atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
		if((rxData < 0) || (rxData > deltagainMax)){
			atbm_printk_err("invalid delta_gain4_5g:%d\n", rxData);
			goto efuse_err;
		}
		hw_priv->efuse.delta_gain4_5g = rxData;
		atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
		if((rxData < 0) || (rxData > deltagainMax)){
			atbm_printk_err("invalid delta_gain5_5g:%d\n", rxData);
			goto efuse_err;
		}
		hw_priv->efuse.delta_gain5_5g = rxData;
		atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
		if((rxData < 0) || (rxData > deltagainMax)){
			atbm_printk_err("invalid delta_gain6_5g:%d\n", rxData);
			goto efuse_err;
		}
		hw_priv->efuse.delta_gain6_5g = rxData;
		atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
		if((rxData < 0) || (rxData > deltagainMax)){
			atbm_printk_err("invalid delta_gain7_5g:%d\n", rxData);
			goto efuse_err;
		}
		hw_priv->efuse.delta_gain7_5g = rxData;
		atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
		if((rxData < 0) || (rxData > deltagainMax)){
			atbm_printk_err("invalid delta_gain8_5g:%d\n", rxData);
			goto efuse_err;
		}
		hw_priv->efuse.delta_gain8_5g = rxData;
		atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
		if((rxData < 0) || (rxData > deltagainMax)){
			atbm_printk_err("invalid delta_gain9_5g:%d\n", rxData);
			goto efuse_err;
		}
		hw_priv->efuse.delta_gain9_5g = rxData;
		atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
		if((rxData < 0) || (rxData > deltagainMax)){
			atbm_printk_err("invalid delta_gain10_5g:%d\n", rxData);
			goto efuse_err;
		}
		hw_priv->efuse.delta_gain10_5g = rxData;
		
		atbm_CmdLine_GetHex(&pRxData, &writeEfuseFlag);
		set_deltagain_flag = 0x02;
		atbm_printk_err("set efuse data is delta_gain 5g[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]\n",
			hw_priv->efuse.delta_gain1_5g,hw_priv->efuse.delta_gain2_5g,hw_priv->efuse.delta_gain3_5g,hw_priv->efuse.delta_gain4_5g,hw_priv->efuse.delta_gain5_5g,
			hw_priv->efuse.delta_gain6_5g,hw_priv->efuse.delta_gain7_5g,hw_priv->efuse.delta_gain8_5g,hw_priv->efuse.delta_gain9_5g,hw_priv->efuse.delta_gain10_5g);
	}
	else if(memcmp(cmd, "setEfuse_tjroom", 15) == 0)
	{
		atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
		hw_priv->efuse.Tj_room= rxData;
		if(rxData < 0 || rxData > 127){
			atbm_printk_err("ERR!!!  efuse_Tj_room range is 0~127,now input value is [%d] \n",rxData);
			return -EINVAL;
			goto efuse_err;
		}
		atbm_CmdLine_GetHex(&pRxData, &writeEfuseFlag);
		atbm_printk_err("set efuse data is Tj_room[%d]\n",hw_priv->efuse.Tj_room);
	}
	else if(memcmp(cmd, "set_efuse_mac", 13) == 0 || memcmp(cmd, "setEfuse_mac", 12) == 0)
	{ 
		atbm_CmdLine_GetHex(&pRxData, &rxData);
		hw_priv->efuse.mac[0] = rxData;
		//atbm_printk_err("%s %d\n", __func__, __LINE__);
		atbm_CmdLine_GetHex(&pRxData, &rxData);
		hw_priv->efuse.mac[1] = rxData;
		//atbm_printk_err("%s %d\n", __func__, __LINE__);
		atbm_CmdLine_GetHex(&pRxData, &rxData);
		hw_priv->efuse.mac[2] = rxData;
		//atbm_printk_err("%s %d\n", __func__, __LINE__);
		atbm_CmdLine_GetHex(&pRxData, &rxData);
		hw_priv->efuse.mac[3] = rxData;
		//atbm_printk_err("%s %d\n", __func__, __LINE__);
		atbm_CmdLine_GetHex(&pRxData, &rxData);
		hw_priv->efuse.mac[4] = rxData;
		//atbm_printk_err("%s %d\n", __func__, __LINE__);
		atbm_CmdLine_GetHex(&pRxData, &rxData);
		hw_priv->efuse.mac[5] = rxData;
		writeEfuseFlag = 1;
		atbm_printk_err("set efuse data is mac[%02x:%02x:%02x:%02x:%02x:%02x]\n",
					hw_priv->efuse.mac[0],hw_priv->efuse.mac[1],hw_priv->efuse.mac[2],
					hw_priv->efuse.mac[3],hw_priv->efuse.mac[4],hw_priv->efuse.mac[5]);
	}

	if(set_deltagain_flag)
	{
		if(hw_priv->chip_id != HW_CHIP_VERSION_Cronus)
		{
		    u32 i;
		    u8 delta_gains[11] = {0};
			//0:not use deltagain and tpc
   			//1:use deltagain and use tpc
    		//2:not use deltagain and use tpc
    		//3:use deltagain and not use tpc
			int use_deltagain_and_tpc = 2;
			//Cronuslite、Oceanus 停止固件根据信道设置deltagain，否则驱动设置的寄存器值会被固件刷掉
			ret = wsm_write_mib(hw_priv, WSM_MIB_ID_TX_HOST_CTRL_DELTAGAIN_AND_TPC, &use_deltagain_and_tpc, sizeof(use_deltagain_and_tpc), 0);
			if(ret != 0){
				atbm_printk_err("write mib failed(%d)\n", ret);
			}
            i = 0;
            delta_gains[i++] = set_deltagain_flag; //1: 2.4G; 2: 5G; 3: 2.4G&5G
            if(set_deltagain_flag & 0x01)
            {//2.4G
                delta_gains[i++] = hw_priv->efuse.delta_gain1;
                delta_gains[i++] = hw_priv->efuse.delta_gain2;
                delta_gains[i++] = hw_priv->efuse.delta_gain3;
            }
            else /*if(set_deltagain_flag & 0x02)*/
            {//5G
                delta_gains[i++] = hw_priv->efuse.delta_gain1_5g;
                delta_gains[i++] = hw_priv->efuse.delta_gain2_5g;
                delta_gains[i++] = hw_priv->efuse.delta_gain3_5g;
                delta_gains[i++] = hw_priv->efuse.delta_gain4_5g;
                delta_gains[i++] = hw_priv->efuse.delta_gain5_5g;
                delta_gains[i++] = hw_priv->efuse.delta_gain6_5g;
                delta_gains[i++] = hw_priv->efuse.delta_gain7_5g;
                delta_gains[i++] = hw_priv->efuse.delta_gain8_5g;
                delta_gains[i++] = hw_priv->efuse.delta_gain9_5g;
                delta_gains[i++] = hw_priv->efuse.delta_gain10_5g;
            }
			ret = wsm_write_mib(hw_priv, WSM_MIB_ID_USR_TX_DELTA_GAIN_CONFIG, (void *)delta_gains, (size_t)i, 0);
			if(ret != 0){
				atbm_printk_err("write mib(0x%X) failed(%d)\n",WSM_MIB_ID_USR_TX_DELTA_GAIN_CONFIG, ret);
			}
		}
	}

	if(writeEfuseFlag)
	{

		if(set_dcxo_flag)
		{
			atbm_printk_err("set dcxo effect immediately && SET TO EFUSE\n");
			atbm_DCXOCodeWrite(hw_priv, hw_priv->efuse.dcxo_trim);
		}
		if(set_deltagain_flag)
		{
			atbm_printk_err("set deltagin effect immediately && SET TO EFUSE\n");
			if(hw_priv->chip_id == HW_CHIP_VERSION_Cronus)
				atbm_etf_set_deltagain(hw_priv->efuse);	
		}
		
		ret = atbm_save_efuse(hw_priv, &hw_priv->efuse);
		if (ret == 0)
		{
			atbm_printk_err("setEfuse success \n");
		}else
		{
			atbm_printk_err("setEfuse failed [%d]\n", ret);
		}
		efuse_remainbit = wsm_get_efuse_status(hw_priv, NULL);
		atbm_printk_err("##after write efuse_remainbit:%d##\n", efuse_remainbit);
	}
	else
	{
		if(set_dcxo_flag)
		{
			atbm_printk_err("set dcxo effect immediately \n");
			atbm_DCXOCodeWrite(hw_priv, hw_priv->efuse.dcxo_trim);
		}
		else
		{
			atbm_printk_err("set deltagain effect immediately \n");
			if(hw_priv->chip_id == HW_CHIP_VERSION_Cronus)
				atbm_etf_set_deltagain(hw_priv->efuse);
		}
	}
efuse_err:
	atbm_kfree(extra);

	return ret;
}
#ifdef CONFIG_ATBM_SUPPORT_BLUEZ
extern int ieee80211_ble_bluez_send_frame(
			struct hci_dev *hdev,struct sk_buff *skb);
extern struct hci_dev *f_hdev;
static int atbm_ioctl_set_ble_pub_mac(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	int i = 0;
	int strheadLen = 0;
	int ret = -EINVAL;
	atbm_printk_err("atbm_ioctl_set_ble_pub_mac\n");
	char cmd[50] = "";
	char *pRxData = NULL;
	char *extra = NULL;
	int rxData = 0;
	u8 ble_pub_mac[9];
	struct sk_buff *skb;
	if(!(extra = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		return -EINVAL;
	}
	if((ret = copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(extra);
		return -EINVAL;
	}
	extra[wrqu->data.length] = 0;

	for(i=0;i<wrqu->data.length;i++)
	{
		if(extra[i] == ',')
		{
			strheadLen=i;
			memcpy(cmd, extra, i);
			break;
		}
	}

	atbm_printk_err("cmd:%s\n", cmd);
	pRxData = &extra[strheadLen];
	atbm_printk_err("cmd:%s\n", pRxData);
	if(memcmp(cmd, "setble_pub_mac", 14) == 0)
	{
		ble_pub_mac[0]=0x08;
		ble_pub_mac[1]=0xff;
		ble_pub_mac[2]=0x06;
		atbm_CmdLine_GetHex(&pRxData, &rxData);
		ble_pub_mac[8] = rxData;
		atbm_CmdLine_GetHex(&pRxData, &rxData);
		ble_pub_mac[7] = rxData;
		atbm_CmdLine_GetHex(&pRxData, &rxData);
		ble_pub_mac[6] = rxData;
		atbm_CmdLine_GetHex(&pRxData, &rxData);
		ble_pub_mac[5] = rxData;
		atbm_CmdLine_GetHex(&pRxData, &rxData);
		ble_pub_mac[4] = rxData;
		atbm_CmdLine_GetHex(&pRxData, &rxData);
		ble_pub_mac[3] = rxData;

		atbm_printk_err("set ble pub mac[%02x:%02x:%02x:%02x:%02x:%02x]\n",
					ble_pub_mac[8],ble_pub_mac[7],ble_pub_mac[6],
					ble_pub_mac[5],ble_pub_mac[4],ble_pub_mac[3]);

    // 分配一个新的 sk_buff
    skb = bt_skb_alloc(sizeof(ble_pub_mac), GFP_KERNEL);
    if (!skb) {
        printk(KERN_ERR "Failed to allocate skb\n");
        return -ENOMEM;
    }
	bt_cb(skb)->pkt_type=HCI_COMMAND_PKT;
    // 复制广播数据到 sk_buff 数据部分
    memcpy(skb_put(skb, sizeof(ble_pub_mac)), ble_pub_mac, sizeof(ble_pub_mac));
	int i=0;
	for(i=0;i<9;i++)
	{
		atbm_printk_err("-0x%x\n",skb->data[i]);

	}

    skb->len =  sizeof(ble_pub_mac);
    // 发送 HCI 命令
    ret = ieee80211_ble_bluez_send_frame(f_hdev, skb);
    if (ret < 0) {
        pr_err("Failed to send HCI command: %d\n", ret);
        kfree_skb(skb);
        return ret;
    }

    pr_info("set ble pub mac successfully\n");
	}
	return ret;
}
#endif
static int atbm_ioctl_set_all_efuse(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	int i = 0;
	int ret = -EINVAL;
	char *extra = NULL;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	int strheadLen = 0;
	char *pRxData;
	int rxData;
	char cmd[50] = "";
	int efuse_remainbit = 0;
	struct efuse_headr efuse_data;
	int deltagainMax = 31;
	
	//struct Tjroom_temperature_t tjroom_temp;
#if 1
	atbm_printk_wext("####### efuse\n");
	if(!(extra = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		return -EINVAL;
	}
	if((ret = copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(extra);
		return -EINVAL;
	}
	extra[wrqu->data.length] = 0;

	for(i=0;i<wrqu->data.length;i++)
	{
		if(extra[i] == ',')
		{
			strheadLen=i;
			memcpy(cmd, extra, i);
			break;
		}
	}

	atbm_printk_err("cmd:%s\n", cmd);
	
	atbm_printk_err("length=%d,data=%s,strheadLen=%d\n", wrqu->data.length, extra, strheadLen);
	if (strheadLen >= wrqu->data.length){
		atbm_kfree(extra);
		return -EINVAL;
	}
	
	memset(&efuse_data, 0, sizeof(efuse_data));
	if ((ret = wsm_get_efuse_data(hw_priv, &efuse_data, sizeof(efuse_data))) == 0){
		memcpy(&hw_priv->efuse, &efuse_data, sizeof(struct efuse_headr));
	}
	else{
		atbm_printk_err("read efuse failed\n");
	}

	if(hw_priv->chip_version == OCEANUS)
		deltagainMax = 63;	

	pRxData = &extra[strheadLen];
	atbm_printk_err("cmd:%s\n", pRxData);

	
//-------------------dcxo
	atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
	hw_priv->efuse.dcxo_trim = rxData;
	if((rxData < 0) || (rxData > 127))
	{
		atbm_printk_err("invalid dcxo:%d\n", rxData);
		goto error;
	}
	atbm_printk_err("set efuse data is dcxo[%d]\n",hw_priv->efuse.dcxo_trim);

	//-------------------2.4G delta gain
	atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
	hw_priv->efuse.delta_gain1 = rxData;
	if((rxData < 0) || (rxData > deltagainMax))
	{
		atbm_printk_err("invalid delta_gain1:%d\n", rxData);
		goto error;
	}
	atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
	hw_priv->efuse.delta_gain2 = rxData;
	if((rxData < 0) || (rxData > deltagainMax))
	{
		atbm_printk_err("invalid delta_gain2:%d\n", rxData);
		goto error;
	}
	atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
	hw_priv->efuse.delta_gain3 = rxData;
	if((rxData < 0) || (rxData > deltagainMax))
	{
		atbm_printk_err("invalid delta_gain3:%d\n", rxData);
		goto error;
	}
	atbm_printk_err("set efuse data is delta_gain[%d,%d,%d]\n",
		hw_priv->efuse.delta_gain1,hw_priv->efuse.delta_gain2,hw_priv->efuse.delta_gain3);

	//-------------------Tjroom
	atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
	hw_priv->efuse.Tj_room = rxData;
	if((rxData < 0) || (rxData > 127))
	{
		atbm_printk_err("invalid Tj_room:%d\n", rxData);
		goto error;
	}
	atbm_printk_err("set efuse data is Tj_room[%d]\n",hw_priv->efuse.dcxo_trim);

	//-------------------MAC
	atbm_CmdLine_GetHex(&pRxData, &rxData);
	hw_priv->efuse.mac[0] = rxData;
	
	atbm_CmdLine_GetHex(&pRxData, &rxData);
	hw_priv->efuse.mac[1] = rxData;

	atbm_CmdLine_GetHex(&pRxData, &rxData);
	hw_priv->efuse.mac[2] = rxData;

	atbm_CmdLine_GetHex(&pRxData, &rxData);
	hw_priv->efuse.mac[3] = rxData;

	atbm_CmdLine_GetHex(&pRxData, &rxData);
	hw_priv->efuse.mac[4] = rxData;

	atbm_CmdLine_GetHex(&pRxData, &rxData);
	hw_priv->efuse.mac[5] = rxData;

	atbm_printk_err("set efuse data is mac[%02x:%02x:%02x:%02x:%02x:%02x]\n",
				hw_priv->efuse.mac[0],hw_priv->efuse.mac[1],hw_priv->efuse.mac[2],
				hw_priv->efuse.mac[3],hw_priv->efuse.mac[4],hw_priv->efuse.mac[5]);
	//-------------------5G delta gain	
	if(hw_priv->chip_version == OCEANUS)
	{
		//5G deltagain
		if(strlen(pRxData) >= 19){
			atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
			if((rxData < 0) || (rxData > deltagainMax)){
				atbm_printk_err("invalid delta_gain1_5g:%d\n", rxData);
				goto error;
			}
			hw_priv->efuse.delta_gain1_5g = rxData;
			atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
			if((rxData < 0) || (rxData > deltagainMax)){
				atbm_printk_err("invalid delta_gain2_5g:%d\n", rxData);
				goto error;
			}
			hw_priv->efuse.delta_gain2_5g = rxData;
			atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
			if((rxData < 0) || (rxData > deltagainMax)){
				atbm_printk_err("invalid delta_gain3_5g:%d\n", rxData);
				goto error;
			}
			hw_priv->efuse.delta_gain3_5g = rxData;
			atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
			if((rxData < 0) || (rxData > deltagainMax)){
				atbm_printk_err("invalid delta_gain4_5g:%d\n", rxData);
				goto error;
			}
			hw_priv->efuse.delta_gain4_5g = rxData;
			atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
			if((rxData < 0) || (rxData > deltagainMax)){
				atbm_printk_err("invalid delta_gain5_5g:%d\n", rxData);
				goto error;
			}
			hw_priv->efuse.delta_gain5_5g = rxData;
			atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
			if((rxData < 0) || (rxData > deltagainMax)){
				atbm_printk_err("invalid delta_gain6_5g:%d\n", rxData);
				goto error;
			}
			hw_priv->efuse.delta_gain6_5g = rxData;
			atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
			if((rxData < 0) || (rxData > deltagainMax)){
				atbm_printk_err("invalid delta_gain7_5g:%d\n", rxData);
				goto error;
			}
			hw_priv->efuse.delta_gain7_5g = rxData;
			atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
			if((rxData < 0) || (rxData > deltagainMax)){
				atbm_printk_err("invalid delta_gain8_5g:%d\n", rxData);
				goto error;
			}
			hw_priv->efuse.delta_gain8_5g = rxData;
			atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
			if((rxData < 0) || (rxData > deltagainMax)){
				atbm_printk_err("invalid delta_gain9_5g:%d\n", rxData);
				goto error;
			}
			hw_priv->efuse.delta_gain9_5g = rxData;
			atbm_CmdLine_GetSignInteger(&pRxData, &rxData);
			if((rxData < 0) || (rxData > deltagainMax)){
				atbm_printk_err("invalid delta_gain10_5g:%d\n", rxData);
				goto error;
			}
			hw_priv->efuse.delta_gain10_5g = rxData;
		}
	}

	
	atbm_printk_err("efuse data:[%d,%d,%d,%d,%d,%d,%d,%d,%02x:%02x:%02x:%02x:%02x:%02x]\n",
		hw_priv->efuse.version,hw_priv->efuse.dcxo_trim,
		hw_priv->efuse.delta_gain1,hw_priv->efuse.delta_gain2,hw_priv->efuse.delta_gain3,
		hw_priv->efuse.Tj_room,hw_priv->efuse.PowerSupplySel,hw_priv->efuse.topref_ctrl_bias_res_trim,
		hw_priv->efuse.mac[0],hw_priv->efuse.mac[1],hw_priv->efuse.mac[2],
		hw_priv->efuse.mac[3],hw_priv->efuse.mac[4],hw_priv->efuse.mac[5]);

	if(hw_priv->chip_version == OCEANUS)
		atbm_printk_init("5G deltagain [%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]\n", hw_priv->efuse.delta_gain1_5g,hw_priv->efuse.delta_gain2_5g,
		hw_priv->efuse.delta_gain3_5g,hw_priv->efuse.delta_gain4_5g,hw_priv->efuse.delta_gain5_5g,hw_priv->efuse.delta_gain6_5g,
		hw_priv->efuse.delta_gain7_5g,hw_priv->efuse.delta_gain8_5g,hw_priv->efuse.delta_gain9_5g,hw_priv->efuse.delta_gain10_5g);

	ret = atbm_save_efuse(hw_priv, &hw_priv->efuse);
	if (ret == 0)
	{
		atbm_printk_err("setEfuse success \n");
	}else
	{
		atbm_printk_err("setEfuse failed [%d]\n", ret);
	}
	efuse_remainbit = wsm_get_efuse_status(hw_priv, NULL);
	atbm_printk_err("##after write efuse_remainbit:%d##\n", efuse_remainbit);
#endif
error:	
	atbm_kfree(extra);

	return ret;
}

static int atbm_ioctl_get_Tjroom(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	int ret = -EINVAL;
	char *pbuff = NULL;
	//union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct Tjroom_temperature_t tjroom_temp;

	if(!(pbuff = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		return -EINVAL;
	}

	memset(&tjroom_temp, 0, sizeof(tjroom_temp));

	if((ret = wsm_get_Tjroom_temperature(hw_priv, &tjroom_temp, sizeof(tjroom_temp))) == 0)
	{
		atbm_printk_always("Tjroom:%d, treading:%d,stempC:%d\n", tjroom_temp.Tjroom,
			tjroom_temp.tempC,tjroom_temp.stempC);
	}
	else
	{
		atbm_printk_err("get Tjroom failed\n");
	}

	sprintf(pbuff, "Tjroom:%d, treading:%d,stempC:%d\n", tjroom_temp.Tjroom,tjroom_temp.tempC,tjroom_temp.stempC);

	if(extra){
		sprintf(extra,"\nsTjroom:%d\ntreading:%d\nstempC:%d\n", tjroom_temp.Tjroom,tjroom_temp.tempC,tjroom_temp.stempC);
		//memcpy(ext, &efuse_data,sizeof(efuse_data));
		wrqu->data.length = strlen(extra);
	}

	atbm_kfree(pbuff);
	
	return ret;
}

static int atbm_ioctl_set_calibrate_flag(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	int i = 0;
	int ret = -EINVAL;
	char *extra = NULL;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	int strheadLen = 0;
	char *pRxData;
	int caliFlag;
	char cmd[50] = "";
	char writebuf[128] = "";

	if(!(extra = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		return -EINVAL;
	}
	if((ret = copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(extra);
		return -EINVAL;
	}
	extra[wrqu->data.length] = 0;

	for(i=0;i<wrqu->data.length;i++)
	{
		if(extra[i] == ',')
		{
			strheadLen=i;
			memcpy(cmd, extra, i);
			break;
		}
	}

	atbm_printk_err("cmd:%s\n", cmd);
	
	atbm_printk_err("length=%d,data=%s,strheadLen=%d\n", wrqu->data.length, extra, strheadLen);
	if (strheadLen >= wrqu->data.length){
		atbm_kfree(extra);
		return -EINVAL;
	}

	pRxData = &extra[strheadLen];
	atbm_printk_err("cmd:%s\n", pRxData);

	atbm_CmdLine_GetSignInteger(&pRxData, &caliFlag);

	if((caliFlag != 0) && (caliFlag != 1))
	{
		atbm_printk_err("Invalid parameter:%s\n", pRxData);
		return -EINVAL;
	}

	if(caliFlag == 1)
	{
		memset(writebuf, 0, sizeof(writebuf));
		sprintf(writebuf, "set_cali_flag,%d ", caliFlag);		
		atbm_printk_init("cmd: %s\n", writebuf);	
	}
	else
	{
		memset(writebuf, 0, sizeof(writebuf));
		sprintf(writebuf, "set_cali_flag,%d ", caliFlag);			
		atbm_printk_init("cmd: %s\n", writebuf);			
	}
	ret = wsm_write_mib(hw_priv, WSM_MIB_ID_FW_CMD, writebuf, strlen(writebuf), 0);
	if(ret < 0){
		atbm_printk_err("%s: write mib failed(%d). \n",__func__, ret);
	}
			
	atbm_kfree(extra);

	return ret;
}


#ifdef CONFIG_ATBM_IWPRIV_USELESS
static int atbm_ioctl_freqoffset(struct net_device *dev, struct iw_request_info *info, union iwreq_data  *wrqu, char *extra)
{
	int i = 0;	
	int ret = 0;
	int iResult=0;
	
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	//struct atbm_vif *vif;
	struct efuse_headr efuse_d,efuse_bak;
	u32 dcxo;
	int freqErrorHz;
	int ucWriteEfuseFlag = 0;
	int channel = 0;
	char *SavaEfuse_p = NULL;
	
	u8 buff[512];
	struct rxstatus_signed rxs_s;

	memset(&rxs_s,0,sizeof(struct rxstatus));
	memset(&efuse_d,0,sizeof(struct efuse_headr));
	memset(&efuse_bak,0,sizeof(struct efuse_headr));

	if(wrqu->data.length <= 3){
		atbm_printk_err("need to input parameters\n");
		atbm_printk_err("e.g: ./iwpriv wlan0 start_rx 1,0\n");
		return -EINVAL;
	}

	if(!(extra = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL)))
		return -EINVAL;

	if((ret = copy_from_user(extra, wrqu->data.pointer, wrqu->data.length)) != 0){
		atbm_kfree(extra);
		return -EINVAL;
	}

	for(i=0;extra[i] != ',';i++){
		if(extra[i] == ','){
			break;
		}
		channel = channel * 10 +(extra[i] - 0x30);
	}

	for(i=0;i<wrqu->data.length;i++){
			if(extra[i] == ','){
				SavaEfuse_p = extra +i + 1;
				break;
			}	
		}

	if((NULL == SavaEfuse_p) || (SavaEfuse_p[0] == '\0')){
		atbm_printk_err("invalid SavaEfuse_p\n");
		atbm_printk_err("ucWriteEfuseFlag value: 1:save efuse,0:not save efuse\n");
		atbm_kfree(extra);
		return -EINVAL;
	}

	for(i=0;SavaEfuse_p[i] != '\0';i++){
		if(1 == i){
		atbm_printk_err("invalid SavaEfuse_p1\n");
		atbm_printk_err("ucWriteEfuseFlag value: 1:save efuse,0:not save efuse\n");
		atbm_kfree(extra);
			return 	-EINVAL;
		}
		ucWriteEfuseFlag = ucWriteEfuseFlag * 10 + (SavaEfuse_p[i] - 0x30);
	}

	atbm_printk_wext("channel:%d ucWriteEfuseFlag:%d\n",channel, ucWriteEfuseFlag);

	
	if((ucWriteEfuseFlag != 0) && (ucWriteEfuseFlag != 1)){
		atbm_printk_err("invalid WriteEfuseFlag\n");
		atbm_kfree(extra);
		return -EINVAL;
	}
	
	if(channel <= 0 || channel > 14){
			atbm_printk_err("invalid channel!\n");
			atbm_kfree(extra);
			return -EINVAL;
		}

	for(i=0;i<wrqu->data.length;i++){
		if(extra[i] == ',')
			extra[i] =' ';
	}
		
	if(Test_FreqOffset(hw_priv,&dcxo, &freqErrorHz, &rxs_s, channel)){
		atbm_printk_err("Test_FreqOffset Error\n");
		iResult = -1;
		goto FEEQ_ERR;
	}
	//tmp = atbm_DCXOCodeRead(hw_priv);printk("tmp %d\n"tmp);	
	if(ucWriteEfuseFlag)
	{
		atbm_printk_wext("ucWriteEfuseFlag :%d\n",ucWriteEfuseFlag);
		wsm_get_efuse_data(hw_priv,(void *)&efuse_d,sizeof(struct efuse_headr));

		if(efuse_d.version == 0)
		{
			iResult = -3;
			goto FEEQ_ERR;
		}
		efuse_d.dcxo_trim = dcxo;
		/*
		*LMC_STATUS_CODE__EFUSE_VERSION_CHANGE	failed because efuse version change  
		*LMC_STATUS_CODE__EFUSE_FIRST_WRITE, 		failed because efuse by first write   
		*LMC_STATUS_CODE__EFUSE_PARSE_FAILED,		failed because efuse data wrong, cannot be parase
		*LMC_STATUS_CODE__EFUSE_FULL,				failed because efuse have be writen full
		*/
		ret = wsm_efuse_change_data_cmd(hw_priv, &efuse_d,0);
		if (ret == LMC_STATUS_CODE__EFUSE_FIRST_WRITE)
		{
			iResult = -3;
		}else if (ret == LMC_STATUS_CODE__EFUSE_PARSE_FAILED)
		{
			iResult = -4;
		}else if (ret == LMC_STATUS_CODE__EFUSE_FULL)
		{
			iResult = -5;
		}else if (ret == LMC_STATUS_CODE__EFUSE_VERSION_CHANGE)
		{
			iResult = -6;
		}else
		{
			iResult = 0;
		}

		frame_hexdump("efuse_d", (u8 *)&efuse_d, sizeof(struct efuse_headr));
		wsm_get_efuse_data(hw_priv,(void *)&efuse_bak, sizeof(struct efuse_headr));
		frame_hexdump("efuse_bak", (u8 *)&efuse_bak, sizeof(struct efuse_headr));
		
		if(memcmp((void *)&efuse_bak,(void *)&efuse_d, sizeof(struct efuse_headr)) !=0)
		{
			iResult = -2;
		}else
		{
			iResult = 0;
		}
		
	}

	
FEEQ_ERR:	
	
	sprintf(buff, "cfo:%d,evm:%d,gainImb:%d, phaseImb:%d,dcxo:%d,result:%d (0:OK; -1:FreqOffset Error; -2:efuse hard error;"
		" -3:efuse no written; -4:efuse anaysis failed; -5:efuse full; -6:efuse version change)",
	rxs_s.Cfo,
	rxs_s.evm,
	rxs_s.GainImb,
	rxs_s.PhaseImb,
	dcxo,
	iResult
	);

	if((ret = copy_to_user(wrqu->data.pointer, buff, strlen(buff))) != 0){
		atbm_kfree(extra);
		return -EINVAL;
	}
	atbm_kfree(extra);
	return ret;
}
#endif





/*
	iwpriv wlan0 common singletone,<channle>
	channel:[1,14]
*/
static int atbm_ioctl_send_singleTone(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *vif;
	struct ieee80211_sub_if_data *sdata_tmp = NULL;
	struct atbm_vif *priv;// = ABwifi_get_vif_from_ieee80211(&sdata->vif);
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	char *ptr = NULL;
	char const *pos = NULL;
	char const *pos_end = NULL;
	int ret = 0;
	int i   = 0;
	int len = 0;
	int channel = 0;
	int mode = 2;
	int rateIdx = 7;
	int bw = 0;
	int chOff = 0;

	
	ETF_HE_TX_CONFIG_REQ  EtfConfig;
	//atbm_printk_err("start tx join status [%d] \n",priv->join_status);
	
	list_for_each_entry(sdata_tmp, &local->interfaces, list){
	    if (sdata_tmp->vif.type == NL80211_IFTYPE_AP){
		 		atbm_printk_err("driver run ap mode! not allow start tx! \n");
	             return -EINVAL;
	    }
		priv = ABwifi_get_vif_from_ieee80211(&sdata_tmp->vif);
		if(priv->join_status == 4){
			atbm_printk_err("driver on  sta mode connect ap! not allow start tx! \n");
			return -EINVAL;
		}
    }
	
	if(ETF_bStartTx || ETF_bStartRx){
		
		if(ETF_bStartTx){
			atbm_ioctl_stop_tx(dev,info,wrqu,ext);
			msleep(500);
		}
		else{
			atbm_printk_err("Error! already start_rx, please stop_rx first!\n");
			return 0;
		}
		
	}
	
	ptr = (char *)atbm_kzalloc(wdata->data.length+1, GFP_KERNEL);
	
	if(!ptr){
		ret =  -ENOMEM;
		goto exit;
	}
	
	if(copy_from_user(ptr, wdata->data.pointer, wdata->data.length)){
		ret =  -EINVAL;
		goto exit;
	}
	
	ptr[wrqu->data.length] = 0;
	
	for(i = 0;i<wrqu->data.length;i++){
		if(ptr[i] == ',')
			ptr[i] = ATBM_SPACE;
	}
	
	pos = atbm_skip_space(ptr,wrqu->data.length+1);

	if(pos == NULL){
		ret = -EINVAL;
		goto exit;
	}

	pos = pos+strlen("singletone");

	pos = atbm_skip_space(pos,wrqu->data.length+1-(pos-ptr));

	if(pos == NULL){
		ret = -EINVAL;
		goto exit;
	}

	len = wrqu->data.length + 1 - (pos - ptr);

	if(len <= 0){
		ret = -EINVAL;
		goto exit;
	}

	pos_end = memchr(pos,ATBM_TAIL,len);
	
	if(pos_end != NULL)
		len = pos_end - pos+1;

	/*
	*parase channel
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos,len,channel,atbm_accsii_to_int,false,exit,ret);
	if((channel < 0) || (channel > 165)){
		ret = -EINVAL;
		goto exit;
	}

	
	atbm_printk_always("**singletone start***\n");
	memset(&EtfConfig, 0, sizeof(EtfConfig));
	atbm_ETF_PHY_TxRxParamInit(&EtfConfig);

	atbm_etf_tx_rx_param_config(&EtfConfig.TxConfig, channel, mode, rateIdx, bw, chOff, 0);
	EtfConfig.TxConfig.PSDULen = 1024;

	hw_priv->etf_channel = channel;
	hw_priv->etf_channel_type = 0;
	//hw_priv->etf_channel_type |= (0xb << 16);//use rx flag start rx status timer in lmac

	atbm_for_each_vif(hw_priv,vif,i){
		if((vif != NULL)){
			mutex_lock(&hw_priv->conf_mutex);
			ret = atbm_set_channel(hw_priv, BIT(WSM_SET_CHANTYPE_FLAGS__ETF_TEST_START));//set channel and start TPC
			if(ret != 0)
			{
				atbm_printk_err("atbm_set_channel err,ret:%d\n", ret);
				//goto exit;
			}
			mutex_unlock(&hw_priv->conf_mutex);
			break;
		}
	}

	mutex_lock(&hw_priv->conf_mutex);				
	ETF_bStartTx = 2;
	mutex_unlock(&hw_priv->conf_mutex);

	//atbm_ETF_TxConfigShow(&EtfConfig.TxConfig);
	atbm_ETF_PHY_Start_Tx_Step1(&EtfConfig);
	atbm_ETF_PHY_Start_Tx_Step2(&EtfConfig);

	atbm_for_each_vif(hw_priv,vif,i){
		if((vif != NULL)){
			mutex_lock(&hw_priv->conf_mutex);
			ret = atbm_set_channel(hw_priv, 0);//set channel and start TPC
			if(ret != 0)
			{
				atbm_printk_err("atbm_set_channel err,ret:%d\n", ret);
			}
			mutex_unlock(&hw_priv->conf_mutex);
			break;
		}
	}

	atbm_ETF_PHY_Stop_Tx(NULL);

	atbm_ETF_SingleToneEnable();
	
exit:
	if(ptr)
		atbm_kfree(ptr);

	return ret;
}


static int atbm_ioctl_set_duty_ratio(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	int i = 0;
	int ii = 0;
	int ret = -EINVAL;
	int duty_ratio = 0;
	char *extra = NULL;
	char *ptr = NULL;
	int flag = 0;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *vif = NULL;


	if(!(extra = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		return -EINVAL;
	}
	if((ret = copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(extra);
		ret = -EINVAL;
		goto exit;
	}
	extra[wrqu->data.length] = 0;

	atbm_printk_always("atbm_ioctl_set_duty_ratio:%s\n",extra);

	if(wrqu->data.length < strlen("duty_ratio,")){
		atbm_printk_err("e.g: ./iwpriv wlan0 common duty_ratio,<duty_ratio_value>\n");
		atbm_printk_err("e.g: ./iwpriv wlan0 common duty_ratio,0.1\n");
		atbm_kfree(extra);
		ret = -EINVAL;
		goto exit;
	}


	for(ii = 0;ii<wrqu->data.length;ii++){
		if(extra[ii] == ',')
		{
			ptr = &extra[ii+1];


			break;
		}
	}
	atbm_printk_always(" ptr:%s\n",ptr);

	for(ii = 0;ii<strlen(ptr);ii++)
	{
		if(ptr[ii] == ','){
			break;
		}
		if(ptr[ii] == '.'){
			flag = 1;
			continue;
		}	
		duty_ratio = duty_ratio* 10 +(ptr[ii] - 0x30);
	}

	if(flag == 0)
		duty_ratio = duty_ratio * 10;

	if((duty_ratio < 0) || (duty_ratio > 10))
	{
		atbm_printk_always("[ERROR]invalid duty_ratio:%d\n",duty_ratio);
		ret = -1;
		goto exit;
	}

	atbm_printk_always("duty_ratio:%d%%\n",duty_ratio*10);
	
	g_DutyCycle = duty_ratio;

exit:
	if(extra)
		atbm_kfree(extra);
	return ret;
}

#ifdef CONFIG_ATBM_SUPPORT_AP_CONFIG
static int atbm_ioctl_set_ap_conf(struct net_device *dev, struct iw_request_info *info, 
										   union iwreq_data *wrqu, char *ext)
{
	struct ieee80211_internal_ap_conf conf_req;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	char *extra = NULL;
	const char* pos;
	const char* pos_end;
	int ret = 0;
	int len = 0;
	int channel = 0;
	int i;
	if(!ieee80211_sdata_running(sdata)){
		ret = -ENETDOWN;
		goto exit;
	}
	
	extra = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL);

	if(extra == NULL){
		ret =  -ENOMEM;
		goto exit;
	}

	if((ret = copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))){
		return -EINVAL;
		goto exit;
	}
	
	extra[wrqu->data.length] = 0;

	for(i = 0;i<wrqu->data.length;i++){
		if(extra[i] == ',')
			extra[i] = ATBM_SPACE;
	}
	
	atbm_printk_debug("%s:%s %d\n",__func__,extra,wrqu->data.length);
	pos = atbm_skip_space(extra,wrqu->data.length);

	if(pos == NULL){
		ret = -EINVAL;
		goto exit;
	}

	pos = pos+strlen("ap_conf");

	pos = atbm_skip_space(pos,wrqu->data.length-(pos-extra));

	if(pos == NULL){
		ret = -EINVAL;
		goto exit;
	}
	len = wrqu->data.length - (pos - extra);

	if(len <= 0){
		ret = -EINVAL;
		goto exit;
	}
	
	pos_end = memchr(pos,ATBM_TAIL,len);
	if(pos_end != NULL)
		len = pos_end - pos;

	if(len>2){
		ret = -EINVAL;
		goto exit;
	}

	if(atbm_accsii_to_int(pos,len,&channel) == false){		
		ret = -EINVAL;
		goto exit;
	}

	if(channel && (ieee8011_channel_valid(&sdata->local->hw,channel) == false)){
		ret = -EINVAL;
		goto exit;
	}
	
	memset(&conf_req,0,sizeof(struct ieee80211_internal_ap_conf));
	conf_req.channel = (u8)channel;

	if(atbm_internal_update_ap_conf(sdata,&conf_req,conf_req.channel == 0?true:false) == false)
		ret = -EINVAL;
	
exit:
	if(extra)
		atbm_kfree(extra);

	return ret;
}
#endif
#ifdef CONFIG_ATBM_MONITOR_SPECIAL_MAC
static int atbm_ioctl_rx_monitor_mac(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_internal_mac_monitor mac_moniter;
	char *ptr = NULL;
	char const *pos = NULL;
	char const *pos_next = NULL;
	char const *pos_end = NULL;
	int ret = 0;
	int i   = 0;
	int len = 0;
	int enable = 0;
	int index = -1;

	if(ieee80211_get_channel_mode(local,NULL) != CHAN_MODE_FIXED){
		ret =  -EINVAL;
		goto exit;
	}
	
	ptr = (char *)atbm_kzalloc(wdata->data.length+1, GFP_KERNEL);
	
	if(!ptr){
		ret =  -ENOMEM;
		goto exit;
	}
	
	if(copy_from_user(ptr, wdata->data.pointer, wdata->data.length)){
		ret =  -EINVAL;
		goto exit;
	}
	
	ptr[wrqu->data.length] = 0;
	
	for(i = 0;i<wrqu->data.length;i++){
		if(ptr[i] == ',')
			ptr[i] = ATBM_SPACE;
	}
	
	pos = atbm_skip_space(ptr,wrqu->data.length+1);

	if(pos == NULL){
		ret = -EINVAL;
		goto exit;
	}

	pos = pos+strlen("monitor_mac");

	pos = atbm_skip_space(pos,wrqu->data.length+1-(pos-ptr));

	if(pos == NULL){
		ret = -EINVAL;
		goto exit;
	}

	len = wrqu->data.length+1 - (pos - ptr);

	if(len <= 0){
		ret = -EINVAL;
		goto exit;
	}

	/*
	*parase enable
	*/
	pos_next = memchr(pos,ATBM_SPACE,len);

	if(pos_next == NULL)
		pos_next = memchr(pos,ATBM_TAIL,len);
	
	if(pos_next - pos > 1){
		ret = -EINVAL;
		goto exit;
	}
	pos_end = memchr(pos,ATBM_TAIL,len);
	
	if(pos_end != NULL)
		len = pos_end - pos;
	
	memset(&mac_moniter,0,sizeof(struct ieee80211_internal_mac_monitor));
	
	if(atbm_accsii_to_int(pos,pos_next - pos,&enable) == false){		
		ret = -EINVAL;
		goto exit;
	}

	if(enable)
		mac_moniter.flags |= IEEE80211_INTERNAL_MAC_MONITOR_START;
	else 
		mac_moniter.flags |= IEEE80211_INTERNAL_MAC_MONITOR_STOP;

	pos_next++;
	len -= (pos_next-pos);
	pos = pos_next;

	/*
	*parase index
	*/

	pos_next = memchr(pos,ATBM_SPACE,len);

	if(pos_next == NULL)
		pos_next = memchr(pos,ATBM_TAIL,len);

	if(pos_next == NULL)
		pos_next = pos+1;

	if(pos_next - pos > 1){
		ret = -EINVAL;
		goto exit;
	}

	if(atbm_accsii_to_int(pos,pos_next - pos,&index) == false){		
		ret = -EINVAL;
		goto exit;
	}

	if((index != 0)&&(index != 1)){
		ret = -EINVAL;
		goto exit;
	}

	pos_next++;
	len -= (pos_next-pos);
	pos = pos_next;
	mac_moniter.index = index;
	
	if(enable){
		u8 mac_len = 0;
		u8 hex;
		u8 mac_begin = 0;
		i = 0;
		/*
		*00:11:22:33:44:55
		*/
		if(len != 17){
			ret = -EINVAL;
			goto exit;
		}
		
		for(i=0;i<17;i++){
		
			if(pos[i] == ':'){
				mac_len++;
				mac_begin = 0;
				continue;
			}

			if(mac_len>=6){
				ret = -EINVAL;
				goto exit;
			}

			if(mac_begin>1){
				ret = -EINVAL;
				goto exit;
			}
			if (atbm_accsii_to_hex(pos[i],&hex) == false){
				goto exit;
			}
			mac_moniter.mac[mac_len] *= 16;
			mac_moniter.mac[mac_len] += hex;
			mac_begin++;
		}
	}else if(atbm_skip_space(pos,len) != NULL){
		ret = -EINVAL;
		goto exit;
	}
	
	if(atbm_internal_mac_monitor(&local->hw,&mac_moniter) == false){
		ret = -EINVAL;
		goto exit;
	}
exit:
	if(ptr)
		atbm_kfree(ptr);

	return ret;
}
static int atbm_ioctl_rx_monitor_mac_status(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct ieee80211_internal_mac_monitor mac_moniter;
	int ret = 0;
	char *results = NULL;
	int i = 0;
	int copy_len = 0;
	int total_len = 0;
	
	memset(&mac_moniter,0,sizeof(struct ieee80211_internal_mac_monitor));

	mac_moniter.flags = IEEE80211_INTERNAL_MAC_MONITOR_RESULTS;
	
	results = (char *)atbm_kzalloc(1024, GFP_KERNEL);

	if(results == NULL){
		ret =  -ENOMEM;
		goto exit;
	}
	
	if(atbm_internal_mac_monitor(&local->hw,&mac_moniter) == false){
		ret = -EINVAL;
		goto exit;
	}

	copy_len = scnprintf(results+total_len,1024-total_len,"mac monitor status --->\n");

	total_len += copy_len;
	for(i = 0;i < IEEE80211_INTERNAL_MAC_MONITOR_RESULTS;i++){
		
		if(mac_moniter.reults[i].used == 0)
			break;
		copy_len = scnprintf(results+total_len,1024-total_len,
			  "index=[%d],enable[%d],forcestop[%d],mac=[%pM],found=[%d],rssi[%d],deltime[%d]ms\n",
			  mac_moniter.reults[i].index,
			  mac_moniter.reults[i].enabled,
			  mac_moniter.reults[i].forcestop,
			  mac_moniter.reults[i].mac,
			  mac_moniter.reults[i].found,
			  mac_moniter.reults[i].rssi >= 128 ? mac_moniter.reults[i].rssi-256:mac_moniter.reults[i].rssi,
			  mac_moniter.reults[i].delta_time);
		if(copy_len > 0)
			total_len+=copy_len;
		else
			break;
	}

	if(extra){
		memcpy(extra,results,total_len);	
		wrqu->data.length = total_len + 1;
	}
exit:
	if(results)
		atbm_kfree(results);
	return ret;
}
#endif

#ifdef CONFIG_IEEE80211_SPECIAL_FILTER
static int atbm_ioctl_rx_filter_frame(struct net_device *dev, 
			struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_special_filter filter;
	char *ptr = NULL;
	char const *pos = NULL;
	int ret = 0;
	int len = 0;
	int i = 0;
	char type = 0;
	char hex = 0;
	char n_params = 0;
	
	if (!ieee80211_sdata_running(sdata)){
		ret =  -ENETDOWN;
		goto exit;
	}
	
	ptr = (char *)atbm_kzalloc(wdata->data.length+1, GFP_KERNEL);
	
	if(!ptr){
		ret =  -ENOMEM;
		goto exit;
	}

	if(copy_from_user(ptr, wdata->data.pointer, wdata->data.length)){
		ret =  -EINVAL;
		goto exit;
	}
	
	ptr[wrqu->data.length] = ATBM_SPACE;
	
	for(i = 0;i<wrqu->data.length;i++){
		if((ptr[i] == ATBM_LINEF) || 
		   (ptr[i] == ATBM_ENTER) || 
		   (ptr[i] == ATBM_TAIL) ||
		   (ptr[i] == ',') || (ptr[i] == 0)){
		   if(ptr[i] == ',')
		   		n_params ++;
		   ptr[i] = ATBM_SPACE;
		}
	}

	if(n_params != 1){
		atbm_printk_debug("%s:n_params(%d)\n",__func__,n_params);
		ret = -EINVAL;
		goto exit;
	}
	
	pos = atbm_skip_space(ptr,wrqu->data.length+1);

	if(pos == NULL){
		atbm_printk_debug("%s:pos == NULL\n",__func__);
		ret = -EINVAL;
		goto exit;
	}

	pos = pos+strlen("filter_frame");

	pos = atbm_skip_space(pos,wrqu->data.length+1-(pos-ptr));

	if(pos == NULL){
		atbm_printk_debug("%s:pos == NULL2\n",__func__);
		ret = -EINVAL;
		goto exit;
	}

	len = wrqu->data.length + 1 - (pos - ptr);

	if(len < 2){
		atbm_printk_debug("%s:len(%d)\n",__func__,len);
		ret = -EINVAL;
		goto exit;
	}

	for(i=0;i<2;i++){
		if (atbm_accsii_to_hex(pos[i],&hex) == false){
			goto exit;
		}
		type *= 16;
		type += hex;
	}

	pos += 2;
	len -= 2;

	if(atbm_skip_space(pos,len)){
		atbm_printk_debug("%s:pos is not null\n",__func__);
		ret = -EINVAL;
		goto exit;
	}
	memset(&filter,0,sizeof(struct ieee80211_special_filter));
	filter.filter_action = type;
	filter.flags = SPECIAL_F_FLAGS_FRAME_TYPE;
	atbm_printk_debug("%s:action(%x)\n",__func__,filter.filter_action);
	ieee80211_special_filter_register(sdata,&filter);
exit:
	if(ptr)
		atbm_kfree(ptr);	
	return ret;
}
static int atbm_ioctl_rx_filter_ie(struct net_device *dev, 
			struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_special_filter filter;
	char *ptr = NULL;
	char const *pos = NULL;
	char const *pos_next = NULL;
	int ret = 0;
	int len = 0;
	int i = 0;
	char n_params = 0;
	int ie_oui[4] = {0,0,0,0};
	
	
	if (!ieee80211_sdata_running(sdata)){
		ret =  -ENETDOWN;
		goto exit;
	}
	
	ptr = (char *)atbm_kzalloc(wdata->data.length+1, GFP_KERNEL);
	
	if(!ptr){
		ret = -ENOMEM;
		goto exit;
	}

	if(copy_from_user(ptr, wdata->data.pointer, wdata->data.length)){
		ret =  -EINVAL;
		goto exit;
	}
	
	ptr[wrqu->data.length] = ATBM_SPACE;
	
	for(i = 0;i<wrqu->data.length;i++){
		if((ptr[i] == ATBM_LINEF) || 
		   (ptr[i] == ATBM_ENTER) || 
		   (ptr[i] == ATBM_TAIL) ||
		   (ptr[i] == ',')||(ptr[i] == 0)){
		   if(ptr[i] == ',')
		   		n_params ++;
		   ptr[i] = ATBM_SPACE;
		}
	}

	if((n_params != 1)&&(n_params != 4)){
		atbm_printk_debug("%s:1\n",__func__);
		ret = -EINVAL;
		goto exit;
	}
	pos = atbm_skip_space(ptr,wrqu->data.length+1);

	if(pos == NULL){
		atbm_printk_debug("%s:2\n",__func__);
		ret = -EINVAL;
		goto exit;
	}

	pos = pos+strlen("filter_ie");

	pos = atbm_skip_space(pos,wrqu->data.length+1-(pos-ptr));

	if(pos == NULL){
		atbm_printk_debug("%s:3\n",__func__);
		ret = -EINVAL;
		goto exit;
	}

	len = wrqu->data.length + 1 - (pos - ptr);

	if(len < 2){	
		atbm_printk_debug("%s:5\n",__func__);
		ret = -EINVAL;
		goto exit;
	}

	for(i = 0;i<n_params;i++){

		if( len <= 0 ){
			atbm_printk_debug("%s:6 %d\n",__func__,i);
			ret = -EINVAL;
			goto exit;
		}
				
		pos_next = memchr(pos,ATBM_SPACE,len);
		if(pos_next == NULL){
			atbm_printk_debug("%s:7 %d\n",__func__,i);
			ret = -EINVAL;
			goto exit;
		}			
		
		if((pos_next - pos > 3) || (pos_next - pos < 1)){
			atbm_printk_debug("%s:8 %d\n",__func__,i);
			ret = -EINVAL;
			goto exit;
		}
		atbm_accsii_to_int(pos,pos_next-pos,&ie_oui[i]);
		len -= (pos_next + 1 - pos);
		pos = pos_next + 1;
		
	}

	memset(&filter,0,sizeof(struct ieee80211_special_filter));

	filter.filter_action = ie_oui[0];
	filter.flags = SPECIAL_F_FLAGS_FRAME_IE;
	
	if(n_params > 1){
		filter.oui[0] = ie_oui[1];
		filter.oui[1] = ie_oui[2];
		filter.oui[2] = ie_oui[3];
		filter.flags |= SPECIAL_F_FLAGS_FRAME_OUI;
	}
	atbm_printk_debug("%s:ie[%d],oui[%d:%d:%d]\n",__func__,filter.filter_action,filter.oui[0],filter.oui[1],filter.oui[2]);
	ieee80211_special_filter_register(sdata,&filter);
exit:
	if(ptr)
		atbm_kfree(ptr);	
	return ret;

}
static int atbm_ioctl_rx_filter_clear(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	const char *pos;
	char *ptr = NULL;
	int ret = 0;
	int i = 0;
	u8 n_params = 0;
	
	if (!ieee80211_sdata_running(sdata)){
		ret =  -ENETDOWN;
		goto exit;
	}
	
	ptr = (char *)atbm_kzalloc(wdata->data.length+1, GFP_KERNEL);
	
	if(!ptr){
		ret = -ENOMEM;
		goto exit;
	}

	if(copy_from_user(ptr, wdata->data.pointer, wdata->data.length)){
		ret =  -EINVAL;
		goto exit;
	}
	
	ptr[wrqu->data.length] = ATBM_SPACE;
	
	for(i = 0;i<wrqu->data.length;i++){
		if((ptr[i] == ATBM_LINEF) || 
		   (ptr[i] == ATBM_ENTER) || 
		   (ptr[i] == ATBM_TAIL) ||
		   (ptr[i] == ',')||(ptr[i] == 0)){
		   if(ptr[i] == ',')
		   		n_params ++;
		   ptr[i] = ATBM_SPACE;
		}
	}

	if(n_params > 0){
		atbm_printk_debug("%s:n_params (%d)\n",__func__,n_params);
		ret = -EINVAL;
		goto exit;
	}

	pos = atbm_skip_space(ptr,wrqu->data.length+1);

	if(pos == NULL){
		atbm_printk_debug("%s:2\n",__func__);
		ret = -EINVAL;
		goto exit;
	}
	
	pos = pos+strlen("filter_clear");
	
	if(atbm_skip_space(pos,wrqu->data.length+1 - (pos-ptr))){
		atbm_printk_debug("%s:ptr err\n",__func__);
		ret = -EINVAL;
		goto exit;
	}
	
	ieee80211_special_filter_clear(sdata);
exit:
	if(ptr){
		atbm_kfree(ptr);
	}
	return ret;
}
static int atbm_ioctl_rx_filter_show(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_special_filter_table *tables = NULL;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	const char *pos;
	char *ptr = NULL;
	int ret = 0;
	int i = 0;
	u8 n_params = 0;
	char *results = NULL;
	int copy_len = 0;
	int total_len = 0;

	if (!ieee80211_sdata_running(sdata)){
		ret =  -ENETDOWN;
		goto exit;
	}
	
	ptr = (char *)atbm_kzalloc(wdata->data.length+1,GFP_KERNEL);
	
	if(!ptr){
		ret = -ENOMEM;
		goto exit;
	}
	
	tables = atbm_kzalloc(sizeof(struct ieee80211_special_filter_table),GFP_KERNEL);
	
	if(tables == NULL){
		ret = -ENOMEM;
		goto exit;
	}
	/*
	*max is 513
	*/
	results = atbm_kzalloc(512,GFP_KERNEL);
	
	if(results == NULL){
		ret = -ENOMEM;
		goto exit;
	}
	
	if(copy_from_user(ptr, wdata->data.pointer, wdata->data.length)){
		ret =  -EINVAL;
		goto exit;
	}
	
	ptr[wrqu->data.length] = ATBM_SPACE;
	
	for(i = 0;i<wrqu->data.length;i++){
		if((ptr[i] == ATBM_LINEF) || 
		   (ptr[i] == ATBM_ENTER) || 
		   (ptr[i] == ATBM_TAIL) ||
		   (ptr[i] == ',')||(ptr[i] == 0)){
		   if(ptr[i] == ',')
		   		n_params ++;
		   ptr[i] = ATBM_SPACE;
		}
	}

	if(n_params > 0){
		atbm_printk_debug("%s:n_params (%d)\n",__func__,n_params);
		ret = -EINVAL;
		goto exit;
	}

	pos = atbm_skip_space(ptr,wrqu->data.length+1);

	if(pos == NULL){
		atbm_printk_debug("%s:2\n",__func__);
		ret = -EINVAL;
		goto exit;
	}
	
	pos = pos + strlen("filter_show");
	
	if(atbm_skip_space(pos,wrqu->data.length+1 - (pos-ptr))){
		atbm_printk_debug("%s:ptr err (%s)(%d)\n",__func__,pos,wrqu->data.length+1 - (int)(pos-ptr));
		ret = -EINVAL;
		goto exit;
	}

	ieee80211_special_filter_request(sdata,tables);

	copy_len = scnprintf(results+total_len,512-total_len,"filter table --->\n");
	total_len += copy_len;
	
	for(i = 0;i < tables->n_filters;i++){

		if((tables->table[i].flags & IEEE80211_SPECIAL_FILTER_MASK) == SPECIAL_F_FLAGS_FRAME_TYPE)
			copy_len = scnprintf(results+total_len,512-total_len,"filter[%d]: frame [%x]\n",i,tables->table[i].filter_action);
		else if((tables->table[i].flags & IEEE80211_SPECIAL_FILTER_MASK) == SPECIAL_F_FLAGS_FRAME_IE)
			copy_len = scnprintf(results+total_len,512-total_len,"filter[%d]: ie[%d]\n",i,tables->table[i].filter_action);
		else if((tables->table[i].flags & IEEE80211_SPECIAL_FILTER_MASK) == (SPECIAL_F_FLAGS_FRAME_IE | SPECIAL_F_FLAGS_FRAME_OUI)){
			copy_len = scnprintf(results+total_len,512-total_len,"filter[%d]: ie[%d] oui[%d:%d:%d]\n",i,tables->table[i].filter_action,
				tables->table[i].oui[0],tables->table[i].oui[1],tables->table[i].oui[2]);
		}else {
			copy_len = scnprintf(results+total_len,512-total_len,"filter[%d]: unkown\n",i);
		}
		if(copy_len > 0)
			total_len+=copy_len;
		else
			break;
	}
	
	if(extra){
		memcpy(extra,results,total_len);	
		wrqu->data.length = total_len + 1;
	}
	
exit:
	if(ptr)
		atbm_kfree(ptr);
	if(tables)
		atbm_kfree(tables);
	if(results)
		atbm_kfree(results);
	return ret;
}
#endif
#ifdef HIDDEN_SSID
//extern int atbm_upload_beacon_private(struct atbm_vif *priv);
static int atbm_ioctl_set_hidden_ssid(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	int ret = 0,i=0,strheadLen=0;
//	int len = 0;
	char *ptr = NULL;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	//struct ieee80211_local *local = sdata->local;
	//struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *priv = (struct atbm_vif *)sdata->vif.drv_priv;


	atbm_printk_wext("atbm_ioctl_set_hidden_ssid()\n\n");

	if(sdata->vif.type != NL80211_IFTYPE_STATION){

		if(!(ptr = (char *)atbm_kzalloc(wdata->data.length+1, GFP_KERNEL)))
			return -ENOMEM;
		if((ret = copy_from_user(ptr, wdata->data.pointer, wdata->data.length))){
			atbm_kfree(ptr);
			return -EINVAL;
		}
		ptr[wdata->data.length] = 0;
//		len = wdata->data.length-1;
		for(i=0;i<wdata->data.length;i++)
		{
			if(ptr[i] == ',')
			{
				strheadLen=i+1;
				break;
			}
		}
		atbm_printk_err("strheadLen = %d,ptr[strheadLen] = %d wdata->data.length = %d \n",strheadLen,ptr[strheadLen],wdata->data.length);
		if( strheadLen != 0 ){
			if(ptr[strheadLen] == 0x30)
				priv->hidden_ssid = 0;
			else
				priv->hidden_ssid = 1;
		}else{
			atbm_printk_err("set hide ssid paramters error \n");
			atbm_kfree(ptr);
			ret = -1;
			goto Error;
		}

		ret = atbm_upload_beacon_private(priv);
		if(ret < 0){
			atbm_printk_err("error, upload beacon private failed %d\n", ret);
			atbm_kfree(ptr);
			goto Error;
		}

		atbm_kfree(ptr);



	}else{
		atbm_printk_err("warning, only AP mode support hidden ssid func, type(%d) \n", sdata->vif.type);
	}

Error:
	return ret;
}
#endif
static int atbm_ioctl_associate_sta_status(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_local *local = sdata->local;	
	struct ieee80211_channel_state *chan_state = ieee80211_get_channel_state(local, sdata);
	struct sta_info *sta;
	const char *pos;
	char *ptr = NULL;
	int ret = 0;
	int i = 0;
	u8 n_params = 0;
	char *results = NULL;
	int copy_len = 0;
	int total_len = 0;

	if (!ieee80211_sdata_running(sdata)){
		ret =  -ENETDOWN;
		goto exit;
	}
	
	if(chan_state->oper_channel == NULL){
		ret =  -ENETDOWN;
		goto exit;
	}
	ptr = (char *)atbm_kzalloc(wdata->data.length+1,GFP_KERNEL);
	
	if(!ptr){
		ret = -ENOMEM;
		goto exit;
	}
	/*
	*max is 513
	*/
	results = atbm_kzalloc(512,GFP_KERNEL);
	
	if(results == NULL){
		ret = -ENOMEM;
		goto exit;
	}
	
	if(copy_from_user(ptr, wdata->data.pointer, wdata->data.length)){
		ret =  -EINVAL;
		goto exit;
	}
	
	ptr[wrqu->data.length] = ATBM_SPACE;
	
	for(i = 0;i<wrqu->data.length;i++){
		if((ptr[i] == ATBM_LINEF) || 
		   (ptr[i] == ATBM_ENTER) || 
		   (ptr[i] == ATBM_TAIL) ||
		   (ptr[i] == ',')||(ptr[i] == 0)){
		   if(ptr[i] == ',')
		   		n_params ++;
		   ptr[i] = ATBM_SPACE;
		}
	}

	if(n_params > 0){
		atbm_printk_debug("%s:n_params (%d)\n",__func__,n_params);
		ret = -EINVAL;
		goto exit;
	}

	pos = atbm_skip_space(ptr,wrqu->data.length+1);

	if(pos == NULL){
		atbm_printk_debug("%s:2\n",__func__);
		ret = -EINVAL;
		goto exit;
	}
	
	pos = pos + strlen("stations_show");
	
	if(atbm_skip_space(pos,wrqu->data.length+1 - (pos-ptr))){
		atbm_printk_debug("%s:ptr err (%s)(%d)\n",__func__,pos,wrqu->data.length+1 - (int)(pos-ptr));
		ret = -EINVAL;
		goto exit;
	}

	copy_len = scnprintf(results+total_len,512-total_len,"station show -->\n");
	total_len += copy_len;
	
	mutex_lock(&local->sta_mtx);
	list_for_each_entry_rcu(sta, &local->sta_list, list) {
		struct wsm_sta_info_req req;
		struct wsm_sta_info     info;
		if((sta->sdata != sdata) || 
		   (sta->uploaded == false) || 
		   (sta->dead == true) ||
		   (test_sta_flag(sta, WLAN_STA_AUTHORIZED) == 0)){
			continue;
		}
		req.flags = WSM_STA_REQ_FLAGS__TXRATE;
		memcpy(req.mac,sta->sta.addr,6); 
		atbm_req_sta_info(local->hw.priv,&req,&info,0);
		/*
		*mac,rss and rate;
		*/
		if(	sta->sta.he_cap.has_he)
			{
			copy_len = scnprintf(results + total_len,512-total_len,"mac[%pM],rssi[%d],bg[%x],11n[%x] ax[%x]\n",
			sta->sta.addr,(s8)-atbm_ewma_read(&sta->avg_signal2),sta->sta.supp_rates[chan_state->oper_channel->band],
			sta->sta.ht_cap.mcs.rx_mask[0],  ((1<<((sta->sta.he_cap.he_mcs_nss_supp.rx_mcs_80&0x03)*2+8))-1) 	);			
			}
			else
				{
				copy_len = scnprintf(results + total_len,512-total_len,"mac[%pM],rssi[%d],bg[%x],11n[%x]\n",
				sta->sta.addr,(s8)-atbm_ewma_read(&sta->avg_signal2),sta->sta.supp_rates[chan_state->oper_channel->band],
				sta->sta.ht_cap.mcs.rx_mask[0]);
				}
		
		if(copy_len > 0)
			total_len += copy_len;
		else {
			break;
		}		
	}
	mutex_unlock(&local->sta_mtx);
	if(extra){
		memcpy(extra,results,total_len);	
		wrqu->data.length = total_len + 1;
	}
	
exit:
	if(ptr)
		atbm_kfree(ptr);
	if(results)
		atbm_kfree(results);
	return ret;
}



/*
*iface receive special type package 0xf0
*/
/*
static int atbm_ioctl_subtype(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	return 0;
}
*/

/*
*gpio config iwpriv wlan0 common gpio_conf,gpio,dir,pup or pud  
*/
static int atbm_ioctl_gpio_config(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv = (struct atbm_common *)local->hw.priv;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	char *ptr = NULL;
	char const *pos = NULL;
	char const *pos_end = NULL;
	int ret = 0;
	int i   = 0;
	int len = 0;
	int gpio = 0;
	int dir = 0;
	int pup = 0;

	if (!ieee80211_sdata_running(sdata)){
		ret =  -ENETDOWN;
		goto exit;
	}
	
	ptr = (char *)atbm_kzalloc(wdata->data.length+1, GFP_KERNEL);
	
	if(!ptr){
		ret =  -ENOMEM;
		goto exit;
	}
	
	if(copy_from_user(ptr, wdata->data.pointer, wdata->data.length)){
		ret =  -EINVAL;
		goto exit;
	}
	
	ptr[wrqu->data.length] = 0;
	
	for(i = 0;i<wrqu->data.length;i++){
		if(ptr[i] == ',')
			ptr[i] = ATBM_SPACE;
	}
	
	pos = atbm_skip_space(ptr,wrqu->data.length+1);

	if(pos == NULL){
		ret = -EINVAL;
		goto exit;
	}

	pos = pos+strlen("gpio_conf");

	pos = atbm_skip_space(pos,wrqu->data.length+1-(pos-ptr));

	if(pos == NULL){
		ret = -EINVAL;
		goto exit;
	}

	len = wrqu->data.length + 1 - (pos - ptr);

	if(len <= 0){
		ret = -EINVAL;
		goto exit;
	}

	pos_end = memchr(pos,ATBM_TAIL,len);
	
	if(pos_end != NULL)
		len = pos_end - pos+1;
	
	/*
	*parase gpio
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos,len,gpio,atbm_accsii_to_int,false,exit,ret);
	/*
	*parase dir
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos,len,dir,atbm_accsii_to_int,false,exit,ret);	
	if((dir != 0)&&(dir != 1)){
		ret = -EINVAL;
		goto exit;
	}
	/*
	*process pup
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos,len,pup,atbm_accsii_to_int,false,exit,ret);
	if((pup != 0)&&(pup != 1)){
		ret = -EINVAL;
		goto exit;
	}
	/*
	*set gpio config
	*/
	atbm_printk_err("%s:gpio[%d],dir[%d],pup[%d]\n",__func__,gpio,dir,pup);
	ret = atbm_internal_gpio_config(hw_priv,gpio,dir?true:false,pup ? true:false,false);
	
exit:
	if(ptr)
		atbm_kfree(ptr);
	
	return ret;
}
/*
*gpio config iwpriv wlan0 common gpio_output,gpio,val  
*/
static int atbm_ioctl_gpio_output(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv = (struct atbm_common *)local->hw.priv;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	char *ptr = NULL;
	char const *pos = NULL;
	char const *pos_end = NULL;
	int ret = 0;
	int i   = 0;
	int len = 0;
	int gpio = 0;
	int val = 0;

	if (!ieee80211_sdata_running(sdata)){
		ret =  -ENETDOWN;
		goto exit;
	}
	
	ptr = (char *)atbm_kzalloc(wdata->data.length+1, GFP_KERNEL);
	
	if(!ptr){
		ret =  -ENOMEM;
		goto exit;
	}
	
	if(copy_from_user(ptr, wdata->data.pointer, wdata->data.length)){
		ret =  -EINVAL;
		goto exit;
	}
	
	ptr[wrqu->data.length] = 0;
	
	for(i = 0;i<wrqu->data.length;i++){
		if(ptr[i] == ',')
			ptr[i] = ATBM_SPACE;
	}
	
	pos = atbm_skip_space(ptr,wrqu->data.length+1);

	if(pos == NULL){
		ret = -EINVAL;
		goto exit;
	}

	pos = pos+strlen("gpio_out");

	pos = atbm_skip_space(pos,wrqu->data.length+1-(pos-ptr));

	if(pos == NULL){
		ret = -EINVAL;
		goto exit;
	}

	len = wrqu->data.length + 1 - (pos - ptr);

	if(len <= 0){
		ret = -EINVAL;
		goto exit;
	}

	pos_end = memchr(pos,ATBM_TAIL,len);
	
	if(pos_end != NULL)
		len = pos_end - pos+1;
	
	/*
	*parase gpio
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos,len,gpio,atbm_accsii_to_int,false,exit,ret);
	/*
	*parase dir
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos,len,val,atbm_accsii_to_int,false,exit,ret);	
	if((val != 0)&&(val != 1)){
		ret = -EINVAL;
		goto exit;
	}
	atbm_printk_err("%s:gpio[%d],val[%d]\n",__func__,gpio,val);
	ret = atbm_internal_gpio_output(hw_priv,gpio,val ? true: false);
exit:
	if(ptr)
		atbm_kfree(ptr);

	return ret;
}
/*
u16 txop;
u16 cw_min;
u16 cw_max;
u8 aifs;

*edca_params iwpriv wlan0 common edca_params,queue,aifs,cw_min,cw_max,txop  
*/
static int atbm_ioctl_edca_params(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	//struct ieee80211_local *local = sdata->local;
	//struct atbm_common *hw_priv = (struct atbm_common *)local->hw.priv;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	char *ptr = NULL;
	char const *pos = NULL;
	char const *pos_end = NULL;
	int ret = 0;
	int i   = 0;
	int queue = 0;
	int aifs = 0;
	int cw_min = 0;
	int cw_max = 0;
	int txop   = 0;
	int len = 0;

	if (!ieee80211_sdata_running(sdata)){
		ret =  -ENETDOWN;
		goto exit;
	}
	
	ptr = (char *)atbm_kzalloc(wdata->data.length+1, GFP_KERNEL);
	
	if(!ptr){
		ret =  -ENOMEM;
		goto exit;
	}
	
	if(copy_from_user(ptr, wdata->data.pointer, wdata->data.length)){
		ret =  -EINVAL;
		goto exit;
	}
	
	ptr[wrqu->data.length] = 0;
	
	for(i = 0;i<wrqu->data.length;i++){
		if(ptr[i] == ',')
			ptr[i] = ATBM_SPACE;
	}
	
	pos = atbm_skip_space(ptr,wrqu->data.length+1);

	if(pos == NULL){
		ret = -EINVAL;
		goto exit;
	}

	pos = pos+strlen("edca_params");

	pos = atbm_skip_space(pos,wrqu->data.length+1-(pos-ptr));

	if(pos == NULL){
		ret = -EINVAL;
		goto exit;
	}

	len = wrqu->data.length + 1 - (pos - ptr);

	if(len <= 0){
		ret = -EINVAL;
		goto exit;
	}

	pos_end = memchr(pos,ATBM_TAIL,len);
	
	if(pos_end != NULL)
		len = pos_end - pos+1;
	
	/*
	*parase queue
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos,len,queue,atbm_accsii_to_int,false,exit,ret);
	if((queue < 0) || (queue >= 4)){
		ret = -EINVAL;
		goto exit;
	}
	
	/*
	*parase aifs
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos,len,aifs,atbm_accsii_to_int,false,exit,ret);	
	
	if((aifs < 0) || (aifs > 0xff)){
		ret = -EINVAL;
		goto exit;
	}

	/*
	*parase cw_min
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos,len,cw_min,atbm_accsii_to_int,false,exit,ret);	
	
	if((cw_min < 0) || (cw_min > 0xffff)){
		ret = -EINVAL;
		goto exit;
	}

	/*
	*parase cw_max
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos,len,cw_max,atbm_accsii_to_int,false,exit,ret);	
	
	if((cw_max < 0) || (cw_max > 0xffff)){
		ret = -EINVAL;
		goto exit;
	}
	
	/*
	*parase txop
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos,len,txop,atbm_accsii_to_int,false,exit,ret);	
	
	if((txop < 0) || (txop > 0xffff)){
		ret = -EINVAL;
		goto exit;
	}
	atbm_printk_err("%s:queue[%d],aifs[%d],cw_min[%d],cw_max[%d],txop[%d]\n",sdata->name,queue,aifs,
	cw_min,cw_max,txop);

	if(atbm_internal_edca_update(sdata,queue,aifs,cw_min,cw_max,txop) == false){
		ret = -EINVAL;
		goto exit;
	}
exit:
	if(ptr)
		atbm_kfree(ptr);

	return ret;
}
/*

queue:
	0:voice traffic
	1:video traffic
	2:best effort
	3:background trafic

u16 txop;
u16 cw_min;
u16 cw_max;
u8 aifs;

*edca_params iwpriv wlan0 common get_edca_params  
*/

static int atbm_ioctl_get_edca_params(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	int ret = 0,i,len=0;
	
	//unsigned char *ptr = NULL;
	const char queue_string[][20]={
		"voice_traffic",
		"video_traffic",
		"best_effort",
		"background_trafic"
	};
	struct edca_data{
	/* CWmin (in slots) for the access class. */
	/* [in] */ u16 cwMin;

	/* CWmax (in slots) for the access class. */
	/* [in] */ u16 cwMax;

	/* AIFS (in slots) for the access class. */
	/* [in] */ u8 aifns;

	/* TX OP Limit (in microseconds) for the access class. */
	/* [in] */ u16 txOpLimit;
	};
	//struct edca_data edca_value[4];
	
//	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
//	struct ieee80211_local *local;
//	struct atbm_common *hw_priv;
	struct atbm_vif *priv;

	if (!ieee80211_sdata_running(sdata)){
		atbm_printk_err("sdata not running \n");
		ret =  -ENETDOWN;
		goto exit1;
	}
	
//	local = sdata->local;
//	hw_priv=local->hw.priv;
	priv = (struct atbm_vif *)sdata->vif.drv_priv;
	
//	ptr = wdata->data.pointer;
	

	for(i = 0; i < 4;i++){
		atbm_printk_err("[%s] \n",queue_string[i]);
		atbm_printk_err("aifns : %d \n",priv->edca.params[i].aifns);
		atbm_printk_err("cwMax : %d \n",priv->edca.params[i].cwMax);
		atbm_printk_err("cwMin : %d \n",priv->edca.params[i].cwMin);
		atbm_printk_err("txOpLimit : %d \n",priv->edca.params[i].txOpLimit);
		if(extra){
			sprintf(extra + len,"\n[%s]\naifs:%d\ncwMax:%d\ncwMin:%d\ntxOpLimit:%d\n",queue_string[i],priv->edca.params[i].aifns,
							priv->edca.params[i].cwMax,priv->edca.params[i].cwMin,priv->edca.params[i].txOpLimit);
			len = strlen(extra);
		}
	}
	
	if(extra){	
		wrqu->data.length = strlen(extra);
	}
	
exit1:
	return ret;

}

static int atbm_ioctl_get_txposer_status(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{

	//union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct power_status_t tx_power_status;
	int ret = -1;
	
	memset(&tx_power_status, 0, sizeof(tx_power_status));
	if((ret=wsm_get_tx_power_status(hw_priv, &tx_power_status, sizeof(tx_power_status))) == 0){
		atbm_printk_always("current TX power status:b[%d/10]dB,20M gn[%d/10dB],40M gn[%d/10]dB\n",
			tx_power_status.b_txpower_delta_value_multiple_10,
			tx_power_status.gn_20m_txpower_delta_value_multiple_10,
			tx_power_status.gn_40m_txpower_delta_value_multiple_10);
	}
	else{
		atbm_printk_err("get tx power status failed\n");
	}
	if(extra){
		sprintf(extra,"\ncurrent_TX_power_status:b[%d/10]dB,20M_gn[%d/10dB],40M_gn[%d/10]dB\n",
			tx_power_status.b_txpower_delta_value_multiple_10,
			tx_power_status.gn_20m_txpower_delta_value_multiple_10,
			tx_power_status.gn_40m_txpower_delta_value_multiple_10);
		wrqu->data.length = strlen(extra);
	}
	return ret;
}

#ifdef CUSTOM_FEATURE_MAC /* To use macaddr and ps mode of customers */
extern int atbm_access_file(char *path, char *buffer, int size, int isRead);
#endif
#ifdef CONFIG_TXPOWER_DCXO_VALUE
//txpower and dcxo config file
#define STR_FILE CONFIG_TXPOWER_DCXO_VALUE
#else
#define STR_FILE "/tmp/atbm_txpwer_dcxo_cfg.txt"
#endif
/*
	@ori_deltagain：芯片efuse中的deltagain
	@cfg_delta：atbm_txpwer_dcxo_cfg.txt里配置的delta
	在芯片原有deltagain的基础上增减配置文件里的delta
	置文件里的delta有效范围：6062C：【0，31】；6162：【0，64】
	需要注意：如果efuse里原有的deltagain加上配置文件里的deltgain超出了有效范围，则使用有效范围里的极限值
*/
int CalculateFinalDeltagain(struct atbm_common *hw_priv, int ori_deltagain, int cfg_delta)
{
	int new_deltagain = 0;
	int deltagaindB = 0;
	int delta_gain = ori_deltagain;
	int delta_gain_adjust = 0; 
	int i32delta_gain = 0;
	int bitWidth = 5;
	int valid_deltagain = 16;
	int deltaGainMax = 375;
	int delta_min = -15;
	int delta_max = 15;

	if(hw_priv->chip_id == HW_CHIP_VERSION_Oceanus_FM)
	{
		bitWidth = 6;
		valid_deltagain = 32;
		delta_min = -31;
		delta_max = 31;
		deltaGainMax = 775;
	}

	delta_gain = delta_gain==valid_deltagain?0:delta_gain;
	delta_gain = N_BIT_TO_SIGNED_32BIT(delta_gain, bitWidth);

	
	if((cfg_delta >= delta_min) && (cfg_delta < 0))
	{
		delta_gain_adjust = 0 - cfg_delta;
		delta_gain_adjust = delta_gain_adjust==valid_deltagain?0:delta_gain_adjust;
		delta_gain_adjust = N_BIT_TO_SIGNED_32BIT(delta_gain_adjust, bitWidth);

		deltagaindB = (delta_gain - delta_gain_adjust)*100/4;
	}
	else if((cfg_delta >= 0) && (cfg_delta <= delta_max))
	{
		delta_gain_adjust = cfg_delta;
		delta_gain_adjust = delta_gain_adjust==valid_deltagain?0:delta_gain_adjust;
		delta_gain_adjust = N_BIT_TO_SIGNED_32BIT(delta_gain_adjust, bitWidth);

		deltagaindB = (delta_gain + delta_gain_adjust)*100/4;
		
	}
	else
	{
		atbm_printk_always("invalid cfg_delta[%d],use efuse deltagain\n", cfg_delta);
		atbm_printk_always("cfg deltagain valid value:[%d~%d]\n", delta_min, delta_max);
		return ori_deltagain;
	}

	if(deltagaindB < -deltaGainMax)
		deltagaindB = -deltaGainMax;
	else if(deltagaindB > deltaGainMax)
		deltagaindB = deltaGainMax;

	//if((deltagaindB >= -deltaGainMax) && (deltagaindB <= deltaGainMax))
	{
		if(deltagaindB >=0){
			i32delta_gain = (int)((deltagaindB)*4);
		}
		else{
			i32delta_gain = (int)((deltagaindB)*4);
		}
		new_deltagain = i32delta_gain/100;
		SIGNED_32BIT_TO_N_BIT(new_deltagain, bitWidth);
	}

	atbm_printk_always("[%d] + [%d] = [%d]\n", ori_deltagain, cfg_delta, new_deltagain);

	return new_deltagain;
}

void atbm_efuse_deltagain_copy2_array(struct efuse_headr *efuse, u8 *deltagain_arry)
{
/*
	deltagain_arry[0] = efuse->delta_gain1;
	deltagain_arry[1] = efuse->delta_gain2;
	deltagain_arry[2] = efuse->delta_gain3;
	deltagain_arry[3] = efuse->delta_gain1_5g;
	deltagain_arry[4] = efuse->delta_gain2_5g;
	deltagain_arry[5] = efuse->delta_gain3_5g;
	deltagain_arry[6] = efuse->delta_gain4_5g;
	deltagain_arry[7] = efuse->delta_gain5_5g;
	deltagain_arry[8] = efuse->delta_gain6_5g;
	deltagain_arry[9] = efuse->delta_gain7_5g;
	deltagain_arry[10] = efuse->delta_gain8_5g;
	deltagain_arry[11] = efuse->delta_gain9_5g;
	deltagain_arry[12] = efuse->delta_gain10_5g;
	*/

	memcpy(deltagain_arry, &efuse->delta_gain1, 3);
	memcpy(&deltagain_arry[3], &efuse->delta_gain1_5g, 10);

}

void atbm_efuse_print(struct efuse_headr *efuse)
{
	atbm_printk_always("specific: %d\n", efuse->specific);
	atbm_printk_always("version: %d\n", efuse->version);
	atbm_printk_always("dcxo_trim: %d\n", efuse->dcxo_trim);
	atbm_printk_always("delta_gain1: %d\n", efuse->delta_gain1);
	atbm_printk_always("delta_gain2: %d\n", efuse->delta_gain2);
	atbm_printk_always("delta_gain3: %d\n", efuse->delta_gain3);
	atbm_printk_always("delta_gain1_5g: %d\n", efuse->delta_gain1_5g);
	atbm_printk_always("delta_gain2_5g: %d\n", efuse->delta_gain2_5g);
	atbm_printk_always("delta_gain3_5g: %d\n", efuse->delta_gain3_5g);
	atbm_printk_always("delta_gain4_5g: %d\n", efuse->delta_gain4_5g);
	atbm_printk_always("delta_gain5_5g: %d\n", efuse->delta_gain5_5g);
	atbm_printk_always("delta_gain6_5g: %d\n", efuse->delta_gain6_5g);
	atbm_printk_always("delta_gain7_5g: %d\n", efuse->delta_gain7_5g);
	atbm_printk_always("delta_gain8_5g: %d\n", efuse->delta_gain8_5g);
	atbm_printk_always("delta_gain9_5g: %d\n", efuse->delta_gain9_5g);
	atbm_printk_always("delta_gain10_5g: %d\n", efuse->delta_gain10_5g);
	atbm_printk_always("Tj_room: %d\n", efuse->Tj_room);
	atbm_printk_always("pdetK1: %d\n", efuse->pdetK1);
	atbm_printk_always("pdetK2: %d\n", efuse->pdetK2);
	atbm_printk_always("pdetK3: %d\n", efuse->pdetK3);
	atbm_printk_always("mac: %02X:%02X:%02X:%02X:%02X:%02X\n", efuse->mac[0], efuse->mac[1], efuse->mac[2], 
		efuse->mac[3], efuse->mac[4], efuse->mac[5]);
}

int atbm_set_txpwr_by_atbm_txpwer_dcxo_cfg_file(struct atbm_common *hw_priv, char *pathfile, int useFlag)
{
	char readbuf[256] = "";
	int deltagain[14]={0};
	u8 efuse_deltagain[13] = {0};
	u8 delta_gains[14] = {0};
	struct efuse_headr efuse;
	int i = 0;
	int ret = 0;

	memset(&efuse, 0, sizeof(efuse));

	//use delta_gain and dcxo value in config file,when file is exist
	if(atbm_access_file(pathfile,readbuf,sizeof(readbuf),1) > 0)
	{
		atbm_printk_init("param:%s",readbuf);
		atbm_get_delta_gain(readbuf,deltagain);

		//for(i=0;i<14;i++)
			//atbm_printk_init("[%d]=%d\n", i, deltagain[i]);

		atbm_efuse_deltagain_copy2_array(&hw_priv->efuse, efuse_deltagain);
				
		if(hw_priv->chip_id == HW_CHIP_VERSION_Oceanus_FM)
		{
			//6162(Oceanus)
			if(deltagain[13]==0)
				deltagain[13] = hw_priv->efuse.dcxo_trim;

			//在efuse的deltagain基础上增加配置文件里的deltagain
			for(i=0;i<sizeof(efuse_deltagain);i++)
				delta_gains[i+1] = CalculateFinalDeltagain(hw_priv, efuse_deltagain[i], deltagain[i]);
			memcpy(&efuse.delta_gain1, &delta_gains[1], 3);
			memcpy(&efuse.delta_gain1_5g, &delta_gains[4], 10);
			//atbm_efuse_print(&hw_priv->efuse);
			//atbm_efuse_print(&efuse);

			atbm_DCXOCodeWrite(hw_priv, deltagain[13]);
		}
		else
		{
			//6062C(CronusLite)
			if(deltagain[3]==0)
				deltagain[3] = hw_priv->efuse.dcxo_trim;

			//在efuse的deltagain基础上增加配置文件里的deltagain
			for(i=0;i<3;i++)
				delta_gains[i+1] = CalculateFinalDeltagain(hw_priv, efuse_deltagain[i], deltagain[i]);

			memcpy(&efuse.delta_gain1, &delta_gains[1], 3);
			atbm_DCXOCodeWrite(hw_priv, deltagain[3]);
		}

		if(hw_priv->chip_id != HW_CHIP_VERSION_Cronus)
		{
		    i = 0;
			//0:not use deltagain and tpc
			//1:use deltagain and use tpc
			//2:not use deltagain and use tpc
			//3:use deltagain and not use tpc
			int use_deltagain_and_tpc = 2;
			//Cronuslite、Oceanus 停止固件根据信道设置deltagain，否则驱动设置的寄存器值会被固件刷掉
			ret = wsm_write_mib(hw_priv, WSM_MIB_ID_TX_HOST_CTRL_DELTAGAIN_AND_TPC, &use_deltagain_and_tpc, sizeof(use_deltagain_and_tpc), 0);
			if(ret != 0){
				atbm_printk_err("write mib failed(%d)\n", ret);
			}
            i = 0;
			if(hw_priv->chip_id == HW_CHIP_VERSION_Cronus_Lite_FM)
            	delta_gains[0] = 1; //1: 2.4G; 2: 5G; 3: 2.4G&5G
            else if(hw_priv->chip_id == HW_CHIP_VERSION_Oceanus_FM)
            	delta_gains[0] = 3; //1: 2.4G; 2: 5G; 3: 2.4G&5G

			if(0 == useFlag)
			{
				//2.4G
				memcpy(&delta_gains[1], &hw_priv->efuse.delta_gain1, 3);//2.4G deltagain
				memcpy(&delta_gains[4], &hw_priv->efuse.delta_gain1_5g, 10);//2.4G deltagain

				memcpy(&efuse, &hw_priv->efuse, sizeof(efuse));
			}
            
            ret = wsm_write_mib(hw_priv, WSM_MIB_ID_USR_TX_DELTA_GAIN_CONFIG, (void *)delta_gains, sizeof(delta_gains), 0);
            if(ret != 0){
                atbm_printk_err("write mib(0x%X) failed(%d)\n",WSM_MIB_ID_USR_TX_DELTA_GAIN_CONFIG, ret);
            }
		}
		
		atbm_etf_set_deltagain(efuse);
	}
	return 0;
}

//extern void atbm_get_delta_gain(char *srcData,int *allgain,int *bgain,int *gngain);
static int atbm_ioctl_set_txpwr_by_file(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	//union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	int i = 0, ret = -1, strheadLen = 0;
	char *pbuffer = NULL;
	char *pRxData = NULL;
	int useConfigFIle = 0;

	if(g_CertSRRCFlag >= 1)// SRRC TEST
		return -1;

	if(!(pbuffer = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		return -EINVAL;
	}
	if((ret = copy_from_user(pbuffer, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(pbuffer);
		return -EINVAL;
	}
	pbuffer[wrqu->data.length] = 0;

	for(i=0;i<wrqu->data.length;i++)
	{
		if(pbuffer[i] == ',')
		{
			strheadLen=i;
			break;
		}
	}
	
	atbm_printk_err("length=%d,data=%s,strheadLen=%d\n", wrqu->data.length, pbuffer, strheadLen);
	if (strheadLen >= wrqu->data.length){
		atbm_kfree(pbuffer);
		return -EINVAL;
	}

	pRxData = &pbuffer[strheadLen];
	atbm_printk_err("cmd:%s\n", pRxData);
	atbm_CmdLine_GetSignInteger(&pRxData, &useConfigFIle);
	
	ret = atbm_set_txpwr_by_atbm_txpwer_dcxo_cfg_file(hw_priv, STR_FILE, useConfigFIle);

	return ret;
}



static int atbm_ioctl_set_rate_txpwr_by_file(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	//union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	int i,ret = -1,strheadLen = 0;
	//int if_id = -1;
	//s8 rate_txpower[23] = {0};//validfalg,data
	char *pbuffer = NULL;
	char *pRxData = NULL;
	int useConfigFIle = 0;

	if(g_CertSRRCFlag >= 1)// SRRC TEST
		return -1;

	if(!(pbuffer = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		return -EINVAL;
	}
	if((ret = copy_from_user(pbuffer, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(pbuffer);
		return -EINVAL;
	}
	pbuffer[wrqu->data.length] = 0;

	for(i=0;i<wrqu->data.length;i++)
	{
		if(pbuffer[i] == ',')
		{
			strheadLen=i;
			break;
		}
	}
	
	atbm_printk_err("length=%d,data=%s,strheadLen=%d\n", wrqu->data.length, pbuffer, strheadLen);
	if (strheadLen >= wrqu->data.length){
		atbm_kfree(pbuffer);
		return -EINVAL;
	}

	pRxData = &pbuffer[strheadLen];
	atbm_printk_err("cmd:%s\n", pRxData);
	atbm_CmdLine_GetSignInteger(&pRxData, &useConfigFIle);
	memset(MCS_LUT_20M_delta_mode, 0, sizeof(MCS_LUT_20M_delta_mode));
	memset(MCS_LUT_20M_delta_bw, 0, sizeof(MCS_LUT_20M_delta_bw));
	memset(MCS_LUT_40M_delta_mode, 0, sizeof(MCS_LUT_40M_delta_mode));
	memset(MCS_LUT_40M_delta_bw, 0, sizeof(MCS_LUT_40M_delta_bw));

	//------------cronus
	memset(MCS_LUT_20M_delta_mode_Cronus, 0, sizeof(MCS_LUT_20M_delta_mode_Cronus));
	memset(MCS_LUT_20M_delta_bw_Cronus, 0, sizeof(MCS_LUT_20M_delta_bw_Cronus));
	memset(MCS_LUT_40M_delta_mode_Cronus, 0, sizeof(MCS_LUT_40M_delta_mode_Cronus));
	memset(MCS_LUT_40M_delta_bw_Cronus, 0, sizeof(MCS_LUT_40M_delta_bw_Cronus));
/*
	if(atbm_hw_priv_chip_id() == HW_CHIP_VERSION_Cronus)
		atbm_cronus_ladder_txpower_init();
	else
		atbm_ladder_txpower_init();
*/
	atbm_set_tx_power(hw_priv,0);
#ifdef CONFIG_RATE_TXPOWER

	if(atbm_hw_priv_chip_id() == HW_CHIP_VERSION_Cronus)
	{
		if(useConfigFIle){
			atbm_cronus_wsm_set_rate_power(hw_priv);
		}else
		{
			atbm_cronus_init_cfg_rate_power();
		}
	}
	else
	{
		if(useConfigFIle){
			wsm_set_rate_power(hw_priv);
			//atbm_printk_err("TxPwrDbg-ioctl set\n");
		}else
		{
			atbm_init_cfg_rate_power(hw_priv);
		}
	}
	
#else
	atbm_printk_err("undefine CONFIG_RATE_TXPOWER,so Not support\n");
#endif
	if(pbuffer)
		atbm_kfree(pbuffer);

	return ret;
}

static int atbm_ioctl_set_management_frame_rate(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	//union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	int i,ret = -1,strheadLen = 0;
	int if_id = -1;
	int rateIndex = 0;
	char *pbuffer = NULL;
	char *pRxData = NULL;

	if(!(pbuffer = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		return -EINVAL;
	}
	if((ret = copy_from_user(pbuffer, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(pbuffer);
		return -EINVAL;
	}
	pbuffer[wrqu->data.length] = 0;

	for(i=0;i<wrqu->data.length;i++)
	{
		if(pbuffer[i] == ',')
		{
			strheadLen=i;
			break;
		}
	}
	
	atbm_printk_err("length=%d,data=%s,strheadLen=%d\n", wrqu->data.length, pbuffer, strheadLen);
	if (strheadLen >= wrqu->data.length){
		atbm_kfree(pbuffer);
		return -EINVAL;
	}

	pRxData = &pbuffer[strheadLen];
	atbm_printk_err("cmd:%s\n", pRxData);
	atbm_CmdLine_GetSignInteger(&pRxData, &rateIndex);
	ret = wsm_write_mib(hw_priv, WSM_MIB_ID_SET_BEACON_RATE, &rateIndex, sizeof(rateIndex), if_id);
	if(ret < 0){
		atbm_printk_err("write mib failed(%d). \n", ret);
	}

	if(pbuffer)
		atbm_kfree(pbuffer);

	return ret;
}
extern int tx_rate;
static int atbm_ioctl_set_tx_rate(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	//union iwreq_data *wdata = (union iwreq_data *)wrqu;
//	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
//	struct ieee80211_local *local = sdata->local;
//	struct atbm_common *hw_priv=local->hw.priv;
	int i,ret = -1,strheadLen = 0;
//	int if_id = -1;
	int rts_th = 0;
	char *pbuffer = NULL;
	char *pRxData = NULL;

	if(!(pbuffer = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		return -EINVAL;
	}
	if((ret = copy_from_user(pbuffer, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(pbuffer);
		return -EINVAL;
	}
	pbuffer[wrqu->data.length] = 0;

	for(i=0;i<wrqu->data.length;i++)
	{
		if(pbuffer[i] == ',')
		{
			strheadLen=i;
			break;
		}
	}
	
	atbm_printk_err("length=%d,data=%s,strheadLen=%d\n", wrqu->data.length, pbuffer, strheadLen);
	if (strheadLen >= wrqu->data.length){
		atbm_kfree(pbuffer);
		return -EINVAL;
	}

	pRxData = &pbuffer[strheadLen];
	atbm_printk_err("cmd:%s\n", pRxData);
	atbm_CmdLine_GetSignInteger(&pRxData, &rts_th);

	if((rts_th > 47)||(rts_th<0)){
		atbm_printk_err("write tx_rate error(%d). \n", rts_th);
		goto __free;
	}
	tx_rate = rts_th;

	if(ret < 0){
		atbm_printk_err("write mib failed(%d). \n", ret);
	}
	
__free:
	if(pbuffer){
		atbm_kfree(pbuffer);
	}
	return ret;
}

#ifdef WLAN_CAPABILITY_RADIO_MEASURE

static int atbm_ioctl_wfa_certificate(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	char *buff_data = NULL;
	int i = 0,strheadLen = 0;
	int wfa_value = 0;
	
	buff_data = atbm_kzalloc(wrqu->data.length+1, GFP_KERNEL);

	if(buff_data == NULL){
		atbm_printk_wext("%s atbm_kzalloc error!\n",__func__);
		return  -EINVAL;
	}
	
	if(copy_from_user(buff_data, wrqu->data.pointer, wrqu->data.length)){
		atbm_printk_wext("%s copy_from_user error!\n",__func__);
		atbm_kfree(buff_data);
		return -EINVAL;
	}
	buff_data[wrqu->data.length] = 0;
	for(i=0;i<wrqu->data.length;i++)
	{
		if(buff_data[i] == ',')
		{
			strheadLen=i+1;
			break;
		}
	}
	wfa_value = buff_data[strheadLen];
	atbm_printk_wext("wfa_value = %d \n",wfa_value);
	if(wfa_value == 0x30){
		local->ext_wfa_test_flag = 0;
	}else{
		local->ext_wfa_test_flag = 1;
	}
	atbm_printk_err("%s , %s WFA certificate\n",__func__,local->ext_wfa_test_flag?"OPEN":"CLOSE");

	if(buff_data)
		atbm_kfree(buff_data);
	return 0;
}
#endif
#ifdef CONFIG_ATBM_SUPPORT_GRO
static int atbm_ioctl_napi_stats(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	char *results = NULL;
	int ret = 0;
	int total_len = 0;
	int copy_len;
	int index =0;
	int i = 0;
	int len = 0;
	char *ptr = NULL;
	char const *pos = NULL;
	char const *pos_end = NULL;
	/*
	*max is 513
	*/
	results = atbm_kzalloc(512,GFP_KERNEL);
	
	if(results == NULL){
		ret = -ENOMEM;
		goto exit;
	}
	ptr = (char *)atbm_kzalloc(wdata->data.length+1, GFP_KERNEL);
	
	if(!ptr){
		ret =  -ENOMEM;
		goto exit;
	}
	
	if(copy_from_user(ptr, wdata->data.pointer, wdata->data.length)){
		ret =  -EINVAL;
		goto exit;
	}
	
	ptr[wrqu->data.length] = 0;
	
	for(i = 0;i<wrqu->data.length;i++){
		if(ptr[i] == ',')
			ptr[i] = ATBM_SPACE;
	}
	
	pos = atbm_skip_space(ptr,wrqu->data.length+1);

	if(pos == NULL){
		ret = -EINVAL;
		goto exit;
	}

	pos = pos+strlen("napi_stats");

	pos = atbm_skip_space(pos,wrqu->data.length+1-(pos-ptr));

	if(pos == NULL){
		ret = -EINVAL;
		goto exit;
	}

	len = wrqu->data.length + 1 - (pos - ptr);

	if(len <= 0){
		ret = -EINVAL;
		goto exit;
	}

	pos_end = memchr(pos,ATBM_TAIL,len);
	
	if(pos_end != NULL)
		len = pos_end - pos+1;
	/*
	*parase queue
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos,len,index,atbm_accsii_to_int,false,exit,ret);
	
	if((index < 0) || (index >= 65)){
		ret = -EINVAL;
		goto exit;
	}
	
	copy_len = scnprintf(results+total_len,512 - total_len,"[napi_show]:\n");
	total_len += copy_len;

	for(;index < 65; index++){
		
		if(total_len > 512 - 40){
			break;
		}
		
		copy_len = scnprintf(results + total_len,512 - total_len,"napi[%d]=%lu\n",index,sdata->napi_trace[index]);
		if(copy_len > 0)
			total_len += copy_len;
		else {
			break;
		}		
	}

	if(extra){
		memcpy(extra,results,total_len);	
		wrqu->data.length = total_len + 1;
	}
	
exit:
	if(results)
		atbm_kfree(results);
	if(ptr)
		atbm_kfree(ptr);
	return ret;
}
#endif
static int atbm_ioctl_set_rts_threshold(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	//union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	int i,ret = -1,strheadLen = 0;
	int if_id = -1;
	int rts_th = 0;
	char *pbuffer = NULL;
	char *pRxData = NULL;

	if(!(pbuffer = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		return -EINVAL;
	}
	if((ret = copy_from_user(pbuffer, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(pbuffer);
		return -EINVAL;
	}
	pbuffer[wrqu->data.length] = 0;

	for(i=0;i<wrqu->data.length;i++)
	{
		if(pbuffer[i] == ',')
		{
			strheadLen=i;
			break;
		}
	}
	
	atbm_printk_err("length=%d,data=%s,strheadLen=%d\n", wrqu->data.length, pbuffer, strheadLen);
	if (strheadLen >= wrqu->data.length){
		atbm_kfree(pbuffer);
		return -EINVAL;
	}

	pRxData = &pbuffer[strheadLen];
	atbm_printk_err("cmd:%s\n", pRxData);
	atbm_CmdLine_GetSignInteger(&pRxData, &rts_th);

	
	ret = (wsm_write_mib(hw_priv, WSM_MIB_ID_DOT11_RTS_THRESHOLD,&rts_th, sizeof(rts_th), if_id));
	atbm_tool_rts_threshold = rts_th;
	

	if(ret < 0){
		atbm_printk_err("write mib failed(%d). \n", ret);
	}

	if(pbuffer)
		atbm_kfree(pbuffer);

	return ret;
}



static int atbm_ioctl_atbm_get_work_channel(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	unsigned short channel = 0;
    struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
  	channel = atbm_get_work_channel(sdata,0);
	atbm_printk_err("%s,current work channel is [%d]\n", __func__,channel);
   
    if(ext){	
		sprintf(ext,"\ncurrent work channel:%u\n",channel);
		//memcpy(ext,(char *)&channel,sizeof(channel));	
		wrqu->data.length = strlen(ext);
	}
    return 0;


}


static int atbm_ioctl_set_country_code(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{

#ifdef  CONFIG_ATBM_5G_PRETEND_2G
	atbm_printk_err(" atbm_ioctl_set_country_code:Not only 2.4G mode not support set country code! \n");
	return 0;
#else
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	unsigned char *country_code = NULL;
	char *pbuffer = NULL;
	int ret = 0,i,strheadLen = 0;
	
	if(!ieee80211_sdata_running(sdata)){
		ret = -ENETDOWN;
		goto exit;
	}
	if(!(pbuffer = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		ret = -EINVAL;
		goto exit;
	}
	if((ret = copy_from_user(pbuffer, wrqu->data.pointer, wrqu->data.length))){
		//atbm_kfree(pbuffer);
		ret = -EINVAL;
		goto exit;
	}
	pbuffer[wrqu->data.length] = 0;

	for(i=0;i<wrqu->data.length;i++)
	{
		if(pbuffer[i] == ',')
		{
			strheadLen=i;
			break;
		}
	}

	atbm_printk_err("length=%d,data=%s,strheadLen=%d\n", wrqu->data.length, pbuffer, strheadLen);
	if (strheadLen >= wrqu->data.length){
		//atbm_kfree(pbuffer);
		ret = -EINVAL;
		goto exit;
	}
	
	country_code = &pbuffer[strheadLen+1];
	//atbm_printk_err("atbm_dev_set_country_code:country_code = %c%c---------------\n",country_code[0],country_code[1]);
	

	if(atbm_set_country_code_on_driver(local,country_code) < 0){
		ret = -EINVAL;
		goto exit;
	}

	ret = 0;
	
	
exit:
	if(pbuffer)
		atbm_kfree(pbuffer);
	return ret;
#endif		
}

static int atbm_ioctl_get_country_code(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
#ifdef  CONFIG_ATBM_5G_PRETEND_2G
	atbm_printk_err("atbm_ioctl_get_country_code: Not only 2.4G mode not support set country code! \n");
	return 0;
#else
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	atbm_printk_err("atbm_dev_set_country_code:country_code = %c%c,support channel[%d]\n",
					local->country_code[0],local->country_code[1],local->country_support_chan);		
	 if(ext){	
		sprintf(ext,"\ncurrent country_code is:%c%c ,support channel[%d]\n",
					local->country_code[0],local->country_code[1],local->country_support_chan);
		wrqu->data.length = strlen(ext);
	}
	return 0;
#endif
}




static int atbm_set_power_save_mode(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	int ps_elems = 0;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	int i,strheadLen = 0;
	char *buff_data =NULL;

	if(sdata->vif.type != NL80211_IFTYPE_STATION){
		return  -EINVAL;
	}
	
	if(sdata->u.mgd.associated == NULL){
		return  -EINVAL;
	}
	
	buff_data = atbm_kzalloc(wrqu->data.length+1, GFP_KERNEL);

	if(buff_data == NULL){
		atbm_printk_wext("%s atbm_kzalloc error!\n",__func__);
		return  -EINVAL;
	}
	
	if(copy_from_user(buff_data, wrqu->data.pointer, wrqu->data.length)){
		atbm_printk_wext("%s copy_from_user error!\n",__func__);
		atbm_kfree(buff_data);
		return -EINVAL;
	}
	buff_data[wrqu->data.length] = 0;
	for(i=0;i<wrqu->data.length;i++)
	{
		if(buff_data[i] == ',')
		{
			strheadLen=i+1;
			break;
		}
	}
	
	
	ps_elems = buff_data[strheadLen];

	if (ps_elems == 0x30)
		sdata->u.mgd.powersave_enable = false;
	else
		sdata->u.mgd.powersave_enable = true;

	ieee80211_recalc_ps(local, -1);
	
	atbm_kfree(buff_data);
	return 0;
}


static int atbm_ioctl_get_cfg_txpower_by_file(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	int ret = 0;
#if defined (CONFIG_TXPOWER_DCXO_VALUE) || defined(CONFIG_RATE_TXPOWER)
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	char *results = NULL;
	int total_len = 0;

	if(g_CertSRRCFlag >= 1)// SRRC TEST
		return -1;

	total_len = atbm_internat_cmd_get_cfg_txpower(hw_priv,&results);
	
	if(extra && total_len > 0){
		
		memcpy(extra,results,total_len);	
		wrqu->data.length = total_len + 1;
	}

	if(results)
		atbm_kfree(results);
	ret = 0;
#else
	atbm_printk_err("undefine CONFIG_RATE_TXPOWER or CONFIG_TXPOWER_DCXO_VALUE,so Not support\n");
#endif

	return ret;
}


/*
iwpriv wlan0 common set_power_target,<powertar>
powertar:dBm
*/
static int atbm_ioctl_set_powerTarget(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	char *ptr = NULL;
	char const *pos = NULL;
	char const *pos_end = NULL;
	int ret = 0;
	int i   = 0;
	int powerTar = 0;
	int len = 0;
	int flag = 0;
	int neg = 0;
	u32 mcsLUTAddr = 0;
	
	if(g_CertSRRCFlag >= 1)// SRRC TEST
		return -1;

	if (!ieee80211_sdata_running(sdata)){
		ret =  -ENETDOWN;
		goto exit;
	}
	
	ptr = (char *)atbm_kzalloc(wdata->data.length+1, GFP_KERNEL);
	
	if(!ptr){
		ret =  -ENOMEM;
		goto exit;
	}
	
	if(copy_from_user(ptr, wdata->data.pointer, wdata->data.length)){
		ret =  -EINVAL;
		goto exit;
	}
	
	ptr[wrqu->data.length] = 0;
	
	for(i = 0;i<wrqu->data.length;i++){
		if(ptr[i] == ',')
			ptr[i] = ATBM_SPACE;
	}
	
	pos = atbm_skip_space(ptr,wrqu->data.length+1);

	if(pos == NULL){
		ret = -EINVAL;
		goto exit;
	}

	pos = pos+strlen("set_power_target");

	pos = atbm_skip_space(pos,wrqu->data.length+1-(pos-ptr));

	if(pos == NULL){
		ret = -EINVAL;
		goto exit;
	}

	len = wrqu->data.length + 1 - (pos - ptr);

	if(len <= 0){
		ret = -EINVAL;
		goto exit;
	}

	pos_end = memchr(pos,ATBM_TAIL,len);
	
	if(pos_end != NULL)
		len = pos_end - pos+1;

	atbm_printk_always("pos:%s\n", pos);

	for(i=0;pos[i] != '\0';i++){
		if(pos[i] == '-'){
			neg = 1;
			continue;
		}
		if(pos[i] == '.'){
			flag = 1;
			continue;
		}	
		powerTar = powerTar* 10 +(pos[i] - 0x30);
	}

	if(flag == 0)
		powerTar = powerTar * 10;

	if(powerTar > 300)
	{
		atbm_printk_err("Invalid Param\n");
		goto exit;
	}

	powerTar = (powerTar * 4)/10;
	
	if(neg)
		powerTar = 0 - powerTar;
	
	
	atbm_printk_always("powerTar:%d,0x%x\n", powerTar, powerTar);

	mcsLUTAddr = atbm_Get_MCS_LUT_Addr(WiFiMode, OFDMMode, ChBW, RateIndex);

	if(atbm_hw_priv_chip_id() == HW_CHIP_VERSION_Cronus)
		HW_WRITE_REG_BIT(mcsLUTAddr, 7, 0, (u32)powerTar);//function register
	else
		HW_WRITE_REG_BIT(mcsLUTAddr, 15, 8, (u32)powerTar);//function register

	//HW_WRITE_REG_BIT(0x0AC80CF8, 15, 8, (u32)powerTar);//force register
	
exit:
	if(ptr)
		atbm_kfree(ptr);

	return ret;
}
//iwpriv wlan0 common get_power_target
static int atbm_ioctl_get_powerTarget(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	//union iwreq_data *wdata = (union iwreq_data *)wrqu;
	char *ptr = NULL;
	//char const *pos = NULL;
	//char const *pos_end = NULL;
	int ret = 0;
	//int i   = 0;
	int powerTar = 0;
	//int len = 0;
	//int flag = 0;
	//int neg = 0;
	u32 mcsLUTAddr = 0;

	if (!ieee80211_sdata_running(sdata)){
		ret =  -ENETDOWN;
		goto exit;
	}


	mcsLUTAddr = atbm_Get_MCS_LUT_Addr(WiFiMode, OFDMMode, ChBW, RateIndex);

	if(atbm_hw_priv_chip_id() == HW_CHIP_VERSION_Cronus)
		powerTar = (s8)HW_READ_REG_BIT(mcsLUTAddr, 7, 0);//function register
	else
		powerTar = (s8)HW_READ_REG_BIT(mcsLUTAddr, 15, 8);//function register

	powerTar = (powerTar * 10)/4;

	atbm_printk_always("powerTar:%d/10\n", powerTar);
exit:
	if(ptr)
		atbm_kfree(ptr);

	return ret;
}

void get_power_by_mode(int wifi_mode, int ofdm_mode, int bw, int rateIndex)
{
	u32 mcsLUTAddr = 0;
	int power_tar = 0;

	mcsLUTAddr = atbm_Get_MCS_LUT_Addr(wifi_mode, ofdm_mode, bw, rateIndex);
	
	power_tar = HW_READ_REG_BIT(mcsLUTAddr, 15, 8);//read current pow
	atbm_printk_always("0x%x:%x\n", mcsLUTAddr, power_tar);
}

void atbm_cronus_set_tx_power(int functionIndex, int wifi_mode, int ofdm_mode, int bw, int rateIndex, int rateMax, int power)
{
	u32 mcsLUTAddr = 0;
	int index = 0;
	int i = 0;
	int power_target = 0;
	int total_delta_power = 0;

	if(functionIndex == 0)//set specific rate power
	{

		mcsLUTAddr = atbm_Get_MCS_LUT_Addr(wifi_mode, ofdm_mode, bw, rateIndex);
		index = atbm_Get_MCS_LUT_Offset_Index(wifi_mode, ofdm_mode, bw, rateIndex);

		if(bw == 0)
			power_target = MCS_LUT_20M_delta_mode_Cronus[index] + MCS_LUT_20M_delta_bw_Cronus[index] + power;
		else
			power_target = MCS_LUT_40M_delta_mode_Cronus[index-36] + MCS_LUT_40M_delta_bw_Cronus[index-36] + power;

		if(power_target > 30*4)
			power_target = 30*4;
		
		HW_WRITE_REG_BIT(mcsLUTAddr, 7, 0, (u32)power_target);//function register
		if(bw == 0)
			MCS_LUT_20M_Modify_Cronus[index] = power;
		else
			MCS_LUT_40M_Modify_Cronus[index - 36] = power;
		HW_WRITE_REG_BIT(0x0AC80CF8, 15, 8, (u32)power_target);//force register


		
	}
	else if(functionIndex == 1)//set specific mode power
	{		
		//20M
		for(i=0;i<=rateMax;i++)
		{
			index = atbm_Get_MCS_LUT_Offset_Index(wifi_mode, ofdm_mode, 0, i);
			MCS_LUT_20M_delta_mode_Cronus[index] = power;
			total_delta_power = MCS_LUT_20M_delta_bw_Cronus[index] + power;
			atbm_cronus_set_power_by_mode(wifi_mode, ofdm_mode, 0, i, MCS_LUT_20M_Modify_Cronus[index], total_delta_power, 0);//set 20M delta power
		}
		//40M
		for(i=0;i<=rateMax;i++)
		{
			index = atbm_Get_MCS_LUT_Offset_Index(wifi_mode, ofdm_mode, 1, i);
			MCS_LUT_40M_delta_mode_Cronus[index - 36] = power;
			total_delta_power = MCS_LUT_40M_delta_bw_Cronus[index - 36] + power;
			atbm_cronus_set_power_by_mode(wifi_mode, ofdm_mode, 1, i, MCS_LUT_40M_Modify_Cronus[index-36], total_delta_power, 0);//set 40M delta power
		}
	}
	else if(functionIndex == 2)//set specific bw power
	{
		for(i=0;i<36;i++)
		{
			if(bw == 0)
			{
				MCS_LUT_20M_delta_bw_Cronus[i] = power;
				total_delta_power = MCS_LUT_20M_delta_mode_Cronus[i] + power;
				atbm_cronus_set_power_by_bandwidth(bw, (i<<2), MCS_LUT_20M_Modify_Cronus[i], total_delta_power);
			}
			else
			{
				MCS_LUT_40M_delta_bw_Cronus[i] = power;
				total_delta_power = MCS_LUT_40M_delta_mode_Cronus[i] + power;
				atbm_cronus_set_power_by_bandwidth(bw, (i<<2), MCS_LUT_40M_Modify_Cronus[i], total_delta_power);
			}
		}
		
	}
}

void atbm_oceanus_set_tx_power(int functionIndex, int wifi_mode, int ofdm_mode, int bw, int rateIndex, int rateMax, int power)
{
	u32 mcsLUTAddr = 0;
	int index = 0;
	int i = 0;
	int power_target = 0;
	int total_delta_power = 0;

    s8 *pMcsLut20M = atbm_oceanus_mcs_lut_get(0);	//BW 20M	
    s8 *pMcsLut40M = atbm_oceanus_mcs_lut_get(1);	//BW 40M	
	if(functionIndex == 0)//set specific rate power
	{

		mcsLUTAddr = atbm_Get_MCS_LUT_Addr(wifi_mode, ofdm_mode, bw, rateIndex);
		index = atbm_Get_MCS_LUT_Offset_Index(wifi_mode, ofdm_mode, bw, rateIndex);

		if(bw == 0)
			power_target = MCS_LUT_20M_delta_mode[index-BLE_MCS_LUT_NUM] + MCS_LUT_20M_delta_bw[index-BLE_MCS_LUT_NUM] + power;
		else
			power_target = MCS_LUT_40M_delta_mode[index-OFDM_40M_OFFSET] + MCS_LUT_40M_delta_bw[index-OFDM_40M_OFFSET] + power;

		if(power_target > WIFI_POWER_TARGET_MAX*4)
			power_target = WIFI_POWER_TARGET_MAX*4;
		
		HW_WRITE_REG_BIT(mcsLUTAddr, 15, 8, (u32)power_target);//function register
		if(bw == 0)
			pMcsLut20M[index-BLE_MCS_LUT_NUM] = power;
		else
			pMcsLut40M[index-OFDM_40M_OFFSET] = power;
		//HW_WRITE_REG_BIT(0x0AC80CF8, 15, 8, (u32)power_target);//force register
		
	}
	else if(functionIndex == 1)//set specific mode power
	{		
		//20M
		for(i=0;i<=rateMax;i++)
		{
			index = atbm_Get_MCS_LUT_Offset_Index(wifi_mode, ofdm_mode, 0, i);
			MCS_LUT_20M_delta_mode[index-BLE_MCS_LUT_NUM] = power;
			//atbm_printk_always("MCS_LUT_20M_delta_mode[%d]=%d\n", index-BLE_MCS_LUT_NUM, MCS_LUT_20M_delta_mode[index-BLE_MCS_LUT_NUM]);
			total_delta_power = MCS_LUT_20M_delta_bw[index-BLE_MCS_LUT_NUM] + power;
			atbm_set_power_by_mode(wifi_mode, ofdm_mode, 0, i, pMcsLut20M[index-BLE_MCS_LUT_NUM], total_delta_power, 0);//set 20M delta power
		}
		//40M
		for(i=0;i<=rateMax;i++)
		{
			if(wifi_mode == ATBM_WIFI_MODE_DSSS)
				return;
			index = atbm_Get_MCS_LUT_Offset_Index(wifi_mode, ofdm_mode, 1, i);
			MCS_LUT_40M_delta_mode[index-OFDM_40M_OFFSET] = power;
			//atbm_printk_always("MCS_LUT_20M_delta_mode[%d]=%d\n", index-OFDM_40M_OFFSET, MCS_LUT_40M_delta_mode[index-OFDM_40M_OFFSET]);
			total_delta_power = MCS_LUT_40M_delta_bw[index-OFDM_40M_OFFSET] + power;
			atbm_set_power_by_mode(wifi_mode, ofdm_mode, 1, i, pMcsLut40M[index-OFDM_40M_OFFSET], total_delta_power, 0);//set 40M delta power
		}
	}
	else if(functionIndex == 2)//set specific bw power
	{
		for(i=0;i<OFDM_20M_INDEX_MAX;i++)
		{
			if(bw == 0)
			{
				MCS_LUT_20M_delta_bw[i] = power;
				total_delta_power = MCS_LUT_20M_delta_mode[i] + power;
				atbm_set_power_by_bandwidth(bw, (i<<2), pMcsLut20M[i], total_delta_power);
			}
			else
			{
				if(i<OFDM_40M_INDEX_MAX)
				{
					MCS_LUT_40M_delta_bw[i] = power;
					total_delta_power = MCS_LUT_40M_delta_mode[i] + power;
					atbm_set_power_by_bandwidth(bw, (i<<2), pMcsLut40M[i], total_delta_power);
				}
			}
		}
		
	}
}


/*
iwpriv wlan0 common set_txpower,<function_index>,<mode>,<rate>,<bw>,<power>
power:dBm
*/
static int atbm_ioctl_set_txpower(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	char *ptr = NULL;
	char const *pos = NULL;
	char const *pos_end = NULL;
	int ret = 0;
	int i   = 0;
//	int index = 0;
	int functionIndex = 0;
	int mode = 0;
	int wifi_mode = 0;
	int ofdm_mode = 0;
	int rateIndex = 0;
	int rateMin = 0;
	int rateMax = 0;
	int bw = 0;
	int power = 0;
//	int power_target = 0;
//	int total_delta_power = 0;
	int len = 0;
	//int flag = 0;
	int neg = 0;
//	u32 mcsLUTAddr = 0;

	if(g_CertSRRCFlag >= 1)// SRRC TEST
		return -1;

	if (!ieee80211_sdata_running(sdata)){
		ret =  -ENETDOWN;
		goto exit;
	}
	
	ptr = (char *)atbm_kzalloc(wdata->data.length+1, GFP_KERNEL);
	
	if(!ptr){
		ret =  -ENOMEM;
		goto exit;
	}
	
	if(copy_from_user(ptr, wdata->data.pointer, wdata->data.length)){
		ret =  -EINVAL;
		goto exit;
	}
	
	ptr[wrqu->data.length] = 0;
	
	for(i = 0;i<wrqu->data.length;i++){
		if(ptr[i] == ',')
			ptr[i] = ATBM_SPACE;
	}
	
	pos = atbm_skip_space(ptr,wrqu->data.length+1);

	if(pos == NULL){
		ret = -EINVAL;
		goto exit;
	}

	pos = pos+strlen("set_txpower");

	pos = atbm_skip_space(pos,wrqu->data.length+1-(pos-ptr));

	if(pos == NULL){
		ret = -EINVAL;
		goto exit;
	}

	len = wrqu->data.length + 1 - (pos - ptr);

	if(len <= 0){
		ret = -EINVAL;
		goto exit;
	}

	pos_end = memchr(pos,ATBM_TAIL,len);
	
	if(pos_end != NULL)
		len = pos_end - pos+1;

	atbm_printk_always("pos:%s\n", pos);

	/*
	*parase functionIndex
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos,len,functionIndex,atbm_accsii_to_int,false,exit,ret);
	if((functionIndex < 0) || (functionIndex > 2)){
		atbm_printk_err("Invalid function index\n");
		ret = -EINVAL;
		goto exit;
	}

	switch(functionIndex){
	case 0:
		/*
		*parase mode
		*/
		ATBM_WEXT_PROCESS_PARAMS(pos,len,mode,atbm_accsii_to_int,false,exit,ret);

		/*
		*parase rateIndex
		*/
		ATBM_WEXT_PROCESS_PARAMS(pos,len,rateIndex,atbm_accsii_to_int,false,exit,ret);

		/*
		*parase bw
		*/
		ATBM_WEXT_PROCESS_PARAMS(pos,len,bw,atbm_accsii_to_int,false,exit,ret);
		
		/*
		*parase power
		*/
		ATBM_WEXT_PROCESS_PARAMS(pos,len,power,atbm_accsii_to_10Xfloat,false,exit,ret);
		break;
	case 1:
		/*
		*parase mode
		*/
		ATBM_WEXT_PROCESS_PARAMS(pos,len,mode,atbm_accsii_to_int,false,exit,ret);

		/*
		*parase bw
		*/
		//ATBM_WEXT_PROCESS_PARAMS(pos,len,bw,atbm_accsii_to_int,false,exit,ret);
		
		/*
		*parase power
		*/
		ATBM_WEXT_PROCESS_PARAMS(pos,len,power,atbm_accsii_to_10Xfloat,false,exit,ret);
		break;
	case 2:
		/*
		*parase bw
		*/
		ATBM_WEXT_PROCESS_PARAMS(pos,len,bw,atbm_accsii_to_int,false,exit,ret);
		
		/*
		*parase power
		*/
		ATBM_WEXT_PROCESS_PARAMS(pos,len,power,atbm_accsii_to_10Xfloat,false,exit,ret);
		break;
	default:
		atbm_printk_err("Invalid function index\n");
		break;
	}

	power = (power * 4)/10;
	
	if(neg)
		power = 0 - power;
	
	atbm_printk_always("power:%d,0x%x\n", power, power);
	switch(mode){
	case 0://DSSS
		wifi_mode = ATBM_WIFI_MODE_DSSS;
		rateMin = 0;
		rateMax = 3;
		break;
	case 1://LM
		wifi_mode  = ATBM_WIFI_MODE_OFDM;
		ofdm_mode = ATBM_WIFI_OFDM_MD_LM;
		rateMin = 0;
		rateMax = 7;
		break;
	case 2://MM
		wifi_mode  = ATBM_WIFI_MODE_OFDM;
		ofdm_mode = ATBM_WIFI_OFDM_MD_MM;
		rateMin = 0;
		rateMax = 7;
		break;
	case 3://HE-SU
		wifi_mode  = ATBM_WIFI_MODE_OFDM;
		ofdm_mode = ATBM_WIFI_OFDM_MD_HE_SU;
		rateMin = 0;
		rateMax = 11;
		break;
	case 4://VHT
		wifi_mode  = ATBM_WIFI_MODE_OFDM;
		ofdm_mode = ATBM_WIFI_OFDM_MD_VHT;
		rateMin = 0;
		rateMax = 9;//power表中20M/40M都有mcs9的寄存器
		break;
	default:
		break;
	}


	if((mode < 0) || (mode > 4)){
		atbm_printk_err("Invalid mode index\n");
		ret = -EINVAL;
		goto exit;
	}

	if((rateIndex < rateMin) || (rateIndex > rateMax)){
		atbm_printk_err("Invalid rate index\n");
		ret = -EINVAL;
		goto exit;
	}

	bw = bw?1:0;

	atbm_printk_always("power:%d,0x%x\n", power, power);

	memset(rate_txpower_cfg, 0, sizeof(rate_txpower_cfg));//clear config set power
	if(atbm_hw_priv_chip_id() == HW_CHIP_VERSION_Cronus)
		atbm_cronus_set_tx_power(functionIndex, wifi_mode, ofdm_mode, bw, rateIndex, rateMax, power);
	else
		atbm_oceanus_set_tx_power(functionIndex, wifi_mode, ofdm_mode, bw, rateIndex, rateMax, power);
exit:
	if(ptr)
		atbm_kfree(ptr);

	return ret;
}

/*
	get current power config table
	iwpriv wlan0 common set_txpower,<function_index>,<mode>,<rate>,<bw>
*/
static int atbm_ioctl_get_txpower(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	int ret = 0;

	if(g_CertSRRCFlag >= 1)// SRRC TEST
		return -1;
	if(atbm_hw_priv_chip_id() == HW_CHIP_VERSION_Cronus)
		ret = atbm_cronus_internat_cmd_get_txpower(NULL,NULL);
	else
		ret = atbm_internat_cmd_get_txpower(NULL,NULL);
	return ret;
}


static int atbm_ioctl_set_ladder_txpower(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	int i,strheadLen = 0;
	char *buff_data =NULL;
//	int ps_elems = 0;
	int ret = 0,ladder_value = 0;
	struct atbm_common *hw_priv=local->hw.priv;
	if(g_CertSRRCFlag >= 1){// SRRC TEST
		atbm_printk_err("%s g_CertSRRCFlag=%d!\n",__func__,g_CertSRRCFlag);
		return -1;
	}
	
	buff_data = atbm_kzalloc(wrqu->data.length+1, GFP_KERNEL);

	if(buff_data == NULL){
		atbm_printk_err("%s atbm_kzalloc error!\n",__func__);
		ret = -1;
		goto err;
	}
	
	if(copy_from_user(buff_data, wrqu->data.pointer, wrqu->data.length)){
		atbm_printk_err("%s copy_from_user error!\n",__func__);
		ret = -1;
		goto err;
	}
	buff_data[wrqu->data.length] = 0;
	for(i=0;i<wrqu->data.length;i++)
	{
		if(buff_data[i] == ',')
		{
			strheadLen=i+1;
			break;
		}
	}
	
	if(false == atbm_accsii_to_int(&buff_data[strheadLen],wrqu->data.length - strheadLen - 1,&ladder_value)){
		
		atbm_printk_err("power_value = %d \n",ladder_value);
		ret = -1;
		goto err;
	}
	atbm_printk_err("ladder_value=%d \n",ladder_value);
	if(atbm_hw_priv_chip_id() == HW_CHIP_VERSION_Cronus)
		atbm_cronus_init_cfg_rate_power();
	else
		atbm_init_cfg_rate_power(hw_priv);
	switch(ladder_value){
		case 0:{
			if(atbm_hw_priv_chip_id() == HW_CHIP_VERSION_Cronus)
				atbm_cronus_ladder_txpower_init();
			else
				atbm_ladder_txpower_init();
			}break;
		case 15:{
			if(atbm_hw_priv_chip_id() == HW_CHIP_VERSION_Cronus)
				atbm_cronus_ladder_txpower_15();
			else
				atbm_ladder_txpower_15();
			}break;
		case 63:{
			if(atbm_hw_priv_chip_id() == HW_CHIP_VERSION_Cronus)
				atbm_cronus_ladder_txpower_63();
			else
				atbm_ladder_txpower_63();
			}break;
		default:{
			atbm_printk_err("atbm_ioctl_set_ladder_txpower:%d err,only support=>0,15,63\n",ladder_value);
			ret = -1;
			}break;
	}
	atbm_set_tx_power(hw_priv,ladder_value);
err:
	if(buff_data){
		atbm_kfree(buff_data);
		buff_data = NULL;
	}
	return 0;
}
/*
	range [-8:8]

*/
static int atbm_ioctl_set_rate_txpower_mode(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	//struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
//	struct ieee80211_local *local = sdata->local;
	int i,strheadLen = 0;
	char *buff_data =NULL;
//	int ps_elems = 0;
	int power_value = 0,ret = 0;

	if(g_CertSRRCFlag >= 1)// SRRC TEST
		return -1;
	
	buff_data = atbm_kzalloc(wrqu->data.length+1, GFP_KERNEL);

	if(buff_data == NULL){
		atbm_printk_wext("%s atbm_kzalloc error!\n",__func__);
		return  -EINVAL;
	}
	
	if(copy_from_user(buff_data, wrqu->data.pointer, wrqu->data.length)){
		atbm_printk_wext("%s copy_from_user error!\n",__func__);
		atbm_kfree(buff_data);
		return -EINVAL;
	}
	buff_data[wrqu->data.length] = 0;
	for(i=0;i<wrqu->data.length;i++)
	{
		if(buff_data[i] == ',')
		{
			strheadLen=i+1;
			break;
		}
	}
	
	
	//ps_elems = buff_data[strheadLen];
	atbm_printk_err("wrqu->data.length = %d,buff_data[%d]=%c \n",wrqu->data.length,strheadLen,buff_data[strheadLen]);
	if(false == atbm_accsii_to_int(&buff_data[strheadLen],wrqu->data.length - strheadLen - 1,&power_value)){
		
		atbm_printk_err("power_value = %d \n",power_value);
		ret = -1;
		goto err;
	}
	
	atbm_printk_err("power_value = %d \n",power_value);
#if 0
	if(power_value <= 16 && power_value >= -16){
		power_value = power_value * 2;
	}else{
		atbm_printk_err("power_value %d overflow!\n",power_value);
		ret = -1;
		goto err;
	}
	atbm_printk_err("power_value = %d \n",power_value);
	
    s8 *pMcsLut20M = atbm_oceanus_mcs_lut_get(0);	//BW 20M	
    s8 *pMcsLut40M = atbm_oceanus_mcs_lut_get(1);	//BW 40M	
	for(i=0;i<36;i++){
		/*
			set HT20 all rate power
		*/
		MCS_LUT_20M_delta_bw[i] = power_value;
		total_delta_power = MCS_LUT_20M_delta_mode[i] + power_value;
		atbm_set_power_by_bandwidth(0, (i<<2), pMcsLut20M[i], total_delta_power);
		
#ifndef ATBM_NOT_SUPPORT_40M_CHW
		/*
			set HT40 all rate power
		*/
		MCS_LUT_40M_delta_bw[i] = power_value;
		total_delta_power = MCS_LUT_40M_delta_mode[i] + power_value;
		atbm_set_power_by_bandwidth(1, (i<<2), pMcsLut40M[i], total_delta_power);
#endif
	}
#endif	
	if(atbm_hw_priv_chip_id() == HW_CHIP_VERSION_Cronus)
		ret = atbm_cronus_set_txpower_mode(power_value);
	else
		ret = atbm_set_txpower_mode(power_value);
err:
	if(buff_data){
		atbm_kfree(buff_data);
		buff_data = NULL;
	}
	return ret;

}

static int atbm_ioctl_ctrl_use_deltagain_and_tpc(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	//union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	int i,ret = -1,strheadLen = 0;
	char *pbuffer = NULL;
	char *pRxData = NULL;

	//0:not use deltagain and tpc
    //1:use deltagain and use tpc
    //2:not use deltagain and use tpc
    //3:use deltagain and not use tpc
	int use_deltagain_and_tpc = 0;

	if(!(pbuffer = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		return -EINVAL;
	}
	if((ret = copy_from_user(pbuffer, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(pbuffer);
		return -EINVAL;
	}
	pbuffer[wrqu->data.length] = 0;

	for(i=0;i<wrqu->data.length;i++)
	{
		if(pbuffer[i] == ',')
		{
			strheadLen=i;
			break;
		}
	}
	
	atbm_printk_err("length=%d,data=%s,strheadLen=%d\n", wrqu->data.length, pbuffer, strheadLen);
	if (strheadLen >= wrqu->data.length){
		atbm_kfree(pbuffer);
		return -EINVAL;
	}

	pRxData = &pbuffer[strheadLen];
	atbm_printk_err("cmd:%s\n", pRxData);
	atbm_CmdLine_GetSignInteger(&pRxData, &use_deltagain_and_tpc);

	ret = wsm_write_mib(hw_priv, WSM_MIB_ID_TX_HOST_CTRL_DELTAGAIN_AND_TPC, &use_deltagain_and_tpc, sizeof(use_deltagain_and_tpc), 0);
	if(ret != 0){
		atbm_printk_err("write mib failed(%d)\n", ret);
	}

	if(pbuffer)
		atbm_kfree(pbuffer);

	return ret;
}

static int atbm_ioctl_set_power_threshold(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	//union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	int i,ret = -1,strheadLen = 0;
	char *pbuffer = NULL;
	char *pRxData = NULL;
	int tx_power_max_thr_5G = 0;
	int rssi_decide_tx_thr_5G = 0;
	int tx_power_max_thr_2G = 0;
	int rssi_thr_for_rx_low_energy = 0;
	u8 param[4] = {0};
	
	if(!(pbuffer = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		return -EINVAL;
	}
	if((ret = copy_from_user(pbuffer, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(pbuffer);
		return -EINVAL;
	}
	pbuffer[wrqu->data.length] = 0;

	for(i=0;i<wrqu->data.length;i++)
	{
		if(pbuffer[i] == ',')
		{
			strheadLen=i;
			break;
		}
	}

	pRxData = &pbuffer[strheadLen];
	atbm_printk_err("cmd:%s\n", pRxData);
	atbm_CmdLine_GetSignInteger(&pRxData, &tx_power_max_thr_5G);
	atbm_CmdLine_GetSignInteger(&pRxData, &rssi_decide_tx_thr_5G);
	atbm_CmdLine_GetSignInteger(&pRxData, &tx_power_max_thr_2G);
	atbm_CmdLine_GetSignInteger(&pRxData, &rssi_thr_for_rx_low_energy);

	param[0] = tx_power_max_thr_5G;
	param[1] = rssi_decide_tx_thr_5G;
	param[2] = tx_power_max_thr_2G;
	param[3] = rssi_thr_for_rx_low_energy;
	
	//固件宏 REDUCE_TX_POWER_CONSUMPTION 打开才有效
	ret = wsm_write_mib(hw_priv, WSM_MIB_ID_TARGET_POWER_THRESHOLD, &param[0], sizeof(param), 0);
	if(ret != 0){
		atbm_printk_err("write mib failed(%d)\n", ret);
	}

	if(pbuffer)
		atbm_kfree(pbuffer);

	return ret;
}

static int atbm_ioctl_set_reduce_tx_rx_power_consumption(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	//union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	int i,ret = -1,strheadLen = 0;
	char *pbuffer = NULL;
	char *pRxData = NULL;
	int tx_power_consumption = 0;
	int rx_power_consumption = 0;
	u8 param[2] = {0};

	if(!(pbuffer = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		return -EINVAL;
	}

	if((ret = copy_from_user(pbuffer, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(pbuffer);
		return -EINVAL;
	}

	pbuffer[wrqu->data.length] = 0;

	for(i=0;i<wrqu->data.length;i++)
	{
		if(pbuffer[i] == ',')
		{
			strheadLen=i;
			break;
		}
	}

	atbm_printk_err("length=%d,data=%s,strheadLen=%d\n", wrqu->data.length, pbuffer, strheadLen);
	if (strheadLen >= wrqu->data.length){
		atbm_kfree(pbuffer);
		return -EINVAL;
	}

	pRxData = &pbuffer[strheadLen];
	atbm_printk_err("cmd:%s\n", pRxData);
	atbm_CmdLine_GetSignInteger(&pRxData, &tx_power_consumption);
	atbm_CmdLine_GetSignInteger(&pRxData, &rx_power_consumption);

	param[0] = tx_power_consumption;
	param[1] = rx_power_consumption;

	ret = wsm_write_mib(hw_priv, WSM_MIB_ID_REDUCE_TX_RX_POWER_CONSUMPTION, &param, sizeof(param), 0);
	if(ret != 0){
		atbm_printk_err("write mib failed(%d)\n", ret);
	}

	if(pbuffer)
		atbm_kfree(pbuffer);

	return ret;
}

//整机校准时，补偿deltagain
/*
	iwpriv wlan0 common set_deltagain,<deltagain_4x>
	@deltagain_4x:6162:[-31,31]; 6062C:[-15,15];
*/
static int atbm_ioctl_set_calibrate_deltagain(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	//union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	int i,ret = -1,strheadLen = 0;
	char *pbuffer = NULL;
	char *pRxData = NULL;
	int deltagain_4x = 0;
	int deltagain_min = -15;
	int deltagain_max = 15;

	if(!(pbuffer = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		return -EINVAL;
	}

	if((ret = copy_from_user(pbuffer, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(pbuffer);
		return -EINVAL;
	}

	pbuffer[wrqu->data.length] = 0;

	for(i=0;i<wrqu->data.length;i++)
	{
		if(pbuffer[i] == ',')
		{
			strheadLen=i;
			break;
		}
	}

	atbm_printk_err("length=%d,data=%s,strheadLen=%d\n", wrqu->data.length, pbuffer, strheadLen);
	if (strheadLen >= wrqu->data.length){
		atbm_kfree(pbuffer);
		return -EINVAL;
	}

	pRxData = &pbuffer[strheadLen];
	atbm_printk_err("cmd:%s\n", pRxData);
	atbm_CmdLine_GetSignInteger(&pRxData, &deltagain_4x);

	if(hw_priv->chip_id == HW_CHIP_VERSION_Oceanus_FM)
	{
		deltagain_min = -31;
		deltagain_max = 31;
	}

	deltagain_4x = max(min(deltagain_4x, deltagain_max), deltagain_min);

	HW_WRITE_REG_BIT(0x0ACB8B28, 6, 0, deltagain_4x);

	if(pbuffer)
		atbm_kfree(pbuffer);

	return ret;
}

//整机校准时校准后verify tx，做温度补偿
/*
	iwpriv wlan0 common set_tpc,<tjroom>
	@tjroom:最后一个信道校准时获取的实际温度
*/
static int atbm_ioctl_set_calibrate_TPC(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	//union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	int i,ret = -1,strheadLen = 0;
	char *pbuffer = NULL;
	char *pRxData = NULL;
	int tjroom = 0;
	int digGain_TPC = 0;
	int delta_gain_multiple_1000 = 0;
	int tpc_min = -64;
	int tpc_max = 63;
	struct Tjroom_temperature_t tjroom_temp;

	if(!(pbuffer = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		return -EINVAL;
	}

	if((ret = copy_from_user(pbuffer, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(pbuffer);
		return -EINVAL;
	}

	pbuffer[wrqu->data.length] = 0;

	for(i=0;i<wrqu->data.length;i++)
	{
		if(pbuffer[i] == ',')
		{
			strheadLen=i;
			break;
		}
	}

	atbm_printk_err("length=%d,data=%s,strheadLen=%d\n", wrqu->data.length, pbuffer, strheadLen);
	if (strheadLen >= wrqu->data.length){
		atbm_kfree(pbuffer);
		return -EINVAL;
	}

	pRxData = &pbuffer[strheadLen];
	atbm_printk_err("cmd:%s\n", pRxData);
	atbm_CmdLine_GetSignInteger(&pRxData, &tjroom);

	memset(&tjroom_temp, 0, sizeof(tjroom_temp));

	//-------------获取当前温度
	if((ret = wsm_get_Tjroom_temperature(hw_priv, &tjroom_temp, sizeof(tjroom_temp))) == 0)
	{
		atbm_printk_always("treading:%d,stempC:%d\n", tjroom_temp.tempC, tjroom_temp.stempC);
	}
	else
	{
		atbm_printk_err("get Tjroom failed\n");
		tjroom_temp.stempC = tjroom;
	}


	if (hw_priv->etf_channel >= 36)
		delta_gain_multiple_1000 = (56*4);
	else
	{
		if(hw_priv->chip_id == HW_CHIP_VERSION_Oceanus_FM)
			delta_gain_multiple_1000 = (21*4);
		else
			delta_gain_multiple_1000 = (17*4);
	}

	digGain_TPC = (tjroom_temp.stempC - tjroom)*delta_gain_multiple_1000/1000;
	digGain_TPC = min(max(tpc_min, digGain_TPC), tpc_max);
	
	HW_WRITE_REG_BIT(0x0ACB8B2C, 6, 0, digGain_TPC);

	if(pbuffer)
		atbm_kfree(pbuffer);

	return ret;
}

//设置DCXO
/*
	iwpriv wlan0 common set_dcxo,<dcxo>
	@dcxo:[0,127]
*/
static int atbm_ioctl_set_dcxo(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	//union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	int i,ret = -1,strheadLen = 0;
	char *pbuffer = NULL;
	char *pRxData = NULL;
	int dcxo = 0;
	int dcxo_min = 0;
	int dcxo_max = 127;

	if(!(pbuffer = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		return -EINVAL;
	}

	if((ret = copy_from_user(pbuffer, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(pbuffer);
		return -EINVAL;
	}

	pbuffer[wrqu->data.length] = 0;

	for(i=0;i<wrqu->data.length;i++)
	{
		if(pbuffer[i] == ',')
		{
			strheadLen=i;
			break;
		}
	}

	atbm_printk_err("length=%d,data=%s,strheadLen=%d\n", wrqu->data.length, pbuffer, strheadLen);
	if (strheadLen >= wrqu->data.length){
		atbm_kfree(pbuffer);
		return -EINVAL;
	}

	pRxData = &pbuffer[strheadLen];
	atbm_printk_err("cmd:%s\n", pRxData);
	atbm_CmdLine_GetSignInteger(&pRxData, &dcxo);

	dcxo = min(max(dcxo_min, dcxo), dcxo_max);
	
	HW_WRITE_REG_BIT(0x16101410, 6, 0, dcxo);

	if(pbuffer)
		atbm_kfree(pbuffer);

	return ret;
}

//iwpriv wlan0 common get_dcxo
static int atbm_ioctl_get_dcxo(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	//union iwreq_data *wdata = (union iwreq_data *)wrqu;
	//struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	//struct ieee80211_local *local = sdata->local;
	//struct atbm_common *hw_priv=local->hw.priv;
	//int i,ret = -1,strheadLen = 0;
	//char *pbuffer = NULL;
	//char *pRxData = NULL;
	int dcxo = 0;


	dcxo = HW_READ_REG_BIT(0x16101410, 6, 0);
	
	atbm_printk_always("current dcxo=%d\n", dcxo);

	return 0;
}

struct wsm_2AntExtraIoSet {
	u8 io22ToSet;
	u8 io23ToSet;
};

static inline int wsm_set_2AntExtraIo(struct atbm_common *hw_priv,
					const struct wsm_2AntExtraIoSet *arg,
					int if_id)
{
	struct {
	       u8	io22ToSet;
	       u8	io23ToSet;
	       u16	reserved;
	} val;

	val.io22ToSet = arg->io22ToSet;
	val.io23ToSet = arg->io23ToSet;
	val.reserved = 0;

	return wsm_write_mib(hw_priv, WSM_MIB_ID_SET_2ANT_EXTRA_IO, &val,
			     sizeof(val), if_id);
}

static int wsm_get_2AntExtraIo(struct atbm_common *hw_priv,
					struct wsm_2AntExtraIoSet *arg,
					int if_id)
{
	u32 outVal = 0;
	u8 ret = 0;
	
	ret = wsm_read_mib(hw_priv, WSM_MIB_ID_GET_2ANT_EXTRA_IO, &outVal,
			     sizeof(outVal), if_id);
	arg->io22ToSet = (u8)(outVal&0x1);
	arg->io23ToSet = (u8)((outVal&0x2)>>1);
	return ret;
}


/*
1(write),2(read)
iwpriv wlan0 common 2ant_extraio_ctl,1,0,1 
Set IO 22, 23 output value to 0 and 1 respectively if 1st parameter is 1(write)
*/

static int atbm_ioctl_2AntExtraIoSet(struct net_device *dev, struct iw_request_info *ifno, union iwreq_data *wrqu, char *ext)
{

	int i = 0;
	int ret = 0;
	char *extra = NULL;
	char *pRxData = NULL;
	unsigned int bWrite=0;
	unsigned int io22ToSet = 0;
	unsigned int io23ToSet = 0;
	char *fwCmdBuf = NULL;
	struct wsm_2AntExtraIoSet TwoAntIo={255,255};

	
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;

	struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *vif;
	if(!(extra = atbm_kmalloc(wrqu->data.length+1,GFP_KERNEL)))
		return -ENOMEM;
	if(copy_from_user(extra, wrqu->data.pointer, wrqu->data.length)){
		atbm_kfree(extra);
		return -EINVAL;
	}

	if(wrqu->data.length <= 1){
		atbm_printk_err("invalid parameter!\n");
		atbm_printk_err("e.g:./iwpriv wlan0 fwcmd intr\n");
		atbm_kfree(extra);
		return -EINVAL;
	}

	for(i=0;i<wrqu->data.length;i++){
		if(extra[i] == ',')
		{
			//atbm_printk_err("i-%d\n",i);
			extra[i] =' ';
		}
	}

	if(extra[0] == ' '){
		atbm_printk_err("invalid parameter!\n");
		atbm_kfree(extra);
		return -EINVAL;
	}
	atbm_printk_debug("2AntIoSet:extra = %s  %d\n", extra, wrqu->data.length);
	//fwcmd_common_deal(sdata,extra,wrqu->data.length);
	pRxData = &extra[16+1];
	atbm_CmdLine_GetInteger(&pRxData, &bWrite);

	pRxData = &extra[16+1+2];
	atbm_CmdLine_GetInteger(&pRxData, &io22ToSet);
	pRxData = &extra[16+1+2+2];
	atbm_CmdLine_GetInteger(&pRxData, &io23ToSet);

	atbm_printk_always("bWrite=%d,io22ToSet=%d,io23ToSet=%d\n",bWrite,io22ToSet,io23ToSet);

	if(((io22ToSet !=0)&&(io22ToSet !=1)) || ((io23ToSet !=0)&&(io23ToSet !=1)))
	{
		ret = -2;
		atbm_printk_err("invalid value of io22ToSet or io23ToSet\n");
		goto end;
	}


	i = 0;
	atbm_for_each_vif(hw_priv,vif,i){
		if (vif != NULL)
		{
			atbm_printk_wext("exttra = %s\n", extra);
			/*
			WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_SET_2ANT_EXTRA_IO,
				extra, wrqu->data.length, vif->if_id));
			break;
			*/
			if(bWrite == 1)
			{
				TwoAntIo.io22ToSet = io22ToSet;
				TwoAntIo.io23ToSet = io23ToSet;
				atbm_printk_debug("write IO:%d,%d,id-%d\n",TwoAntIo.io22ToSet, TwoAntIo.io23ToSet,vif->if_id);
				wsm_set_2AntExtraIo(hw_priv, &TwoAntIo, vif->if_id);
			}
			else if(bWrite == 0)
			{
				wsm_get_2AntExtraIo(hw_priv, &TwoAntIo, vif->if_id);
				atbm_printk_debug("read IO:%d,%d,id-%d\n",TwoAntIo.io22ToSet, TwoAntIo.io23ToSet,vif->if_id);
			}
			else
				atbm_printk_err("invalid parameter\n");
		}
	}
end:
	atbm_kfree(extra);
	return ret;
	
}


static int atbm_ioctl_set_ampdu(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	char *pbuffer =NULL;
	int ampdu_value = 0;
	int ret = 0,i,strheadLen = 0;
	char *pRxData = NULL;
	
	if(!(pbuffer = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		ret = -EINVAL;
		goto exit;
	}
	if((ret = copy_from_user(pbuffer, wrqu->data.pointer, wrqu->data.length))){
		ret = -EINVAL;
		goto exit;
	}
	pbuffer[wrqu->data.length] = 0;

	for(i=0;i<wrqu->data.length;i++)
	{
		if(pbuffer[i] == ',')
		{
			strheadLen=i;
			break;
		}
	}
	if (strheadLen >= wrqu->data.length){
		ret = -EINVAL;
		goto exit;
	}
	
	pRxData = &pbuffer[strheadLen+1];
	atbm_CmdLine_GetSignInteger(&pRxData, &ampdu_value);

	atbm_printk_err("atbm_ioctl_reset_ampdu = %d \n",ampdu_value);
	atbm_internal_wsm_set_ampdu(sdata,ampdu_value);
	
exit:
	if(pbuffer)
		atbm_kfree(pbuffer);
	return ret;

}

static int atbm_ioctl_set_single_scan_time(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct ieee80211_hw *hw = &local->hw;
	struct atbm_common *hw_priv = (struct atbm_common *) hw->priv;
	char *pbuffer =NULL;
	int scan_value = 0;
	int ret = 0,i,strheadLen = 0;
	char *pRxData = NULL;
	
	if(!(pbuffer = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		ret = -EINVAL;
		goto exit;
	}
	if((ret = copy_from_user(pbuffer, wrqu->data.pointer, wrqu->data.length))){
		ret = -EINVAL;
		goto exit;
	}
	pbuffer[wrqu->data.length] = 0;

	for(i=0;i<wrqu->data.length;i++)
	{
		if(pbuffer[i] == ',')
		{
			strheadLen=i;
			break;
		}
	}
	if (strheadLen >= wrqu->data.length){
		ret = -EINVAL;
		goto exit;
	}
	
	pRxData = &pbuffer[strheadLen+1];
	atbm_CmdLine_GetSignInteger(&pRxData, &scan_value);

	atbm_printk_err("atbm_scan_value = %d \n",scan_value);
	hw_priv->single_channel_scan_reduce_time = scan_value;
	
exit:
	if(pbuffer)
		atbm_kfree(pbuffer);
	return ret;

}
#ifdef ATBM_USE_FASTLINK
static int atbm_ioctl_reset_fastlink(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	char *pbuffer =NULL;
	int reset_value = 0;
	int ret = 0,i,strheadLen = 0;
	char *pRxData = NULL;
	
	if(!(pbuffer = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		ret = -EINVAL;
		goto exit;
	}
	if((ret = copy_from_user(pbuffer, wrqu->data.pointer, wrqu->data.length))){
		ret = -EINVAL;
		goto exit;
	}
	pbuffer[wrqu->data.length] = 0;

	for(i=0;i<wrqu->data.length;i++)
	{
		if(pbuffer[i] == ',')
		{
			strheadLen=i;
			break;
		}
	}
	if (strheadLen >= wrqu->data.length){
		ret = -EINVAL;
		goto exit;
	}
	
	pRxData = &pbuffer[strheadLen+1];
	atbm_CmdLine_GetSignInteger(&pRxData, &reset_value);

	atbm_printk_err("atbm_ioctl_reset_fastlink = %d \n",reset_value);
	sdata->fastlink_enable = reset_value;
	
exit:
	if(pbuffer)
		atbm_kfree(pbuffer);
	return ret;

}

#endif

static int atbm_ioctl_get_pa_status(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{

	int ret = 0;
	int powerTar_40x = 0;
	u32 mcsLUTAddr = 0;
	int power_limit_40x = 90;//2.25dBm


	

	mcsLUTAddr = atbm_Get_MCS_LUT_Addr(WiFiMode, OFDMMode, ChBW, RateIndex);
		
	powerTar_40x = (s8)HW_READ_REG_BIT(mcsLUTAddr, 15, 8);//function register

	powerTar_40x = (powerTar_40x * 10);

	if(WiFiMode == ATBM_WIFI_MODE_DSSS)
		atbm_printk_always("PA off\n");
	else{
		if(ChBW == ATBM_WIFI_BW_40M)
			power_limit_40x = -50;//-1.25dBm
		if(powerTar_40x < power_limit_40x)
			atbm_printk_always("PA off\n");
		else
			atbm_printk_always("PA on\n");
	}

	return ret;
}


#ifdef CONFIG_ATBM_GET_GPIO4
u8 io2pin[12] = {24, 25, 9, 8, 7, 6, 5, 4, 16, 17, 18, 19};//gpio0 -- gpio11
u16 muxOffset[6] = {0x34, 0x14, 0x10, 0x0C, 0x24, 0x28};

bool gpio_input_value(struct atbm_common *hw_priv, u32 channel_no){
	u32 reg_value = 0;
	
	reg_value = HW_READ_REG_BIT(0x16800020, io2pin[channel_no], io2pin[channel_no]);
	return (reg_value & 0x01);
}

void gpio_output_value(struct atbm_common *hw_priv, u32 channel_no, u32 level){
	
	HW_WRITE_REG_BIT(0x16800024, io2pin[channel_no], io2pin[channel_no], level);
}

void gpio_input_config(struct atbm_common *hw_priv, u32 channel_no){

	HW_WRITE_REG_BIT(0x16800034, io2pin[channel_no], io2pin[channel_no], 1);
	HW_WRITE_REG_BIT(0x16800028, io2pin[channel_no], io2pin[channel_no], 0);
}

void gpio_output_config(struct atbm_common *hw_priv, u32 channel_no){
	
	HW_WRITE_REG_BIT(0x16800034, io2pin[channel_no], io2pin[channel_no], 0);
	HW_WRITE_REG_BIT(0x16800028, io2pin[channel_no], io2pin[channel_no], 1);
}


int gpio_set_pin_func(struct atbm_common *hw_priv, int channel_no,int func)
{
	u32 value = 0;
	u32 regaddr = 0;
	
	if(channel_no > 11){
		return 0;
	}
	/*
	if(channel_no == 22 || channel_no == 23)
	{
		atbm_usb_write_bit(hw_priv, 0x17400000,8,8,1);
	}	
	*/
	func = func&0xf;
	regaddr = 0x17400000 + muxOffset[channel_no/2];
	atbm_printk_always("regaddr:0x%x\n", regaddr);
	if((channel_no == 0) || (channel_no == 2) || (channel_no == 4) || (channel_no == 6) || (channel_no == 9) || (channel_no == 11)){
		HW_WRITE_REG_BIT(regaddr,19,16,func);
	}
	else {
		HW_WRITE_REG_BIT(regaddr,3,0,func); 	
	}
	//gpio_db_en
	HW_WRITE_REG_BIT(0x16800070, io2pin[channel_no], io2pin[channel_no], 0);

	return 0;
}

void gpio_pull_value(struct atbm_common *hw_priv, u32 channel_no,bool value)
{
	u32 regaddr;
	/*
	if(channel_no == 22 || channel_no == 23)
	{
		atbm_usb_write_bit(hw_priv, 0x17400000,8,8,1);
	}
	*/
	regaddr = 0x17400000 + muxOffset[channel_no/2];
	atbm_printk_always("1 regaddr:0x%x\n", regaddr);
	if(value&0x01)
	{//pull up
		if((channel_no == 0) || (channel_no == 2) || (channel_no == 4) || (channel_no == 6) || (channel_no == 9) || (channel_no == 11))
		{
			HW_WRITE_REG_BIT(regaddr,27,27,1);      
			HW_WRITE_REG_BIT(regaddr,28,28,0);      
		}
		else
		{
			HW_WRITE_REG_BIT(regaddr,11,11,1);      
			HW_WRITE_REG_BIT(regaddr,12,12,0);      
		}
	}
	else
	{//pull down
		if((channel_no == 0) || (channel_no == 2) || (channel_no == 4) || (channel_no == 6) || (channel_no == 9) || (channel_no == 11))
		{
			HW_WRITE_REG_BIT(regaddr,27,27,0);      
			HW_WRITE_REG_BIT(regaddr,28,28,1);      
		}
		else
		{
			HW_WRITE_REG_BIT(regaddr,11,11,0);      
			HW_WRITE_REG_BIT(regaddr,12,12,1);      
		}
	}

}

void gpio_drive_capability(struct atbm_common *hw_priv, u32 channel_no)
{
	u32 regaddr;
	/*
	if(channel_no == 22 || channel_no == 23)
	{
		atbm_usb_write_bit(hw_priv, 0x17400000,8,8,1);
	}
	*/
	regaddr = 0x17400000 + muxOffset[channel_no/2];
	atbm_printk_always("2 regaddr:0x%x\n", regaddr);
	if((channel_no == 0) || (channel_no == 2) || (channel_no == 4) || (channel_no == 6) || (channel_no == 9) || (channel_no == 11))
	{
		HW_WRITE_REG_BIT(regaddr, 26, 25, 3);
	}
	else
	{
		HW_WRITE_REG_BIT(regaddr, 10, 9, 3);
	}
}


int Atbm_Input_Value_Gpio(struct atbm_common *hw_priv, int channel_no)
{
	u32 inputVal = 0;

	//9:GPIO_FUNC_T
	gpio_set_pin_func(hw_priv, channel_no, 9);

	gpio_input_config(hw_priv, channel_no);

	inputVal = gpio_input_value(hw_priv, channel_no);

	return inputVal;
}

void Atbm_Output_Value_Gpio(struct atbm_common *hw_priv, int channel_no, int level)
{
	//9:GPIO_FUNC_T
	gpio_set_pin_func(hw_priv, channel_no, 9);

	gpio_output_config(hw_priv, channel_no);
	
	gpio_output_value(hw_priv, channel_no, level);
}


int atbm_ioctl_get_gpio4(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra){
	int gpio_data = 0;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct ieee80211_hw *hw = &local->hw;
	struct atbm_common *hw_priv = (struct atbm_common *) hw->priv;
	gpio_data = Atbm_Input_Value_Gpio(hw_priv, 4);
	if(extra){
		sprintf(extra,"GPIO_value:%d\n",gpio_data);
		//memcpy((char *)extra, (char *)&gpio_data,sizeof(int));
		wrqu->data.length = strlen(extra);
	}
	return  gpio_data;
}

#endif
int atbm_ioctl_get_cfo_cali_data(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct ieee80211_hw *hw = &local->hw;
	struct atbm_common *hw_priv = (struct atbm_common *) hw->priv;
	u8 dcxo = 0;
	u8 crystal_type = 0;
	struct cfo_ppm_t cfo_val = {0};

	if(atbm_get_cfo_allow_flag() == 0){
		atbm_printk_err("atbm_ioctl_get_cfo_cali_data:not support \n");
		return 0;
	}
	
	crystal_type = atbm_get_chip_crystal_type(hw_priv);
	
	//dcxo = atbm_DCXOCodeRead(hw_priv);

	wsm_get_cfo_ppm_correction_value(hw_priv,&cfo_val,sizeof(struct cfo_ppm_t));
	dcxo = cfo_val.dcxo;
	
	if(crystal_type==ATBM_SHARE_CRYSTAL)
		atbm_printk_err("crystal_type=2,%s ,rx_cfo:%d ppm,tx_cfo:%d ppm,cfo_ppm_has_init_flag:%d\n", "share crystal",cfo_val.rx_cfo,
		cfo_val.tx_cfo,cfo_val.cfo_ppm_has_init_flag);
	else
		atbm_printk_err("crystal_type=%d,%s,dcxo:%d ,rx_cfo:%d ppm,cfo_ppm_has_init_flag:%d\n", crystal_type,"independent crystal", dcxo,cfo_val.rx_cfo,cfo_val.cfo_ppm_has_init_flag);
		

	if(extra){	
		if(crystal_type==ATBM_SHARE_CRYSTAL)
			sprintf(extra,"crystal_type=2,%s ,rx_cfo:%d ppm,tx_cfo:%d ppm,cfo_ppm_has_init_flag:%d\n", "share crystal",cfo_val.rx_cfo,
			cfo_val.tx_cfo,cfo_val.cfo_ppm_has_init_flag);
		else
			sprintf(extra,"crystal_type=%d,%s,dcxo:%d ,mrxcfo:%d ppm,cfo_ppm_has_init_flag:%d\n", crystal_type,"independent crystal", dcxo,cfo_val.rx_cfo,cfo_val.cfo_ppm_has_init_flag);

		//sprintf(extra,"\n%s,dcxo:%d,cfo:%dppm\n",crystal_type==2?"share crystal":"independent crystal",dcxo,cfo_val);
    	wrqu->data.length = strlen(extra);
	}
	
	return  0;
}

int atbm_ioctl_send_deauth(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
//	struct ieee80211_local *local = sdata->local;
//	struct ieee80211_hw *hw = &local->hw;
//	struct atbm_common *hw_priv = (struct atbm_common *) hw->priv;
	u8 da[6]={0xff,0xff,0xff,0xff,0xff,0xff};
	//struct atbm_vif *priv;
//	struct atbm_vif *priv = (struct atbm_vif *)sdata->vif.drv_priv;

//	priv->hw_priv;
	
	if (!ieee80211_sdata_running(sdata)){
		return  -ENETDOWN;
		
	}
//	sdata->send_deauth = 1;
	atbm_send_deauth_disossicate(sdata,da);
	return 0;
}

int atbm_ioctl_send_vendor_ie(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
//	struct ieee80211_local *local = sdata->local;
	//struct ieee80211_hw *hw = &local->hw;
//	struct atbm_common *hw_priv = (struct atbm_common *) hw->priv;
	struct atbm_vendor_cfg_ie private_ie;
	char *data = NULL,*ptr,*str;
	int i,cmd_len,ssid_len,pw_len,ret=0,num=0;
	u8 channels[14] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14};
	int chan = 0;
	int j = 0;
	
	if (!ieee80211_sdata_running(sdata)){
		atbm_printk_err("net interface not runing! \n");
		ret =  -EINVAL;
		goto exit;

	}

	data = (char *)atbm_kzalloc(wrqu->data.length+1, GFP_KERNEL);
	if(data == NULL){
		ret =  -ENOMEM;
		goto exit;
	}
	if(copy_from_user((void *)data, wrqu->data.pointer, wrqu->data.length)){
		ret =  -EINVAL;
		goto exit;
	}
	
//	str = strchr(data,',');
//	ptr = strchr(str+1,',');

	str = NULL;
	ptr = NULL;
	for(i=0;i<wrqu->data.length;i++){
		if((data[i] == ',') && (num == 0)){
				num++;
				str = data + i;//ssid
		}
		else if((data[i] == ',') && (num == 1)){
			ptr = data + i ;//psk
			break;
		}
	}
	if(!str || !ptr){
		atbm_printk_err("atbm_ioctl_send_probe_resp , Data parsing error! ,data = %s\n",data);
		goto exit;
	}
	cmd_len = str - data;
	ssid_len = ptr - str -1;
	pw_len = wrqu->data.length - cmd_len - ssid_len - 2 -1;
	
	
	memset(&private_ie,0,sizeof(struct atbm_vendor_cfg_ie));
	private_ie.ie_id = 221;
	private_ie.OUI[0] = (ATBM_6441_PRIVATE_OUI >> 24) & 0xFF;
	private_ie.OUI[1] = (ATBM_6441_PRIVATE_OUI >> 16) & 0xFF;
	private_ie.OUI[2] = (ATBM_6441_PRIVATE_OUI >> 8) & 0xFF;
	private_ie.OUI[3] = ATBM_6441_PRIVATE_OUI & 0xFF;
	memcpy(private_ie.ssid,str+1,ssid_len);
	private_ie.ssid_len = ssid_len;
	memcpy(private_ie.password,ptr+1,pw_len);
	private_ie.password_len = pw_len;
	private_ie.ie_len = sizeof(struct atbm_vendor_cfg_ie) - 2;

	
	
	chan = atbm_get_work_channel(sdata,0);

	atbm_printk_err("ssid=%s , ssid_len = %d,psk=%s,psk_len=%d ,channel = %d\n",
			private_ie.ssid,private_ie.ssid_len,private_ie.password,private_ie.password_len,chan);
	
	
	if(chan > 14 || chan < 1){
		for(j=0;j<14;j++)
			channels[j] = j+1;
	}else{
		for(j=0;j<14;j++)
			channels[j] = chan;
	}
	ieee80211_sta_triger_positive_scan(sdata,channels,14,NULL,0,
				(u8 *)&private_ie,sizeof(struct atbm_vendor_cfg_ie),NULL);
	ret = 0;
	
exit:
	if(data)
		atbm_kfree(data);
	return ret;
}

#ifdef CONFIG_IEEE80211_SEND_SPECIAL_MGMT

extern int ieee80211_send_action_mgmt_queue(struct atbm_common *hw_priv,char *buff,bool start);
extern int ieee80211_send_probe_resp_mgmt_queue(struct atbm_common *hw_priv,char *buff,bool start);
static int atbm_ioctl_send_action(struct net_device *dev, struct iw_request_info *info,  union iwreq_data  *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
//	struct atbm_vif *priv = (struct atbm_vif *)sdata->vif.drv_priv;
	int ret = 0;
	int work_chan = 0;
	int i,strheadLen = 0;
	char *buff_data =NULL;
	int action_code = 0;
	struct atbm_customer_action customer_action_ie;
	
	if (!ieee80211_sdata_running(sdata)){
		atbm_printk_err("atbm_ioctl_send_action:net interface not runing! \n");

		return  -EINVAL;

	}


	if(sdata->vif.type != NL80211_IFTYPE_AP){
		work_chan = atbm_get_work_channel(sdata,1);
		if(!work_chan){
			atbm_printk_err("atbm_ioctl_send_action: work chan = 0! \n");
			return -1;
		}
		atbm_printk_err("atbm_ioctl_send_action:sdata->vif.type = %d %s\n",sdata->vif.type,sdata->vif.type == NL80211_IFTYPE_STATION?"STATION":"OTHER");
	}
	
/************************** get action code********************************************/
	
	buff_data = atbm_kzalloc(wrqu->data.length+1, GFP_KERNEL);

	if(buff_data == NULL){
		atbm_printk_err("%s atbm_kzalloc error!\n",__func__);
		ret =  -EINVAL;
		goto err1;
	}

	if(copy_from_user(buff_data, wrqu->data.pointer, wrqu->data.length)){
		atbm_printk_err("%s copy_from_user error!\n",__func__);
	//	atbm_kfree(buff_data);
		ret =  -EINVAL;
		goto err;
	}
	buff_data[wrqu->data.length] = 0;
	for(i=0;i<wrqu->data.length;i++)
	{
		if(buff_data[i] == ',')
		{
			strheadLen=i+1;
			break;
		}
	}
	
	sscanf(buff_data,"send_action,%d",&action_code);

	atbm_printk_always("atbm_ioctl_send_action :buff_data=%s, action code = %d \n",buff_data,action_code);


/**********************************************************************/
	
	memset(&customer_action_ie,0,sizeof(struct atbm_customer_action));

	customer_action_ie.sdata = sdata;
	customer_action_ie.action = action_code;
	ieee80211_send_action_mgmt_queue(hw_priv,(char *)&customer_action_ie,1);
	//hw_priv->start_send_action = true;
	//wake_up(&hw_priv->send_prvmgmt_wq);


err:
	if(buff_data)
		atbm_kfree(buff_data);
err1:
	return 0;
}


static int atbm_ioctl_stop_send_action(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct ieee80211_hw *hw = &local->hw;
	struct atbm_common *hw_priv = (struct atbm_common *) hw->priv;

	if (!ieee80211_sdata_running(sdata)){
		atbm_printk_err("atbm_ioctl_send_action:net interface not runing! \n");

		return  -EINVAL;

	}


	//hw_priv->start_send_action = false;
	
	ieee80211_send_action_mgmt_queue(hw_priv,NULL,0);
	atbm_printk_err("atbm_ioctl_stop_send_action! \n");
	return 0;
}

static int atbm_ioctl_send_probe_resp(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct ieee80211_hw *hw = &local->hw;
	struct atbm_common *hw_priv = (struct atbm_common *) hw->priv;
//	struct sk_buff *skb = NULL;
	
	struct atbm_vendor_cfg_ie private_ie;
	struct atbm_ap_vendor_cfg_ie ap_vendor_cfg_ie;

	char *data = NULL,*ptr,*str;
	int i,cmd_len,ssid_len,pw_len,ret = 0,num = 0;

	if (!ieee80211_sdata_running(sdata)){
		atbm_printk_err("net interface not runing! \n");
		ret =  -EINVAL;
		goto exit;

	}
	if(sdata->vif.type != NL80211_IFTYPE_AP){
		atbm_printk_err("atbm_ioctl_send_probe_resp , not ap mode! \n");
		return false;
	}

	
	data = (char *)atbm_kzalloc(wrqu->data.length+1, GFP_KERNEL);
	if(data == NULL){
		ret =  -ENOMEM;
		goto exit;
	}
	if(copy_from_user((void *)data, wrqu->data.pointer, wrqu->data.length)){
		ret =  -EINVAL;
		goto exit;
	}
	
//	str = strchr(data,',');
//	ptr = strchr(str+1,',');
	str = NULL;
	ptr = NULL;
	for(i=0;i<wrqu->data.length;i++){
		if((data[i] == ',') && (num == 0)){
				num++;
				str = data + i;//ssid
		}
		else if((data[i] == ',') && (num == 1)){
			ptr = data + i ;//psk
			break;
		}
	}
	if(!str || !ptr){
		atbm_printk_err("atbm_ioctl_send_probe_resp , Data parsing error! ,data = %s\n",data);
		goto exit;
	}
	
	cmd_len = str - data;
	ssid_len = ptr - str -1;
	pw_len = wrqu->data.length - cmd_len - ssid_len - 2 -1;
	
	
	memset(&private_ie,0,sizeof(struct atbm_vendor_cfg_ie));
	private_ie.ie_id = 221;
	private_ie.OUI[0] = (ATBM_6441_PRIVATE_OUI >> 24) & 0xFF;
	private_ie.OUI[1] = (ATBM_6441_PRIVATE_OUI >> 16) & 0xFF;
	private_ie.OUI[2] = (ATBM_6441_PRIVATE_OUI >> 8) & 0xFF;
	private_ie.OUI[3] = ATBM_6441_PRIVATE_OUI & 0xFF;
	memcpy(private_ie.ssid,str+1,ssid_len);
	private_ie.ssid_len = ssid_len;
	memcpy(private_ie.password,ptr+1,pw_len);
	private_ie.password_len = pw_len;
	private_ie.ie_len = sizeof(struct atbm_vendor_cfg_ie) - 2;

	atbm_printk_err("ssid=%s , ssid_len = %d,psk=%s,psk_len=%d \n",
			private_ie.ssid,private_ie.ssid_len,private_ie.password,private_ie.password_len);
	
	
	memset(&ap_vendor_cfg_ie,0,sizeof(struct atbm_ap_vendor_cfg_ie));

	memcpy(&ap_vendor_cfg_ie.private_ie ,&private_ie,sizeof(struct atbm_vendor_cfg_ie)); 
	
	ap_vendor_cfg_ie.ap_sdata = sdata;

	//hw_priv->start_send_prbresp = true;
	//wake_up(&hw_priv->send_prvmgmt_wq);
	ieee80211_send_probe_resp_mgmt_queue(hw_priv,(char *)&ap_vendor_cfg_ie,1);
	//ieee80211_queue_work(&sdata->local->hw,&hw_priv->send_prbresp_work);

	ret = 0;
	
exit:
	if(data)
		atbm_kfree(data);
	return ret;
}
static int atbm_ioctl_stop_send_probe_resp(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct ieee80211_hw *hw = &local->hw;
	struct atbm_common *hw_priv = (struct atbm_common *) hw->priv;

	if (!ieee80211_sdata_running(sdata)){
		atbm_printk_err("net interface not runing! \n");

		return  -EINVAL;

	}
	if(sdata->vif.type != NL80211_IFTYPE_AP){
		atbm_printk_err("atbm_ioctl_stop_send_probe_resp , not ap mode! \n");
		return false;
	}

	

//	hw_priv->start_send_prbresp = false;
	ieee80211_send_probe_resp_mgmt_queue(hw_priv,NULL,0);

	atbm_printk_err("atbm_ioctl_stop_send_probe_resp! \n");
	return 0;
}
#endif

int atbm_ioctl_get_vendor_ie(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct atbm_vendor_cfg_ie *private_ie;
	private_ie = atbm_internal_get_6441_vendor_ie();
	if(!private_ie){
		atbm_printk_err("not recive 6441 vendor ie!");
		return -1;
	}
	atbm_printk_err("atbm_ioctl_get_vendor_ie : ssid[%s],password[%s] \n",private_ie->ssid,private_ie->password);

	if(extra){
		sprintf(extra,"\nie_id:%d\nie_len:%d\nOUI:%02x %02x %02x %02x\nssid:%s\nssid_len:%d\npassword:%s\npassword_len:%d\n",
			private_ie->ie_id,private_ie->ie_len,private_ie->OUI[0],private_ie->OUI[1],private_ie->OUI[2],private_ie->OUI[3],
			private_ie->ssid,private_ie->ssid_len,private_ie->password,private_ie->password_len);
		//memcpy(extra, private_ie,sizeof(struct atbm_vendor_cfg_ie));
	//	wrqu->data.length = sizeof(struct atbm_vendor_cfg_ie) + 1;
		wrqu->data.length = strlen(extra);
	}	

	return 0;
}
#ifdef CONFIG_ATBM_AP_CHANNEL_CHANGE_EVENT
extern void ieee80211_start_ap_changechannel_work(struct ieee80211_sub_if_data *ap_sdata,int channel,u8 chann_type);
extern void ieee80211_bss_info_change_notify(struct ieee80211_sub_if_data *sdata,u32 changed);
extern int ieee80211_set_channel(struct wiphy *wiphy,struct net_device *netdev,
										struct ieee80211_channel *chan,enum nl80211_channel_type channel_type);
int atbm_ioctl_change_ap_channel(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct ieee80211_hw *hw = &local->hw;
	struct atbm_common *hw_priv = (struct atbm_common *) hw->priv;
	char * data = NULL;
	int channel = 0,chan_type = 0,freq = 0,ret = 0;
	struct ieee80211_channel * chan ;//=req->bss->channel;
 //	enum nl80211_channel_type channel_type ;//= NL80211_CHAN_HT20;	

	if(sdata->vif.type != NL80211_IFTYPE_AP){
		ret = -EOPNOTSUPP;
		goto exit;
	}
	
	

	data = (char *)atbm_kzalloc(wrqu->data.length+1, GFP_KERNEL);
	if(data == NULL){
		atbm_printk_err("data malloc  err!\n");
		ret =  -ENOMEM;
		goto exit;
	}
	if(copy_from_user((void *)data, wrqu->data.pointer, wrqu->data.length)){
		atbm_printk_err("copy_from_user  err!\n");
		ret =  -EINVAL;
		goto exit;
	}
	atbm_printk_err("atbm_ioctl_change_ap_channel:%s \n",data);
	sscanf(data,"change_chan,%d,%d",&channel,&chan_type);
	
	
	
	if(channel < 0 || channel > 14){
		atbm_printk_err("channel = %d  err!\n",channel);
		ret = -EINVAL;
		goto exit;
	}
	if((hw_priv->chip_version == CRONUS_NO_HT40) || (hw_priv->chip_version == CRONUS_NO_HT40_LDPC)){
		if(chan_type)
			atbm_printk_err("WARNING!!!!chip_version is only support HT20 \n");
		chan_type = NL80211_CHAN_HT20;
	}else{
#ifndef ATBM_NOT_SUPPORT_40M_CHW
#ifdef ATBM_SUPPORT_WIDTH_40M	
		if(chan_type){
			if(channel < 6){
				chan_type = (NL80211_CHAN_HT40PLUS);//2
			}else{
				chan_type = (NL80211_CHAN_HT40MINUS);//3
			}
		}else
#endif
#endif
		{
			chan_type = NL80211_CHAN_HT20;//1
		}
	}
	if(channel == 14)
		freq = 2484;
	else{
		freq = (channel-1)*5 + 2412;
	}
	chan = ieee80211_get_channel(local->hw.wiphy, freq);
	if(chan == NULL){
		atbm_printk_err("ieee80211_get_channel  err!\n");
		ret = -EINVAL;
		goto exit;
	}
	atbm_printk_err("channel:%d , HT40:%d \n",channel,chan_type);	

	atbm_printk_err("ieee80211_start_ap_changechannel_tmp\n");
	//set channel change to hal
	ieee80211_set_channel(local->hw.wiphy,sdata->dev,chan,chan_type);
	//change AP beacon.prbrsp DSparam
	ieee80211_bss_info_change_notify(sdata,	BSS_CHANGED_BEACON|BSS_CHANGED_HT_CHANNEL_TYPE);
	/*set channel change to hostapd*/
	ieee80211_start_ap_changechannel_work(sdata,channel,chan_type);

exit:
	return ret;
}

#endif
int atbm_ioctl_set_listen_probe_req(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
//	struct ieee80211_local *local = sdata->local;
//	struct ieee80211_hw *hw = &local->hw;
//	struct atbm_common *hw_priv = (struct atbm_common *) hw->priv;
	int ret,wifi_status = 0,channel = 0;
	char * data = NULL;
	if (!ieee80211_sdata_running(sdata)){
		atbm_printk_err("net interface not runing! \n");
		ret =  -EINVAL;
		goto exit;

	}
	
	if(sdata->vif.type != NL80211_IFTYPE_STATION){
		ret = -EOPNOTSUPP;
		goto exit;
	}
	
	
	
	data = (char *)atbm_kzalloc(wrqu->data.length+1, GFP_KERNEL);
	if(data == NULL){
		ret =  -ENOMEM;
		goto exit;
	}
	if(copy_from_user((void *)data, wrqu->data.pointer, wrqu->data.length)){
		ret =  -EINVAL;
		goto exit;
	}

	sscanf(data,"listen_probe_req,%d",&channel);
	//str = strchr(data,',');
	//channel = simple_strtol(str+1);
	if(channel < 0 || channel > 14){
		atbm_printk_err("channel = %d  err!\n",channel);
		ret = -EINVAL;
		goto exit;
	}

	wifi_status = atbm_get_sta_wifi_connect_status(sdata);
	
	atbm_printk_err("channel = %d wifi_status = %s\n",channel,wifi_status == 1?"CONNECTED":"DISCONNECT");
	if(wifi_status == 0){	
		ret = ieee80211_set_sta_channel(sdata,channel);
	}else{
		struct ieee80211_special_filter filter;
		memset(&filter,0,sizeof(struct ieee80211_special_filter));
		if(channel){
			filter.filter_action = 0x40;
			filter.flags = SPECIAL_F_FLAGS_FRAME_TYPE;
			atbm_printk_err("%s:action(%x)\n",__func__,filter.filter_action);
			ret = ieee80211_special_filter_register(sdata,&filter);	
		}else{
			ret = ieee80211_special_filter_clear(sdata);
		}
	}

exit:
	if(data)
		atbm_kfree(data);
	return ret;

}
#ifdef SDIO_BUS

//extern int atbm_device_hibernate(struct atbm_common *hw_priv);
extern int atbm_device_wakeup(struct atbm_common *hw_priv);
/*
static int atbm_ioctl_device_hibernate(struct net_device *dev, struct iw_request_info *ifno, union iwreq_data *wrqu, char *ext)
{
	int i = 0;	
	int ret = 0;
	u8 hibernate = 0;
	char *extra = NULL;
	char *ptr = NULL;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
//	struct atbm_common *hw_priv=local->hw.priv;

	if(!(extra = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		return -EINVAL;
	}
	if((ret = copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(extra);
		ret = -EINVAL;
		goto exit;
	}
	extra[wrqu->data.length] = 0;

	atbm_printk_always("extra:%s\n",extra);

	if(wrqu->data.length < strlen("hibernate_en,")){
		atbm_printk_err("e.g: ./iwpriv wlan0 common sleep_en,<sleep_enable>\n");
		atbm_printk_err("e.g: ./iwpriv wlan0 common sleep_en,0\n");
		atbm_kfree(extra);
		ret = -EINVAL;
		goto exit;
	}

	for(i = 0;i<wrqu->data.length;i++){
		if(extra[i] == ',')
		{
			ptr = &extra[i+1];
			break;
		}
	}
	atbm_printk_always(" ptr:%s\n",ptr);
	hibernate = *ptr - 0x30;
	atbm_printk_always("%s set sdiodevice hibernate:%d\n", __func__, hibernate);
	//if (hibernate){
	//	atbm_device_hibernate(hw_priv);
	//}
	//else {
	//	atbm_device_wakeup(hw_priv);
	//}

exit:
	if(extra)
		atbm_kfree(extra);
	return ret;
}
*/
#endif

static int atbm_ioctl_get_cca_threshold(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	int ret = 0;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	int cca_threshold;
	ret = wsm_get_cca_threshold(hw_priv,&cca_threshold,sizeof(int));

	if(ret == 0){
		atbm_printk_err("\ncca_threshold:%d\n",cca_threshold);
		sprintf(extra,"\ncca_threshold:%d\n",cca_threshold);
	}else{
		atbm_printk_err("\ncca_threshold get fail\n");
		sprintf(extra,"\ncca_threshold get fail\n");
	}
	wrqu->data.length = strlen(extra);
	return 0;
}

static int atbm_ioctl_get_tx_parameter(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	int ret = 0;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	int tx_lifetime;
	ret = wsm_get_tx_lifetime(hw_priv,&tx_lifetime,sizeof(int));

	if(ret == 0){
		sprintf(extra,"\ntx_lifetime:%d\n",tx_lifetime);
	}else{
		sprintf(extra,"\ntx_lifetime get fail\n");
	}
	wrqu->data.length = strlen(extra);
	return 0;
}


static int atbm_ioctl_get_rts_threshold(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	int ret = 0;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	int rts_threshold, packet_time;
	int value_array[2];
	ret = wsm_get_rts_threshold(hw_priv,(void *)value_array,sizeof(int)*2);
	if(ret == 0)
	{
		rts_threshold = value_array[0];
		packet_time = *((u16 *)(&value_array[1])) * 32;
	}
	if(ret == 0){
		sprintf(extra,"\nthreshold:%d, packet_time:%d\n",rts_threshold, packet_time);
	}else{
		sprintf(extra,"\nthreshold get fail\n");
	}
	wrqu->data.length = strlen(extra);
	return 0;
}

static int atbm_ioctl_get_rts_duration(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	int ret = 0;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	int rts_duration;
	ret = wsm_get_rts_duration(hw_priv,&rts_duration,sizeof(int));
	if(ret == 0){
		if(rts_duration == 0)
		{
			sprintf(extra,"\nuse defaule rts_duration");
		}
		else
		{
			sprintf(extra,"\nrts_duration:%d\n",rts_duration);
		}
	}else{
		sprintf(extra,"\nrts_duration get fail\n");
	}
	wrqu->data.length = strlen(extra);
	return 0;
	
}

/*

struct RX_QA_STATUS_S{
	s8 initial_gain;
	s8 noise_level_dBm;
	u8 cca_indication;
	s8 update_gain;
	uint32 reserved;
};

*/

static int atbm_ioctl_get_noise_level(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	int ret = 0;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	//struct RX_QA_STATUS_S noise_level;
	char initial_gain = 0;
	int noise_level_dBm = 0;
	//iwpriv wlan0 fwcmd agc,1  ==>AgcStop
	//memset(&noise_level,0,sizeof(noise_level));
	char *AgcStop = "agc,2";
	char *AgcStart = "agc,1";
	ret = wsm_write_mib(hw_priv, WSM_MIB_ID_FW_CMD,AgcStop,5, 0);
	if(ret < 0){
		atbm_printk_err("atbm_ioctl_get_noise_level:AgcStop err! \n");
		return -1;
	}
	msleep(100);
	//ret = wsm_get_noise_level(hw_priv,&noise_level,sizeof(struct RX_QA_STATUS_S));
	
	//if(noise_level.noise_level_dBm > 127)
		//noise_level.noise_level_dBm -= 256;
	//initial_gain = HW_READ_REG_BIT(0xAC804A0, 6, 0);
	initial_gain = HW_READ_REG_BIT(0xAC80428, 31, 25);//Oceanus
	initial_gain = N_BIT_TO_SIGNED_32BIT(initial_gain, 7);

	//noise_level_dBm = HW_READ_REG_BIT(0xAC80CD8, 29, 20);
	//noise_level_dBm = HW_READ_REG_BIT(0xAC80C30, 10, 2);//th provided originally for ocea
	noise_level_dBm = HW_READ_REG_BIT(0xAC80C30, 8, 1);//Corrected one for ocea, S(9,1),[8:0]
	//noise_level_dBm = N_BIT_TO_SIGNED_32BIT(noise_level_dBm, 10);
	//noise_level_dBm = N_BIT_TO_SIGNED_32BIT(noise_level_dBm, 9);//th provided originally for ocea
	noise_level_dBm = N_BIT_TO_SIGNED_32BIT(noise_level_dBm, 8);//Corrected one for ocea

	atbm_printk_err("OrigIniG = %d, OrigNLdBm = %d\n",initial_gain, noise_level_dBm);
	
	if(initial_gain == 60)
	{
		atbm_printk_err("\nexact noise_level_dBm = [%d/4] = [%d] \n",noise_level_dBm,noise_level_dBm>>2);
		sprintf(extra,"\nexact noise_level_dBm = [%d/4] = [%d] \n",noise_level_dBm,noise_level_dBm>>2);
	}
	else
	{
		atbm_printk_err("\ninitg = [%d/4],reckon noise_level_dBm = [%d/4]=[%d]\n",initial_gain,noise_level_dBm,noise_level_dBm>>2);
		sprintf(extra,"\ninitg = [%d/4],reckon noise_level_dBm = [%d/4]=[%d]\n",initial_gain,noise_level_dBm,noise_level_dBm>>2);
	}

	wrqu->data.length = strlen(extra);
	ret = wsm_write_mib(hw_priv, WSM_MIB_ID_FW_CMD,AgcStart,5, 0);
	if(ret < 0){
		atbm_printk_err("atbm_ioctl_get_noise_level:AgcStart err! \n");
		return -1;
	}
	return 0;
}


const char* FwRateIndexStr[] =
{
	/*11b rate index*/
	"B_1M", 
	"B_2M", 
	"B55M", 
	"B11M", 

	/*unused rate index*/
	"B11M", 
	"B11M", 

	/*11 g rate index*/
	"G_6M", 
	"G_9M", 
	"G12M", 
	"G18M", 
	"G24M", 
	"G36M", 
	"G48M", 
	"G54M", 

	/*11n rate index*/
	"N_S0", 
	"N_S1", 
	"N_S2", 
	"N_S3", 
	"N_S4", 
	"N_S5", 
	"N_S6", 
	"N_S7", 

	/*msc32*/
	"NMS32", 

	/*HE Rate Index*/
	"HER0", 
	"HER1", 
	"HER2", 
	"HES0", 
	"HES1", 
	"HES2", 
	"HES3", 
	"HES4", 
	"HES5", 
	"HES6", 
	"HES7", 
	"HES8", 
	"HES9", 
	"HE10", 
	"HE11", 

    "VNS0", 
	"VNS1", 
	"VNS2", 
	"VNS3", 
	"VNS4", 
	"VNS5", 
	"VNS6", 
	"VNS7", 
	"VNS8", 
	"VNS9", 
};


char * val_to_mode(u8 mode)
{
	char* mode_str;

	if(2 == mode)
		mode_str = "OLEGACY";
	else if(4 == mode)
		mode_str = "OGREEN";
	else if(5 == mode)
		mode_str = "OMIXED";
	else if(6 == mode)
		mode_str = "OHE_SU";
	else if(7 == mode)
		mode_str = "OHE_ER";
	else if(8 == mode)
		mode_str = "OHE_TB";
	else if(9 == mode)
		mode_str = "OHE_MU";
	else
		mode_str = "NULL";

	atbm_printk_always("%s\n",mode_str);

	return mode_str;
}
	

const char * val_to_rate(u8 rateIndex)
{
	const char* rate_str;

	rate_str = FwRateIndexStr[rateIndex];

	return rate_str;	
}


static int atbm_ioctl_get_snr(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	int ret = 0;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	short levm = 0;
	short devm = 0;
	short mode = 0;
	short rix = 0;
	int rval = 0;
	//unsigned short snr = 0;
	
	ret = wsm_get_snr(hw_priv,&rval,sizeof(int));
	levm = rval&0x7F;//[6:0]
	devm = rval&0x7F80>>7;//[14:7]
	mode = rval&0xFF0000>>16;//[23:16]
	rix = rval&0xFF000000>>24;//[31:24]

	atbm_printk_always("rVal:%x,LE:%d,DE:%d,Mode:%s,RRI:%s\n", rval, levm, devm, val_to_mode(mode),val_to_rate(rix));

	if(ret == 0){
		sprintf(extra,"\nlmac get snr_val = %d\n",devm);
		atbm_printk_always("\nlmac get snr_val = %d\n",devm);
	}else{
		sprintf(extra,"\nsnr get fail\n");
		atbm_printk_always("\nsnr get fail\n");
	}
	wrqu->data.length = strlen(extra);
	return 0;
	
}


static int atbm_ioctl_get_tx_packet_status(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	int ret = 0;
	char mac_addr[6] = {0};
	unsigned char ptr[128]={0};
	unsigned long tx_total_pkt = 0;
	unsigned long tx_fail_pkt = 0;
	int ptr_len = 0;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);



	

	ptr_len = wdata->data.length - 1;
	atbm_printk_wext("atbm_ioctl_get_rate()  %d\n",ptr_len);

	if (sdata->vif.type == NL80211_IFTYPE_AP) {
		
		
		if(copy_from_user(ptr, wdata->data.pointer, ptr_len)){
			atbm_printk_wext("%s() copy userspace data err!!\n",__func__);
			return -EINVAL;
		}
		atbm_printk_err("ptr %s\n",ptr);
		if(ptr_len != 31){
			atbm_printk_err("ptr_len=%d != 31 is error\n",ptr_len);
			return -1;
		}
		sscanf(ptr, "get_tx_status,%02x:%02x:%02x:%02x:%02x:%02x", (int*)&mac_addr[0], (int*)&mac_addr[1], (int*)&mac_addr[2]
			, (int*)&mac_addr[3], (int*)&mac_addr[4], (int*)&mac_addr[5]);

		atbm_printk_err("mac addr: %pM\n",mac_addr);
		
	}
	

	if(atbm_internal_get_tx_pkt_status(sdata,mac_addr,&tx_total_pkt,&tx_fail_pkt) < 0){
		atbm_printk_err("get tx pkt status err! \n");
		return -1;
	}

		
	if(extra){	
		if(sdata->vif.type == NL80211_IFTYPE_AP){
			sprintf(extra,"\n[AP MODE]sta_mac:%pM , tx_pkt=%lu , tx_fail_pkt=%lu\n",mac_addr,tx_total_pkt,tx_fail_pkt);
		}else{
			sprintf(extra,"\n[STA MODE]tx_pkt=%lu , tx_fail_pkt=%lu\n",tx_total_pkt,tx_fail_pkt);
		}
		wrqu->data.length = strlen(extra);
	}

	return ret;
}

static int atbm_ioctl_update_beacon_loss_conter(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	int ptr_len = 0,conter = 0;
	unsigned char ptr[64] = {0};


	ptr_len = wdata->data.length - 1;
	atbm_printk_err("atbm_ioctl_get_rate()  %d\n",ptr_len);
	if(copy_from_user(ptr, wdata->data.pointer, ptr_len)){
		atbm_printk_err("%s() copy userspace data err!!\n",__func__);
		return -EINVAL;
	}
	atbm_printk_err("ptr %s\n",ptr);
	sscanf(ptr,"beacon_loss_cnt,%d",&conter);
	atbm_printk_err("conter:%d \n",conter);
	if(conter < 0 || conter > 255){
		atbm_printk_err("%s() conter(%d) not in range(0,255)\n",__func__,conter);
		return -EINVAL;
	}
	if(atbm_internal_wsm_set_beacon_loss_cnt(sdata,conter) < 0){
		atbm_printk_err("%s() set becon loss data err!!\n",__func__);
		return -EINVAL;
	}

	return 0;
}

extern int atbm_wifi_bt_comb_get(void);
extern void atbm_wifi_bt_comb_set(int status);
extern void atbm_wifi_insmod_stat_set(unsigned int state);

static int atbm_ioctl_fw_reset(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	//union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	int i,strheadLen = 0;
	char *buff_data =NULL;
	int ps_elems = 0;

	buff_data = atbm_kzalloc(wrqu->data.length+1, GFP_KERNEL);

	if(buff_data == NULL){
		atbm_printk_wext("%s atbm_kzalloc error!\n",__func__);
		return  -EINVAL;
	}
	
	if(copy_from_user(buff_data, wrqu->data.pointer, wrqu->data.length)){
		atbm_printk_wext("%s copy_from_user error!\n",__func__);
		atbm_kfree(buff_data);
		return -EINVAL;
	}
	buff_data[wrqu->data.length] = 0;
	for(i=0;i<wrqu->data.length;i++)
	{
		if(buff_data[i] == ',')
		{
			strheadLen=i+1;
			break;
		}
	}
	
	
	ps_elems = buff_data[strheadLen];
#ifdef CONFIG_ATBM_BLE_CODE_SRAM
	if (ps_elems == 0x31){
		atbm_wifi_bt_comb_set(1);

	}
#ifdef ATBM_ADD_CRERT_FIRMWARE

	else if(ps_elems == 0x32){
		atbm_wifi_bt_comb_set(2);
	}
#endif
	else
#endif
	{
#ifdef CONFIG_ATBM_ONLY_COMB_FIRMWARE
		atbm_wifi_bt_comb_set(1);
#else

		atbm_wifi_bt_comb_set(0);
#endif
	}
	
//	atbm_wifi_bt_comb_set(hw_priv->wifi_ble_comb);
	atbm_printk_err("\n======>>> reload %s firmware <<<======\n\n",atbm_wifi_bt_comb_get() !=0 ?(atbm_wifi_bt_comb_get()==2?"WIFI BT COMB CERT":"WIFI BLE COMB"):"only WIFI");
	atbm_kfree(buff_data);
	atbm_wifi_insmod_stat_set(0);
	atbm_bh_halt(hw_priv);

	
	return 0;
}

static int atbm_ioctl_set_packet_interval(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	//union iwreq_data *wdata = (union iwreq_data *)wrqu;
	//struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	//struct ieee80211_local *local = sdata->local;
	//struct atbm_common *hw_priv=local->hw.priv;
	int i,ret = -1,strheadLen = 0;
	char *pbuffer = NULL;
	char *pRxData = NULL;
	int interval = 0;

	return 0;

	if(!(pbuffer = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		return -EINVAL;
	}
	if((ret = copy_from_user(pbuffer, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(pbuffer);
		return -EINVAL;
	}
	pbuffer[wrqu->data.length] = 0;

	for(i=0;i<wrqu->data.length;i++)
	{
		if(pbuffer[i] == ',')
		{
			strheadLen=i;
			break;
		}
	}
	
	atbm_printk_err("length=%d,data=%s,strheadLen=%d\n", wrqu->data.length, pbuffer, strheadLen);
	if (strheadLen >= wrqu->data.length){
		atbm_kfree(pbuffer);
		return -EINVAL;
	}

	pRxData = &pbuffer[strheadLen];
	atbm_printk_err("cmd:%s\n", pRxData);
	atbm_CmdLine_GetSignInteger(&pRxData, &interval);

	if(interval < 0)
		interval = 16;
	g_PacketInterval = interval;
	atbm_printk_always("set pakcet interval:%d us\n", g_PacketInterval);

	if(pbuffer)
		atbm_kfree(pbuffer);

	return ret;
}

static int atbm_ioctl_set_efuse_gain_trim(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;

	struct efuse_headr efuse_data,reg_efuse;
	int low_compensation_val = 0,mid_compensation_val = 0,max_compensation_val = 0;
	int ret,write_rom;
	char writebuf[128] = "";
	int compensation_val[3] = {0};
//	struct cfg_txpower_t configured_txpower,configured_txpower_temp;
	int prv_deltagain;
	int delta_gain[3],deal_gain[3];
	int gain_for = 0;
//	static int read_flag = 0;
	/*
		Get efuse deltAgain of the register
	*/



	if(copy_from_user((void *)writebuf, wrqu->data.pointer, wrqu->data.length)){
		atbm_printk_err("copy_from_user  err!\n");
		ret =  -EINVAL;
		goto exit;
	}
	atbm_printk_err("atbm_ioctl_set_efuse_gain_trim:%s \n",writebuf);
	sscanf(writebuf,"efuse_gain_trim,%d,%d,%d,%d",
			&low_compensation_val,&mid_compensation_val,&max_compensation_val,&write_rom);
	memset(writebuf,0,sizeof(writebuf));
	memset(&efuse_data,0,sizeof(struct efuse_headr));
	memset(&reg_efuse,0,sizeof(struct efuse_headr));
//	memset(&configured_txpower_temp,0,sizeof(configured_txpower_temp));
//	memset(&configured_txpower,0,sizeof(configured_txpower));
	compensation_val[0] = low_compensation_val;
	compensation_val[1] = mid_compensation_val;
	compensation_val[2] = max_compensation_val;
	atbm_printk_err("gain:%d %d %d,write=%d \n",compensation_val[0],compensation_val[1],compensation_val[2],write_rom);
	atbm_get_reg_deltagain(&reg_efuse);
	

	if(memcmp(&efuse_data,&reg_efuse,sizeof(struct efuse_headr)) != 0){
		atbm_printk_err("gain1:%d gain2:%d gain3:%d \n",
					reg_efuse.delta_gain1,
					reg_efuse.delta_gain2,
					reg_efuse.delta_gain3);
		delta_gain[0] = reg_efuse.delta_gain1;//hw_priv->efuse.delta_gain1;
		delta_gain[1] = reg_efuse.delta_gain2;//hw_priv->efuse.delta_gain2;
		delta_gain[2] = reg_efuse.delta_gain3;//hw_priv->efuse.delta_gain3;

		delta_gain[0] > 127?delta_gain[0]-=256:delta_gain[0];
		delta_gain[1] > 127?delta_gain[1]-=256:delta_gain[0];
		delta_gain[2] > 127?delta_gain[2]-=256:delta_gain[0];
		
		delta_gain[0] < 0?delta_gain[0]+=32:delta_gain[0];
		delta_gain[1] < 0?delta_gain[1]+=32:delta_gain[1];
		delta_gain[2] < 0?delta_gain[2]+=32:delta_gain[2];
		
		atbm_printk_err("gain1:%d gain2:%d gain3:%d \n",
					delta_gain[0],
					delta_gain[1],
					delta_gain[2]);
	}
	else{
		
		if ((ret = wsm_get_efuse_data(hw_priv, &efuse_data, sizeof(efuse_data))) == 0){	
			atbm_printk_init("Get efuse data is [%d,%d,%d,%d,%d,%d,%d,%d,%02x:%02x:%02x:%02x:%02x:%02x]\n",
					efuse_data.version,efuse_data.dcxo_trim,efuse_data.delta_gain1,efuse_data.delta_gain2,efuse_data.delta_gain3,
					efuse_data.Tj_room,efuse_data.topref_ctrl_bias_res_trim,efuse_data.PowerSupplySel,efuse_data.mac[0],efuse_data.mac[1],
					efuse_data.mac[2],efuse_data.mac[3],efuse_data.mac[4],efuse_data.mac[5]);

		}else{
			atbm_printk_err("wsm_get_efuse_data failed\n");
			return -1;
		}
		delta_gain[0] = efuse_data.delta_gain1;
		delta_gain[1] = efuse_data.delta_gain2;
		delta_gain[2] = efuse_data.delta_gain3;
		if(compensation_val[0] == 0 
			&& compensation_val[1] == 0 
			&& compensation_val[2] == 0){
			atbm_printk_err("compensation_val == 0, not change! \n");
			return 0;
		}
	}	


	
	
//	if(compensation_val[0] > 128)
//		compensation_val[0] -= 256;
	atbm_printk_err("befor deltagain = %d %d %d\n",delta_gain[0],delta_gain[1],delta_gain[2]);
	atbm_printk_err("change deltagain = %d %d %d\n",compensation_val[0],compensation_val[1],compensation_val[2]);
	for(gain_for = 0;gain_for<3;gain_for++)	{
		
		if(compensation_val[gain_for] == 0){
			deal_gain[gain_for] = delta_gain[gain_for];
			atbm_printk_err("continue deltagain[%d] = %d ,%d\n",gain_for,delta_gain[gain_for],compensation_val[gain_for]);
			continue;
		}
		if(compensation_val[gain_for] > 128)
			compensation_val[gain_for] -= 256;
		atbm_printk_err("compensation_val[%d] = %d\n",gain_for,compensation_val[gain_for]);
	
		prv_deltagain = delta_gain[gain_for];
		if(prv_deltagain < 16){//
			
			if(compensation_val[gain_for] >= 0){// gain add
				prv_deltagain = prv_deltagain + compensation_val[gain_for];
				if(prv_deltagain > 128)
					prv_deltagain -= 256;
			
				if(prv_deltagain > 15){
					prv_deltagain = 15;
				}
				
			}else{//gain sub
				prv_deltagain = prv_deltagain + compensation_val[gain_for];
				
				if(prv_deltagain > 128)
					prv_deltagain -= 256;
				
				if(prv_deltagain < 0){
					prv_deltagain += 32;
					if(prv_deltagain < 17)
						prv_deltagain = 17;
				}
				
			}

		}else if(prv_deltagain >= 16){//
		
			if(compensation_val[gain_for] >= 0){// gain add
				prv_deltagain += compensation_val[gain_for];
				if(prv_deltagain > 128)
					prv_deltagain -= 256;
		
				if(prv_deltagain >= 32){
					prv_deltagain = (prv_deltagain - 32);
					prv_deltagain > 15?prv_deltagain=15:prv_deltagain;
				}
				
			}else{//gain sub
				prv_deltagain += compensation_val[gain_for];
				if(prv_deltagain > 128)
					prv_deltagain -= 256;
				if(prv_deltagain < 17){
					prv_deltagain = 17;
					
				}
				
			}

		}
		if(prv_deltagain > 128)
				prv_deltagain -= 256;
		deal_gain[gain_for] = prv_deltagain;

		atbm_printk_err("[%d] prv_deltagain = %d ,gain[%d] = %d compensation_val = %d\n",gain_for,
								prv_deltagain,gain_for,deal_gain[gain_for],compensation_val[gain_for]);
	}

//	write_rom = msg->value;
		
//	memset(writebuf, 0, sizeof(writebuf));

//	sprintf(writebuf, "set_txpwr_and_dcxo,%d,%d,%d,%d ", 
//		deal_gain[0],deal_gain[1], deal_gain[2],hw_priv->efuse.dcxo_trim
//	);
	atbm_printk_err("end deltagain = %d %d %d\n",deal_gain[0],deal_gain[1],deal_gain[2]);
	
	reg_efuse.delta_gain1 = deal_gain[0];
	reg_efuse.delta_gain2 = deal_gain[1];
	reg_efuse.delta_gain3 = deal_gain[2];
	
	atbm_printk_init("cmd: %d %d %d\n", reg_efuse.delta_gain1,reg_efuse.delta_gain2,reg_efuse.delta_gain3);
	atbm_set_reg_deltagain(&reg_efuse);
	
	//ret = wsm_write_mib(hw_priv, WSM_MIB_ID_FW_CMD, writebuf, strlen(writebuf), 0);
	//if(ret < 0){
	//	atbm_printk_err("%s: write mib failed(%d). \n",__func__, ret);
	//	return -EINVAL;
	//}
	
	if(write_rom == 1){
		delta_gain[0] = hw_priv->efuse.delta_gain1;
		delta_gain[1] = hw_priv->efuse.delta_gain2;
		delta_gain[2] = hw_priv->efuse.delta_gain3;
		hw_priv->efuse.delta_gain1 = deal_gain[0];
		hw_priv->efuse.delta_gain2 = deal_gain[1];
		hw_priv->efuse.delta_gain3 = deal_gain[2];	
		if((ret = atbm_save_efuse(hw_priv,&hw_priv->efuse)) != 0){
			atbm_printk_err("%s: atbm_save_efuse failed(%d). \n",__func__, ret);
			hw_priv->efuse.delta_gain1 = delta_gain[0];
			hw_priv->efuse.delta_gain2 = delta_gain[1];
			hw_priv->efuse.delta_gain3 = delta_gain[2];
			ret = -EINVAL;
		}

		atbm_printk_init("dcxo:%d,gain1:%d,gain2:%d,gain3:%d,[%02x:%02x:%02x:%02x:%02x:%02x]\n",
					hw_priv->efuse.dcxo_trim,hw_priv->efuse.delta_gain1,hw_priv->efuse.delta_gain2,hw_priv->efuse.delta_gain3,
					hw_priv->efuse.mac[0],hw_priv->efuse.mac[1],hw_priv->efuse.mac[2],hw_priv->efuse.mac[3],
					hw_priv->efuse.mac[4],hw_priv->efuse.mac[5]);
	}
exit:	
	return ret;

}

static int atbm_set_bus_awake(struct net_device* dev, struct iw_request_info* info, union iwreq_data* wrqu, char* ext)
{
	struct ieee80211_sub_if_data* sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	union iwreq_data* wdata = (union iwreq_data*)wrqu;
	struct ieee80211_local* local = sdata->local;
	struct atbm_common* hw_priv = local->hw.priv;
	int wake;
	int ret = 0;
	int i = 0;
	int len = 0;
	char* ptr = NULL;
	char const* pos = NULL;
	char const* pos_end = NULL;

	ptr = (char*)atbm_kzalloc(wdata->data.length + 1, GFP_KERNEL);

	if (!ptr) {
		ret = -ENOMEM;
		goto exit;
	}

	if (copy_from_user(ptr, wdata->data.pointer, wdata->data.length)) {
		ret = -EINVAL;
		goto exit;
	}

	ptr[wrqu->data.length] = 0;

	for (i = 0;i < wrqu->data.length;i++) {
		if (ptr[i] == ',')
			ptr[i] = ATBM_SPACE;
	}

	pos = atbm_skip_space(ptr, wrqu->data.length + 1);

	if (pos == NULL) {
		ret = -EINVAL;
		goto exit;
	}

	pos = pos + strlen("bus_awake");

	pos = atbm_skip_space(pos, wrqu->data.length + 1 - (pos - ptr));

	if (pos == NULL) {
		ret = -EINVAL;
		goto exit;
	}

	len = wrqu->data.length + 1 - (pos - ptr);

	if (len <= 0) {
		ret = -EINVAL;
		goto exit;
	}

	pos_end = memchr(pos, ATBM_TAIL, len);

	if (pos_end != NULL)
		len = pos_end - pos + 1;
	/*
	*parase wake
	*/
	ATBM_WEXT_PROCESS_PARAMS(pos, len, wake, atbm_accsii_to_int, false, exit, ret);

	if ((wake != 0) && (wake != 1)) {
		ret = -EINVAL;
		goto exit;
	}

	atbm_printk_always("%s:wake(%d)\n", __func__, wake);

	if (wake) {
		hw_priv->sbus_ops->power_mgmt(hw_priv->sbus_priv, false);
	}
	else {
		hw_priv->sbus_ops->power_mgmt(hw_priv->sbus_priv, true);
	}
exit:
	if (ptr)
		atbm_kfree(ptr);
	return ret;
}

#ifdef CONFIG_JUAN_MISC

static int atbm_ioctl_common_get_tim(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	int ret = 0;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct TIM_Parameters tim_val;
	int tim_con = 0;
	//unsigned short snr = 0;

	if(!ieee80211_sdata_running(sdata) && (sdata->vif.type != NL80211_IFTYPE_AP)){
		atbm_printk_err("atbm_ioctl_common_get_tim : not ap mode! \n");
		return -ENETDOWN;

	}

	
	ret = wsm_get_tim(hw_priv,&tim_val,sizeof(struct TIM_Parameters));
	tim_con = tim_val.tim_val & 1;
	if(tim_con == 1)
	{
		tim_val.tim_val = tim_val.tim_val & (~(1<<0));
		tim_val.tim_val = tim_val.tim_val << 8;
		tim_val.tim_val |= 1;
	}
	else
	{
		tim_val.tim_val = tim_val.tim_val<< 8;
	}
	if(ret == 0){
		sprintf(extra,"\nbeacon tim :%s,Bitmap control:0x%02x , Partial Virtual Bitmap:0x%x\n",tim_val.tim_user_control_ena==0?

"aotu control tim":"customer control tim",
			tim_val.tim_val & 0xff,tim_val.tim_val>>8);
	}else{
		sprintf(extra,"\beacon tim get fail\n");
	}
	wrqu->data.length = strlen(extra);
	return 0;
	
}

static int atbm_ioctl_common_set_tim(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	int ret = 0;
	int i,j = 0;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct TIM_Parameters tim_val;
	char *tim_data = NULL;
	int bit_con = 0;
	int enable = 0, tim_bit = 0;
	
	if(!ieee80211_sdata_running(sdata) && (sdata->vif.type != NL80211_IFTYPE_AP)){
		atbm_printk_err("atbm_ioctl_common_get_tim : not ap mode! \n");
		return -ENETDOWN;

	}


	tim_data = atbm_kzalloc(wrqu->data.length+1, GFP_KERNEL);
	
	if(tim_data == NULL){
		ret = -EINVAL;
		goto set_tim_exit;
	}

	if(copy_from_user(tim_data, wrqu->data.pointer, wrqu->data.length)){
		ret =  -EINVAL;
		goto set_tim_exit;
	}
	atbm_printk_err("val set\n");
	for(i = 0;i < wrqu->data.length - 1; i++)
	{
		if(tim_data[i] == ',')
		{
		 	j ++ ;
			continue;
		}
		if(j == 1)
		{
			enable = enable * 10 + (tim_data[i] - 0x30);
		}
		else if(j == 2)
		{
			if(tim_data[i] >= '0' && tim_data[i] <= '9')
			{
				tim_bit = tim_bit * 16 + (tim_data[i] - 0x30);
			}
			else if(tim_data[i] >= 'a' && tim_data[i] <= 'f')
			{
				tim_bit = tim_bit * 16 + (tim_data[i] - 87);
			}
			else if(tim_data[i] >= 'A' && tim_data[i] <= 'F')
			{
				tim_bit = tim_bit * 16 + (tim_data[i] - 55);
			}
		}
	}
	bit_con = tim_bit & 0xff;
	tim_bit = tim_bit>>8;
	if(bit_con == 1)
	{
		tim_bit |= 1;
	}
	else
	{
		tim_bit &= (~(1<<0));
	}
	atbm_printk_err("get data : ena:%d, val:%x \n",enable,tim_bit);
//	atbm_CmdLine_GetHex();
	tim_val.tim_user_control_ena = enable;
	tim_val.tim_val = tim_bit;
	
	ret = wsm_set_tim(hw_priv,&tim_val,sizeof(struct TIM_Parameters));

	if(ret == 0){
		sprintf(extra,"\nset_tim success\n");
	}else{
		sprintf(extra,"\nset_tim fail\n");
	}
	wrqu->data.length = strlen(extra);

	//ret = 0;
set_tim_exit:
	if(tim_data)
		atbm_kfree(tim_data);
	return ret;
	
}

#endif




#ifdef CONFIG_ATBM_BLE_ADV_COEXIST
static int atbm_ioctl_ble_coexist_cmd(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct wsm_ble_msg_coex_start ble_coex = {0};
	int ret = -1;
	char *pbuffer = NULL;
	int ble_en = 0;
	int ble_interval = 0;
	int ble_adv = 0;
	int ble_scan = 0;
	int ble_scan_win = 0;
	int ble_adv_chan = 0;
	int ble_scan_chan = 0;
	
	if(!(pbuffer = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		goto EXIT;
	}

	if((ret = copy_from_user(pbuffer, wrqu->data.pointer, wrqu->data.length))){
		atbm_printk_err("copy_from_user failed!\n");
		goto EXIT;
	}
	
	pbuffer[wrqu->data.length] = 0;

	sscanf(pbuffer,"enable,%d",&ble_en);
	sscanf(pbuffer,"interval,%d",&ble_interval);
	sscanf(pbuffer,"adv,%d",&ble_adv);
	sscanf(pbuffer,"scan,%d",&ble_scan);
	sscanf(pbuffer,"scan_win,%d",&ble_scan_win);
	sscanf(pbuffer,"adv_chan,%d",&ble_adv_chan);
	sscanf(pbuffer,"scan_chan,%d",&ble_scan_chan);

	atbm_printk_init("ble_en:%d,ble_interval:%d,ble_adv:%d,ble_scan:%d,ble_scan_win:%d,ble_adv_chan:%d,ble_scan_chan:%d\n",
						ble_en, ble_interval, ble_adv, ble_scan, ble_scan_win, ble_adv_chan, ble_scan_chan);

	if(ble_en){
		if((ble_adv == 0) && (ble_scan == 0)){
			atbm_printk_err("both adv and scan is close!\n");
			goto EXIT;
		}
		
		if((ble_scan) && (ble_scan_win == 0)){
			atbm_printk_err("ble scan enable, but scan_win is 0!\n");
			goto EXIT;
		}
		
		if((ble_adv_chan != 0) && (ble_adv_chan >= 37) && (ble_adv_chan <= 39)){
			ble_coex.chan_flag |= BIT(ble_adv_chan - 37);
		}
		
		if((ble_scan_chan != 0) && (ble_scan_chan >= 37) && (ble_scan_chan <= 39)){
			ble_coex.chan_flag |= BIT(ble_scan_chan - 37 + 3);
		}
		
		if(ble_adv){
			ble_coex.coex_flag |= BIT(0);
		}
		
		if(ble_scan){
			ble_coex.coex_flag |= BIT(1);
		}
		
		ble_coex.interval = ble_interval;
		ble_coex.scan_win = ble_scan_win;
		ble_coex.ble_id = BLE_MSG_COEXIST_START;
		ret = wsm_ble_msg_coexist_start(hw_priv, &ble_coex, 0);
	}
	else{
		ble_coex.ble_id = BLE_MSG_COEXIST_STOP;
		ret = wsm_ble_msg_coexist_stop(hw_priv, (struct wsm_ble_msg *)&ble_coex, 0);
	}
	
EXIT:	
	if(ret == 0){
		sprintf(extra,"\nble_coexist success\n");
	}else{
		sprintf(extra,"\nble_coexist fail\n");
	}
	wrqu->data.length = strlen(extra);
	if(pbuffer){
		atbm_kfree(pbuffer);
	}

	return  ret;
}

static int atbm_ioctl_ble_set_adv_data_cmd(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct wsm_ble_msg_adv_data ble_adv = {0};
	int ret = -1;
	char *pbuffer = NULL;
	char *pTmp = NULL;
	unsigned int rxData;

	if(!(pbuffer = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		atbm_printk_err("atbm_kmalloc failed!\n");
		goto EXIT;
	}

	if((ret = copy_from_user(pbuffer, wrqu->data.pointer, wrqu->data.length))){
		atbm_printk_err("copy_from_user failed!\n");
		goto EXIT;
	}

	pTmp = strstr(pbuffer, "mac:");
	if(pTmp){
		pTmp = pTmp + strlen("mac:");
		atbm_CmdLine_GetHex(&pTmp, &rxData);
		ble_adv.mac[0] = rxData;
		atbm_CmdLine_GetHex(&pTmp, &rxData);
		ble_adv.mac[1] = rxData;
		atbm_CmdLine_GetHex(&pTmp, &rxData);
		ble_adv.mac[2] = rxData;
		atbm_CmdLine_GetHex(&pTmp, &rxData);
		ble_adv.mac[3] = rxData;
		atbm_CmdLine_GetHex(&pTmp, &rxData);
		ble_adv.mac[4] = rxData;
		atbm_CmdLine_GetHex(&pTmp, &rxData);
		ble_adv.mac[5] = rxData;
	}
	
	pTmp = strstr(pbuffer, "data:");
	if(pTmp){
		pTmp = pTmp + strlen("data:");
		ble_adv.adv_data_len = strlen(pTmp);
		if(ble_adv.adv_data_len > 31){
			ble_adv.adv_data_len = 31;
		}
		memcpy(ble_adv.adv_data, pTmp, 31);
		ble_adv.ble_id = BLE_MSG_SET_ADV_DATA;
		ret = wsm_ble_msg_set_adv_data(hw_priv, &ble_adv, 0);
	}

EXIT:	
	if(ret == 0){
		sprintf(extra,"\nble set adv data success\n");
	}else{
		sprintf(extra,"\nble set adv data fail\n");
	}
	wrqu->data.length = strlen(extra);
	if(pbuffer){
		atbm_kfree(pbuffer);
	}

	return  ret;
}
#endif

#ifdef CONFIG_ATBM_SUPPORT_REKEY
static int atbm_ioctl_set_rekey(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	int ret = 0;
	bool mgd_lock = false;
	char *ptr  = NULL;
	const char *pos;
	const char *pos_end;
	int len = 0;
	int i = 0;
	int enable = 0;
	printk("%s %d\n",__func__,__LINE__);
	
	if(!ieee80211_sdata_running(sdata)){
		printk("%s %d\n",__func__,__LINE__);
		atbm_printk_wext("%s:%d\n",__func__,__LINE__);
		ret = -ENETDOWN;
		goto exit;
	}

	if(sdata->vif.type != NL80211_IFTYPE_STATION){
		printk("%s %d\n",__func__,__LINE__);
		ret = -EINVAL;
		goto exit;
	}

	mutex_lock(&sdata->u.mgd.mtx);
	
	mgd_lock = true;
	
	if (!sdata->u.mgd.associated){
		printk("%s %d\n",__func__,__LINE__);
		ret = -EINVAL;
		goto exit;
	}

	if(sdata->vif.rekey_set == 0){
		atbm_printk_err("key data not set\n");
		ret = -EINVAL;
		goto exit;
	}
	if(!(ptr = atbm_kmalloc(wrqu->data.length + 1, GFP_KERNEL))){
		printk("%s %d\n",__func__,__LINE__);
		ret = -ENOMEM;
		goto exit;
	}
	
	if((ret = copy_from_user(ptr, wrqu->data.pointer, wrqu->data.length)) != 0){
		ret = -ENOMEM;
		printk("%s %d\n",__func__,__LINE__);
		goto  exit;
	}

	ptr[wrqu->data.length] = 0;
	
	for(i = 0;i<wrqu->data.length;i++){
		if(ptr[i] == ',')
			ptr[i] = ATBM_SPACE;
	}
	
	pos = atbm_skip_space(ptr,wrqu->data.length+1);

	if(pos == NULL){
		printk("%s %d\n",__func__,__LINE__);
		ret = -EINVAL;
		goto exit;
	}

	pos = pos+strlen("set_rekay");

	pos = atbm_skip_space(pos,wrqu->data.length+1-(pos-ptr));

	if(pos == NULL){
		printk("pos == null\n");
		ret = -EINVAL;
		goto exit;
	}

	len = wrqu->data.length + 1 - (pos - ptr);

	if(len <= 0){
		printk("len %d\n",len);
		ret = -EINVAL;
		goto exit;
	}

	pos_end = memchr(pos,ATBM_TAIL,len);
	
	if(pos_end != NULL)
		len = pos_end - pos+1;
	
	ATBM_WEXT_PROCESS_PARAMS(pos,len,enable,atbm_accsii_to_int,false,exit,ret);

	if((enable != 0) && (enable != 1)){
		printk("enable %d\n",enable);
		ret = -EINVAL;
		goto exit;
	}

	drv_set_rekey_data(sdata->local,sdata,enable);
	
exit:
	if(mgd_lock == true)
		mutex_unlock(&sdata->u.mgd.mtx);
	if(ptr)
		atbm_kfree(ptr);
	return ret;
}
#endif

static int atbm_ioctl_common_set_temp_dcxo_table(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	int ret = 0;

	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_temp_dcxo_table temp_dcxo_table;

	memset(&temp_dcxo_table, 0, sizeof(temp_dcxo_table));
	
	atbm_read_temp_dcxo_table(&temp_dcxo_table);

	ret = wsm_set_temp_dcxo_table(hw_priv, &temp_dcxo_table, 0);
	if(ret != 0)
		atbm_printk_err("set temp and dcxo table failed,ret:%d\n", ret);
	
	return ret;
	
}

static int atbm_ioctl_common_cmd(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	int ret = 0,cmd_match = 0;
	iwripv_common_cmd_t *cmd = common_cmd;
	char *ptr = NULL;
	char *comma = NULL;
	int input_cmd_len = 0;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	atbm_printk_wext("atbm_ioctl_common_cmd(), length %d\n",wdata->data.length);

	if(!(ptr = (char *)atbm_kzalloc(wdata->data.length+1, GFP_KERNEL)))
		return -ENOMEM;

	memset(ptr,0,wdata->data.length+1);
	if((ret = copy_from_user(ptr, wdata->data.pointer, wdata->data.length))){
		atbm_kfree(ptr);
		return -EINVAL;
	}
	atbm_printk_err("cmd:%s , len=%d \n",ptr,wdata->data.length);

	comma = strchr(ptr,',');
	if(comma == NULL)
		input_cmd_len = wdata->data.length - 1;
	else{
		atbm_printk_err("comma=%s, input_cmd_len = %ld\n",comma,comma - ptr);
		input_cmd_len = comma - ptr;
	}
		

	
	while(cmd->cmd){
		if((input_cmd_len == cmd->cmd_len) &&(memcmp(ptr, cmd->cmd, cmd->cmd_len) == 0)){
			cmd_match = 1;
			break;
		}
		cmd++;
	}
	if(cmd_match)
		ret = cmd->handler(dev, info, wrqu, extra);
//	else
//		ret = atbm_ioctl_common_help(dev, info, wrqu, extra);
#if 0
	if(memcmp(ptr, "get_rssi", 8) == 0)
		ret = atbm_ioctl_get_rssi(dev, info, wrqu, extra);
	else if(memcmp(ptr, "stations_show", 13) == 0)
		ret = atbm_ioctl_associate_sta_status(dev, info, wrqu, extra);
	else if(memcmp(ptr, "getSigmstarEfuse", 16) == 0)
		ret = atbm_ioctl_get_SIGMSTAR_256BITSEFUSE(dev, info, wrqu, extra);
	else if(memcmp(ptr, "setSigmstarEfuse", 16) == 0)
		ret = atbm_ioctl_set_SIGMSTAR_256BITSEFUSE(dev, info, wrqu, extra);
#ifdef CONFIG_ATBM_IWPRIV_USELESS
	else if(memcmp(ptr, "channel_test_start", 18) == 0)
		ret = atbm_ioctl_channel_test_start(dev, info, wrqu, extra);
#endif
	else if(memcmp(ptr, "get_channel_idle", 16) == 0)
		ret = atbm_ioctl_get_channel_idle(dev, info, wrqu, extra);
	else if(memcmp(ptr, "setEfuse_dcxo", 13) == 0)
		ret = atbm_ioctl_set_efuse(dev, info, wrqu, extra);
	else if(memcmp(ptr, "setEfuse_deltagain", 18) == 0)
		ret = atbm_ioctl_set_efuse(dev, info, wrqu, extra);
	else if(memcmp(ptr, "setEfuse_tjroom", 15) == 0)
		ret = atbm_ioctl_set_efuse(dev, info, wrqu, extra);
	else if(memcmp(ptr, "setEfuse_mac", 12) == 0)
		ret = atbm_ioctl_set_efuse(dev, info, wrqu, extra);
	else if(memcmp(ptr, "getEfuse", 8) == 0)
		ret = atbm_ioctl_get_efuse(dev, info, wrqu, extra);
	else if(memcmp(ptr, "EfusefreeSpace", 14) == 0)
		ret = atbm_ioctl_get_efuse_free_space(dev, info, wrqu, extra);
	else if(memcmp(ptr, "getefusefirst", 13) == 0)
		ret = atbm_ioctl_get_efuse_first(dev, info, wrqu, extra);
#ifdef CONFIG_ATBM_IWPRIV_USELESS
	else if(memcmp(ptr, "freqoffset", 10) == 0)
		ret = atbm_ioctl_freqoffset(dev, info, wrqu, extra);
#endif
	else if(memcmp(ptr, "singletone", 10) == 0)
		ret = atbm_ioctl_send_singleTone(dev, info, wrqu, extra);
#ifdef CONFIG_ATBM_IWPRIV_USELESS
	else if(memcmp(ptr, "duty_ratio", 10) == 0)
		ret = atbm_ioctl_set_duty_ratio(dev, info, wrqu, extra);
#endif
#ifdef CONFIG_ATBM_SUPPORT_AP_CONFIG
	else if(memcmp(ptr, "ap_conf", 7) == 0)
		ret = atbm_ioctl_set_ap_conf(dev, info, wrqu, extra);
#endif
#ifdef CONFIG_ATBM_MONITOR_SPECIAL_MAC
	else if(memcmp(ptr, "monitor_mac", 11) == 0)
		ret = atbm_ioctl_rx_monitor_mac(dev, info, wrqu, extra);
	else if(memcmp(ptr, "monitor_status", 14) == 0)
		ret = atbm_ioctl_rx_monitor_mac_status(dev, info, wrqu, extra);
#endif
#ifdef CONFIG_IEEE80211_SPECIAL_FILTER
	else if(memcmp(ptr,"filter_frame",12) == 0){
		ret = atbm_ioctl_rx_filter_frame(dev, info, wrqu, extra);
	}else if(memcmp(ptr,"filter_ie",9) == 0){
		ret = atbm_ioctl_rx_filter_ie(dev, info, wrqu, extra);
	}else if(memcmp(ptr,"filter_clear",12) == 0){
		ret = atbm_ioctl_rx_filter_clear(dev, info, wrqu, extra);
	}else if(memcmp(ptr,"filter_show",11) == 0){
		ret = atbm_ioctl_rx_filter_show(dev, info, wrqu, extra);
	}
#endif
#ifdef HIDDEN_SSID
	else if(memcmp(ptr, "ap_hide_ssid", 12) == 0){
		ret = atbm_ioctl_set_hidden_ssid(dev, info, wrqu, extra);
	}
#endif
	else if(memcmp(ptr,"get_last_mac",12) == 0){
		ret = atbm_ioctl_get_Last_mac(dev, info, wrqu, extra);
	}else if(memcmp(ptr,"get_first_mac",13) == 0){
		ret = atbm_ioctl_get_First_mac(dev, info, wrqu, extra);
	}else if(memcmp(ptr,"get_tjroom",10) == 0){
		ret = atbm_ioctl_get_Tjroom(dev, info, wrqu, extra);
	}else if(memcmp(ptr,"set_cali_flag",13) == 0){
		ret = atbm_ioctl_set_calibrate_flag(dev, info, wrqu, extra);
	}else if(memcmp(ptr,"set_all_efuse",13) == 0){
		ret = atbm_ioctl_set_all_efuse(dev, info, wrqu, extra);
	}else if(memcmp(ptr,"stations_show",13) == 0){
		ret = atbm_ioctl_associate_sta_status(dev, info, wrqu, extra);
	}
#ifdef ATBM_PRIVATE_IE
	else if(memcmp(ptr,"ipc_reset",9) == 0){
		atbm_ioctl_ie_ipc_clear_insert_data(dev, info, wrqu, extra);
	}
#endif

//#ifdef ATBM_PS_MODE
	else if(memcmp(ptr,"power_save",10) == 0){

		atbm_set_power_save_mode(dev, info, wrqu, extra);
	}
#ifdef JA_SUBTYPE_OXF0
else if(memcmp(ptr,"subtype",7) == 0){
		ret = atbm_ioctl_subtype(dev, info, wrqu, extra);
	}
#endif
	else if(memcmp(ptr,"gpio_conf",9) == 0){
		ret = atbm_ioctl_gpio_config(dev, info, wrqu, extra);
	}else if(memcmp(ptr,"gpio_out",8) == 0){
		ret = atbm_ioctl_gpio_output(dev, info, wrqu, extra);
	}else if(memcmp(ptr,"edca_params",11) == 0){
		ret = atbm_ioctl_edca_params(dev, info, wrqu, extra);
	}else if(memcmp(ptr,"get_edca_params",15) == 0){
		ret = atbm_ioctl_get_edca_params(dev, info, wrqu, extra);
	}else if(memcmp(ptr,"txpower_status",14) == 0){
		ret = atbm_ioctl_get_txposer_status(dev, info, wrqu, extra);
	}else if(memcmp(ptr,"settxpower_byfile",17) == 0){
		ret = atbm_ioctl_set_txpwr_by_file(dev, info, wrqu, extra);
	}else if(memcmp(ptr,"set_rate_power",14) == 0){
		ret = atbm_ioctl_set_rate_txpwr_by_file(dev, info, wrqu, extra);
	}else if(memcmp(ptr,"set_frame_rate",14) == 0){
		ret = atbm_ioctl_set_management_frame_rate(dev, info, wrqu, extra);
	}else if(memcmp(ptr,"get_maxrate",11) == 0){
		ret = atbm_ioctl_get_cur_max_rate(dev, info, wrqu, extra);
	}else if (memcmp(ptr,"atbm_get_work_channel",16) == 0){
		ret = atbm_ioctl_atbm_get_work_channel(dev, info, wrqu, extra);
	}else
		ret = -1;
#endif

	//atbm_printk_err("extra:%s\n",extra);
	if(ret < 0)
		atbm_printk_err("atbm_ioctl_common_cmd(), error %s\n", ptr);
	atbm_kfree(ptr);
	
	return ret;
}


#ifdef CONFIG_ATBM_STA_LISTEN
static int atbm_ioctl_set_sta_channel(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	unsigned char *ptr = NULL;
	const unsigned char *pos = NULL;
	const unsigned char *pos_end = NULL;
	int len = 0;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	int channel = 0;
	int ret = 0;
	
	if(wdata->data.length <= 0){
		ret =  -EINVAL;
		goto exit;
	}
	ptr = (char *)atbm_kzalloc(wdata->data.length+1, GFP_KERNEL);

	if(ptr == NULL){
		ret = -ENOMEM;
		goto exit;
	}

	if(copy_from_user(ptr, wdata->data.pointer, wdata->data.length) != 0){
		ret = -EINVAL;
		goto exit;
	}
	
	atbm_printk_wext("%s:len(%d)(%s)\n",__func__,wdata->data.length,ptr);
	/*
	*skip space
	*/

	pos = atbm_skip_space(ptr,wdata->data.length);

	if(pos == NULL){		
		ret = -EINVAL;
		goto exit;
	}
	
	len = (int)(wdata->data.length - (pos-ptr));

	pos_end = memchr(pos,ATBM_TAIL,len);

	if(pos_end != NULL)
		len = pos_end - pos;
	
	if(len >2){
		ret = -EINVAL;
		goto exit;
	}
	
	if(atbm_accsii_to_int(pos,len,&channel) == false){		
		ret = -EINVAL;
		goto exit;
	}
	ret = ieee80211_set_sta_channel(sdata,channel);
	
exit:
	if(ptr)
		atbm_kfree(ptr);
	return ret;
}

#endif
static int atbm_ioctl_rx_statistic(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{

	u32 rx_status_success = 0;
	u32 rx_status_error = 0;
	u8 *status = NULL;
	int len = 0;
	int ret = 0;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;

	ret = atbm_internal_get_start_rx_results(hw_priv,&rx_status_success,&rx_status_error);
	if(ret < 0){
		atbm_printk_wext("%s:atbm_internal_get_start_rx_results err\n",__func__);
		goto exit;
	}

	status = atbm_kzalloc(512,GFP_KERNEL);
	
	if(status == NULL){
		atbm_printk_wext("%s:atbm_kzalloc err\n",__func__);
		ret = -ENOMEM;
		goto exit;
	}

	len = scnprintf(status,512,"rxSuccess:%d, rxError:%d\n", rx_status_success, rx_status_error);

	if(extra){
		memcpy(extra,status,len);	
		wrqu->data.length = strlen(status) + 1;
	}
exit:
	if(status)
		atbm_kfree(status);

	return ret;
}
#ifdef CONFIG_ATBM_PRIV_HELP
static int atbm_ioctl_command_help(struct net_device * dev,struct iw_request_info * ifno,union iwreq_data * wrqu,char * ext)
{
	int ret = 0;
		atbm_printk_err("usage: 		iwpriv wlan0 <cmd> [para]..,\n");
		atbm_printk_err("fwdbg		<0|1> = gets/sets firmware debug message switch, when setting, 0, close, 1, open\n");
		atbm_printk_err("fwcmd		<firmware cmd line> = letting firmware performance command,e.g:fwcmd para1,para2,parq3....\n");
		atbm_printk_err("start_tx 	<channel,mode,rate,bw,chOff,len> = start transmmit \n");
		atbm_printk_err("		channel value:1~14 \n");
		atbm_printk_err("		mode 0:11b;1:11g:2:11n:3:11ax \n");
		atbm_printk_err("		rate id: \n");
		atbm_printk_err("			11b [0,1,2,3] --> [1M, 2M, 5.5M, 11M] \n");
		atbm_printk_err("			11g [0,1,2,3,4,5,6,7] --> [6M, 9M, 12M, 18M, 24M, 36M, 48M, 54M] \n");
		atbm_printk_err("			11n [0,1,2,3,4,5,6,7] --> [mcs0, ..... mcs7] \n");
		atbm_printk_err("			11ax [0,11] --> [mcs0, mcs11] \n");
		atbm_printk_err("		bw 0:20M;1:40M;2:RU242;3:RU106; \n");
		atbm_printk_err("		chOff 0:zero;1:10U;2:10L; \n");
		atbm_printk_err("		len range:100~1024\n");
		atbm_printk_err("stop_tx		NO prarameter = stop transmmit\n");
		atbm_printk_err("start_rx 	<<channel>,<bw>,<chOff>> = start receive,parameter:channel,channel type;1:40M,0:20M\n");
		atbm_printk_err("		channel value:1~14 \n");
		atbm_printk_err("		bw 0:20M;1:40M;2:RU242;3:RU106; \n");
		atbm_printk_err("		chOff 0:zero;1:10U;2:10L; \n");
		atbm_printk_err("stop_rx		NO parameter = stop receive\n");
		atbm_printk_err("set_gi		<0|1> = 1,support shortGI; 0, not support shortGI\n");
		atbm_printk_err("rts_threshold		<show|value> = show or set rts threshold\n");
		atbm_printk_err("rate		<show|free|rate id> = show rate,or free a fixed rate,or forces a fixed rate support");
		atbm_printk_err(" rate id: 1, 2, 5.5, 11, 6, 9, 12, 18, 24, 36, 48, 54, 6.5, 13, 19.5,26, 39, 52, 58.5, 65\n");
		atbm_printk_err("getmac		NO Parameter = get mac address\n");
		atbm_printk_err("wolEn		<0|1> = 1,enable wol; 0, disable wol\n");
		atbm_printk_err("get_rx_stats	<1>\n");
		atbm_printk_err("freqoffset		<0|1> 0:not write efuse; 1: write efuse \n");
		atbm_printk_err("etf_result_get  get the test result \n");
#ifdef ATBM_PRIVATE_IE
		atbm_printk_err("insert_data      insert private user data to (Probe Req or Beacon or Probe Resp) \n");
		atbm_printk_err("get_data         obtain private user data from the broadcast packet \n");
		atbm_printk_err("send_msg         start private scan according to the channel setting \n");
		atbm_printk_err("recv_msg         recv broadcast device information of contain private IE \n");
		atbm_printk_err("ipc_reset        reset ipc private scan function \n");
		atbm_printk_err("channel_switch   switch channle to another \n");
#endif
//		atbm_printk_err("hide_ssid        Enable or Disble hidden ssid func, only for AP mode \n");
		atbm_printk_err("set_freq		 param1: channel [1:14],param2: freq val(2380 or 2504) \n");
		atbm_printk_err("set_txpower         1: high tx power, 0: normal tx power \n");
		atbm_printk_err("get_tp_rate         obtain current throughput, AP mode need to add mac address of station \n");
		atbm_printk_err("best_ch_start         start the best channel scan \n");
		atbm_printk_err("best_ch_end		   end the best channel scan \n");
		atbm_printk_err("best_ch_rslt		   get the best channel scan result \n");
#ifdef SSTAR_FUNCTION
		atbm_printk_err("getSigmstarEfuse		   get sigmstar 256bits efuse \n");
		atbm_printk_err("setSigmstarEfuse		   set sigmstar 256bits efuse \n");
#endif
		
		atbm_printk_err("common help 	get lager infomation\n");
	return ret;
}
#endif

static const struct iw_priv_args atbm_priv_tab[] = {
#ifdef CONFIG_ATBM_PRIV_HELP
		{SIOCIWFIRSTPRIV + 0x0, IW_PRIV_TYPE_CHAR | 17, 0, 		"help"},
#endif //CONFIG_ATBM_PRIV_HELP
		{SIOCIWFIRSTPRIV + 1, IW_PRIV_TYPE_CHAR | 1000, 0, 		"start_tx"},
		{SIOCIWFIRSTPRIV + 2, IW_PRIV_TYPE_CHAR | 513, 	0, 		"stop_tx"},
		{SIOCIWFIRSTPRIV + 3, IW_PRIV_TYPE_CHAR | 50, 	0, 		"start_rx"},
		{SIOCIWFIRSTPRIV + 4, 0, IW_PRIV_TYPE_CHAR|513, 		"stop_rx"},
		{SIOCIWFIRSTPRIV + 5, IW_PRIV_TYPE_CHAR | 5, 0, 		"fwdbg"},
		{SIOCIWFIRSTPRIV + 6, 0, IW_PRIV_TYPE_CHAR|513, 		"rx_statis"},
		{SIOCIWFIRSTPRIV + 7, IW_PRIV_TYPE_CHAR | 1024,IW_PRIV_TYPE_CHAR|1024, "common"},
		{SIOCIWFIRSTPRIV + 8, IW_PRIV_TYPE_CHAR | 50, 0, 		"fwcmd"},
		{SIOCIWFIRSTPRIV + 9, IW_PRIV_TYPE_CHAR | 10,  0, 		"set_gi"},
		{SIOCIWFIRSTPRIV + 10, IW_PRIV_TYPE_CHAR | 10, 0, 		"getmac"},
#ifdef CONFIG_ATBM_START_WOL_TEST
		{SIOCIWFIRSTPRIV + 11, IW_PRIV_TYPE_CHAR | 10, 0,		"wolEn"},
#endif //CONFIG_ATBM_START_WOL_TEST
#ifdef CONFIG_ATBM_IWPRIV_USELESS
		{SIOCIWFIRSTPRIV + 12, IW_PRIV_TYPE_CHAR | 16, 0, 		"get_rx_stats"},
#endif //CONFIG_ATBM_IWPRIV_USELESS
		{SIOCIWFIRSTPRIV + 13, IW_PRIV_TYPE_CHAR | 10, 0, 		"rts_threshold"},
		{SIOCIWFIRSTPRIV + 14, IW_PRIV_TYPE_CHAR | 16, 0, 		"etf_result_get"},
		//{SIOCIWFIRSTPRIV + 15, IW_PRIV_TYPE_CHAR | 32, 0, "get_state"},
		//{SIOCIWFIRSTPRIV + 16, IW_PRIV_TYPE_CHAR | 32, 0, "hide_ssid"},
		{SIOCIWFIRSTPRIV + 15, IW_PRIV_TYPE_CHAR | 600, IW_PRIV_TYPE_CHAR|600, "best_ch_scan"},				
		{SIOCIWFIRSTPRIV + 16, IW_PRIV_TYPE_CHAR | 32, 0, 		"set_txpower"},
		{SIOCIWFIRSTPRIV + 17, IW_PRIV_TYPE_CHAR | 32, 0, 		"get_tp_rate"},
		{SIOCIWFIRSTPRIV + 18, IW_PRIV_TYPE_CHAR | 32, 0, 		"set_freq"},
#ifdef CONFIG_ATBM_STA_LISTEN
		{SIOCIWFIRSTPRIV + 19, IW_PRIV_TYPE_CHAR | 32, 0, 		"sta_channel"},
#endif //CONFIG_ATBM_STA_LISTEN

#ifdef ATBM_PRIVATE_IE
		/*Private IE for Scan*/
		{SIOCIWFIRSTPRIV + 20, IW_PRIV_TYPE_CHAR | 500, 0, 		"insert_data"},
		{SIOCIWFIRSTPRIV + 21, IW_PRIV_TYPE_CHAR | 500, 0, 		"get_data"},
		{SIOCIWFIRSTPRIV + 22, IW_PRIV_TYPE_CHAR | 500, 0, 		"send_msg"},
		{SIOCIWFIRSTPRIV + 23, IW_PRIV_TYPE_CHAR | 500, 0, 		"recv_msg"},
#endif	//ATBM_PRIVATE_IE
};

const iw_handler atbm_priv_handler[]={
#ifdef CONFIG_ATBM_PRIV_HELP
	[0] = (iw_handler)atbm_ioctl_command_help,
#endif
	[1] = (iw_handler)atbm_ioctl_start_tx,
	[2] = (iw_handler)atbm_ioctl_stop_tx,
	[3] = (iw_handler)atbm_ioctl_start_rx,
	[4] = (iw_handler)atbm_ioctl_stop_rx,
	[5] = (iw_handler)atbm_ioctl_fwdbg,
	[6] = (iw_handler)atbm_ioctl_rx_statistic,
	[7] = (iw_handler)atbm_ioctl_common_cmd,
	[8] = (iw_handler)atbm_ioctl_fwcmd,
	[9] = (iw_handler)atbm_ioctl_ctl_gi,
	[10] = (iw_handler)atbm_ioctl_getmac,
#ifdef CONFIG_ATBM_START_WOL_TEST
	[11] = (iw_handler)atbm_ioctl_start_wol,
#endif//CONFIG_ATBM_START_WOL_TEST
#ifdef CONFIG_ATBM_IWPRIV_USELESS
	[12] = (iw_handler)atbm_ioctl_get_rx_stats,
#endif //CONFIG_ATBM_IWPRIV_USELESS
	[13] = (iw_handler)atbm_ioctl_set_rts,
	[14] = (iw_handler)atbm_ioctl_etf_result_get,	
	
	[15] = (iw_handler)atbm_ioctl_best_ch_scan,
	[16] = (iw_handler)atbm_ioctl_set_txpw,
	[17] = (iw_handler)atbm_ioctl_get_rate,
	[18] = (iw_handler)atbm_ioctl_set_freq,
#ifdef CONFIG_ATBM_STA_LISTEN
	[19] = (iw_handler)atbm_ioctl_set_sta_channel,
#endif//CONFIG_ATBM_STA_LISTEN
#ifdef ATBM_PRIVATE_IE
	/*Private IE for Scan*/
	[20] = (iw_handler)atbm_ioctl_ie_insert_user_data,
	[21] = (iw_handler)atbm_ioctl_ie_get_user_data,
	[22] = (iw_handler)atbm_ioctl_ie_send_msg,
	[23] = (iw_handler)atbm_ioctl_ie_recv_msg,
#endif	//ATBM_PRIVATE_IE
};


void register_wext_common(struct ieee80211_local *local){

#ifdef CONFIG_WIRELESS_EXT
#ifdef CONFIG_CFG80211_WEXT
	if(local->hw.wiphy->wext)
	{
	        atbm_handlers_def.standard = local->hw.wiphy->wext->standard;
	        atbm_handlers_def.num_standard = local->hw.wiphy->wext->num_standard;
		#if WIRELESS_EXT >= 17
	        atbm_handlers_def.get_wireless_stats = local->hw.wiphy->wext->get_wireless_stats;
		#endif
#endif
		#ifdef CONFIG_WEXT_PRIV
			atbm_handlers_def.num_private = sizeof(atbm_priv_handler)/sizeof(atbm_priv_handler[0]) ;
			atbm_handlers_def.private = atbm_priv_handler ;
			atbm_handlers_def.num_private_args = sizeof(atbm_priv_tab)/sizeof(atbm_priv_tab[0]);
			atbm_handlers_def.private_args = (struct iw_priv_args *)atbm_priv_tab;
		#endif
#ifdef CONFIG_CFG80211_WEXT
	}
#endif
#endif

}

int atbm_ioctl_etf_result_get(struct net_device *dev, struct iw_request_info *info, union iwreq_data  *wrqu, char *extra)
{
	int ret = 0;
	u8 chipid = 0;
	u8 buff[512];

	chipid = chipversion;
	memset(buff, 0, 512);
#ifdef ATBM_PRODUCT_TEST_USE_FEATURE_ID
	sprintf(buff, "%d%dcfo:%d,txevm:%d,rxevm:%d,dcxo:%d,txrssi:%d,rxrssi:%d,result:%d (0:OK; -1:FreqOffset Error; -2:efuse hard error;"
		" -3:efuse no written; -4:efuse anaysis failed; -5:efuse full; -6:efuse version change; -7:rx null)",
	gRxs_s.valid,
	chipid,
	gRxs_s.Cfo,
	gRxs_s.txevm,
	gRxs_s.evm,
	gRxs_s.dcxo,
	gRxs_s.TxRSSI,
	gRxs_s.RxRSSI,
	gRxs_s.result
	);
#else
	//cvte 6022 use older version atbm_iw tool
	sprintf(buff, "%dcfo:%d,txevm:%d,rxevm:%d,dcxo:%d,txrssi:%d,rxrssi:%d,result:%d,share:%d (0:OK; -1:FreqOffset Error; -2:efuse hard error;"
		" -3:efuse no written; -4:efuse anaysis failed; -5:efuse full; -6:efuse version change; -7:rx null)",
	gRxs_s.valid,
	gRxs_s.Cfo,
	gRxs_s.txevm,
	gRxs_s.evm,
	gRxs_s.dcxo,
	gRxs_s.TxRSSI,
	gRxs_s.RxRSSI,
	gRxs_s.result,
	etf_config.chip_crystal_type
	);
#endif
	if((ret = copy_to_user(wrqu->data.pointer, buff, strlen(buff))) != 0){
		return -EINVAL;
	}

	return ret;
}


#endif //CONFIG_WIRELESS_EXT





//config etf test arguments by config_param.txt
static int etf_rx_status_get(struct atbm_common *hw_priv)
{
	int ret = 0;
	int i = 0;
	struct rxstatus rxs; 
	char *extra = NULL;
	struct atbm_vif *vif;

	atbm_printk_err("[%s]:%d\n", __func__, __LINE__);
	if(!(extra = atbm_kmalloc(sizeof(struct rxstatus), GFP_KERNEL)))
	{
		atbm_printk_err("%s:malloc failed\n", __func__);
		return -ENOMEM;	
	}

	atbm_for_each_vif(hw_priv,vif,i){
		if (vif != NULL)
		{
			ret = wsm_read_mib(hw_priv, WSM_MIB_ID_GET_ETF_RX_STATS,
				extra, sizeof(struct rxstatus), vif->if_id);
			break;
		}
	}
	memcpy(&rxs, extra, sizeof(struct rxstatus));

	if(rxs.probcnt == 0)
	{
		atbm_printk_err("%s:rxs.probcnt == 0\n", __func__);
		goto out;
	}

	if(ret == 0)
	{
		gRxs_s.evm				= rxs.evm/rxs.probcnt;
		gRxs_s.RxRSSI			= (s16)N_BIT_TO_SIGNED_32BIT(rxs.RSSI, 8)*4;
		gRxs_s.RxRSSI += etf_config.cableloss;	
	}
	else
	{
		atbm_printk_err("%s:get rx status failed\n", __func__);
	}
out:
	if(extra)
		atbm_kfree(extra);
	return ret;

}

#ifdef CONFIG_ATBM_IWPRIV_USELESS
#ifdef CONFIG_WIRELESS_EXT
static int getFreqoffsetHz(struct atbm_common *hw_priv, struct rxstatus_signed *rxs_s)
{
	struct rxstatus rxs; 
	int FreqOffsetHz;
	char *extra = NULL;
	int i = 0;
	long lcfo = 0;
	struct atbm_vif *vif;
	if(!(extra = atbm_kmalloc(sizeof(struct rxstatus), GFP_KERNEL)))
		return -ENOMEM;

	atbm_for_each_vif(hw_priv,vif,i){
				if (vif != NULL)
				{
					WARN_ON(wsm_read_mib(hw_priv, WSM_MIB_ID_GET_ETF_RX_STATS,
						extra, sizeof(struct rxstatus), vif->if_id));
					break;
				}
			}
	memcpy(&rxs, extra, sizeof(struct rxstatus));
	
	atbm_kfree(extra);
	
#if 0
	printk("Cfo:%d,RSSI:%d,evm:%d,GainImb:%d, PhaseImb:%d, FreqOffsetHz:%d\n",
	rxs.Cfo,
	rxs.RSSI,
	rxs.evm,
	rxs.GainImb,
	rxs.PhaseImb,
	FreqOffsetHz
	);
#endif
	
	rxs_s->GainImb		= (s16)N_BIT_TO_SIGNED_32BIT(rxs.GainImb, 10);
	rxs_s->PhaseImb		= (s16)N_BIT_TO_SIGNED_32BIT(rxs.PhaseImb, 10);
	rxs_s->Cfo			= (s16)N_BIT_TO_SIGNED_32BIT(rxs.Cfo, 16);
	rxs_s->evm			= rxs.evm;
	rxs_s->RxRSSI		= (s8)N_BIT_TO_SIGNED_32BIT(rxs.RSSI, 8);
	lcfo 				= rxs_s->Cfo;
	FreqOffsetHz 		= (int)(((lcfo*12207)/10)*(-1));

	atbm_printk_wext("Host Rx: Cfo:%d,RxRSSI:%d,evm:%d,GainImb:%d, PhaseImb:%d, FreqOffsetHz:%d\n",
					rxs_s->Cfo,
					rxs_s->RxRSSI,
					rxs_s->evm,
					rxs_s->GainImb,
					rxs_s->PhaseImb,
					FreqOffsetHz
					);
	

	return FreqOffsetHz;
}

static int _getMaxRssiInd(struct rxstatus_signed rxs_arr[], int cnt)
{
	int i=0;
	struct rxstatus_signed *rxsMax;
	int cntMax = 0;

	rxsMax = &rxs_arr[0];

	atbm_printk_wext("_getMaxRssiInd()\n");
	
	for(i=1; i<cnt; i++)
	{
#if 0
		printk("#Cfo:%d,RxRSSI:%d,evm:%d,GainImb:%d, PhaseImb:%d\n",
		rxs_arr[i].Cfo,
		rxs_arr[i].RxRSSI,
		rxs_arr[i].evm,
		rxs_arr[i].GainImb,
		rxs_arr[i].PhaseImb
		);
#endif
		if(rxs_arr[i].RxRSSI >= rxsMax->RxRSSI)
		{
			if(rxs_arr[i].evm <= rxsMax->evm)
			{
				rxsMax = &rxs_arr[i];
				cntMax = i;
			}
		}
	}

	atbm_printk_wext("Host Rx: MaxRssi[%d]#Cfo:%d,RxRSSI:%d,evm:%d,GainImb:%d, PhaseImb:%d\n",
	cntMax,
	rxsMax->Cfo,
	rxsMax->RxRSSI,
	rxsMax->evm,
	rxsMax->GainImb,
	rxsMax->PhaseImb
	);

	return cntMax;
}

static int Test_FreqOffset(struct atbm_common *hw_priv, u32 *dcxo, int *pfreqErrorHz, struct rxstatus_signed *rxs_s, int channel)
{
	u8 CodeValue;
	u8 CodeValuebak;
	u8 CodeStart,CodeEnd;
	int b_fail =1;
	int freqErrorHz = 0;
	int targetFreqOffset = TARGET_FREQOFFSET_HZ;
	struct atbm_vif *vif;

	char cmd[64];
	int i = 0;
	int itmp=0;
	int index=0;
	u8 ucDbgPrintOpenFlag = 1;
	struct rxstatus_signed rxs_arr[FREQ_CNT];
	int freqErrorHz_arr[FREQ_CNT];
	
	CodeValue = atbm_DCXOCodeRead(hw_priv);	
	atbm_DCXOCodeWrite(hw_priv,CodeValue);	

	if(ETF_bStartTx || ETF_bStartRx){
		atbm_printk_err("Error! already start_tx, please stop_tx first!\n");
		return b_fail;
	}


	//./iwpriv wlan0 fwdbg 1
	ucDbgPrintOpenFlag = 1;
	atbm_for_each_vif(hw_priv,vif,i){
				if (vif != NULL)
				{
					WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_DBG_PRINT_TO_HOST,
						&ucDbgPrintOpenFlag, sizeof(ucDbgPrintOpenFlag), vif->if_id));
					break;
				}
			}


	//start DUT Rx

	sprintf(cmd,  "monitor 1,%d,0",channel);
	
	atbm_printk_wext("start DUT Rx CMD:%s\n", cmd);
	atbm_for_each_vif(hw_priv,vif,i){
		if (vif != NULL)
		{
			ETF_bStartRx = 1;
			WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_FW_CMD,
				cmd, strlen(cmd) + 10, vif->if_id));
			break;
		}
	}

	CodeStart = DCXO_CODE_MINI;
	CodeEnd = DCXO_CODE_MAX;

	msleep(50);
	atbm_printk_wext("CodeValue default:%d\n",CodeValue);
	while(1)
	{
		CodeValuebak = CodeValue;
		for (itmp=0;itmp<FREQ_CNT;itmp++)
		{
			msleep(10);
			freqErrorHz_arr[itmp] = getFreqoffsetHz(hw_priv, &rxs_arr[itmp]);
		}
		index = _getMaxRssiInd(rxs_arr,FREQ_CNT);
		freqErrorHz = freqErrorHz_arr[index];
		memcpy(rxs_s, &rxs_arr[index], sizeof(struct rxstatus_signed));

		if (freqErrorHz >= targetFreqOffset)
		{
			CodeStart = CodeValue;
			CodeValue += (CodeEnd - CodeStart)/2;
			CodeStart = CodeValuebak;

			atbm_printk_wext("freqErrorHz:%d >= targetFreqOffset%d,CodeValue%d CodeEnd[%d]. CodeStart[%d]\n",
				freqErrorHz,targetFreqOffset,	CodeValue,CodeEnd , CodeStart);
			
			atbm_DCXOCodeWrite(hw_priv,CodeValue);
			if (CodeValue >= 0xff)
			{
				break;
			}
		}
		else if ((int)freqErrorHz <= -targetFreqOffset)
		{
			CodeEnd = CodeValue;
			CodeValue -= (CodeEnd - CodeStart)/2;
			CodeEnd = CodeValuebak;
			
			atbm_printk_wext("freqErrorHz:%d <= targetFreqOffset%d,CodeValue%d CodeEnd[%d]. CodeStart[%d]\n",
				freqErrorHz,targetFreqOffset,	CodeValue,CodeEnd , CodeStart);
			
			atbm_DCXOCodeWrite(hw_priv,CodeValue);
			if (CodeValue < 0x01)
			{
				break;
			}
			if (0x01 == CodeEnd)
			{
				break;
			}
		}
		else
		{
			atbm_printk_wext("[PASS]freqErrorKHz[%d] CodeValue[%d]!\n",freqErrorHz/1000,CodeValue);
			b_fail = 0;
			*dcxo = CodeValue;
			*pfreqErrorHz = freqErrorHz;
			break;
		}

		if(CodeValue == CodeValuebak)
		{
			break;
		}

	}


	mutex_lock(&hw_priv->conf_mutex);
	memset(cmd,0,sizeof(cmd));
	memcpy(cmd, "monitor 0", 9);
	//stop DUT Rx
#if 1	
	atbm_printk_wext("stop DUT Rx CMD:%s\n", cmd);
	atbm_for_each_vif(hw_priv,vif,i){
		if (vif != NULL)
		{
			ETF_bStartRx = 0;
			WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_FW_CMD,
				cmd, strlen(cmd) + 10, vif->if_id));
			break;
		}
	}
	mutex_unlock(&hw_priv->conf_mutex);

	//./iwpriv wlan0 fwdbg 1
	ucDbgPrintOpenFlag = 0;
	atbm_for_each_vif(hw_priv,vif,i){
				if (vif != NULL)
				{
					WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_DBG_PRINT_TO_HOST,
						&ucDbgPrintOpenFlag, sizeof(ucDbgPrintOpenFlag), vif->if_id));
					break;
				}
			}

	
#endif
	return b_fail;
}
#endif
#endif

/*
return value:
	0: success.
	1:freq offset not ok yet,need next test
	2:dcxo cal max
	3.dcxo cal min
*/
static int Test_FreqOffset_v2(struct atbm_common *hw_priv, u32 *dcxo, int *pfreqErrorHz)
{
	u8 CodeValue,CodeValuebak;
	int b_fail =1;
	int freqErrorHz = 0;
	int targetFreqOffset = TARGET_FREQOFFSET_HZ;
	static int first_cal = 0;

	if(etf_config.freq_ppm != 0)
		targetFreqOffset = etf_config.freq_ppm;

	if((etf_config.default_dcxo != 0)
		&& (etf_config.default_dcxo >= 0)
		&& (etf_config.default_dcxo <= 127)
		&& (first_cal == 0))
	{
		first_cal = 1;
		atbm_printk_debug("wirte default dcxo when calibration firstly\n");
		CodeValue = etf_config.default_dcxo;
		atbm_DCXOCodeWrite(hw_priv,CodeValue);	
	}
	else
	{
		CodeValue = atbm_DCXOCodeRead(hw_priv);	
		atbm_DCXOCodeWrite(hw_priv,CodeValue);	
	}


	atbm_printk_always("CodeValue default:%d\n",CodeValue);

	
	CodeValuebak = CodeValue;

	freqErrorHz = gRxs_s.Cfo;

	//no freqOffset Calibration,
	if(etf_config.noFfreqCaliFalg == 1)
	{
		atbm_printk_always("targetFreqOffset[%d~%d]Hz\n", -targetFreqOffset,targetFreqOffset);
		if ((freqErrorHz > targetFreqOffset) || (freqErrorHz < -targetFreqOffset))
		{
			atbm_printk_always("fail freqError[%d]Hz dcxo[%d]!\n",freqErrorHz,CodeValue);
			b_fail = 2;
		}
		else
		{
			atbm_printk_always("pass freqError[%d]Hz dcxo[%d]!\n",freqErrorHz,CodeValue);
			b_fail = 0;
		}
		*dcxo = CodeValue;
		return b_fail;
	}

	if (freqErrorHz > targetFreqOffset)
	{
		CodeStart = CodeValue;
		CodeValue += (CodeEnd - CodeStart)/2;
		CodeStart = CodeValuebak;

		atbm_printk_always("freqErrorHz[%d] > targetFreqOffset[%d],CodeValue[%d] ,CodeStart[%d], CodeEnd[%d] . \n",
			freqErrorHz,targetFreqOffset,	CodeValue, CodeStart ,CodeEnd );
		
		atbm_DCXOCodeWrite(hw_priv,CodeValue);

		b_fail = 1;
		if (CodeValue >= 126)
		{
			b_fail = 2;
		}
		if (CodeValue >= 0xff)
		{
			b_fail = 2;
		}
	}
	else if ((int)freqErrorHz < -targetFreqOffset)
	{
		CodeEnd = CodeValue;
		CodeValue -= (CodeEnd - CodeStart)/2;
		CodeEnd = CodeValuebak;

		atbm_printk_always("freqErrorHz[%d] < targetFreqOffset[%d],CodeValue[%d] ,CodeStart[%d], CodeEnd[%d] . \n",
			freqErrorHz,targetFreqOffset,	CodeValue, CodeStart ,CodeEnd );
		atbm_DCXOCodeWrite(hw_priv,CodeValue);

		b_fail = 1;
		
		if (CodeValue <= 2)
		{
			b_fail = 3;
		}
		if (0x01 == CodeEnd)
		{
			b_fail = 3;
		}
	}
	else
	{
		first_cal = 0;
		atbm_printk_always("[dcxo PASS]freqErrorKHz[%d] CodeValue[%d]!\n",freqErrorHz/1000,CodeValue);
		b_fail = 0;
		*dcxo = CodeValue;
		*pfreqErrorHz = freqErrorHz;
	}

	if((CodeEnd == CodeStart) ||
		((CodeEnd - CodeStart) == 1) ||
		((CodeEnd - CodeStart) == -1))
	{
		atbm_printk_always("CodeValue[%d] ,CodeStart[%d], CodeEnd[%d] . \n",CodeValue, CodeStart ,CodeEnd);
		b_fail = 2;
	}

	
	return b_fail;

}


static int atbm_freqoffset_save_efuse(struct atbm_common *hw_priv,struct rxstatus_signed rxs_s,u32 dcxo)
{
	int ret = 0;
	int iResult=0;
	//struct atbm_vif *vif;
	struct efuse_headr efuse_d,efuse_bak;
	
	
	//u8 buff[512];

	memset(&efuse_d,0,sizeof(struct efuse_headr));
	memset(&efuse_bak,0,sizeof(struct efuse_headr));

	

	//tmp = atbm_DCXOCodeRead(hw_priv);printk("tmp %d\n"tmp);	
	if(ucWriteEfuseFlag)
	{
		atbm_printk_always("ucWriteEfuseFlag :%d\n",ucWriteEfuseFlag);
		wsm_get_efuse_data(hw_priv,(void *)&efuse_d,sizeof(struct efuse_headr));

		if(efuse_d.version == 0)
		{
			//The first time efuse is written,all the data should be written, 
			//The production test only modifies part of the value, so efuse cannot be written.
			iResult = -3;
			goto FEEQ_ERR;
		}

		if(efuse_d.dcxo_trim == dcxo) // old dcxo equal new dcxo, no need to write efuse.
		{
			atbm_printk_always(" old dcxo equal new dcxo, no need to write efuse.\n");
			iResult = 0;
			goto FEEQ_ERR;
		}
		efuse_d.dcxo_trim = dcxo;
		//write amc address
		if(etf_config.writemacflag == 1){
			memcpy(efuse_d.mac, etf_config.writemac, 6);
		}
		/*
		*LMC_STATUS_CODE__EFUSE_VERSION_CHANGE	failed because efuse version change  
		*LMC_STATUS_CODE__EFUSE_FIRST_WRITE, 		failed because efuse by first write   
		*LMC_STATUS_CODE__EFUSE_PARSE_FAILED,		failed because efuse data wrong, cannot be parase
		*LMC_STATUS_CODE__EFUSE_FULL,				failed because efuse have be writen full
		*/
		ret = wsm_efuse_change_data_cmd(hw_priv, &efuse_d,0);
		if (ret == LMC_STATUS_CODE__EFUSE_FIRST_WRITE)
		{
			iResult = -3;
		}else if (ret == LMC_STATUS_CODE__EFUSE_PARSE_FAILED)
		{
			iResult = -4;
		}else if (ret == LMC_STATUS_CODE__EFUSE_FULL)
		{
			iResult = -5;
		}else if (ret == LMC_STATUS_CODE__EFUSE_VERSION_CHANGE)
		{
			iResult = -6;
		}else
		{
			iResult = 0;
		}
		frame_hexdump("efuse_d", (u8 *)&efuse_d, sizeof(struct efuse_headr));
		wsm_get_efuse_data(hw_priv,(void *)&efuse_bak, sizeof(struct efuse_headr));
		frame_hexdump("efuse_bak", (u8 *)&efuse_bak, sizeof(struct efuse_headr));
		
		if(memcmp((void *)&efuse_bak,(void *)&efuse_d, sizeof(struct efuse_headr)) !=0)
		{
			iResult = -2;
		}else
		{
			iResult = 0;
		}
		
	}

	
FEEQ_ERR:	
	
	/*sprintf(buff, "cfo:%d,evm:%d,gainImb:%d, phaseImb:%d,dcxo:%d,result:%d (0:OK; -1:FreqOffset Error; -2:efuse hard error;"
		" -3:efuse no written; -4:efuse anaysis failed; -5:efuse full; -6:efuse version change)",
	rxs_s.Cfo,
	rxs_s.evm,
	rxs_s.GainImb,
	rxs_s.PhaseImb,
	dcxo,
	iResult
	);*/

	//if((ret = copy_to_user(wrqu->data.pointer, buff, strlen(buff))) != 0){
	//	return -EINVAL;
	//}

	return iResult;
}


/**************************************************************************
**
** NAME         LMC_FM_GetATBMIe
**
** PARAMETERS:  pElements  -> Pointer to the Ie list
**              Length     -> Size of the Ie List
**              
** RETURNS:     Pointer to element if found or 0 otherwise.
**
** DESCRIPTION  Searches for ATBM test element  from a given IE list.
** 
**************************************************************************/
static u8* LMC_FM_GetATBMIe(u8 *pElements,u16 Length)
{
  u8     ATBMIeOui[3]   = ATBM_OUI	;
  
  struct ATBM_TEST_IE  *Atbm_Ie;
	//dump_mem(pElements,Length);

   if(Length > sizeof(struct ATBM_TEST_IE)){
		pElements += Length-sizeof(struct ATBM_TEST_IE);
		Atbm_Ie =(struct ATBM_TEST_IE  *) pElements;
		/*
		DBG_Printf("Atbm_Ie->oui_type %x,Atbm_Ie->oui %x %x,size %x\n",
			Atbm_Ie->oui_type,
			Atbm_Ie->oui[2],
			ATBMIeOui[2],
			sizeof(struct ATBM_TEST_IE));
		
		dump_mem(pElements,16);*/

		 if(pElements[0]== D11_WIFI_ELT_ID){
			 if((memcmp(Atbm_Ie->oui,ATBMIeOui,3)==0)&&
			 	(Atbm_Ie->oui_type== WIFI_ATBM_IE_OUI_TYPE) ){
				return pElements;
			}
		 }
   }

  return (u8 *)NULL  ;
}//end LMC_FM_GetP2PIe()

static int etf_v2_compare_test_result(void)
{	
	if((etf_config.txpwrmax == 0) && (etf_config.txpwrmin == 0))
	{
		etf_config.txpwrmax = 65536;
		if((efuse_data_etf.specific & 0x1))//outerPA(6038)
		{



			etf_config.txpwrmin = -60+etf_config.cableloss;	
		}
		else
		{
			etf_config.txpwrmin = -84+etf_config.cableloss;	
		}
		atbm_printk_wext("Use default Txrssimin threshold[-84+120]:%d\n", etf_config.txpwrmin);
	}

	if((etf_config.txevmthreshold != 0) && (gRxs_s.txevm > etf_config.txevmthreshold))
	{
		atbm_printk_err("Test txevm:%d > threshold txevm:%d\n", gRxs_s.txevm, etf_config.txevmthreshold);
		return 1;
	}

	if((etf_config.rxevmthreshold != 0) && (gRxs_s.evm > etf_config.rxevmthreshold))
	{
		atbm_printk_err("Test rxevm:%d > threshold rxevm:%d\n", gRxs_s.evm, etf_config.rxevmthreshold);
		return 2;
	}
	if((etf_config.txpwrmax != 0) && (etf_config.txpwrmin != 0) &&
		((gRxs_s.TxRSSI > etf_config.txpwrmax) ||
		(gRxs_s.TxRSSI < etf_config.txpwrmin)))
	{
		atbm_printk_err("Test txpower:%d,txpowermax:%d, txpowermin:%d\n",
			gRxs_s.TxRSSI, etf_config.txpwrmax, etf_config.txpwrmin);
		return 3;
	}
	if((etf_config.rxpwrmax != 0) && (etf_config.rxpwrmin!= 0) &&
		((gRxs_s.RxRSSI > etf_config.rxpwrmax) ||
		(gRxs_s.RxRSSI < etf_config.rxpwrmin)))
	{
		atbm_printk_err("Test rxpower:%d,rxpowermax:%d, rxpowermin:%d\n",
			gRxs_s.RxRSSI, etf_config.rxpwrmax, etf_config.rxpwrmin);
		return 4;
	}

	return 0;
}

void etf_v2_scan_end(struct atbm_common *hw_priv, struct ieee80211_vif *vif )
{
	int ret = 0;
	int result = 0;//(0:OK; -1:FreqOffset Error; -2:Write efuse Failed;-3:efuse not write;-4:rx fail)
	u32 dcxo = 0;
	int freqErrorHz;
	int ErrCode = -1;

	if(hw_priv->etf_test_v2 == 0)
	{
		atbm_test_rx_cnt = 0;
	}
	else
	{
		ret = etf_rx_status_get(hw_priv);
		if(ret != 0)
		{
			atbm_test_rx_cnt = 0;
			atbm_printk_wext("%s:etf_rx_status_get failed,ret:%d\n", __func__, ret);
		}
	}
	msleep(10);

	if(atbm_test_rx_cnt <= 5){
		memset(&gRxs_s, 0, sizeof(struct rxstatus_signed));
		//up(&hw_priv->scan.lock);
		//hw_priv->etf_test_v2 = 0;
		atbm_printk_always("etf rx data[%d] less than 5 packet\n",atbm_test_rx_cnt);
		ErrCode = -7;
		goto Error;
	}
	
	gRxs_s.TxRSSI += etf_config.cableloss;
	gRxs_s.txevm = txevm_total/atbm_test_rx_cnt;
	
	atbm_printk_always("Average: Cfo:%d,TxRSSI:%d,RxRSSI:%d,txevm:%d,rxevm:%d\n",	
	gRxs_s.Cfo,
	gRxs_s.TxRSSI,
	gRxs_s.RxRSSI,
	gRxs_s.txevm,
	gRxs_s.evm
	);
	
#if 0//CONFIG_ATBM_PRODUCT_TEST_NO_UART
	int efuse_remainbit = 0;

	efuse_remainbit = wsm_get_efuse_status(hw_priv, vif);
	printk("efuse remain bit:%d\n", efuse_remainbit);

	if(efuse_remainbit < 8)
	{		
		printk("##efuse is full,do not calibrte FreqOffset\n##");
		dcxo = efuse_data_etf.dcxo_trim;
		if(etf_config.freq_ppm != 0)
		{
			if((gRxs_s.Cfo > -etf_config.freq_ppm) &&
				(gRxs_s.Cfo < etf_config.freq_ppm))
			{
				printk("#1#cur cfo:%d, targetFreqOffset:%d\n",
					gRxs_s.Cfo, etf_config.freq_ppm);
				goto success;
			}
			else
			{
				printk("#1#cur cfo:%d, targetFreqOffset:%d\n",
					gRxs_s.Cfo, etf_config.freq_ppm);
				goto Error;
			}
		}
		else
		{
			if((gRxs_s.Cfo > -TARGET_FREQOFFSET_HZ) &&
				(gRxs_s.Cfo < TARGET_FREQOFFSET_HZ))
			{
				printk("#2#cur cfo:%d, targetFreqOffset:%d\n",
					gRxs_s.Cfo, TARGET_FREQOFFSET_HZ);
				goto success;
			}
			else
			{
				printk("#2#cur cfo:%d, targetFreqOffset:%d\n",
					gRxs_s.Cfo, TARGET_FREQOFFSET_HZ);
				goto Error;
			}
		}
	}
#endif
	if(etf_config.chip_crystal_type != 2)
	{
		if(etf_config.freq_ppm != 0)
			result = Test_FreqOffset_v2(hw_priv,&dcxo,&freqErrorHz);
		else
		{
			dcxo = efuse_data_etf.dcxo_trim;
			atbm_printk_always("Not need to Calibrate FreqOffset\n");
			result = 0;
			goto success;
		}
	}
	else
	{
		dcxo = efuse_data_etf.dcxo_trim;
		atbm_printk_always("Crystal,Not need to Calibrate FreqOffset\n");
		result = 0;
		goto success;
	}
	if(result == 1)
	{
		//start next scan
		atbm_printk_always("start next scan\n");

		//mutex_lock(&hw_priv->conf_mutex);
		//wsm_stop_tx(hw_priv);
		//mutex_unlock(&hw_priv->conf_mutex);

		msleep(100);
		txevm_total = 0;
		atbm_test_rx_cnt = 0;
		wsm_start_tx_v2(hw_priv,vif);
	}
	else  if(result == 0)  //etf dcxo success
	{
success:
		if((ErrCode = etf_v2_compare_test_result()) != 0)
			goto Error;
		atbm_printk_always("etf test success \n");
		gRxs_s.result = atbm_freqoffset_save_efuse(hw_priv,gRxs_s,dcxo);

		gRxs_s.dcxo = dcxo;
		gRxs_s.valid = 1;
		up(&hw_priv->scan.lock);
		hw_priv->etf_test_v2 = 0;
		//del_timer_sync(&hw_priv->etf_expire_timer);
#ifdef CONFIG_ATBM_PRODUCT_TEST_USE_GOLDEN_LED
		Atbm_Test_Success = 1;
		wsm_send_result(hw_priv,vif);
#endif
		
	}else
	{
Error:
		gRxs_s.result = ErrCode;
		gRxs_s.dcxo = dcxo;
		gRxs_s.valid = 1;
		atbm_printk_always("etf test Fail \n");
		up(&hw_priv->scan.lock);
		hw_priv->etf_test_v2 = 0;
		//del_timer_sync(&hw_priv->etf_expire_timer);
#ifdef CONFIG_ATBM_PRODUCT_TEST_USE_GOLDEN_LED
		Atbm_Test_Success = -1;
		wsm_send_result(hw_priv,vif);
#endif

	}
	atbm_test_rx_cnt = 0;
	txevm_total = 0;
	//ETF_bStartTx = 0;
}

void etf_v2_scan_rx(struct atbm_common *hw_priv,struct sk_buff *skb,u8 rssi )
{

	s32 Cfo = 0;
	s32  RSSI = 0;
	s32 tmp = 0;
	s16 txevm  = 0;
	struct ATBM_TEST_IE  *Atbm_Ie = NULL;
	u8 *data = (u8 *)skb->data + offsetof(struct atbm_ieee80211_mgmt, u.probe_resp.variable);
	int len = skb->len - offsetof(struct atbm_ieee80211_mgmt, u.probe_resp.variable);
	Atbm_Ie = (struct ATBM_TEST_IE  *)LMC_FM_GetATBMIe(data,len);
	
	if((Atbm_Ie)
#ifdef ATBM_PRODUCT_TEST_USE_FEATURE_ID
		&& (Atbm_Ie->featureid == etf_config.featureid)
#endif
		)
	{
		tmp				= Atbm_Ie->result[1];
		tmp				= (s32)N_BIT_TO_SIGNED_32BIT(tmp, 16);
		if(Atbm_Ie->resverd & BIT(0))
			Cfo = (s32)(((tmp*12207)/160));//6431 as golden
		else
			Cfo = (s32)(((tmp*12207)/10));	//6421 as golden 
		txevm				= (s16)N_BIT_TO_SIGNED_32BIT(Atbm_Ie->result[2], 16);
		RSSI			= (s16)N_BIT_TO_SIGNED_32BIT(Atbm_Ie->result[3], 10);
		
		if( RSSI < etf_config.rssifilter)
		{
			atbm_printk_always("[%d]: Cfo:%d,TxRSSI:%d,rssifilter[%d] rx dump packet,throw......\n",
			atbm_test_rx_cnt,	
			Cfo,
			RSSI,
			etf_config.rssifilter
			);
			return;
		}

		if(txevm < etf_config.txevm)
		{
			if(atbm_test_rx_cnt == 0)
			{		
				gRxs_s.Cfo = Cfo;
				//gRxs_s.evm = evm;
				gRxs_s.TxRSSI = RSSI;
			}else
			{

				gRxs_s.Cfo = (gRxs_s.Cfo*3 + Cfo )/4;
				//gRxs_s.evm = evm;
				gRxs_s.TxRSSI = RSSI;
				//gRxs_s.TxRSSI = (gRxs_s.TxRSSI*3*10 + RSSI*10 +5)/40;

			}

			atbm_printk_always("[%d]: Cfo1:%d, Cfo:%d,TxRSSI:%d,txevm:%d\n",
			atbm_test_rx_cnt,
			tmp,
			Cfo,
			RSSI,txevm
			);

			//printk("etf_v2_scan_rx %d,cnt %d,[0x%x,0x%x,0x%x,0x%x,0x%x]\n",Atbm_Ie->test_type,atbm_test_rx_cnt,
			//	Atbm_Ie->result[0],Atbm_Ie->result[1],Atbm_Ie->result[2],Atbm_Ie->result[3],Atbm_Ie->result[3]);
			txevm_total += txevm;
			atbm_test_rx_cnt++;
		}
		
	}

}

/*
start return  1
stop return 0
*/
int ETF_Test_is_Start(void){
	if(ETF_bStartTx || ETF_bStartRx)
		return 1;
	return 0;
}

void ETF_Test_Init(void){
	ETF_bStartTx = 0;
	ETF_bStartRx = 0;
}

void etf_param_init(struct atbm_common *hw_priv)
{
	atbm_test_rx_cnt = 0;
	txevm_total = 0;

	ucWriteEfuseFlag = 0;

	CodeStart = DCXO_CODE_MINI;
	CodeEnd = DCXO_CODE_MAX;

	memset(&etf_config, 0, sizeof(struct etf_test_config));
	memset(&gRxs_s, 0, sizeof(struct rxstatus_signed));

	chipversion = atbm_get_chip_version(hw_priv);
	atbm_printk_wext("chipversion:0x%x\n", chipversion);
}


