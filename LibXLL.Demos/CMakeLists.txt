include(../cmake/Sanitizers.cmake)

add_library(xllDemo SHARED xllDemo.cpp)
target_link_libraries(xllDemo PUBLIC LibXLL)
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
target_link_libraries(exeDemo PUBLIC LibXLL)
target_compile_definitions(exeDemo PRIVATE NOMINMAX)

add_executable(DemoUtilsEnsure Utils/DemoUtilsEnsure.cpp)
target_link_libraries(DemoUtilsEnsure PUBLIC LibXLL)
target_compile_definitions(DemoUtilsEnsure PRIVATE NOMINMAX)

add_executable(DemoTypesExpected Types/DemoTypesExpected.cpp)
target_link_libraries(DemoTypesExpected PUBLIC LibXLL)
target_compile_definitions(DemoTypesExpected PRIVATE NOMINMAX)

add_executable(DemoExpectedSameType Types/DemoExpectedSameType.cpp)
target_link_libraries(DemoExpectedSameType PUBLIC LibXLL)
target_compile_definitions(DemoExpectedSameType PRIVATE NOMINMAX)

add_executable(DemoArrayShape Types/DemoArrayShape.cpp)
target_link_libraries(DemoArrayShape PUBLIC LibXLL)
target_compile_definitions(DemoArrayShape PRIVATE NOMINMAX)

add_executable(DemoAny Types/DemoAny.cpp)
target_link_libraries(DemoAny PUBLIC LibXLL)
target_compile_definitions(DemoAny PRIVATE NOMINMAX)

add_executable(DemoTuple Types/DemoTuple.cpp)
target_link_libraries(DemoTuple PUBLIC LibXLL)
target_compile_definitions(DemoTuple PRIVATE NOMINMAX)

add_executable(DemoStringEnum Types/DemoStringEnum.cpp)
target_link_libraries(DemoStringEnum PUBLIC LibXLL)
target_compile_definitions(DemoStringEnum PRIVATE NOMINMAX)

# Deliberate-leak demo used to verify ASan is wired up correctly.
# The plain target runs without sanitizers; DemoAsanLeak.address enables ASan.
add_executable(DemoAsanLeak Utils/DemoAsanLeak.cpp)
target_compile_definitions(DemoAsanLeak PRIVATE NOMINMAX)

add_library(xllDemoTypesExpected SHARED Types/xllDemoTypesExpected.cpp)
target_link_libraries(xllDemoTypesExpected PUBLIC LibXLL)
set_target_properties(xllDemoTypesExpected PROPERTIES OUTPUT_NAME "expected_demo" SUFFIX ".xll")
target_compile_definitions(xllDemoTypesExpected PRIVATE NOMINMAX)

add_library(xllDemoExpectedValidation SHARED Types/xllDemoExpectedValidation.cpp)
target_link_libraries(xllDemoExpectedValidation PUBLIC LibXLL)
set_target_properties(xllDemoExpectedValidation PROPERTIES OUTPUT_NAME "expected_validation" SUFFIX ".xll")
target_compile_definitions(xllDemoExpectedValidation PRIVATE NOMINMAX)

add_library(xllDemoAny SHARED Types/xllDemoAny.cpp)
target_link_libraries(xllDemoAny PUBLIC LibXLL)
set_target_properties(xllDemoAny PROPERTIES OUTPUT_NAME "any_demo" SUFFIX ".xll")
target_compile_definitions(xllDemoAny PRIVATE NOMINMAX)

add_library(xllDemoStringEnum SHARED Types/xllDemoStringEnum.cpp)
target_link_libraries(xllDemoStringEnum PUBLIC LibXLL)
set_target_properties(xllDemoStringEnum PROPERTIES OUTPUT_NAME "string_enum_demo" SUFFIX ".xll")
target_compile_definitions(xllDemoStringEnum PRIVATE NOMINMAX)


include(../cmake/CompilerWarnings.cmake)
set_project_warnings(
        xllDemo
#        ${myproject_WARNINGS_AS_ERRORS}
        ""
        ""
        ""
        ""
        "")

if (NOT WIN32)
    foreach (sanitizer IN ITEMS address undefined thread memory)
        add_library(xllDemo.${sanitizer} SHARED xllDemo.cpp)
        target_link_libraries(xllDemo.${sanitizer} PUBLIC LibXLL)
        target_enable_sanitizer(xllDemo.${sanitizer} ${sanitizer})

        add_executable(exeDemo.${sanitizer} exeDemo.cpp)
        target_link_libraries(exeDemo.${sanitizer} PUBLIC LibXLL)
        target_enable_sanitizer(exeDemo.${sanitizer} ${sanitizer})
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

