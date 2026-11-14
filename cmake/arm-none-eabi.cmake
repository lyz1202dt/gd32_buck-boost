set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

find_program(CMAKE_C_COMPILER arm-none-eabi-gcc REQUIRED)
find_program(CMAKE_ASM_COMPILER arm-none-eabi-gcc REQUIRED)
find_program(CMAKE_AR arm-none-eabi-ar REQUIRED)
find_program(CMAKE_OBJCOPY arm-none-eabi-objcopy REQUIRED)
find_program(CMAKE_SIZE arm-none-eabi-size REQUIRED)

set(MCU_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16")
set(COMMON_FLAGS "${MCU_FLAGS} -ffunction-sections -fdata-sections")

set(CMAKE_C_FLAGS_INIT "${COMMON_FLAGS} -Wno-unknown-pragmas")
set(CMAKE_ASM_FLAGS_INIT "${MCU_FLAGS} -x assembler-with-cpp")

set(CMAKE_C_FLAGS_DEBUG_INIT "-O0 -g3")
set(CMAKE_C_FLAGS_RELEASE_INIT "-Os -DNDEBUG")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()
