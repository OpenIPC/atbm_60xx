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
#include <errno.h>
#include <string.h>
#include "sysinit/sysinit.h"
#include "syscfg/syscfg.h"
#include "stats/stats.h"
#include "host/ble_hs.h"
#include "ble_hs_priv.h"
#include "nimble/nimble_npl.h"
#ifndef MYNEWT
#include "nimble/nimble_port.h"
#endif

#define BLE_HS_HCI_EVT_COUNT    (MYNEWT_VAL(BLE_TRANSPORT_EVT_COUNT) + \
                                 MYNEWT_VAL(BLE_TRANSPORT_EVT_DISCARDABLE_COUNT))

static void ble_hs_event_rx_hci_ev(struct ble_npl_event *ev);
#if NIMBLE_BLE_CONNECT
static void ble_hs_event_tx_notify(struct ble_npl_event *ev);
#endif
static void ble_hs_event_reset(struct ble_npl_event *ev);
static void ble_hs_event_start_stage1(struct ble_npl_event *ev);
static void ble_hs_event_start_stage2(struct ble_npl_event *ev);
static void ble_hs_timer_sched(int32_t ticks_from_now);

struct os_mempool ble_hs_hci_ev_pool;
static os_membuf_t ble_hs_hci_os_event_buf[
    OS_MEMPOOL_SIZE(BLE_HS_HCI_EVT_COUNT, sizeof (struct ble_npl_event))
];

/** OS event - triggers tx of pending notifications and indications. */
static struct ble_npl_event ble_hs_ev_tx_notifications;

/** OS event - triggers a full reset. */
static struct ble_npl_event ble_hs_ev_reset;

static struct ble_npl_event ble_hs_ev_start_stage1;
static struct ble_npl_event ble_hs_ev_start_stage2;

uint8_t ble_hs_sync_state;
uint8_t ble_hs_enabled_state;
static int ble_hs_reset_reason;

#define BLE_HS_SYNC_RETRY_TIMEOUT_MS    100 /* ms */

static void *ble_hs_parent_task;

/**
 * Handles unresponsive timeouts and periodic retries in case of resource
 * shortage.
 */
static struct ble_npl_callout ble_hs_timer;

/* Shared queue that the host uses for work items. */
static struct ble_npl_eventq *ble_hs_evq;

static struct ble_mqueue ble_hs_rx_q;

static struct ble_npl_mutex ble_hs_mutex;

/** These values keep track of required ATT and GATT resources counts.  They
 * increase as services are added, and are read when the ATT server and GATT
 * server are started.
 */
uint16_t ble_hs_max_attrs;
uint16_t ble_hs_max_services;
uint16_t ble_hs_max_client_configs;

#if MYNEWT_VAL(BLE_HS_DEBUG)
static uint8_t ble_hs_dbg_mutex_locked;
#endif

STATS_SECT_DECL(ble_hs_stats) ble_hs_stats;
STATS_NAME_START(ble_hs_stats)
    STATS_NAME(ble_hs_stats, conn_create)
    STATS_NAME(ble_hs_stats, conn_delete)
    STATS_NAME(ble_hs_stats, hci_cmd)
    STATS_NAME(ble_hs_stats, hci_event)
    STATS_NAME(ble_hs_stats, hci_invalid_ack)
    STATS_NAME(ble_hs_stats, hci_unknown_event)
    STATS_NAME(ble_hs_stats, hci_timeout)
    STATS_NAME(ble_hs_stats, reset)
    STATS_NAME(ble_hs_stats, sync)
    STATS_NAME(ble_hs_stats, pvcy_add_entry)
    STATS_NAME(ble_hs_stats, pvcy_add_entry_fail)
STATS_NAME_END(ble_hs_stats)

struct ble_npl_eventq *
ble_hs_evq_get(void)
{
    return ble_hs_evq;
}

void
ble_hs_evq_set(struct ble_npl_eventq *evq)
{
    ble_hs_evq = evq;
}

#if MYNEWT_VAL(BLE_HS_DEBUG)
int
ble_hs_locked_by_cur_task(void)
{
#if MYNEWT
    struct os_task *owner;

    if (!ble_npl_os_started()) {
        return ble_hs_dbg_mutex_locked;
    }

    owner = ble_hs_mutex.mu.mu_owner;
    return owner != NULL && owner == os_sched_get_current_task();
#else
    return 1;
#endif
}
#endif

/**
 * Indicates whether the host's parent task is currently running.
 */
int
ble_hs_is_parent_task(void)
{
    return !ble_npl_os_started() ||
           ble_npl_get_current_task_id() == ble_hs_parent_task;
}

/**
 * Locks the BLE host mutex.  Nested locks allowed.
 */
void
ble_hs_lock_nested(void)
{
    int rc;

#if MYNEWT_VAL(BLE_HS_DEBUG)
    if (!ble_npl_os_started()) {
        ble_hs_dbg_mutex_locked = 1;
        return;
    }
#endif

    rc = ble_npl_mutex_pend(&ble_hs_mutex, BLE_NPL_TIME_FOREVER);
    BLE_HS_DBG_ASSERT_EVAL(rc == 0 || rc == OS_NOT_STARTED);
}

/**
 * Unlocks the BLE host mutex.  Nested locks allowed.
 */
void
ble_hs_unlock_nested(void)
{
    int rc;

#if MYNEWT_VAL(BLE_HS_DEBUG)
    if (!ble_npl_os_started()) {
        ble_hs_dbg_mutex_locked = 0;
        return;
    }
#endif

    rc = ble_npl_mutex_release(&ble_hs_mutex);
    BLE_HS_DBG_ASSERT_EVAL(rc == 0 || rc == OS_NOT_STARTED);
}

/**
 * Locks the BLE host mutex.  Nested locks not allowed.
 */
void
ble_hs_lock(void)
{
    BLE_HS_DBG_ASSERT(!ble_hs_locked_by_cur_task());
#if MYNEWT_VAL(BLE_HS_DEBUG)
    if (!ble_npl_os_started()) {
        BLE_HS_DBG_ASSERT(!ble_hs_dbg_mutex_locked);
    }
#endif

    ble_hs_lock_nested();
}

/**
 * Unlocks the BLE host mutex.  Nested locks not allowed.
 */
void
ble_hs_unlock(void)
{
#if MYNEWT_VAL(BLE_HS_DEBUG)
    if (!ble_npl_os_started()) {
        BLE_HS_DBG_ASSERT(ble_hs_dbg_mutex_locked);
    }
#endif

    ble_hs_unlock_nested();
}

void
ble_hs_process_rx_data_queue(void)
{
    struct os_mbuf *om;

    while ((om = ble_mqueue_get(&ble_hs_rx_q)) != NULL) {
        ble_hs_hci_evt_acl_process(om);
    }
}

static int
ble_hs_wakeup_tx_conn(struct ble_hs_conn *conn)
{
    struct os_mbuf_pkthdr *omp;
    struct os_mbuf *om;
    int rc;

    while ((omp = STAILQ_FIRST(&conn->bhc_tx_q)) != NULL) {
        STAILQ_REMOVE_HEAD(&conn->bhc_tx_q, omp_next);

        om = OS_MBUF_PKTHDR_TO_MBUF(omp);
        rc = ble_hs_hci_acl_tx_now(conn, &om);
        if (rc == BLE_HS_EAGAIN) {
            /* Controller is at capacity.  This packet will be the first to
             * get transmitted next time around.
             */
            STAILQ_INSERT_HEAD(&conn->bhc_tx_q, OS_MBUF_PKTHDR(om), omp_next);
            return BLE_HS_EAGAIN;
        }
    }

    return 0;
}

/**
 * Schedules the transmission of all queued ACL data packets to the controller.
 */
void
ble_hs_wakeup_tx(void)
{
    struct ble_hs_conn *conn;
    int rc;

    ble_hs_lock();

    /* If there is a connection with a partially transmitted packet, it has to
     * be serviced first.  The controller is waiting for the remainder so it
     * can reassemble it.
     */
    for (conn = ble_hs_conn_first();
         conn != NULL;
         conn = SLIST_NEXT(conn, bhc_next)) {

        if (conn->bhc_flags & BLE_HS_CONN_F_TX_FRAG) {
            rc = ble_hs_wakeup_tx_conn(conn);
            if (rc != 0) {
                goto done;
            }
            break;
        }
    }

    /* For each connection, transmit queued packets until there are no more
     * packets to send or the controller's buffers are exhausted.
     */
    for (conn = ble_hs_conn_first();
         conn != NULL;
         conn = SLIST_NEXT(conn, bhc_next)) {

        rc = ble_hs_wakeup_tx_conn(conn);
        if (rc != 0) {
            goto done;
        }
    }

done:
    ble_hs_unlock();
}

static void
ble_hs_clear_rx_queue(void)
{
    struct os_mbuf *om;

    while ((om = ble_mqueue_get(&ble_hs_rx_q)) != NULL) {
        os_mbuf_free_chain(om);
    }
}

int
ble_hs_is_enabled(void)
{
    return ble_hs_enabled_state == BLE_HS_ENABLED_STATE_ON;
}

int
ble_hs_synced(void)
{
    return ble_hs_sync_state == BLE_HS_SYNC_STATE_GOOD;
}

static int
ble_hs_sync(void)
{
    ble_npl_time_t retry_tmo_ticks;
    int rc;

    /* Set the sync state to "bringup."  This allows the parent task to send
     * the startup sequence to the controller.  No other tasks are allowed to
     * send any commands.
     */
    ble_hs_sync_state = BLE_HS_SYNC_STATE_BRINGUP;

    rc = ble_hs_startup_go();
    if (rc == 0) {
        ble_hs_sync_state = BLE_HS_SYNC_STATE_GOOD;
    } else {
        ble_hs_sync_state = BLE_HS_SYNC_STATE_BAD;
    }

    retry_tmo_ticks = ble_npl_time_ms_to_ticks32(BLE_HS_SYNC_RETRY_TIMEOUT_MS);
    ble_hs_timer_sched(retry_tmo_ticks);

    if (rc == 0) {
        rc = ble_hs_misc_restore_irks();
        if (rc != 0) {
            BLE_HS_LOG(INFO, "Failed to restore IRKs from store; status=%d\n",
                       rc);
        }

        if (ble_hs_cfg.sync_cb != NULL) {
            ble_hs_cfg.sync_cb();
        }

        STATS_INC(ble_hs_stats, sync);
    }

    return rc;
}

static int
ble_hs_reset(void)
{
    uint16_t conn_handle;
    int rc;

    STATS_INC(ble_hs_stats, reset);

    ble_hs_sync_state = 0;

    /* Reset transport.  Assume success; there is nothing we can do in case of
     * failure.  If the transport failed to reset, the host will reset itself
     * again when it fails to sync with the controller.
     */
    (void)ble_hci_trans_reset();

    ble_hs_clear_rx_queue();
	
    /* Clear adverising and scanning states. */
    ble_gap_reset_state(ble_hs_reset_reason);

    /* Clear configured addresses. */
    ble_hs_id_reset();

    if (ble_hs_cfg.reset_cb != NULL && ble_hs_reset_reason != 0) {
        ble_hs_cfg.reset_cb(ble_hs_reset_reason);
    }
    ble_hs_reset_reason = 0;

    rc = ble_hs_sync();
    return rc;
}

/**
 * Called when the host timer expires.  Handles unresponsive timeouts and
 * periodic retries in case of resource shortage.
 */
extern void exit_ble(void);
static void
ble_hs_timer_exp(struct ble_npl_event *ev)
{
    int32_t ticks_until_next;

    switch (ble_hs_sync_state) {
    case BLE_HS_SYNC_STATE_GOOD:
#if NIMBLE_BLE_CONNECT
        ticks_until_next = ble_gattc_timer();
        ble_hs_timer_sched(ticks_until_next);

        ticks_until_next = ble_l2cap_sig_timer();
        ble_hs_timer_sched(ticks_until_next);

        ticks_until_next = ble_sm_timer();
        ble_hs_timer_sched(ticks_until_next);

        ticks_until_next = ble_hs_conn_timer();
        ble_hs_timer_sched(ticks_until_next);
#endif

        ticks_until_next = ble_gap_timer();
        ble_hs_timer_sched(ticks_until_next);

        break;

    case BLE_HS_SYNC_STATE_BAD:
		//exit_ble();
        ble_hs_reset();
        break;

    case BLE_HS_SYNC_STATE_BRINGUP:
    default:
        /* The timer should not be set in this state. */
        assert(0);
        break;
    }

}

static void
ble_hs_timer_reset(uint32_t ticks)
{
    int rc;

    if (!ble_hs_is_enabled()) {
        ble_npl_callout_stop(&ble_hs_timer);
    } else {
        rc = ble_npl_callout_reset(&ble_hs_timer, ticks);
        BLE_HS_DBG_ASSERT_EVAL(rc == 0);
    }
}

static void
ble_hs_timer_sched(int32_t ticks_from_now)
{
    ble_npl_time_t abs_time;

    if (ticks_from_now == BLE_HS_FOREVER) {
        return;
    }

    /* Reset timer if it is not currently scheduled or if the specified time is
     * sooner than the previous expiration time.
     */
    abs_time = ble_npl_time_get() + ticks_from_now;
    if (!ble_npl_callout_is_active(&ble_hs_timer) ||
            ((ble_npl_stime_t)(abs_time -
                               ble_npl_callout_get_ticks(&ble_hs_timer))) < 0) {
        ble_hs_timer_reset(ticks_from_now);
    }
}

void
ble_hs_timer_resched(void)
{
    /* Reschedule the timer to run immediately.  The timer callback will query
     * each module for an up-to-date expiration time.
     */
    ble_hs_timer_reset(0);
}

static void
ble_hs_sched_start_stage2(void)
{
    ble_npl_eventq_put((struct ble_npl_eventq *)ble_hs_evq_get(),
                       &ble_hs_ev_start_stage2);
}

void
ble_hs_sched_start(void)
{
#ifdef MYNEWT
    ble_npl_eventq_put((struct ble_npl_eventq *)os_eventq_dflt_get(),
                       &ble_hs_ev_start_stage1);
#else
    ble_npl_eventq_put(nimble_port_get_dflt_eventq(), &ble_hs_ev_start_stage1);
#endif
}

static void
ble_hs_event_rx_hci_ev(struct ble_npl_event *ev)
{
    struct ble_hci_ev *hci_ev;
    int rc;

    hci_ev = ble_npl_event_get_arg(ev);

    rc = os_memblock_put(&ble_hs_hci_ev_pool, ev);
    BLE_HS_DBG_ASSERT_EVAL(rc == 0);

    ble_hs_hci_evt_process(hci_ev);
}

#if NIMBLE_BLE_CONNECT
static void
ble_hs_event_tx_notify(struct ble_npl_event *ev)
{
    ble_gatts_tx_notifications();
}
#endif

static void
ble_hs_event_rx_data(struct ble_npl_event *ev)
{
    ble_hs_process_rx_data_queue();
}

static void
ble_hs_event_reset(struct ble_npl_event *ev)
{
    ble_hs_reset();
}

/**
 * Implements the first half of the start process.  This just enqueues another
 * event on the host parent task's event queue.
 *
 * Starting is done in two stages to allow the application time to configure
 * the event queue to use after system initialization but before the host
 * starts.
 */
static void
ble_hs_event_start_stage1(struct ble_npl_event *ev)
{
    ble_hs_sched_start_stage2();
}

/**
 * Implements the second half of the start process.  This actually starts the
 * host.
 *
 * Starting is done in two stages to allow the application time to configure
 * the event queue to use after system initialization but before the host
 * starts.
 */
static void
ble_hs_event_start_stage2(struct ble_npl_event *ev)
{
    int rc;

    rc = ble_hs_start();
    assert(rc == 0);
}

void
ble_hs_enqueue_hci_event(uint8_t *hci_evt)
{
    struct ble_npl_event *ev;

    ev = os_memblock_get(&ble_hs_hci_ev_pool);
    if (ev == NULL) {
        ble_hci_trans_buf_free(hci_evt);
    } else {
        ble_npl_event_init(ev, ble_hs_event_rx_hci_ev, hci_evt);
        ble_npl_eventq_put(ble_hs_evq, ev);
    }
}

/**
 * Schedules for all pending notifications and indications to be sent in the
 * host parent task.
 */
void
ble_hs_notifications_sched(void)
{
#if !MYNEWT_VAL(BLE_HS_REQUIRE_OS)
    if (!ble_npl_os_started()) {
        ble_gatts_tx_notifications();
        return;
    }
#endif

    ble_npl_eventq_put(ble_hs_evq, &ble_hs_ev_tx_notifications);
}

void
ble_hs_sched_reset(int reason)
{
    BLE_HS_DBG_ASSERT(ble_hs_reset_reason == 0);

    ble_hs_reset_reason = reason;
    ble_npl_eventq_put(ble_hs_evq, &ble_hs_ev_reset);
}

void
ble_hs_hw_error(uint8_t hw_code)
{
    ble_hs_sched_reset(BLE_HS_HW_ERR(hw_code));
}

int
ble_hs_start(void)
{
    int rc;

    ble_hs_lock();
    switch (ble_hs_enabled_state) {
    case BLE_HS_ENABLED_STATE_ON:
        rc = BLE_HS_EALREADY;
        break;

    case BLE_HS_ENABLED_STATE_STOPPING:
        rc = BLE_HS_EBUSY;
        break;

    case BLE_HS_ENABLED_STATE_OFF:
        ble_hs_enabled_state = BLE_HS_ENABLED_STATE_ON;
        rc = 0;
        break;

    default:
        assert(0);
        rc = BLE_HS_EUNKNOWN;
        break;
    }
    ble_hs_unlock();

    if (rc != 0) {
        return rc;
    }

    ble_hs_parent_task = ble_npl_get_current_task_id();

#if MYNEWT_VAL(SELFTEST)
    /* Stop the timer just in case the host was already running (e.g., unit
     * tests).
     */
    ble_npl_callout_stop(&ble_hs_timer);
#endif

    ble_npl_callout_init(&ble_hs_timer, ble_hs_evq, ble_hs_timer_exp, NULL);

#if NIMBLE_BLE_CONNECT
    rc = ble_gatts_start();
    if (rc != 0) {
        return rc;
    }
#endif
    ble_hs_sync();

    return 0;
}

/**
 * Called when a data packet is received from the controller.  This function
 * consumes the supplied mbuf, regardless of the outcome.
 *
 * @param om                    The incoming data packet, beginning with the
 *                                  HCI ACL data header.
 *
 * @return                      0 on success; nonzero on failure.
 */
static int
ble_hs_rx_data(struct os_mbuf *om, void *arg)
{
    int rc;

    /* If flow control is enabled, mark this packet with its corresponding
     * connection handle.
     */
    ble_hs_flow_track_data_mbuf(om);

    rc = ble_mqueue_put(&ble_hs_rx_q, ble_hs_evq, om);
    if (rc != 0) {
        os_mbuf_free_chain(om);
        return BLE_HS_EOS;
    }

    return 0;
}

/**
 * Enqueues an ACL data packet for transmission.  This function consumes the
 * supplied mbuf, regardless of the outcome.
 *
 * @param om                    The outgoing data packet, beginning with the
 *                                  HCI ACL data header.
 *
 * @return                      0 on success; nonzero on failure.
 */
int ble_hs_tx_data(struct os_mbuf *om)
{
    return ble_hci_trans_hs_acl_tx(om);
}


SRAM_CODE int ble_hs_dtm_tx_test(uint8_t channel, uint8_t length, uint8_t Packet_Payload, uint8_t phy_mode)
{
	uint8_t rc;
	uint8_t buf[4];
	buf[0] = channel;
	buf[1] = length;
	buf[2] = Packet_Payload;
	buf[3] = phy_mode;

	iot_printf("HCI_LE_Enhanced_Transmitter_Test:");
	iot_printf("buf[0] = %0x, buf[1] = %0x, buf[2] = %0x, buf[3] = %0x \n\n",
		buf[0],buf[1],buf[2],buf[3]);
    rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                      BLE_HCI_OCF_LE_TX_TEST_V2),
                                      buf, sizeof(buf), NULL, 0);

	if(rc == 0){
		iot_printf(" \r\n HCI_LE_TRANSMITTER_TEST_OCF SUCCEED\r\n");

	}else{
		iot_printf(" \r\n HCI_LE_TRANSMITTER_TEST_OCF FAILED\r\n");
	}
    return rc;
}

SRAM_CODE int ble_hs_dtm_rx_test(uint8_t channel, uint8_t phy_mode)
{

	uint8_t rc;
	uint8_t buf[3];
	buf[0] = channel;
	buf[1] = phy_mode;
	buf[2] = 0x00;
	iot_printf("HCI_LE_Enhanced_Receiver_Test:");
	iot_printf("buf[0] = %0x, buf[1] = %0x\n\n",buf[0],buf[1]);
    rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                      BLE_HCI_OCF_LE_RX_TEST_V2),
                                      buf, sizeof(buf), NULL, 0);
	if(rc == 0){
		iot_printf(" \r\n HCI_LE_RECEIVER_TEST_OCF SUCCEED\r\n");

	}else{
		iot_printf(" \r\n HCI_LE_RECEIVER_TEST_OCF FAILED\r\n");
	}
    return rc;
}

SRAM_CODE int ble_hs_dtm_end(void)
{

    uint8_t rc;
	uint8_t rspbuf[2];
	uint8_t rsplen;

	rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_TEST_END),
                            NULL, 0, rspbuf, 2);

							
	if(rc == 0){
		iot_printf(" \r\n HCI_LE_Test_End SUCCEED\r\n");
		return (rspbuf[1]<<8 | rspbuf[0]);

	}else{
		iot_printf(" \r\n HCI_LE_Test_End FAILED\r\n");
		return rc;
	}
}

SRAM_CODE int ble_tone_tx_start(uint8_t channel, uint8_t power)
{
	int rc;
	uint8_t buf[2];
	
	buf[0] = channel;
	buf[1] = power;

	iot_printf("ble_tone_tx_start, ch:%d, pwr:%d\n", channel, power);
    rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_VENDOR, BLE_HCI_VS_TONE_TX_START),
        						buf, sizeof(buf), NULL, 0);
	if(rc == 0){
		iot_printf(" \r\n ble_tone_tx_start SUCCEED\r\n");

	}else{
		iot_printf(" \r\n ble_tone_tx_start FAILED\r\n");
	}
	return rc;
}

SRAM_CODE int ble_tone_tx_stop(void)
{
	int rc;

	iot_printf("ble_tone_tx_stop\n");
    rc =  ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_VENDOR, BLE_HCI_VS_TONE_TX_STOP),
        						NULL, 0, NULL, 0);
	if(rc == 0){
		iot_printf(" \r\n ble_tone_tx_stop SUCCEED\r\n");

	}else{
		iot_printf(" \r\n ble_tone_tx_stop FAILED\r\n");
	}
	return rc;
}

SRAM_CODE int ble_start_srrc(void)
{
	int rc;

	iot_printf("ble_start_srrc\n");
    rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_VENDOR, BLE_HCI_VS_START_SRRC),
        						NULL, 0, NULL, 0);
	if(rc == 0){
		iot_printf(" \r\n ble_start_srrc SUCCEED\r\n");

	}else{
		iot_printf(" \r\n ble_start_srrc FAILED\r\n");
	}
	return rc;
}

SRAM_CODE int ble_stop_srrc(void)
{
	int rc;

	iot_printf("ble_stop_srrc\n");
    rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_VENDOR, BLE_HCI_VS_STOP_SRRC),
        						NULL, 0, NULL, 0);
	if(rc == 0){
		iot_printf(" \r\n ble_stop_srrc SUCCEED\r\n");

	}else{
		iot_printf(" \r\n ble_stop_srrc FAILED\r\n");
	}
	return rc;
}

SRAM_CODE int ble_hs_scan_set_name_filt(uint8_t name_len, uint8_t *name)
{
	int rc;
	uint8_t buf[32];


	if(name_len > 31){
		return -1;
	}
	
	buf[0] = name_len;
	memcpy(&buf[1], name, name_len);

    rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_VENDOR, BLE_HCI_VS_SET_SCAN_NAME_FILT),
        						buf, (buf[0]+1), NULL, 0);
	
	return rc;
}


static uint8_t scan_report_filter_count = 0;
static uint8_t scan_report_filter[3][33] = {0};

SRAM_CODE static void
print_bytes(const uint8_t *bytes, int len)
{
    int i;

    for (i = 0; i < len; i++) {
        iot_printf("%s0x%02x", i != 0 ? ":" : "", bytes[i]);
    }
}

SRAM_CODE static void ble_hs_scan_report_filter_print(void)
{
    int i = 0;
    
    iot_printf("there are %d scan filters\n", scan_report_filter_count);
    for (i = 0; i < scan_report_filter_count; ++i) {
        if (scan_report_filter[i][0] == 0) {
            iot_printf("%d. start_byte_index: %2d, filter string:%s\n", i+1, 
                                                                        scan_report_filter[i][1],
                                                                        &scan_report_filter[i][3]);
        } else {
            iot_printf("%d. start_byte_index: %2d, filter string:", i + 1,
                                                                    scan_report_filter[i][1]);
            print_bytes(&scan_report_filter[i][3], scan_report_filter[i][2]);
            iot_printf("\n");
        }
    } 
}
static SRAM_CODE int
parse_arg_byte_stream_delim(char *sval, char *delims, int max_len,
                            uint8_t *dst, int *out_len)
{
    unsigned long ul;
    char *endptr;
    char *token;
    int i;

    i = 0;
    for (token = strtok(sval, delims);
         token != NULL;
         token = strtok(NULL, delims)) {

        if (i >= max_len) {
            return EINVAL;
        }
        ul = strtoul(token, &endptr, 16);		
        if (sval[0] == '\0' || *endptr != '\0' || ul > UINT8_MAX) {
            return -1;
        }

        dst[i] = ul;
        i++;
    }

    *out_len = i;

    return 0;
}

// parse hex string to normal string
SRAM_CODE static int ble_hs_parse_hex_string_to_string(uint8_t *hex_string, uint8_t *normal_string, uint8_t *const filter_string_len)
{
    uint8_t hex_string_len;
    uint8_t normal_string_len;
    int tmp_len;
    int rc;
    
    hex_string_len = strlen(hex_string);
    if ((hex_string_len + 1) % 3 != 0) {
        iot_printf("invalid hex string len, please check hex string format\n");
        return -1;
    }
    normal_string_len = (hex_string_len + 1)/3;

    if (normal_string_len > CMD_FILTER_STRING_MAX_LENGTH) {
        iot_printf("err: normal string is too long\n");
        return -1;
    }

    rc = parse_arg_byte_stream_delim(hex_string, ":", normal_string_len, normal_string, &tmp_len);

    //iot_printf("normal_string_len is %d, tmp_len is %d\n", normal_string_len, tmp_len);
    
    if (rc != 0) {
        iot_printf("err: parse_arg_byte_stream_delim fail\n");
        return rc;
    }

    if (tmp_len != normal_string_len) {
        return EINVAL;
    }

    *filter_string_len = normal_string_len;
    return 0;
}


SRAM_CODE int ble_scan_filt_set_by_user(uint8_t start_byte_index, bool flag_hex_string, uint8_t *user_string)
{
    int rc;
    uint8_t filter_string[60] = {0};
    uint8_t user_string_len = 0;

    user_string_len = strlen(user_string);

    if (user_string_len > 59) {
        iot_printf("err: user_string is too long\n");
        return -1;
    }

    memcpy(filter_string, user_string, user_string_len);

    return ble_hs_scan_filt_set(start_byte_index, flag_hex_string, filter_string);
}



SRAM_CODE int ble_hs_scan_filt_set(uint8_t start_byte_index, bool flag_hex_string, uint8_t *filter_string)
{
	int rc;
    int i = 1;
    uint8_t filter_string_len = 0;
	uint8_t buf[32] = {0};
    uint8_t normal_string[32] ={0};
    uint8_t tmp[33] = {0};

    //iot_printf("strlen is %d\n", strlen(filter_string));
    if (scan_report_filter_count >= 3) {
        iot_printf("can't set new scan filter for having reached its max num 3\n");
        return -1;
    }

    if (flag_hex_string) {
        tmp[0] = 1; // 1 --- hex string, 0 --- normal string
        rc = ble_hs_parse_hex_string_to_string(filter_string, normal_string, &filter_string_len);
        if (rc != 0) {
            iot_printf("parse hex string to normal string fail\n");
            return rc;
        }
        filter_string = normal_string;
    } else {
        filter_string_len = strlen(filter_string);
    }

    if (start_byte_index < 0 || start_byte_index > CMD_SCAN_FILTER_BYTE_INDEX_MAX_VALUE) {
        iot_printf("start_byte_index invalid\n");
        return -1;
    }
    if (filter_string_len > CMD_FILTER_STRING_MAX_LENGTH || filter_string_len <= 0) {
        iot_printf("err: filter_string_len: %d, is too big or <= 0\n", filter_string_len);
        return -1;
    }
    if (start_byte_index + filter_string_len > CMD_ADV_PDU_PAYLOAD_MAX_LEN) {
        iot_printf("invalid params, can't set (start_byte + string_len) > adv packet's max length\n");
        return -1;
    }

    /** buf[0] indicate set scan filter or clear scan filter
      * 0 --- clear scan filter
      * 1 --- set scan filter
      */
    buf[0] = 1;
    buf[1] = start_byte_index;
    buf[2] = filter_string_len;
    memcpy(&buf[3], filter_string, filter_string_len);
    memcpy(&tmp[1], &buf[1], 31);

    rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_VENDOR, BLE_HCI_VS_SCAN_FILT_CONTROL),
        						buf, sizeof(buf), NULL, 0);
    if (rc != 0) {
        iot_printf("scan_filt_set cmd send failed\n");
        return rc;
    }
    iot_printf("before set:\n");
    ble_hs_scan_report_filter_print();
    memcpy(scan_report_filter[scan_report_filter_count], tmp, 32);
    scan_report_filter_count++;
    iot_printf("after set:\n");
    ble_hs_scan_report_filter_print();
  
	return rc;
}

SRAM_CODE int ble_hs_scan_filt_clr(void)
{
	uint8_t buf;
    int rc;
    int i = 0;

    iot_printf("before clr_scan_filter:\n");
    ble_hs_scan_report_filter_print();
    
    buf = 0;
    rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_VENDOR, BLE_HCI_VS_SCAN_FILT_CONTROL),
        						&buf, sizeof(buf), NULL, 0);
    if (rc != 0) {
        iot_printf("scan_filt_clr cmd send failed\n");
        return rc;
    }

    for (i = 0; i < scan_report_filter_count; ++i) {
        memset(scan_report_filter[i], 0, 32 * sizeof(uint8_t));
    }
    scan_report_filter_count = 0; 
    iot_printf("after clr_scan_filter:\n");
    ble_hs_scan_report_filter_print();
  
	return rc;
}


SRAM_CODE int ble_hs_afh_param_set(uint8_t threshold, uint8_t period)
{
    int rc;
	uint8_t buf[2];
	
	buf[0] = threshold;
	buf[1] = period;

    rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_VENDOR, BLE_HCI_VS_SET_AFH_PARAM),
        						buf, 2, NULL, 0);

	if(rc == 0){
		iot_printf("ble set afh param SUCCEED\r\n");
	}else{
		iot_printf("ble set afh param FAILED\r\n");
	}
	
}


SRAM_CODE int ble_hs_set_ce_test(uint8_t enable, int8_t rssi)
{
	int rc;
	uint8_t buf[2];

	
	buf[0] = enable;
	buf[1] = rssi;

    rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_VENDOR, BLE_HCI_VS_SET_CE_TEST),
        						buf, 2, NULL, 0);

	if(rc == 0){
		iot_printf("ble set ce test SUCCEED\r\n");

	}else{
		iot_printf("ble set ce test FAILED\r\n");
	}
	
	return rc;
}


SRAM_CODE int ble_hs_hci_set_pub_addr(uint8_t *addr)
{
	int rc;
	uint8_t buf[BLE_DEV_ADDR_LEN];

	memcpy(&buf[0], addr, BLE_DEV_ADDR_LEN);
    rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_VENDOR, BLE_HCI_VS_SET_PUB_ADDR),
        						buf, BLE_DEV_ADDR_LEN, NULL, 0);

	if(rc == 0){
		iot_printf("ble set pub addr SUCCEED\r\n");

	}else{
		iot_printf("ble set pub addr FAILED\r\n");
	}

	return rc;
}


void ble_hs_init(void)
{
    int rc;

    /* Ensure this function only gets called by sysinit. */
    SYSINIT_ASSERT_ACTIVE();

    /* Create memory pool of OS events */
    rc = os_mempool_init(&ble_hs_hci_ev_pool, BLE_HS_HCI_EVT_COUNT,
                         sizeof (struct ble_npl_event), ble_hs_hci_os_event_buf,
                         "ble_hs_hci_ev_pool");
    SYSINIT_PANIC_ASSERT(rc == 0);

    /* These get initialized here to allow unit tests to run without a zeroed
     * bss.
     */
    ble_hs_reset_reason = 0;
    ble_hs_enabled_state = BLE_HS_ENABLED_STATE_OFF;

#if NIMBLE_BLE_CONNECT
    ble_npl_event_init(&ble_hs_ev_tx_notifications, ble_hs_event_tx_notify,
                       NULL);
#endif
    ble_npl_event_init(&ble_hs_ev_reset, ble_hs_event_reset, NULL);
    ble_npl_event_init(&ble_hs_ev_start_stage1, ble_hs_event_start_stage1,
                       NULL);
    ble_npl_event_init(&ble_hs_ev_start_stage2, ble_hs_event_start_stage2,
                       NULL);

    ble_hs_hci_init();

    rc = ble_hs_conn_init();
    SYSINIT_PANIC_ASSERT(rc == 0);

#if MYNEWT_VAL(BLE_PERIODIC_ADV)
    rc = ble_hs_periodic_sync_init();
    SYSINIT_PANIC_ASSERT(rc == 0);
#endif

#if NIMBLE_BLE_CONNECT
    rc = ble_l2cap_init();
    SYSINIT_PANIC_ASSERT(rc == 0);

    rc = ble_att_init();
    SYSINIT_PANIC_ASSERT(rc == 0);

    rc = ble_att_svr_init();
    SYSINIT_PANIC_ASSERT(rc == 0);

    rc = ble_gattc_init();
    SYSINIT_PANIC_ASSERT(rc == 0);

    rc = ble_gatts_init();
    SYSINIT_PANIC_ASSERT(rc == 0);
#endif
    rc = ble_gap_init();
    SYSINIT_PANIC_ASSERT(rc == 0);

    ble_hs_stop_init();

    ble_mqueue_init(&ble_hs_rx_q, ble_hs_event_rx_data, NULL);

    rc = stats_init_and_reg(
        STATS_HDR(ble_hs_stats), STATS_SIZE_INIT_PARMS(ble_hs_stats,
        STATS_SIZE_32), STATS_NAME_INIT_PARMS(ble_hs_stats), "ble_hs");
    SYSINIT_PANIC_ASSERT(rc == 0);

    rc = ble_npl_mutex_init(&ble_hs_mutex);
    SYSINIT_PANIC_ASSERT(rc == 0);

#if MYNEWT_VAL(BLE_HS_DEBUG)
    ble_hs_dbg_mutex_locked = 0;
#endif

#ifdef MYNEWT
    ble_hs_evq_set((struct ble_npl_eventq *)os_eventq_dflt_get());
#else
    ble_hs_evq_set(nimble_port_get_dflt_eventq());
#endif

    /* Enqueue the start event to the default event queue.  Using the default
     * queue ensures the event won't run until the end of main().  This allows
     * the application to configure this package in the meantime.
     */
#if 0//MYNEWT_VAL(BLE_HS_AUTO_START)
#ifdef MYNEWT
    ble_npl_eventq_put((struct ble_npl_eventq *)os_eventq_dflt_get(),
                       &ble_hs_ev_start_stage1);
#else
    ble_npl_eventq_put(nimble_port_get_dflt_eventq(), &ble_hs_ev_start_stage1);
#endif
#endif
}

/* Transport APIs for HS side */

int
ble_transport_to_hs_evt_impl(void *buf)
{
    return ble_hs_hci_rx_evt(buf, NULL);
}

int
ble_transport_to_hs_acl_impl(struct os_mbuf *om)
{
    return ble_hs_rx_data(om, NULL);
}

int
ble_transport_to_hs_iso_impl(struct os_mbuf *om)
{
    os_mbuf_free_chain(om);

    return 0;
}

void
ble_transport_hs_init(void)
{
    ble_hs_init();
	ble_hci_trans_cfg_hs(ble_hs_hci_rx_evt, NULL, ble_hs_rx_data, NULL);
}

void ble_hs_free(void)
{
	ble_npl_callout_free(&ble_hs_timer);
	ble_npl_mutex_free(&ble_hs_mutex);
	ble_gatts_reset();
}



