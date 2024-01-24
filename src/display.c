#include <stdio.h>

#include "display.h"
#include "settings.h"
#include "pins.h"
#include "globals.h"
#include "hardware/pio.h"

uint DEBUG = 0;

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

void print_settings(void) {
  printf("\r\nSettings:\r\n");
  printf("    %15s: %d\r\n", "Data streaming", settings.stream_toggle);
  printf("    %15s: %d\r\n", "UART instance", settings.uart);
  printf("    %15s: %d\r\n", "Baudrate", settings.baudrate);
  printf("    %15s: %d\r\n", "Trigger out pin", TRIGGER_OUT_PIN);
  printf("    %15s: %d\r\n", "debug out pin", DEBUG_OUT_PIN);
  printf("    %15s: %d\r\n", "Glitch delay cycles", settings.glitch_delay);
  printf("    %15s: %d\r\n", "Glitch length cycles", settings.glitch_length);
  
  printf("    %15s: %d and %d\r\n", "Power pins", POWER1_PIN, POWER2_PIN);

  printf("    %15s: %d\r\n", "Pattern length", settings.pat_len);
  printf("    %15s: \r\n", "Pattern");
  hexdump(settings.pattern, settings.pat_len < 128 ? settings.pat_len : 128, 0x10);
}

void print_state(void) {
  printf("\r\nState:\r\n");
  printf("    %15s: [%9d]\r\n", "Bytes seen", rx_cnt);
  printf("    %15s: [%4d/%4d]\r\n", "Queue size", queue_get_level(&packet_queue), QUEUE_SIZE);
  printf("    %15s: [%4d/%4d]\r\n", "Pattern matched", cur_pos, settings.pat_len);
  printf("    %15s: %d%d\r\n", "Power", gpio_get(POWER1_PIN), gpio_get(POWER2_PIN));
  printf("    %15s: %d\r\n", "PIO instruction", pio_sm_get_pc(pio, pio_sm) - pio_offset);
  // printf("    %15s: %08x\r\n", "PIO IRQ", pio->irq);
  // printf("    %15s: %08x\r\n", "DEBUG", DEBUG);
}

void print_commands(void) {
  printf("\r\nCommands:\r\n");
  printf("    %15s: %s\r\n", "h", "Show help");
  printf("    %15s: %s\r\n", "r", "Reset");
  printf("    %15s: %s\r\n", "b", "Reboot into RP2040 bootloader");
  printf("    %15s: %s\r\n", "w", "Write settings to flash");
  printf("    %15s: %s\r\n", "d", "Dump currently read bytes over this channel");
  printf("    %15s: %s\r\n", "t", "Toggle streaming of currently read bytes over this channel");
  printf("    %15s: %s\r\n", "o", "Power OFF");
  printf("    %15s: %s\r\n", "i", "Power ON");
  printf("    %15s: %s\r\n", "a <c1> <c2>", "Arm glitch with <c1> cycles delay and <c2> cycles length");
  printf("    %15s: %s\r\n", "c <baud>", "Set baudrate");
  printf("    %15s: %s\r\n", "p <len> <pat>", "Set trigger pattern to provided pattern");
}