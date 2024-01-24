cmake_minimum_required(VERSION 3.13)

add_compile_definitions(USBD_PRODUCT="PICO_FI_CONTROL")
add_compile_definitions(PICO_STDIO_USB_SUPPORT_CHARS_AVAILABLE_CALLBACK=1)

include(pico_sdk_import.cmake)

project(pico_fi_control)

pico_sdk_init()

add_executable(pico_fi_control
  core1.c
  display.c
  flash.c
  settings.c
  pico_trigger_uart.c)
# pico_set_binary_type(pico_fi_control copy_to_ram)
pico_generate_pio_header(pico_fi_control ${CMAKE_CURRENT_LIST_DIR}/trigger_basic.pio)

pico_set_linker_script(pico_fi_control ${CMAKE_SOURCE_DIR}/memmap_custom.ld)
pico_set_program_name(pico_fi_control "pico_fi_control")
pico_set_program_version(pico_fi_control "0.1")

target_link_libraries(pico_fi_control 
  pico_stdlib
  pico_multicore
  pico_stdio_usb
  pico_flash
  hardware_pio
  hardware_dma
  hardware_irq
  hardware_uart
  hardware_exception)

pico_add_extra_outputs(pico_fi_control)

find_package( Doxygen )
if ( DOXYGEN_FOUND )
  set( DOXYGEN_OUTPUT_DIRECTORY docs )
  set( DOXYGEN_COLLABORATION_GRAPH YES )
  set( DOXYGEN_EXTRACT_ALL YES )
  set( DOXYGEN_CLASS_DIAGRAMS YES )
  set( DOXYGEN_HIDE_UNDOC_RELATIONS NO )
  set( DOXYGEN_HAVE_DOT YES )
  set( DOXYGEN_CLASS_GRAPH YES )
  set( DOXYGEN_CALL_GRAPH YES )
  set( DOXYGEN_CALLER_GRAPH YES )
  set( DOXYGEN_COLLABORATION_GRAPH YES )
  set( DOXYGEN_BUILTIN_STL_SUPPORT YES )
  set( DOXYGEN_EXTRACT_PRIVATE YES )
  set( DOXYGEN_EXTRACT_PACKAGE YES )
  set( DOXYGEN_EXTRACT_STATIC YES )
  set( DOXYGEN_EXTRACT_LOCALMETHODS YES )
  set( DOXYGEN_UML_LOOK YES )
  set( DOXYGEN_UML_LIMIT_NUM_FIELDS 50 )
  set( DOXYGEN_TEMPLATE_RELATIONS YES )
  set( DOXYGEN_DOT_GRAPH_MAX_NODES 100 )
  set( DOXYGEN_MAX_DOT_GRAPH_DEPTH 0 )
  set( DOXYGEN_DOT_TRANSPARENT YES )
  doxygen_add_docs( doxygen ${RPP_PROJECT_SOURCE_DIR} )
else()
  message( "Doxygen need to be installed to generate the doxygen documentation" )
endif()