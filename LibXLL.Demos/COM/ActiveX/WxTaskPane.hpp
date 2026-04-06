// ---------------------------------------------------------------------------
// WxTaskPane.hpp — wxWidgets content implementation for TaskPaneControl.
//
// This class satisfies the Content concept required by TaskPaneControl<Content>:
//
//   - Constructible with (HWND parent, int width, int height)
//   - Has void resize(int width, int height)
//   - Destructor handles framework-appropriate cleanup
//
// It also provides two static helpers for process-wide framework management:
//
//   - WxTaskPane::initialize() — lazy one-time wxWidgets bootstrap
//   - WxTaskPane::shutdown()   — wxWidgets teardown at DLL unload
//
// WXWIDGETS USAGE
// ---------------
// wxWidgets is initialised lazily the first time a WxTaskPane is constructed.
// wxNativeContainerWindow wraps the pre-existing Win32 HWND so that ordinary
// wxPanel / wxButton children can be parented to it.  Because there is no wx
// message loop, wx teardown APIs (wxApp::OnExit, wxEntryCleanup) are NOT safe
// to call while windows still exist; see the destructor for the workaround.
// ---------------------------------------------------------------------------

#pragma once

// wx headers first — they include <windows.h> internally.
#include <wx/app.h>
#include <wx/button.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/nativewin.h>

#include <iostream>

class WxTaskPane
{
public:
    // -----------------------------------------------------------------------
    // initialize()
    //
    // Lazily initialises the wxWidgets subsystem the first time it is called.
    // Safe to call multiple times — subsequent calls are no-ops.
    //
    // Steps:
    //   1. If already initialised (flag set), return immediately.
    //   2. If wxTheApp already exists (someone else initialised wx), adopt it
    //      and set the flag.
    //   3. Otherwise allocate a bare wxApp, call wxEntryStart() to set up the
    //      Win32 message infrastructure, and call CallOnInit() so that any
    //      wxApp::OnInit override runs.
    //
    // SetExitOnFrameDelete(false) is critical: without it, wx would call
    // wxExit() when the first top-level window closes, killing the host.
    //
    // Returns true on success, false if wxEntryStart() fails.
    // -----------------------------------------------------------------------
    static bool initialize()
    {
        if (readyFlag()) return true;
        if (wxTheApp) { readyFlag() = true; return true; }

        int argc = 0;
        wxApp::SetInstance(new wxApp());
        if (!wxEntryStart(argc, static_cast<char**>(nullptr)))
            return false;

        if (wxTheApp)
        {
            wxTheApp->SetExitOnFrameDelete(false);
            wxTheApp->CallOnInit();
        }
        readyFlag() = true;
        return true;
    }

    // -----------------------------------------------------------------------
    // shutdown()
    //
    // Tears down the wxWidgets subsystem.  Should be called from
    // OnDisconnection or DllMain on DLL_PROCESS_DETACH after all WxTaskPane
    // instances have been destroyed.
    //
    // NOTE: It is NOT safe to call this while any wxWindow objects still
    // exist, because wxEntryCleanup() calls DestroyAllWindows() and similar
    // routines that require a consistent wx object graph.  See the destructor
    // for why WxTaskPane intentionally abandons (rather than destroys) its wx
    // objects.
    // -----------------------------------------------------------------------
    static void shutdown()
    {
        if (!readyFlag()) return;
        if (wxTheApp) wxTheApp->OnExit();
        wxEntryCleanup();
        readyFlag() = false;
    }

    // -----------------------------------------------------------------------
    // Constructor — creates the wxWidgets window hierarchy inside parentHwnd.
    //
    // wxNativeContainerWindow adopts parentHwnd: it does not create a new
    // Win32 window but instead wraps the existing HWND so that wx can treat
    // it as a top-level window and parent ordinary wx child windows to it.
    //
    // Layout:
    //   wxNativeContainerWindow (wraps parentHwnd)
    //     └─ wxPanel (m_wxPanel, full size, no border)
    //          └─ wxBoxSizer (vertical)
    //               ├─ stretch spacer
    //               ├─ wxButton ("Click me!", centred)
    //               └─ stretch spacer
    //
    // The button's wxEVT_BUTTON handler shows a wxMessageBox.  Excel's Win32
    // message pump dispatches WM_COMMAND to the button, which wx translates
    // into a wxCommandEvent that fires the bound lambda.
    // -----------------------------------------------------------------------
    WxTaskPane(HWND parentHwnd, int w, int h)
    {
        if (!initialize())
        {
            std::cerr << "[xlCOM]   wx initialisation failed\n";
            return;
        }

        // Wrap the existing HWND as a wx top-level container.
        m_wxContainer = new wxNativeContainerWindow(parentHwnd);
        // Full-size panel with no border so it fills the container exactly.
        m_wxPanel = new wxPanel(m_wxContainer, wxID_ANY,
                                wxDefaultPosition, wxSize(w, h),
                                wxNO_BORDER);

        auto* sizer  = new wxBoxSizer(wxVERTICAL);
        auto* button = new wxButton(m_wxPanel, wxID_ANY, "Click me!");

        // Lambda bound directly to the button; no event table entry needed.
        button->Bind(wxEVT_BUTTON, [](wxCommandEvent&)
        {
            wxMessageBox("Hello from the Task Pane!", "Task Pane",
                         wxOK | wxICON_INFORMATION);
        });

        // Equal stretch spacers above and below the button centre it
        // vertically.
        sizer->AddStretchSpacer();
        sizer->Add(button, 0, wxALIGN_CENTER);
        sizer->AddStretchSpacer();

        m_wxPanel->SetSizer(sizer);
        m_wxPanel->Layout();
        std::cerr << "[xlCOM]   wx content created\n";
    }

    // -----------------------------------------------------------------------
    // resize — called by TaskPaneControl::SetObjectRects() when the task pane
    // is resized.  Repositions the wxPanel and re-runs its sizer layout.
    // -----------------------------------------------------------------------
    void resize(int w, int h)
    {
        if (m_wxPanel)
        {
            m_wxPanel->SetSize(0, 0, w, h);
            m_wxPanel->Layout();
        }
    }

    // -----------------------------------------------------------------------
    // Destructor — wx object abandonment strategy.
    //
    // Calling wxWindow::Destroy() or delete on m_wxPanel / m_wxContainer
    // would normally schedule deferred window deletion via a wx idle event.
    // But without a wx event loop, idle events never process, leaving wx in a
    // partially destructed state.  Moreover, wx's internal OnUpdateUI
    // machinery (DoUpdateWindowUI) walks the window tree during destruction
    // and accesses members that may already be invalid without a running loop.
    //
    // The safe approach is to abandon the wx pointers (set to nullptr without
    // calling any wx teardown API) and let DestroyWindow() on the parent HWND
    // destroy the underlying Win32 child windows.  The wx wrapper objects
    // become "orphaned" — they hold a dangling HWND — but because the process
    // exits (or the DLL unloads) shortly after, this is harmless.
    // -----------------------------------------------------------------------
    ~WxTaskPane()
    {
        m_wxPanel     = nullptr;
        m_wxContainer = nullptr;
    }

    // Non-copyable, non-movable (wx pointers are not safe to transfer).
    WxTaskPane(const WxTaskPane&)            = delete;
    WxTaskPane& operator=(const WxTaskPane&) = delete;
    WxTaskPane(WxTaskPane&&)                 = delete;
    WxTaskPane& operator=(WxTaskPane&&)      = delete;

private:
    // -----------------------------------------------------------------------
    // readyFlag()
    //
    // Returns a reference to a process-wide boolean that tracks whether
    // wxEntryStart() has been called successfully.  Using a function-local
    // static makes initialisation order well-defined and avoids the static-
    // initialisation order fiasco across translation units.
    // -----------------------------------------------------------------------
    static bool& readyFlag()
    {
        static bool ready = false;
        return ready;
    }

    // wxNativeContainerWindow wraps the pre-existing Win32 HWND so that
    // wxWidgets child windows can be parented to it normally.  Ownership is
    // via the raw pointer; see the destructor for why we do NOT call Destroy().
    wxNativeContainerWindow* m_wxContainer = nullptr;

    // The top-level wxPanel inside m_wxContainer.  All UI widgets are children
    // of this panel.  Same ownership caveat as m_wxContainer.
    wxPanel*                 m_wxPanel     = nullptr;
};

