# ---------------------------------------------------------------------------
# xlCOM-Qt.dll — COM add-in demo using Qt Widgets as task pane content.
# Qt Quick (QML) alternative: swap QtTaskPane for QtQuickTaskPane in
# Handlers.cpp and link Qt6::Quick + Qt6::Qml instead.
# ---------------------------------------------------------------------------

if(NOT Qt6_FOUND)
    return()
endif()

add_library(xlCOM-Qt SHARED Handlers.cpp)

set_target_properties(xlCOM-Qt PROPERTIES
    PREFIX ""
    AUTOMOC ON      # Required for Q_OBJECT classes inside Qt task pane headers
)

target_include_directories(xlCOM-Qt PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"      # QtTaskPane.hpp, QtQuickTaskPane.hpp
)

target_link_libraries(xlCOM-Qt PRIVATE
    LibXLL.COM      # COM server boilerplate (INTERFACE library)
    stb::stb        # stb_image — header-only PNG decoder
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets    # Qt Widgets — used by QtTaskPane
    # Qt6::Quick Qt6::Qml   # Enable for QtQuickTaskPane
    foo::rc         # Embedded resources (ribbon XML, images)
)

# ---------------------------------------------------------------------------
# .reg file generation
# ---------------------------------------------------------------------------

set(_dll_path "${CMAKE_CURRENT_BINARY_DIR}/xlCOM-Qt.dll")
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

add_custom_target(RegisterDll-Qt
    COMMAND regsvr32 /s "$<TARGET_FILE:xlCOM-Qt>"
    DEPENDS xlCOM-Qt
    COMMENT "Registering xlCOM-Qt.dll (requires elevated privileges for HKCR)"
)

add_custom_target(UnregisterDll-Qt
    COMMAND regsvr32 /u /s "$<TARGET_FILE:xlCOM-Qt>"
    DEPENDS xlCOM-Qt
    COMMENT "Unregistering xlCOM-Qt.dll"
)

