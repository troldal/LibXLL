// ---------------------------------------------------------------------------
// Win32TaskPane.hpp — pure Win32 content implementation for TaskPaneControl.
//
// This class satisfies the Content concept required by TaskPaneControl<Content>:
//
//   - Constructible with (HWND parent, int width, int height)
//   - Has void resize(int width, int height)
//   - Destructor handles cleanup
//
// No GUI framework is required — only the Windows SDK.  All child windows are
// created with CreateWindowExW and positioned manually.
//
// LAYOUT
// ------
//   parentHwnd  (owned by TaskPaneControl — the ActiveX container HWND)
//     └─ m_button  (BUTTON class, centred in the pane)
//
// The button sends WM_COMMAND to parentHwnd when clicked.  Because
// TaskPaneControl's window class uses DefWindowProcW, WM_COMMAND is not
// handled there.  We therefore subclass parentHwnd with our own WndProc to
// intercept BN_CLICKED and show a MessageBox.
//
// THREADING
// ---------
// All Win32 window operations happen on Excel's STA thread.  No extra threads
// or message pumps are involved.
// ---------------------------------------------------------------------------

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <iostream>

class Win32TaskPane
{
public:
    // -----------------------------------------------------------------------
    // Constructor — creates a BUTTON child window centred inside parentHwnd.
    //
    // Subclasses parentHwnd so that WM_COMMAND / BN_CLICKED from the button
    // can be handled.  The original WndProc is stored and called for all
    // unhandled messages.
    // -----------------------------------------------------------------------
    Win32TaskPane(HWND parentHwnd, int w, int h)
        : m_parent(parentHwnd)
    {
        // Subclass the parent so we can intercept WM_COMMAND from the button.
        // Store a pointer to this instance in the HWND's user data so the
        // static WndProc can route messages back to us.
        m_prevWndProc = reinterpret_cast<WNDPROC>(
            SetWindowLongPtrW(m_parent, GWLP_WNDPROC,
                              reinterpret_cast<LONG_PTR>(&Win32TaskPane::subclassProc)));
        SetWindowLongPtrW(m_parent, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(this));

        // Create a standard BUTTON control as a child of the container.
        m_button = CreateWindowExW(
            0, L"BUTTON", L"Click me!",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 0, kButtonWidth, kButtonHeight,
            m_parent, reinterpret_cast<HMENU>(kButtonId),
            GetModuleHandleW(nullptr), nullptr);

        if (m_button)
        {
            // Use the system default GUI font (Segoe UI on modern Windows).
            HFONT hFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
            SendMessageW(m_button, WM_SETFONT,
                         reinterpret_cast<WPARAM>(hFont), TRUE);
        }

        // Position the button in the centre of the pane.
        centreButton(w, h);

        std::cerr << "[xlCOM]   Win32 content created\n";
    }

    // -----------------------------------------------------------------------
    // resize — called by TaskPaneControl::SetObjectRects() when the task pane
    // is resized.  Re-centres the button in the new client area.
    // -----------------------------------------------------------------------
    void resize(int w, int h)
    {
        centreButton(w, h);
    }

    // -----------------------------------------------------------------------
    // Destructor — removes the subclass and destroys the button.
    //
    // The button is a child of m_parent, so DestroyWindow(m_parent) (called
    // by TaskPaneControl::deactivate after the Content destructor returns)
    // would also destroy it.  We destroy it explicitly here so the subclass
    // WndProc is not called for the button's WM_DESTROY after the
    // Win32TaskPane object has been freed.
    // -----------------------------------------------------------------------
    ~Win32TaskPane()
    {
        // Restore the original WndProc before tearing down.
        if (m_parent && m_prevWndProc)
        {
            SetWindowLongPtrW(m_parent, GWLP_WNDPROC,
                              reinterpret_cast<LONG_PTR>(m_prevWndProc));
            SetWindowLongPtrW(m_parent, GWLP_USERDATA, 0);
        }

        if (m_button)
        {
            DestroyWindow(m_button);
            m_button = nullptr;
        }
    }

    // Non-copyable, non-movable (raw HWND ownership).
    Win32TaskPane(const Win32TaskPane&)            = delete;
    Win32TaskPane& operator=(const Win32TaskPane&) = delete;
    Win32TaskPane(Win32TaskPane&&)                 = delete;
    Win32TaskPane& operator=(Win32TaskPane&&)      = delete;

private:
    // Control ID for the button — arbitrary non-zero value.
    static constexpr int kButtonId     = 101;
    // Default button dimensions (in pixels).
    static constexpr int kButtonWidth  = 120;
    static constexpr int kButtonHeight = 30;

    HWND    m_parent      = nullptr;   // the container HWND (not owned)
    HWND    m_button      = nullptr;   // the BUTTON child (owned)
    WNDPROC m_prevWndProc = nullptr;   // original WndProc of m_parent

    // -----------------------------------------------------------------------
    // centreButton — positions m_button in the centre of a w×h client area.
    // -----------------------------------------------------------------------
    void centreButton(int w, int h) const
    {
        if (!m_button) return;
        const int x = (w - kButtonWidth)  / 2;
        const int y = (h - kButtonHeight) / 2;
        SetWindowPos(m_button, nullptr, x, y, kButtonWidth, kButtonHeight,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // -----------------------------------------------------------------------
    // subclassProc — static WndProc installed on m_parent via SetWindowLongPtr.
    //
    // Intercepts WM_COMMAND with BN_CLICKED from the button and shows a
    // MessageBox.  All other messages are forwarded to the original WndProc
    // (DefWindowProcW, stored in m_prevWndProc).
    // -----------------------------------------------------------------------
    static LRESULT CALLBACK subclassProc(HWND hwnd, UINT msg,
                                          WPARAM wParam, LPARAM lParam)
    {
        auto* self = reinterpret_cast<Win32TaskPane*>(
            GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        if (self && msg == WM_COMMAND &&
            LOWORD(wParam) == kButtonId && HIWORD(wParam) == BN_CLICKED)
        {
            MessageBoxW(hwnd,
                        L"Hello from the Task Pane!",
                        L"Task Pane",
                        MB_OK | MB_ICONINFORMATION);
            return 0;
        }

        // Forward to the original WndProc.
        if (self && self->m_prevWndProc)
            return CallWindowProcW(self->m_prevWndProc, hwnd, msg, wParam, lParam);

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
};

