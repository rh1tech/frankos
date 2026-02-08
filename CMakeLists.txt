cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico2)
set(PICO_PLATFORM rp2350-arm-s)

set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_LIST_DIR}/lib/FreeRTOS-Kernel)

include(pico_sdk_import.cmake)
include(FreeRTOS_Kernel_import.cmake)

project(rhea C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

pico_sdk_init()

# Tell FreeRTOS port where to find FreeRTOSConfig.h
set(FREERTOS_CONFIG_FILE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/src CACHE STRING "")

# HDMI driver as separate static library (no FreeRTOS dependency)
add_library(hdmi_driver STATIC
    drivers/hdmi/hdmi.c
)
target_include_directories(hdmi_driver PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/drivers/hdmi
)
target_include_directories(hdmi_driver PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)
target_link_libraries(hdmi_driver
    pico_stdlib
    pico_multicore
    hardware_dma
    hardware_pio
    hardware_clocks
    hardware_irq
)

# Main executable
add_executable(rhea
    src/main.c
    src/display.c
)
target_include_directories(rhea PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/drivers/hdmi
)
target_link_libraries(rhea
    hdmi_driver
    pico_stdlib
    pico_multicore
    hardware_clocks
    FreeRTOS-Kernel-Heap4
)

# USB output for printf, no UART
pico_enable_stdio_usb(rhea 1)
pico_enable_stdio_uart(rhea 0)

pico_add_extra_outputs(rhea)
