add_library(pid STATIC pid.c)
target_include_directories(pid PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
