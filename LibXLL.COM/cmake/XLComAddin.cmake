# ---------------------------------------------------------------------------
# XLComAddin.cmake — helper function for COM add-in targets using LibXLL.COM
#
# Usage:
#   xlcom_configure_addin(<target>
#       ADDIN_GUID              "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX"
#       ADDIN_PROGID            "Company.AddinName.Connect"
#       ADDIN_FRIENDLY_NAME     "My Excel Add-in"
#       ADDIN_DESCRIPTION       "My Excel COM Add-in description"
#       TASKPANE_GUID           "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX"
#       TASKPANE_PROGID         "Company.AddinName.TaskPane"
#       TASKPANE_FRIENDLY_NAME  "My Task Pane Control"
#   )
#
# This function:
#   1. Defines CMake variables <TARGET>_ADDIN_GUID, <TARGET>_ADDIN_PROGID,
#      <TARGET>_ADDIN_FRIENDLY_NAME, <TARGET>_ADDIN_DESCRIPTION, and
#      <TARGET>_SETUP in the caller's scope, where <TARGET> is the target
#      name uppercased with non-alphanumeric characters replaced by "_".
#      <TARGET>_SETUP contains a ready-to-paste static com::AddIn definition.
#   2. Adds PRIVATE compile definitions on the target for the same four
#      <TARGET>_ADDIN_* variables so they are available as preprocessor macros.
#   3. Configures register_dev.reg and unregister_dev.reg from the shared
#      templates in LibXLL.COM/Registry/, substituting all per-addin values.
#   4. Adds RegisterDll-<target> and UnregisterDll-<target> convenience
#      custom targets (require elevated privileges for HKCR writes).
#
# The C++ add-in identity (GUID, ProgID, friendly name, description) is
# supplied via the com::AddIn class declared in Handlers.cpp — see
# AddIn.hpp for details.
#
# Note: The TASKPANE_* values are used only for .reg file generation.
# The task pane CLSID and ProgID visible to C++ are supplied through the
# TaskPaneTraits struct defined alongside the TaskPaneControl instantiation
# in the consumer's Handlers.cpp (see ActiveX/TaskPaneControl.hpp).
# ---------------------------------------------------------------------------

function(xlcom_configure_addin target)
    cmake_parse_arguments(ARG ""
        "ADDIN_GUID;ADDIN_PROGID;ADDIN_FRIENDLY_NAME;ADDIN_DESCRIPTION;\
TASKPANE_GUID;TASKPANE_PROGID;TASKPANE_FRIENDLY_NAME"
        "" ${ARGN})

    foreach(_req ADDIN_GUID ADDIN_PROGID ADDIN_FRIENDLY_NAME ADDIN_DESCRIPTION
                 TASKPANE_GUID TASKPANE_PROGID TASKPANE_FRIENDLY_NAME)
        if(NOT ARG_${_req})
            message(FATAL_ERROR "xlcom_configure_addin(${target}): ${_req} is required")
        endif()
    endforeach()

    # ---- Normalised target prefix (uppercase, non-alnum → underscore) -------
    string(TOUPPER "${target}" _prefix)
    string(REGEX REPLACE "[^A-Z0-9]" "_" _prefix "${_prefix}")

    set(${_prefix}_ADDIN_GUID            "${ARG_ADDIN_GUID}"            PARENT_SCOPE)
    set(${_prefix}_ADDIN_PROGID          "${ARG_ADDIN_PROGID}"          PARENT_SCOPE)
    set(${_prefix}_ADDIN_FRIENDLY_NAME   "${ARG_ADDIN_FRIENDLY_NAME}"   PARENT_SCOPE)
    set(${_prefix}_ADDIN_DESCRIPTION     "${ARG_ADDIN_DESCRIPTION}"     PARENT_SCOPE)

    # A ready-to-paste C++ snippet that constructs the com::AddIn for this target.
    string(CONCAT _setup
        "static const com::AddIn s_addin(\n"
        "    \"${ARG_ADDIN_GUID}\",\n"
        "    L\"${ARG_ADDIN_PROGID}\",\n"
        "    L\"${ARG_ADDIN_FRIENDLY_NAME}\",\n"
        "    L\"${ARG_ADDIN_DESCRIPTION}\"\n"
        ");\n"
    )
    set(${_prefix}_SETUP "${_setup}" PARENT_SCOPE)

    # ---- Compile definitions on the target -----------------------------------
    target_compile_definitions(${target} PRIVATE
        "${_prefix}_ADDIN_GUID=\"${ARG_ADDIN_GUID}\""
        "${_prefix}_ADDIN_PROGID=L\"${ARG_ADDIN_PROGID}\""
        "${_prefix}_ADDIN_FRIENDLY_NAME=L\"${ARG_ADDIN_FRIENDLY_NAME}\""
        "${_prefix}_ADDIN_DESCRIPTION=L\"${ARG_ADDIN_DESCRIPTION}\""

    )

    # ---- .reg file generation -----------------------------------------------
    set(XLLCOM_ADDIN_GUID            "${ARG_ADDIN_GUID}")
    set(XLLCOM_ADDIN_PROGID          "${ARG_ADDIN_PROGID}")
    set(XLLCOM_ADDIN_FRIENDLY_NAME   "${ARG_ADDIN_FRIENDLY_NAME}")
    set(XLLCOM_ADDIN_DESCRIPTION     "${ARG_ADDIN_DESCRIPTION}")
    set(XLLCOM_TASKPANE_GUID         "${ARG_TASKPANE_GUID}")
    set(XLLCOM_TASKPANE_PROGID       "${ARG_TASKPANE_PROGID}")
    set(XLLCOM_TASKPANE_FRIENDLY_NAME "${ARG_TASKPANE_FRIENDLY_NAME}")

    # Build the escaped DLL path for the .reg file.
    set(_dll_path "${CMAKE_CURRENT_BINARY_DIR}/${target}.dll")
    cmake_path(NATIVE_PATH _dll_path NORMALIZE _dll_path_native)
    string(REPLACE "\\" "\\\\" XLLCOM_DLL_PATH_REG "${_dll_path_native}")
    unset(_dll_path)
    unset(_dll_path_native)

    set(_reg_dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../Registry")
    configure_file(
        "${_reg_dir}/register_dev.reg.in"
        "${CMAKE_CURRENT_BINARY_DIR}/register_dev.reg"
        @ONLY
    )
    configure_file(
        "${_reg_dir}/unregister_dev.reg.in"
        "${CMAKE_CURRENT_BINARY_DIR}/unregister_dev.reg"
        @ONLY
    )

    # ---- register / unregister convenience targets --------------------------
    add_custom_target(RegisterDll-${target}
        COMMAND regsvr32 /s "$<TARGET_FILE:${target}>"
        DEPENDS ${target}
        COMMENT "Registering ${target}.dll (requires elevated privileges for HKCR)"
    )
    add_custom_target(UnregisterDll-${target}
        COMMAND regsvr32 /u /s "$<TARGET_FILE:${target}>"
        DEPENDS ${target}
        COMMENT "Unregistering ${target}.dll"
    )
endfunction()

