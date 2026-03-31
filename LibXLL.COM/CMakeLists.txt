#cmake_minimum_required(VERSION 3.20)
#project(xlCOM)
#
#set(CMAKE_CXX_STANDARD 23)

# ---------------------------------------------------------------------------
# Require MSVC — MIDL is an MSVC/Windows SDK feature.
# ---------------------------------------------------------------------------
if(NOT WIN32)
    message(FATAL_ERROR "LibXLL.COM requires Win32.")
endif()

# ---------------------------------------------------------------------------
# MIDL — currently unused. The GUIDs that MIDL would generate (CLSID_Connect,
# LIBID_xlCOMLib) are defined directly in C++ source, and no .tlb is shipped.
# Kept here commented-out in case a type library is needed in the future.
# ---------------------------------------------------------------------------
#set(_midl_hints "")
#if(DEFINED ENV{WindowsSdkDir})
#    if(DEFINED CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
#        list(APPEND _midl_hints
#            "$ENV{WindowsSdkDir}/bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/x64")
#    endif()
#    list(APPEND _midl_hints "$ENV{WindowsSdkDir}/bin/x64")
#endif()
#
#file(GLOB _sdk_versioned_bins
#    "C:/Program Files (x86)/Windows Kits/10/bin/10.*/x64")
#list(SORT _sdk_versioned_bins ORDER DESCENDING)
#list(APPEND _midl_hints ${_sdk_versioned_bins})
#
#find_program(MIDL_EXECUTABLE midl.exe
#    HINTS   ${_midl_hints}
#    DOC     "MIDL compiler (part of the Windows SDK)"
#    NO_DEFAULT_PATH
#)
#
#if(NOT MIDL_EXECUTABLE)
#    message(FATAL_ERROR
#        "MIDL.exe was not found.\n"
#        "Install the Windows SDK and ensure the correct SDK version is selected in CMake.")
#endif()
#
#message(STATUS "Found MIDL: ${MIDL_EXECUTABLE}")
#
#set(MIDL_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/midl")
#file(MAKE_DIRECTORY "${MIDL_OUTPUT_DIR}")
#
#set(MIDL_HEADER  "${MIDL_OUTPUT_DIR}/xlCOM_i.h")
#set(MIDL_IID_SRC "${MIDL_OUTPUT_DIR}/xlCOM_i.c")
#set(MIDL_TLB     "${MIDL_OUTPUT_DIR}/xlCOM.tlb")
#
#add_custom_command(
#    OUTPUT  "${MIDL_HEADER}" "${MIDL_IID_SRC}" "${MIDL_TLB}"
#    COMMAND "${MIDL_EXECUTABLE}"
#            /nologo
#            /char  signed
#            /env   win64
#            /Oicf
#            /out   "${MIDL_OUTPUT_DIR}"
#            /h     "xlCOM_i.h"
#            /iid   "xlCOM_i.c"
#            /tlb   "xlCOM.tlb"
#            "${CMAKE_CURRENT_SOURCE_DIR}/xlCOM.idl"
#    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/xlCOM.idl"
#    COMMENT "Compiling xlCOM.idl with MIDL"
#    VERBATIM
#)

# ---------------------------------------------------------------------------
# xlCOM.dll — the Excel COM add-in
# ---------------------------------------------------------------------------
add_library(xlCOM SHARED
    dllmain.cpp
    Connect.cpp
    #"${MIDL_IID_SRC}"      # MIDL-generated IID/CLSID definitions (unused)
)

# No "lib" prefix — the output should be xlCOM.dll, not libxlCOM.dll.
set_target_properties(xlCOM PROPERTIES PREFIX "")

target_include_directories(xlCOM PRIVATE
    #"${MIDL_OUTPUT_DIR}"           # for xlCOM_i.h (unused)
    "${CMAKE_CURRENT_SOURCE_DIR}"   # for imports.h, Connect.h
)

# Precompiled header for faster builds.
target_precompile_headers(xlCOM PRIVATE pch.h)

target_link_libraries(xlCOM PRIVATE
    ole32       # CoTaskMemAlloc, StringFromGUID2, ...
    oleaut32    # SafeArrayXxx, ...
    uuid        # IID_IUnknown, IID_IDispatch, IID_IClassFactory, ...
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
