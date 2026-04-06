// ---------------------------------------------------------------------------
// FltkTaskPane.hpp — FLTK content implementation for TaskPaneControl.
//
// This class satisfies the Content concept required by TaskPaneControl<Content>:
//
//   - Constructible with (HWND parent, int width, int height)
//   - Has void resize(int width, int height)
//   - Destructor handles framework-appropriate cleanup
//
// It also provides two static helpers for process-wide framework management:
//
//   - FltkTaskPane::initialize() — lazy one-time FLTK bootstrap
//   - FltkTaskPane::shutdown()   — FLTK teardown at DLL unload
//
// FLTK EMBEDDING STRATEGY
// -----------------------
// FLTK has no built-in API for adopting a foreign HWND as a parent (unlike
// Qt's QWindow::fromWinId or wx's wxNativeContainerWindow).  We therefore:
//
//   1. Create an Fl_Window (initially top-level) and show() it to force FLTK
//      to create the underlying Win32 HWND.
//   2. Use Win32 SetParent() to reparent the FLTK window into the ActiveX
//      container HWND, and adjust styles from WS_POPUP to WS_CHILD.
//   3. Move / resize the window to fill the container.
//
// This is the standard technique for embedding FLTK into a foreign window
// hierarchy.  Because FLTK's event loop is never run (Fl::run / Fl::wait are
// never called), the host application's Win32 message pump dispatches messages
// to the FLTK child HWNDs directly.
//
// THREADING
// ---------
// All FLTK window operations happen on Excel's STA thread (the GUI thread).
// No extra threads or message pumps are involved.  Fl::run() is never called —
// Excel's own Win32 message pump dispatches messages to the FLTK child HWNDs.
// ---------------------------------------------------------------------------

#pragma once

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/fl_ask.H>
#include <FL/platform.H>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <iostream>

class FltkTaskPane
{
public:
    // -----------------------------------------------------------------------
    // initialize()
    //
    // Lazily initialises the FLTK subsystem the first time it is called.
    // Safe to call multiple times — subsequent calls are no-ops.
    //
    // fl_open_display() ensures that FLTK's internal Win32 state (display
    // connection, default visual, etc.) is ready.  On Windows this is normally
    // done implicitly by Fl_Window::show(), but calling it explicitly here
    // keeps the initialisation path deterministic.
    //
    // Returns true on success.
    // -----------------------------------------------------------------------
    static bool initialize()
    {
        if (readyFlag()) return true;
        fl_open_display();
        readyFlag() = true;
        return true;
    }

    // -----------------------------------------------------------------------
    // shutdown()
    //
    // Tears down the FLTK subsystem.  Should be called from OnDisconnection
    // or DllMain on DLL_PROCESS_DETACH after all FltkTaskPane instances have
    // been destroyed.
    //
    // FLTK does not require explicit global teardown in the way that wxWidgets
    // (wxEntryCleanup) or Qt (delete qApp) do.  This method is provided for
    // API symmetry with WxTaskPane/QtTaskPane and to allow future cleanup.
    // -----------------------------------------------------------------------
    static void shutdown()
    {
        readyFlag() = false;
    }

    // -----------------------------------------------------------------------
    // Constructor — creates an FLTK window hierarchy inside parentHwnd.
    //
    // Steps:
    //   1. Ensure FLTK is initialised.
    //   2. Create an Fl_Window and populate it with child widgets.
    //   3. Call show() to materialise the native HWND.
    //   4. Reparent the FLTK HWND into parentHwnd using Win32 SetParent().
    //   5. Adjust window styles to WS_CHILD and reposition to fill the
    //      container.
    //
    // Layout:
    //   parentHwnd  (Win32, owned by TaskPaneControl)
    //     └─ m_fltkWindow  (Fl_Window, reparented via SetParent)
    //          └─ Fl_Button ("Click me!", centred)
    //
    // The button's callback shows an fl_message dialog.
    // -----------------------------------------------------------------------
    FltkTaskPane(HWND parentHwnd, int w, int h)
        : m_parent(parentHwnd)
    {
        if (!initialize())
        {
            std::cerr << "[xlCOM]   FLTK initialisation failed\n";
            return;
        }

        // Create the FLTK window.  Position (0,0) and the given size.
        m_fltkWindow = new Fl_Window(0, 0, w, h);
        m_fltkWindow->box(FL_FLAT_BOX);
        m_fltkWindow->color(FL_BACKGROUND_COLOR);

        // Create a centred button.
        static constexpr int kBtnW = 120;
        static constexpr int kBtnH = 30;
        const int bx = (w - kBtnW) / 2;
        const int by = (h - kBtnH) / 2;
        m_button = new Fl_Button(bx, by, kBtnW, kBtnH, "Click me!");
        m_button->callback([](Fl_Widget*, void*)
        {
            fl_message("%s", "Hello from the Task Pane!");
        });

        m_fltkWindow->end();

        // show() creates the native Win32 HWND.
        m_fltkWindow->show();

        // Retrieve the FLTK window's native HWND.
        HWND fltkHwnd = fl_xid(m_fltkWindow);
        if (!fltkHwnd)
        {
            std::cerr << "[xlCOM]   FLTK fl_xid returned null\n";
            return;
        }

        // Reparent into the ActiveX container HWND.
        // Change style from WS_POPUP/WS_OVERLAPPED to WS_CHILD.
        LONG_PTR style = GetWindowLongPtrW(fltkHwnd, GWL_STYLE);
        style &= ~(WS_POPUP | WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_THICKFRAME);
        style |= WS_CHILD;
        SetWindowLongPtrW(fltkHwnd, GWL_STYLE, style);

        // Remove extended styles that are inappropriate for a child window.
        LONG_PTR exStyle = GetWindowLongPtrW(fltkHwnd, GWL_EXSTYLE);
        exStyle &= ~(WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
        SetWindowLongPtrW(fltkHwnd, GWL_EXSTYLE, exStyle);

        SetParent(fltkHwnd, parentHwnd);

        // Position to fill the container exactly.
        SetWindowPos(fltkHwnd, nullptr, 0, 0, w, h,
                     SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

        std::cerr << "[xlCOM]   FLTK content created\n";
    }

    // -----------------------------------------------------------------------
    // resize — called by TaskPaneControl::SetObjectRects() when the task pane
    // is resized.  Resizes the FLTK window and re-centres the button.
    // -----------------------------------------------------------------------
    void resize(int w, int h)
    {
        if (m_fltkWindow)
        {
            m_fltkWindow->resize(0, 0, w, h);

            // Re-centre the button inside the new dimensions.
            if (m_button)
            {
                static constexpr int kBtnW = 120;
                static constexpr int kBtnH = 30;
                const int bx = (w - kBtnW) / 2;
                const int by = (h - kBtnH) / 2;
                m_button->resize(bx, by, kBtnW, kBtnH);
            }

            m_fltkWindow->redraw();

            // Also move the native HWND in case FLTK's resize doesn't update
            // the Win32 geometry for a reparented child.
            HWND fltkHwnd = fl_xid(m_fltkWindow);
            if (fltkHwnd)
            {
                SetWindowPos(fltkHwnd, nullptr, 0, 0, w, h,
                             SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
    }

    // -----------------------------------------------------------------------
    // Destructor — hides and deletes the FLTK window.
    //
    // Hiding before deleting ensures the native HWND is destroyed in an
    // orderly fashion.  FLTK's Fl_Window destructor calls hide() internally,
    // but being explicit here avoids any ordering surprises with the
    // reparented window hierarchy.
    // -----------------------------------------------------------------------
    ~FltkTaskPane()
    {
        if (m_fltkWindow)
        {
            m_fltkWindow->hide();
            // Fl_Window's destructor will delete child widgets (Fl_Button)
            // because FLTK groups own their children.
            delete m_fltkWindow;
            m_fltkWindow = nullptr;
            m_button     = nullptr;
        }
    }

    // Non-copyable, non-movable (FLTK widget ownership).
    FltkTaskPane(const FltkTaskPane&)            = delete;
    FltkTaskPane& operator=(const FltkTaskPane&) = delete;
    FltkTaskPane(FltkTaskPane&&)                 = delete;
    FltkTaskPane& operator=(FltkTaskPane&&)      = delete;

private:
    // -----------------------------------------------------------------------
    // readyFlag()
    //
    // Returns a reference to a process-wide boolean that tracks whether
    // fl_open_display() has been called.  Function-local static avoids the
    // static-initialisation order fiasco.
    // -----------------------------------------------------------------------
    static bool& readyFlag()
    {
        static bool ready = false;
        return ready;
    }

    HWND        m_parent     = nullptr;   // the container HWND (not owned)

    // The top-level Fl_Window that has been reparented into m_parent.
    // Owns the entire child widget hierarchy; deleting it tears down
    // the FLTK tree and destroys the native HWND.
    Fl_Window*  m_fltkWindow = nullptr;

    // The centred button — owned by m_fltkWindow (FLTK parent-child
    // ownership).  Stored here so resize() can reposition it.
    Fl_Button*  m_button     = nullptr;
};


