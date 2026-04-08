// ---------------------------------------------------------------------------
// ImGuiRenderGuard.hpp — multi-context ImGui safety utilities.
//
// Two problems arise when multiple ImGui windows (each with their own
// ImGuiContext) render on the same STA thread via WM_TIMER:
//
// 1. RE-ENTRANCE — DXGI Present(1, 0) with vsync can pump the Win32
//    message queue while it blocks for the vertical blank.  A nested
//    WM_TIMER for another window could trigger a second renderFrame()
//    that corrupts the in-progress frame.
//
// 2. CONTEXT CONTAMINATION — even if the nested renderFrame() is blocked,
//    the other window's WndProc still calls SetCurrentContext() to forward
//    messages to ImGui_ImplWin32_WndProcHandler.  When Present() returns,
//    the global ImGui context points at the wrong window.
//
// ImGuiRenderLock     — solves problem 1 (RAII acquire/release of the flag).
// ImGuiContextGuard   — solves problem 2 (RAII save/restore of the context).
// ---------------------------------------------------------------------------

#pragma once

#include "imgui.h"

// RAII lock: sets the render guard on construction, clears it on destruction.
// Usage pattern in renderFrame():
//
//   if (ImGuiRenderLock::isLocked()) return;   // another frame is in progress
//   ImGuiRenderLock lock;                       // acquired; auto-released on all exits
//
// The check must remain manual: constructing ImGuiRenderLock while already
// locked would incorrectly clear the flag when the inner lock destructs.
struct ImGuiRenderLock
{
    // Returns true if another renderFrame() is currently in progress.
    static bool isLocked() { return s_flag(); }

    ImGuiRenderLock()  { s_flag() = true;  }
    ~ImGuiRenderLock() { s_flag() = false; }
    ImGuiRenderLock(const ImGuiRenderLock&)            = delete;
    ImGuiRenderLock& operator=(const ImGuiRenderLock&) = delete;

private:
    // Process-wide re-entrance flag — owned exclusively by ImGuiRenderLock.
    static bool& s_flag()
    {
        static bool s_rendering = false;
        return s_rendering;
    }
};

// RAII guard: saves the current ImGuiContext on construction, restores it
// on destruction.  Place at the top of every WndProc that touches ImGui
// so that message dispatch inside DXGI Present cannot leak a context switch.
struct ImGuiContextGuard
{
    ImGuiContext* saved;
    ImGuiContextGuard() : saved(ImGui::GetCurrentContext()) {}
    ~ImGuiContextGuard() { ImGui::SetCurrentContext(saved); }
    ImGuiContextGuard(const ImGuiContextGuard&)            = delete;
    ImGuiContextGuard& operator=(const ImGuiContextGuard&) = delete;
};
