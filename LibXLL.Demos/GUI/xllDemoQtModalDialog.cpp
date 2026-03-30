// xllDemoQtModalDialog.cpp
//
// Minimal demo: a single XLL command that shows a modal Qt dialog.
//
// When you only need modal dialogs (no persistent non-modal frames), the
// code is dramatically simpler than xllDemoQtDialog.cpp:
//
//   - A QApplication is created on xlAutoOpen and destroyed on xlAutoClose.
//     Both run on Excel's main thread, which becomes Qt's "GUI thread".
//   - The command creates the dialog, calls exec(), reads the result
//     — all on the same thread, no cross-thread marshaling required.
//   - No dedicated UI thread, no dispatchers, no futures/promises,
//     no message-pumping wait loop.
//
// The only non-trivial part is NativeOwnerModal: because we create the
// dialog with a nullptr parent (to avoid Qt parent-chain issues with a
// foreign HWND), we use Win32 to manually set the owner, centre the dialog
// over Excel, and disable/re-enable the Excel window for proper modal
// semantics.
//
// Include order note
// ------------------
// WIN32_LEAN_AND_MEAN must be defined before any #include <windows.h>.
// Qt headers must come before LibXLL / ExcelSDK headers because
// xlcall.hpp contains a bare #include <windows.h> that, without
// WIN32_LEAN_AND_MEAN, drags in winsock.h and causes conflicts.

// Must be first — before any #include that transitively reaches <windows.h>.
#define WIN32_LEAN_AND_MEAN

#include <memory>
#include <string>

#include <QApplication>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

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
        // Make the dialog visually owned by Excel.
        SetWindowLongPtr(dialogHwnd, GWLP_HWNDPARENT,
                         reinterpret_cast<LONG_PTR>(ownerHwnd));

        // Centre the dialog over the owner.
        RECT ow{}, wd{};
        GetWindowRect(ownerHwnd,  &ow);
        GetWindowRect(dialogHwnd, &wd);
        const int dw = wd.right  - wd.left;
        const int dh = wd.bottom - wd.top;
        SetWindowPos(dialogHwnd, nullptr,
                     ow.left + (ow.right  - ow.left - dw) / 2,
                     ow.top  + (ow.bottom - ow.top  - dh) / 2,
                     0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

        // Disable the owner — standard modal semantics.
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
// ============================================================================

class GreetingDialog : public QDialog
{
public:
    explicit GreetingDialog(QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Qt inside an XLL");
        resize(360, 150);

        auto* vbox = new QVBoxLayout(this);
        auto* hbox = new QHBoxLayout();
        auto* btns = new QHBoxLayout();

        auto* label = new QLabel("Enter your name:");
        m_input     = new QLineEdit();
        m_input->setMinimumWidth(200);

        hbox->addWidget(label);
        hbox->addWidget(m_input, 1);

        auto* btnOk     = new QPushButton("OK");
        auto* btnCancel = new QPushButton("Cancel");
        btns->addStretch();
        btns->addWidget(btnOk);
        btns->addWidget(btnCancel);
        btns->addStretch();

        vbox->addStretch();
        vbox->addLayout(hbox);
        vbox->addSpacing(10);
        vbox->addLayout(btns);
        vbox->addStretch();

        connect(btnOk,     &QPushButton::clicked, this, &QDialog::accept);
        connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
        connect(m_input, &QLineEdit::returnPressed, this, &QDialog::accept);
    }

    ~GreetingDialog() override = default;

    [[nodiscard]] std::string GetInput() const
    {
        return m_input->text().toStdString();
    }

private:
    QLineEdit* m_input = nullptr;
};

// ============================================================================
// QApplication lifetime — managed via a unique_ptr in an anonymous namespace.
//
// Unlike wxWidgets (which has wxInitialize/wxUninitialize), Qt requires
// exactly one QApplication to exist for the entire time any Qt widget code
// runs.  We create it on xlAutoOpen and destroy it on xlAutoClose.
// ============================================================================

namespace {
    int    s_argc = 0;
    char*  s_argv[] = { nullptr };
    std::unique_ptr<QApplication> s_app;
} // anonymous namespace

// ============================================================================
// Lifecycle: create / destroy QApplication on Excel's main thread
// ============================================================================

auto qtModalOnOpen =
    xll::OnOpen()
    | xll::Before([] {
        s_app = std::make_unique<QApplication>(s_argc, s_argv);
    });
XLL_REGISTER(qtModalOnOpen);

auto qtModalOnClose =
    xll::OnClose()
    | xll::Before([] {
        s_app.reset();
    });
XLL_REGISTER(qtModalOnClose);

// ============================================================================
// Command: QT.MODAL.GREETING
// ============================================================================

auto qtModalGreetingCmd =
    xll::Command("QT.MODAL.GREETING")
    | xll::Procedure("ShowQtModalGreeting")
    | xll::Category("Qt Examples")
    | xll::Description(
        "Shows a modal Qt dialog parented to the Excel window, "
        "then greets the user with xll::alert.");
XLL_REGISTER(qtModalGreetingCmd);

XLL_FUNCTION void XLLAPI ShowQtModalGreeting()
{
    HWND excelHwnd = xll::get_hwnd();
    if (!excelHwnd) return;

    GreetingDialog dlg;
    dlg.show();   // Realise the HWND before setting up the owner.
    NativeOwnerModal modal(
        reinterpret_cast<HWND>(dlg.winId()), excelHwnd);

    if (dlg.exec() == QDialog::Accepted) {
        const std::string name = dlg.GetInput();
        if (!name.empty()) {
            xll::alert(xll::String("Hello, " + name + "!"));
        }
    }
}

