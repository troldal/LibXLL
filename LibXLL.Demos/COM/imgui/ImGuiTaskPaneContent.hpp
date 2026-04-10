// ---------------------------------------------------------------------------
// ImGuiTaskPaneContent.hpp — demo content for the ImGui task pane.
//
// Renders a centred "Show Message" button with an indeterminate progress bar
// inside a full-pane ImGui window.  Satisfies the WindowContent concept used
// by ImGuiTaskPane<T>.
// ---------------------------------------------------------------------------

#pragma once

#include "imgui.h"
#include "SetStyle.hpp"              // HighlightedExcelButton
#include "ImGuiWindowContent.hpp"    // FrameAction

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

struct TaskPaneDemoContent
{
    FrameAction renderContent()
    {
        // ---- Fullscreen pane window -----------------------------------------
        const ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(io.DisplaySize);
        constexpr ImGuiWindowFlags kPaneFlags =
            ImGuiWindowFlags_NoTitleBar            |
            ImGuiWindowFlags_NoResize              |
            ImGuiWindowFlags_NoMove                |
            ImGuiWindowFlags_NoScrollbar           |
            ImGuiWindowFlags_NoCollapse            |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoSavedSettings;
        ImGui::Begin("##MainPane", nullptr, kPaneFlags);

        const ImVec2 avail   = ImGui::GetContentRegionAvail();
        const float  btnW    = ImGui::CalcTextSize("Show Message").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        const float  btnH    = ImGui::GetFrameHeight();
        const float  spacing = ImGui::GetStyle().ItemSpacing.y;
        const float  barH    = ImGui::GetFrameHeight() * 0.5f;
        const float  totalH  = btnH + spacing + barH;
        const float  groupY  = (avail.y - totalH) * 0.5f;
        const float  centerX = (avail.x - btnW) * 0.5f;

        // Button — centred horizontally within the group.
        ImGui::SetCursorPos(ImVec2(centerX, groupY));
        if (HighlightedExcelButton("Show Message"))
        {
            MessageBoxW(nullptr,
            L"Hello from the ImGui task pane!",
            L"XLThermo",
            MB_OK | MB_ICONINFORMATION);
        }

        // Indeterminate (marquee) progress bar — same width as the button.
        ImGui::SetCursorPos(ImVec2(centerX - btnW, groupY + btnH + spacing));
        ImGui::ProgressBar(-1.0f * static_cast<float>(ImGui::GetTime()), ImVec2(btnW * 3, barH / 2.0));

        ImGui::End();
        // --------------------------------------------------------------------

        return FrameAction::Continue;
    }
};

