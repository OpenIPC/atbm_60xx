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

#include <stddef.h>
#include "os/os.h"
#include "sysinit/sysinit.h"
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#if NIMBLE_CFG_CONTROLLER
#include "controller/ble_ll.h"
#include "nimble/transport.h"
#endif

static struct ble_npl_eventq g_eventq_dflt;

pAtbm_thread_t host_thread = NULL;
int nimble_th_exit = 0;

extern void os_msys_init(void);
extern void os_mempool_module_init(void);

void
nimble_port_init(void)
{
    /* Initialize default event queue */
    ble_npl_eventq_init(&g_eventq_dflt);
    /* Initialize the global memory pool */
    os_mempool_module_init();
    os_msys_init();

    /* XXX Need to have template for store */
    ble_store_ram_init();
	ble_hci_ram_init();	

    /* Initialize the host */
    ble_transport_hs_init();
}

void nimble_port_release(void)
{
	ble_npl_eventq_release(&g_eventq_dflt);
}

void
nimble_port_run(void)
{
    struct ble_npl_event *ev;

    while (1) {
        ev = ble_npl_eventq_get(&g_eventq_dflt, BLE_NPL_TIME_FOREVER);
		if(nimble_th_exit){
			break;
		}
		if(ev){
	        ble_npl_event_run(ev);
		}
    }
	atbm_ThreadStopEvent(host_thread);
}

struct ble_npl_eventq *
nimble_port_get_dflt_eventq(void)
{
    return &g_eventq_dflt;
}


void nimble_port_atbmos_init(atbm_void(* host_task_fn))
{
	if(host_task_fn != NULL){
		nimble_th_exit = 0;
		host_thread = atbm_createThread(host_task_fn,(atbm_void*)ATBM_NULL, BLE_TASK_PRIO);
	}
}

void nimble_port_atbmos_free(void)
{
	struct ble_npl_event ev = {0};
	
	if(host_thread){
		nimble_th_exit = 1;
		ble_npl_eventq_put(&g_eventq_dflt, &ev);
		atbm_stopThread(host_thread);
		host_thread = NULL;
	}

	os_msys_deinit();
	ble_hci_ram_deinit();
}


