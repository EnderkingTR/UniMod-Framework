# CompilerWarnings.cmake - Strict compiler warnings and safety flags
function(unimod_set_target_warnings TARGET_NAME)
    if(MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE
            /W4
            /permissive-
            /utf-8
            /EHsc
            $<$<CONFIG:Release>:/O2 /Oi /Gy>
        )
    else()
        target_compile_options(${TARGET_NAME} PRIVATE
            -Wall
            -Wextra
            -Wno-unused-parameter
            -Wno-template-body
            $<$<CONFIG:Release>:-O3>
        )
    endif()
endfunction()
