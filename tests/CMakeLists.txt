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
        call_static_method_two_arg
        call_static_method_other_class
        call_static_field
        initialize_class
        create_instance
        instance_fields
        static_reference_field
        just_return
        )
    add_min_jvm_executable(${name})
    add_test(NAME test_${name} COMMAND $<TARGET_FILE:test_${name}>)
    set_property(TEST test_${name} APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/nativelib:$ENV{LD_LIBRARY_PATH})
endforeach()

foreach(name IN ITEMS
        First.class
        CallStaticMethodNoArg.class
        CallStaticMethodOneArg.class
        CallStaticMethodTwoArg.class
        CallStaticMethodCaller.class
        CallStaticMethodCallee.class
        StaticField.class
        InitializeClass.class
        CreateInstance.class
        InstanceFields.class
        StaticReferenceField.class
        JustReturn.class
        )
    configure_file(${name} . COPYONLY)
endforeach()
