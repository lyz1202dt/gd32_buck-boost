cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/arm-none-eabi.cmake)

project(gd32_proj LANGUAGES C ASM)

#add_subdirectory(bootloader)

add_subdirectory(Firmware)

add_library(freertos_config INTERFACE)
target_include_directories(freertos_config SYSTEM INTERFACE
    "${PROJECT_SOURCE_DIR}/App/Core"
)
target_link_libraries(freertos_config INTERFACE Firmware)

set(FREERTOS_PORT GCC_ARM_CM4F CACHE STRING "")
set(FREERTOS_HEAP 4 CACHE STRING "")

add_subdirectory(third_party/FreeRTOS-Kernel)
add_subdirectory(Lib/i2c)
add_subdirectory(Lib/usart)
add_subdirectory(third_party/HAL_I2C_0.96_OLED_Driver)

add_subdirectory(App)
