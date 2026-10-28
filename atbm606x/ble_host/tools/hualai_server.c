#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <mcheck.h>


extern bool atbm_ble_is_quit;
extern int lib_ble_start;
int test_flag = 0;

void *test_thread = NULL;

int test_fun(void)
{
	test_flag = 1;
	printf("======>>test_fun>>>\n");
	while(test_flag){
		ble_npl_time_delay(ble_npl_time_ms_to_ticks32(10));
	}
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	atbm_stopThread(test_thread);
	return 0;
}


#if 0
int main(int argc, char *argv[])
{
	int i = 0;
	uint8_t *test_ptr = NULL;

	while(1){
		test_ptr = malloc(1024);
		memset(test_ptr, 0, 1024);
		for(i=0; i< 1024; i++){
			test_ptr[i] = i;
		}
		test_thread = atbm_createThread(test_fun, NULL, 5);
		printf("wait 10s 111:%d\n", test_ptr[25]);
		ble_npl_time_delay(ble_npl_time_ms_to_ticks32(5000));
		test_flag = 0;
		free(test_ptr);
		test_ptr = NULL;
		printf("wait 10s 222\n");
		ble_npl_time_delay(ble_npl_time_ms_to_ticks32(5000));
	}
}
#endif


#if 1
int main(int argc, char *argv[])
{
//	int i = 0;

//	mtrace();

	while(1){
//		lib_ble_reduce_mem_open();
		lib_ble_main_init();
		ble_npl_time_delay(ble_npl_time_ms_to_ticks32(2000));
		printf("atbm_ble_is_quit\n");
#if 0
		while(1){
			ble_npl_time_delay(ble_npl_time_ms_to_ticks32(2000));
		}
#endif
		lib_ble_thread_exit();
		printf("atbm_ble_loop\n");
		ble_npl_time_delay(ble_npl_time_ms_to_ticks32(2000));
		system("echo 1 > /proc/sys/vm/drop_caches");
		system("free");
		ble_npl_time_delay(ble_npl_time_ms_to_ticks32(1000));
	}

//	muntrace();

	return 0;
}
#endif

