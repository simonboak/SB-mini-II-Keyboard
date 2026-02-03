cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(sb_mini_ii_keyboard C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(sb_mini_ii_keyboard
    main.c
)

target_include_directories(sb_mini_ii_keyboard PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(sb_mini_ii_keyboard
    pico_stdlib
    tinyusb_host
    tinyusb_board
)

# Use UART for stdio since USB port is used for host mode
pico_enable_stdio_usb(sb_mini_ii_keyboard 0)
pico_enable_stdio_uart(sb_mini_ii_keyboard 1)

pico_add_extra_outputs(sb_mini_ii_keyboard)
