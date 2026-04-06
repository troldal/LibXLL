# ---------------------------------------------------------------------------
# xlCOM-Wx.dll — COM add-in demo using wxWidgets as task pane content.
# ---------------------------------------------------------------------------

add_library(xlCOM-Wx SHARED Handlers.cpp)

set_target_properties(xlCOM-Wx PROPERTIES PREFIX "")

target_include_directories(xlCOM-Wx PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"      # WxTaskPane.hpp
)

target_link_libraries(xlCOM-Wx PRIVATE
    LibXLL.COM      # COM server boilerplate (INTERFACE library)
    stb::stb        # stb_image — header-only PNG decoder
    wx::core        # wxWidgets core
    wx::base        # wxWidgets base
    foo::rc         # Embedded resources (ribbon XML, images)
)

target_compile_definitions(xlCOM-Wx PRIVATE
    # /sdl promotes C4996 to an error after /wd4996 has suppressed it.
    # _CRT_SECURE_NO_WARNINGS removes the deprecation markup at the
    # preprocessor level so /sdl has nothing to promote.
    $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
)

# ---------------------------------------------------------------------------
# .reg file generation
# ---------------------------------------------------------------------------

set(_dll_path "${CMAKE_CURRENT_BINARY_DIR}/xlCOM-Wx.dll")
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

add_custom_target(RegisterDll-Wx
    COMMAND regsvr32 /s "$<TARGET_FILE:xlCOM-Wx>"
    DEPENDS xlCOM-Wx
    COMMENT "Registering xlCOM-Wx.dll (requires elevated privileges for HKCR)"
)

add_custom_target(UnregisterDll-Wx
    COMMAND regsvr32 /u /s "$<TARGET_FILE:xlCOM-Wx>"
    DEPENDS xlCOM-Wx
    COMMENT "Unregistering xlCOM-Wx.dll"
)

