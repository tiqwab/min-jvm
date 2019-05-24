cmake_minimum_required(VERSION 3.14)
project(min_jvm C)

set(CMAKE_C_STANDARD 99)

add_compile_options("-Wall")

add_executable(min_jvm main.c)