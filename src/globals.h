#ifndef GLOBALS_H_
#define GLOBALS_H_

#include "pico/util/queue.h"

#include "settings.h"
#include "hardware/pio.h"

#define QUEUE_SIZE 8192

extern uint DEBUG;

extern queue_t packet_queue;
extern int cur_pos;
extern uint rx_cnt;

extern PIO pio;
extern uint pio_sm;
extern uint pio_offset;

extern settings_t settings;

#endif