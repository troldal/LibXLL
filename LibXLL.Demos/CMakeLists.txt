add_library(xllDemo SHARED xllDemo.cpp)
target_link_libraries(xllDemo PUBLIC LibXLL)
set_target_properties(xllDemo PROPERTIES OUTPUT_NAME "example" SUFFIX ".xll")

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(xllDemo PRIVATE ${CLANG_WARNINGS})
endif ()

#if (NOT WIN32)
#    target_compile_options(xllDemo PUBLIC -stdlib=libc++)
#    target_link_options(xllDemo PUBLIC -stdlib=libc++)
#endif()


add_executable(exeDemo exeDemo.cpp)
target_link_libraries(exeDemo PUBLIC LibXLL)


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

    add_library(xllDemo.ASAN SHARED xllDemo.cpp)
    target_link_libraries(xllDemo.ASAN PUBLIC LibXLL)
    target_compile_options(xllDemo.ASAN PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
    target_link_options(xllDemo.ASAN PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)

    add_executable(exeDemo.ASAN exeDemo.cpp)
    target_link_libraries(exeDemo.ASAN PUBLIC xllDemo.ASAN)
    target_compile_options(exeDemo.ASAN PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
    target_link_options(exeDemo.ASAN PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)

    add_library(xllDemo.UBSAN SHARED xllDemo.cpp)
    target_link_libraries(xllDemo.UBSAN PUBLIC LibXLL)
    target_compile_options(xllDemo.UBSAN PRIVATE -fsanitize=undefined -fno-omit-frame-pointer -g)
    target_link_options(xllDemo.UBSAN PRIVATE -fsanitize=undefined -fno-omit-frame-pointer -g)

    add_executable(exeDemo.UBSAN exeDemo.cpp)
    target_link_libraries(exeDemo.UBSAN PUBLIC xllDemo.UBSAN)
    target_compile_options(exeDemo.UBSAN PRIVATE -fsanitize=undefined -fno-omit-frame-pointer -g)
    target_link_options(exeDemo.UBSAN PRIVATE -fsanitize=undefined -fno-omit-frame-pointer -g)

    add_library(xllDemo.TSAN SHARED xllDemo.cpp)
    target_link_libraries(xllDemo.TSAN PUBLIC LibXLL)
    target_compile_options(xllDemo.TSAN PRIVATE -fsanitize=thread -fPIC -g)
    target_link_options(xllDemo.TSAN PRIVATE -fsanitize=thread -fPIC -g)

    add_executable(exeDemo.TSAN exeDemo.cpp)
    target_link_libraries(exeDemo.TSAN PUBLIC xllDemo.TSAN)
    target_compile_options(exeDemo.TSAN PRIVATE -fsanitize=thread -fPIE -pie -g)
    target_link_options(exeDemo.TSAN PRIVATE -fsanitize=thread -fPIE -pie -g)

    add_library(xllDemo.MSAN SHARED xllDemo.cpp)
    target_link_libraries(xllDemo.MSAN PUBLIC LibXLL)
    target_compile_options(xllDemo.MSAN PRIVATE -fsanitize=memory -fPIC -fno-omit-frame-pointer -g -fno-optimize-sibling-calls -fsanitize-recover -O1)
    target_link_options(xllDemo.MSAN PRIVATE -fsanitize=memory -fPIC -fno-omit-frame-pointer -g -fno-optimize-sibling-calls -fsanitize-recover -O1)

    add_executable(exeDemo.MSAN exeDemo.cpp)
    target_link_libraries(exeDemo.MSAN PUBLIC xllDemo.MSAN)
    target_compile_options(exeDemo.MSAN PRIVATE -fsanitize=memory -fPIE -fno-omit-frame-pointer -g -fno-optimize-sibling-calls -fsanitize-recover -O1)
    target_link_options(exeDemo.MSAN PRIVATE -fsanitize=memory -fPIE -fno-omit-frame-pointer -g -fno-optimize-sibling-calls -fsanitize-recover -O1)

#    add_executable(LibXLL.Tests.TSAN
#            String.cpp
#            Variant.cpp
#            Array.cpp
#    )
#    target_link_libraries(LibXLL.Tests.TSAN PRIVATE Catch2::Catch2WithMain LibXLL)
#    target_compile_options(LibXLL.Tests.TSAN PRIVATE -fsanitize=thread -fno-omit-frame-pointer -g)
#    target_link_options(LibXLL.Tests.TSAN PRIVATE -fsanitize=thread -fno-omit-frame-pointer -g)

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