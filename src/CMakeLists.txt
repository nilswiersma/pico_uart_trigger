cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(pico_trigger_uart)

pico_sdk_init()

add_executable(pico_trigger_uart pico_trigger_uart.c)
# pico_set_binary_type(pico_trigger_uart copy_to_ram)
# pico_generate_pio_header(pico_trigger_uart ${CMAKE_CURRENT_LIST_DIR}/usb_sniff.pio)

pico_set_linker_script(pico_trigger_uart ${CMAKE_SOURCE_DIR}/memmap_custom.ld)
pico_set_program_name(pico_trigger_uart "pico_trigger_uart")
pico_set_program_version(pico_trigger_uart "0.0.1")

target_link_libraries(pico_trigger_uart 
  pico_stdlib pico_multicore pico_stdio_usb pico_flash 
  hardware_pio hardware_dma hardware_irq hardware_uart hardware_exception)

pico_add_extra_outputs(pico_trigger_uart)