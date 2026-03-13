# AddinTests — XLL add-ins exercised via MockXL::Session.
#
# Each add-in is built as a shared library (.xll) and loaded at runtime by a
# corresponding Catch2 test executable that drives it through MockXL::Session.
# XLL_PATH is injected into the test executable via target_compile_definitions
# using a generator expression so the test always finds the right binary.

# ---------------------------------------------------------------------------
# Catch2 — shared by all addin test drivers in this directory
# ---------------------------------------------------------------------------

add_library(Catch2ForAddinTests STATIC ../UnitTests/Catch2/catch_amalgamated.cpp)
target_include_directories(Catch2ForAddinTests SYSTEM PUBLIC ../UnitTests/Catch2)
target_compile_definitions(Catch2ForAddinTests PRIVATE NOMINMAX CATCH_AMALGAMATED_CUSTOM_MAIN)

# ---------------------------------------------------------------------------
# xllAny — addin under test
# ---------------------------------------------------------------------------

add_library(xllAny MODULE xllAny.cpp)
target_link_libraries(xllAny PRIVATE LibXLL project_warnings project_hardening)
set_target_properties(xllAny PROPERTIES PREFIX "" SUFFIX ".xll")
target_compile_definitions(xllAny PRIVATE NOMINMAX)

# ---------------------------------------------------------------------------
# xllAnyTests — Catch2 driver
# ---------------------------------------------------------------------------

add_executable(xllAnyTests xllAnyTests.cpp)
target_link_libraries(xllAnyTests PRIVATE Catch2ForAddinTests MockXL project_warnings project_hardening)
target_compile_definitions(xllAnyTests PRIVATE
        NOMINMAX
        XLL_PATH="$<TARGET_FILE:xllAny>"
)
add_dependencies(xllAnyTests xllAny)
if (NOT WIN32)
    target_link_options(xllAnyTests PRIVATE -rdynamic)
endif ()

# ---------------------------------------------------------------------------
# Sanitizer variants (Linux only; MSan skipped for GCC)
# ---------------------------------------------------------------------------

if (NOT WIN32)
    foreach (sanitizer IN ITEMS address undefined thread memory)
        if (sanitizer STREQUAL "memory" AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            continue()
        endif ()

        add_library(Catch2ForAddinTests.${sanitizer} STATIC ../UnitTests/Catch2/catch_amalgamated.cpp)
        target_include_directories(Catch2ForAddinTests.${sanitizer} SYSTEM PUBLIC ../UnitTests/Catch2)
        target_compile_definitions(Catch2ForAddinTests.${sanitizer} PRIVATE NOMINMAX CATCH_AMALGAMATED_CUSTOM_MAIN)
        target_enable_sanitizer(Catch2ForAddinTests.${sanitizer} ${sanitizer})

        add_library(xllAny.${sanitizer} MODULE xllAny.cpp)
        target_link_libraries(xllAny.${sanitizer} PRIVATE LibXLL project_warnings project_hardening)
        set_target_properties(xllAny.${sanitizer} PROPERTIES PREFIX "" SUFFIX ".xll" OUTPUT_NAME "xllAny.${sanitizer}")
        target_compile_definitions(xllAny.${sanitizer} PRIVATE NOMINMAX)
        target_enable_sanitizer(xllAny.${sanitizer} ${sanitizer})

        add_executable(xllAnyTests.${sanitizer} xllAnyTests.cpp)
        target_link_libraries(xllAnyTests.${sanitizer} PRIVATE Catch2ForAddinTests.${sanitizer} MockXL project_warnings project_hardening)
        target_compile_definitions(xllAnyTests.${sanitizer} PRIVATE
                NOMINMAX
                XLL_PATH="$<TARGET_FILE:xllAny.${sanitizer}>"
        )
        add_dependencies(xllAnyTests.${sanitizer} xllAny.${sanitizer})
        target_link_options(xllAnyTests.${sanitizer} PRIVATE -rdynamic)
        target_enable_sanitizer(xllAnyTests.${sanitizer} ${sanitizer})
    endforeach ()

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
    # Windows clang-cl: only ASan is supported.
    add_library(Catch2ForAddinTests.address STATIC ../UnitTests/Catch2/catch_amalgamated.cpp)
    target_include_directories(Catch2ForAddinTests.address SYSTEM PUBLIC ../UnitTests/Catch2)
    target_compile_definitions(Catch2ForAddinTests.address PRIVATE NOMINMAX CATCH_AMALGAMATED_CUSTOM_MAIN)
    target_enable_sanitizer(Catch2ForAddinTests.address address)

    add_library(xllAny.address MODULE xllAny.cpp)
    target_link_libraries(xllAny.address PRIVATE LibXLL project_warnings project_hardening)
    set_target_properties(xllAny.address PROPERTIES PREFIX "" SUFFIX ".xll" OUTPUT_NAME "xllAny.address")
    target_compile_definitions(xllAny.address PRIVATE NOMINMAX)
    target_enable_sanitizer(xllAny.address address)

    add_executable(xllAnyTests.address xllAnyTests.cpp)
    target_link_libraries(xllAnyTests.address PRIVATE Catch2ForAddinTests.address MockXL project_warnings project_hardening)
    target_compile_definitions(xllAnyTests.address PRIVATE
            NOMINMAX
            XLL_PATH="$<TARGET_FILE:xllAny.address>"
    )
    add_dependencies(xllAnyTests.address xllAny.address)
    target_enable_sanitizer(xllAnyTests.address address)
endif ()
