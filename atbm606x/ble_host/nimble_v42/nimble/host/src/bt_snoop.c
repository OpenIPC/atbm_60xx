#include "bt_snoop.h"
#include <time.h>


#define BT_SNOOP_PATH "./"

int btsnoop_fd = -1;
uint64_t btsnoop_timer=0;

static const uint64_t BTSNOOP_EPOCH_DELTA = 0x00dcddb30f2f8000ULL;

void be_store_32(uint8_t *buffer,uint32_t value)
{
	uint8_t index = 0;
	
	buffer[index++] = (uint8_t)(value >> 24);
	buffer[index++] = (uint8_t)(value >> 16);
	buffer[index++] = (uint8_t)(value >> 8);
	buffer[index++] = (uint8_t)(value);
}

void be_store_64(uint8_t *buffer,uint64_t value)
{
	uint8_t index = 0;
	
	buffer[index++] = (uint8_t)(value >> 56);
	buffer[index++] = (uint8_t)(value >> 48);
	buffer[index++] = (uint8_t)(value >> 40);
	buffer[index++] = (uint8_t)(value >> 32);
	buffer[index++] = (uint8_t)(value >> 24);
	buffer[index++] = (uint8_t)(value >> 16);
	buffer[index++] = (uint8_t)(value >> 8);
	buffer[index++] = (uint8_t)(value);
} 

uint8_t btsnoop_open(uint8_t *file_name)
{
#if 0
    time_t t;
    struct tm *p;

    time(&t);
    p = gmtime(&t);
    uint8_t bt_snoop_name[64] = {0};

    sprintf(bt_snoop_name,"%sbtsnoop_%04d_%02d_%02d_%02d_%02d_%02d.log",BT_SNOOP_PATH,1900 + p->tm_year,1 + p->tm_mon, \
            p->tm_mday,8 + p->tm_hour,p->tm_min,p->tm_sec);/* time zone hour+8 */


    btsnoop_fd = open(bt_snoop_name, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	printf("bt snoop fd %d\n",(int)btsnoop_fd);

    write(btsnoop_fd, "btsnoop\0\0\0\0\1\0\0\x3\xea", 16);

    return 0;

#else	
	btsnoop_timer=0;
	if(!file_name)
	{
		printf("btsnoop_open file_name is NULL !error\r\n");
		return 1;
	}

	printf("file_name is %s\r\n",file_name);
	btsnoop_fd = open(file_name, O_RDWR | O_CREAT, S_IRWXU | S_IRGRP | S_IROTH|O_TRUNC);  
	if (btsnoop_fd == -1) 
	{  
		printf("btsnoop_open open error\r\n");  
		return 1;  
	}  

	write(btsnoop_fd, "btsnoop\0\0\0\0\1\0\0\x3\xea", 16);
	
	return 0;
	
#endif
		
}

uint8_t btsnoop_close(void)
{
	if(-1 != btsnoop_fd)
	{
		close(btsnoop_fd);
		btsnoop_fd = -1;
	}	
}

uint8_t btsnoop_write(uint8_t type,uint8_t in,uint8_t *data,uint16_t data_len)
{
	uint32_t orig_len = 0;
	uint32_t include_len = 0;
	uint32_t flags = 0;
	uint32_t drops = 0;
	uint64_t timestamp = 0;
	uint64_t tv_usec;
	if(btsnoop_fd != -1)
	{
		if(TRANSPORT_TYPE_CMD == type)
		{
			flags = 2;
			printf("btsnoop -->> HCI LE ");
		}
		else if(TRANSPORT_TYPE_ACL == type)
		{

			flags = in;
			if(in)
				printf("btsnoop <<-- HCI ATT ");
			else
				printf("btsnoop -->> HCI ATT ");
		}
		else if(TRANSPORT_TYPE_EVT == type)
		{
			flags = 3;
			printf("btsnoop <<-- HCI Command Complete ");
		}	
#if 0
		struct timeval curr_time;
		gettimeofday(&curr_time,NULL);
		tv_usec = (curr_time.tv_sec+8*60*60)*1000000ULL+curr_time.tv_usec+BTSNOOP_EPOCH_DELTA;/* time zone hour+8 */	
#else
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		tv_usec=ts.tv_sec*1000000;
		//tv_usec=tv_usec*1000;
		tv_usec=tv_usec+ts.tv_nsec/1000;
		if(!btsnoop_timer)
			btsnoop_timer=tv_usec;
		//printf("btsnoop_timer tv_usec=%d tv_msec=%d\r\n", ts.tv_sec,ts.tv_nsec/1000000);
		printf(" tv_usec=%llu ms\r\n", (tv_usec-btsnoop_timer)/1000);
		
		//printf("‌flags=%d  ts.tv_sec=%d  ts.tv_nsec=%d\r\n", flags,ts.tv_sec,ts.tv_nsec);
#endif
	// 现在，‌ts.tv_sec和ts.tv_nsec包含了从系统启动以来的秒数和纳秒数
		be_store_32((uint8_t *)&orig_len,data_len);
		//printf("orig_len = %x,data_len = %x\r\n",orig_len,data_len);
		
		be_store_32((uint8_t *)&include_len,data_len);
		//printf("include_len = %x,data_len = %x\r\n",include_len,data_len);
		
		be_store_32((uint8_t *)&flags,flags);
		//printf("flags = %x,flags = %x\r\n",flags,flags);
		
		//be_store_32((uint8_t *)&drops,0);
		//printf("drops = %x,drops = %x\r\n",drops,drops);
		
		be_store_64((uint8_t *)&timestamp,tv_usec);
		//printf("timestamp = %lx,tv_usec = %lx\r\n",timestamp,tv_usec);
		
		
		write(btsnoop_fd,&orig_len,sizeof(orig_len));
		write(btsnoop_fd,&include_len,sizeof(include_len));
		write(btsnoop_fd,&flags,sizeof(flags));
		write(btsnoop_fd,&drops,sizeof(drops));
		write(btsnoop_fd,&timestamp,sizeof(timestamp));
		write(btsnoop_fd,data,data_len);
		fdatasync(btsnoop_fd);
	}
	return 0;
	
}












