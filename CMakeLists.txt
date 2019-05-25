cmake_minimum_required(VERSION 3.14)
enable_testing()

project(min_jvm C)

set(CMAKE_C_STANDARD 99)

add_compile_options("-Wall")

add_library(min_jvm main.c main.h)

#
# Test
#

function(add_min_jvm_executable name)
    add_executable(${name} ${name}.c)
    target_link_libraries(${name} min_jvm)
endfunction()

foreach(name IN ITEMS
        test_sample
        test_first
        )
    add_min_jvm_executable(${name})
    add_test(NAME ${name} COMMAND $<TARGET_FILE:${name}>)
endforeach()
