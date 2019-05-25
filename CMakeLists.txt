cmake_minimum_required(VERSION 3.14)
enable_testing()

project(min_jvm C)

set(CMAKE_C_STANDARD 99)

add_compile_options("-Wall")

add_library(min_jvm main.c main.h)

# For test
include(CTest) # This module defines BUILD_TESTING option
if (BUILD_TESTING)
    add_subdirectory(tests)
endif()
