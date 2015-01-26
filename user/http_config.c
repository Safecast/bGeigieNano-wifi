#include <c_types.h>
#include <ip_addr.h>
#include <espconn.h>
#include "http_config.h"
#include "debug.h"
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "flash.h"


static struct espconn httpconfig_conn;
static esp_tcp httpconfig_tcp_conn;

char recv_buffer[1024]; // It's going to be /really/ easy to kill this thing.
int  recv_buffer_size = 0;

// We can only cope with 5 key/values
#define keyvalsize 5
char keys[keyvalsize][20];
char values[keyvalsize][128];

static int ICACHE_FLASH_ATTR parse_get(char *buffer) {

  char prn[1025];
  os_sprintf(prn,"pg: %s",buffer);
  debug(prn);

  //break out all components of first line
  int n;
  for(n=0;n<keyvalsize;n++) { keys[n][0]=0; values[n][0]=0; }

  // buffer looks like: GET /save?ssid=fish&pass=cake&apikey=lakes HTTP/1.0
  // followed by other header lines

  // chop buffer down to first line only
  for(n=0;buffer[n] != 0;n++) { 
    if(buffer[n] == '\n') {buffer[n]=0; break;}
    if(buffer[n] == '\r') {buffer[n]=0; break;}
  }
  os_sprintf(prn,"b1: %s",buffer);
  debug(prn);

  // start at first whitespace
  for(n=0;buffer[n] != 0;n++) {
    if(buffer[n] == ' ') {buffer = buffer+n; break;}
  }
  os_sprintf(prn,"b2: %s",buffer);
  debug(prn);

  // start at first ?
  int found=0;
  for(n=0;buffer[n] != 0;n++) {
    if(buffer[n] == '?') {buffer = buffer+n+1; found=1; break;}
  }
  if(found==0) {debug("failed to find"); return 0;}

  char *key;
  char *value;

  int c=0;
  buffer = strtok (buffer,"&=\n\r");
  for(;;) {
    if(c!=0) buffer = strtok (NULL,"&=\n\r ");
    if(buffer == NULL) break;
  os_sprintf(prn,"bA: %s",buffer);
  debug(prn);
    strcpy(keys[c],buffer);
    buffer = strtok (NULL,"&=\n\r ");
    if(buffer == NULL) break;
  os_sprintf(prn,"bB: %s",buffer);
  debug(prn);
    strcpy(values[c],buffer);
    c++;
    if(c>=keyvalsize) break;
  }
  return 1;
}

static int ICACHE_FLASH_ATTR find_value(const char *search_key,char *result) {
  // now search key/value pairs
  debug("findkey");
  result[0]=0;

  int n;
  char prn[100];
  for(n=0;n<keyvalsize;n++) {
    if(strcmp(keys[n],search_key) == 0) {
      os_sprintf(prn,"found: %s",values[n]);
      debug(prn);
      strcpy(result,values[n]);
      debug("found result");
      return 1;
    }
  }
}

static int ICACHE_FLASH_ATTR parse_header(char *data,int len,struct espconn *conn) {
 
  // Incoming data is URL encoded as follows:
  // http://192.168.0.10/save?ssid=fish&pass=cake&apikey=lakes

  // If ERASE is anywhere in the URL, erase all flash data
  if(strstr(data,"ERASE") != 0) {
    flash_erase_all();
    debug("received erase all");
    char transmission[65];
    transmission[0]=0;
    if(transmission[0] == 0) strcpy(transmission,"ERASE OK");
    sint8 d = espconn_sent(conn,transmission,strlen(transmission));
    espconn_disconnect(conn);
    return 1;
  }
 
  char buffer[1025];
  os_sprintf(buffer,"parse: %s",data);
  debug(buffer);


  parse_get(data); // will kill data buffer

  //Parse out settings
  char ssid[128];
  find_value("ssid",ssid);

  char password[128];
  find_value("pass",password);
  
  char apikey[128];
  find_value("apikey",apikey);

  // TODO: grab current values of "ssid" "pass" and "apikey"

  // We will only set all settings at once
  if((strlen(ssid    ) != 0) &&
     (strlen(password) != 0) &&
     (strlen(apikey)   != 0)) {
    
    debug("flashing settings");
    flash_key_value_set("ssid",ssid);
    flash_key_value_set("pass",password);
    flash_key_value_set("apikey",apikey);
    flash_key_value_set("mode","sta");
    char transmission[65];
    strcpy(transmission,"SET ALL OK");
    sint8 d = espconn_sent(conn,transmission,strlen(transmission));
    espconn_disconnect(conn);
  } else {
    char transmission[65];
    strcpy(transmission,"<h1>NEED ALL OF SSID/PASS/APIKEY</h1>");
    sint8 d = espconn_sent(conn,transmission,strlen(transmission));
    espconn_disconnect(conn);
  }
}

static void ICACHE_FLASH_ATTR httpconfig_recv_cb(void *arg, char *data, unsigned short len) {
  debug("httpconfig_recv cb");
  char buffer[1025];
  os_sprintf(buffer,"recv: %s",data);
  debug(buffer);

  int n;
  for(n=0;n<len;n++) {
    if(recv_buffer_size < 1023) {
      recv_buffer[recv_buffer_size  ] = *(data+n);
      recv_buffer[recv_buffer_size+1] = 0;
      recv_buffer_size++;
    }
  }

  struct espconn *conn=(struct espconn *)arg;

  int found = 0;
  for(n=0;n<recv_buffer_size;n++) if(recv_buffer[n] == '\n') found=1;

  if(found == 1) {
    int res = parse_header(recv_buffer,recv_buffer_size,conn);
  }
  debug("httpconfig_recv cb end");
}

static void ICACHE_FLASH_ATTR httpconfig_recon_cb(void *arg, sint8 err) {
}

static void ICACHE_FLASH_ATTR httpconfig_discon_cb(void *arg) {
}

static void ICACHE_FLASH_ATTR httpconfig_sent_cb(void *arg) {
}

static void ICACHE_FLASH_ATTR httpconfig_connected_cb(void *arg) {
  debug("httpconfig_connect cb");
  recv_buffer_size = 0;
  recv_buffer[0]=0;

  struct espconn *conn=arg;

  espconn_regist_recvcb  (conn, httpconfig_recv_cb);
  espconn_regist_reconcb (conn, httpconfig_recon_cb);
  espconn_regist_disconcb(conn, httpconfig_discon_cb);
  espconn_regist_sentcb  (conn, httpconfig_sent_cb);
  
  // TODO: grab current values of "ssid" "pass" and "apikey"
  //if(strstr(data,"GRABTEST")   != 0) {
  //  char transmission[65];
  //  transmission[0]=0;
  //  debug("received get");
  //  int res = flash_key_value_get("test",transmission);
  //  if(transmission[0] == 0) strcpy(transmission,"NOT FOUND");
  //  sint8 d = espconn_sent(conn,transmission,strlen(transmission));
  //  espconn_disconnect(conn);
  //}

  char *transmission = "HTTP/1.0 200\r\n\r\n<!DOCTYPE html>\r\n<html>\r\n<body>\r\n<form action=\"save\" method=\"get\"> SSID: <input type=\"text\" name=\"ssid\"><br> PASSWORD: <input type=\"text\" name=\"pass\"><br> SAFECAST APIKEY: <input type=\"text\" name=\"apikey\"><br> <input type=\"submit\" value=\"Submit\"> </form> </body> </html>";

  sint8 d = espconn_sent(conn,transmission,strlen(transmission));
  char buffer[20];
  os_sprintf(buffer,"send ret: %d",d);
  debug(buffer);

  debug("httpconfig_connect cb end");
}


void ICACHE_FLASH_ATTR httpconfig_init() {

	httpconfig_conn.type=ESPCONN_TCP;
	httpconfig_conn.state=ESPCONN_NONE;
	httpconfig_tcp_conn.local_port=80;
	httpconfig_conn.proto.tcp=&httpconfig_tcp_conn;

	espconn_regist_connectcb(&httpconfig_conn, httpconfig_connected_cb);
	espconn_accept(&httpconfig_conn);
}
