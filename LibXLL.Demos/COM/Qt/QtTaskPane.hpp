// ---------------------------------------------------------------------------
// QtTaskPane.hpp — Qt Widgets content implementation for TaskPaneControl.
//
// This class satisfies the Content concept required by TaskPaneControl<Content>:
//
//   - Constructible with (HWND parent, int width, int height)
//   - Has void resize(int width, int height)
//   - Destructor handles framework-appropriate cleanup
//
// It also provides two static helpers for process-wide framework management:
//
//   - QtTaskPane::initialize() — lazy one-time QApplication bootstrap
//   - QtTaskPane::shutdown()   — QApplication teardown at DLL unload
//
// QT EMBEDDING STRATEGY
// ---------------------
// We create a plain QWidget, force it to acquire a native Win32 HWND via
// winId(), then use Qt's own QWindow::setParent() to reparent it into a
// foreign QWindow wrapping the ActiveX container HWND.  This keeps Qt's
// internal state (geometry, visibility, parent-child relationship) consistent
// with the actual Win32 window hierarchy, so that Qt's layout manager,
// painting, and event delivery work correctly.
//
// Using raw Win32 SetParent() behind Qt's back does NOT work: Qt still
// thinks the widget is a top-level window and will not lay out or paint its
// children into the reparented HWND.
//
// The QWindow::fromWinId / createWindowContainer pattern is for the opposite
// direction ("foreign window inside a Qt app") and produces a stray
// top-level window when used in this scenario.
//
// DPI SCALING
// -----------
// Qt 6 enables automatic high-DPI scaling by default.  Because the host
// application (Excel) manages DPI and passes pixel coordinates through
// SetObjectRects / MoveWindow, Qt must NOT apply its own scaling on top.
// We disable it via qputenv("QT_ENABLE_HIGHDPI_SCALING", "0") before
// creating QApplication.
//
// THREADING
// ---------
// All Qt window operations happen on Excel's STA thread (the GUI thread).
// No extra threads or message pumps are involved.  QApplication::exec() is
// never called — Excel's own Win32 message pump dispatches messages to the
// Qt child HWNDs.
// ---------------------------------------------------------------------------

#pragma once

#include <QApplication>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QWidget>
#include <QWindow>

#include <iostream>

class QtTaskPane
{
public:
    // -----------------------------------------------------------------------
    // initialize()
    //
    // Lazily creates a QApplication the first time it is called.
    // Safe to call multiple times — subsequent calls are no-ops.
    //
    // A QApplication is required before any QWidget can be created.  Because
    // we are hosted inside Excel's process (an STA COM server), we must not
    // call QApplication::exec() — Excel's own Win32 message pump will
    // dispatch messages to our child windows.
    //
    // Qt 6's automatic high-DPI scaling is disabled before construction so
    // that pixel coordinates from the host (SetObjectRects) are used as-is.
    //
    // Returns true on success.
    // -----------------------------------------------------------------------
    static bool initialize()
    {
        if (qApp) return true;            // already initialised

        // Disable Qt's automatic high-DPI scaling.  The host application
        // (Excel) manages DPI and passes pixel coordinates via
        // SetObjectRects / MoveWindow.  If Qt applies its own 150% (or
        // whatever the display scale factor is) scaling on top, the widget
        // geometry becomes 1.5× too large and appears as a stray window.
        qputenv("QT_ENABLE_HIGHDPI_SCALING", "0");

        // QApplication requires argc by reference; use a static so it
        // outlives the constructor call.
        static int    argc = 0;
        static char*  argv[] = { nullptr };
        new QApplication(argc, argv);     // sets qApp global
        return qApp != nullptr;
    }

    // -----------------------------------------------------------------------
    // shutdown()
    //
    // Tears down the QApplication.  Should be called from OnDisconnection or
    // DllMain on DLL_PROCESS_DETACH after all QtTaskPane instances have been
    // destroyed.
    // -----------------------------------------------------------------------
    static void shutdown()
    {
        if (qApp)
        {
            delete qApp;                  // clears the qApp global
        }
    }

    // -----------------------------------------------------------------------
    // Constructor — creates a Qt widget hierarchy inside parentHwnd.
    //
    // Steps:
    //   1. Wrap the ActiveX container HWND as a foreign QWindow so Qt knows
    //      about the parent.
    //   2. Create a plain QWidget and force a native HWND via winId().
    //   3. Use Qt's QWindow::setParent() to reparent the widget's QWindow
    //      into the foreign parent.  Qt handles WS_CHILD, SetParent, and
    //      internal bookkeeping automatically.
    //   4. Set geometry to fill the container.
    //   5. Build the QWidget child tree (layout + button).
    //
    // Layout:
    //   parentHwnd  (Win32, owned by TaskPaneControl)
    //     └─ m_widget  (QWidget, reparented via QWindow::setParent)
    //          └─ QVBoxLayout
    //               ├─ stretch spacer
    //               ├─ QPushButton ("Click me!", centred)
    //               └─ stretch spacer
    //
    // The button's clicked signal is connected to a lambda that shows a
    // QMessageBox.
    // -----------------------------------------------------------------------
    QtTaskPane(HWND parentHwnd, int w, int h)
        : m_parent(parentHwnd)
    {
        if (!initialize())
        {
            std::cerr << "[xlCOM]   Qt initialisation failed\n";
            return;
        }

        // 1. Wrap the ActiveX container HWND as a foreign QWindow.
        //    Qt needs this object to manage the parent-child relationship.
        m_parentWindow = QWindow::fromWinId(reinterpret_cast<WId>(parentHwnd));

        // 2. Create the root QWidget and force a native HWND.
        m_widget = new QWidget();
        m_widget->winId();

        // 3. Reparent using Qt's own API — this sets WS_CHILD, calls Win32
        //    SetParent(), and updates Qt's internal parent-child state so
        //    layout, painting, and event delivery work correctly.
        m_widget->windowHandle()->setParent(m_parentWindow);

        // 4. Fill the container.
        m_widget->setGeometry(0, 0, w, h);

        // 5. Build the widget tree.
        auto* layout = new QVBoxLayout(m_widget);

        auto* button = new QPushButton("Click me!", m_widget);

        QObject::connect(button, &QPushButton::clicked, [parentHwnd]()
        {
            QMessageBox::information(nullptr,
                                     "Task Pane",
                                     "Hello from the Task Pane!");
        });

        layout->addStretch();
        layout->addWidget(button, 0, Qt::AlignCenter);
        layout->addStretch();

        m_widget->setLayout(layout);
        m_widget->show();

        std::cerr << "[xlCOM]   Qt content created\n";
    }

    // -----------------------------------------------------------------------
    // resize — called by TaskPaneControl::SetObjectRects() when the task pane
    // is resized.  Uses Qt's setGeometry() so the layout manager knows about
    // the new size and re-positions children.
    // -----------------------------------------------------------------------
    void resize(int w, int h)
    {
        if (m_widget)
            m_widget->setGeometry(0, 0, w, h);
    }

    // -----------------------------------------------------------------------
    // Destructor — destroys the Qt widget tree.
    //
    // Deleting m_widget destroys all child QWidgets and the underlying native
    // HWND.  This runs BEFORE TaskPaneControl::deactivate() calls
    // DestroyWindow() on the parent HWND, so the child is gone cleanly
    // before the parent is destroyed.
    //
    // The foreign parent QWindow (m_parentWindow) is also deleted; it is a
    // non-owning wrapper, so deleting it does NOT destroy the underlying
    // container HWND.
    // -----------------------------------------------------------------------
    ~QtTaskPane()
    {
        delete m_widget;
        m_widget = nullptr;

        delete m_parentWindow;
        m_parentWindow = nullptr;
    }

    // Non-copyable, non-movable (Qt widget ownership).
    QtTaskPane(const QtTaskPane&)            = delete;
    QtTaskPane& operator=(const QtTaskPane&) = delete;
    QtTaskPane(QtTaskPane&&)                 = delete;
    QtTaskPane& operator=(QtTaskPane&&)      = delete;

private:
    HWND     m_parent       = nullptr;   // the container HWND (not owned)

    // Foreign QWindow wrapping m_parent.  Used as the Qt parent for
    // m_widget's QWindow.  Non-owning — deleting it does NOT destroy the
    // underlying HWND.
    QWindow* m_parentWindow = nullptr;

    // The root QWidget reparented into m_parent.  Owns the entire child
    // widget hierarchy; deleting it tears down the Qt tree and destroys
    // the native HWND.
    QWidget* m_widget       = nullptr;
};









