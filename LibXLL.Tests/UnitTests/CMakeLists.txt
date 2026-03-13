set(UNIT_TEST_SOURCES
        Any.cpp
        Array.cpp
        Bool.cpp
        Error.cpp
        Expected.cpp
        Int.cpp
        Missing.cpp
        Nil.cpp
        Number.cpp
        Optional.cpp
        String.cpp
        StringEnum.cpp
        Tuple.cpp
        Variant.cpp
)

# ---------------------------------------------------------------------------
# Catch2 — plain build
# ---------------------------------------------------------------------------

add_library(Catch2ForOpenXLL STATIC Catch2/catch_amalgamated.cpp)
target_include_directories(Catch2ForOpenXLL SYSTEM PUBLIC Catch2)
target_compile_definitions(Catch2ForOpenXLL PRIVATE NOMINMAX)

# ---------------------------------------------------------------------------
# Default (no sanitizers)
# ---------------------------------------------------------------------------

add_executable(LibXLL.Tests ${UNIT_TEST_SOURCES})
target_compile_definitions(LibXLL.Tests PRIVATE NOMINMAX)
target_link_libraries(LibXLL.Tests
        PRIVATE
            Catch2ForOpenXLL
            LibXLL
            project_warnings
            project_hardening
)

# ---------------------------------------------------------------------------
# Sanitizer variants (Linux only)
# ---------------------------------------------------------------------------

if (NOT WIN32)
    foreach (sanitizer IN ITEMS address undefined thread memory)
        # Catch2 must be rebuilt with the same sanitizer flags so that all
        # translation units agree on the annotate_string ABI flag.
        add_library(Catch2ForOpenXLL.${sanitizer} STATIC Catch2/catch_amalgamated.cpp)
        target_include_directories(Catch2ForOpenXLL.${sanitizer} SYSTEM PUBLIC Catch2)
        target_compile_definitions(Catch2ForOpenXLL.${sanitizer} PRIVATE NOMINMAX)
        target_enable_sanitizer(Catch2ForOpenXLL.${sanitizer} ${sanitizer})

        add_executable(LibXLL.Tests.${sanitizer} ${UNIT_TEST_SOURCES})
        target_compile_definitions(LibXLL.Tests.${sanitizer} PRIVATE NOMINMAX)
        target_link_libraries(LibXLL.Tests.${sanitizer}
                PRIVATE
                    Catch2ForOpenXLL.${sanitizer}
                    LibXLL
                    project_warnings
                    project_hardening
        )
        target_enable_sanitizer(LibXLL.Tests.${sanitizer} ${sanitizer})
    endforeach ()
endif ()

