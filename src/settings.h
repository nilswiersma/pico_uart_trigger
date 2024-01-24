#ifndef SETTINGS_H_
#define SETTINGS_H_

#include "pico/stdlib.h"

typedef struct {
  uint magic;
  bool stream_toggle;
  uint uart;
  uint pin_trigger_out;
  uint baudrate;
  char pattern[128];
  uint pat_len;
  uint glitch_delay;
  uint glitch_length;
} settings_t; 

void settings_init(void);

#endif