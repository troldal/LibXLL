# ---------------------------------------------------------------------------
# xlCOM-Win32.dll — COM add-in demo using pure Win32 as task pane content.
# No additional GUI framework required beyond the Windows SDK.
# ---------------------------------------------------------------------------

add_library(xlCOM-Win32 SHARED Handlers.cpp)

set_target_properties(xlCOM-Win32 PROPERTIES PREFIX "")

target_include_directories(xlCOM-Win32 PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"      # Win32TaskPane.hpp
)

target_link_libraries(xlCOM-Win32 PRIVATE
    LibXLL.COM      # COM server boilerplate (INTERFACE library)
    stb::stb        # stb_image — header-only PNG decoder
    foo::rc         # Embedded resources (ribbon XML, images)
)

# ---------------------------------------------------------------------------
# .reg file generation
# ---------------------------------------------------------------------------

set(_dll_path "${CMAKE_CURRENT_BINARY_DIR}/xlCOM-Win32.dll")
cmake_path(NATIVE_PATH _dll_path NORMALIZE _dll_path_native)
string(REPLACE "\\" "\\\\" XLCOM_DLL_PATH_REG "${_dll_path_native}")
unset(_dll_path)
unset(_dll_path_native)

configure_file(
    "${CMAKE_SOURCE_DIR}/LibXLL.COM/Registry/register_dev.reg.in"
    "${CMAKE_CURRENT_BINARY_DIR}/register_dev.reg"
    @ONLY
)
configure_file(
    "${CMAKE_SOURCE_DIR}/LibXLL.COM/Registry/unregister_dev.reg.in"
    "${CMAKE_CURRENT_BINARY_DIR}/unregister_dev.reg"
    @ONLY
)

add_custom_target(RegisterDll-Win32
    COMMAND regsvr32 /s "$<TARGET_FILE:xlCOM-Win32>"
    DEPENDS xlCOM-Win32
    COMMENT "Registering xlCOM-Win32.dll (requires elevated privileges for HKCR)"
)

add_custom_target(UnregisterDll-Win32
    COMMAND regsvr32 /u /s "$<TARGET_FILE:xlCOM-Win32>"
    DEPENDS xlCOM-Win32
    COMMENT "Unregistering xlCOM-Win32.dll"
)

