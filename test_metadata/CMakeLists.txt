cmake_minimum_required(VERSION 3.20)
project(TestMetadataExpected CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(test_metadata_expected ../test_metadata_expected.cpp)

target_include_directories(test_metadata_expected PRIVATE
    ${CMAKE_SOURCE_DIR}/../LibXLL.Library
    ${CMAKE_SOURCE_DIR}/../LibXLL.Library/ExcelSDK
    ${CMAKE_SOURCE_DIR}/../cmake-build-debug-clang-21/_deps/expected-src/include
    ${CMAKE_SOURCE_DIR}/../cmake-build-debug-clang-21/_deps/optional-src/include
    ${CMAKE_SOURCE_DIR}/../cmake-build-debug-clang-21/_deps/fixed_string-src/include
    ${CMAKE_SOURCE_DIR}/../../FXT/include
)

