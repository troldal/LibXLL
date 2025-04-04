include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.8.0
)

FetchContent_MakeAvailable(Catch2)

add_executable(LibXLL.Tests
        String.cpp
        Bool.cpp
        Int.cpp
        Number.cpp
        Variant.cpp
        Array.cpp
)
target_link_libraries(LibXLL.Tests PRIVATE Catch2::Catch2WithMain LibXLL)

if (NOT WIN32)
    foreach (sanitizer IN ITEMS address undefined thread memory)
        add_executable(LibXLL.Tests.${sanitizer}
                String.cpp
                Bool.cpp
                Int.cpp
                Number.cpp
                Variant.cpp
                Array.cpp
        )
        target_link_libraries(LibXLL.Tests.${sanitizer} PRIVATE Catch2::Catch2WithMain LibXLL)
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