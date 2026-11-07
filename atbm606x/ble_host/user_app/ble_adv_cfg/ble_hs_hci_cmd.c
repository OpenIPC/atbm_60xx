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

#include <assert.h>
#include "atbm_os_mem.h"

#include "atbm_debug.h"
#include "os/os.h"
#include "nimble/hci_common.h"
#include "nimble/ble_hci_trans.h"
#include "host/ble_monitor.h"
#include "ble_hs_dbg_priv.h"
#include "ble_hs_priv.h"
#include "ble_monitor_priv.h"

static int
ble_hs_hci_cmd_transport(uint8_t *cmdbuf)
{
    int rc;

#if BLE_MONITOR
    ble_monitor_send(BLE_MONITOR_OPCODE_COMMAND_PKT, cmdbuf,
                     cmdbuf[2] + BLE_HCI_CMD_HDR_LEN);
#endif

    rc = ble_hci_trans_hs_cmd_tx(cmdbuf);
    switch (rc) {
    case 0:
        return 0;

    case BLE_ERR_MEM_CAPACITY:
        return BLE_HS_ENOMEM_EVT;

    default:
        return BLE_HS_EUNKNOWN;
    }
}

void
ble_hs_hci_cmd_write_hdr(uint8_t ogf, uint16_t ocf, uint8_t len, void *buf)
{
    uint16_t opcode;
    uint8_t *u8ptr;

    u8ptr = buf;

    opcode = (ogf << 10) | ocf;
    put_le16(u8ptr, opcode);
    u8ptr[2] = len;
}

static int
ble_hs_hci_cmd_send(uint16_t opcode, uint8_t len, const void *cmddata)
{
    uint8_t *buf;
    int rc;
    buf = ble_hci_trans_buf_alloc(BLE_HCI_TRANS_BUF_CMD);
    BLE_HS_DBG_ASSERT(buf != NULL);

    put_le16(buf, opcode);
    buf[2] = len;
    if (len != 0) {
        memcpy(buf + BLE_HCI_CMD_HDR_LEN, cmddata, len);
    }

#if 0//!BLE_MONITOR
    BLE_HS_LOG(DEBUG, "ble_hs_hci_cmd_send: ogf=0x%02x ocf=0x%04x len=%d\n",
               BLE_HCI_OGF(opcode), BLE_HCI_OCF(opcode), len);
    ble_hs_log_flat_buf(buf, len + BLE_HCI_CMD_HDR_LEN);
    BLE_HS_LOG(DEBUG, "\n");
#endif

    rc = ble_hs_hci_cmd_transport(buf);

    if (rc == 0) {
        STATS_INC(ble_hs_stats, hci_cmd);
    } else {
        BLE_HS_LOG(DEBUG, "ble_hs_hci_cmd_send failure; rc=%d\n", rc);
    }

    return rc;
}

int
ble_hs_hci_cmd_send_buf(uint16_t opcode, void *buf, uint8_t buf_len)
{

    return ble_hs_hci_cmd_send(opcode, buf_len, buf);
}


/**
 * Send a LE command from the host to the controller.
 *
 * @param ocf
 * @param len
 * @param cmddata
 *
 * @return int
 */
static int
ble_hs_hci_cmd_le_send(uint16_t ocf, uint8_t len, void *cmddata)
{
    return ble_hs_hci_cmd_send(BLE_HCI_OP(BLE_HCI_OGF_LE, ocf), len, cmddata);
}



static int
ble_hs_hci_cmd_body_le_set_adv_params(const struct hci_adv_params *adv,
                                      uint8_t *dst)
{
    uint16_t itvl;

    BLE_HS_DBG_ASSERT(adv != NULL);

    /* Make sure parameters are valid */
    if ((adv->adv_itvl_min > adv->adv_itvl_max) ||
        (adv->own_addr_type > BLE_HCI_ADV_OWN_ADDR_MAX) ||
        (adv->peer_addr_type > BLE_HCI_ADV_PEER_ADDR_MAX) ||
        (adv->adv_filter_policy > BLE_HCI_ADV_FILT_MAX) ||
        (adv->adv_type > BLE_HCI_ADV_TYPE_MAX) ||
        (adv->adv_channel_map == 0) ||
        ((adv->adv_channel_map & 0xF8) != 0)) {
        /* These parameters are not valid */
        return -1;
    }

/* When build with nimBLE controller we know it is BT5 compliant so no need
 * to limit non-connectable advertising interval
 */
#if MYNEWT_VAL(BLE_DEVICE)
    itvl = BLE_HCI_ADV_ITVL_MIN;
#else
    /* Make sure interval is valid for advertising type. */
        itvl = BLE_HCI_ADV_ITVL_NONCONN_MIN;
#endif

    /* Do not check if high duty-cycle directed */
    if (adv->adv_type != BLE_HCI_ADV_TYPE_ADV_DIRECT_IND_HD) {
        if ((adv->adv_itvl_min < itvl) ||
            (adv->adv_itvl_min > BLE_HCI_ADV_ITVL_MAX)) {
            return -1;
        }
    }

    put_le16(dst, adv->adv_itvl_min);
    put_le16(dst + 2, adv->adv_itvl_max);
    dst[4] = adv->adv_type;
    dst[5] = adv->own_addr_type;
    dst[6] = adv->peer_addr_type;
    memcpy(dst + 7, adv->peer_addr, BLE_DEV_ADDR_LEN);
    dst[13] = adv->adv_channel_map;
    dst[14] = adv->adv_filter_policy;

    return 0;
}

int
ble_hs_hci_cmd_build_le_set_adv_params(const struct hci_adv_params *adv,
                                       uint8_t *dst, int dst_len)
{
    BLE_HS_DBG_ASSERT(dst_len >= BLE_HCI_SET_ADV_PARAM_LEN);

    return ble_hs_hci_cmd_body_le_set_adv_params(adv, dst);
}

/**
 * Set advertising data
 *
 * OGF = 0x08 (LE)
 * OCF = 0x0008
 *
 * @param data
 * @param len
 * @param dst
 *
 * @return int
 */
static int
ble_hs_hci_cmd_body_le_set_adv_data(const uint8_t *data, uint8_t len,
                                    uint8_t *dst)
{
    /* Check for valid parameters */
    if (((data == NULL) && (len != 0)) || (len > BLE_HCI_MAX_ADV_DATA_LEN)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    memset(dst, 0, BLE_HCI_SET_ADV_DATA_LEN);
    dst[0] = len;
    memcpy(dst + 1, data, len);

    return 0;
}

/**
 * Set advertising data
 *
 * OGF = 0x08 (LE)
 * OCF = 0x0008
 *
 * @param data
 * @param len
 * @param dst
 *
 * @return int
 */
int
ble_hs_hci_cmd_build_le_set_adv_data(const uint8_t *data, uint8_t len,
                                     uint8_t *dst, int dst_len)
{
    BLE_HS_DBG_ASSERT(dst_len >= BLE_HCI_SET_ADV_DATA_LEN);

    return ble_hs_hci_cmd_body_le_set_adv_data(data, len, dst);
}
#if 0
static int
ble_hs_hci_cmd_body_le_set_scan_rsp_data(const uint8_t *data, uint8_t len,
                                         uint8_t *dst)
{
    /* Check for valid parameters */
    if (((data == NULL) && (len != 0)) ||
         (len > BLE_HCI_MAX_SCAN_RSP_DATA_LEN)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    memset(dst, 0, BLE_HCI_SET_SCAN_RSP_DATA_LEN);
    dst[0] = len;
    memcpy(dst + 1, data, len);

    return 0;
}

int
ble_hs_hci_cmd_build_le_set_scan_rsp_data(const uint8_t *data, uint8_t len,
                                          uint8_t *dst, int dst_len)
{
    BLE_HS_DBG_ASSERT(dst_len >= BLE_HCI_SET_SCAN_RSP_DATA_LEN);

    return ble_hs_hci_cmd_body_le_set_scan_rsp_data(data, len, dst);
}


#endif //0

static void
ble_hs_hci_cmd_body_le_set_adv_enable(uint8_t enable, uint8_t *dst)
{
    dst[0] = enable;
}

void
ble_hs_hci_cmd_build_le_set_adv_enable(uint8_t enable, uint8_t *dst,
                                       int dst_len)
{
    BLE_HS_DBG_ASSERT(dst_len >= BLE_HCI_SET_ADV_ENABLE_LEN);

    ble_hs_hci_cmd_body_le_set_adv_enable(enable, dst);
}

static int
ble_hs_hci_cmd_body_le_set_scan_params(
    uint8_t scan_type, uint16_t scan_itvl, uint16_t scan_window,
    uint8_t own_addr_type, uint8_t filter_policy, uint8_t *dst) {

    /* Make sure parameters are valid */
    if ((scan_type != BLE_HCI_SCAN_TYPE_PASSIVE) &&
        (scan_type != BLE_HCI_SCAN_TYPE_ACTIVE)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    /* Check interval and window */
    if ((scan_itvl < BLE_HCI_SCAN_ITVL_MIN) ||
        (scan_itvl > BLE_HCI_SCAN_ITVL_MAX) ||
        (scan_window < BLE_HCI_SCAN_WINDOW_MIN) ||
        (scan_window > BLE_HCI_SCAN_WINDOW_MAX) ||
        (scan_itvl < scan_window)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    /* Check own addr type */
    if (own_addr_type > BLE_HCI_ADV_OWN_ADDR_MAX) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    /* Check scanner filter policy */
    if (filter_policy > BLE_HCI_SCAN_FILT_MAX) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    dst[0] = scan_type;
    put_le16(dst + 1, scan_itvl);
    put_le16(dst + 3, scan_window);
    dst[5] = own_addr_type;
    dst[6] = filter_policy;

    return 0;
}

int
ble_hs_hci_cmd_build_le_set_scan_params(uint8_t scan_type,
                                        uint16_t scan_itvl,
                                        uint16_t scan_window,
                                        uint8_t own_addr_type,
                                        uint8_t filter_policy,
                                        uint8_t *dst, int dst_len)
{
    BLE_HS_DBG_ASSERT(dst_len >= BLE_HCI_SET_SCAN_PARAM_LEN);

    return ble_hs_hci_cmd_body_le_set_scan_params(scan_type, scan_itvl,
                                              scan_window, own_addr_type,
                                              filter_policy, dst);
}

static void
ble_hs_hci_cmd_body_le_set_scan_enable(uint8_t enable, uint8_t filter_dups,
                                       uint8_t *dst)
{
    dst[0] = enable;
    dst[1] = filter_dups;
}

void
ble_hs_hci_cmd_build_le_set_scan_enable(uint8_t enable, uint8_t filter_dups,
                                        uint8_t *dst, uint8_t dst_len)
{
    BLE_HS_DBG_ASSERT(dst_len >= BLE_HCI_SET_SCAN_ENABLE_LEN);

    ble_hs_hci_cmd_body_le_set_scan_enable(enable, filter_dups, dst);
}

/**
 * Reset the controller and link manager.
 *
 * @return int
 */
int
ble_hs_hci_cmd_reset(void)
{
    return ble_hs_hci_cmd_send(BLE_HCI_OP(BLE_HCI_OGF_CTLR_BASEBAND,
                                          BLE_HCI_OCF_CB_RESET),
                               0, NULL);
}


/**
 * Read the transmit power level used for LE advertising channel packets.
 *
 * @return int
 */
int
ble_hs_hci_cmd_read_adv_pwr(void)
{
    return ble_hs_hci_cmd_send(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                          BLE_HCI_OCF_LE_RD_ADV_CHAN_TXPWR),
                               0, NULL);
}


/**
 * Read the RSSI for a given connection handle
 *
 * NOTE: OGF=0x05 OCF=0x0005
 *
 * @param handle
 *
 * @return int







 * OGF=0x08 OCF=0x0031






 * OGF=0x08 OCF=0x0032
 */
