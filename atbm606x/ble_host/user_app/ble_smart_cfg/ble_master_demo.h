#ifndef _BLE_MASTER_DEMO_H
#define _BLE_MASTER_DEMO_H
#include "nimble/ble.h"
#include "nimble/nimble_opt.h"
#include "modlog/modlog.h"

#include "host/ble_gatt.h"
#include "host/ble_gap.h"
#include "host/ble_uuid.h"

#if 0
struct ble_gap_white_entry;
struct ble_hs_adv_fields;
struct ble_gap_upd_params;
struct ble_gap_conn_params;
struct hci_adv_params;
struct ble_l2cap_sig_update_req;
struct ble_l2cap_sig_update_params;
union ble_store_value;
union ble_store_key;
struct ble_gap_adv_params;
struct ble_gap_conn_desc;
struct ble_gap_disc_params;
#endif

extern void ble_master_connect(char *name);
extern int ble_master_connect_addr(uint8_t *addr,uint8_t addr_type);


struct ble_gatt_characteristic {
    uint16_t def_handle;
    uint16_t val_handle;
    uint8_t properties;
    ble_uuid_any_t uuid;
    uint16_t dsc_handle;
};
struct ble_gatt_svc_array {
    uint16_t start_handle;
    uint16_t end_handle;
    ble_uuid_any_t uuid;
   struct ble_gatt_characteristic chr[30];
   uint8_t chr_numble;
};
struct ble_host_dis_gap_gatt{
	uint8_t svc_numble;
	struct ble_gatt_svc_array svc[15];
};

#endif
