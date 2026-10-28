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

#include "syscfg/syscfg.h"

#include <assert.h>
#include "atbm_hal.h"

#include "nimble/nimble_opt.h"
#include "host/ble_hs_adv.h"
#include "host/ble_hs_hci.h"
#include "ble_hs_priv.h"

#define bssnz_t
#undef BLE_ADV_INSTANCES
#define BLE_ADV_INSTANCES 1
/**
 * GAP - Generic Access Profile.
 *
 * Design overview:
 *
 * GAP procedures are initiated by the application via function calls.  Such
 * functions return when either of the following happens:
 *
 * (1) The procedure completes (success or failure).
 * (2) The procedure cannot proceed until a BLE peer responds.
 *
 * For (1), the result of the procedure if fully indicated by the function
 * return code.
 * For (2), the procedure result is indicated by an application-configured
 * callback.  The callback is executed when the procedure completes.
 *
 * The GAP is always in one of two states:
 * 1. Free
 * 2. Preempted
 *
 * While GAP is in the free state, new procedures can be started at will.
 * While GAP is in the preempted state, no new procedures are allowed.  The
 * host sets GAP to the preempted state when it needs to ensure no ongoing
 * procedures, a condition required for some HCI commands to succeed.  The host
 * must take care to take GAP out of the preempted state as soon as possible.
 *
 * Notes on thread-safety:
 * 1. The ble_hs mutex must always be unlocked when an application callback is
 *    executed.  The purpose of this requirement is to allow callbacks to
 *    initiate additional host procedures, which may require locking of the
 *    mutex.
 * 2. Functions called directly by the application never call callbacks.
 *    Generally, these functions lock the ble_hs mutex at the start, and only
 *    unlock it at return.
 * 3. Functions which do call callbacks (receive handlers and timer
 *    expirations) generally only lock the mutex long enough to modify
 *    affected state and make copies of data needed for the callback.  A copy
 *    of various pieces of data is called a "snapshot" (struct
 *    ble_gap_snapshot).  The sole purpose of snapshots is to allow callbacks
 *    to be executed after unlocking the mutex.
 */

/** GAP procedure op codes. */
#define BLE_GAP_OP_NULL                         0
#define BLE_GAP_OP_M_DISC                       1
#define BLE_GAP_OP_M_CONN                       2
#define BLE_GAP_OP_S_ADV                        1
struct find_field_data {
    uint8_t type;
    const struct ble_hs_adv_field *field;
};

/**
 * If an attempt to cancel an active procedure fails, the attempt is retried
 * at this rate (ms).
 */
#define BLE_GAP_CANCEL_RETRY_TIMEOUT_MS         100 /* ms */

#define BLE_GAP_UPDATE_TIMEOUT_MS               30000 /* ms */


/**
 * The state of the in-progress master connection.  If no master connection is
 * currently in progress, then the op field is set to BLE_GAP_OP_NULL.
 */
struct ble_gap_master_state {
    uint8_t op;

    uint8_t exp_set:1;
    ble_npl_time_t exp_os_ticks;

    ble_gap_event_fn *cb;
    void *cb_arg;

};
static bssnz_t struct ble_gap_master_state ble_gap_master;


/**
 * The state of the in-progress slave connection.  If no slave connection is
 * currently in progress, then the op field is set to BLE_GAP_OP_NULL.
 */
struct ble_gap_slave_state {
    uint8_t op;

    unsigned int our_addr_type:2;
    unsigned int preempted:1;  /** Set to 1 if advertising was preempted. */
    unsigned int connectable:1;


/* timer is used only with legacy advertising */
    unsigned int exp_set:1;
    ble_npl_time_t exp_os_ticks;

    ble_gap_event_fn *cb;
    void *cb_arg;
};

static bssnz_t struct ble_gap_slave_state ble_gap_slave;




static int ble_gap_adv_enable_tx(int enable);

#if NIMBLE_BLE_SCAN && !MYNEWT_VAL(BLE_EXT_ADV)
static int ble_gap_disc_enable_tx(int enable, int filter_duplicates);
#endif


/*****************************************************************************
 * $log                                                                      *
 *****************************************************************************/

SRAM_CODE int ble_gap_set_default_tx_power(int8_t dbm)
{
	uint8_t buf[1];

	if((dbm < 0) || (dbm > 15)){
		return BLE_ERR_INV_HCI_CMD_PARMS;
	}

	iot_printf("\n[ATBM lOG]set default tx power(%d)\n", dbm);
	buf[0] = dbm;
    return ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_VENDOR, BLE_HCI_VS_SET_DEFAULT_PWR),
        						buf, sizeof(buf), NULL, 0, NULL);
}


/*****************************************************************************
 * $misc                                                                     *
 *****************************************************************************/

SRAM_CODE static int
ble_gap_event_listener_call(struct ble_gap_event *event);

SRAM_CODE static int
ble_gap_call_event_cb(struct ble_gap_event *event,
                      ble_gap_event_fn *cb, void *cb_arg)
{
    int rc;


    if (cb != NULL) {
        rc = cb(event, cb_arg);
    } else {
        rc = 0;
    }

    return rc;
}



SRAM_CODE static void
ble_gap_master_reset_state(void)
{
    ble_gap_master.op = BLE_GAP_OP_NULL;
    ble_gap_master.exp_set = 0;
   // ble_gap_master.conn.cancel = 0;

    ble_hs_timer_resched();
}

SRAM_CODE static void
ble_gap_slave_reset_state(uint8_t instance)
{
    ble_gap_slave.op = BLE_GAP_OP_NULL;

#if !MYNEWT_VAL(BLE_EXT_ADV)
    ble_gap_slave.exp_set = 0;
    ble_hs_timer_resched();
#endif
}


SRAM_CODE static void
ble_gap_master_extract_state(struct ble_gap_master_state *out_state,
                             int reset_state)
{
    ble_hs_lock();

    *out_state = ble_gap_master;

    if (reset_state) {
        ble_gap_master_reset_state();
    }

    ble_hs_unlock();
}

SRAM_CODE static void
ble_gap_slave_extract_cb(uint8_t instance,
                         ble_gap_event_fn **out_cb, void **out_cb_arg)
{
    ble_hs_lock();

    *out_cb = ble_gap_slave.cb;
    *out_cb_arg = ble_gap_slave.cb_arg;
    ble_gap_slave_reset_state(instance);

    ble_hs_unlock();
}

SRAM_CODE void
ble_gap_adv_finished(uint8_t instance, int reason, uint16_t conn_handle,
                     uint8_t num_events)
{
    struct ble_gap_event event;
    ble_gap_event_fn *cb;
    void *cb_arg;

    ble_gap_slave_extract_cb(instance, &cb, &cb_arg);
    if (cb != NULL) {
        memset(&event, 0, sizeof event);
        event.type = BLE_GAP_EVENT_ADV_COMPLETE;
        event.adv_complete.reason = reason;
        cb(&event, cb_arg);
    }
}


SRAM_CODE static void
ble_gap_disc_report(void *desc)
{
    struct ble_gap_master_state state;
    struct ble_gap_event event;

    memset(&event, 0, sizeof event);
#if MYNEWT_VAL(BLE_EXT_ADV)
    event.type = BLE_GAP_EVENT_EXT_DISC;
    event.ext_disc = *((struct ble_gap_ext_disc_desc *)desc);
#else
    event.type = BLE_GAP_EVENT_DISC;
    event.disc = *((struct ble_gap_disc_desc *)desc);
#endif

    ble_gap_master_extract_state(&state, 0);
    if (state.cb) {
        state.cb(&event, state.cb_arg);
    }

}

#if NIMBLE_BLE_SCAN
SRAM_CODE static void
ble_gap_disc_complete(void)
{
    struct ble_gap_master_state state;
    struct ble_gap_event event;

    memset(&event, 0, sizeof event);
    event.type = BLE_GAP_EVENT_DISC_COMPLETE;
    event.disc_complete.reason = 0;

    ble_gap_master_extract_state(&state, 1);
    if (state.cb) {
        ble_gap_call_event_cb(&event, state.cb, state.cb_arg);
    }

}
#endif



#if !MYNEWT_VAL(BLE_EXT_ADV)
SRAM_CODE static uint32_t
ble_gap_slave_ticks_until_exp(void)
{
    ble_npl_stime_t ticks;

    if (ble_gap_slave.op == BLE_GAP_OP_NULL || !ble_gap_slave.exp_set) {
        /* Timer not set; infinity ticks until next event. */
        return BLE_HS_FOREVER;
    }

    ticks = ble_gap_slave.exp_os_ticks - ble_npl_time_get();
    if (ticks > 0) {
        /* Timer not expired yet. */
        return ticks;
    }

    /* Timer just expired. */
    return 0;
}
#endif



SRAM_CODE static void
ble_gap_master_set_timer(uint32_t ticks_from_now)
{
    ble_gap_master.exp_os_ticks = ble_npl_time_get() + ticks_from_now;
    ble_gap_master.exp_set = 1;

    ble_hs_timer_resched();
}

#if !MYNEWT_VAL(BLE_EXT_ADV)
SRAM_CODE static void
ble_gap_slave_set_timer(uint32_t ticks_from_now)
{
    ble_gap_slave.exp_os_ticks = ble_npl_time_get() + ticks_from_now;
    ble_gap_slave.exp_set = 1;

    ble_hs_timer_resched();
}
#endif


/**
 * Tells you if there is an active central GAP procedure (connect or discover).
 */
SRAM_CODE int
ble_gap_master_in_progress(void)
{
    return ble_gap_master.op != BLE_GAP_OP_NULL;
}

SRAM_CODE int
ble_gap_adv_active_instance(uint8_t instance)
{
    /* Assume read is atomic; mutex not necessary. */
    return ble_gap_slave.op == BLE_GAP_OP_S_ADV;
}
SRAM_CODE int
ble_hs_adv_parse(const uint8_t *data, uint8_t length,
                 ble_hs_adv_parse_func_t func, void *user_data)
{
    const struct ble_hs_adv_field *field;

    while (length > 1) {
        field = (const void *) data;

        if (field->length >= length) {
            return BLE_HS_EBADDATA;
        }

        if (func(field, user_data) == 0) {
            return 0;
        }

        length -= 1 + field->length;
        data += 1 + field->length;
    }

    return 0;
}
SRAM_CODE static int
find_field_func(const struct ble_hs_adv_field *field, void *user_data)
{
    struct find_field_data *ffd = user_data;

    if (field->type != ffd->type) {
        return BLE_HS_EAGAIN;
    }

    ffd->field = field;

    return 0;
}



SRAM_CODE int
ble_hs_adv_find_field(uint8_t type, const uint8_t *data, uint8_t length,
                      const struct ble_hs_adv_field **out)
{
    int rc;
    struct find_field_data ffd = {
            .type = type,
            .field = NULL,
    };

    rc = ble_hs_adv_parse(data, length, find_field_func, &ffd);
    if (rc != 0) {
        return rc;
    }

    if (!ffd.field) {
        return BLE_HS_ENOENT;
    }

    *out = ffd.field;

    return 0;
}

SRAM_CODE static int
ble_gap_rx_adv_report_sanity_check(uint8_t *adv_data, uint8_t adv_data_len)
{
    const struct ble_hs_adv_field *flags;
    int rc;


    if (ble_gap_master.op != BLE_GAP_OP_M_DISC) {
        return -1;
    }


    return 0;
}

SRAM_CODE void
ble_gap_rx_adv_report(struct ble_gap_disc_desc *desc)
{
#if !NIMBLE_BLE_SCAN
    return;
#else

    if (ble_gap_rx_adv_report_sanity_check(desc->data, desc->length_data)) {
        return;
    }

    ble_gap_disc_report(desc);
#endif

}



SRAM_CODE static int
ble_gap_rd_rem_sup_feat_tx(uint16_t handle)
{
    uint8_t buf[BLE_HCI_CONN_RD_REM_FEAT_LEN];
    int rc;

    rc = ble_hs_hci_cmd_build_le_read_remote_feat(handle, buf, sizeof buf);
    if (rc != 0) {
        return BLE_HS_EUNKNOWN;
    }

    rc = ble_hs_hci_cmd_tx_empty_ack(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                                BLE_HCI_OCF_LE_RD_REM_FEAT),
                                     buf, sizeof(buf));
    if (rc != 0) {
        return rc;
    }

    return 0;
}



SRAM_CODE static uint32_t
ble_gap_master_ticks_until_exp(void)
{
    ble_npl_stime_t ticks;

    if (ble_gap_master.op == BLE_GAP_OP_NULL || !ble_gap_master.exp_set) {
        /* Timer not set; infinity ticks until next event. */
        return BLE_HS_FOREVER;
    }

    ticks = ble_gap_master.exp_os_ticks - ble_npl_time_get();
    if (ticks > 0) {
        /* Timer not expired yet. */
        return ticks;
    }

    /* Timer just expired. */
    return 0;
}

SRAM_CODE static int32_t
ble_gap_master_timer(void)
{
    uint32_t ticks_until_exp;
    int rc;

    ticks_until_exp = ble_gap_master_ticks_until_exp();
    if (ticks_until_exp != 0) {
        /* Timer not expired yet. */
        return ticks_until_exp;
    }

    /*** Timer expired; process event. */

    switch (ble_gap_master.op) {

    case BLE_GAP_OP_M_DISC:
#if NIMBLE_BLE_SCAN && !MYNEWT_VAL(BLE_EXT_ADV)
        /* When a discovery procedure times out, it is not a failure. */
        rc = ble_gap_disc_enable_tx(0, 0);
        if (rc != 0) {
            /* Failed to stop discovery; try again in 100 ms. */
            return ble_npl_time_ms_to_ticks32(BLE_GAP_CANCEL_RETRY_TIMEOUT_MS);
        }

        ble_gap_disc_complete();
#else
        assert(0);
#endif
        break;

    default:
        BLE_HS_DBG_ASSERT(0);
        break;
    }

    return BLE_HS_FOREVER;
}

#if !MYNEWT_VAL(BLE_EXT_ADV)
SRAM_CODE static int32_t
ble_gap_slave_timer(void)
{
    uint32_t ticks_until_exp;
    int rc;

    ticks_until_exp = ble_gap_slave_ticks_until_exp();
    if (ticks_until_exp != 0) {
        /* Timer not expired yet. */
        return ticks_until_exp;
    }

    /*** Timer expired; process event. */

    /* Stop advertising. */
    rc = ble_gap_adv_enable_tx(0);
    if (rc != 0) {
        /* Failed to stop advertising; try again in 100 ms. */
        return 100;
    }

    /* Clear the timer and cancel the current procedure. */
    ble_gap_slave_reset_state(0);

    /* Indicate to application that advertising has stopped. */
    ble_gap_adv_finished(0, BLE_HS_ETIMEOUT, 0, 0);

    return BLE_HS_FOREVER;
}
#endif


/**
 * Handles timed-out GAP procedures.
 *
 * @return                      The number of ticks until this function should
 *                                  be called again.
 */
SRAM_CODE int32_t
ble_gap_timer(void)
{
    int32_t update_ticks;
    int32_t master_ticks;
    int32_t min_ticks;

    master_ticks = ble_gap_master_timer();

    min_ticks = master_ticks;

#if !MYNEWT_VAL(BLE_EXT_ADV)
    min_ticks = min(min_ticks, ble_gap_slave_timer());
#endif

    return min_ticks;
}

/*****************************************************************************
 * $white list                                                               *
 *****************************************************************************/

SRAM_CODE static int
ble_gap_adv_enable_tx(int enable)
{
    uint8_t buf[BLE_HCI_SET_ADV_ENABLE_LEN];
    uint16_t opcode;
    int rc;

    opcode = BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_SET_ADV_ENABLE);
    ble_hs_hci_cmd_build_le_set_adv_enable(!!enable, buf, sizeof buf);

    rc = ble_hs_hci_cmd_tx_empty_ack(opcode, buf, sizeof(buf));
    if (rc != 0) {
        return rc;
    }

    return 0;
}

SRAM_CODE static int
ble_gap_adv_stop_no_lock(void)
{
#if !NIMBLE_BLE_ADVERTISE
    return BLE_HS_ENOTSUP;
#else

    bool active;
    int rc;


    //STATS_INC(ble_gap_stats, adv_stop);

    active = ble_gap_adv_active();

    //BLE_HS_LOG(INFO, "GAP procedure initiated: stop advertising.\n");

    rc = ble_gap_adv_enable_tx(0);
    if (rc != 0) {
        goto done;
    }

    ble_gap_slave_reset_state(0);

    if (!active) {
        rc = BLE_HS_EALREADY;
    } else {
        rc = 0;
    }

done:
    if (rc != 0) {
        //STATS_INC(ble_gap_stats, adv_stop_fail);
    }

    return rc;
#endif
}

SRAM_CODE int
ble_gap_adv_stop(void)
{
#if !NIMBLE_BLE_ADVERTISE || MYNEWT_VAL(BLE_EXT_ADV)
    return BLE_HS_ENOTSUP;
#else

    int rc;

    ble_hs_lock();
    rc = ble_gap_adv_stop_no_lock();
    ble_hs_unlock();

    return rc;
#endif
}

/*****************************************************************************
 * $advertise                                                                *
 *****************************************************************************/
SRAM_CODE static int
ble_gap_adv_type(const struct ble_gap_adv_params *adv_params)
{
    switch (adv_params->conn_mode) {
    case BLE_GAP_CONN_MODE_NON:
        if (adv_params->disc_mode == BLE_GAP_DISC_MODE_NON) {
            return BLE_HCI_ADV_TYPE_ADV_NONCONN_IND;
        } else {
            return BLE_HCI_ADV_TYPE_ADV_SCAN_IND;
        }

    case BLE_GAP_CONN_MODE_UND:
        return BLE_HCI_ADV_TYPE_ADV_IND;

    case BLE_GAP_CONN_MODE_DIR:
        if (adv_params->high_duty_cycle) {
            return BLE_HCI_ADV_TYPE_ADV_DIRECT_IND_HD;
        } else {
            return BLE_HCI_ADV_TYPE_ADV_DIRECT_IND_LD;
        }

    default:
        //BLE_HS_DBG_ASSERT(0);
        return BLE_HCI_ADV_TYPE_ADV_IND;
    }
}

SRAM_CODE static void
ble_gap_adv_dflt_itvls(uint8_t conn_mode,
                       uint16_t *out_itvl_min, uint16_t *out_itvl_max)
{

        *out_itvl_min = BLE_GAP_ADV_FAST_INTERVAL2_MIN;
        *out_itvl_max = BLE_GAP_ADV_FAST_INTERVAL2_MAX;
}

SRAM_CODE static int
ble_gap_adv_params_tx(uint8_t own_addr_type, const ble_addr_t *peer_addr,
                      const struct ble_gap_adv_params *adv_params)

{
    const ble_addr_t *peer_any = BLE_ADDR_ANY;
    struct hci_adv_params hci_adv_params;
    uint8_t buf[BLE_HCI_SET_ADV_PARAM_LEN];
    int rc;

    if (peer_addr == NULL) {
        peer_addr = peer_any;
    }

    hci_adv_params.own_addr_type = own_addr_type;
    hci_adv_params.peer_addr_type = peer_addr->type;
    memcpy(hci_adv_params.peer_addr, peer_addr->val,
           sizeof hci_adv_params.peer_addr);

    /* Fill optional fields if application did not specify them. */
    if (adv_params->itvl_min == 0 && adv_params->itvl_max == 0) {
        ble_gap_adv_dflt_itvls(adv_params->conn_mode,
                               &hci_adv_params.adv_itvl_min,
                               &hci_adv_params.adv_itvl_max);
    } else {
        hci_adv_params.adv_itvl_min = adv_params->itvl_min;
        hci_adv_params.adv_itvl_max = adv_params->itvl_max;
    }
    if (adv_params->channel_map == 0) {
        hci_adv_params.adv_channel_map = BLE_GAP_ADV_DFLT_CHANNEL_MAP;
    } else {
        hci_adv_params.adv_channel_map = adv_params->channel_map;
    }

    /* Zero is the default value for filter policy and high duty cycle */
    hci_adv_params.adv_filter_policy = adv_params->filter_policy;

    hci_adv_params.adv_type = ble_gap_adv_type(adv_params);
    rc = ble_hs_hci_cmd_build_le_set_adv_params(&hci_adv_params,
                                                buf, sizeof buf);
    if (rc != 0) {
        return BLE_HS_EINVAL;
    }

    rc = ble_hs_hci_cmd_tx_empty_ack(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                                BLE_HCI_OCF_LE_SET_ADV_PARAMS),
                                     buf, sizeof(buf));
    if (rc != 0) {
        return rc;
    }

    return 0;
}

SRAM_CODE static int
ble_gap_adv_validate(uint8_t own_addr_type, const ble_addr_t *peer_addr,
                     const struct ble_gap_adv_params *adv_params)
{
    if (adv_params == NULL) {
        return BLE_HS_EINVAL;
    }

    if (own_addr_type > BLE_HCI_ADV_OWN_ADDR_MAX) {
        return BLE_HS_EINVAL;
    }

    if (adv_params->disc_mode >= BLE_GAP_DISC_MODE_MAX) {
        return BLE_HS_EINVAL;
    }

    if (ble_gap_slave.op != BLE_GAP_OP_NULL) {
        return BLE_HS_EALREADY;
    }

    return 0;
}

SRAM_CODE int
ble_gap_adv_start(uint8_t own_addr_type, const ble_addr_t *direct_addr,
                  int32_t duration_ms,
                  const struct ble_gap_adv_params *adv_params,
                  ble_gap_event_fn *cb, void *cb_arg)
{
#if !NIMBLE_BLE_ADVERTISE || MYNEWT_VAL(BLE_EXT_ADV)
    return BLE_HS_ENOTSUP;
#else
    uint32_t duration_ticks;
    int rc;

   // STATS_INC(ble_gap_stats, adv_start);

    ble_hs_lock();

    rc = ble_gap_adv_validate(own_addr_type, direct_addr, adv_params);
    if (rc != 0) {
        goto done;
    }

    if (duration_ms != BLE_HS_FOREVER) {
        rc = ble_npl_time_ms_to_ticks(duration_ms, &duration_ticks);
        if (rc != 0) {
            /* Duration too great. */
            rc = BLE_HS_EINVAL;
            goto done;
        }
    }

    if (!ble_hs_is_enabled()) {
        rc = BLE_HS_EDISABLED;
        goto done;
    }


//    BLE_HS_LOG(INFO, "GAP procedure initiated: advertise; ");
//    ble_gap_log_adv(own_addr_type, direct_addr, adv_params);
//    BLE_HS_LOG(INFO, "\n");

    ble_gap_slave.cb = cb;
    ble_gap_slave.cb_arg = cb_arg;
    ble_gap_slave.our_addr_type = own_addr_type;

    if (adv_params->conn_mode != BLE_GAP_CONN_MODE_NON) {
        ble_gap_slave.connectable = 1;
    } else {
        ble_gap_slave.connectable = 0;
    }

    rc = ble_gap_adv_params_tx(own_addr_type, direct_addr, adv_params);
    if (rc != 0) {
        goto done;
    }

    ble_gap_slave.op = BLE_GAP_OP_S_ADV;

    rc = ble_gap_adv_enable_tx(1);
    if (rc != 0) {
        ble_gap_slave_reset_state(0);
        goto done;
    }

    if (duration_ms != BLE_HS_FOREVER) {
        ble_gap_slave_set_timer(duration_ticks);
    }

    rc = 0;

done:
    ble_hs_unlock();

    if (rc != 0) {
        //STATS_INC(ble_gap_stats, adv_start_fail);
    }
    return rc;
#endif
}

SRAM_CODE int
ble_gap_adv_set_data(const uint8_t *data, int data_len)
{
#if !NIMBLE_BLE_ADVERTISE || MYNEWT_VAL(BLE_EXT_ADV)
    return BLE_HS_ENOTSUP;
#else

    uint8_t buf[BLE_HCI_SET_ADV_DATA_LEN];
    uint16_t opcode;
    int rc;

    STATS_INC(ble_gap_stats, adv_set_data);

    ble_hs_lock();

    opcode = BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_SET_ADV_DATA);
    rc = ble_hs_hci_cmd_build_le_set_adv_data(data, data_len, buf,
                                              sizeof(buf));
    if (rc != 0) {
        goto done;
    }

    rc = ble_hs_hci_cmd_tx_empty_ack(opcode, buf, sizeof(buf));
    if (rc != 0) {
        goto done;
    }

    rc = 0;

done:
    ble_hs_unlock();
    return rc;
#endif

}
/*
SRAM_CODE int
ble_gap_adv_rsp_set_data(const uint8_t *data, int data_len)
{
#if !NIMBLE_BLE_ADVERTISE || MYNEWT_VAL(BLE_EXT_ADV)
    return BLE_HS_ENOTSUP;
#else

    uint8_t buf[BLE_HCI_SET_SCAN_RSP_DATA_LEN];


















    rc = 0;

done:
    ble_hs_unlock();
    return rc;
#endif
}
*/
SRAM_CODE int
ble_gap_adv_active(void)
{
    return ble_gap_adv_active_instance(0);
}


/*****************************************************************************
 * $discovery procedures                                                     *
 *****************************************************************************/


#if NIMBLE_BLE_SCAN
SRAM_CODE static int
ble_gap_disc_enable_tx(int enable, int filter_duplicates)
{
    uint8_t buf[BLE_HCI_SET_SCAN_ENABLE_LEN];
    int rc;

    ble_hs_hci_cmd_build_le_set_scan_enable(!!enable, !!filter_duplicates,
                                            buf, sizeof buf);
    rc = ble_hs_hci_cmd_tx_empty_ack(
        BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_SET_SCAN_ENABLE),
        buf, sizeof(buf));
    if (rc != 0) {
        return rc;
    }

    return 0;
}

SRAM_CODE static int
ble_gap_disc_tx_params(uint8_t own_addr_type,
                       const struct ble_gap_disc_params *disc_params)
{
    uint8_t buf[BLE_HCI_SET_SCAN_PARAM_LEN];
    uint8_t scan_type;
    int rc;

    if (disc_params->passive) {
        scan_type = BLE_HCI_SCAN_TYPE_PASSIVE;
    } else {
        scan_type = BLE_HCI_SCAN_TYPE_ACTIVE;
    }

    rc = ble_hs_hci_cmd_build_le_set_scan_params(scan_type,
                                                 disc_params->itvl,
                                                 disc_params->window,
                                                 own_addr_type,
                                                 disc_params->filter_policy,
                                                 buf, sizeof buf);
    if (rc != 0) {
        return BLE_HS_EINVAL;
    }

    rc = ble_hs_hci_cmd_tx_empty_ack(
        BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_SET_SCAN_PARAMS),
        buf, sizeof(buf));
    if (rc != 0) {
        return rc;
    }

    return 0;
}

SRAM_CODE static int
ble_gap_disc_disable_tx(void)
{

    return ble_gap_disc_enable_tx(0, 0);
}

SRAM_CODE static int
ble_gap_disc_cancel_no_lock(void)
{
    int rc;

    //STATS_INC(ble_gap_stats, discover_cancel);

    if (!ble_gap_disc_active()) {
        rc = BLE_HS_EALREADY;
        goto done;
    }

    rc = ble_gap_disc_disable_tx();
    if (rc != 0) {
        goto done;
    }

    ble_gap_master_reset_state();

done:
    if (rc != 0) {
        ///STATS_INC(ble_gap_stats, discover_cancel_fail);
    }

    return rc;
}
#endif

SRAM_CODE int
ble_gap_disc_cancel(void)
{
#if !NIMBLE_BLE_SCAN
    return BLE_HS_ENOTSUP;
#else

    int rc;

    ble_hs_lock();
    rc = ble_gap_disc_cancel_no_lock();
    ble_hs_unlock();

    return rc;
#endif
}

#if NIMBLE_BLE_SCAN
SRAM_CODE static int
ble_gap_disc_ext_validate(uint8_t own_addr_type)
{

    if (ble_gap_disc_active()) {
        return BLE_HS_EALREADY;
    }

    if (!ble_hs_is_enabled()) {
        return BLE_HS_EDISABLED;
    }

    return 0;
}
#endif


#if NIMBLE_BLE_SCAN 
SRAM_CODE static void
ble_gap_disc_fill_dflts(struct ble_gap_disc_params *disc_params)
{
   if (disc_params->itvl == 0) {
        if (disc_params->limited) {
            disc_params->itvl = BLE_GAP_LIM_DISC_SCAN_INT;
        } else {
            disc_params->itvl = BLE_GAP_SCAN_FAST_INTERVAL_MIN;
        }
    }

    if (disc_params->window == 0) {
        if (disc_params->limited) {
            disc_params->window = BLE_GAP_LIM_DISC_SCAN_WINDOW;
        } else {
            disc_params->window = BLE_GAP_SCAN_FAST_WINDOW;
        }
    }
}

SRAM_CODE static int
ble_gap_disc_validate(uint8_t own_addr_type,
                      const struct ble_gap_disc_params *disc_params)
{
    if (disc_params == NULL) {
        return BLE_HS_EINVAL;
    }

    return ble_gap_disc_ext_validate(own_addr_type);
}
#endif

SRAM_CODE int
ble_gap_disc(uint8_t own_addr_type, int32_t duration_ms,
             const struct ble_gap_disc_params *disc_params,
             ble_gap_event_fn *cb, void *cb_arg)
{
#if !NIMBLE_BLE_SCAN
    return BLE_HS_ENOTSUP;
#else


    struct ble_gap_disc_params params;
    uint32_t duration_ticks = 0;
    int rc;

   // STATS_INC(ble_gap_stats, discover);

    ble_hs_lock();

    /* Make a copy of the parameter strcuture and fill unspecified values with
     * defaults.
     */
    params = *disc_params;
    ble_gap_disc_fill_dflts(&params);

    rc = ble_gap_disc_validate(own_addr_type, &params);
    if (rc != 0) {
        goto done;
    }

    if (duration_ms == 0) {
        duration_ms = BLE_GAP_DISC_DUR_DFLT;
    }

    if (duration_ms != BLE_HS_FOREVER) {
        rc = ble_npl_time_ms_to_ticks(duration_ms, &duration_ticks);
        if (rc != 0) {
            /* Duration too great. */
            rc = BLE_HS_EINVAL;
            goto done;
        }
    }



   // ble_gap_master.disc.limited = params.limited;
    ble_gap_master.cb = cb;
    ble_gap_master.cb_arg = cb_arg;


    rc = ble_gap_disc_tx_params(own_addr_type, &params);
    if (rc != 0) {
		iot_printf("%s:%d\n",__FUNCTION__,__LINE__);
        goto done;
    }

    ble_gap_master.op = BLE_GAP_OP_M_DISC;

    rc = ble_gap_disc_enable_tx(1, params.filter_duplicates);
    if (rc != 0) {
        ble_gap_master_reset_state();
		iot_printf("%s:%d\n",__FUNCTION__,__LINE__);
        goto done;
    }

    if (duration_ms != BLE_HS_FOREVER) {
        ble_gap_master_set_timer(duration_ticks);
    }

    rc = 0;

done:
    ble_hs_unlock();

    if (rc != 0) {
        STATS_INC(ble_gap_stats, discover_fail);
    }
    return rc;
#endif
}

SRAM_CODE int
ble_gap_disc_active(void)
{
    /* Assume read is atomic; mutex not necessary. */
    return ble_gap_master.op == BLE_GAP_OP_M_DISC;
}



/*****************************************************************************
 * $terminate connection procedure                                           *

     * We check if element exists on the list only for sanity to let caller
     * know whether it registered its listener before.
     */



/*****************************************************************************
 * $init                                                                     *
 *****************************************************************************/

SRAM_CODE int
ble_gap_init(void)
{
    int rc;

    memset(&ble_gap_master, 0, sizeof ble_gap_master);
    memset(&ble_gap_slave, 0, sizeof ble_gap_slave);

   // ble_npl_mutex_init(&preempt_done_mutex);


    return 0;

err:
    return rc;
}

int ble_gap_free(void)
{
	//ble_npl_mutex_free(&preempt_done_mutex);
	return 0;
}

