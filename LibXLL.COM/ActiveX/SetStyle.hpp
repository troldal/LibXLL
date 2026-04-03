#pragma once

#include <imgui.h>

inline void SetStyleRestDark()
{
    // Rest style by AaronBeardless from ImThemes
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha                            = 1.0f;
    style.DisabledAlpha                    = 0.5f;
    style.WindowPadding                    = ImVec2(13.0f, 10.0f);
    style.WindowRounding                   = 0.0f;
    style.WindowBorderSize                 = 1.0f;
    style.WindowMinSize                    = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign                 = ImVec2(0.5f, 0.5f);
    style.WindowMenuButtonPosition         = ImGuiDir_Right;
    style.ChildRounding                    = 3.0f;
    style.ChildBorderSize                  = 1.0f;
    style.PopupRounding                    = 5.0f;
    style.PopupBorderSize                  = 1.0f;
    style.FramePadding                     = ImVec2(20.0f, 8.1f);
    style.FrameRounding                    = 2.0f;
    style.FrameBorderSize                  = 0.0f;
    style.ItemSpacing                      = ImVec2(3.0f, 3.0f);
    style.ItemInnerSpacing                 = ImVec2(3.0f, 8.0f);
    style.CellPadding                      = ImVec2(6.0f, 14.1f);
    style.IndentSpacing                    = 0.0f;
    style.ColumnsMinSpacing                = 10.0f;
    style.ScrollbarSize                    = 10.0f;
    style.ScrollbarRounding                = 2.0f;
    style.GrabMinSize                      = 12.1f;
    style.GrabRounding                     = 1.0f;
    style.TabRounding                      = 2.0f;
    style.TabBorderSize                    = 0.0f;
    style.TabCloseButtonMinWidthUnselected = 5.0f;
    style.ColorButtonPosition              = ImGuiDir_Right;
    style.ButtonTextAlign                  = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign              = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text]                  = ImVec4(0.98039216f, 0.98039216f, 0.98039216f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.49803922f, 0.49803922f, 0.49803922f, 1.0f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.09411765f, 0.09411765f, 0.09411765f, 1.0f);
    style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.15686275f, 0.15686275f, 0.15686275f, 1.0f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.09411765f, 0.09411765f, 0.09411765f, 1.0f);
    style.Colors[ImGuiCol_Border]                = ImVec4(1.0f, 1.0f, 1.0f, 0.09803922f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(1.0f, 1.0f, 1.0f, 0.09803922f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(1.0f, 1.0f, 1.0f, 0.15686275f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.0f, 0.0f, 0.0f, 0.047058824f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.11764706f, 0.11764706f, 0.11764706f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.15686275f, 0.15686275f, 0.15686275f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.11764706f, 0.11764706f, 0.11764706f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.0f, 0.0f, 0.0f, 0.10980392f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(1.0f, 1.0f, 1.0f, 0.39215687f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(1.0f, 1.0f, 1.0f, 0.47058824f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.0f, 0.0f, 0.0f, 0.09803922f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(1.0f, 1.0f, 1.0f, 0.39215687f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(1.0f, 1.0f, 1.0f, 0.3137255f);
    style.Colors[ImGuiCol_Button]                = ImVec4(1.0f, 1.0f, 1.0f, 0.09803922f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(1.0f, 1.0f, 1.0f, 0.15686275f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.0f, 0.0f, 0.0f, 0.047058824f);
    style.Colors[ImGuiCol_Header]                = ImVec4(1.0f, 1.0f, 1.0f, 0.09803922f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(1.0f, 1.0f, 1.0f, 0.15686275f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.0f, 0.0f, 0.0f, 0.047058824f);
    style.Colors[ImGuiCol_Separator]             = ImVec4(1.0f, 1.0f, 1.0f, 0.15686275f);
    style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(1.0f, 1.0f, 1.0f, 0.23529412f);
    style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(1.0f, 1.0f, 1.0f, 0.23529412f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(1.0f, 1.0f, 1.0f, 0.15686275f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(1.0f, 1.0f, 1.0f, 0.23529412f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(1.0f, 1.0f, 1.0f, 0.23529412f);
    style.Colors[ImGuiCol_Tab]                   = ImVec4(1.0f, 1.0f, 1.0f, 0.09803922f);
    style.Colors[ImGuiCol_TabHovered]            = ImVec4(1.0f, 1.0f, 1.0f, 0.15686275f);
    style.Colors[ImGuiCol_TabSelected]           = ImVec4(1.0f, 1.0f, 1.0f, 0.3137255f);
    style.Colors[ImGuiCol_TabDimmed]             = ImVec4(0.0f, 0.0f, 0.0f, 0.15686275f);
    style.Colors[ImGuiCol_TabDimmedSelected]     = ImVec4(1.0f, 1.0f, 1.0f, 0.23529412f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(1.0f, 1.0f, 1.0f, 0.3529412f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(1.0f, 1.0f, 1.0f, 0.3529412f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.15686275f, 0.15686275f, 0.15686275f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong]     = ImVec4(1.0f, 1.0f, 1.0f, 0.3137255f);
    style.Colors[ImGuiCol_TableBorderLight]      = ImVec4(1.0f, 1.0f, 1.0f, 0.19607843f);
    style.Colors[ImGuiCol_TableRowBg]            = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.0f, 1.0f, 1.0f, 0.019607844f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_DragDropTarget]        = ImVec4(0.16862746f, 0.23137255f, 0.5372549f, 1.0f);
    style.Colors[ImGuiCol_NavCursor]             = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
    style.Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);
    style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.0f, 0.0f, 0.0f, 0.5647059f);
}

inline void SetStyleRestLight()
{
    // Rest style by AaronBeardless from ImThemes
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha                            = 1.0f;
    style.DisabledAlpha                    = 0.5f;
    style.WindowPadding                    = ImVec2(13.0f, 10.0f);
    style.WindowRounding                   = 0.0f;
    style.WindowBorderSize                 = 1.0f;
    style.WindowMinSize                    = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign                 = ImVec2(0.5f, 0.5f);
    style.WindowMenuButtonPosition         = ImGuiDir_Right;
    style.ChildRounding                    = 3.0f;
    style.ChildBorderSize                  = 1.0f;
    style.PopupRounding                    = 5.0f;
    style.PopupBorderSize                  = 1.0f;
    style.FramePadding                     = ImVec2(20.0f, 8.1f);
    style.FrameRounding                    = 2.0f;
    style.FrameBorderSize                  = 0.0f;
    style.ItemSpacing                      = ImVec2(3.0f, 3.0f);
    style.ItemInnerSpacing                 = ImVec2(3.0f, 8.0f);
    style.CellPadding                      = ImVec2(6.0f, 14.1f);
    style.IndentSpacing                    = 0.0f;
    style.ColumnsMinSpacing                = 10.0f;
    style.ScrollbarSize                    = 10.0f;
    style.ScrollbarRounding                = 2.0f;
    style.GrabMinSize                      = 12.1f;
    style.GrabRounding                     = 1.0f;
    style.TabRounding                      = 2.0f;
    style.TabBorderSize                    = 0.0f;
    style.TabCloseButtonMinWidthUnselected = 5.0f;
    style.ColorButtonPosition              = ImGuiDir_Right;
    style.ButtonTextAlign                  = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign              = ImVec2(0.0f, 0.0f);


    style.Colors[ImGuiCol_Text]         = ImVec4(0.09019608f, 0.09019608f, 0.09019608f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.49803922f, 0.49803922f, 0.49803922f, 1.0f);

    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.9647059f, 0.9647059f, 0.9647059f, 1.0f);
    style.Colors[ImGuiCol_ChildBg]  = ImVec4(0.9411765f, 0.9411765f, 0.9411765f, 1.0f);
    style.Colors[ImGuiCol_PopupBg]  = ImVec4(0.9647059f, 0.9647059f, 0.9647059f, 1.0f);

    style.Colors[ImGuiCol_Border]       = ImVec4(0.0f, 0.0f, 0.0f, 0.09803922f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    style.Colors[ImGuiCol_FrameBg]        = ImVec4(0.0f, 0.0f, 0.0f, 0.047058824f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.0f, 0.0f, 0.0f, 0.078431375f);
    style.Colors[ImGuiCol_FrameBgActive]  = ImVec4(0.0f, 0.0f, 0.0f, 0.11764706f);

    style.Colors[ImGuiCol_TitleBg]          = ImVec4(0.92156863f, 0.92156863f, 0.92156863f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive]    = ImVec4(0.9019608f, 0.9019608f, 0.9019608f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.92156863f, 0.92156863f, 0.92156863f, 1.0f);

    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    style.Colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.0f, 0.0f, 0.0f, 0.039215688f);
    style.Colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.0f, 0.0f, 0.0f, 0.19607843f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.0f, 0.0f, 0.0f, 0.27450982f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.0f, 0.0f, 0.0f, 0.3529412f);

    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.09019608f, 0.09019608f, 0.09019608f, 1.0f);

    style.Colors[ImGuiCol_SliderGrab]       = ImVec4(0.0f, 0.0f, 0.0f, 0.23529412f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.3137255f);

    style.Colors[ImGuiCol_Button]        = ImVec4(0.0f, 0.0f, 0.0f, 0.047058824f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 0.078431375f);
    style.Colors[ImGuiCol_ButtonActive]  = ImVec4(0.0f, 0.0f, 0.0f, 0.11764706f);

    style.Colors[ImGuiCol_Header]        = ImVec4(0.0f, 0.0f, 0.0f, 0.047058824f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.0f, 0.0f, 0.0f, 0.078431375f);
    style.Colors[ImGuiCol_HeaderActive]  = ImVec4(0.0f, 0.0f, 0.0f, 0.11764706f);

    style.Colors[ImGuiCol_Separator]        = ImVec4(0.0f, 0.0f, 0.0f, 0.15686275f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.0f, 0.0f, 0.0f, 0.23529412f);
    style.Colors[ImGuiCol_SeparatorActive]  = ImVec4(0.0f, 0.0f, 0.0f, 0.23529412f);

    style.Colors[ImGuiCol_ResizeGrip]        = ImVec4(0.0f, 0.0f, 0.0f, 0.15686275f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.0f, 0.0f, 0.0f, 0.23529412f);
    style.Colors[ImGuiCol_ResizeGripActive]  = ImVec4(0.0f, 0.0f, 0.0f, 0.23529412f);

    style.Colors[ImGuiCol_Tab]               = ImVec4(0.0f, 0.0f, 0.0f, 0.047058824f);
    style.Colors[ImGuiCol_TabHovered]        = ImVec4(0.0f, 0.0f, 0.0f, 0.078431375f);
    style.Colors[ImGuiCol_TabSelected]       = ImVec4(0.0f, 0.0f, 0.0f, 0.15686275f);
    style.Colors[ImGuiCol_TabDimmed]         = ImVec4(0.0f, 0.0f, 0.0f, 0.039215688f);
    style.Colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.0f, 0.0f, 0.0f, 0.09803922f);

    style.Colors[ImGuiCol_PlotLines]            = ImVec4(0.09019608f, 0.09019608f, 0.09019608f, 0.47058824f);
    style.Colors[ImGuiCol_PlotLinesHovered]     = ImVec4(0.09019608f, 0.09019608f, 0.09019608f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram]        = ImVec4(0.09019608f, 0.09019608f, 0.09019608f, 0.47058824f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.09019608f, 0.09019608f, 0.09019608f, 1.0f);

    style.Colors[ImGuiCol_TableHeaderBg]     = ImVec4(0.92156863f, 0.92156863f, 0.92156863f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.0f, 0.0f, 0.0f, 0.19607843f);
    style.Colors[ImGuiCol_TableBorderLight]  = ImVec4(0.0f, 0.0f, 0.0f, 0.11764706f);
    style.Colors[ImGuiCol_TableRowBg]        = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_TableRowBgAlt]     = ImVec4(0.0f, 0.0f, 0.0f, 0.019607844f);

    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.16862746f, 0.23137255f, 0.5372549f, 0.35f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.16862746f, 0.23137255f, 0.5372549f, 1.0f);

    style.Colors[ImGuiCol_NavCursor]             = ImVec4(0.09019608f, 0.09019608f, 0.09019608f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.09019608f, 0.09019608f, 0.09019608f, 0.7f);
    style.Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.2f, 0.2f, 0.2f, 0.1f);
    style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.0f, 0.0f, 0.0f, 0.35f);
}

inline void SetStyleDarcula()
{
    // Darcula style by ice1000 from ImThemes
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha                            = 1.0f;
    style.DisabledAlpha                    = 0.6f;
    style.WindowPadding                    = ImVec2(8.0f, 8.0f);
    style.WindowRounding                   = 5.3f;
    style.WindowBorderSize                 = 1.0f;
    style.WindowMinSize                    = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign                 = ImVec2(0.0f, 0.5f);
    style.WindowMenuButtonPosition         = ImGuiDir_Left;
    style.ChildRounding                    = 0.0f;
    style.ChildBorderSize                  = 1.0f;
    style.PopupRounding                    = 0.0f;
    style.PopupBorderSize                  = 1.0f;
    style.FramePadding                     = ImVec2(4.0f, 3.0f);
    style.FrameRounding                    = 2.3f;
    style.FrameBorderSize                  = 1.0f;
    style.ItemSpacing                      = ImVec2(8.0f, 6.5f);
    style.ItemInnerSpacing                 = ImVec2(4.0f, 4.0f);
    style.CellPadding                      = ImVec2(4.0f, 2.0f);
    style.IndentSpacing                    = 21.0f;
    style.ColumnsMinSpacing                = 6.0f;
    style.ScrollbarSize                    = 14.0f;
    style.ScrollbarRounding                = 5.0f;
    style.GrabMinSize                      = 10.0f;
    style.GrabRounding                     = 2.3f;
    style.TabRounding                      = 4.0f;
    style.TabBorderSize                    = 0.0f;
    style.TabCloseButtonMinWidthUnselected = 0.0f;
    style.ColorButtonPosition              = ImGuiDir_Right;
    style.ButtonTextAlign                  = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign              = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text]                  = ImVec4(0.73333335f, 0.73333335f, 0.73333335f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.34509805f, 0.34509805f, 0.34509805f, 1.0f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.23529412f, 0.24705882f, 0.25490198f, 0.94f);
    style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.23529412f, 0.24705882f, 0.25490198f, 0.0f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.23529412f, 0.24705882f, 0.25490198f, 0.94f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.33333334f, 0.33333334f, 0.33333334f, 0.5f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.15686275f, 0.15686275f, 0.15686275f, 0.0f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.16862746f, 0.16862746f, 0.16862746f, 0.54f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.4509804f, 0.6745098f, 0.99607843f, 0.67f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.47058824f, 0.47058824f, 0.47058824f, 0.67f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.039215688f, 0.039215688f, 0.039215688f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.0f, 0.0f, 0.0f, 0.51f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.15686275f, 0.28627452f, 0.47843137f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.27058825f, 0.28627452f, 0.2901961f, 0.8f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.27058825f, 0.28627452f, 0.2901961f, 0.6f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.21960784f, 0.30980393f, 0.41960785f, 0.51f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.21960784f, 0.30980393f, 0.41960785f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.13725491f, 0.19215687f, 0.2627451f, 0.91f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.8980392f, 0.8980392f, 0.8980392f, 0.83f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.69803923f, 0.69803923f, 0.69803923f, 0.62f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.29803923f, 0.29803923f, 0.29803923f, 0.84f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.33333334f, 0.3529412f, 0.36078432f, 0.49f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.21960784f, 0.30980393f, 0.41960785f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.13725491f, 0.19215687f, 0.2627451f, 1.0f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.33333334f, 0.3529412f, 0.36078432f, 0.53f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.4509804f, 0.6745098f, 0.99607843f, 0.67f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.47058824f, 0.47058824f, 0.47058824f, 0.67f);
    style.Colors[ImGuiCol_Separator]             = ImVec4(0.3137255f, 0.3137255f, 0.3137255f, 1.0f);
    style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.3137255f, 0.3137255f, 0.3137255f, 1.0f);
    style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.3137255f, 0.3137255f, 0.3137255f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(1.0f, 1.0f, 1.0f, 0.85f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(1.0f, 1.0f, 1.0f, 0.6f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(1.0f, 1.0f, 1.0f, 0.9f);
    style.Colors[ImGuiCol_Tab]                   = ImVec4(0.1764706f, 0.34901962f, 0.5764706f, 0.862f);
    style.Colors[ImGuiCol_TabHovered]            = ImVec4(0.25882354f, 0.5882353f, 0.9764706f, 0.8f);
    style.Colors[ImGuiCol_TabSelected]           = ImVec4(0.19607843f, 0.40784314f, 0.6784314f, 1.0f);
    style.Colors[ImGuiCol_TabDimmed]             = ImVec4(0.06666667f, 0.101960786f, 0.14509805f, 0.9724f);
    style.Colors[ImGuiCol_TabDimmedSelected]     = ImVec4(0.13333334f, 0.25882354f, 0.42352942f, 1.0f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.60784316f, 0.60784316f, 0.60784316f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.0f, 0.42745098f, 0.34901962f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.8980392f, 0.69803923f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.0f, 0.6f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.1882353f, 0.1882353f, 0.2f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.30980393f, 0.30980393f, 0.34901962f, 1.0f);
    style.Colors[ImGuiCol_TableBorderLight]      = ImVec4(0.22745098f, 0.22745098f, 0.24705882f, 1.0f);
    style.Colors[ImGuiCol_TableRowBg]            = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.0f, 1.0f, 1.0f, 0.06f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.18431373f, 0.39607844f, 0.7921569f, 0.9f);
    style.Colors[ImGuiCol_DragDropTarget]        = ImVec4(1.0f, 1.0f, 0.0f, 0.9f);
    style.Colors[ImGuiCol_NavCursor]             = ImVec4(0.25882354f, 0.5882353f, 0.9764706f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
    style.Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);
    style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.8f, 0.8f, 0.8f, 0.35f);
}

inline void SetStyleMicrosoft()
{
    // Microsoft style by usernameiwantedwasalreadytaken from ImThemes
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha                            = 1.0f;
    style.DisabledAlpha                    = 0.6f;
    style.WindowPadding                    = ImVec2(4.0f, 6.0f);
    style.WindowRounding                   = 0.0f;
    style.WindowBorderSize                 = 0.0f;
    style.WindowMinSize                    = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign                 = ImVec2(0.0f, 0.5f);
    style.WindowMenuButtonPosition         = ImGuiDir_Left;
    style.ChildRounding                    = 0.0f;
    style.ChildBorderSize                  = 1.0f;
    style.PopupRounding                    = 0.0f;
    style.PopupBorderSize                  = 1.0f;
    style.FramePadding                     = ImVec2(8.0f, 6.0f);
    style.FrameRounding                    = 0.0f;
    style.FrameBorderSize                  = 1.0f;
    style.ItemSpacing                      = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing                 = ImVec2(8.0f, 6.0f);
    style.CellPadding                      = ImVec2(4.0f, 2.0f);
    style.IndentSpacing                    = 20.0f;
    style.ColumnsMinSpacing                = 6.0f;
    style.ScrollbarSize                    = 20.0f;
    style.ScrollbarRounding                = 0.0f;
    style.GrabMinSize                      = 5.0f;
    style.GrabRounding                     = 0.0f;
    style.TabRounding                      = 4.0f;
    style.TabBorderSize                    = 0.0f;
    style.TabCloseButtonMinWidthUnselected = 0.0f;
    style.ColorButtonPosition              = ImGuiDir_Right;
    style.ButtonTextAlign                  = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign              = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text]                  = ImVec4(0.09803922f, 0.09803922f, 0.09803922f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.49803922f, 0.49803922f, 0.49803922f, 1.0f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.9490196f, 0.9490196f, 0.9490196f, 1.0f);
    style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.9490196f, 0.9490196f, 0.9490196f, 1.0f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.0f, 0.46666667f, 0.8392157f, 0.2f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.0f, 0.46666667f, 0.8392157f, 1.0f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.039215688f, 0.039215688f, 0.039215688f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.15686275f, 0.28627452f, 0.47843137f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.0f, 0.0f, 0.0f, 0.51f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.85882354f, 0.85882354f, 0.85882354f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.85882354f, 0.85882354f, 0.85882354f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.6862745f, 0.6862745f, 0.6862745f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.0f, 0.0f, 0.0f, 0.2f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.09803922f, 0.09803922f, 0.09803922f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.6862745f, 0.6862745f, 0.6862745f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.85882354f, 0.85882354f, 0.85882354f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.0f, 0.46666667f, 0.8392157f, 0.2f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.0f, 0.46666667f, 0.8392157f, 1.0f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.85882354f, 0.85882354f, 0.85882354f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.0f, 0.46666667f, 0.8392157f, 0.2f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.0f, 0.46666667f, 0.8392157f, 1.0f);
    style.Colors[ImGuiCol_Separator]             = ImVec4(0.42745098f, 0.42745098f, 0.49803922f, 0.5f);
    style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.09803922f, 0.4f, 0.7490196f, 0.78f);
    style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.09803922f, 0.4f, 0.7490196f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.25882354f, 0.5882353f, 0.9764706f, 0.2f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.25882354f, 0.5882353f, 0.9764706f, 0.67f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.25882354f, 0.5882353f, 0.9764706f, 0.95f);
    style.Colors[ImGuiCol_Tab]                   = ImVec4(0.1764706f, 0.34901962f, 0.5764706f, 0.862f);
    style.Colors[ImGuiCol_TabHovered]            = ImVec4(0.25882354f, 0.5882353f, 0.9764706f, 0.8f);
    style.Colors[ImGuiCol_TabSelected]           = ImVec4(0.19607843f, 0.40784314f, 0.6784314f, 1.0f);
    style.Colors[ImGuiCol_TabDimmed]             = ImVec4(0.06666667f, 0.101960786f, 0.14509805f, 0.9724f);
    style.Colors[ImGuiCol_TabDimmedSelected]     = ImVec4(0.13333334f, 0.25882354f, 0.42352942f, 1.0f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.60784316f, 0.60784316f, 0.60784316f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.0f, 0.42745098f, 0.34901962f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.8980392f, 0.69803923f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.0f, 0.6f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.1882353f, 0.1882353f, 0.2f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.30980393f, 0.30980393f, 0.34901962f, 1.0f);
    style.Colors[ImGuiCol_TableBorderLight]      = ImVec4(0.22745098f, 0.22745098f, 0.24705882f, 1.0f);
    style.Colors[ImGuiCol_TableRowBg]            = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.0f, 1.0f, 1.0f, 0.06f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.25882354f, 0.5882353f, 0.9764706f, 0.35f);
    style.Colors[ImGuiCol_DragDropTarget]        = ImVec4(1.0f, 1.0f, 0.0f, 0.9f);
    style.Colors[ImGuiCol_NavCursor]             = ImVec4(0.25882354f, 0.5882353f, 0.9764706f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
    style.Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);
    style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.8f, 0.8f, 0.8f, 0.35f);
}

inline void SetStyleFluentWinUIDark()
{
    // Fluent WinUI 3 Dark — based on WinUI 3 design tokens
    // Base:    #202020  Layer/Card: #2C2C2C  Accent: #60CDFF
    ImGuiStyle& style = ImGui::GetStyle();

    // --- Metrics ---
    style.Alpha                            = 1.0f;
    style.DisabledAlpha                    = 0.5f;
    style.WindowPadding                    = ImVec2(16.0f, 16.0f);
    style.WindowRounding                   = 8.0f;
    style.WindowBorderSize                 = 1.0f;
    style.WindowMinSize                    = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign                 = ImVec2(0.0f, 0.5f);
    style.WindowMenuButtonPosition         = ImGuiDir_Left;
    style.ChildRounding                    = 4.0f;
    style.ChildBorderSize                  = 1.0f;
    style.PopupRounding                    = 8.0f;
    style.PopupBorderSize                  = 1.0f;
    style.FramePadding                     = ImVec2(8.0f, 6.0f);
    style.FrameRounding                    = 4.0f;
    style.FrameBorderSize                  = 1.0f;
    style.ItemSpacing                      = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing                 = ImVec2(6.0f, 4.0f);
    style.CellPadding                      = ImVec2(6.0f, 4.0f);
    style.IndentSpacing                    = 20.0f;
    style.ColumnsMinSpacing                = 6.0f;
    style.ScrollbarSize                    = 8.0f;
    style.ScrollbarRounding                = 100.0f;   // pill shape
    style.GrabMinSize                      = 10.0f;
    style.GrabRounding                     = 100.0f;   // pill shape
    style.TabRounding                      = 4.0f;
    style.TabBorderSize                    = 0.0f;
    style.TabCloseButtonMinWidthUnselected = 0.0f;
    style.ColorButtonPosition              = ImGuiDir_Right;
    style.ButtonTextAlign                  = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign              = ImVec2(0.0f, 0.0f);

    // --- Colors ---
    // TextFillColorPrimary / Secondary / Disabled
    style.Colors[ImGuiCol_Text]                  = ImVec4(1.0f,    1.0f,    1.0f,   1.0f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(1.0f,    1.0f,    1.0f,   0.3628f);
    // SolidBackgroundFillColorBase #202020
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.1255f, 0.1255f, 0.1255f, 1.0f);
    // LayerFillColorDefault #2C2C2C
    style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.1725f, 0.1725f, 0.1725f, 1.0f);
    // FlyoutBackground #303030
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.1882f, 0.1882f, 0.1882f, 1.0f);
    // ControlStrokeColorDefault rgba(255,255,255,0.0837)
    style.Colors[ImGuiCol_Border]                = ImVec4(1.0f,    1.0f,    1.0f,   0.0837f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.0f,    0.0f,    0.0f,   0.0f);
    // ControlFillColorDefault / Secondary / Tertiary
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(1.0f,    1.0f,    1.0f,   0.0605f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(1.0f,    1.0f,    1.0f,   0.0837f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(1.0f,    1.0f,    1.0f,   0.0326f);
    // Title bar: slightly darker than window base
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.1098f, 0.1098f, 0.1098f, 1.0f);  // #1C1C1C
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.1255f, 0.1255f, 0.1255f, 1.0f);  // #202020
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.1098f, 0.1098f, 0.1098f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.1451f, 0.1451f, 0.1451f, 1.0f);  // #252525
    // Thin pill-shaped overlay scrollbar
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.0f,    0.0f,    0.0f,   0.0f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(1.0f,    1.0f,    1.0f,   0.40f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(1.0f,    1.0f,    1.0f,   0.55f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(1.0f,    1.0f,    1.0f,   0.30f);
    // Accent #60CDFF = (0.3765, 0.8039, 1.0)
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.3765f, 0.8039f, 1.0f,   1.0f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.3765f, 0.8039f, 1.0f,   1.0f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.3765f, 0.8039f, 1.0f,   0.75f);
    // Buttons use ControlFill tokens
    style.Colors[ImGuiCol_Button]                = ImVec4(1.0f,    1.0f,    1.0f,   0.0605f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(1.0f,    1.0f,    1.0f,   0.0837f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(1.0f,    1.0f,    1.0f,   0.0326f);
    // Selectables / list items use SubtleFill tokens
    style.Colors[ImGuiCol_Header]                = ImVec4(1.0f,    1.0f,    1.0f,   0.0605f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(1.0f,    1.0f,    1.0f,   0.0837f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(1.0f,    1.0f,    1.0f,   0.0326f);
    style.Colors[ImGuiCol_Separator]             = ImVec4(1.0f,    1.0f,    1.0f,   0.0837f);
    style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(1.0f,    1.0f,    1.0f,   0.1686f);
    style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(1.0f,    1.0f,    1.0f,   0.2549f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(1.0f,    1.0f,    1.0f,   0.10f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(1.0f,    1.0f,    1.0f,   0.20f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(1.0f,    1.0f,    1.0f,   0.35f);
    // Tabs blend into the window; selected tab relies on the indicator underline drawn by ImGui
    style.Colors[ImGuiCol_Tab]                   = ImVec4(0.0f,    0.0f,    0.0f,   0.0f);
    style.Colors[ImGuiCol_TabHovered]            = ImVec4(1.0f,    1.0f,    1.0f,   0.0837f);
    style.Colors[ImGuiCol_TabSelected]           = ImVec4(1.0f,    1.0f,    1.0f,   0.0605f);
    style.Colors[ImGuiCol_TabDimmed]             = ImVec4(0.0f,    0.0f,    0.0f,   0.0f);
    style.Colors[ImGuiCol_TabDimmedSelected]     = ImVec4(1.0f,    1.0f,    1.0f,   0.0419f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(1.0f,    1.0f,    1.0f,   0.50f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.3765f, 0.8039f, 1.0f,   1.0f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.3765f, 0.8039f, 1.0f,   0.80f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.3765f, 0.8039f, 1.0f,   1.0f);
    style.Colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.1725f, 0.1725f, 0.1725f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong]     = ImVec4(1.0f,    1.0f,    1.0f,   0.12f);
    style.Colors[ImGuiCol_TableBorderLight]      = ImVec4(1.0f,    1.0f,    1.0f,   0.07f);
    style.Colors[ImGuiCol_TableRowBg]            = ImVec4(0.0f,    0.0f,    0.0f,   0.0f);
    style.Colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.0f,    1.0f,    1.0f,   0.03f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.3765f, 0.8039f, 1.0f,   0.25f);
    style.Colors[ImGuiCol_DragDropTarget]        = ImVec4(0.3765f, 0.8039f, 1.0f,   1.0f);
    style.Colors[ImGuiCol_NavCursor]             = ImVec4(0.3765f, 0.8039f, 1.0f,   1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f,    1.0f,    1.0f,   0.70f);
    style.Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.0f,    0.0f,    0.0f,   0.45f);
    style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.0f,    0.0f,    0.0f,   0.54f);
}

inline void SetStyleFluentWinUILight()
{
    // Fluent WinUI 3 Light — based on WinUI 3 design tokens
    // Base:    #F3F3F3  Layer/Card: #FFFFFF  Accent: #0078D4
    ImGuiStyle& style = ImGui::GetStyle();

    // --- Metrics (same as dark variant) ---
    style.Alpha                            = 1.0f;
    style.DisabledAlpha                    = 0.5f;
    style.WindowPadding                    = ImVec2(16.0f, 16.0f);
    style.WindowRounding                   = 8.0f;
    style.WindowBorderSize                 = 1.0f;
    style.WindowMinSize                    = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign                 = ImVec2(0.0f, 0.5f);
    style.WindowMenuButtonPosition         = ImGuiDir_Left;
    style.ChildRounding                    = 4.0f;
    style.ChildBorderSize                  = 1.0f;
    style.PopupRounding                    = 8.0f;
    style.PopupBorderSize                  = 1.0f;
    style.FramePadding                     = ImVec2(8.0f, 6.0f);
    style.FrameRounding                    = 4.0f;
    style.FrameBorderSize                  = 1.0f;
    style.ItemSpacing                      = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing                 = ImVec2(6.0f, 4.0f);
    style.CellPadding                      = ImVec2(6.0f, 4.0f);
    style.IndentSpacing                    = 20.0f;
    style.ColumnsMinSpacing                = 6.0f;
    style.ScrollbarSize                    = 8.0f;
    style.ScrollbarRounding                = 100.0f;   // pill shape
    style.GrabMinSize                      = 10.0f;
    style.GrabRounding                     = 100.0f;   // pill shape
    style.TabRounding                      = 4.0f;
    style.TabBorderSize                    = 0.0f;
    style.TabCloseButtonMinWidthUnselected = 0.0f;
    style.ColorButtonPosition              = ImGuiDir_Right;
    style.ButtonTextAlign                  = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign              = ImVec2(0.0f, 0.0f);

    // --- Colors ---
    // TextFillColorPrimary composited on white ≈ #1C1C1C; Disabled ≈ rgba(0,0,0,0.36)
    style.Colors[ImGuiCol_Text]                  = ImVec4(0.1098f, 0.1098f, 0.1098f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.0f,    0.0f,    0.0f,    0.3614f);
    // SolidBackgroundFillColorBase #F3F3F3
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.9529f, 0.9529f, 0.9529f, 1.0f);
    // CardBackgroundFillColorDefault: white at 70 % over base
    style.Colors[ImGuiCol_ChildBg]               = ImVec4(1.0f,    1.0f,    1.0f,    0.70f);
    // FlyoutBackground #FCFCFC
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.9882f, 0.9882f, 0.9882f, 1.0f);
    // ControlStrokeColorDefault rgba(0,0,0,0.0578)
    style.Colors[ImGuiCol_Border]                = ImVec4(0.0f,    0.0f,    0.0f,    0.0578f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.0f,    0.0f,    0.0f,    0.0f);
    // ControlFillColorDefault / Secondary / Tertiary
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(1.0f,    1.0f,    1.0f,    0.703f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.9765f, 0.9765f, 0.9765f, 0.80f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.9765f, 0.9765f, 0.9765f, 0.60f);
    // Title bar: slightly darker than window base
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.9020f, 0.9020f, 0.9020f, 1.0f);  // #E6E6E6
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.8784f, 0.8784f, 0.8784f, 1.0f);  // #E0E0E0
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.9020f, 0.9020f, 0.9020f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.9529f, 0.9529f, 0.9529f, 1.0f);
    // Thin pill-shaped overlay scrollbar
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.0f,    0.0f,    0.0f,    0.0f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.0f,    0.0f,    0.0f,    0.35f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.0f,    0.0f,    0.0f,    0.50f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.0f,    0.0f,    0.0f,    0.65f);
    // Accent #0078D4 = (0.0, 0.4706, 0.8314)
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.0f,    0.4706f, 0.8314f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.0f,    0.4706f, 0.8314f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.0f,    0.4078f, 0.7216f, 1.0f);  // AccentFillSecondary
    // Buttons use ControlFill tokens
    style.Colors[ImGuiCol_Button]                = ImVec4(1.0f,    1.0f,    1.0f,    0.703f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.9765f, 0.9765f, 0.9765f, 0.80f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.9765f, 0.9765f, 0.9765f, 0.60f);
    // Selectables / list items use SubtleFill tokens
    style.Colors[ImGuiCol_Header]                = ImVec4(0.0f,    0.0f,    0.0f,    0.04f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.0f,    0.0f,    0.0f,    0.07f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.0f,    0.0f,    0.0f,    0.10f);
    style.Colors[ImGuiCol_Separator]             = ImVec4(0.0f,    0.0f,    0.0f,    0.0803f);
    style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.0f,    0.0f,    0.0f,    0.15f);
    style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.0f,    0.0f,    0.0f,    0.20f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.0f,    0.0f,    0.0f,    0.10f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.0f,    0.0f,    0.0f,    0.20f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.0f,    0.0f,    0.0f,    0.35f);
    // Tabs blend into the window
    style.Colors[ImGuiCol_Tab]                   = ImVec4(0.0f,    0.0f,    0.0f,    0.0f);
    style.Colors[ImGuiCol_TabHovered]            = ImVec4(0.0f,    0.0f,    0.0f,    0.04f);
    style.Colors[ImGuiCol_TabSelected]           = ImVec4(0.0f,    0.0f,    0.0f,    0.0f);
    style.Colors[ImGuiCol_TabDimmed]             = ImVec4(0.0f,    0.0f,    0.0f,    0.0f);
    style.Colors[ImGuiCol_TabDimmedSelected]     = ImVec4(0.0f,    0.0f,    0.0f,    0.04f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.0f,    0.0f,    0.0f,    0.50f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.0f,    0.4706f, 0.8314f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.0f,    0.4706f, 0.8314f, 0.80f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.0f,    0.4706f, 0.8314f, 1.0f);
    // Table header: one step darker than window background
    style.Colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.9216f, 0.9216f, 0.9216f, 1.0f);  // #EBEBEB
    style.Colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.0f,    0.0f,    0.0f,    0.15f);
    style.Colors[ImGuiCol_TableBorderLight]      = ImVec4(0.0f,    0.0f,    0.0f,    0.09f);
    style.Colors[ImGuiCol_TableRowBg]            = ImVec4(0.0f,    0.0f,    0.0f,    0.0f);
    style.Colors[ImGuiCol_TableRowBgAlt]         = ImVec4(0.0f,    0.0f,    0.0f,    0.02f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.0f,    0.4706f, 0.8314f, 0.20f);
    style.Colors[ImGuiCol_DragDropTarget]        = ImVec4(0.0f,    0.4706f, 0.8314f, 1.0f);
    style.Colors[ImGuiCol_NavCursor]             = ImVec4(0.0f,    0.4706f, 0.8314f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.0f,    0.0f,    0.0f,    0.70f);
    style.Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.2f,    0.2f,    0.2f,    0.10f);
    style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.0f,    0.0f,    0.0f,    0.30f);
}

