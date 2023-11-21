
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

#define LED_PIN PICO_DEFAULT_LED_PIN
#define TRIGGER_OUT_PIN 2

#define QUEUE_SIZE 8192

extern char __flash_binary_start;
extern char __flash_persistent_start;

queue_t packet_queue;

int cur_pos = 0;
uint rx_cnt = 0;

void hexdump(void *ptr, uint len, uint col_width) {
  if (col_width == 0)
    col_width = 0x10;

  for (int i=0; i < len; i += 0x10) {
    printf("0x%08x: ", ptr+i);
    for (int j=0; j < col_width; j++) {
      if (i+j < len) {
        printf("%02x ", *((uint8_t*)(ptr+i+j)));
      } else {
        printf("   ");
      }
    }
    printf(" ");
    for (int j=0; j < col_width; j++) {
      if (i+j < len) {
        if (*((char*)(ptr+i+j)) >= 0x20 && *((char*)(ptr+i+j)) <= 0x7e) {
          printf("%c ", *((char*)(ptr+i+j)));
        } else {
          printf(". ");
        }
      } else {
        printf("   ");
      }
    }
    printf("\r\n");
  }
}

typedef struct {
  uint magic;
  bool stream_toggle;
  uint uart;
  uint pin_trigger_out;
  uint baudrate;
  char pattern[128];
  uint pat_len;
} settings_t; 

settings_t settings;

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

void settings_init(void) {
  // Read from flash, if possible
  uint8_t *persistent_base = (uint8_t *) &__flash_persistent_start;

  uint32_t magic = *((uint32_t*) persistent_base);
  if (magic == 0x41424344) {
    memcpy(&settings, persistent_base, sizeof(settings_t));
  } else {
    settings.magic = 0x41424344;
    settings.uart = 0;
    settings.pin_trigger_out = 2;
    settings.baudrate = 115200;
    strcpy(settings.pattern, "test");
    settings.pat_len = 4;
  }
}

void print_settings(void) {
  printf("\r\nSettings:\r\n");
  printf("    %15s: %d\r\n", "Data streaming", settings.stream_toggle);
  printf("    %15s: %d\r\n", "UART instance", settings.uart);
  printf("    %15s: %d\r\n", "Baudrate", settings.baudrate);
  printf("    %15s: %d\r\n", "Trigger out pin", settings.pin_trigger_out);
  printf("    %15s: \r\n", "Pattern");
  hexdump(settings.pattern, settings.pat_len, 0x10);
}

void print_state(void) {
  printf("\r\nState:\r\n");
  printf("    %15s: [%9d]\r\n", "Bytes seen", rx_cnt);
  printf("    %15s: [%4d/%4d]\r\n", "Queue size", queue_get_level(&packet_queue), QUEUE_SIZE);
  printf("    %15s: [%4d/%4d]\r\n", "Pattern matched", cur_pos, settings.pat_len);
}

void print_commands(void) {
  printf("\r\nCommands:\r\n");
  printf("    %15s: %s\r\n", "h", "Show help");
  printf("    %15s: %s\r\n", "b", "Reboot into bootloader");
  printf("    %15s: %s\r\n", "w", "Write settings to flash");
  printf("    %15s: %s\r\n", "d", "Dump currently read bytes over this channel");
  printf("    %15s: %s\r\n", "t", "Toggle streaming of currently read bytes over this channel");
  printf("    %15s: %s\r\n", "c <baud>", "Set baudrate");
  printf("    %15s: %s\r\n", "p <len> <pat>", "Set trigger pattern to provided pattern");
}

void queue_dump(uint len) {
  char ch;
  for (int i = 0; i < len; i++) {
    while (!uart_is_writable(uart0))
      tight_loop_contents();
    if (queue_try_remove(&packet_queue, &ch)) {
      gpio_xor_mask(1<<LED_PIN);
      putchar(ch);
    }
  }
}

void cmd_dump(void) {
  uint len = queue_get_level(&packet_queue);
  if (len) {
    printf("%u:", len);
    queue_dump(queue_get_level(&packet_queue));
  }
}

void cmd_stream_toggle(void) {
  settings.stream_toggle = !settings.stream_toggle;
  printf(" %15s: %d\r\n", "Data streaming", settings.stream_toggle);
}

void __not_in_flash_func(cmd_write_settings_flash)(void* p) {
  uint32_t offset = &__flash_persistent_start - &__flash_binary_start;
  uint8_t page_buffer[FLASH_PAGE_SIZE];

  memcpy(page_buffer, &settings, sizeof(settings_t));

  // Minimum erase is FLASH_SECTOR_SIZE
  flash_range_erase(offset, FLASH_SECTOR_SIZE);
  // Minimum write is FLASH_PAGE_SIZE
  flash_range_program(offset, page_buffer, FLASH_PAGE_SIZE);
}

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
    if (sscanf(readbuf, "%u", &pat_len)) {
      if (pat_len > 128)
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


int main(void)
{
  int ret;

  if (!set_sys_clock_khz(250000, true))
    printf("[!] Error setting clock\r\n");

  stdio_usb_init();
  stdio_set_translate_crlf(&stdio_usb, false);

  // Initialize GPIO for error indicating LED
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, true);
  gpio_put(LED_PIN, true);

  gpio_init(TRIGGER_OUT_PIN);
  gpio_set_dir(TRIGGER_OUT_PIN, true);
  gpio_set_slew_rate(TRIGGER_OUT_PIN, GPIO_SLEW_RATE_FAST);
  // gpio_set_drive_strength(TRIGGER_OUT_PIN, GPIO_DRIVE_STRENGTH_12MA);
  gpio_set_pulls(TRIGGER_OUT_PIN, true, false);
  gpio_put(TRIGGER_OUT_PIN, false);

  gpio_init(15);
  gpio_set_dir(15, true);

  queue_init(&packet_queue, 1, QUEUE_SIZE);

  settings_init();

  multicore_launch_core1(core1_main); // Start core1_main on another core
  
  while (1) {
    int cmd = 0;
    // Check for commands
    cmd = getchar_timeout_us(1);
    if (cmd != PICO_ERROR_TIMEOUT) {
      switch (cmd & 0xff) {
        case 'b':
          reset_usb_boot(LED_PIN, 0);
          break;
        case 'd':
          cmd_dump();
          break;
        case 't':
          cmd_stream_toggle();
          print_settings();
          break;
        case 'w':
          ret = flash_safe_execute(&cmd_write_settings_flash, NULL, 10000);
          break;
        case 'c':
          cmd_set_baud();
          print_settings();
          break;
        case 'p':
          cmd_set_pat();
          print_settings();
          break;
        case '\r':
        case '\n':
        case 'h':
          printf("\r\n----- UART Pico trigger -----\r\n");
          print_state();
          print_settings();
          print_commands();
          break;
      }
    }

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