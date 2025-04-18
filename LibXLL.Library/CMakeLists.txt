if (NOT WIN32)
    add_library(LinuxUtils INTERFACE)
    target_include_directories(LinuxUtils INTERFACE Utils/linux)
endif (NOT WIN32)

add_library(CommonUtils INTERFACE)
target_include_directories(CommonUtils INTERFACE Utils/common)

add_library(ExcelSDK INTERFACE)
target_include_directories(ExcelSDK INTERFACE ExcelSDK)

add_library(LibXLL INTERFACE)
target_include_directories(ExcelSDK INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(LibXLL INTERFACE CommonUtils ExcelSDK fxt::fxt)

if (NOT WIN32)
    target_link_libraries(LibXLL INTERFACE LinuxUtils)
endif (NOT WIN32)