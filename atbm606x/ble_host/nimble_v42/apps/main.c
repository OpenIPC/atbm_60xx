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

#include "ble_hci_ram.h"
#include "nimble/nimble_port.h"
#include "syscfg/syscfg.h"
#include "host/ble_hs.h"
//#include "atbm_nimble_api.h"
#include "bt_snoop.h"
extern int atbm_ble_exit(void);

extern void nimble_host_task(void *param);
extern int btshell_entry(void);
extern void ble_smart_gatt_svcs_init(void);
extern void nimble_port_init(void);
extern void nimble_port_atbmos_init(atbm_void(* host_task_fn));
extern void cli_init(void);
extern bool g_is_quit;

void cli_free(void);
void nimble_port_atbmos_free(void);
void ble_hci_sync_init(void);
int ble_hci_sync_get(void);
int lib_ble_reduce_mem = 0;
extern int open_atbm_ioctl(void);
extern int ble_hs_startup_read_local_ver_tx(void);
pAtbm_thread_t ble_ioclt_monitor_thread = NULL;


int ble_ioclt_monitor_task(void)
{
	int rc=0,num;
	while((rc==0)&&(g_is_quit==0))
	{
		if(num==110)
		{
			num=0;
			rc=ble_hs_startup_read_local_ver_tx();
			if(rc)
			{
				iot_printf("======>>error ioclt >>>\n");
			}
		}
		num++;
		ble_npl_time_delay(ble_npl_time_ms_to_ticks32(100));
	}
	iot_printf("atbm_stopThread(ble_ioclt_monitor_thread)\n");
	return 0;
}


void ble_gatt_svcs_init(void)
{
#if MYNEWT_VAL_SHELL_CMD
	btshell_entry();
#endif
}


int nimble_main(void)
{
#ifdef BLE_MESH_OPEN
	nimble_port_init();
	nimble_port_atbmos_init(NULL);
#if MYNEWT_VAL_BLE_MESH_SHELL
	ble_mesh_shell_init();
#else
	btMesh_cmd_init();
	ble_mesh_demo();
#endif //MYNEWT_VAL_BLE_MESH_SHELL

#else	//BLE_MESH_OPEN
	nimble_port_init();
	ble_gatt_svcs_init();

	nimble_port_atbmos_init(nimble_host_task);
#endif //BLE_MESH_OPEN
	cli_init();

	return 0;
}

void nimble_release(void)
{
	cli_free();
	nimble_port_atbmos_free();
	nimble_port_release();
	ble_gap_free();
	ble_hs_hci_free();
	ble_hs_free();
}
char connect_ap[64];


#ifndef CONFIG_LINUX_BLE_STACK_LIB
int main(int argc,char * argv[])
//int atbm_ble_start(void)
{
	int ret;
	if(argc > 1 && argv[1]){
		memcpy(connect_ap,argv[1],strlen(argv[1]));
		printf("connect_ap file: %s ,%s,%d\n",connect_ap,argv[1],strlen(argv[1]));
	}
	iot_printf("======>>atbm_ble_start>>>\n");
	ret = open_atbm_ioctl();
	if(ret)
		return -1;
	//btsnoop_open("hci_btsnoop.log");
	nimble_main();
	//atbm_startconfig cmd
	atcmd_init_ble();
	//
	hif_ioctl_init();
	//
	ble_hs_sched_start();
	iot_printf("hif_ioctl_loop\n");
	//ble_ioclt_monitor_thread = atbm_createThread(ble_ioclt_monitor_task, (atbm_void*)ATBM_NULL, BLE_APP_PRIO);
	//start ioctl to wifi driver
	hif_ioctl_loop();
	iot_printf("ble_hs_hci_cmd_reset\n");
	nimble_release();
	//reset LL BLE when quit
	//ble_hs_hci_cmd_reset();
	ble_smart_cfg_exit();
	//btsnoop_close();
	//atbm_stopThread(ble_ioclt_monitor_thread);
	return 0;
}
#endif


int lib_ble_start = 0;
pAtbm_thread_t lib_ble_thread = NULL;







int lib_ble_main(void* param)
{
	int ret;
	ret = open_atbm_ioctl();
	if(ret)
		return -1;
	lib_ble_start = 1;
	iot_printf("======>>atbm_ble_start>>>\n");
	//btsnoop_open("hci_btsnoop.log");
	ble_hci_sync_init();
	nimble_main();
	//atbm_startconfig cmd
	atcmd_init_ble();
	//
	hif_ioctl_init();
	//
	ble_hs_sched_start();
	iot_printf("hif_ioctl_loop\n");
	//ble_ioclt_monitor_thread = atbm_createThread(ble_ioclt_monitor_task, (atbm_void*)ATBM_NULL, BLE_APP_PRIO);
	//start ioctl to wifi driver
	hif_ioctl_loop();
	//atbm_ble_exit();
	iot_printf("ble_hs_hci_cmd_reset\n");
	nimble_release();
	//btsnoop_close();
	//atbm_stopThread(ble_ioclt_monitor_thread);
	lib_ble_start = 0;
	
	return 0;
}

extern bool atbm_ble_is_quit;
void lib_ble_thread_exit()
{
	atbm_ble_is_quit = 1;
	while(lib_ble_start){
		ble_npl_time_delay(ble_npl_time_ms_to_ticks32(1000));
	}
	ble_smart_cfg_exit();
	atbm_stopThread(lib_ble_thread);
}


void lib_ble_reduce_mem_open(void)
{
	lib_ble_reduce_mem = 1;
}

void lib_ble_main_init(void)
{
	if(lib_ble_start){
		iot_printf("error lib_ble_main_init already!\n");
		return;
	}
	ble_hci_sync_init();
	iot_printf("lib_ble_main_init\n");
	lib_ble_thread = atbm_createThread(lib_ble_main, (atbm_void*)ATBM_NULL, BLE_APP_PRIO);

	while(0 == ble_hci_sync_get()){
		ble_npl_time_delay(ble_npl_time_ms_to_ticks32(10));
	}
}
