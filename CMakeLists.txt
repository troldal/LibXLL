cmake_minimum_required(VERSION 3.30)
project(LibXLL)

set(CMAKE_CXX_STANDARD 23)

find_path(GTL_INCLUDE_DIRS "gtl")

add_library(ExcelSDK STATIC ExcelSDK/XLCALL.CPP)
target_include_directories(ExcelSDK PUBLIC ExcelSDK)

#set(CLANG_WARNINGS
#        -Wall
#        -Wextra # reasonable and standard
#        -Wshadow # warn the user if a variable declaration shadows one from a parent context
#        -Wnon-virtual-dtor # warn the user if a class with virtual functions has a non-virtual destructor. This helps
#        # catch hard to track down memory errors
#        -Wold-style-cast # warn for c-style casts
#        -Wcast-align # warn for potential performance problem casts
#        -Wunused # warn on anything being unused
#        -Woverloaded-virtual # warn if you overload (not override) a virtual function
#        -Wpedantic # warn if non-standard C++ is used
#        -Wconversion # warn on type conversions that may lose data
#        -Wsign-conversion # warn on sign conversions
#        -Wnull-dereference # warn if a null dereference is detected
#        -Wdouble-promotion # warn if float is implicit promoted to double
#        -Wformat=2 # warn on security issues around functions that format output (ie printf)
#        -Wimplicit-fallthrough # warn on statements that fallthrough without an explicit annotation
#)

add_library(LibXLL SHARED demo.cpp)
target_link_libraries(LibXLL PUBLIC ExcelSDK)
target_include_directories(LibXLL PRIVATE ${GTL_INCLUDE_DIRS})
#target_compile_options(LibXLL PRIVATE "/J" "/utf-8" "/bigobj")
#target_compile_options(LibXLL PRIVATE ${CLANG_WARNINGS})
set_target_properties(LibXLL PROPERTIES OUTPUT_NAME "example" SUFFIX ".xll")