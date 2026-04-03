// xllDemoFltkModalDialog.cpp
//
// Minimal demo: a single XLL command that shows a modal FLTK dialog.
//
// This is the FLTK counterpart of xllDemoWxModalDialog.cpp.  The same
// NativeOwnerModal pattern is used to make the dialog properly modal
// over Excel's window.
//
// FLTK does not need an "app object" or special initialisation — the
// library initialises itself lazily on first use.  No xlAutoOpen /
// xlAutoClose hooks are required.
//
// Include order note
// ------------------
// WIN32_LEAN_AND_MEAN must be defined before any #include <windows.h>.
// FLTK headers must come before LibXLL / ExcelSDK headers because
// xlcall.hpp contains a bare #include <windows.h> that, without
// WIN32_LEAN_AND_MEAN, drags in winsock.h and causes conflicts.

// Must be first — before any #include that transitively reaches <windows.h>.
#define WIN32_LEAN_AND_MEAN

#include <string>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>
#include <FL/platform.H>          // fl_xid()

#include <Auto.hpp>
#include <Commands.hpp>
#include <Register.hpp>
#include <Types.hpp>

// ============================================================================
// NativeOwnerModal
//
// RAII helper: sets a Win32 owner relationship, centres the dialog over the
// owner, and disables the owner for the lifetime of this object.
// ============================================================================

class NativeOwnerModal
{
public:
    NativeOwnerModal(HWND dialogHwnd, HWND ownerHwnd)
        : m_owner(ownerHwnd)
    {
        SetWindowLongPtr(dialogHwnd, GWLP_HWNDPARENT,
                         reinterpret_cast<LONG_PTR>(ownerHwnd));

        RECT ow{}, wd{};
        GetWindowRect(ownerHwnd,  &ow);
        GetWindowRect(dialogHwnd, &wd);
        const int dw = wd.right  - wd.left;
        const int dh = wd.bottom - wd.top;
        SetWindowPos(dialogHwnd, nullptr,
                     ow.left + (ow.right  - ow.left - dw) / 2,
                     ow.top  + (ow.bottom - ow.top  - dh) / 2,
                     0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

        EnableWindow(ownerHwnd, FALSE);
    }

    ~NativeOwnerModal()
    {
        EnableWindow(m_owner, TRUE);
        SetForegroundWindow(m_owner);
    }

    NativeOwnerModal(const NativeOwnerModal&)            = delete;
    NativeOwnerModal& operator=(const NativeOwnerModal&) = delete;

private:
    HWND m_owner;
};

// ============================================================================
// GreetingDialog
//
// A small modal FLTK window with a text input and OK / Cancel buttons.
// Call run() to show the dialog modally; it returns the entered text,
// or an empty string if the user cancelled.
// ============================================================================

class GreetingDialog : public Fl_Window
{
public:
    GreetingDialog()
        : Fl_Window(360, 120, "FLTK inside an XLL")
    {
        begin();
        m_label = new Fl_Box(10, 10, 120, 25, "Enter your name:");
        m_label->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

        m_input = new Fl_Input(130, 10, 210, 25);

        m_ok     = new Fl_Button( 170, 70, 80, 30, "OK");
        m_cancel = new Fl_Button( 260, 70, 80, 30, "Cancel");

        m_ok->callback(on_ok, this);
        m_cancel->callback(on_cancel, this);

        // Enter in the input field confirms.
        m_input->when(FL_WHEN_ENTER_KEY);
        m_input->callback(on_ok, this);

        end();
        set_modal();
    }

    /// Show the dialog modally and return the entered text
    /// (empty if the user cancelled or closed the window).
    [[nodiscard]] std::string run(HWND excelHwnd)
    {
        show();

        // fl_xid() returns the Win32 HWND after the window is shown.
        NativeOwnerModal modal(fl_xid(this), excelHwnd);

        while (shown()) {
            Fl::wait();
        }

        return m_confirmed ? m_input->value() : std::string{};
    }

private:
    static void on_ok(Fl_Widget*, void* data)
    {
        auto* self = static_cast<GreetingDialog*>(data);
        self->m_confirmed = true;
        self->hide();
    }

    static void on_cancel(Fl_Widget*, void* data)
    {
        auto* self = static_cast<GreetingDialog*>(data);
        self->m_confirmed = false;
        self->hide();
    }

    Fl_Box*    m_label   = nullptr;
    Fl_Input*  m_input   = nullptr;
    Fl_Button* m_ok      = nullptr;
    Fl_Button* m_cancel  = nullptr;
    bool       m_confirmed = false;
};

// ============================================================================
// Command: FLTK.MODAL.GREETING
// ============================================================================

auto fltkModalGreetingCmd =
    xll::Command("FLTK.MODAL.GREETING")
    | xll::Procedure("ShowFltkModalGreeting")
    | xll::Category("FLTK Examples")
    | xll::Description(
        "Shows a modal FLTK dialog parented to the Excel window, "
        "then greets the user with xll::alert.");
XLL_REGISTER(fltkModalGreetingCmd);

XLL_FUNCTION void XLLAPI ShowFltkModalGreeting()
{
    HWND excelHwnd = xll::get_hwnd();
    if (!excelHwnd) return;

    GreetingDialog dlg;
    const std::string name = dlg.run(excelHwnd);

    if (!name.empty()) {
        xll::alert(xll::String("Hello, " + name + "!"));
    }
}

