#include <string.h>

#include "globals.h"
#include "settings.h"

extern char __flash_binary_start;
extern char __flash_persistent_start;

settings_t settings;

void settings_init(void) {
  // Read from flash, if possible
  uint8_t *persistent_base = (uint8_t *) &__flash_persistent_start;

  uint32_t magic = *((uint32_t*) persistent_base);
  if (magic == 0x41424344) {
    memcpy(&settings, persistent_base, sizeof(settings_t));
  } else {
    settings.magic = 0x41424344;
    settings.uart = 0;
    settings.baudrate = 115200;
    strcpy(settings.pattern, "test");
    settings.pat_len = 4;
  }
}