Include(FetchContent)
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.8.0 # or a later release
)

FetchContent_MakeAvailable(Catch2)

#find_package(Iconv)

add_executable(LibXLL.Tests
        String.cpp
        Variant.cpp
        Array.cpp
)
target_link_libraries(LibXLL.Tests PRIVATE Catch2::Catch2WithMain LibXLL)

if (NOT WIN32)

    add_executable(LibXLL.Tests.ASAN
            String.cpp
            Variant.cpp
            Array.cpp
    )
    target_link_libraries(LibXLL.Tests.ASAN PRIVATE Catch2::Catch2WithMain LibXLL)
    target_compile_options(LibXLL.Tests.ASAN PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
    target_link_options(LibXLL.Tests.ASAN PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)

    add_executable(LibXLL.Tests.UBSAN
            String.cpp
            Variant.cpp
            Array.cpp
    )
    target_link_libraries(LibXLL.Tests.UBSAN PRIVATE Catch2::Catch2WithMain LibXLL)
    target_compile_options(LibXLL.Tests.UBSAN PRIVATE -fsanitize=undefined -fno-omit-frame-pointer -g)
    target_link_options(LibXLL.Tests.UBSAN PRIVATE -fsanitize=undefined -fno-omit-frame-pointer -g)

    add_executable(LibXLL.Tests.TSAN
            String.cpp
            Variant.cpp
            Array.cpp
    )
    target_link_libraries(LibXLL.Tests.TSAN PRIVATE Catch2::Catch2WithMain LibXLL)
    target_compile_options(LibXLL.Tests.TSAN PRIVATE -fsanitize=thread -fno-omit-frame-pointer -g)
    target_link_options(LibXLL.Tests.TSAN PRIVATE -fsanitize=thread -fno-omit-frame-pointer -g)

#    add_executable(LibXLL.Tests.MSAN
#            String.cpp
#            Variant.cpp
#            Array.cpp
#    )
#    target_link_libraries(LibXLL.Tests.MSAN PRIVATE Catch2::Catch2WithMain LibXLL)
#    target_compile_options(LibXLL.Tests.MSAN PRIVATE -fsanitize=memory -fPIE -fno-omit-frame-pointer -g -fno-optimize-sibling-calls -fsanitize-recover -O1)
#    target_link_options(LibXLL.Tests.MSAN PRIVATE -fsanitize=memory -fPIE -fno-omit-frame-pointer -g -fno-optimize-sibling-calls -fsanitize-recover -O1)


#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,leak -fno-omit-frame-pointer -g")
endif (NOT WIN32)