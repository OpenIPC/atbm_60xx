
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "btgatt_client.h"


#if 1

#define OBJ_DEVICE_NAME    "wyze-lamp"
#define SCAN_TIME_SEC      (5)

#define LAMP_SERVE_UUID    "0000ffe0-0000-1000-8000-00805f9b34fb"
#define LAMP_WRITE_UUID    "0000ffe1-0000-1000-8000-00805f9b34fb"
#define LAMP_READ_UUID     "0000ffe4-0000-1000-8000-00805f9b34fb"

static void test_disconnect_fn(int32_t errCode, void *user_data)
{
	printf("test_disconnect_fn:%d\n", errCode);
}

static void test_notify_fn(uint16_t value_hanlde, const uint8_t *value, uint16_t length)
{
	int i;
	
	printf("notify rx, val_handle(%d):", value_hanlde);
	for(i=0; i<length; i++){
		printf("%02X ", value[i]);
	}
	printf("\n");
}

int main(int argc, char *argv[])
{
	 int i = 0, num = 0;
	 int rc, fd;
	 size_t scan_cnt;
	 struct advertising_devices *scan_result = NULL, *scan = NULL;
	 struct advertising_devices device;
	 uint16_t notify_value_handle, notify_desc_handle, write_value_handle;
	 uint8_t *value = NULL;
	 uint16_t value_len;
	 uint8_t data[] = {0xAA, 0x55, 0x43, 0x03, 0x27, 0x01, 0x6C};
	 uint8_t data1[] = {0xAA, 0x55, 0x43, 0x13, 0x02, 0x76, 0x76, 0x4B, 0x31, 0x71, 0x73, 0x68,
						0x46, 0x5A, 0x35, 0x33, 0x59, 0x6C, 0x68, 0x70, 0x64, 0x07, 0x14};

	 hlgatt_client_init();

#if 1	
	 hlgatt_client_get_advertising_devices(&scan_result, &scan_cnt, NULL, 5);
	 printf("test scan cnt:%d\n", scan_cnt);
	 for(i=0; i<scan_cnt; i++){
		 scan = (scan_result + i);
		 printf("scan[%d], rssi:%d, name:%s, addr_type:%d, addr:%s\n", i, scan->rssi, scan->name, scan->addr_type, scan->addr);
	 }
	 printf("test scan end\n");
	 free(scan_result);
#endif

	 hlgatt_client_get_advertising_devices(&scan_result, &scan_cnt, OBJ_DEVICE_NAME, SCAN_TIME_SEC);

	 printf("test scan cnt:%d\n", scan_cnt);
	 for (i = 0; i < scan_cnt; i++) {
	  scan = (scan_result + i);
	  printf("scan[%d], rssi:%d, name:%s, addr_type:%d, addr:%s\n", i, scan->rssi, scan->name, scan->addr_type, scan->addr);
#if 0
	  num = (scan->rssi > (scan_result + num)->rssi) ? i : num;
	  if (i + 1 == scan_cnt) {
	   memcpy(&device, scan_result + num, sizeof(struct advertising_devices));
	  }
#endif
	 }
	 free(scan_result);
//	 printf("device[%d], rssi:%d, name:%s, addr_type:%d, addr:%s\n", i, device.rssi, device.name, device.addr_type, device.addr);

	 /////////////////////////////////////////////////////////////
	 // 该部分代码作调试，因为灯口有问题，扫不到name
	 device.addr_type = 0;
	 memset(device.addr, 0x00, sizeof(device.addr));
//	 memcpy(device.addr, "84:C2:E4:C2:C5:26", sizeof("84:C2:E4:C2:C5:26"));
	 memcpy(device.addr, "84:C2:E4:C2:C3:B6", sizeof("84:C2:E4:C2:C3:B6"));
	 printf("device[%d], rssi:%d, name:%s, addr_type:%d, addr:%s\n", i, device.rssi, device.name, device.addr_type, device.addr);
	 /////////////////////////////////////////////////////////////

	 fd = hlgatt_client_start(device.addr_type, device.addr, test_disconnect_fn);
	 if((fd <= 0) || (fd > 4)){
	  printf("hlgatt_client_start error(%d)\n", fd);
	  return -1;
	 }
	 
	 hlgatt_client_register_notify_eigenvalue_callback(fd, test_notify_fn);
	 
#if 0
	 rc = hlgatt_client_get_eigenvalue_handle(fd, "0xFFE0", "0xFFE4", &read_value_handle);
	 if(rc != 0){
	  printf("hlgatt_client_get_eigenvalue_handle error(%d)\n", rc);
	  return -1;
	 }
	 printf("read value handle is [%d]\n", read_value_handle);


	 rc = hlgatt_client_get_eigenvalue_handle(fd, "0xFFE0", "0xFFE1", &read_value_handle);
	 if(rc != 0){
	  printf("hlgatt_client_get_eigenvalue_handle error(%d)\n", rc);
	  return -1;
	 }
	 printf("read value handle is [%d]\n", read_value_handle);
#endif

	 rc = hlgatt_client_get_eigenvalue_handle(fd, (uint8_t *)LAMP_SERVE_UUID, (uint8_t *)LAMP_READ_UUID, &notify_value_handle, &notify_desc_handle);
	 if(rc != 0){
	  printf("hlgatt_client_get_eigenvalue_handle error(%d)\n", rc);
	  return -1;
	 }
	 printf("notify value handle is [%d], notify_desc_handle[%d]\n", notify_value_handle, notify_desc_handle);	 

	 rc = hlgatt_client_get_eigenvalue_handle(fd, (uint8_t *)LAMP_SERVE_UUID, (uint8_t *)LAMP_WRITE_UUID, &write_value_handle, NULL);
	 if(rc != 0){
	  printf("hlgatt_client_get_eigenvalue_handle error(%d)\n", rc);
	  return -1;
	 }
	 printf("write value handle is [%d]\n", write_value_handle);


	 rc = hlgatt_client_notify_on(fd, notify_desc_handle, 1);
	 if(rc != 0){
	  printf("hlgatt_client_get_eigenvalue_handle error(%d)\n", rc);
	  return -1;
	 }

	 rc = hlgatt_client_write_value(fd, write_value_handle, data, sizeof(data));
	 if(rc != 0){
	  printf("hlgatt_client_write_value error(%d)\n", rc);
	  return -1;
	 }

	 sleep(1);

	 rc = hlgatt_client_write_value(fd, write_value_handle, data1, sizeof(data1));
	 if(rc != 0){
	  printf("hlgatt_client_write_value error(%d)\n", rc);
	  return -1;
	 }
	 
	 sleep(1);

#if 0	 
	 for (i = 0; i < 5; i++) { 
	  rc = hlgatt_client_write_value(fd, write_value_handle, data, sizeof(data));
	  if(rc != 0){
	   printf("hlgatt_client_write_value error(%d)\n", rc);
	   return -1;
	  }
	  sleep(1);
	 }

	 rc = hlgatt_client_notify_on(fd, notify_desc_handle, 0);
	 if(rc != 0){
	  printf("hlgatt_client_get_eigenvalue_handle error(%d)\n", rc);
	  return -1;
	 }

	 for (i = 0; i < 5; i++) { 
	  rc = hlgatt_client_write_value(fd, write_value_handle, data, sizeof(data));
	  if(rc != 0){
	   printf("hlgatt_client_write_value error(%d)\n", rc);
	   return -1;
	  }
	  
//	  rc = hlgatt_client_read_value(fd, read_value_handle, &value, &value_len);
//	  if(rc != 0){
//	   printf("hlgatt_client_read_value error(%d)\n", rc);
//	   return -1;
//	  }
	  
//	  if(value && value_len){
//	   printf("client read(%d):%s\n", value_len, value);
//	   free(value);
//	  }

	  sleep(1);
	 }
#endif
	 
	 hlgatt_client_end(fd);
	 hlgatt_client_deinit();

	 return EXIT_SUCCESS;
}

#else

static void test_disconnect_fn(int32_t errCode, void *user_data)
{
	printf("test_disconnect_fn:%d\n", errCode);
}

int main(int argc, char *argv[])
{
	int i = 0;
	int rc,fd;
	struct advertising_devices *scan_result;
	struct advertising_devices *scan;
	int scan_cnt = 0;
	uint16_t value_handle;
	uint8_t *value;
	uint16_t value_len;
	uint8_t data[100];

	printf("ble_client_cfg_task start\n");
	hlgatt_client_init();

#if 1	
	hlgatt_client_get_advertising_devices(&scan_result, &scan_cnt, NULL, 5);
	printf("test scan cnt:%d\n", scan_cnt);
	for(i=0; i<scan_cnt; i++){
		scan = (scan_result + i);
		printf("scan[%d], rssi:%d, name:%s, addr_type:%d, addr:%s\n", i, scan->rssi, scan->name, scan->addr_type, scan->addr);
	}
	printf("test scan end\n");
	free(scan_result);
#endif

	hlgatt_client_get_advertising_devices(&scan_result, &scan_cnt, "k40-mu", 3);
	printf("test scan name cnt:%d\n", scan_cnt);
	if(scan_cnt != 1){
		printf("scan cnt error\n");
		return -1;
	}
	
	fd = hlgatt_client_start(scan_result->addr_type, scan_result->addr, test_disconnect_fn);
	free(scan_result);
	if((fd <= 0) || (fd > 4)){
		printf("hlgatt_client_start error(%d)\n", fd);
		return -1;
	}
	
	rc = hlgatt_client_get_eigenvalue_handle(fd, "0x181C", "0x2A8A", &value_handle, NULL);
	if(rc != 0){
		printf("hlgatt_client_get_eigenvalue_handle error(%d)\n", rc);
		return -1;
	}
	
	rc = hlgatt_client_write_value(fd, value_handle, "test123", strlen("test123"));
	if(rc != 0){
		printf("hlgatt_client_write_value error(%d)\n", rc);
		return -1;
	}

	rc = hlgatt_client_read_value(fd, value_handle, &value, &value_len);
	if(rc != 0){
		printf("hlgatt_client_read_value error(%d)\n", rc);
		return -1;
	}

	if(value && value_len){
		printf("client read(%d):%s\n", value_len, value);
		free(value);
	}

	rc = hlgatt_client_get_eigenvalue_handle(fd, "0000aaa0-0000-1000-8000-aabbccddeeff", "0000aaa1-0000-1000-8000-aabbccddeeff", &value_handle, NULL);
	if(rc != 0){
		printf("hlgatt_client_get_eigenvalue_handle error(%d)\n", rc);
		return -1;
	}

	rc = hlgatt_client_read_value(fd, value_handle, &value, &value_len);
	if(rc != 0){
		printf("hlgatt_client_read_value error(%d)\n", rc);
		return -1;
	}
	
	if(value && value_len){
		printf("client read(%d):%s\n", value_len, value);
		free(value);
	}
	ble_npl_time_delay(ble_npl_time_ms_to_ticks32(1000));

	for(i=0; i<10; i++){
		
		memset(data, 0, 100);
		sprintf(data, "test_send:%d", i);
	
		rc = hlgatt_client_write_value(fd, value_handle, data, strlen(data));
		if(rc != 0){
			printf("hlgatt_client_write_value error(%d)\n", rc);
			return -1;
		}

		rc = hlgatt_client_read_value(fd, value_handle, &value, &value_len);
		if(rc != 0){
			printf("hlgatt_client_read_value error(%d)\n", rc);
			return -1;
		}
		
		if(value && value_len){
			printf("client read(%d):%s\n", value_len, value);
			free(value);
		}

		ble_npl_time_delay(ble_npl_time_ms_to_ticks32(1000));
	}
	
	hlgatt_client_end(fd);
	hlgatt_client_deinit();
	
	sleep(1);
	printf("ble_client_cfg_task end\n");	
	return 0;
}

#endif

