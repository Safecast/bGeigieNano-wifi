#include <string.h>

void debug(char *data) {
  uart0_tx_buffer(data,strlen(data));
}
