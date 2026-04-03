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
set(XLCOM_TASKPANE_GUID "7C2D4F8A-3E1B-4A5C-9D6E-8F0B2A1C3D5E")

cmrc_add_resource_library(foo-resources ALIAS foo::rc NAMESPACE foo
    ribbon.xml
    button-help_dark48px.png
    button-help_light48px.png
    Inter-VariableFont.ttf
)

# ---------------------------------------------------------------------------
# xlCOM.dll — the Excel COM add-in
# ---------------------------------------------------------------------------
add_library(xlCOM SHARED
    Handlers.cpp
    # ActiveX/QtQuickTaskPane.hpp   # Listed so AUTOMOC discovers Q_OBJECT in the header
    # ImGui Win32 + DX11 backends — compiled directly into xlCOM so the DLL
    # carries its own copy and does not conflict with other imgui consumers.
    "${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp"
)

# No "lib" prefix — the output should be xlCOM.dll, not libxlCOM.dll.
set_target_properties(xlCOM PROPERTIES
    PREFIX ""
    # AUTOMOC ON   # Enable when using a Qt content type (e.g. QtQuickTaskPane)
)

target_include_directories(xlCOM PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"          # for COM/, Utils/, COMServer.hpp
    "${imgui_SOURCE_DIR}/backends"         # imgui_impl_win32.h, imgui_impl_dx11.h
)

target_link_libraries(xlCOM PRIVATE
    ole32       # CoTaskMemAlloc, StringFromGUID2, ...
    oleaut32    # SafeArrayXxx, ...
    uuid        # IID_IUnknown, IID_IDispatch, IID_IClassFactory, ...
    stb::stb    # stb_image — header-only PNG decoder (replaces GDI+)
    fixed_string::fixed_string  # compile-time string NTTP for DispatchCallback
    imgui                       # Dear ImGui core — used by ActiveX/ImGuiTaskPane.hpp
    d3d11                       # Direct3D 11 — swap chain / device / context
    dxgi                        # DXGI — adapter / swap chain creation
    d3dcompiler                 # D3DCompile — shader compilation (imgui_impl_dx11)
    dwmapi                      # DWM — dark-mode support in imgui_impl_win32
    # wx::core wx::base           # wxWidgets — used by ActiveX/WxTaskPane.hpp
    # Qt6::Widgets                  # Qt Widgets — used by ActiveX/QtTaskPane.hpp
    # fltk::fltk                    # FLTK — used by ActiveX/FltkTaskPane.hpp
    # Qt6::Quick Qt6::Qml           # Qt Quick — used by ActiveX/QtQuickTaskPane.hpp
    foo::rc
)

# Inject the GUID string into the compiler so Connect.h can use __declspec(uuid).
target_compile_definitions(xlCOM PRIVATE
    "XLCOM_CONNECT_GUID=\"${XLCOM_CONNECT_GUID}\""
    NOMINMAX    # Prevent <windows.h> min/max macros from breaking std::min/std::max
    # wxWidgets with MSVC /sdl promotes C4996 to an error; this suppresses it
    # at the preprocessor level so /sdl has nothing to promote.
    _CRT_SECURE_NO_WARNINGS
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
