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
#include "http_config.h"
#include "debug.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void loop(os_event_t *events);
LOCAL os_timer_t network_timer;


int line_buffer_pos = 0;
char line_buffer[256];
void ICACHE_FLASH_ATTR wifi_config_ap();

int c_mode = 0;

//Main code function
static void ICACHE_FLASH_ATTR loop(os_event_t *events) {

  int c = uart0_rx_one_char();

  if(c != -1) {
    if((c == '\r') || (c == '\n')) {
      line_buffer[line_buffer_pos]=0;
      
      if(strlen(line_buffer) > 10) {
        char nema_data[256];
        strcpy(nema_data,line_buffer);
        //REPLACE THIS LINE TO ENABLE PARSING/SENDING OF DATA
        //if(!safecast_sending_in_progress()) safecast_send_nema(nema_data);
      }
      line_buffer_pos = 0;
 
    } else {
      line_buffer[line_buffer_pos] = c;
      line_buffer_pos++;
      if(line_buffer_pos > 255) line_buffer_pos=0;
    }
  }


  int config = GPIO_INPUT_GET(0);
  if(config!=1) {
    if(c_mode != 1) {
      char buffer[30];
      os_sprintf(buffer,"config: %d",config);
      debug(buffer);
      wifi_config_ap();
      c_mode=1;
    }
  }

  os_delay_us(100);
  system_os_post(user_procTaskPrio, 0, 0 );
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

void ICACHE_FLASH_ATTR wifi_config_station() {
    // Wifi configuration

    #ifdef USE_HARDCODED_SSID
      char ssid[32] = SSID;
      char password[64] = SSID_PASSWORD;
    #endif

    #ifndef USE_HARDCODED_SSID
      char ssid[32];
      char password[64];
      int res = flash_key_value_get("ssid",ssid);
      res = flash_key_value_get("pass",password);
    #endif

    struct station_config stationConf;

    //Set station mode
    wifi_set_opmode( 0x1 );

    //Set ap settings
    stationConf.bssid_set = 0;
    os_memcpy(&stationConf.ssid, ssid, 32);
    os_memcpy(&stationConf.password, password, 64);

    wifi_station_set_config(&stationConf);
    debug("init wifi");
    debug(ssid);
    debug(password);
}

void ICACHE_FLASH_ATTR wifi_config_ap() {
  wifi_station_disconnect();
//  wifi_set_opmode(0x3); //reset to AP+STA mode
  flash_key_value_set("mode","ap");
  system_restart();
}

//Init function 
void ICACHE_FLASH_ATTR user_init() {

    // Set UART Speed (default appears to be rather odd 77KBPS)
    uart_init(BIT_RATE_9600,BIT_RATE_9600);
    os_delay_us(10000);
    debug("System init");
    debug("System init");
    debug("System init");

    char apikey[128];
    int res = flash_key_value_get("apikey",apikey);
    debug(apikey);

    gpio_init();
    // check GPIO setting (for config mode selection)
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
    PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO0_U);

    char mode[128];
    mode[0]=0;
    res = flash_key_value_get("mode",mode);

    char buffer[50];
    os_sprintf(buffer,"mode: %s",mode);
    debug(buffer);

    if(strcmp(mode,"sta") == 0) {
      debug("Booting in station mode");
      wifi_config_station();
      network_init();  // only require for station mode
    } else {
      debug("Booting in AP mode");
      struct station_config stationConf;
      stationConf.bssid_set = 0;
      os_memcpy(&stationConf.ssid, "", 32);
      os_memcpy(&stationConf.password, "", 64);
      wifi_station_set_config(&stationConf);
      wifi_set_opmode(0x3);
      flash_key_value_set("mode","sta");
    }

    httpconfig_init();

    //Start os task
    system_os_task(loop, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);
    system_os_post(user_procTaskPrio, 0, 0 );

    debug("System init complete");
}
