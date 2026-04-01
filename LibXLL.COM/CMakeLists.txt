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
set(XLCOM_CONNECT_GUID "1E0739E0-A1B5-4AB4-953C-2D8959196A13")

cmrc_add_resource_library(foo-resources ALIAS foo::rc NAMESPACE foo
    ribbon.xml
    button-help_dark48px.png
    button-help_light48px.png
)

# ---------------------------------------------------------------------------
# xlCOM.dll — the Excel COM add-in
# ---------------------------------------------------------------------------
add_library(xlCOM SHARED
    dllmain.cpp
    Connect.cpp
)

# No "lib" prefix — the output should be xlCOM.dll, not libxlCOM.dll.
set_target_properties(xlCOM PROPERTIES PREFIX "")

target_include_directories(xlCOM PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"   # for COMInterfaces.hpp, Connect.hpp
)

target_link_libraries(xlCOM PRIVATE
    ole32       # CoTaskMemAlloc, StringFromGUID2, ...
    oleaut32    # SafeArrayXxx, ...
    uuid        # IID_IUnknown, IID_IDispatch, IID_IClassFactory, ...
    stb::stb    # stb_image — header-only PNG decoder (replaces GDI+)
    foo::rc
)

# Inject the GUID string into the compiler so Connect.h can use __declspec(uuid).
target_compile_definitions(xlCOM PRIVATE
    "XLCOM_CONNECT_GUID=\"${XLCOM_CONNECT_GUID}\""
)

# ---------------------------------------------------------------------------
# .reg file generation
# ---------------------------------------------------------------------------

# Compute the DLL output path at configure time and escape backslashes for
# the .reg string format (\ → \\).  cmake_path converts CMake's forward-slash
# paths to the native Windows form before the escaping step.
set(_dll_path "${CMAKE_CURRENT_BINARY_DIR}/xlCOM.dll")
cmake_path(NATIVE_PATH _dll_path NORMALIZE _dll_path_native)
string(REPLACE "\\" "\\\\" XLCOM_DLL_PATH_REG "${_dll_path_native}")
unset(_dll_path)
unset(_dll_path_native)

# Both @XLCOM_CONNECT_GUID@ and @XLCOM_DLL_PATH_REG@ are substituted in a
# single configure_file pass.  Output lands in the per-platform build directory
# so each toolchain/configuration gets its own correctly-pathed .reg files.
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Registry/register_dev.reg.in"
    "${CMAKE_CURRENT_BINARY_DIR}/register_dev.reg"
    @ONLY
)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Registry/unregister_dev.reg.in"
    "${CMAKE_CURRENT_BINARY_DIR}/unregister_dev.reg"
    @ONLY
)


# ---------------------------------------------------------------------------
# Convenience targets — run as Administrator for HKCR writes.
# ---------------------------------------------------------------------------
add_custom_target(RegisterDll
    COMMAND regsvr32 /s "$<TARGET_FILE:xlCOM>"
    DEPENDS xlCOM
    COMMENT "Registering xlCOM.dll (requires elevated privileges for HKCR)"
)

add_custom_target(UnregisterDll
    COMMAND regsvr32 /u /s "$<TARGET_FILE:xlCOM>"
    DEPENDS xlCOM
    COMMENT "Unregistering xlCOM.dll"
)
