add_compile_options("-Wall" "-g")

function(add_min_jvm_executable name)
    add_executable(test_${name} ${name}.c)
    target_link_libraries(test_${name} min_jvm)
endfunction()

foreach(name IN ITEMS
        sample
        first
        call_static_method_no_arg
        call_static_method_one_arg
        )
    add_min_jvm_executable(${name})
    add_test(NAME test_${name} COMMAND $<TARGET_FILE:test_${name}>)
endforeach()

foreach(name IN ITEMS
        First.class
        CallStaticMethodNoArg.class
        CallStaticMethodOneArg.class
        )
    configure_file(${name} . COPYONLY)
endforeach()
