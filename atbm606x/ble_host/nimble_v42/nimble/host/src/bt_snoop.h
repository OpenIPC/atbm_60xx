#ifndef BTSNOOP_H_H_H
#define BTSNOOP_H_H_H


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <sys/time.h>
#include <string.h>



//typedef char 		int8_t;
//typedef short 		int16_t;
//typedef int 		int32_t;
//typedef long 		int64_t;

typedef unsigned char 		uint8_t;
typedef unsigned short 		uint16_t;
typedef unsigned int 		uint32_t;
//typedef unsigned long	uint64;

#define TRANSPORT_TYPE_CMD		1
#define TRANSPORT_TYPE_ACL		2
#define TRANSPORT_TYPE_EVT		4

uint8_t btsnoop_open(uint8_t *file_name);
uint8_t btsnoop_close(void);
uint8_t btsnoop_write(uint8_t type,uint8_t in,uint8_t *data,uint16_t data_len);


#endif






