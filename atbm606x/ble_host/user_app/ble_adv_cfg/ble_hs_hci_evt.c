/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "atbm_os_mem.h"

#include "atbm_debug.h"
#include "os/os.h"
#include "nimble/hci_common.h"
#include "nimble/ble_hci_trans.h"
#include "host/ble_gap.h"
#include "host/ble_monitor.h"
#include "ble_hs_priv.h"
#include "ble_hs_dbg_priv.h"

_Static_assert(sizeof (struct hci_data_hdr) == BLE_HCI_DATA_HDR_SZ,
               "struct hci_data_hdr must be 4 bytes");

typedef int ble_hs_hci_evt_fn(uint8_t event_code, uint8_t *data, int len);
static ble_hs_hci_evt_fn ble_hs_hci_evt_le_meta;

typedef int ble_hs_hci_evt_le_fn(uint8_t subevent, uint8_t *data, int len);
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_adv_rpt;
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_dir_adv_rpt;
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_ext_adv_rpt;
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_scan_timeout;
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_adv_set_terminated;

/* Statistics */
struct host_hci_stats
{
    uint32_t events_rxd;
    uint32_t good_acks_rxd;
    uint32_t bad_acks_rxd;
    uint32_t unknown_events_rxd;
};

#define BLE_HS_HCI_EVT_TIMEOUT        50      /* Milliseconds. */

/** Dispatch table for incoming HCI events.  Sorted by event code field. */
struct ble_hs_hci_evt_dispatch_entry {
    uint8_t event_code;
    ble_hs_hci_evt_fn *cb;
};

static const struct ble_hs_hci_evt_dispatch_entry ble_hs_hci_evt_dispatch[] = {
   // { BLE_HCI_EVCODE_DISCONN_CMP, ble_hs_hci_evt_disconn_complete },
   // { BLE_HCI_EVCODE_ENCRYPT_CHG, ble_hs_hci_evt_encrypt_change },
 //   { BLE_HCI_EVCODE_HW_ERROR, ble_hs_hci_evt_hw_error },
   // { BLE_HCI_EVCODE_NUM_COMP_PKTS, ble_hs_hci_evt_num_completed_pkts },
  //  { BLE_HCI_EVCODE_ENC_KEY_REFRESH, ble_hs_hci_evt_enc_key_refresh },
    { BLE_HCI_EVCODE_LE_META, ble_hs_hci_evt_le_meta },
  //  { BLE_HCI_EVCODE_VENDOR_DEBUG, ble_hs_hci_evt_vendor },
};

#define BLE_HS_HCI_EVT_DISPATCH_SZ \
    (sizeof ble_hs_hci_evt_dispatch / sizeof ble_hs_hci_evt_dispatch[0])

/** Dispatch table for incoming LE meta events.  Sorted by subevent field. */
struct ble_hs_hci_evt_le_dispatch_entry {
    uint8_t subevent;
    ble_hs_hci_evt_le_fn *cb;
};

static const struct ble_hs_hci_evt_le_dispatch_entry
        ble_hs_hci_evt_le_dispatch[] = {
    //{ BLE_HCI_LE_SUBEV_CONN_COMPLETE, ble_hs_hci_evt_le_conn_complete },
    { BLE_HCI_LE_SUBEV_ADV_RPT, ble_hs_hci_evt_le_adv_rpt },
//    { BLE_HCI_LE_SUBEV_CONN_UPD_COMPLETE,    ble_hs_hci_evt_le_conn_upd_complete },
    //{ BLE_HCI_LE_SUBEV_LT_KEY_REQ, ble_hs_hci_evt_le_lt_key_req },
    //{ BLE_HCI_LE_SUBEV_REM_CONN_PARM_REQ, ble_hs_hci_evt_le_conn_parm_req },
   // { BLE_HCI_LE_SUBEV_ENH_CONN_COMPLETE, ble_hs_hci_evt_le_conn_complete },
    { BLE_HCI_LE_SUBEV_DIRECT_ADV_RPT, ble_hs_hci_evt_le_dir_adv_rpt },
   // { BLE_HCI_LE_SUBEV_PHY_UPDATE_COMPLETE,  ble_hs_hci_evt_le_phy_update_complete },
    //{ BLE_HCI_LE_SUBEV_EXT_ADV_RPT, ble_hs_hci_evt_le_ext_adv_rpt },
    //{ BLE_HCI_LE_SUBEV_RD_REM_USED_FEAT,  ble_hs_hci_evt_le_rd_rem_used_feat_complete },
    { BLE_HCI_LE_SUBEV_SCAN_TIMEOUT,   ble_hs_hci_evt_le_scan_timeout },
   // { BLE_HCI_LE_SUBEV_ADV_SET_TERMINATED,ble_hs_hci_evt_le_adv_set_terminated },
};

#define BLE_HS_HCI_EVT_LE_DISPATCH_SZ \
    (sizeof ble_hs_hci_evt_le_dispatch / sizeof ble_hs_hci_evt_le_dispatch[0])

static const struct ble_hs_hci_evt_dispatch_entry *
ble_hs_hci_evt_dispatch_find(uint8_t event_code)
{
    const struct ble_hs_hci_evt_dispatch_entry *entry;
    int i;

    for (i = 0; i < BLE_HS_HCI_EVT_DISPATCH_SZ; i++) {
        entry = ble_hs_hci_evt_dispatch + i;
        if (entry->event_code == event_code) {
            return entry;
        }
    }

    return NULL;
}

static const struct ble_hs_hci_evt_le_dispatch_entry *
ble_hs_hci_evt_le_dispatch_find(uint8_t event_code)
{
    const struct ble_hs_hci_evt_le_dispatch_entry *entry;
    int i;

    for (i = 0; i < BLE_HS_HCI_EVT_LE_DISPATCH_SZ; i++) {
        entry = ble_hs_hci_evt_le_dispatch + i;
        if (entry->subevent == event_code) {
            return entry;
        }
    }

    return NULL;
}

static int
ble_hs_hci_evt_le_meta(uint8_t event_code, uint8_t *data, int len)
{
    const struct ble_hs_hci_evt_le_dispatch_entry *entry;
    uint8_t subevent;
    int rc;

    if (len < BLE_HCI_EVENT_HDR_LEN + BLE_HCI_LE_MIN_LEN) {
        return BLE_HS_ECONTROLLER;
    }

    subevent = data[2];
    entry = ble_hs_hci_evt_le_dispatch_find(subevent);
    if (entry != NULL) {
        rc = entry->cb(subevent, data + BLE_HCI_EVENT_HDR_LEN,
                           len - BLE_HCI_EVENT_HDR_LEN);
        if (rc != 0) {
            return rc;
        }
    }

    return 0;
}


static int
ble_hs_hci_evt_le_adv_rpt_first_pass(uint8_t *data, int len)
{
    uint8_t num_reports;
    int off;
    int i;

    if (len < BLE_HCI_LE_ADV_RPT_MIN_LEN) {
        return BLE_HS_ECONTROLLER;
    }

    num_reports = data[1];
    if (num_reports < BLE_HCI_LE_ADV_RPT_NUM_RPTS_MIN ||
        num_reports > BLE_HCI_LE_ADV_RPT_NUM_RPTS_MAX) {
        return BLE_HS_EBADDATA;
    }

    off = 2; /* Subevent code and num reports. */
    for (i = 0; i < num_reports; i++) {
        /* Move past event type (1), address type (1) and address (6) */
        off += 8;

        /* Add advertising data length (N), length (1) and rssi (1) */
        off += data[off];
        off += 2;

        /* Make sure we are not past length */
        if (off > len) {
            return BLE_HS_ECONTROLLER;
        }
    }

    /* Make sure length was correct */
    if (off != len) {
        return BLE_HS_ECONTROLLER;
    }

    return 0;
}

static int
ble_hs_hci_evt_le_adv_rpt(uint8_t subevent, uint8_t *data, int len)
{
    struct ble_gap_disc_desc desc = {0};
    uint8_t num_reports;
    int off;
    int rc;
    int i;

    /* Validate the event is formatted correctly */
    rc = ble_hs_hci_evt_le_adv_rpt_first_pass(data, len);
    if (rc != 0) {
        return rc;
    }

    desc.direct_addr = *BLE_ADDR_ANY;

    off = 2; /* skip sub-event and num reports */
    num_reports = data[1];
    for (i = 0; i < num_reports; i++) {
        desc.event_type = data[off];
        ++off;

        desc.addr.type = data[off];
        ++off;

        memcpy(desc.addr.val, data + off, 6);
        off += 6;

        desc.length_data = data[off];
        ++off;

        desc.data = data + off;
        off += desc.length_data;

        desc.rssi = data[off];
        ++off;

        ble_gap_rx_adv_report(&desc);
    }

    return 0;
}

static int
ble_hs_hci_evt_le_dir_adv_rpt(uint8_t subevent, uint8_t *data, int len)
{
    struct ble_gap_disc_desc desc = {0};
    uint8_t num_reports;
    int suboff;
    int off;
    int i;

    if (len < BLE_HCI_LE_ADV_DIRECT_RPT_LEN) {
        return BLE_HS_ECONTROLLER;
    }

    num_reports = data[1];
    if (len != 2 + num_reports * BLE_HCI_LE_ADV_DIRECT_RPT_SUB_LEN) {
        return BLE_HS_ECONTROLLER;
    }

    /* Data fields not present in a direct advertising report. */
    desc.data = NULL;
    desc.length_data = 0;

    for (i = 0; i < num_reports; i++) {
        suboff = 0;

        off = 2 + suboff * num_reports + i;
        desc.event_type = data[off];
        suboff++;

        off = 2 + suboff * num_reports + i;
        desc.addr.type = data[off];
        suboff++;

        off = 2 + suboff * num_reports + i * 6;
        memcpy(desc.addr.val, data + off, 6);
        suboff += 6;

        off = 2 + suboff * num_reports + i;
        desc.direct_addr.type = data[off];
        suboff++;

        off = 2 + suboff * num_reports + i * 6;
        memcpy(desc.direct_addr.val, data + off, 6);
        suboff += 6;

        off = 2 + suboff * num_reports + i;
        desc.rssi = data[off];
        suboff++;

        ble_gap_rx_adv_report(&desc);
    }

    return 0;
}




static int
ble_hs_hci_evt_le_scan_timeout(uint8_t subevent, uint8_t *data, int len)
{
	iot_printf("scan_timeout\n");
        return 0;
}


int
ble_hs_hci_evt_process(uint8_t *data)
{
    const struct ble_hs_hci_evt_dispatch_entry *entry;
    uint8_t event_code;
    uint8_t param_len;
    int event_len;
    int rc;

    /* Count events received */
    STATS_INC(ble_hs_stats, hci_event);

    /* Display to console */
    //ble_hs_dbg_event_disp(data);

    /* Process the event */
    event_code = data[0];
    param_len = data[1];

    event_len = param_len + 2;

    entry = ble_hs_hci_evt_dispatch_find(event_code);
    if (entry == NULL) {
        STATS_INC(ble_hs_stats, hci_unknown_event);
        rc = BLE_HS_ENOTSUP;
    } else {
        rc = entry->cb(event_code, data, event_len);
    }

    ble_hci_trans_buf_free(data);

    return rc;
}


