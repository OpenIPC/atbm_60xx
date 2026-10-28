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

#include "atbm_hal.h"
#include "nimble/hci_common.h"
#include "host/ble_hs_hci.h"
#include "ble_hs_priv.h"

uint16_t
ble_hs_hci_util_handle_pb_bc_join(uint16_t handle, uint8_t pb, uint8_t bc)
{
    BLE_HS_DBG_ASSERT(handle <= 0x0fff);
    BLE_HS_DBG_ASSERT(pb <= 0x03);
    BLE_HS_DBG_ASSERT(bc <= 0x03);

    return (handle  << 0)   |
           (pb      << 12)  |
           (bc      << 14);
}

int
ble_hs_hci_util_read_adv_tx_pwr(int8_t *out_tx_pwr)
{
    uint8_t params_len;
    int rc;

    rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                      BLE_HCI_OCF_LE_RD_ADV_CHAN_TXPWR),
                           NULL, 0,out_tx_pwr, 1, &params_len);
    if (rc != 0) {
        return rc;
    }

    if (params_len != 1                     ||
        *out_tx_pwr < BLE_HCI_ADV_CHAN_TXPWR_MIN ||
        *out_tx_pwr > BLE_HCI_ADV_CHAN_TXPWR_MAX) {
        BLE_HS_LOG(WARN, "advertiser txpwr out of range\n");
    }

    return 0;
}

int
ble_hs_hci_util_rand(void *dst, int len)
{
    uint8_t rsp_buf[BLE_HCI_LE_RAND_LEN];
    uint8_t params_len;
    uint8_t *u8ptr;
    int chunk_sz;
    int rc;

    u8ptr = dst;
    while (len > 0) {
        rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_RAND),
                               NULL, 0, rsp_buf, sizeof rsp_buf, &params_len);
        if (rc != 0) {
            return rc;
        }
        if (params_len != sizeof rsp_buf) {
            return BLE_HS_ECONTROLLER;
        }

        chunk_sz = atbm_min(len, sizeof rsp_buf);
        memcpy(u8ptr, rsp_buf, chunk_sz);

        len -= chunk_sz;
        u8ptr += chunk_sz;
    }

    return 0;
}

#if 0
int
ble_hs_hci_util_set_random_addr(const uint8_t *addr)
{
    uint8_t buf[BLE_HCI_SET_RAND_ADDR_LEN];
    int rc;

    /* set the address in the controller */

    rc = ble_hs_hci_cmd_build_set_random_addr(addr, buf, sizeof(buf));
    if (rc != 0) {
        return rc;
    }

    rc = ble_hs_hci_cmd_tx_empty_ack(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                                BLE_HCI_OCF_LE_SET_RAND_ADDR),
                                     buf, BLE_HCI_SET_RAND_ADDR_LEN);
    if (rc != 0) {
        return rc;
    }

    return 0;
}
#endif

