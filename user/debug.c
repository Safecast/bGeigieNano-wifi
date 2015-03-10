#include <string.h>
#include "debug.h"
#include "ets_sys.h"

void ICACHE_FLASH_ATTR debug(char *data) {
  uart0_tx_buffer(data,strlen(data));
  uart0_tx_buffer("\r\n",2);
}
