cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(sb_mini_ii_keyboard C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# Keyboard layout, selected at build time: cmake -DKEYBOARD_LAYOUT=de ..
set(KEYBOARD_LAYOUT "us" CACHE STRING "Keyboard layout (us, de)")
set_property(CACHE KEYBOARD_LAYOUT PROPERTY STRINGS us de)

set(LAYOUT_HEADER "layouts/layout_${KEYBOARD_LAYOUT}.h")
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${LAYOUT_HEADER})
    message(FATAL_ERROR
        "Unknown KEYBOARD_LAYOUT '${KEYBOARD_LAYOUT}': ${LAYOUT_HEADER} does not exist")
endif()
message(STATUS "Keyboard layout: ${KEYBOARD_LAYOUT} (${LAYOUT_HEADER})")

add_executable(sb_mini_ii_keyboard
    main.c
)

target_include_directories(sb_mini_ii_keyboard PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(sb_mini_ii_keyboard PRIVATE
    KEYBOARD_LAYOUT_HEADER="${LAYOUT_HEADER}"
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
