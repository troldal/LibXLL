// ---------------------------------------------------------------------------
// TaskPaneControl.hpp — minimal ActiveX control for Excel custom task panes.
//
// OVERVIEW
// --------
// This header implements a single COM class, TaskPaneControl, which is the
// ActiveX control that Office hosts inside a custom task pane (CTP).  When a
// COM add-in creates a CTP and passes this control's ProgID/CLSID as the
// content object, Excel instantiates the class via its IClassFactory, then
// calls DoVerb(OLEIVERB_INPLACEACTIVATE) to embed it visually inside the pane.
//
// INTERFACE CHAIN
// ---------------
// The control must satisfy a specific set of COM interfaces that Office checks
// during the hosting sequence:
//
//   IOleObject           — object identity, lifecycle, verbs (DoVerb).
//   IOleInPlaceObject    — provides the control's native HWND; handles
//                          repositioning and deactivation.
//   IOleInPlaceActiveObject — keyboard / modeless-dialog coordination with
//                          the host frame while the control is UI-active.
//   IViewObject          — lets the host render the control into a DC (used
//                          for print-preview and drag-drop thumbnails).
//   IPersistStreamInit   — allows the host to save/restore the control's state
//                          in a stream.  We carry no persistent state, so all
//                          methods are no-ops.
//   IOleControl          — marks this as an ActiveX "control" (vs. a plain OLE
//                          object); provides accelerator/mnemonic info.
//
// THREADING MODEL
// ---------------
// Excel is an STA application.  All methods are called on Excel's main thread.
// There is no secondary thread, no wx event loop, and no message pump owned by
// this DLL — Excel's own pump processes WM_* messages for the child HWND.
//
// WXWIDGETS USAGE
// ---------------
// wxWidgets is initialised lazily the first time the control is activated.
// wxNativeContainerWindow wraps the pre-existing Win32 HWND so that ordinary
// wxPanel / wxButton children can be parented to it.  Because there is no wx
// message loop, wx teardown APIs (wxApp::OnExit, wxEntryCleanup) are NOT safe
// to call while windows still exist; see deactivate() for the workaround.
//
// MUST be included AFTER COM/Macros.hpp (which brings in COMServer.hpp and
// its global variables: g_lockCount, g_hModule, detail:: helpers).
// ---------------------------------------------------------------------------

#pragma once

// wx headers first — they include <windows.h> internally.
#include <wx/app.h>
#include <wx/button.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/nativewin.h>

// OLE / ActiveX interfaces (from Windows SDK).
#include <oleidl.h>     // IOleObject, IOleInPlaceObject, IOleInPlaceSite …
#include <ocidl.h>      // IOleControl, IPersistStreamInit, CONTROLINFO …

#include <iostream>
#include <new>

// ---------------------------------------------------------------------------
// CLSID & ProgID for the TaskPaneControl.
//
// The CLSID is the binary key used by COM's class-object lookup (CoCreateInstance
// / DllGetClassObject).  It is stored in the registry under HKCR\CLSID\{...}.
//
// The ProgID is a human-readable string alias for the CLSID.  Excel VBA can
// refer to the control by ProgID when constructing a custom task pane:
//   Application.CTP.Add "xlCOM.TaskPaneCtrl"
//
// {7C2D4F8A-3E1B-4A5C-9D6E-8F0B2A1C3D5E}
// ---------------------------------------------------------------------------

inline constexpr CLSID CLSID_TaskPaneControl =
    {0x7C2D4F8A, 0x3E1B, 0x4A5C, {0x9D, 0x6E, 0x8F, 0x0B, 0x2A, 0x1C, 0x3D, 0x5E}};

// Human-readable ProgID registered under HKCR\xlCOM.TaskPaneCtrl.
inline constexpr wchar_t kProgID_TaskPane[] = L"xlCOM.TaskPaneCtrl";

// ---------------------------------------------------------------------------
// detail namespace — internal helpers for wx initialisation and Win32 window
// class registration.  Not part of the public API.
// ---------------------------------------------------------------------------

namespace detail {

// ---------------------------------------------------------------------------
// wxReadyFlag()
//
// Returns a reference to a process-wide boolean that tracks whether
// wxEntryStart() has been called successfully.  Using a function-local static
// makes initialisation order well-defined and avoids the static-initialisation
// order fiasco across translation units.
// ---------------------------------------------------------------------------
inline bool& wxReadyFlag()
{
    static bool ready = false;
    return ready;
}

// ---------------------------------------------------------------------------
// ensureWxInit()
//
// Lazily initialises the wxWidgets subsystem the first time it is called.
// Safe to call multiple times — subsequent calls are no-ops.
//
// Steps:
//   1. If already initialised (flag set), return immediately.
//   2. If wxTheApp already exists (someone else initialised wx), adopt it and
//      set the flag.
//   3. Otherwise allocate a bare wxApp, call wxEntryStart() to set up the
//      Win32 message infrastructure, and call CallOnInit() so that any
//      wxApp::OnInit override runs.
//
// SetExitOnFrameDelete(false) is critical: without it, wx would call
// wxExit() when the first top-level window closes, killing the host process.
//
// Returns true on success, false if wxEntryStart() fails.
// ---------------------------------------------------------------------------
inline bool ensureWxInit()
{
    if (wxReadyFlag()) return true;
    if (wxTheApp) { wxReadyFlag() = true; return true; }

    int argc = 0;
    wxApp::SetInstance(new wxApp());
    if (!wxEntryStart(argc, static_cast<char**>(nullptr)))
        return false;

    if (wxTheApp)
    {
        wxTheApp->SetExitOnFrameDelete(false);
        wxTheApp->CallOnInit();
    }
    wxReadyFlag() = true;
    return true;
}

// ---------------------------------------------------------------------------
// shutdownWx()
//
// Tears down the wxWidgets subsystem.  Should be called from DllMain on
// DLL_PROCESS_DETACH (or equivalent) after all controls have been destroyed.
//
// NOTE: It is NOT safe to call this while any wxWindow objects still exist,
// because wxEntryCleanup() calls DestroyAllWindows() and similar routines that
// require a consistent wx object graph.  See deactivate() for why the control
// intentionally abandons (rather than destroys) its wx objects.
// ---------------------------------------------------------------------------
inline void shutdownWx()
{
    if (!wxReadyFlag()) return;
    if (wxTheApp) wxTheApp->OnExit();
    wxEntryCleanup();
    wxReadyFlag() = false;
}

// ---------------------------------------------------------------------------
// controlWindowClass()
//
// Registers a Win32 window class for the control's container HWND and returns
// its name.  Uses a local static flag so that RegisterClassExW is called only
// once per process.
//
// The window class uses DefWindowProcW as its WndProc (all standard message
// handling) and CS_HREDRAW | CS_VREDRAW so the window redraws completely when
// resized — appropriate for a control that delegates painting to its children.
//
// g_hModule is the HINSTANCE of the hosting DLL, supplied by COMServer.hpp.
// ---------------------------------------------------------------------------
inline const wchar_t* controlWindowClass()
{
    static bool registered = false;
    static const wchar_t name[] = L"xlCOM_TaskPaneCtrl";
    if (!registered)
    {
        WNDCLASSEXW wc = { sizeof(wc) };
        wc.lpfnWndProc   = DefWindowProcW;
        wc.hInstance      = g_hModule;
        wc.lpszClassName  = name;
        wc.hbrBackground  = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1); // system window colour
        wc.style          = CS_HREDRAW | CS_VREDRAW;
        RegisterClassExW(&wc);
        registered = true;
    }
    return name;
}

} // namespace detail

// ===========================================================================
// TaskPaneControl
//
// The concrete COM class that Office hosts in a custom task pane.
//
// Lifetime management
// -------------------
// m_ref is a per-instance reference count managed with Interlocked operations
// so that AddRef/Release are thread-safe even though the STA contract means
// they are always called on the same thread in practice.
//
// g_lockCount (from COMServer.hpp) is the DLL-wide server lock count.  It is
// incremented in the constructor and decremented in Release() when the object
// is destroyed, ensuring that DllCanUnloadNow() returns FALSE while any
// instance is alive.
//
// Activation lifecycle (happy path)
// ----------------------------------
//   1. Excel calls SetClientSite() — stores a back-pointer to the host site.
//   2. Excel calls DoVerb(OLEIVERB_INPLACEACTIVATE) — triggers activateInPlace(),
//      which creates the Win32 child window and builds the wxWidgets content.
//   3. Excel may call SetObjectRects() repeatedly as the task pane is resized.
//   4. Excel calls Close() or InPlaceDeactivate() to tear down the control.
// ===========================================================================

class TaskPaneControl
    : public IOleObject              // core OLE embedding interface
    , public IOleInPlaceObject       // in-place window management
    , public IOleInPlaceActiveObject // keyboard / frame interaction while active
    , public IViewObject             // rendering into an arbitrary DC
    , public IPersistStreamInit      // stream-based persistence (all no-ops here)
    , public IOleControl             // ActiveX control marker + accelerator info
{
public:
    // Constructor — starts the reference count at 1 (caller owns the initial ref)
    // and increments the DLL server lock so the DLL cannot be unloaded while
    // this instance exists.
    TaskPaneControl() : m_ref(1) { InterlockedIncrement(&g_lockCount); }

    // Destructor — deactivates the control (destroys the HWND and notifies the
    // in-place site) and releases the stored client-site pointer.
    ~TaskPaneControl()
    {
        deactivate();
        if (m_clientSite) { m_clientSite->Release(); m_clientSite = nullptr; }
    }

    // =========================================================================
    // IUnknown
    //
    // QueryInterface maps requested IIDs to the correct vtable pointer for each
    // inherited interface.  Because C++ multiple inheritance produces multiple
    // vtables, each interface must be cast to its own base type before being
    // returned — returning "this" uncasted would give the wrong vtable for any
    // interface other than the first in the inheritance list.
    //
    // Special cases:
    //   IID_IDispatch  — Office occasionally QI's for IDispatch to invoke
    //                    ambient-property queries.  We alias it to IOleObject*
    //                    rather than implementing a real ITypeInfo / Invoke;
    //                    this satisfies the QI without crashing callers that
    //                    only check the pointer for null.
    //   IID_IOleWindow — IOleInPlaceObject inherits IOleWindow, so either IID
    //                    resolves to the same vtable pointer.
    //   IID_IViewObject2 — IViewObject2 extends IViewObject with GetExtent; we
    //                    don't implement the extra method but returning the
    //                    IViewObject vtable lets callers that simply check
    //                    presence proceed without failure.
    //   IID_IPersist   — IPersistStreamInit inherits IPersist (GetClassID), so
    //                    either IID resolves to the IPersistStreamInit vtable.
    // =========================================================================

    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override
    {
        if (!ppv) return E_POINTER;

        if (riid == IID_IUnknown || riid == IID_IOleObject)
            *ppv = static_cast<IOleObject*>(this);
        else if (riid == IID_IDispatch)
            *ppv = static_cast<IOleObject*>(this);  // minimal IDispatch — alias to IOleObject
        else if (riid == IID_IOleInPlaceObject || riid == IID_IOleWindow)
            *ppv = static_cast<IOleInPlaceObject*>(this);
        else if (riid == IID_IOleInPlaceActiveObject)
            *ppv = static_cast<IOleInPlaceActiveObject*>(this);
        else if (riid == IID_IViewObject || riid == IID_IViewObject2)
            *ppv = static_cast<IViewObject*>(this);
        else if (riid == IID_IPersist || riid == IID_IPersistStreamInit)
            *ppv = static_cast<IPersistStreamInit*>(this);
        else if (riid == IID_IOleControl)
            *ppv = static_cast<IOleControl*>(this);
        else
        {
            *ppv = nullptr;
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    }

    // Standard thread-safe reference counting via Interlocked operations.
    STDMETHODIMP_(ULONG) AddRef()  override { return InterlockedIncrement(&m_ref); }

    // Release decrements the count.  When it reaches zero the object destroys
    // itself and decrements the DLL-wide lock count so the server can unload.
    STDMETHODIMP_(ULONG) Release() override
    {
        const LONG r = InterlockedDecrement(&m_ref);
        if (r == 0) { InterlockedDecrement(&g_lockCount); delete this; }
        return r;
    }

    // =========================================================================
    // IOleObject
    //
    // IOleObject is the primary interface through which a container manages an
    // embedded or linked object.  For an in-process ActiveX control hosted in a
    // task pane, the most important methods are SetClientSite, DoVerb,
    // GetMiscStatus, and GetUserClassID.  The remainder are either not called by
    // Office for this hosting scenario or are genuinely not needed for a
    // windowless-free, non-linked, in-process control.
    // =========================================================================

    // SetClientSite — called by the container immediately after creating the
    // object.  The IOleClientSite pointer is the control's back-channel to the
    // host: through it the control can query for IOleInPlaceSite, request
    // repaints, navigate links, etc.  We store and AddRef the pointer.
    STDMETHODIMP SetClientSite(IOleClientSite* pSite) override
    {
        if (m_clientSite) m_clientSite->Release();
        m_clientSite = pSite;
        if (m_clientSite) m_clientSite->AddRef();
        return S_OK;
    }

    // GetClientSite — returns the currently stored client site with an AddRef'd
    // pointer so the caller is responsible for Release.
    STDMETHODIMP GetClientSite(IOleClientSite** ppSite) override
    {
        if (!ppSite) return E_POINTER;
        *ppSite = m_clientSite;
        if (m_clientSite) m_clientSite->AddRef();
        return S_OK;
    }

    // SetHostNames — the container supplies the application name and document
    // name so the object can use them in window title bars when open-edited.
    // A task-pane control is always in-place; it never shows its own title bar,
    // so this information is irrelevant.
    STDMETHODIMP SetHostNames(LPCOLESTR, LPCOLESTR) override { return S_OK; }

    // Close — the container is requesting that the object transition to the
    // loaded (not active) state.  We simply deactivate, which destroys the HWND
    // and notifies the in-place site.  The dwSaveOption (OLECLOSE_SAVEIFDIRTY,
    // OLECLOSE_NOSAVE, OLECLOSE_PROMPTSAVE) is ignored because the control has
    // no persistent dirty state.
    STDMETHODIMP Close(DWORD /*dwSaveOption*/) override
    {
        deactivate();
        return S_OK;
    }

    // DoVerb — executes a named action on the object.  This is the main entry
    // point through which Office activates the control.
    //
    // Recognised verbs:
    //   OLEIVERB_INPLACEACTIVATE (-5) — create the HWND and become in-place
    //                                   active, but do NOT show the focus rect
    //                                   or merge menus (no UI activation).
    //   OLEIVERB_UIACTIVATE      (-4) — as above, plus notify the site that we
    //                                   are UI-active (the control can now handle
    //                                   keyboard input and merge menus).
    //   OLEIVERB_SHOW            (-1) — make the object visible; treated as
    //                                   in-place activation because the control
    //                                   has no open-edit state.
    //   OLEIVERB_HIDE            (-3) — hide the HWND without destroying it.
    //
    // Any other verb returns E_NOTIMPL, which is correct per the OLE spec for
    // verbs the object does not support.
    STDMETHODIMP DoVerb(LONG iVerb, LPMSG /*lpmsg*/,
                         IOleClientSite* /*pActiveSite*/, LONG /*lindex*/,
                         HWND hwndParent, LPCRECT posRect) override
    {
        std::cerr << "[xlCOM] TaskPaneControl::DoVerb iVerb=" << iVerb << '\n';

        if (iVerb == OLEIVERB_INPLACEACTIVATE ||
            iVerb == OLEIVERB_UIACTIVATE      ||
            iVerb == OLEIVERB_SHOW)
        {
            const bool wantUI = (iVerb == OLEIVERB_UIACTIVATE);
            return activateInPlace(hwndParent, posRect, wantUI);
        }
        if (iVerb == OLEIVERB_HIDE)
        {
            if (m_hwnd) ShowWindow(m_hwnd, SW_HIDE);
            return S_OK;
        }
        return E_NOTIMPL;
    }

    // SetExtent — the container tells the object its desired display size in
    // HIMETRIC units (100 nm per unit, i.e. 1/100 mm).  We store it so that
    // GetExtent can echo it back; actual pixel sizing is driven by
    // SetObjectRects() and MoveWindow().
    STDMETHODIMP SetExtent(DWORD /*dwAspect*/, SIZEL* pSizel) override
    {
        if (!pSizel) return E_POINTER;
        m_extent = *pSizel;
        return S_OK;
    }

    // GetExtent — returns the control's preferred size to the container.
    // We return whatever was last set by SetExtent, or the 5000×5000 HIMETRIC
    // default (≈ 5 cm × 5 cm) if SetExtent has not been called yet.
    STDMETHODIMP GetExtent(DWORD /*dwAspect*/, SIZEL* pSizel) override
    {
        if (!pSizel) return E_POINTER;
        *pSizel = m_extent;
        return S_OK;
    }

    // GetUserClassID — returns the CLSID that identifies this class to users
    // (e.g. for display in object browsers).  For a simple control the user
    // class ID equals the server CLSID.
    STDMETHODIMP GetUserClassID(CLSID* pClsid) override
    {
        if (!pClsid) return E_POINTER;
        *pClsid = CLSID_TaskPaneControl;
        return S_OK;
    }

    // GetMiscStatus — returns a bitmask of OLEMISC_* flags that describe the
    // object's capabilities and requirements to the container:
    //
    //   OLEMISC_RECOMPOSEONRESIZE   — the control wants to be notified (via
    //                                 SetObjectRects) when its size changes so
    //                                 it can recompose its content.
    //   OLEMISC_INSIDEOUT           — the control can activate in-place; the
    //                                 container need not open a separate window.
    //   OLEMISC_ACTIVATEWHENVISIBLE — the container should automatically call
    //                                 DoVerb(OLEIVERB_INPLACEACTIVATE) when the
    //                                 object first becomes visible.
    //   OLEMISC_SETCLIENTSITEFIRST  — the container must call SetClientSite
    //                                 before IPersistStorage::Load or
    //                                 IPersistStreamInit::InitNew/Load so that
    //                                 the object can read ambient properties
    //                                 during initialisation.
    STDMETHODIMP GetMiscStatus(DWORD /*dwAspect*/, DWORD* pdwStatus) override
    {
        if (!pdwStatus) return E_POINTER;
        *pdwStatus = OLEMISC_RECOMPOSEONRESIZE
                   | OLEMISC_INSIDEOUT
                   | OLEMISC_ACTIVATEWHENVISIBLE
                   | OLEMISC_SETCLIENTSITEFIRST;
        return S_OK;
    }

    // ---- IOleObject stubs ------------------------------------------------
    // The following methods are part of the IOleObject contract but are not
    // required for a simple in-place-only task-pane control.  Each returns
    // E_NOTIMPL (or S_OK where the spec says the object may silently succeed)
    // so that the container knows the feature is absent rather than receiving
    // a crash or silent misbehaviour.

    // SetMoniker / GetMoniker — used for linked (not embedded) objects to
    // associate a persistent moniker (file path, URL, etc.).  Not applicable.
    STDMETHODIMP SetMoniker(DWORD, IMoniker*)          override { return E_NOTIMPL; }
    STDMETHODIMP GetMoniker(DWORD, DWORD, IMoniker**)  override { return E_NOTIMPL; }

    // InitFromData — initialise the object from an IDataObject (drag-drop or
    // clipboard paste).  The control has no data transfer format.
    STDMETHODIMP InitFromData(IDataObject*, BOOL, DWORD) override { return E_NOTIMPL; }

    // GetClipboardData — produce an IDataObject suitable for the clipboard.
    // The control provides no clipboard content of its own.
    STDMETHODIMP GetClipboardData(DWORD, IDataObject**) override { return E_NOTIMPL; }

    // EnumVerbs — enumerates the set of verbs the object supports.  Returning
    // E_NOTIMPL causes the container to fall back to the registry-stored verb
    // list under HKCR\CLSID\{...}\Verb\, which we do not populate; the container
    // must therefore know to call DoVerb directly with standard verb IDs.
    STDMETHODIMP EnumVerbs(IEnumOLEVERB**)              override { return E_NOTIMPL; }

    // Update — request that any embedded sub-objects update their caches.
    // No sub-objects; return S_OK to indicate nothing needed doing.
    STDMETHODIMP Update()                               override { return S_OK; }

    // IsUpToDate — query whether all sub-object caches are current.
    // Always current (no sub-objects), so S_OK (= up to date).
    STDMETHODIMP IsUpToDate()                           override { return S_OK; }

    // GetUserType — returns a human-readable type name for the object
    // (e.g. "Spreadsheet").  Not used in the task-pane scenario.
    STDMETHODIMP GetUserType(DWORD, LPOLESTR*)          override { return E_NOTIMPL; }

    // Advise / Unadvise / EnumAdvise — the OLE advisory notification mechanism
    // for data / view change events (distinct from IConnectionPoint).  The
    // control raises no IOleObject-level notifications.
    STDMETHODIMP Advise(IAdviseSink*, DWORD*)           override { return E_NOTIMPL; }
    STDMETHODIMP Unadvise(DWORD)                        override { return E_NOTIMPL; }
    STDMETHODIMP EnumAdvise(IEnumSTATDATA**)            override { return E_NOTIMPL; }

    // SetColorScheme — passes a LOGPALETTE describing the container's palette.
    // Not used; the control paints with system colours.
    STDMETHODIMP SetColorScheme(LOGPALETTE*)            override { return E_NOTIMPL; }

    // =========================================================================
    // IOleWindow / IOleInPlaceObject
    //
    // IOleInPlaceObject inherits IOleWindow and adds the in-place deactivation
    // and rectangle-update methods.  These are called by the container to manage
    // the control's visible window during the in-place active state.
    // =========================================================================

    // GetWindow — returns the control's container HWND.  The host uses this
    // handle to parent its own UI or to set focus.  Returns E_FAIL if the
    // control has not yet been activated (no HWND exists).
    STDMETHODIMP GetWindow(HWND* phwnd) override
    {
        if (!phwnd) return E_POINTER;
        *phwnd = m_hwnd;
        return m_hwnd ? S_OK : E_FAIL;
    }

    // ContextSensitiveHelp — called when the user presses F1 while the object
    // is active.  Not implemented; the host handles help navigation.
    STDMETHODIMP ContextSensitiveHelp(BOOL) override { return E_NOTIMPL; }

    // InPlaceDeactivate — the container requests full deactivation (both UI and
    // in-place).  Delegates to deactivate() which destroys the HWND and
    // releases the in-place site.
    STDMETHODIMP InPlaceDeactivate() override
    {
        deactivate();
        return S_OK;
    }

    // UIDeactivate — the container requests that the control give up its UI
    // activation state (remove focus rectangles, de-merge menus) but remain
    // in-place active (HWND stays visible).  Because this control does not
    // merge menus or install accelerators there is nothing to undo.
    STDMETHODIMP UIDeactivate() override { return S_OK; }

    // SetObjectRects — called whenever the control's position or size changes
    // within its container (e.g. when the user resizes the task pane).
    //
    // posRect is in the coordinate space of the container window.
    // clipRect is the visible portion after clipping (ignored here; we trust
    // WS_CLIPCHILDREN on the parent to handle overdraw).
    //
    // We move the Win32 HWND to the new position, then tell the wxPanel to
    // resize itself so that its sizer re-lays out the child widgets.
    STDMETHODIMP SetObjectRects(LPCRECT posRect, LPCRECT /*clipRect*/) override
    {
        if (!posRect) return E_POINTER;
        if (m_hwnd)
        {
            const int w = posRect->right  - posRect->left;
            const int h = posRect->bottom - posRect->top;
            MoveWindow(m_hwnd, posRect->left, posRect->top, w, h, TRUE);
            if (m_wxPanel)
            {
                m_wxPanel->SetSize(0, 0, w, h);
                m_wxPanel->Layout();
            }
        }
        return S_OK;
    }

    // ReactivateAndUndo — reactivate after an undo that reversed the last
    // activation.  Not applicable to a non-document control.
    STDMETHODIMP ReactivateAndUndo() override { return E_NOTIMPL; }

    // =========================================================================
    // IOleInPlaceActiveObject
    //
    // These methods are called by the container frame while the control is the
    // "active object" — i.e. it holds keyboard focus and participates in the
    // host's message loop.  For a control that does not merge menus or install
    // frame-level accelerators, all methods are trivial.
    // =========================================================================

    // TranslateAccelerator — the host offers each WM_KEYDOWN message to the
    // active object before processing it itself.  Returning S_FALSE tells the
    // host that the control did not consume the keystroke and the host should
    // continue with its own accelerator table.
    STDMETHODIMP TranslateAccelerator(LPMSG) override { return S_FALSE; }

    // OnFrameWindowActivate — the top-level Excel frame window has been
    // activated or deactivated (e.g. user Alt-Tabbed away).  No action needed.
    STDMETHODIMP OnFrameWindowActivate(BOOL)  override { return S_OK; }

    // OnDocWindowActivate — the MDI document window (workbook) has been
    // activated or deactivated.  No action needed.
    STDMETHODIMP OnDocWindowActivate(BOOL)    override { return S_OK; }

    // ResizeBorder — the host frame or document window border has changed size;
    // the active object may need to resize its toolbar bands.  This control has
    // no toolbar bands.
    STDMETHODIMP ResizeBorder(LPCRECT, IOleInPlaceUIWindow*, BOOL) override
    { return S_OK; }

    // EnableModeless — the host is about to display (fEnable=FALSE) or has
    // dismissed (fEnable=TRUE) a modal dialog.  The control should disable or
    // re-enable any modeless windows it owns.  We have none in the minimal
    // implementation.
    STDMETHODIMP EnableModeless(BOOL) override { return S_OK; }

    // =========================================================================
    // IViewObject
    //
    // IViewObject lets the container render the object's visual content into an
    // arbitrary DC — used for print-preview, drag-drop thumbnails, and metafile
    // capture.  The control is a live HWND window and its children draw
    // themselves, so a full metafile render is not implemented.  Draw() provides
    // a minimal fill so that drag thumbnails are not completely blank.
    // =========================================================================

    // Draw — render the object into hdcDraw within the bounding rectangle
    // lprcBounds (in logical coordinates of hdcDraw).  We simply fill the
    // rectangle with COLOR_WINDOW (the system window background colour) as a
    // placeholder.  The control parameters (aspect, index, pAspectInfo,
    // ptd, hdcTargetDev, continue-callback) are all ignored.
    STDMETHODIMP Draw(DWORD, LONG, void*, DVTARGETDEVICE*, HDC,
                       HDC hdcDraw, LPCRECTL lprcBounds, LPCRECTL,
                       BOOL(CALLBACK*)(ULONG_PTR), ULONG_PTR) override
    {
        // Minimal paint — fill with the window background colour.
        if (hdcDraw && lprcBounds)
        {
            RECT rc = { lprcBounds->left, lprcBounds->top,
                        lprcBounds->right, lprcBounds->bottom };
            FillRect(hdcDraw, &rc,
                     reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
        }
        return S_OK;
    }

    // GetColorSet — returns a LOGPALETTE describing the colours the object
    // uses for rendering.  Not implemented; the control uses system colours.
    STDMETHODIMP GetColorSet(DWORD, LONG, void*, DVTARGETDEVICE*,
                              HDC, LOGPALETTE**) override
    { return E_NOTIMPL; }

    // Freeze / Unfreeze — freeze/thaw the object's visual representation at a
    // specific aspect so that subsequent Draw() calls show a static snapshot.
    // Not implemented; the control always draws its current live state.
    STDMETHODIMP Freeze(DWORD, LONG, void*, DWORD*) override { return E_NOTIMPL; }
    STDMETHODIMP Unfreeze(DWORD)                     override { return E_NOTIMPL; }

    // SetAdvise — register an IAdviseSink to be notified when the view changes
    // (e.g. the object needs the container to repaint).  We store the sink and
    // AddRef it.  In a full implementation we would call
    // pSink->OnViewChange(DVASPECT_CONTENT, -1) when the content changes.
    STDMETHODIMP SetAdvise(DWORD, DWORD, IAdviseSink* pSink) override
    {
        if (m_adviseSink) m_adviseSink->Release();
        m_adviseSink = pSink;
        if (m_adviseSink) m_adviseSink->AddRef();
        return S_OK;
    }

    // GetAdvise — retrieves the currently registered view advise sink and the
    // aspect / advise flags it was registered with.  We always report
    // DVASPECT_CONTENT and no special flags.
    STDMETHODIMP GetAdvise(DWORD* pAspects, DWORD* pAdvf,
                            IAdviseSink** ppSink) override
    {
        if (pAspects) *pAspects = DVASPECT_CONTENT;
        if (pAdvf)    *pAdvf    = 0;
        if (ppSink)
        {
            *ppSink = m_adviseSink;
            if (m_adviseSink) m_adviseSink->AddRef();
        }
        return S_OK;
    }

    // =========================================================================
    // IPersist / IPersistStreamInit
    //
    // These interfaces allow the container to save and restore the control's
    // internal state to/from a COM stream (IStream).  This control has no
    // persistent state, so all methods succeed immediately without reading or
    // writing any data.  IPersist::GetClassID is still needed because it lets
    // the container identify which class to instantiate when it later loads the
    // saved document.
    // =========================================================================

    // GetClassID — returns the CLSID of this class so the container can
    // re-create the correct object when loading a saved document.
    STDMETHODIMP GetClassID(CLSID* pClsid) override
    {
        if (!pClsid) return E_POINTER;
        *pClsid = CLSID_TaskPaneControl;
        return S_OK;
    }

    // IsDirty — S_FALSE means the object has no unsaved changes.  Because the
    // control carries no persistent state, it is never "dirty".
    STDMETHODIMP IsDirty() override { return S_FALSE; }

    // Load — called by the container to restore state from a previously saved
    // IStream.  Nothing to restore; return S_OK.
    STDMETHODIMP Load(LPSTREAM) override { return S_OK; }

    // Save — called by the container to save current state into an IStream for
    // later reloading.  Nothing to save; return S_OK.
    STDMETHODIMP Save(LPSTREAM, BOOL) override { return S_OK; }

    // GetSizeMax — the container queries the maximum number of bytes the object
    // would write in Save().  We always write zero bytes.
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER* pcbSize) override
    {
        if (pcbSize) pcbSize->QuadPart = 0;
        return S_OK;
    }

    // InitNew — called instead of Load() when the control is being created
    // fresh (no saved stream to restore from).  Initialise to defaults.
    // Nothing to initialise; return S_OK.
    STDMETHODIMP InitNew() override { return S_OK; }

    // =========================================================================
    // IOleControl
    //
    // IOleControl is the interface that distinguishes ActiveX controls from
    // plain OLE objects.  Its primary method, GetControlInfo, supplies
    // accelerator key information so the container can pre-dispatch keystrokes
    // to the control.  The other methods handle event freezing and ambient
    // property changes.
    // =========================================================================

    // GetControlInfo — fills a CONTROLINFO structure describing the control's
    // mnemonic accelerators.  We have none, so hAccel=nullptr / cAccel=0.
    // dwFlags=0 means the control does not want RETURN or ESC translated for it.
    STDMETHODIMP GetControlInfo(CONTROLINFO* pCI) override
    {
        if (!pCI) return E_POINTER;
        pCI->cb      = sizeof(CONTROLINFO);
        pCI->hAccel  = nullptr; // no accelerator table
        pCI->cAccel  = 0;       // no accelerator entries
        pCI->dwFlags = 0;       // CTRLINFO_EATS_RETURN / CTRLINFO_EATS_ESCAPE not set
        return S_OK;
    }

    // OnMnemonic — the user pressed a mnemonic key that matches one of the
    // accelerators returned in GetControlInfo.  We register none, so this is
    // never called in practice.
    STDMETHODIMP OnMnemonic(MSG*) override { return S_OK; }

    // OnAmbientPropertyChange — the container notifies the control that one of
    // its ambient properties (background colour, font, locale, etc.) has
    // changed.  The control could re-query the property via IDispatch on the
    // client site.  We do not read ambient properties, so this is a no-op.
    STDMETHODIMP OnAmbientPropertyChange(DISPID) override { return S_OK; }

    // FreezeEvents — when fFreeze=TRUE, the control must stop firing events
    // (e.g. connection-point events) until a matching FreezeEvents(FALSE) call.
    // This control fires no COM events, so this is a no-op.
    STDMETHODIMP FreezeEvents(BOOL) override { return S_OK; }

    // =========================================================================
    // Private members
    // =========================================================================

private:
    // COM reference count (per-instance, Interlocked).
    LONG              m_ref;

    // Back-pointer to the container's client site.  Stored from SetClientSite();
    // used in activateInPlace() to QI for IOleInPlaceSite.
    IOleClientSite*   m_clientSite  = nullptr;

    // The in-place site interface obtained during activation.  Used to call
    // OnInPlaceActivate, OnUIActivate, GetWindow, GetWindowContext, and the
    // corresponding deactivation notifications.  Released in deactivate().
    IOleInPlaceSite*  m_inPlaceSite = nullptr;

    // Optional view-change advise sink set by SetAdvise().  Not actively used
    // in this implementation but stored so GetAdvise() can return it.
    IAdviseSink*      m_adviseSink  = nullptr;

    // The Win32 child window created during in-place activation.  This is the
    // HWND that the container embeds in its task pane.  It is the parent of all
    // wxWidgets windows created in createWxContent().
    HWND              m_hwnd        = nullptr;

    // Guard flag: true while the control is in-place active.
    // Prevents re-entrant activation and makes deactivate() idempotent.
    bool              m_active      = false;

    // Preferred display size in HIMETRIC units (1 unit = 0.01 mm).
    // Default 5000×5000 ≈ 5 cm × 5 cm.  Echoed back by GetExtent().
    SIZEL             m_extent      = { 5000, 5000 };

    // wxNativeContainerWindow wraps the pre-existing Win32 HWND (m_hwnd) so
    // that wxWidgets child windows can be parented to it normally.  Ownership
    // is via the raw pointer; see deactivate() for why we do NOT call Destroy().
    wxNativeContainerWindow* m_wxContainer = nullptr;

    // The top-level wxPanel inside m_wxContainer.  All UI widgets are children
    // of this panel.  Same ownership caveat as m_wxContainer.
    wxPanel*                 m_wxPanel     = nullptr;

    // -------------------------------------------------------------------------
    // activateInPlace
    //
    // Called from DoVerb() when the control should become in-place active.
    //
    // Steps:
    //   1. Guard against re-entrant or duplicate activation.
    //   2. QI the client site for IOleInPlaceSite and ask whether it accepts
    //      in-place activation.
    //   3. Call OnInPlaceActivate() to tell the site the control is now active.
    //   4. Retrieve the parent HWND and the position rectangle from the site via
    //      GetWindowContext().  If the site provides an HWND, use it; otherwise
    //      fall back to hwndParent from DoVerb.
    //   5. Create a Win32 child window (WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN)
    //      at the supplied position.  WS_CLIPCHILDREN prevents the container
    //      window from painting over the control's wx children.
    //   6. Optionally call OnUIActivate() if the caller requested full UI
    //      activation (focus + menu merging).
    //   7. Build the wxWidgets content tree inside the new HWND.
    //
    // uiActivate — when true, call OnUIActivate() after creating the window.
    //              This signals to the host that the control has keyboard focus
    //              and may merge menus / install accelerators.
    // -------------------------------------------------------------------------
    HRESULT activateInPlace(HWND hwndParent, LPCRECT posRect, bool uiActivate)
    {
        if (m_active) return S_OK;

        std::cerr << "[xlCOM] TaskPaneControl::activateInPlace"
                  << (uiActivate ? " (UI)" : " (in-place only)") << '\n';

        // Obtain IOleInPlaceSite from the client site.
        if (!m_clientSite) return E_FAIL;
        HRESULT hr = m_clientSite->QueryInterface(
            IID_IOleInPlaceSite, reinterpret_cast<void**>(&m_inPlaceSite));
        if (FAILED(hr))
        {
            std::cerr << "[xlCOM]   QI for IOleInPlaceSite failed hr=0x"
                      << std::hex << hr << std::dec << '\n';
            return hr;
        }

        // CanInPlaceActivate() returns S_OK if the site permits in-place
        // activation.  Any other return (S_FALSE, E_*) means we cannot proceed.
        if (m_inPlaceSite->CanInPlaceActivate() != S_OK)
        {
            std::cerr << "[xlCOM]   CanInPlaceActivate returned failure\n";
            return E_FAIL;
        }
        // Notify the site that in-place activation has begun.  The site will
        // typically disable its own scroll bars and prepare the container window
        // to host the control's HWND.
        m_inPlaceSite->OnInPlaceActivate();

        // Retrieve parent window and position from the site.
        HWND                 siteHwnd  = nullptr;
        IOleInPlaceFrame*    pFrame    = nullptr;  // frame-level UI window
        IOleInPlaceUIWindow* pUIWin    = nullptr;  // document-level UI window
        RECT                 posRect2  = {};
        RECT                 clipRect  = {};
        OLEINPLACEFRAMEINFO  frameInfo = { sizeof(frameInfo) };

        // Use the caller-supplied rect as the initial position.  GetWindowContext
        // may override it with a more accurate value from the site.
        if (posRect)
            posRect2 = *posRect;

        // GetWindow() gives us the container HWND to use as our parent.
        m_inPlaceSite->GetWindow(&siteHwnd);
        // GetWindowContext() fills posRect2 and clipRect with the control's
        // position within the container, and supplies pFrame / pUIWin for menu
        // merging and toolbar negotiation.  We release both immediately because
        // we do not perform any toolbar or menu negotiation.
        m_inPlaceSite->GetWindowContext(&pFrame, &pUIWin,
                                         &posRect2, &clipRect, &frameInfo);
        if (pFrame) pFrame->Release();
        if (pUIWin) pUIWin->Release();

        // Prefer the HWND reported by the site; fall back to what DoVerb gave us.
        HWND parentHwnd = siteHwnd ? siteHwnd : hwndParent;
        std::cerr << "[xlCOM]   parentHwnd=" << static_cast<void*>(parentHwnd)
                  << " rect=(" << posRect2.left << ',' << posRect2.top
                  << ',' << posRect2.right << ',' << posRect2.bottom << ")\n";

        if (!parentHwnd) return E_FAIL;

        // Create the child HWND.  Clamp to at least 1×1 in case the container
        // reports a zero-size rect before layout has completed.
        int w = posRect2.right  - posRect2.left;
        int h = posRect2.bottom - posRect2.top;
        if (w <= 0) w = 1;
        if (h <= 0) h = 1;

        m_hwnd = CreateWindowExW(
            0,                              // no extended styles
            detail::controlWindowClass(),   // our registered window class
            L"",                            // no caption
            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
            posRect2.left, posRect2.top, w, h,
            parentHwnd, nullptr, g_hModule, nullptr);

        if (!m_hwnd)
        {
            std::cerr << "[xlCOM]   CreateWindowEx failed err="
                      << GetLastError() << '\n';
            return E_FAIL;
        }

        m_active = true;

        // Only UI-activate when explicitly asked.  UI-activation signals that
        // the control owns keyboard focus and may merge menus.  For
        // OLEIVERB_INPLACEACTIVATE and OLEIVERB_SHOW we skip this step.
        if (uiActivate)
            m_inPlaceSite->OnUIActivate();

        // Build the wxWidgets content tree inside m_hwnd.
        createWxContent(w, h);

        ShowWindow(m_hwnd, SW_SHOW);
        std::cerr << "[xlCOM]   TaskPaneControl activated OK\n";
        return S_OK;
    }

    // -------------------------------------------------------------------------
    // createWxContent
    //
    // Builds the wxWidgets window hierarchy inside the already-created m_hwnd.
    // Called once from activateInPlace().
    //
    // wxNativeContainerWindow adopts m_hwnd: it does not create a new Win32
    // window but instead wraps the existing HWND so that wx can treat it as a
    // top-level window and parent ordinary wx child windows to it.
    //
    // Layout:
    //   wxNativeContainerWindow (wraps m_hwnd)
    //     └─ wxPanel (m_wxPanel, full size, no border)
    //          └─ wxBoxSizer (vertical)
    //               ├─ stretch spacer
    //               ├─ wxButton ("Click me!", centred)
    //               └─ stretch spacer
    //
    // The button's wxEVT_BUTTON handler shows a wxMessageBox.  Excel's Win32
    // message pump dispatches WM_COMMAND to the button, which wx translates
    // into a wxCommandEvent that fires the bound lambda.
    // -------------------------------------------------------------------------
    void createWxContent(int w, int h)
    {
        if (!detail::ensureWxInit())
        {
            std::cerr << "[xlCOM]   wx initialisation failed\n";
            return;
        }

        // Wrap the existing HWND as a wx top-level container.
        m_wxContainer = new wxNativeContainerWindow(m_hwnd);
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

        // Equal stretch spacers above and below the button centre it vertically.
        sizer->AddStretchSpacer();
        sizer->Add(button, 0, wxALIGN_CENTER);
        sizer->AddStretchSpacer();

        m_wxPanel->SetSizer(sizer);
        m_wxPanel->Layout();
        std::cerr << "[xlCOM]   wx content created\n";
    }

    // -------------------------------------------------------------------------
    // deactivate
    //
    // Tears down the in-place active state.  Idempotent — safe to call multiple
    // times.
    //
    // wx object abandonment strategy
    // --------------------------------
    // Calling wxWindow::Destroy() or delete on m_wxPanel / m_wxContainer would
    // normally schedule deferred window deletion via a wx idle event.  But
    // without a wx event loop, idle events never process, leaving wx in a
    // partially destructed state.  Moreover, wx's internal OnUpdateUI machinery
    // (DoUpdateWindowUI) walks the window tree during destruction and accesses
    // members that may already be invalid without a running loop.
    //
    // The safe approach is to abandon the wx pointers (set to nullptr without
    // calling any wx teardown API) and let DestroyWindow() below destroy the
    // underlying Win32 HWNDs.  The wx wrapper objects become "orphaned" — they
    // hold a dangling HWND — but because the process exits (or the DLL unloads)
    // shortly after, this is harmless for the current experimental scope.
    //
    // Notification sequence (per OLE spec):
    //   1. Destroy the HWND so the visual is gone before we tell the site.
    //   2. Call OnUIDeactivate(FALSE) — the control is relinquishing UI focus.
    //   3. Call OnInPlaceDeactivate() — the control is no longer in-place active.
    //   4. Release the IOleInPlaceSite pointer (balances the QI AddRef).
    // -------------------------------------------------------------------------
    void deactivate()
    {
        if (!m_active) return;

        std::cerr << "[xlCOM] TaskPaneControl::deactivate\n";

        // Abandon wx object pointers — do NOT call Destroy() or delete
        // on them.  We run without a wx event loop, so wx teardown APIs
        // crash (DoUpdateWindowUI accesses partially-destructed state).
        // DestroyWindow below will destroy the native child windows;
        // the wx wrappers become orphaned but harmless for this experiment.
        m_wxPanel     = nullptr;
        m_wxContainer = nullptr;

        if (m_hwnd)
        {
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
        }

        if (m_inPlaceSite)
        {
            m_inPlaceSite->OnUIDeactivate(FALSE);   // relinquish UI focus
            m_inPlaceSite->OnInPlaceDeactivate();   // fully deactivated
            m_inPlaceSite->Release();
            m_inPlaceSite = nullptr;
        }

        // Release the view advise sink if one was registered.
        if (m_adviseSink) { m_adviseSink->Release(); m_adviseSink = nullptr; }

        m_active = false;
    }
};

// ===========================================================================
// TaskPaneControlFactory
//
// An IClassFactory implementation that creates TaskPaneControl instances.
//
// COM's class-object lookup (CoGetClassObject / DllGetClassObject) always
// returns an IClassFactory.  The factory is a separate object from the control
// itself: one factory instance can create many control instances.
//
// This factory does NOT register itself with the global lock count; it is
// ephemeral and its lifetime is managed by the caller via AddRef/Release.
// ===========================================================================

class TaskPaneControlFactory : public IClassFactory // NOLINT
{
public:
    // Initial ref count of 1: the factory is created with one owned reference
    // (held by DllGetClassObject / taskPaneGetClassObject).
    TaskPaneControlFactory() : m_ref(1) {}

    // Standard IUnknown — only IClassFactory / IUnknown are supported.
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override
    {
        if (!ppv) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_IClassFactory)
        {
            *ppv = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef()  override { return InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() override
    {
        const LONG r = InterlockedDecrement(&m_ref);
        if (r == 0) delete this;
        return r;
    }

    // CreateInstance — the canonical COM factory method.
    //
    // pOuter — non-null means the caller wants COM aggregation.  We do not
    //           support aggregation, so we return CLASS_E_NOAGGREGATION.
    // riid    — the interface the caller wants on the new object (typically
    //           IID_IOleObject or IID_IUnknown).
    // ppv     — receives the interface pointer if successful.
    //
    // Pattern: allocate → QI → Release.  The initial ref count of 1 (from the
    // constructor) keeps the object alive for the QI call.  After QI either
    // succeeds (adding another ref via AddRef) or fails, Release() brings the
    // count back down to 0 if QI failed, deleting the object, or to 1 if QI
    // succeeded, leaving exactly one owned reference held by the caller via *ppv.
    STDMETHODIMP CreateInstance(IUnknown* pOuter, REFIID riid, void** ppv) override
    {
        if (!ppv)    return E_POINTER;
        if (pOuter)  return CLASS_E_NOAGGREGATION;

        auto* pCtrl = new(std::nothrow) TaskPaneControl();
        if (!pCtrl) return E_OUTOFMEMORY;

        const HRESULT hr = pCtrl->QueryInterface(riid, ppv);
        pCtrl->Release(); // balance constructor ref; object lives if QI AddRef'd it
        return hr;
    }

    // LockServer — increments or decrements the DLL-wide server lock count.
    // While the lock count is > 0, DllCanUnloadNow() returns S_FALSE, preventing
    // the DLL from being unloaded even if no objects are currently alive.
    // Containers call LockServer(TRUE) before creating objects to ensure the
    // DLL stays loaded during a batch of CreateInstance calls.
    STDMETHODIMP LockServer(BOOL fLock) override
    {
        if (fLock) InterlockedIncrement(&g_lockCount);
        else       InterlockedDecrement(&g_lockCount);
        return S_OK;
    }

private:
    LONG m_ref;
};

// ===========================================================================
// detail — registration hooks
//
// COMServer.hpp defines three function-pointer slots that any header can
// populate to extend the server's DLL entry points:
//
//   g_pfnExtraGetClassObject — called from DllGetClassObject for CLSIDs that
//                              COMServer.hpp does not itself handle.
//   g_pfnExtraRegister       — called from DllRegisterServer to write registry
//                              entries for additional classes.
//   g_pfnExtraUnregister     — called from DllUnregisterServer to remove those
//                              entries.
//
// The static-initialisation trick (s_taskPaneHooked) installs the hooks at
// program startup — before any DLL entry point is called — so that by the time
// DllGetClassObject is first invoked, the hook is already in place.
// ===========================================================================

namespace detail {

// ---------------------------------------------------------------------------
// taskPaneGetClassObject
//
// Called from the DLL's DllGetClassObject export (via g_pfnExtraGetClassObject)
// when the CLSID matches CLSID_TaskPaneControl.
//
// Allocates a new TaskPaneControlFactory, QI's it for the requested interface
// (usually IID_IClassFactory), releases the factory's own ref, and returns the
// QI result.  If the CLSID does not match, returns CLASS_E_CLASSNOTAVAILABLE
// so the dispatcher tries other registered factories.
// ---------------------------------------------------------------------------
inline HRESULT taskPaneGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    if (rclsid == CLSID_TaskPaneControl)
    {
        auto* pFactory = new(std::nothrow) TaskPaneControlFactory();
        if (!pFactory) return E_OUTOFMEMORY;
        const HRESULT hr = pFactory->QueryInterface(riid, ppv);
        pFactory->Release();
        return hr;
    }
    return CLASS_E_CLASSNOTAVAILABLE;
}

// ---------------------------------------------------------------------------
// taskPaneRegister
//
// Called from DllRegisterServer to write all registry entries required for
// COM and ActiveX recognition.  dllPath is the full path to this DLL as
// reported by GetModuleFileNameW.
//
// Registry keys written:
//
//   HKCR\CLSID\{CLSID}
//       (default) = "xlCOM TaskPaneControl"   — human-readable name
//
//   HKCR\CLSID\{CLSID}\InprocServer32
//       (default)       = <dllPath>            — path to this DLL
//       ThreadingModel  = "Apartment"          — STA (must match Excel's STA)
//
//   HKCR\CLSID\{CLSID}\Control
//       (empty key)                             — marks as ActiveX control;
//                                                 containers such as VBA scan
//                                                 for this key to distinguish
//                                                 controls from plain objects.
//
//   HKCR\CLSID\{CLSID}\Implemented Categories\{40FC6ED4-...}
//       (empty key)                             — CATID_Control category;
//                                                 used by OLE component category
//                                                 managers and Insert Object dialogs.
//
//   HKCR\CLSID\{CLSID}\ProgID
//       (default) = "xlCOM.TaskPaneCtrl"        — forward ProgID lookup
//
//   HKCR\xlCOM.TaskPaneCtrl
//       (default) = "xlCOM TaskPaneControl"     — reverse ProgID entry (display name)
//
//   HKCR\xlCOM.TaskPaneCtrl\CLSID
//       (default) = "{CLSID}"                   — ProgID → CLSID mapping used by
//                                                 CLSIDFromProgID().
// ---------------------------------------------------------------------------
inline HRESULT taskPaneRegister(const wchar_t* dllPath)
{
    wchar_t clsidStr[64] = {};
    StringFromGUID2(CLSID_TaskPaneControl, clsidStr,
                    static_cast<int>(_countof(clsidStr)));

    wchar_t key[512] = {};

    // HKCR\CLSID\{...}
    swprintf_s(key, L"CLSID\\%s", clsidStr);
    HRESULT hr = SetRegString(HKEY_CLASSES_ROOT, key, nullptr,
                              L"xlCOM TaskPaneControl");
    if (FAILED(hr)) return hr;

    // InprocServer32
    swprintf_s(key, L"CLSID\\%s\\InprocServer32", clsidStr);
    hr = SetRegString(HKEY_CLASSES_ROOT, key, nullptr, dllPath);
    if (FAILED(hr)) return hr;
    hr = SetRegString(HKEY_CLASSES_ROOT, key, L"ThreadingModel", L"Apartment");
    if (FAILED(hr)) return hr;

    // Control (empty key — marks this as an ActiveX control)
    swprintf_s(key, L"CLSID\\%s\\Control", clsidStr);
    hr = SetRegString(HKEY_CLASSES_ROOT, key, nullptr, L"");
    if (FAILED(hr)) return hr;

    // Implemented Categories → CATID_Control
    // {40FC6ED4-2438-11CF-A3DB-080036F12502} is the well-known CATID for
    // "OLE Control" as defined in objbase.h / comcat.h.
    swprintf_s(key,
        L"CLSID\\%s\\Implemented Categories\\"
        L"{40FC6ED4-2438-11CF-A3DB-080036F12502}", clsidStr);
    hr = SetRegString(HKEY_CLASSES_ROOT, key, nullptr, L"");
    if (FAILED(hr)) return hr;

    // ProgID
    swprintf_s(key, L"CLSID\\%s\\ProgID", clsidStr);
    hr = SetRegString(HKEY_CLASSES_ROOT, key, nullptr, kProgID_TaskPane);
    if (FAILED(hr)) return hr;

    // Reverse ProgID → CLSID
    hr = SetRegString(HKEY_CLASSES_ROOT, kProgID_TaskPane, nullptr,
                      L"xlCOM TaskPaneControl");
    if (FAILED(hr)) return hr;

    wchar_t progKey[512] = {};
    swprintf_s(progKey, L"%s\\CLSID", kProgID_TaskPane);
    hr = SetRegString(HKEY_CLASSES_ROOT, progKey, nullptr, clsidStr);
    if (FAILED(hr)) return hr;

    return S_OK;
}

// ---------------------------------------------------------------------------
// taskPaneUnregister
//
// Called from DllUnregisterServer to remove all registry entries written by
// taskPaneRegister().  Deletes the CLSID key tree and the ProgID key tree.
// DeleteRegKey (from COMServer.hpp) performs a recursive delete so all sub-
// keys are removed even if they were not originally written by this function.
// ---------------------------------------------------------------------------
inline void taskPaneUnregister()
{
    wchar_t clsidStr[64] = {};
    StringFromGUID2(CLSID_TaskPaneControl, clsidStr,
                    static_cast<int>(_countof(clsidStr)));

    wchar_t key[512] = {};
    swprintf_s(key, L"CLSID\\%s", clsidStr);
    DeleteRegKey(HKEY_CLASSES_ROOT, key);           // removes entire CLSID subtree
    DeleteRegKey(HKEY_CLASSES_ROOT, kProgID_TaskPane); // removes ProgID subtree
}

// ---------------------------------------------------------------------------
// s_taskPaneHooked — static-initialisation hook
//
// This inline variable is initialised by a lambda that runs at DLL load time
// (before any exported function is called) and installs the three function
// pointers into the global slots defined by COMServer.hpp.
//
// Because the variable is inline, the ODR guarantees exactly one copy of this
// initialisation across all translation units that include this header.
//
// After this runs:
//   DllGetClassObject  → routes TaskPaneControl CLSIDs to taskPaneGetClassObject
//   DllRegisterServer  → writes TaskPaneControl registry entries
//   DllUnregisterServer → removes TaskPaneControl registry entries
// ---------------------------------------------------------------------------
inline const bool s_taskPaneHooked = [] {
    g_pfnExtraGetClassObject = &taskPaneGetClassObject;
    g_pfnExtraRegister       = &taskPaneRegister;
    g_pfnExtraUnregister     = &taskPaneUnregister;
    return true;
}();

} // namespace detail

