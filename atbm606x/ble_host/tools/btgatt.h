#ifndef __BTGATT_H
#define __BTGATT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef int (*send_func_t)(uint8_t *send_str);
typedef void (*receive_func_t)(uint8_t *rec_str, size_t len);
typedef void (*disconnect_func_t)(int errCode);

int bluetooth_connect_broadcast_start(char *name, char *buff, int size, unsigned int timeout);
void bluetooth_connect_broadcast_stop(void);
void bluetooth_connect_callback_set(receive_func_t rec_func, send_func_t send_func);
void bluetooth_disconnect_callback_set(disconnect_func_t disconnect_func);

void bluetooth_broadcast_start(char *name, char *broadcastBody, size_t broadcastLen, uint16_t interval);
void bluetooth_broadcast_stop(void);

int bluetooth_message_send(uint8_t *sendMessage, uint16_t sendSize);
void bluetooth_hci_init(); // blue init called once

#ifdef __cplusplus
}
#endif

#endif /* __BTGATT_H */
