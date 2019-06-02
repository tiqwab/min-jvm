cmake_minimum_required(VERSION 3.14)
enable_testing()

project(min_jvm C)

set(CMAKE_C_STANDARD 99)

add_compile_options("-Wall")

add_library(min_jvm main.c main.h)

# copy_files("java/lang/*.class" ".")
file(COPY "stdlib/" DESTINATION "./")

# For test
include(CTest) # This module defines BUILD_TESTING option
if (BUILD_TESTING)
    file(COPY "stdlib/" DESTINATION "tests/")
    add_subdirectory(tests)
endif()
