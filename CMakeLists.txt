cmake_minimum_required(VERSION 3.14)
enable_testing()

project(min_jvm C)

set(CMAKE_C_STANDARD 99)

add_compile_options("-Wall")

add_executable(min_jvm main.c)

add_executable(test_sample test_sample.c)
add_test(NAME bar COMMAND $<TARGET_FILE:test_sample>)