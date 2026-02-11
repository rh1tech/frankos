set(PICO_BOARD pico2 CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(minesweeper C CXX ASM)
pico_sdk_init()

set(CMAKE_C_STANDARD 11)

set(COMPILED_DIR "${CMAKE_SOURCE_DIR}/../../compiled")

add_executable(${PROJECT_NAME}
    main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../api/m-os-api-sdtfn.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../api/m-os-api-math.c
)

target_include_directories(${PROJECT_NAME} BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../api
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../api/FreeRTOS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../api
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_options(${PROJECT_NAME} PRIVATE
    -Xlinker
    --print-memory-usage
    -r
)

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
target_compile_options(${PROJECT_NAME} PUBLIC
    -fno-builtin-printf
    -fno-jump-tables
    -Werror=implicit-function-declaration
    -include _ansi.h
)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
    configSTACK_DEPTH_TYPE=uint32_t
)

pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_enable_stdio_usb(${PROJECT_NAME} 0)

file(WRITE "${CMAKE_BINARY_DIR}/minesweeper.inf" "Minesweeper")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:${PROJECT_NAME}>
        ${COMPILED_DIR}/minesweeper
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_BINARY_DIR}/minesweeper.inf"
        ${COMPILED_DIR}/minesweeper.inf
)
