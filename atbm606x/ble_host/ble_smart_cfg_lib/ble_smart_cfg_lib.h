
#ifndef _BLE_SMART_CFG_LIB_H
#define _BLE_SMART_CFG_LIB_H

#include <stdint.h>

#define BLE_SMART_CFG_TIMEOUT_MS				(600*1000)
#define BLE_CONN_NAME							"AltoBeam"

#define BLE_SMART_CFG_STATUS_SUCESS				0x0
#define BLE_SMART_CFG_SSID_PWD_TRANS_END		0xFE
#define BLE_SMART_CFG_STATUS_END				0xFF
#define BLE_SMART_CFG_STATUS_BUSY				0xF0
#define BLE_SMART_CFG_STATUS_TIMEOUT			0xF1
#define BLE_SMART_CFG_STATUS_SSID_LEN_ERR		0xF2
#define BLE_SMART_CFG_STATUS_SSID_ERR			0xF3
#define BLE_SMART_CFG_STATUS_PWD_LEN_ERR		0xF4
#define BLE_SMART_CFG_STATUS_PWD_ERR			0xF5
#define BLE_SMART_CFG_STATUS_STOP				0xF6

#define ATBM_BLE_SMART_APP 1
extern uint8_t atbm_ble_status;
extern uint16_t ble_conn_handle;
extern uint16_t ble_svc_gatt_changed_val_handle;

extern int gatt_svr_chr_indicate(uint16_t conn_handle, uint16_t chr_val_handle, const char *data, int len);
enum atbm_ble_status_flag {
	ATBM_BLE_STATUS_IDLE=0,
	ATBM_BLE_STATUS_ADVERTISE=1,
	ATBM_BLE_STATUS_SCANING=2,
	ATBM_BLE_STATUS_CONNECT=3,
};


typedef struct ble_smart_cfg_t
{
	uint8_t ssid[32];
	uint8_t pwd[64];
	uint8_t ssid_len;
	uint8_t pwd_len;
	uint8_t ssid_cnt;
	uint8_t pwd_cnt;
	uint8_t step;
}ble_smart_cfg;

struct wsm_ble_smt_ind {
	uint32_t status;
	uint8_t ssid[32];
	uint8_t pwd[64];
	uint8_t ssid_len;
	uint8_t pwd_len;
};



void ble_smart_cfg_startup(void);
void ble_smart_cfg_stop(void);
void ble_smart_gatt_svcs_init(void);
void atbm_print_addr(const void *addr);
void atbm_print_bytes(const uint8_t *bytes, int len);

//void atbm_print_conn_desc(const struct ble_gap_conn_desc *desc);

extern  int ble_write_no_rsp(uint16_t conn_handle, uint16_t attr_handle,const char *data, int len);

#endif

