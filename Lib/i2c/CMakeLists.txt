add_library(i2c_master STATIC i2c.c)
target_include_directories(i2c_master PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(i2c_master PRIVATE Firmware freertos_kernel)
