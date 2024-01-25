
#include <stdio.h>
#include <string.h>

#include "pico/bootrom.h"
#include "pico/flash.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/util/queue.h"

#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/uart.h"
#include "hardware/watchdog.h"

#include "core1.h"
#include "display.h"
#include "flash.h"
#include "globals.h"
#include "pins.h"
#include "settings.h"

#include "trigger_basic.pio.h"

PIO pio;
uint pio_sm;
uint pio_offset;

/**
 * @brief Send `len` bytes from the `packet_queue` over USB serial.
 * 
 * `packet_queue` gets filled by core1 (see core1.c). Use this function to empty `packet_queue`
 * to the host PC.
 * 
 * @param len 
 */
void queue_dump(uint len) {
  char ch;
  for (int i = 0; i < len; i++) {
    while (!uart_is_writable(uart0))
      tight_loop_contents();
    if (queue_try_remove(&packet_queue, &ch)) {
      putchar(ch);
    }
  }
}

/**
 * @brief Command handler for `d`, which sends stored bytes over USB serial.
 * 
 */
void cmd_dump(void) {
  uint len = queue_get_level(&packet_queue);
  if (len) {
    printf("%u:", len);
    queue_dump(len);
  }
}

/**
 * @brief Command handler to toggle sending bytes over USB serial as they are received.
 * 
 */

void cmd_stream_toggle(void) {
  settings.stream_toggle = !settings.stream_toggle;
}


/**
 * @brief Command handler to set the baudrate for the UART that is being sniffed.
 * 
 * This expects a number to be used as baudrate terminated by a null-byte, CTRL-D,
 * \\r or \\n. CTRL-C cancels the input.
 * 
 */
void cmd_set_baud(void) {
  int i;
  uint baudrate;
  char readbuf[128], c;
  memset(readbuf, 0, 128);
  for (i = 0; i < 127; i++) {
    int ret = getchar_timeout_us(5000000);
    if (ret == PICO_ERROR_TIMEOUT)
      break;
    c = ret & 0xff;
    if (c == '\x00' || c == '\x04' || c == '\r' || c == '\n')
      break;
    if (c == '\x03')
      return;
    readbuf[i] = c;
  }
  if (i < 127) {
    readbuf[i] = 0;
    if (sscanf(readbuf, "%u", &baudrate)) {
      settings.baudrate = baudrate;
      uart_init(uart0, settings.baudrate);
    }
  }
}

/**
 * @brief Command handler for configuring the UART pattern to trigger on.
 *
 * This expects a number as length, terminated by a null-byte, CTRL-D, \\r 
 * or \\n (or CTRL-C to cancel), followed by that many (raw) bytes to be used
 * as the pattern to trigger on.
 * 
 */
void cmd_set_pat(void) {
  int i;
  uint pat_len;
  char readbuf[128], c;
  
  memset(readbuf, 0, 128);
  for (i = 0; i < 127; i++) {
    int ret = getchar_timeout_us(5000000);
    if (ret == PICO_ERROR_TIMEOUT)
      break;
    c = ret & 0xff;
    if (c == '\x00' || c == '\x04' || c == '\r' || c == '\n')
      break;
    if (c == ' ' && i != 0)
      break;
    if (c == '\x03')
      return;
    readbuf[i] = c;
  }
  if (i < 127) {
    readbuf[i] = 0;
    if (sscanf(readbuf, "%u", &pat_len) != 1) {
      return;
    }
    if (pat_len > 128) {
        return;
    }
  }

  memset(readbuf, 0, 128);
  for (i = 0; i < pat_len; i++) {
    int ret = getchar_timeout_us(5000000);
    if (ret == PICO_ERROR_TIMEOUT)
      break;
    c = ret & 0xff;
    readbuf[i] = c;
  }
  if (i == pat_len) {
    settings.pat_len = pat_len;
    memcpy(settings.pattern, readbuf, pat_len);
  }
}

/**
 * @brief Handler for when the PIO SM finishes.
 * 
 * This RAM function is used as handler for when the PIO SM
 * that is used to generate a trigger pulse to an external device
 * (e.g. a ChipSHOUTER) finishes. It sends a single 'y' char over the 
 * USB serial to indicate it is done, and clears the interrupt so that
 * the PIO SM can continue with the next attempt.
 */
void __time_critical_func(pio_interrupt_handler)() {
    putchar('y');
    // Clear the PIO interrupt status
    pio_interrupt_clear(pio, 0);
}

/**
 * @brief Command handler for setting up the amount of wait and glitch cycles.
 *
 * Expects two numbers terminated by a null-byte, CTRL-D, \\r or \\n (or CTRL-C to 
 * cancel) each which will be used as the amount of clock cycles for the glitch 
 * delay and glitch duration. It is not translated to seconds, so check the configured
 * clock speed (250MHz at time of writing).
 *
 * Once the two numbers are received, the PIO SM used for generating this signal is 
 * started, which then in "armed" state, which means it is waiting for an edge on 
 * PIO_TRIGGER_IN_PIN (see pins.h).
 * 
 */
void cmd_glitch_configure_arm_and_wait(void) {
  int i;
  char readbuf[128], c;
  uint gd, gl;

  pio_sm_set_enabled(pio, pio_sm, false);
  pio_interrupt_clear(pio, 0);
  pio_sm_clear_fifos(pio, pio_sm);
  pio_sm_restart(pio, pio_sm);
  pio_sm_set_enabled(pio, pio_sm, true);
  
  memset(readbuf, 0, 128);
  for (i = 0; i < 127; i++) {
    int ret = getchar_timeout_us(5000000);
    if (ret == PICO_ERROR_TIMEOUT)
      break;
    c = ret & 0xff;
    if (c == '\x00' || c == '\x04' || c == '\r' || c == '\n')
      break;
    if (c == ' ' && i != 0)
      break;
    if (c == '\x03')
      return;
    readbuf[i] = c;
  }
  if (i < 127) {
    readbuf[i] = 0;
    if (sscanf(readbuf, "%d", &settings.glitch_delay) != 1)
      return;
  }

  memset(readbuf, 0, 128);
  for (i = 0; i < 127; i++) {
    int ret = getchar_timeout_us(5000000);
    if (ret == PICO_ERROR_TIMEOUT)
      break;
    c = ret & 0xff;
    if (c == '\x00' || c == '\x04' || c == '\r' || c == '\n')
      break;
    if (c == ' ' && i != 0)
      break;
    if (c == '\x03')
      return;
    readbuf[i] = c;
  }
  if (i < 127) {
    readbuf[i] = 0;
    if (sscanf(readbuf, "%d", &settings.glitch_length) != 1)
      return;
  }
  
  pio_sm_put_blocking(pio, pio_sm, settings.glitch_delay);  
  pio_sm_put_blocking(pio, pio_sm, settings.glitch_length);

  // pio_interrupt_handler will signal back when trigger has ocurred.
}

/**
 * @brief Read and process a single character command.
 * 
 */

void cmd_get_process() {
  int cmd = 0, ret = 0;
  // Check for commands
  cmd = getchar_timeout_us(1);
  if (cmd != PICO_ERROR_TIMEOUT) {
    switch (cmd & 0xff) {
      case 'i':
        gpio_set_mask(POWER_MASK);
        break;
      case 'o':
        gpio_clr_mask(POWER_MASK);
        break;
      case 'b':
        reset_usb_boot(LED_PIN, 0);
        break;
      case 'r':
        watchdog_enable(1, 1);
        while(1) {};
      case 'd':
        cmd_dump();
        break;
      case 't':
        cmd_stream_toggle();
        print_settings();
        break;
      case 'c':
        cmd_set_baud();
        print_settings();
        break;
      case 'p':
        cmd_set_pat();
        print_settings();
        break;
      case 'a':
        cmd_glitch_configure_arm_and_wait();
        break;
      case 'w':
        ret = flash_safe_execute(&cmd_write_settings_flash, NULL, 10000);
        break;
      case 's':
        print_state();
        break;
      case '\r':
      case '\n':
      case 'h':
        printf("\r\n----- PICO FI CONTROL -----\r\n");
        printf("\r\nControl your FI setup with an RP2040 board.\r\n");
        print_state();
        print_settings();
        print_commands();
        break;
    }
  }

}

/**
 * @brief Set up everything and enter command handler loop.
 * 
 * @return int 
 */
int main(void)
{
  int ret;

  if (!set_sys_clock_khz(250000, true))
    printf("[!] Error setting clock\r\n");

  stdio_usb_init();

  // Wait for connection
  while (!stdio_usb_connected())
    tight_loop_contents();

  stdio_set_translate_crlf(&stdio_usb, false);

  // Initialize GPIO for error indicating LED
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, true);
  gpio_put(LED_PIN, true);

  gpio_init(TRIGGER_OUT_PIN);
  gpio_set_dir(TRIGGER_OUT_PIN, GPIO_OUT);
  gpio_set_slew_rate(TRIGGER_OUT_PIN, GPIO_SLEW_RATE_FAST);
  gpio_set_pulls(TRIGGER_OUT_PIN, true, false);
  gpio_put(TRIGGER_OUT_PIN, 0);

  gpio_init(DEBUG_OUT_PIN);
  gpio_set_dir(DEBUG_OUT_PIN, true);
  gpio_set_slew_rate(DEBUG_OUT_PIN, GPIO_SLEW_RATE_FAST);
  gpio_put(DEBUG_OUT_PIN, 0);

  // Use two PINs, initialized to 12MA, to drive more amps
  gpio_init(POWER1_PIN);
  gpio_init(POWER2_PIN);
  gpio_set_dir(POWER1_PIN, GPIO_OUT);
  gpio_set_dir(POWER2_PIN, GPIO_OUT);
  // gpio_set_slew_rate(POWER1_PIN, GPIO_SLEW_RATE_FAST);
  // gpio_set_slew_rate(POWER2_PIN, GPIO_SLEW_RATE_FAST);
  gpio_set_drive_strength(POWER1_PIN, GPIO_DRIVE_STRENGTH_12MA);
  gpio_set_drive_strength(POWER2_PIN, GPIO_DRIVE_STRENGTH_12MA);
  gpio_clr_mask(POWER_MASK);

  queue_init(&packet_queue, 1, QUEUE_SIZE);

  settings_init();

  pio = pio0;
  pio_sm = pio_claim_unused_sm(pio, true);
  pio_offset = pio_add_program(pio, &trigger_basic_program);
  trigger_basic_init(pio, pio_sm, pio_offset, PIO_TRIGGER_IN_PIN, PIO_TRIGGER_OUT_PIN, PIO_DEBUG_OUT_PIN);

  pio_set_irq0_source_enabled(pio, pis_interrupt0, true);
  irq_set_exclusive_handler(PIO0_IRQ_0, pio_interrupt_handler);
  irq_set_enabled(PIO0_IRQ_0, true);

  multicore_launch_core1(core1_main); // Start core1_main on another core

  while (1) {
    cmd_get_process();
        
    // Check for error states
    if (queue_is_full(&packet_queue)) {
      printf("[!] UART queue is full!\r\n");
      sleep_ms(100);
    }

    if (settings.stream_toggle) {
      queue_dump(32);
      stdio_flush();
    }
  }
}
