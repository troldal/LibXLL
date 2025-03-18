cmake_minimum_required(VERSION 3.30)
project(LibXLL)

set(CMAKE_CXX_STANDARD 23)

find_path(GTL_INCLUDE_DIRS "gtl")

add_library(ExcelSDK STATIC ExcelSDK/XLCALL.CPP)
target_include_directories(ExcelSDK PUBLIC ExcelSDK)

add_library(LibXLL SHARED demo.cpp)
target_link_libraries(LibXLL PUBLIC ExcelSDK)
target_include_directories(LibXLL PRIVATE ${GTL_INCLUDE_DIRS})
set_target_properties(LibXLL PROPERTIES OUTPUT_NAME "example" SUFFIX ".xll")