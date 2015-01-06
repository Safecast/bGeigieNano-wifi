#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"
#include "uart.h"

#include "c_types.h"
#include "espconn.h"
#include "mem.h"
#include "safecast.h"
#include "debug.h"

#define SSID "YOURSSID"
#define SSID_PASSWORD "YOURPASSWORD"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void loop(os_event_t *events);

//Main code function
static void ICACHE_FLASH_ATTR
loop(os_event_t *events)
{
    os_delay_us(10000);
    system_os_post(user_procTaskPrio, 0, 0 );
}

void ICACHE_FLASH_ATTR network_init();

LOCAL os_timer_t network_timer;

void ICACHE_FLASH_ATTR network_wait_for_ip() {

  struct ip_info ipconfig;
  os_timer_disarm(&network_timer);
  wifi_get_ip_info(STATION_IF, &ipconfig);
  if (wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0) {
    char page_buffer[20];
    os_sprintf(page_buffer,"IP: %d.%d.%d.%d\n",IP2STR(&ipconfig.ip));
    uart0_tx_buffer(page_buffer,strlen(page_buffer));
    safecast_send_nema("$BNRDD,0000,2015-01-06T17:31:15Z,0,0,128,V,3537.2633,N,13938.0270,E,37.70,A,9,111*60");
  } else {
    os_timer_setfn(&network_timer, (os_timer_func_t *)network_wait_for_ip, NULL);
    os_timer_arm(&network_timer, 1000, 0);
  }
}

void ICACHE_FLASH_ATTR network_init() {
  os_timer_disarm(&network_timer);
  os_timer_setfn(&network_timer, (os_timer_func_t *)network_wait_for_ip, NULL);
  os_timer_arm(&network_timer, 1000, 0);
}

void ICACHE_FLASH_ATTR wifi_config() {
    // Wifi configuration
    char ssid[32] = SSID;
    char password[64] = SSID_PASSWORD;
    struct station_config stationConf;

    //Set station mode
    wifi_set_opmode( 0x1 );

    //Set ap settings
    os_memcpy(&stationConf.ssid, ssid, 32);
    os_memcpy(&stationConf.password, password, 64);
    wifi_station_set_config(&stationConf);
    uart0_tx_buffer("init",4);
}

//Init function 
void ICACHE_FLASH_ATTR user_init() {

    // Set UART Speed (default appears to be rather odd 77KBPS)
    uart_init(BIT_RATE_9600,BIT_RATE_9600);


    wifi_config();

    //Start os task
    system_os_task(loop, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);

    system_os_post(user_procTaskPrio, 0, 0 );

    network_init();
}
