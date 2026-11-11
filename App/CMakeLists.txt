set(APP_LINKER_SCRIPT "${PROJECT_SOURCE_DIR}/gd32f303xG_flash.ld")

file(GLOB APP_CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Core/*.c"
)

file(GLOB APP_TASK_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Task/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Task/*.cpp"
)

add_executable(App
    ${APP_CORE_SOURCES}
    ${APP_TASK_SOURCES}
    "${PROJECT_SOURCE_DIR}/startup_gd32f30x_xd.S"
)

target_include_directories(App PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Core"
    "${CMAKE_CURRENT_SOURCE_DIR}/Task"
)

target_link_libraries(App PRIVATE Firmware freertos_kernel oled_driver)

target_link_options(App PRIVATE
    "-mcpu=cortex-m4"
    "-mthumb"
    "-mfloat-abi=softfp"
    "-mfpu=fpv4-sp-d16"
    "-T${APP_LINKER_SCRIPT}"
    "--specs=nano.specs"
    "--specs=nosys.specs"
    "-Wl,--gc-sections"
    "-Wl,-Map,$<TARGET_FILE_DIR:App>/$<TARGET_FILE_BASE_NAME:App>.map"
    "-Wl,--print-memory-usage"
)

set_target_properties(App PROPERTIES
    SUFFIX ".elf"
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin"
)

add_custom_command(TARGET App POST_BUILD
    COMMAND "${CMAKE_OBJCOPY}" -O ihex "$<TARGET_FILE:App>" "$<TARGET_FILE_DIR:App>/$<TARGET_FILE_BASE_NAME:App>.hex"
    COMMAND "${CMAKE_OBJCOPY}" -O binary "$<TARGET_FILE:App>" "$<TARGET_FILE_DIR:App>/$<TARGET_FILE_BASE_NAME:App>.bin"
    COMMAND "${CMAKE_SIZE}" "$<TARGET_FILE:App>"
    COMMENT "Generating App.hex and App.bin"
)
