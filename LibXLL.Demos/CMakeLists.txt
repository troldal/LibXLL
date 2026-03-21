include(../cmake/Sanitizers.cmake)
include(../cmake/CompilerWarnings.cmake)
include(../cmake/Hardening.cmake)
create_project_warnings_target(project_warnings)
create_hardening_target(project_hardening)

add_library(xllDemo SHARED xllDemo.cpp)
target_link_libraries(xllDemo PUBLIC LibXLL project_warnings project_hardening)
set_target_properties(xllDemo PROPERTIES OUTPUT_NAME "example" SUFFIX ".xll")
target_compile_definitions(xllDemo PRIVATE NOMINMAX)


if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(xllDemo PRIVATE ${CLANG_WARNINGS})
endif ()

#if (NOT WIN32)
#    target_compile_options(xllDemo PUBLIC -stdlib=libc++)
#    target_link_options(xllDemo PUBLIC -stdlib=libc++)
#endif()


add_executable(exeDemo exeDemo.cpp)
target_link_libraries(exeDemo PUBLIC LibXLL project_warnings project_hardening)
target_compile_definitions(exeDemo PRIVATE NOMINMAX)

add_executable(DemoUtilsEnsure Utils/DemoUtilsEnsure.cpp)
target_link_libraries(DemoUtilsEnsure PUBLIC LibXLL project_warnings project_hardening)
target_compile_definitions(DemoUtilsEnsure PRIVATE NOMINMAX)

add_executable(DemoTypesExpected Types/DemoTypesExpected.cpp)
target_link_libraries(DemoTypesExpected PUBLIC LibXLL project_warnings project_hardening)
target_compile_definitions(DemoTypesExpected PRIVATE NOMINMAX)

add_executable(DemoExpectedSameType Types/DemoExpectedSameType.cpp)
target_link_libraries(DemoExpectedSameType PUBLIC LibXLL project_warnings project_hardening)
target_compile_definitions(DemoExpectedSameType PRIVATE NOMINMAX)

add_executable(DemoArrayShape Types/DemoArrayShape.cpp)
target_link_libraries(DemoArrayShape PUBLIC LibXLL project_warnings project_hardening)
target_compile_definitions(DemoArrayShape PRIVATE NOMINMAX)

add_executable(DemoAny Types/DemoAny.cpp)
target_link_libraries(DemoAny PUBLIC LibXLL project_warnings project_hardening)
target_compile_definitions(DemoAny PRIVATE NOMINMAX)

add_executable(DemoVariant Types/DemoVariant.cpp)
target_link_libraries(DemoVariant PUBLIC LibXLL project_warnings project_hardening)
target_compile_definitions(DemoVariant PRIVATE NOMINMAX)

add_executable(DemoTuple Types/DemoTuple.cpp)
target_link_libraries(DemoTuple PUBLIC LibXLL project_warnings project_hardening)
target_compile_definitions(DemoTuple PRIVATE NOMINMAX)

add_executable(DemoStringEnum Types/DemoStringEnum.cpp)
target_link_libraries(DemoStringEnum PUBLIC LibXLL project_warnings project_hardening)
target_compile_definitions(DemoStringEnum PRIVATE NOMINMAX)

add_executable(DemoMatrix Types/DemoMatrix.cpp)
target_link_libraries(DemoMatrix PUBLIC LibXLL project_warnings project_hardening)
target_compile_definitions(DemoMatrix PRIVATE NOMINMAX)

# Deliberate-leak demo used to verify ASan is wired up correctly.
# The plain target runs without sanitizers; DemoAsanLeak.address enables ASan.
add_executable(DemoAsanLeak Utils/DemoAsanLeak.cpp)
target_compile_definitions(DemoAsanLeak PRIVATE NOMINMAX)

add_library(xllDemoTypesExpected SHARED Types/xllDemoTypesExpected.cpp)
target_link_libraries(xllDemoTypesExpected PUBLIC LibXLL project_warnings project_hardening)
set_target_properties(xllDemoTypesExpected PROPERTIES OUTPUT_NAME "expected_demo" SUFFIX ".xll")
target_compile_definitions(xllDemoTypesExpected PRIVATE NOMINMAX)

add_library(xllDemoExpectedValidation SHARED Types/xllDemoExpectedValidation.cpp)
target_link_libraries(xllDemoExpectedValidation PUBLIC LibXLL project_warnings project_hardening)
set_target_properties(xllDemoExpectedValidation PROPERTIES OUTPUT_NAME "expected_validation" SUFFIX ".xll")
target_compile_definitions(xllDemoExpectedValidation PRIVATE NOMINMAX)

add_library(xllDemoAny SHARED Types/xllDemoAny.cpp)
target_link_libraries(xllDemoAny PUBLIC LibXLL project_warnings project_hardening)
set_target_properties(xllDemoAny PROPERTIES OUTPUT_NAME "any_demo" SUFFIX ".xll")
target_compile_definitions(xllDemoAny PRIVATE NOMINMAX)

add_library(xllDemoStringEnum SHARED Types/xllDemoStringEnum.cpp)
target_link_libraries(xllDemoStringEnum PUBLIC LibXLL project_warnings project_hardening)
set_target_properties(xllDemoStringEnum PROPERTIES OUTPUT_NAME "string_enum_demo" SUFFIX ".xll")
target_compile_definitions(xllDemoStringEnum PRIVATE NOMINMAX)

add_library(xllDemoHandle SHARED Types/xllDemoHandle.cpp)
target_link_libraries(xllDemoHandle PUBLIC LibXLL project_warnings project_hardening)
set_target_properties(xllDemoHandle PROPERTIES OUTPUT_NAME "handle_demo" SUFFIX ".xll")
target_compile_definitions(xllDemoHandle PRIVATE NOMINMAX)

add_library(xllDemoTuple SHARED Types/xllDemoTuple.cpp)
target_link_libraries(xllDemoTuple PUBLIC LibXLL project_warnings project_hardening)
set_target_properties(xllDemoTuple PROPERTIES OUTPUT_NAME "tuple_demo" SUFFIX ".xll")
target_compile_definitions(xllDemoTuple PRIVATE NOMINMAX)

add_library(xllDemoScalarValidation SHARED Types/xllDemoScalarValidation.cpp)
target_link_libraries(xllDemoScalarValidation PUBLIC LibXLL project_warnings project_hardening)
set_target_properties(xllDemoScalarValidation PROPERTIES OUTPUT_NAME "scalar_validation" SUFFIX ".xll" PREFIX "")
target_compile_definitions(xllDemoScalarValidation PRIVATE NOMINMAX)

if (WIN32)
    add_library(xllDemoWxDialog SHARED Types/xllDemoWxDialog.cpp)
    target_link_libraries(xllDemoWxDialog PUBLIC LibXLL project_warnings project_hardening wx::core wx::base)
    set_target_properties(xllDemoWxDialog PROPERTIES OUTPUT_NAME "wx_dialog_demo" SUFFIX ".xll")
    target_compile_definitions(xllDemoWxDialog PRIVATE NOMINMAX
        # /sdl (from project_hardening) promotes C4996 "deprecated function" to
        # an error AFTER /wd4996 has suppressed it, because /sdl is processed
        # later on the command line and overrides /wd flags.  The only fix that
        # wins against /sdl is _CRT_SECURE_NO_WARNINGS, which tells the CRT
        # headers to omit the deprecation markup entirely at the preprocessor
        # level - the warning is never emitted so /sdl has nothing to promote.
        $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
    )
    # No post-build DLL copy needed: wxWidgets is statically linked
    # (BUILD_SHARED_LIBS OFF), so TARGET_RUNTIME_DLLS is empty.

    add_executable(wxDemo WIN32 wxDemo.cpp wxDemo.rc)
    target_link_libraries(wxDemo PRIVATE wx::core wx::base)
    target_compile_definitions(wxDemo PRIVATE NOMINMAX)
    # MSVC (and clang-cl) auto-generate a manifest and embed it at link time.
    # We embed our own via wxDemo.rc, so suppress the auto-generated one to
    # avoid a duplicate RT_MANIFEST resource (CVT1100 / LNK1123).
    if (MSVC)
        target_link_options(wxDemo PRIVATE /MANIFEST:NO)
    endif()
    # No post-build DLL copy needed: wxWidgets is statically linked.
endif()

add_executable(DemoMockExcelScalar Types/DemoMockExcelScalar.cpp)
target_link_libraries(DemoMockExcelScalar PUBLIC MockXL)
target_compile_definitions(DemoMockExcelScalar PRIVATE NOMINMAX)
add_dependencies(DemoMockExcelScalar xllDemoScalarValidation)
if (NOT WIN32)
    target_link_options(DemoMockExcelScalar PRIVATE -rdynamic)
endif()


if (NOT WIN32)
    foreach (sanitizer IN ITEMS address undefined thread memory)
        # MSan (-fsanitize=memory) is Clang-only; skip it for GCC
        if(sanitizer STREQUAL "memory" AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            continue()
        endif()
        add_library(xllDemo.${sanitizer} SHARED xllDemo.cpp)
        target_link_libraries(xllDemo.${sanitizer} PUBLIC LibXLL project_warnings project_hardening)
        target_enable_sanitizer(xllDemo.${sanitizer} ${sanitizer})

        add_executable(exeDemo.${sanitizer} exeDemo.cpp)
        target_link_libraries(exeDemo.${sanitizer} PUBLIC LibXLL project_warnings project_hardening)
        target_enable_sanitizer(exeDemo.${sanitizer} ${sanitizer})

        add_executable(DemoMatrix.${sanitizer} Types/DemoMatrix.cpp)
        target_link_libraries(DemoMatrix.${sanitizer} PUBLIC LibXLL project_warnings project_hardening)
        target_compile_definitions(DemoMatrix.${sanitizer} PRIVATE NOMINMAX)
        target_enable_sanitizer(DemoMatrix.${sanitizer} ${sanitizer})

        add_library(xllDemoScalarValidation.${sanitizer} SHARED Types/xllDemoScalarValidation.cpp)
        target_link_libraries(xllDemoScalarValidation.${sanitizer} PUBLIC LibXLL)
        set_target_properties(xllDemoScalarValidation.${sanitizer} PROPERTIES OUTPUT_NAME "scalar_validation.${sanitizer}" SUFFIX ".xll" PREFIX "")
        target_compile_definitions(xllDemoScalarValidation.${sanitizer} PRIVATE NOMINMAX)
        target_enable_sanitizer(xllDemoScalarValidation.${sanitizer} ${sanitizer})

        add_executable(DemoMockExcelScalar.${sanitizer} Types/DemoMockExcelScalar.cpp)
        target_link_libraries(DemoMockExcelScalar.${sanitizer} PUBLIC MockXL)
        target_compile_definitions(DemoMockExcelScalar.${sanitizer} PRIVATE NOMINMAX)
        target_link_options(DemoMockExcelScalar.${sanitizer} PRIVATE -rdynamic)
        target_enable_sanitizer(DemoMockExcelScalar.${sanitizer} ${sanitizer})
        add_dependencies(DemoMockExcelScalar.${sanitizer} xllDemoScalarValidation.${sanitizer})
    endforeach ()

    # ASan build of the deliberate-leak demo.
    # Run it and LSan reports a direct leak of 40 bytes (10 × sizeof(int)).
    add_executable(DemoAsanLeak.address Utils/DemoAsanLeak.cpp)
    target_compile_definitions(DemoAsanLeak.address PRIVATE NOMINMAX)
    target_enable_sanitizer(DemoAsanLeak.address address)

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
    # Windows clang-cl: only ASan is supported.
    # Note: LeakSanitizer is not available on Windows; use-after-free and other
    # heap errors are still detected.
    add_executable(DemoAsanLeak.address Utils/DemoAsanLeak.cpp)
    target_compile_definitions(DemoAsanLeak.address PRIVATE NOMINMAX)
    target_enable_sanitizer(DemoAsanLeak.address address)
endif()

