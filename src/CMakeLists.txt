cmake_minimum_required(VERSION 3.13)

add_compile_definitions(USBD_PRODUCT="PICO_FI_CONTROL")
add_compile_definitions(PICO_STDIO_USB_SUPPORT_CHARS_AVAILABLE_CALLBACK=1)

include(pico_sdk_import.cmake)

project(pico_fi_control)


pico_sdk_init()

add_executable(pico_fi_control 
  core1.c display.c flash.c settings.c
  pico_trigger_uart.c)
# pico_set_binary_type(pico_fi_control copy_to_ram)
pico_generate_pio_header(pico_fi_control ${CMAKE_CURRENT_LIST_DIR}/trigger_basic.pio)

pico_set_linker_script(pico_fi_control ${CMAKE_SOURCE_DIR}/memmap_custom.ld)
pico_set_program_name(pico_fi_control "pico_fi_control")
pico_set_program_version(pico_fi_control "0.1")

target_link_libraries(pico_fi_control 
  pico_stdlib pico_multicore pico_stdio_usb pico_flash 
  hardware_pio hardware_dma hardware_irq hardware_uart hardware_exception)

pico_add_extra_outputs(pico_fi_control)