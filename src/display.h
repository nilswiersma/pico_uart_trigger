#ifndef DISPLAY_H_
#define DISPLAY_H_

#include "pico/stdlib.h"

void hexdump(void *ptr, uint len, uint col_width);

void print_settings(void);
void print_state(void);
void print_commands(void);

#endif