add_library(usart_driver STATIC usart.c)
target_include_directories(usart_driver PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(usart_driver PRIVATE Firmware freertos_kernel)
