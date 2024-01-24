#ifndef PINS_H_
#define PINS_H_

#include "pico/stdlib.h"

#define LED_PIN PICO_DEFAULT_LED_PIN
#define TRIGGER_OUT_PIN 2
#define DEBUG_OUT_PIN 3

#define POWER1_PIN 14
#define POWER2_PIN 15
#define POWER_MASK ((1 << POWER1_PIN) | (1 << POWER2_PIN))

#define PIO_TRIGGER_IN_PIN 16
#define PIO_TRIGGER_OUT_PIN 17
#define PIO_DEBUG_OUT_PIN 18

#endif