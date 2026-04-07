# ---------------------------------------------------------------------------
# Require WIN32.
# ---------------------------------------------------------------------------
if(NOT WIN32)
    message(FATAL_ERROR "LibXLL.COM requires Win32.")
endif()


# ---------------------------------------------------------------------------
# LibXLL.COM — header-only INTERFACE library providing the COM server
# boilerplate (COMServer.hpp, TaskPaneControl.hpp, COM/, Utils/, Registry/).
# ---------------------------------------------------------------------------
add_library(LibXLL.COM INTERFACE)

target_include_directories(LibXLL.COM INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}"          # for COM/, Utils/, ActiveX/, COMServer.hpp
)

target_link_libraries(LibXLL.COM INTERFACE
    ole32       # CoTaskMemAlloc, StringFromGUID2, ...
    oleaut32    # SafeArrayXxx, ...
    uuid        # IID_IUnknown, IID_IDispatch, IID_IClassFactory, ...
    fixed_string::fixed_string  # compile-time string NTTP for DispatchCallback
)

target_compile_definitions(LibXLL.COM INTERFACE
    NOMINMAX    # Prevent <windows.h> min/max macros from breaking std::min/std::max
    _CRT_SECURE_NO_WARNINGS
)

# ---------------------------------------------------------------------------
# CMake helper — xlcom_configure_addin() for consumer targets.
# ---------------------------------------------------------------------------
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/XLComAddin.cmake")

