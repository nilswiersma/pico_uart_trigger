#ifndef FLASH_H_
#define FLASH_H_

#include "pico/stdlib.h"

extern char __flash_binary_start;
extern char __flash_persistent_start;

void __not_in_flash_func(cmd_write_settings_flash)(void* p);

#endif