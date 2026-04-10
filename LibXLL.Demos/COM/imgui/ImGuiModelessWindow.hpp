// ---------------------------------------------------------------------------
// ImGuiModelessWindow.hpp — type-erased modeless ImGui window.
//
// Returns immediately after creation; rendering is driven by a WM_TIMER
// on Excel's STA message pump so Excel remains fully interactive.
//
// WM_CLOSE hides the window rather than destroying it so it can be
// re-shown cheaply via showOrRaise().  The window and all its resources
// are released when the ImGuiModelessWindow object is destroyed.
//
// USAGE
// -----
//   static std::optional<ImGuiModelessWindow> g_wnd;
//
//   // First click — create:
//   g_wnd = ImGuiModelessWindow::create(owner, MyContent{});
//
//   // Subsequent clicks — show/raise:
//   if (g_wnd) g_wnd->showOrRaise();
//
//   // On disconnect:
//   g_wnd.reset();
//
// MOVE SEMANTICS
// --------------
// ImGuiModelessWindow is movable but not copyable.  The move constructor
// transfers HWND ownership and updates GWLP_USERDATA so that the shared
// WndProc continues to dispatch messages to the correct object.
//
// After the initial create()-into-storage assignment, the object must not
// be moved again (i.e. store it in a stable location such as std::optional
// with static storage duration).
// ---------------------------------------------------------------------------

#pragma once
#include "ImGuiWindowBase.hpp"
#include <optional>

class ImGuiModelessWindow final : public ImGuiWindowBase
{
public:
    // -----------------------------------------------------------------------
    // create — creates the window, shows it, and returns immediately.
    // Returns std::nullopt if window or D3D11 device creation fails.
    //
    //   owner   — Excel top-level HWND; window is positioned near it.
    //   content — any type satisfying WindowContent.
    //   title   — window title bar text.
    //   w, h    — initial client size in pixels.
    // -----------------------------------------------------------------------
    template<WindowContent T>
    [[nodiscard]] static std::optional<ImGuiModelessWindow> create(
        HWND           owner,
        T              content,
        const wchar_t* title = L"Dear ImGui",
        int            w     = 800,
        int            h     = 1200)
    {
        ImGuiModelessWindow wnd;
        wnd.m_content = std::make_unique<ContentModel<T>>(std::move(content));

        if (!wnd.initBase(owner, WS_EX_TOOLWINDOW, title, w, h))
            return std::nullopt;

        // Position near the owner if available.
        if (owner && IsWindow(owner))
        {
            RECT rc{};
            if (GetWindowRect(owner, &rc))
                SetWindowPos(wnd.m_hwnd, nullptr,
                             rc.left + 60, rc.top + 60,
                             0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

        ShowWindow(wnd.m_hwnd, SW_SHOW);
        UpdateWindow(wnd.m_hwnd);
        return std::optional<ImGuiModelessWindow>(std::move(wnd));
    }

    // -----------------------------------------------------------------------
    // showOrRaise — show the window if hidden; bring it to the foreground
    // if it is already visible.
    // -----------------------------------------------------------------------
    void showOrRaise()
    {
        if (!m_hwnd) return;
        ShowWindow(m_hwnd, SW_SHOW);
        SetForegroundWindow(m_hwnd);
    }

    // -----------------------------------------------------------------------
    // Move operations — transfer HWND ownership and keep GWLP_USERDATA and
    // the shared registry in sync with the new object address.
    // -----------------------------------------------------------------------
    ImGuiModelessWindow(ImGuiModelessWindow&& o) noexcept { moveFrom(o); }

    ImGuiModelessWindow& operator=(ImGuiModelessWindow&& o) noexcept
    {
        if (this != &o)
        {
            shutdownBase();   // release current resources before taking o's
            moveFrom(o);
        }
        return *this;
    }

    ImGuiModelessWindow(const ImGuiModelessWindow&)            = delete;
    ImGuiModelessWindow& operator=(const ImGuiModelessWindow&) = delete;

    // Destructor is implicitly defined; ~ImGuiWindowBase() calls shutdownBase().

private:
    bool m_wasVisible = false;   // tracks visibility across WM_ACTIVATEAPP

    ImGuiModelessWindow() = default;

    // WM_CLOSE and RequestClose from renderContent() both hide the window.
    // It can be re-shown with showOrRaise() without recreating any resources.
    void onWindowClose() override
    {
        if (m_hwnd) ShowWindow(m_hwnd, SW_HIDE);
    }

    // Hide when Excel deactivates; restore when it reactivates.
    void onWindowActivateApp(bool active) override
    {
        if (!m_hwnd) return;
        if (!active)
        {
            if (IsWindowVisible(m_hwnd))
            {
                ShowWindow(m_hwnd, SW_HIDE);
                m_wasVisible = true;
            }
        }
        else if (m_wasVisible)
        {
            ShowWindow(m_hwnd, SW_SHOW);
            m_wasVisible = false;
        }
    }

    // -----------------------------------------------------------------------
    // moveFrom — implementation shared by the move constructor and move
    // assignment operator.  Transfers all base and derived members from o,
    // then updates GWLP_USERDATA and the shared registry so the WndProc
    // dispatches messages to this object's address.
    // -----------------------------------------------------------------------
    void moveFrom(ImGuiModelessWindow& o) noexcept
    {
        // Transfer base-class members.
        m_hwnd              = std::exchange(o.m_hwnd,              nullptr);
        m_ctx               = std::exchange(o.m_ctx,               nullptr);
        m_swapChain         = std::exchange(o.m_swapChain,         nullptr);
        m_rtv               = std::exchange(o.m_rtv,               nullptr);
        m_content           = std::move(o.m_content);
        m_width             = o.m_width;
        m_height            = o.m_height;
        m_resizePending     = o.m_resizePending;
        m_swapChainOccluded = o.m_swapChainOccluded;

        // Transfer derived-class member.
        m_wasVisible        = o.m_wasVisible;

        // Point the WndProc at the new object address.
        if (m_hwnd)
            SetWindowLongPtrW(m_hwnd, GWLP_USERDATA,
                              reinterpret_cast<LONG_PTR>(this));

        // Update the shared registry entry from o's address to this.
        auto& r = s_registry;
        if (auto it = std::find(r.begin(), r.end(),
                                static_cast<ImGuiWindowBase*>(&o));
            it != r.end())
            *it = this;
    }
};
