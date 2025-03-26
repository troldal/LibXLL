add_library(xllDemo SHARED xllDemo.cpp)
target_link_libraries(xllDemo PUBLIC LibXLL)
set_target_properties(xllDemo PROPERTIES OUTPUT_NAME "example" SUFFIX ".xll")

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(xllDemo PRIVATE ${CLANG_WARNINGS})
endif ()


add_executable(exeDemo exeDemo.cpp)
target_link_libraries(exeDemo PUBLIC LibXLL)