# ---------------------------------------------------------------------------
# Require WIN32.
# ---------------------------------------------------------------------------
if(NOT WIN32)
    return()
endif()

# ---------------------------------------------------------------------------
# Shared embedded resources — ribbon XML, button images, font.
# Defined here so all framework subdirectories can link against foo::rc.
# ---------------------------------------------------------------------------
cmrc_add_resource_library(foo-resources ALIAS foo::rc NAMESPACE foo
    Resources/XML/ribbon.xml
    Resources/Images/button-help_dark48px.png
    Resources/Images/button-help_light48px.png
    Resources/Fonts/Inter-VariableFont.ttf
)

# ---------------------------------------------------------------------------
# Per-framework COM add-in demos — one DLL per GUI framework.
# ---------------------------------------------------------------------------
if(LIBXLL_DEMOS_IMGUI)
    add_subdirectory(imgui)
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

if(LIBXLL_DEMOS_WIN32)
    add_subdirectory(win32)
endif()
