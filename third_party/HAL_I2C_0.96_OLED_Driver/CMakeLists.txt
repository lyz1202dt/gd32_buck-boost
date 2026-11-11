add_library(oled_driver STATIC
    Src/OLED.c
    Src/OLED_Data.c
)

target_include_directories(oled_driver PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Inc")
