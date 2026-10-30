file(GLOB FIRMWARE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/standard_peripheral/Source/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/GD/GD32F30x/Source/*.c"
)

# System clock selection is board-specific and is maintained by the application.
list(REMOVE_ITEM FIRMWARE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/GD/GD32F30x/Source/system_gd32f30x.c"
)

add_library(Firmware STATIC ${FIRMWARE_SOURCES})

target_include_directories(Firmware PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/standard_peripheral/Include"
    "${CMAKE_CURRENT_SOURCE_DIR}/CMSIS"
    "${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/GD/GD32F30x/Include"
)

target_compile_definitions(Firmware PUBLIC
    GD32F30X_XD
    USE_STDPERIPH_DRIVER
)

set_target_properties(Firmware PROPERTIES
    OUTPUT_NAME firmware
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib"
)
