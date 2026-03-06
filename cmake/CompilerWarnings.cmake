# Creates an INTERFACE library target named <target_name> that carries all
# project warning flags for the current compiler. Link against it with
# target_link_libraries(<your_target> PRIVATE project_warnings) (or the name
# you choose). Calling this function a second time with the same name is a
# no-op so that it is safe to include from multiple subdirectories.
function(create_project_warnings_target target_name)
    if(TARGET ${target_name})
        return()
    endif()

    add_library(${target_name} INTERFACE)

    set(MSVC_WARNINGS
            /permissive-
            /W4
            /w14242
            /w14254
            /w14263
            /w14265
            /w14287
            /we4289
            /w14296
            /w14311
            /w14545
            /w14546
            /w14547
            /w14549
            /w14555
            /w14619
            /w14640
            /w14826
            /w14905
            /w14906
            /w14928
    )

    set(CLANG_WARNINGS
            -pedantic
            -Wall
            -Wextra
            -Wshadow
            -Wnon-virtual-dtor
            -Wold-style-cast
            -Wcast-align
            -Wunused
            -Woverloaded-virtual
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wnull-dereference
            -Wdouble-promotion
            -Wformat=2
            -Wimplicit-fallthrough
            -Weffc++
    )

    set(GCC_WARNINGS
            ${CLANG_WARNINGS}
            -Wmisleading-indentation
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
            -Wnull-dereference
            -Wuseless-cast
            -Wdouble-promotion
            -Wformat=2
    )

    if(MSVC)
        set(SELECTED_WARNINGS ${MSVC_WARNINGS})
    elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        set(SELECTED_WARNINGS ${CLANG_WARNINGS})
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(SELECTED_WARNINGS ${GCC_WARNINGS})
    else()
        message(AUTHOR_WARNING "No compiler warnings set for CXX compiler: '${CMAKE_CXX_COMPILER_ID}'")
    endif()

    target_compile_options(${target_name} INTERFACE
            $<$<COMPILE_LANGUAGE:CXX>:${SELECTED_WARNINGS}>
            $<$<COMPILE_LANGUAGE:C>:${SELECTED_WARNINGS}>
    )
endfunction()

