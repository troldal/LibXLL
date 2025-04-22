add_library(Catch2ForOpenXLL STATIC Catch2/catch_amalgamated.cpp)
target_include_directories(Catch2ForOpenXLL PUBLIC Catch2)


include(FetchContent)

add_executable(LibXLL.Tests
        String.cpp
        Bool.cpp
        Expected.cpp
        Int.cpp
        Number.cpp
        Variant.cpp
        Array.cpp
)
target_link_libraries(LibXLL.Tests PRIVATE Catch2ForOpenXLL LibXLL)

if (NOT WIN32)
    foreach (sanitizer IN ITEMS address undefined thread memory)
        add_executable(LibXLL.Tests.${sanitizer}
                String.cpp
                Bool.cpp
                Expected.cpp
                Int.cpp
                Number.cpp
                Variant.cpp
                Array.cpp
        )
        target_link_libraries(LibXLL.Tests.${sanitizer} PRIVATE Catch2ForOpenXLL LibXLL)
        if (sanitizer STREQUAL "address")
            target_compile_options(LibXLL.Tests.${sanitizer} PRIVATE -fsanitize=${sanitizer} -fno-omit-frame-pointer -g)
            target_link_options(LibXLL.Tests.${sanitizer} PRIVATE -fsanitize=${sanitizer} -fno-omit-frame-pointer -g)
        elseif (sanitizer STREQUAL "undefined")
            target_compile_options(LibXLL.Tests.${sanitizer} PRIVATE -fsanitize=${sanitizer} -fno-omit-frame-pointer -g)
            target_link_options(LibXLL.Tests.${sanitizer} PRIVATE -fsanitize=${sanitizer} -fno-omit-frame-pointer -g)
        elseif (sanitizer STREQUAL "thread")
            target_compile_options(LibXLL.Tests.${sanitizer} PRIVATE -fsanitize=${sanitizer} -fPIE -pie -g)
            target_link_options(LibXLL.Tests.${sanitizer} PRIVATE -fsanitize=${sanitizer} -fPIE -pie -g)
        elseif (sanitizer STREQUAL "memory")
            target_compile_options(LibXLL.Tests.${sanitizer} PRIVATE -fsanitize=${sanitizer} -fPIE -fno-omit-frame-pointer -g -fno-optimize-sibling-calls -fsanitize-recover -O1)
            target_link_options(LibXLL.Tests.${sanitizer} PRIVATE -fsanitize=${sanitizer} -fPIE -fno-omit-frame-pointer -g -fno-optimize-sibling-calls -fsanitize-recover -O1)
        endif ()
    endforeach ()
endif ()

add_compile_definitions(NOMINMAX)