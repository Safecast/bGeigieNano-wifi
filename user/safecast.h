#ifndef SAFECAST_SEND_H
#define SAFECAST_SEND_H

static void ICACHE_FLASH_ATTR safecast_connected_cb(void *arg);
static void ICACHE_FLASH_ATTR safecast_disconnected_cb(void *arg);
static void ICACHE_FLASH_ATTR safecast_reconnected_cb(void *arg, sint8 err);
static void ICACHE_FLASH_ATTR safecast_recv_cb(void *arg, char *data, unsigned short len);
static void ICACHE_FLASH_ATTR safecast_sent_cb(void *arg);
void ICACHE_FLASH_ATTR safecast_send_data();

#endif
