Use a Raspberry Pi Pico to control your FI setup.
Currently focused around triggering on UART communication, to be extended.

- Core 0: handles all IO to host machine via USB serial emulation (from standard tinyusb library).
- Core 1: monitors UART pin and pattern matching.
- PIO0: handles generating trigger pulse for X cycles, Y cycles after matching pattern.

Configuration can be written to flash to survive resets.

Help output:

```
----- UART Pico trigger -----
State:
         Bytes seen: [  1671410]
         Queue size: [   0/8192]
    Pattern matched: [   0/   4]
Settings:
       Last command: ff
     Data streaming: 1
      UART instance: 0
           Baudrate: 115200
    Trigger out pin: 2
            Pattern: 44454344
Commands:
                  h: Show help
                  b: Reboot into bootloader
                  s: Show settings and status
                  d: Dump currently read bytes over this channel
                  t: Toggle streaming of currently read bytes over this channel
           b <baud>: Set baudrate
      p <len> <pat>: Set trigger pattern to provided pattern
```

# Building

Requirements:

- cmake
- gcc-arm-none-eabi
- doxygen

Commands:

```
mkdir build
cd build
PICO_SDK_FETCH_FROM_GIT=1 cmake ../src
make
```

Build docs:

```
make doxygen
```

# Some links

- https://community.element14.com/products/raspberry-pi/b/blog/posts/raspberry-pico-c-sdk-reserve-a-flash-memory-block-for-persistent-storage
- https://github.com/tana/pico_usb_sniffer