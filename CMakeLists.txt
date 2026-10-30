cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/arm-none-eabi.cmake)

project(gd32_proj LANGUAGES C CXX ASM)

#add_subdirectory(bootloader)

add_subdirectory(Firmware)
add_subdirectory(App)
