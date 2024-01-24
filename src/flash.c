
#include <string.h>

#include "pico/flash.h"

#include "flash.h"
#include "settings.h"
#include "globals.h"

extern char __flash_binary_start;
extern char __flash_persistent_start;

void __not_in_flash_func(cmd_write_settings_flash)(void* p) {
  uint32_t offset = &__flash_persistent_start - &__flash_binary_start;
  uint8_t page_buffer[FLASH_PAGE_SIZE];

  memcpy(page_buffer, &settings, sizeof(settings_t));

  // Minimum erase is FLASH_SECTOR_SIZE
  flash_range_erase(offset, FLASH_SECTOR_SIZE);
  // Minimum write is FLASH_PAGE_SIZE
  flash_range_program(offset, page_buffer, FLASH_PAGE_SIZE);
}