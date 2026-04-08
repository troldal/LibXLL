if (NOT WIN32)
    return()
endif()

if(LIBXLL_DEMOS_FLTK)
    add_subdirectory(fltk)
endif()

if(LIBXLL_DEMOS_WX)
    add_subdirectory(wx)
endif()

if(LIBXLL_DEMOS_QT)
    add_subdirectory(Qt)
endif()

if(LIBXLL_DEMOS_IMGUI)
    add_subdirectory(imgui)
endif()
