https://community.element14.com/products/raspberry-pi/b/blog/posts/raspberry-pico-c-sdk-reserve-a-flash-memory-block-for-persistent-storage

https://github.com/tana/pico_usb_sniffer


```
mkdir build
cd build
PICO_SDK_FETCH_FROM_GIT=1 cmake ..
make
```

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