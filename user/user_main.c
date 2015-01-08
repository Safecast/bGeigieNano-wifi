#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"
#include "user_config.h"
#include "uart.h"

#include "c_types.h"
#include "espconn.h"
#include "mem.h"
#include "safecast.h"
#include "debug.h"


#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void loop(os_event_t *events);
LOCAL os_timer_t network_timer;

//Main code function
static void ICACHE_FLASH_ATTR loop(os_event_t *events) {
  os_delay_us(10000);
  system_os_post(user_procTaskPrio, 0, 0 );
/*
  int c = uart0_rx_one_char();

  if(c != -1) {
    char out[30];
    os_sprintf(out,"received %c",c);
    debug(out);
  }
*/
//  os_delay_us(100);
}

void ICACHE_FLASH_ATTR network_wait_for_ip() {

  struct ip_info ipconfig;
  os_timer_disarm(&network_timer);
  wifi_get_ip_info(STATION_IF, &ipconfig);
  if (wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0) {
    char page_buffer[40];
    os_sprintf(page_buffer,"myIP: %d.%d.%d.%d",IP2STR(&ipconfig.ip));
    debug(page_buffer);
    safecast_send_nema("$BNRDD,1010,2015-01-06T17:31:15Z,0,0,128,V,3537.2633,N,13938.0270,E,37.70,A,9,11160");
  } else {
    char page_buffer[40];
    os_sprintf(page_buffer,"network retry, status: %d",wifi_station_get_connect_status());
    if(wifi_station_get_connect_status() == 3) wifi_station_connect();
    debug(page_buffer);
    os_timer_setfn(&network_timer, (os_timer_func_t *)network_wait_for_ip, NULL);
    os_timer_arm(&network_timer, 2000, 0);
  }
}

void ICACHE_FLASH_ATTR network_init() {
  debug("init network 1");
  os_timer_disarm(&network_timer);
  os_timer_setfn(&network_timer, (os_timer_func_t *)network_wait_for_ip, NULL);
  os_timer_arm(&network_timer, 2000, 0);
  debug("init network 2");
}

//Init function 
void ICACHE_FLASH_ATTR user_init() {

    // Set UART Speed (default appears to be rather odd 77KBPS)
    uart_init(BIT_RATE_9600,BIT_RATE_9600);

    //wifi_config();
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
    debug("init wifi");
    debug(SSID);
    debug(SSID_PASSWORD);

    //Start os task
    system_os_task(loop, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);
    system_os_post(user_procTaskPrio, 0, 0 );

    network_init();
}
