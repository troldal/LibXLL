// xllDemoQmlModalDialog.cpp
//
// Minimal demo: a single XLL command that shows a modal QML dialog.
//
// This is the QML counterpart of xllDemoQtModalDialog.cpp.  When you only
// need modal dialogs (no persistent non-modal frames), the code is
// dramatically simpler than xllDemoQmlDialog.cpp:
//
//   - A QGuiApplication and QQmlEngine are created on xlAutoOpen and
//     destroyed on xlAutoClose.  Both run on Excel's main thread.
//   - The command creates a QQmlComponent from an embedded QML string,
//     instantiates it, runs a local QEventLoop until the window closes,
//     and reads back the result — all on the same thread, no cross-thread
//     marshaling required.
//   - No dedicated UI thread, no dispatchers, no futures/promises,
//     no message-pumping wait loop, no sigslot wiring.
//
// The only non-trivial part is NativeOwnerModal: because the QML window
// has no Qt parent (to avoid parent-chain issues with a foreign HWND), we
// use Win32 to manually set the owner, centre the dialog over Excel, and
// disable/re-enable the Excel window for proper modal semantics.
//
// The FluentWinUI3 style is set before any QML is loaded, giving the
// dialog a modern Windows 11 look.
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

#include <cmrc/cmrc.hpp>
CMRC_DECLARE(qml_demo);

#include <QEventLoop>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QWindow>

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
// QML source — loaded from the embedded CMakeRC resource at runtime.
// ============================================================================

static QByteArray loadQml(const char* path)
{
    auto fs   = cmrc::qml_demo::get_filesystem();
    auto file = fs.open(path);
    return QByteArray(file.begin(), static_cast<qsizetype>(file.size()));
}

// ============================================================================
// QGuiApplication + QQmlEngine lifetime — managed via unique_ptrs.
//
// Created on xlAutoOpen, destroyed on xlAutoClose.  Both live on Excel's
// main thread, which is Qt's GUI thread in this modal-only scenario.
//
// FluentWinUI3 must be set after QGuiApplication exists but before any
// QML component is loaded.
// ============================================================================

namespace {
    int    s_argc = 0;
    char*  s_argv[] = { nullptr };
    std::unique_ptr<QGuiApplication> s_app;
    std::unique_ptr<QQmlEngine>      s_engine;
} // anonymous namespace

// ============================================================================
// Lifecycle
// ============================================================================

auto qmlModalOnOpen =
    xll::OnOpen()
    | xll::Before([] {
        s_app = std::make_unique<QGuiApplication>(s_argc, s_argv);
        QQuickStyle::setStyle("FluentWinUI3");
        s_engine = std::make_unique<QQmlEngine>();
    });
XLL_REGISTER(qmlModalOnOpen);

auto qmlModalOnClose =
    xll::OnClose()
    | xll::Before([] {
        s_engine.reset();
        s_app.reset();
    });
XLL_REGISTER(qmlModalOnClose);

// ============================================================================
// Command: QML.MODAL.GREETING
// ============================================================================

auto qmlModalGreetingCmd =
    xll::Command("QML.MODAL.GREETING")
    | xll::Procedure("ShowQmlModalGreeting")
    | xll::Category("QML Examples")
    | xll::Description(
        "Shows a modal QML dialog (FluentWinUI3 style) parented to the "
        "Excel window, then greets the user with xll::alert.");
XLL_REGISTER(qmlModalGreetingCmd);

XLL_FUNCTION void XLLAPI ShowQmlModalGreeting()
{
    HWND excelHwnd = xll::get_hwnd();
    if (!excelHwnd || !s_engine) return;

    QQmlComponent component(s_engine.get());
    component.setData(loadQml("QML/GreetingDialog.qml"), QUrl());
    if (component.isError()) return;

    std::unique_ptr<QObject> obj(component.create());
    auto* window = qobject_cast<QWindow*>(obj.get());
    if (!window) return;

    window->show();
    NativeOwnerModal modal(
        reinterpret_cast<HWND>(window->winId()), excelHwnd);

    // Run a local event loop until the QML window closes itself.
    QEventLoop loop;
    QObject::connect(window, &QWindow::visibleChanged,
        &loop, [&loop](bool visible) {
            if (!visible) loop.quit();
        });
    loop.exec();

    if (obj->property("accepted").toBool()) {
        const std::string name =
            obj->property("inputText").toString().toStdString();
        if (!name.empty()) {
            xll::alert(xll::String("Hello, " + name + "!"));
        }
    }
}

