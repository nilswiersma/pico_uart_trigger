

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "pico/time.h"
#include "pico/bootrom.h"
#include "pico/flash.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/uart.h"

#include "settings.h"
#include "globals.h"
#include "pins.h"

queue_t packet_queue;
int cur_pos;
uint rx_cnt;

void __time_critical_func(core1_main)()
{
  flash_safe_execute_core_init();

  uart_init(uart0, settings.baudrate);
  gpio_set_function(0, GPIO_FUNC_UART);
  gpio_set_function(1, GPIO_FUNC_UART);
  uart_puts(uart0, "Core1 alive!!\r\n");

  while (1) {
    if (uart_is_readable(uart0)) {
      // gpio_xor_mask(1<<LED_PIN);
      char ch = uart_getc(uart0);
      rx_cnt++;

      if (ch == settings.pattern[cur_pos] && cur_pos == settings.pat_len - 1) {
          gpio_xor_mask(1<<TRIGGER_OUT_PIN);
          sleep_us(1);
          gpio_xor_mask(1<<TRIGGER_OUT_PIN);
          cur_pos = 0;
      } else if (ch == settings.pattern[cur_pos]) {
        cur_pos++;
      } else {
        cur_pos = 0;
      }

      queue_add_blocking(&packet_queue, &ch); // Copy packet_pos and send to Core 0
    }
  }
}

