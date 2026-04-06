# ---------------------------------------------------------------------------
# xlCOM-Fltk.dll — COM add-in demo using FLTK as task pane content.
# ---------------------------------------------------------------------------

add_library(xlCOM-Fltk SHARED Handlers.cpp)

set_target_properties(xlCOM-Fltk PROPERTIES PREFIX "")

target_include_directories(xlCOM-Fltk PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"      # FltkTaskPane.hpp
)

target_link_libraries(xlCOM-Fltk PRIVATE
    LibXLL.COM      # COM server boilerplate (INTERFACE library)
    stb::stb        # stb_image — header-only PNG decoder
    fltk::fltk      # FLTK toolkit
    foo::rc         # Embedded resources (ribbon XML, images)
)

# ---------------------------------------------------------------------------
# .reg file generation
# ---------------------------------------------------------------------------

set(_dll_path "${CMAKE_CURRENT_BINARY_DIR}/xlCOM-Fltk.dll")
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

add_custom_target(RegisterDll-Fltk
    COMMAND regsvr32 /s "$<TARGET_FILE:xlCOM-Fltk>"
    DEPENDS xlCOM-Fltk
    COMMENT "Registering xlCOM-Fltk.dll (requires elevated privileges for HKCR)"
)

add_custom_target(UnregisterDll-Fltk
    COMMAND regsvr32 /u /s "$<TARGET_FILE:xlCOM-Fltk>"
    DEPENDS xlCOM-Fltk
    COMMENT "Unregistering xlCOM-Fltk.dll"
)

