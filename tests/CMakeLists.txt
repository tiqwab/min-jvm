function(add_min_jvm_executable name)
    add_executable(test_${name} ${name}.c)
    target_link_libraries(test_${name} min_jvm)
endfunction()

foreach(name IN ITEMS
        sample
        first
        )
    add_min_jvm_executable(${name})
    add_test(NAME test_${name} COMMAND $<TARGET_FILE:test_${name}>)
endforeach()

foreach(name IN ITEMS
        First.class
        )
    configure_file(${name} . COPYONLY)
endforeach()
