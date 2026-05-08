cmake_minimum_required(VERSION 3.28)
project(LibXLL)

set(CMAKE_CXX_STANDARD 23)

include(cmake/CMakeRC.cmake)

#==============================================================================
# Fetch dependency manager
#==============================================================================
include(cmake/get_cpm.cmake)
if(PROJECT_IS_TOP_LEVEL)
    set(CPM_SOURCE_CACHE ${CMAKE_CURRENT_LIST_DIR}/_packages)
endif()

#==============================================================================
# Fetch dependencies
#==============================================================================

if (WIN32 OR EMSCRIPTEN)
    set(CPM_FXT_SOURCE "C:/Dev/XLThermo/FXT")
else ()
    set(CPM_FXT_SOURCE "/mnt/c/Dev/XLThermo/FXT")
endif ()

CPMAddPackage(
        NAME FXT
        GITHUB_REPOSITORY troldal/FXT
        GIT_TAG main
        OPTIONS "FXT_USE_TL_EXPECTED OFF" "FXT_USE_TL_OPTIONAL OFF"
)

CPMAddPackage(
        NAME nlohmann_json
        VERSION 3.12.0
        # the git repo is incredibly large, so we download the archived include directory
        URL https://github.com/nlohmann/json/releases/download/v3.12.0/include.zip
)

CPMAddPackage(
        NAME sigslot
        GITHUB_REPOSITORY palacaze/sigslot
        GIT_TAG v1.2.3
)

CPMAddPackage(
        NAME stb
        GITHUB_REPOSITORY nothings/stb
        GIT_TAG master
)

CPMAddPackage(
        NAME picosha2
        GITHUB_REPOSITORY okdshin/PicoSHA2
        GIT_TAG v1.0.1
)

# stb ships no CMakeLists.txt — wrap it in an INTERFACE target manually.
if(stb_ADDED)
    add_library(stb INTERFACE)
    add_library(stb::stb ALIAS stb)
    target_include_directories(stb INTERFACE "${stb_SOURCE_DIR}")
endif()

option(IMGUI_USE_THREAD_LOCAL_CONTEXT_TLS
        "Append a thread-local GImGui override to imconfig.h at configure time"
        ON)

# These options are read by the community imgui CMakeLists.txt that imguiex
# injects, so they must be set BEFORE CPMAddPackage(imguiex) processes imgui.
if(NOT EMSCRIPTEN)
    set(IMGUI_EXAMPLES TRUE)
endif()
set(IMGUI_DEMO TRUE)
set(IMGUI_ENABLE_STDLIB_SUPPORT TRUE)
set(IMGUI_IMPL_SDL    OFF) # We use Win32 + Direct3D backends.
set(IMGUI_IMPL_GLFW   OFF)
set(IMGUI_IMPL_GLUT   OFF)
set(IMGUI_IMPL_VULKAN OFF)
set(IMGUIEX_ENABLE_FREETYPE ON)

CPMAddPackage(
        NAME imguiex
        GITHUB_REPOSITORY troldal/ImGuiEx
        GIT_TAG main
)

# imgui is fetched by imguiex via FetchContent (not CPM), so imgui_ADDED is
# never set.  Use imgui_SOURCE_DIR (populated globally by FetchContent) instead.
# Append the thread-local context override to imconfig.h after imguiex has
# populated imgui so that imgui_SOURCE_DIR is available.
# A sentinel macro prevents duplicate appends on repeated configure runs.
if(IMGUI_USE_THREAD_LOCAL_CONTEXT_TLS AND imgui_SOURCE_DIR)
    set(_imconfig "${imgui_SOURCE_DIR}/imconfig.h")
    file(READ "${_imconfig}" _imconfig_content)
    if(NOT _imconfig_content MATCHES "IMGUI_TLS_CONTEXT_OVERRIDE")
        file(APPEND "${_imconfig}"
                "\n// Thread-local ImGui context override (injected by CMake).\n"
                "// Enable by defining IMGUI_USE_THREAD_LOCAL_CONTEXT_TLS.\n"
                "#ifndef IMGUI_TLS_CONTEXT_OVERRIDE\n"
                "#define IMGUI_TLS_CONTEXT_OVERRIDE\n"
                "struct ImGuiContext;\n"
                "\n"
                "extern thread_local ImGuiContext* MyImGuiTLS;\n"
                "\n"
                "#define GImGui MyImGuiTLS\n"
                "#endif // IMGUI_TLS_CONTEXT_OVERRIDE\n"
        )
        message(STATUS "imgui: appended thread-local GImGui override to imconfig.h")
    endif()
    unset(_imconfig)
    unset(_imconfig_content)
endif()

# imgui is already added by imguiex's own FetchContent + add_subdirectory —
# do NOT call add_subdirectory again, which would create a duplicate target.


if (WIN32)
    # Qt is installed externally — use find_package rather than CPM.
    # Select the correct Qt variant for the compiler in use:
    #   MSVC / clang-cl → msvc2022_64
    #   GCC  (MinGW)    → mingw_64
    if (MSVC OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC"))
        set(_QT_ROOT "C:/Frameworks/Qt/6.10.1/msvc2022_64")
    else ()
        set(_QT_ROOT "C:/Frameworks/Qt/6.10.1/mingw_64")
    endif ()
    if (EXISTS "${_QT_ROOT}")
        list(APPEND CMAKE_PREFIX_PATH "${_QT_ROOT}")
    endif ()
    find_package(Qt6 COMPONENTS Core Gui Widgets
                    OPTIONAL_COMPONENTS Quick Qml QuickControls2
                    QUIET)

    CPMAddPackage(
            NAME fltk
            GITHUB_REPOSITORY fltk/fltk
            GIT_TAG release-1.4.4
            OPTIONS
            "FLTK_BUILD_TEST OFF"
            "FLTK_BUILD_EXAMPLES OFF"
            "FLTK_BUILD_HTML_DOCS OFF"
    )

    CPMAddPackage(
            NAME wxWidgets
            GITHUB_REPOSITORY wxWidgets/wxWidgets
            GIT_TAG v3.3.2
            OPTIONS
                # Avoid the WebView2 SDK download/extraction that fails on Windows
                # with a "Can't restore time" tar error.
                "wxUSE_WEBVIEW_EDGE OFF"
                # Work around a bug in wxWidgets 3.3.2: gridsel.h declares
                # wxGridSelection (a dllexport class) with a
                # std::unique_ptr<wxGridPrivate::SelectionShape> member, where
                # SelectionShape is only forward-declared. MSVC and GCC tolerate
                # this; clang-cl correctly rejects it with a hard error because
                # dllexport forces instantiation of the implicit destructor, which
                # requires the complete type via std::default_delete. Disabling the
                # grid prevents the offending translation unit from being compiled.
                "wxUSE_GRID OFF"
                # Build wxWidgets as static libraries so that targets linking wx
                # (wxDemo.exe, xllDemoWxDialog.xll) are self-contained.
                # XLLs MUST be statically linked to wx: when Excel calls
                # LoadLibrary on an XLL, Windows resolves the XLL's DLL imports
                # using the standard search order (exe dir, PATH, system dirs) –
                # the XLL's own directory is NOT searched.  Any missing wx DLLs
                # cause LoadLibrary to fail, which Excel reports as "file format
                # and extension don't match".  Static linking eliminates the
                # external DLL dependency entirely.
                "BUILD_SHARED_LIBS OFF"
    )
endif()

add_subdirectory(LibXLL.Library)
if(WIN32)
    add_subdirectory(LibXLL.COM)
endif()

if(PROJECT_IS_TOP_LEVEL)
    option(LIBXLL_ENABLE_MOCKXL
            "Build LibXLL.MockXL (mock Excel server used by addin tests and demos)"
            OFF)
    if(LIBXLL_ENABLE_MOCKXL)
        CPMAddPackage(
                NAME Boost
                VERSION 1.90.0 # Versions less than 1.85.0 may need patches for installation targets.
                URL https://github.com/boostorg/boost/releases/download/boost-1.90.0/boost-1.90.0-cmake.tar.xz
                URL_HASH SHA256=aca59f889f0f32028ad88ba6764582b63c916ce5f77b31289ad19421a96c555f
                SYSTEM YES  # Marks Boost include directories as SYSTEM to suppress warnings from Boost headers.
                OPTIONS "BOOST_ENABLE_CMAKE ON" "BOOST_SKIP_INSTALL_RULES ON" # Set `OFF` for installation
                "BUILD_SHARED_LIBS OFF" "BOOST_INCLUDE_LIBRARIES dll\\\;nowide" # Note the escapes!
        )
        add_subdirectory(LibXLL.MockXL)
    endif()
    add_subdirectory(LibXLL.Demos)
    add_subdirectory(LibXLL.Tests)

    #==============================================================================
    # Documentation (Doxygen → Breathe → Sphinx)
    #==============================================================================
    find_package(Doxygen)
    find_program(SPHINX_EXECUTABLE
            NAMES sphinx-build
            PATHS "C:/Python314/Scripts"
            DOC "Sphinx documentation generator"
    )

    if(DOXYGEN_FOUND AND SPHINX_EXECUTABLE)
        get_target_property(LIBXLL_INCLUDE_DIRS LibXLL INTERFACE_INCLUDE_DIRECTORIES)
        file(GLOB_RECURSE LIBXLL_PUBLIC_HEADERS ${LIBXLL_INCLUDE_DIRS}/*.hpp)

        set(DOCS_DIR          ${CMAKE_CURRENT_SOURCE_DIR}/LibXLL.Docs)
        set(DOCS_BUILD_DIR    ${CMAKE_CURRENT_BINARY_DIR}/docs)
        set(DOXYGEN_OUTPUT_DIR  ${DOCS_BUILD_DIR})
        set(DOXYGEN_XML_DIR   ${DOCS_BUILD_DIR}/xml)
        set(DOXYGEN_XML_INDEX ${DOXYGEN_XML_DIR}/index.xml)
        set(SPHINX_INDEX_FILE ${DOCS_BUILD_DIR}/sphinx/index.html)

        # Stamp the build-tree output path into a generated Doxyfile.
        # The source Doxyfile contains OUTPUT_DIRECTORY = @DOXYGEN_OUTPUT_DIR@
        # which configure_file replaces with the actual build-tree path.
        set(DOXYFILE_IN  ${DOCS_DIR}/Doxyfile.in)
        set(DOXYFILE_OUT ${DOCS_BUILD_DIR}/Doxyfile)
        configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

        # Step 1 – run Doxygen to produce XML consumed by Breathe.
        add_custom_command(
                OUTPUT  ${DOXYGEN_XML_INDEX}
                DEPENDS ${LIBXLL_PUBLIC_HEADERS}
                COMMAND ${DOXYGEN_EXECUTABLE} "${DOXYFILE_OUT}"
                WORKING_DIRECTORY ${DOCS_DIR}
                MAIN_DEPENDENCY   ${DOXYFILE_OUT}
                COMMENT "Doxygen: generating XML for Breathe"
                VERBATIM
        )

        # Step 2 – run Sphinx (with the Breathe extension) to produce HTML.
        add_custom_command(
                OUTPUT  ${SPHINX_INDEX_FILE}
                DEPENDS ${DOXYGEN_XML_INDEX}
                        ${DOCS_DIR}/conf.py
                        ${DOCS_DIR}/index.rst
                        ${DOCS_DIR}/introduction.rst
                        ${DOCS_DIR}/getting-started/index.rst
                        ${DOCS_DIR}/getting-started/installation.rst
                        ${DOCS_DIR}/getting-started/hello-world.rst
                        ${DOCS_DIR}/user-guide/index.rst
                        ${DOCS_DIR}/user-guide/types.rst
                        ${DOCS_DIR}/user-guide/functions.rst
                        ${DOCS_DIR}/user-guide/arguments.rst
                        ${DOCS_DIR}/user-guide/lifecycle.rst
                        ${DOCS_DIR}/api/index.rst
                        ${DOCS_DIR}/api/types.rst
                        ${DOCS_DIR}/api/registration.rst
                        ${DOCS_DIR}/api/auto.rst
                COMMAND ${SPHINX_EXECUTABLE}
                            -b html
                            -Dbreathe_projects.LibXLL=${DOXYGEN_XML_DIR}
                            ${DOCS_DIR}
                            ${DOCS_BUILD_DIR}/sphinx
                COMMENT "Sphinx: building HTML documentation"
                VERBATIM
        )

        add_custom_target(docs DEPENDS ${SPHINX_INDEX_FILE})

        # Open the generated docs in the default system browser.
        # Usage: cmake --build <build-dir> --target docs-open
        if(WIN32)
            set(OPEN_COMMAND cmd /c start "")
        elseif(APPLE)
            set(OPEN_COMMAND open)
        else()
            set(OPEN_COMMAND xdg-open)
        endif()

        add_custom_target(docs-open
                COMMAND ${OPEN_COMMAND} "${SPHINX_INDEX_FILE}"
                DEPENDS docs
                COMMENT "Opening documentation in the default browser"
                VERBATIM
        )

    elseif(NOT DOXYGEN_FOUND)
        message(STATUS "Doxygen not found – 'docs' target will not be available")
    elseif(NOT SPHINX_EXECUTABLE)
        message(STATUS "sphinx-build not found – 'docs' target will not be available")
    endif()
endif()
