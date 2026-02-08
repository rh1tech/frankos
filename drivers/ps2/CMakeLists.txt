# PS/2 Unified Driver
# Uses PIO for both keyboard and mouse with separate state machines

add_library(ps2_driver INTERFACE)

pico_generate_pio_header(ps2_driver ${CMAKE_CURRENT_LIST_DIR}/ps2.pio)

target_sources(ps2_driver INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/ps2.c
)

target_include_directories(ps2_driver INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(ps2_driver INTERFACE
    pico_stdlib
    hardware_pio
    hardware_gpio
    hardware_clocks
)
