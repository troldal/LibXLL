// ---------------------------------------------------------------------------
// ImGuiModalWindow.hpp — type-erased modal ImGui window.
//
// Runs a local PeekMessage loop that blocks the caller until the window is
// closed.  Provides proper modal semantics: disables the owner window for
// the duration, centres the dialog over it, and re-enables / foregrounds
// the owner on exit.
//
// USAGE
// -----
//   // Any type satisfying WindowContent can be passed as content:
//   ImGuiModalWindow::show(owner, MyContent{}, L"My Window", 1200, 900);
//
// LIFECYCLE
// ---------
//   show() is a blocking static factory.  The ImGuiModalWindow object is
//   created on the stack inside show() and destroyed automatically on exit —
//   no heap allocation, no pointer management.
// ---------------------------------------------------------------------------

#pragma once
#include "ImGuiWindowBase.hpp"

class ImGuiModalWindow final : public ImGuiWindowBase
{
public:
    // -----------------------------------------------------------------------
    // show — opens a modal window hosting the given content and blocks until
    // the user closes it.  Returns when the window is destroyed.
    //
    //   owner      — Excel top-level HWND; disabled for the duration.
    //   content    — any type satisfying WindowContent.
    //   title      — window title bar text.
    //   w, h       — initial client size in pixels.
    //   beforeShow — optional callable(HWND); invoked after window creation
    //                but before ShowWindow (e.g. to tweak Win32 styles).
    //   afterShow  — optional callable(HWND); invoked after ShowWindow /
    //                UpdateWindow (e.g. to set focus or apply late tweaks).
    // -----------------------------------------------------------------------
    template<WindowContent T,
             std::invocable<HWND> BeforeFn = decltype([](HWND){}),
             std::invocable<HWND> AfterFn  = decltype([](HWND){})>
    static void show(HWND           owner,
                     T              content,
                     const wchar_t* title      = L"Dear ImGui",
                     int            w          = 1600,
                     int            h          = 1200,
                     BeforeFn       beforeShow = {},
                     AfterFn        afterShow  = {})
    {
        ImGuiModalWindow wnd;
        wnd.m_content = std::make_unique<ContentModel<T>>(std::move(content));

        if (!wnd.initBase(nullptr, 0, title, w, h)) return;

        beforeShow(wnd.m_hwnd);

        // Modal semantics: parent / owner + disable.
        if (owner && IsWindow(owner))
        {
            SetWindowLongPtrW(wnd.m_hwnd, GWLP_HWNDPARENT,
                              reinterpret_cast<LONG_PTR>(owner));

            RECT ow{}, wd{};
            GetWindowRect(owner,      &ow);
            GetWindowRect(wnd.m_hwnd, &wd);
            const int dw = wd.right  - wd.left;
            const int dh = wd.bottom - wd.top;
            SetWindowPos(wnd.m_hwnd, nullptr,
                         ow.left + (ow.right  - ow.left - dw) / 2,
                         ow.top  + (ow.bottom - ow.top  - dh) / 2,
                         0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

            EnableWindow(owner, FALSE);
        }

        ShowWindow(wnd.m_hwnd, SW_SHOW);
        UpdateWindow(wnd.m_hwnd);

        afterShow(wnd.m_hwnd);

        wnd.runModal();

        if (owner && IsWindow(owner))
        {
            EnableWindow(owner, TRUE);
            SetForegroundWindow(owner);
        }
        // ~ImGuiModalWindow() → ~ImGuiWindowBase() → shutdownBase() handles
        // ImGui / D3D11 teardown and DestroyWindow.
    }

private:
    bool m_done = false;

    ImGuiModalWindow() = default;

    // WM_CLOSE and RequestClose from renderContent() both set m_done = true,
    // which causes runModal() to exit its message loop.
    void onWindowClose() override { m_done = true; }

    // Modal windows do not hide on app deactivation — use the inherited
    // no-op default from ImGuiWindowBase::onWindowActivateApp.

    // -----------------------------------------------------------------------
    // runModal — local PeekMessage loop; drives rendering directly rather
    // than relying solely on the WM_TIMER, so the window remains responsive
    // even if Excel's message pump is busy.
    // -----------------------------------------------------------------------
    void runModal()
    {
        MSG msg;
        while (!m_done)
        {
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                if (msg.message == WM_QUIT) { m_done = true; break; }
            }
            if (!m_done) renderFrame();
        }
    }
};
