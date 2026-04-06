# ---------------------------------------------------------------------------
# Require WIN32.
# ---------------------------------------------------------------------------
if(NOT WIN32)
    message(FATAL_ERROR "LibXLL.COM requires Win32.")
endif()

# ---------------------------------------------------------------------------
# Single source of truth for the COM class GUID.
# Change this value here only — Connect.hpp and the .reg files update automatically.
# ---------------------------------------------------------------------------
set(XLCOM_CONNECT_GUID  "1E0739E0-A1B5-4AB4-953C-2D8959196A13" CACHE STRING "" FORCE)
set(XLCOM_TASKPANE_GUID "7C2D4F8A-3E1B-4A5C-9D6E-8F0B2A1C3D5E" CACHE STRING "" FORCE)

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
    "XLCOM_CONNECT_GUID=\"${XLCOM_CONNECT_GUID}\""
    NOMINMAX    # Prevent <windows.h> min/max macros from breaking std::min/std::max
    _CRT_SECURE_NO_WARNINGS
)
