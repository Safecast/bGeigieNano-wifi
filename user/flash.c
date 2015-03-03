#include "flash.h"

ICACHE_FLASH_ATTR static char *find_key(const char *key,char *settings) {

  int n;
  for(n=0;n<1024;n+=128) {
    if(strncmp(key,settings+n,64) == 0) return settings+n;
  }
  return 0;
}

ICACHE_FLASH_ATTR static char *find_free(char *settings) {

  int n;
  for(n=0;n<1024;n+=128) {
    if(settings[n] == 0) return settings+n;
  }
  return 0;
}

ICACHE_FLASH_ATTR void flash_erase_all() {
  char settings[1024];

  int n;
  for(n=0;n<1024;n++) settings[n]=0;

  spi_flash_erase_sector(0x3C);
  spi_flash_write(0x3C000,settings,1024);
}


ICACHE_FLASH_ATTR int flash_key_value_set(const char *key,const char *value) {
  
  if(strlen(key)   > 64) return 1;
  if(strlen(value) > 64) return 1;

  char settings[1024];
  spi_flash_read(0x3C000, (uint32 *) settings, 1024);

  char *location = find_key(key,settings);
  if(location == 0) {
    location = find_free(settings);
  }

  if(location == 0) return 0;

  strcpy(location,key);
  strcpy(location+64,value);

  spi_flash_erase_sector(0x3C);
  spi_flash_write(0x3C000,settings,1024);

  return 1;
}

ICACHE_FLASH_ATTR int flash_key_value_get(char *key,char *value) {
  if(strlen(key)   > 64) return 0;
  if(strlen(value) > 64) return 0;

  char settings[1024];

  spi_flash_read(0x3C000, (uint32 *) settings, 1024);

  char *location = find_key(key,settings);

  if(location == 0) {
    value[0]=0;
    return 0;
  } else {
    strcpy(value,location+64);
  }
  return 1;
}
