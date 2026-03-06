include(../cmake/Sanitizers.cmake)
include(../cmake/CompilerWarnings.cmake)
include(../cmake/Hardening.cmake)
create_project_warnings_target(project_warnings)
create_hardening_target(project_hardening)

add_library(Catch2ForOpenXLL STATIC Catch2/catch_amalgamated.cpp)
target_include_directories(Catch2ForOpenXLL SYSTEM PUBLIC Catch2)

# A separate Catch2 build compiled with ASan instrumentation for the Windows
# clang-cl address sanitizer target. All TUs in one executable must agree on
# the annotate_string ABI flag, so Catch2 must be rebuilt with the same flags.
add_library(Catch2ForOpenXLL.asan STATIC Catch2/catch_amalgamated.cpp)
target_include_directories(Catch2ForOpenXLL.asan SYSTEM PUBLIC Catch2)
target_enable_sanitizer(Catch2ForOpenXLL.asan address)


set(TEST_SOURCES
        String.cpp
        Bool.cpp
        Expected.cpp
        Int.cpp
        Number.cpp
        Nil.cpp
        Missing.cpp
        Error.cpp
        Variant.cpp
        Array.cpp
        Optional.cpp
        Any.cpp
        Tuple.cpp
        StringEnum.cpp
)


add_executable(LibXLL.Tests.default ${TEST_SOURCES})
target_compile_definitions(LibXLL.Tests.default PRIVATE NOMINMAX)
target_link_libraries(LibXLL.Tests.default PRIVATE Catch2ForOpenXLL LibXLL project_warnings project_hardening)


if (NOT WIN32)
    foreach (sanitizer IN ITEMS address undefined thread memory)
        # MSan (-fsanitize=memory) is Clang-only; skip it for GCC
        if(sanitizer STREQUAL "memory" AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            continue()
        endif()
        set(target LibXLL.Tests.${sanitizer})
        add_executable(${target} ${TEST_SOURCES})
        target_compile_definitions(${target} PRIVATE NOMINMAX)
        target_link_libraries(${target} PRIVATE Catch2ForOpenXLL LibXLL project_warnings project_hardening)
        target_enable_sanitizer(${target} ${sanitizer})
    endforeach ()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
    set(target LibXLL.Tests.address)
    add_executable(${target} ${TEST_SOURCES})
    target_compile_definitions(${target} PRIVATE NOMINMAX)
    target_link_libraries(${target} PRIVATE Catch2ForOpenXLL.asan LibXLL project_warnings project_hardening)
    target_enable_sanitizer(${target} address)
endif ()

