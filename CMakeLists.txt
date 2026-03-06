cmake_minimum_required(VERSION 3.28)
project(LibXLL)

set(CMAKE_CXX_STANDARD 23)


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

if (WIN32)
    set(CPM_FXT_SOURCE "D:/Dev/XLThermo/FXT")
else ()
    set(CPM_FXT_SOURCE "/mnt/d/Dev/XLThermo/FXT")
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

add_subdirectory(LibXLL.Library)

if(PROJECT_IS_TOP_LEVEL)
    add_subdirectory(LibXLL.Mock)
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
