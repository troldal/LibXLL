// ---------------------------------------------------------------------------
// TaskPaneControl.hpp — minimal ActiveX control for Excel custom task panes.
//
// Implements the COM interfaces required for in-place activation inside an
// Office task pane.  The visual content is a wxWidgets panel with a label.
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
// {7C2D4F8A-3E1B-4A5C-9D6E-8F0B2A1C3D5E}
// ---------------------------------------------------------------------------

inline constexpr CLSID CLSID_TaskPaneControl =
    {0x7C2D4F8A, 0x3E1B, 0x4A5C, {0x9D, 0x6E, 0x8F, 0x0B, 0x2A, 0x1C, 0x3D, 0x5E}};

inline constexpr wchar_t kProgID_TaskPane[] = L"xlCOM.TaskPaneCtrl";

// ---------------------------------------------------------------------------
// wxWidgets one-shot initialisation (runs on Excel's STA thread).
// No wx event loop — Excel's message pump drives the windows.
// ---------------------------------------------------------------------------

namespace detail {

inline bool& wxReadyFlag()
{
    static bool ready = false;
    return ready;
}

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

inline void shutdownWx()
{
    if (!wxReadyFlag()) return;
    if (wxTheApp) wxTheApp->OnExit();
    wxEntryCleanup();
    wxReadyFlag() = false;
}

// Win32 window class for the control's child HWND.
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
        wc.hbrBackground  = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.style          = CS_HREDRAW | CS_VREDRAW;
        RegisterClassExW(&wc);
        registered = true;
    }
    return name;
}

} // namespace detail

// ===========================================================================
// TaskPaneControl
// ===========================================================================

class TaskPaneControl
    : public IOleObject
    , public IOleInPlaceObject
    , public IOleInPlaceActiveObject
    , public IViewObject
    , public IPersistStreamInit
    , public IOleControl
{
public:
    TaskPaneControl() : m_ref(1) { InterlockedIncrement(&g_lockCount); }

    ~TaskPaneControl()
    {
        deactivate();
        if (m_clientSite) { m_clientSite->Release(); m_clientSite = nullptr; }
    }

    // === IUnknown =========================================================

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

    STDMETHODIMP_(ULONG) AddRef()  override { return InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() override
    {
        const LONG r = InterlockedDecrement(&m_ref);
        if (r == 0) { InterlockedDecrement(&g_lockCount); delete this; }
        return r;
    }

    // === IOleObject =======================================================

    STDMETHODIMP SetClientSite(IOleClientSite* pSite) override
    {
        if (m_clientSite) m_clientSite->Release();
        m_clientSite = pSite;
        if (m_clientSite) m_clientSite->AddRef();
        return S_OK;
    }
    STDMETHODIMP GetClientSite(IOleClientSite** ppSite) override
    {
        if (!ppSite) return E_POINTER;
        *ppSite = m_clientSite;
        if (m_clientSite) m_clientSite->AddRef();
        return S_OK;
    }

    STDMETHODIMP SetHostNames(LPCOLESTR, LPCOLESTR) override { return S_OK; }

    STDMETHODIMP Close(DWORD /*dwSaveOption*/) override
    {
        deactivate();
        return S_OK;
    }

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

    STDMETHODIMP SetExtent(DWORD /*dwAspect*/, SIZEL* pSizel) override
    {
        if (!pSizel) return E_POINTER;
        m_extent = *pSizel;
        return S_OK;
    }
    STDMETHODIMP GetExtent(DWORD /*dwAspect*/, SIZEL* pSizel) override
    {
        if (!pSizel) return E_POINTER;
        *pSizel = m_extent;
        return S_OK;
    }

    STDMETHODIMP GetUserClassID(CLSID* pClsid) override
    {
        if (!pClsid) return E_POINTER;
        *pClsid = CLSID_TaskPaneControl;
        return S_OK;
    }

    STDMETHODIMP GetMiscStatus(DWORD /*dwAspect*/, DWORD* pdwStatus) override
    {
        if (!pdwStatus) return E_POINTER;
        *pdwStatus = OLEMISC_RECOMPOSEONRESIZE
                   | OLEMISC_INSIDEOUT
                   | OLEMISC_ACTIVATEWHENVISIBLE
                   | OLEMISC_SETCLIENTSITEFIRST;
        return S_OK;
    }

    // Stubs — not needed for a minimal task-pane control.
    STDMETHODIMP SetMoniker(DWORD, IMoniker*)          override { return E_NOTIMPL; }
    STDMETHODIMP GetMoniker(DWORD, DWORD, IMoniker**)  override { return E_NOTIMPL; }
    STDMETHODIMP InitFromData(IDataObject*, BOOL, DWORD)        override { return E_NOTIMPL; }
    STDMETHODIMP GetClipboardData(DWORD, IDataObject**) override { return E_NOTIMPL; }
    STDMETHODIMP EnumVerbs(IEnumOLEVERB**)              override { return E_NOTIMPL; }
    STDMETHODIMP Update()                               override { return S_OK; }
    STDMETHODIMP IsUpToDate()                           override { return S_OK; }
    STDMETHODIMP GetUserType(DWORD, LPOLESTR*)          override { return E_NOTIMPL; }
    STDMETHODIMP Advise(IAdviseSink*, DWORD*)           override { return E_NOTIMPL; }
    STDMETHODIMP Unadvise(DWORD)                        override { return E_NOTIMPL; }
    STDMETHODIMP EnumAdvise(IEnumSTATDATA**)            override { return E_NOTIMPL; }
    STDMETHODIMP SetColorScheme(LOGPALETTE*)            override { return E_NOTIMPL; }

    // === IOleWindow / IOleInPlaceObject ====================================

    STDMETHODIMP GetWindow(HWND* phwnd) override
    {
        if (!phwnd) return E_POINTER;
        *phwnd = m_hwnd;
        return m_hwnd ? S_OK : E_FAIL;
    }

    STDMETHODIMP ContextSensitiveHelp(BOOL) override { return E_NOTIMPL; }

    STDMETHODIMP InPlaceDeactivate() override
    {
        deactivate();
        return S_OK;
    }

    STDMETHODIMP UIDeactivate() override { return S_OK; }

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

    STDMETHODIMP ReactivateAndUndo() override { return E_NOTIMPL; }

    // === IOleInPlaceActiveObject ==========================================

    STDMETHODIMP TranslateAccelerator(LPMSG) override { return S_FALSE; }
    STDMETHODIMP OnFrameWindowActivate(BOOL)  override { return S_OK; }
    STDMETHODIMP OnDocWindowActivate(BOOL)    override { return S_OK; }
    STDMETHODIMP ResizeBorder(LPCRECT, IOleInPlaceUIWindow*, BOOL) override
    { return S_OK; }
    STDMETHODIMP EnableModeless(BOOL) override { return S_OK; }

    // === IViewObject ======================================================

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

    STDMETHODIMP GetColorSet(DWORD, LONG, void*, DVTARGETDEVICE*,
                              HDC, LOGPALETTE**) override
    { return E_NOTIMPL; }
    STDMETHODIMP Freeze(DWORD, LONG, void*, DWORD*) override { return E_NOTIMPL; }
    STDMETHODIMP Unfreeze(DWORD)                     override { return E_NOTIMPL; }

    STDMETHODIMP SetAdvise(DWORD, DWORD, IAdviseSink* pSink) override
    {
        if (m_adviseSink) m_adviseSink->Release();
        m_adviseSink = pSink;
        if (m_adviseSink) m_adviseSink->AddRef();
        return S_OK;
    }

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

    // === IPersist / IPersistStreamInit =====================================

    STDMETHODIMP GetClassID(CLSID* pClsid) override
    {
        if (!pClsid) return E_POINTER;
        *pClsid = CLSID_TaskPaneControl;
        return S_OK;
    }

    STDMETHODIMP IsDirty()                           override { return S_FALSE; }
    STDMETHODIMP Load(LPSTREAM)                      override { return S_OK; }
    STDMETHODIMP Save(LPSTREAM, BOOL)                override { return S_OK; }
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER* pcbSize)  override
    {
        if (pcbSize) pcbSize->QuadPart = 0;
        return S_OK;
    }
    STDMETHODIMP InitNew() override { return S_OK; }

    // === IOleControl ======================================================

    STDMETHODIMP GetControlInfo(CONTROLINFO* pCI) override
    {
        if (!pCI) return E_POINTER;
        pCI->cb      = sizeof(CONTROLINFO);
        pCI->hAccel  = nullptr;
        pCI->cAccel  = 0;
        pCI->dwFlags = 0;
        return S_OK;
    }

    STDMETHODIMP OnMnemonic(MSG*)                override { return S_OK; }
    STDMETHODIMP OnAmbientPropertyChange(DISPID) override { return S_OK; }
    STDMETHODIMP FreezeEvents(BOOL)              override { return S_OK; }

    // === private ==========================================================

private:
    LONG              m_ref;
    IOleClientSite*   m_clientSite  = nullptr;
    IOleInPlaceSite*  m_inPlaceSite = nullptr;
    IAdviseSink*      m_adviseSink  = nullptr;
    HWND              m_hwnd        = nullptr;
    bool              m_active      = false;
    SIZEL             m_extent      = { 5000, 5000 }; // HIMETRIC

    wxNativeContainerWindow* m_wxContainer = nullptr;
    wxPanel*                 m_wxPanel     = nullptr;

    // -----------------------------------------------------------------------
    // In-place activation — called from DoVerb.
    // -----------------------------------------------------------------------

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

        if (m_inPlaceSite->CanInPlaceActivate() != S_OK)
        {
            std::cerr << "[xlCOM]   CanInPlaceActivate returned failure\n";
            return E_FAIL;
        }
        m_inPlaceSite->OnInPlaceActivate();

        // Retrieve parent window and position from the site.
        HWND                 siteHwnd  = nullptr;
        IOleInPlaceFrame*    pFrame    = nullptr;
        IOleInPlaceUIWindow* pUIWin    = nullptr;
        RECT                 posRect2  = {};
        RECT                 clipRect  = {};
        OLEINPLACEFRAMEINFO  frameInfo = { sizeof(frameInfo) };

        if (posRect)
            posRect2 = *posRect;

        m_inPlaceSite->GetWindow(&siteHwnd);
        m_inPlaceSite->GetWindowContext(&pFrame, &pUIWin,
                                         &posRect2, &clipRect, &frameInfo);
        if (pFrame) pFrame->Release();
        if (pUIWin) pUIWin->Release();

        HWND parentHwnd = siteHwnd ? siteHwnd : hwndParent;
        std::cerr << "[xlCOM]   parentHwnd=" << static_cast<void*>(parentHwnd)
                  << " rect=(" << posRect2.left << ',' << posRect2.top
                  << ',' << posRect2.right << ',' << posRect2.bottom << ")\n";

        if (!parentHwnd) return E_FAIL;

        // Create the child HWND.  Use at least 1×1 if the rect is empty.
        int w = posRect2.right  - posRect2.left;
        int h = posRect2.bottom - posRect2.top;
        if (w <= 0) w = 1;
        if (h <= 0) h = 1;

        m_hwnd = CreateWindowExW(
            0, detail::controlWindowClass(), L"",
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

        // Only UI-activate when explicitly asked.
        if (uiActivate)
            m_inPlaceSite->OnUIActivate();

        // Build the wxWidgets content.
        createWxContent(w, h);

        ShowWindow(m_hwnd, SW_SHOW);
        std::cerr << "[xlCOM]   TaskPaneControl activated OK\n";
        return S_OK;
    }

    // -----------------------------------------------------------------------
    void createWxContent(int w, int h)
    {
        if (!detail::ensureWxInit())
        {
            std::cerr << "[xlCOM]   wx initialisation failed\n";
            return;
        }

        m_wxContainer = new wxNativeContainerWindow(m_hwnd);
        m_wxPanel = new wxPanel(m_wxContainer, wxID_ANY,
                                wxDefaultPosition, wxSize(w, h),
                                wxNO_BORDER);

        auto* sizer  = new wxBoxSizer(wxVERTICAL);
        auto* button = new wxButton(m_wxPanel, wxID_ANY, "Click me!");

        button->Bind(wxEVT_BUTTON, [](wxCommandEvent&)
        {
            wxMessageBox("Hello from the Task Pane!", "Task Pane",
                         wxOK | wxICON_INFORMATION);
        });

        sizer->AddStretchSpacer();
        sizer->Add(button, 0, wxALIGN_CENTER);
        sizer->AddStretchSpacer();

        m_wxPanel->SetSizer(sizer);
        m_wxPanel->Layout();
        std::cerr << "[xlCOM]   wx content created\n";
    }

    // -----------------------------------------------------------------------
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
            m_inPlaceSite->OnUIDeactivate(FALSE);
            m_inPlaceSite->OnInPlaceDeactivate();
            m_inPlaceSite->Release();
            m_inPlaceSite = nullptr;
        }

        if (m_adviseSink) { m_adviseSink->Release(); m_adviseSink = nullptr; }

        m_active = false;
    }
};

// ===========================================================================
// IClassFactory for TaskPaneControl
// ===========================================================================

class TaskPaneControlFactory : public IClassFactory // NOLINT
{
public:
    TaskPaneControlFactory() : m_ref(1) {}

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

    STDMETHODIMP CreateInstance(IUnknown* pOuter, REFIID riid, void** ppv) override
    {
        if (!ppv)    return E_POINTER;
        if (pOuter)  return CLASS_E_NOAGGREGATION;

        auto* pCtrl = new(std::nothrow) TaskPaneControl();
        if (!pCtrl) return E_OUTOFMEMORY;

        const HRESULT hr = pCtrl->QueryInterface(riid, ppv);
        pCtrl->Release();
        return hr;
    }

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
// Self-registration — hooks into COMServer.hpp extension points.
// ===========================================================================

namespace detail {

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

inline void taskPaneUnregister()
{
    wchar_t clsidStr[64] = {};
    StringFromGUID2(CLSID_TaskPaneControl, clsidStr,
                    static_cast<int>(_countof(clsidStr)));

    wchar_t key[512] = {};
    swprintf_s(key, L"CLSID\\%s", clsidStr);
    DeleteRegKey(HKEY_CLASSES_ROOT, key);
    DeleteRegKey(HKEY_CLASSES_ROOT, kProgID_TaskPane);
}

// Static-init registration block.
inline const bool s_taskPaneHooked = [] {
    g_pfnExtraGetClassObject = &taskPaneGetClassObject;
    g_pfnExtraRegister       = &taskPaneRegister;
    g_pfnExtraUnregister     = &taskPaneUnregister;
    return true;
}();

} // namespace detail








