// ---------------------------------------------------------------------------
// Content type selection — uncomment exactly ONE content include/block below.
// ---------------------------------------------------------------------------

// #include "ActiveX/WxTaskPane.hpp"          // wxWidgets content
// #include "ActiveX/Win32TaskPane.hpp"       // pure Win32 content
// #include "ActiveX/QtTaskPane.hpp"          // Qt Widgets content
// #include "ActiveX/FltkTaskPane.hpp"        // FLTK content
// #include "ActiveX/QtQuickTaskPane.hpp"     // Qt Quick (QML) content
#include "ActiveX/ImGuiTaskPane.hpp"          // Dear ImGui — Win32 + DirectX 11

#include "COM/Macros.hpp"
#include "ActiveX/TaskPaneControl.hpp"
#include "Utils/ImageFromPNGBytes.hpp"
#include "Utils/IsDarkMode.hpp"
#include <cmrc/cmrc.hpp>
#include <iostream>

CMRC_DECLARE(foo);

// ---------------------------------------------------------------------------
// Explicit instantiation of the TaskPaneControl with the chosen content type.
// This causes the compiler to emit the full COM class and factory in this TU.
// ---------------------------------------------------------------------------

// template class TaskPaneControl<WxTaskPane>;
// template class TaskPaneControlFactory<WxTaskPane>;
// template class TaskPaneControl<Win32TaskPane>;
// template class TaskPaneControlFactory<Win32TaskPane>;
// template class TaskPaneControl<QtTaskPane>;
// template class TaskPaneControlFactory<QtTaskPane>;
// template class TaskPaneControl<FltkTaskPane>;
// template class TaskPaneControlFactory<FltkTaskPane>;
// template class TaskPaneControl<QtQuickTaskPane>;
// template class TaskPaneControlFactory<QtQuickTaskPane>;
template class TaskPaneControl<ImGuiTaskPane>;
template class TaskPaneControlFactory<ImGuiTaskPane>;

// ---------------------------------------------------------------------------
// Install COM server hooks so that DllGetClassObject, DllRegisterServer, and
// DllUnregisterServer know about the TaskPaneControl ActiveX class.
// Runs at DLL load time (static initialisation).
// ---------------------------------------------------------------------------

// static const bool s_taskPaneHooked =
//     detail::registerTaskPaneHooks<WxTaskPane>();
// static const bool s_taskPaneHooked =
//     detail::registerTaskPaneHooks<Win32TaskPane>();
// static const bool s_taskPaneHooked =
//     detail::registerTaskPaneHooks<QtTaskPane>();
// static const bool s_taskPaneHooked =
//     detail::registerTaskPaneHooks<FltkTaskPane>();
// static const bool s_taskPaneHooked =
//     detail::registerTaskPaneHooks<QtQuickTaskPane>();
static const bool s_taskPaneHooked =
    detail::registerTaskPaneHooks<ImGuiTaskPane>();

// ---------------------------------------------------------------------------
// Excel Application pointer — captured on connection, released on disconnect.
// Used to call Application.Run("MacroName") from ribbon callbacks.
// ---------------------------------------------------------------------------

static IDispatch* g_excelApp = nullptr;

// ---------------------------------------------------------------------------
// ICTPFactory pointer — captured via ICustomTaskPaneConsumer::CTPFactoryAvailable.
// Used to call ICTPFactory::CreateCTP from ribbon callbacks.
// ---------------------------------------------------------------------------

static IDispatch* g_ctpFactory = nullptr;

// ---------------------------------------------------------------------------
// CustomTaskPane IDispatch — created on first button click, toggled thereafter.
// ---------------------------------------------------------------------------

static IDispatch* g_taskPane = nullptr;

// ---------------------------------------------------------------------------
// OnConnection — fires when Excel loads and connects the add-in.
// ---------------------------------------------------------------------------

auto onConnection = com::OnConnection(
    [](IDispatch* application, ext_ConnectMode connectMode,
       IDispatch* /*addInInst*/, SAFEARRAY** /*custom*/)
    {
        std::cerr << "[xlCOM] OnConnection fired. "
                     "ConnectMode = " << static_cast<int>(connectMode) << '\n';

        if (application)
        {
            application->AddRef();
            g_excelApp = application;
        }
    });
XLL_COM_REGISTER(onConnection);

// ---------------------------------------------------------------------------
// OnDisconnection — fires when Excel unloads the add-in.
// ---------------------------------------------------------------------------

auto onDisconnection = com::OnDisconnection(
    [](ext_DisconnectMode disconnectMode, SAFEARRAY** /*custom*/)
    {
        std::cerr << "[xlCOM] OnDisconnection fired. "
                     "DisconnectMode = " << static_cast<int>(disconnectMode) << '\n';

        if (g_excelApp)
        {
            g_excelApp->Release();
            g_excelApp = nullptr;
        }

        if (g_ctpFactory)
        {
            g_ctpFactory->Release();
            g_ctpFactory = nullptr;
        }

        if (g_taskPane)
        {
            g_taskPane->Release();
            g_taskPane = nullptr;
        }

        // Clean up GUI framework runtime on disconnect.
        // WxTaskPane::shutdown();
        // QtTaskPane::shutdown();
        // FltkTaskPane::shutdown();
        // QtQuickTaskPane::shutdown();
        ImGuiTaskPane::shutdown();
    });
XLL_COM_REGISTER(onDisconnection);

// ---------------------------------------------------------------------------
// OnCTPFactoryAvailable — fired by ICustomTaskPaneConsumer::CTPFactoryAvailable.
// Stores the factory for use in ribbon callbacks.
// ---------------------------------------------------------------------------

auto onCTPFactoryAvailable = com::OnCTPFactoryAvailable(
    [](IDispatch* factory)
    {
        if (factory)
        {
            factory->AddRef();
            g_ctpFactory = factory;
        }
    });
XLL_COM_REGISTER(onCTPFactoryAvailable);

// ---------------------------------------------------------------------------
// OnGetCustomUI — returns the RibbonX XML from the embedded resource.
// ---------------------------------------------------------------------------

auto onGetCustomUI = com::OnGetCustomUI(
    [](const com::String& /*ribbonId*/) -> com::String
    {
        auto fs   = cmrc::foo::get_filesystem();
        auto file = fs.open("ribbon.xml");
        return com::String(std::string(file.begin(), file.end()));
    });
XLL_COM_REGISTER(onGetCustomUI);

// ---------------------------------------------------------------------------
// IDispatch callbacks — registered by name via DispatchCallback<"Name">.
// Connect::GetIDsOfNames and Connect::Invoke resolve these automatically
// through the DispatchRegistry.
// ---------------------------------------------------------------------------

auto onButtonClicked = com::DispatchCallback<"OnButtonClicked">(
    [](DISPPARAMS*, VARIANT*) -> HRESULT
    {
        if (!g_excelApp) return E_FAIL;

        // Resolve "Run" on the Excel Application object.
        LPOLESTR methodName = const_cast<LPOLESTR>(L"Run");
        DISPID   dispId     = 0;
        HRESULT  hr = g_excelApp->GetIDsOfNames(IID_NULL, &methodName, 1,
                                                  LOCALE_USER_DEFAULT, &dispId);
        if (FAILED(hr)) return hr;

        // Build the argument: the XLL command name to execute.
        com::String macroName(L"IM.STATUS");
        VARIANT arg  = {};
        arg.vt       = VT_BSTR;
        arg.bstrVal  = macroName.get();   // non-owning — com::String still owns it

        DISPPARAMS params = {};
        params.rgvarg      = &arg;
        params.cArgs       = 1;

        // Application.Run("WX.MODAL.GREETING")
        return g_excelApp->Invoke(dispId, IID_NULL, LOCALE_USER_DEFAULT,
                                   DISPATCH_METHOD, &params, nullptr, nullptr, nullptr);
    });
XLL_COM_REGISTER(onButtonClicked);

auto onRibbonLoad = com::DispatchCallback<"OnRibbonLoad">(
    [](DISPPARAMS* pDispParams, VARIANT*) -> HRESULT
    {
        // Office passes the IRibbonUI pointer as the first (and only) argument.
        if (pDispParams && pDispParams->cArgs >= 1 &&
            pDispParams->rgvarg[0].vt == VT_DISPATCH &&
            pDispParams->rgvarg[0].pdispVal)
        {
            std::cerr << "[xlCOM] OnRibbonLoad fired — IRibbonUI captured.\n";
        }
        return S_OK;
    });
XLL_COM_REGISTER(onRibbonLoad);

auto getButtonImage = com::DispatchCallback<"GetButtonImage">(
    [](DISPPARAMS*, VARIANT* pVarResult) -> HRESULT
    {
        if (!pVarResult) return E_POINTER;

        const char* resource = isDarkMode()
            ? "button-help_dark48px.png"
            : "button-help_light48px.png";

        auto fs   = cmrc::foo::get_filesystem();
        auto file = fs.open(resource);

        IPictureDisp* pPicture = ImageFromPNGBytes(file);
        if (!pPicture) return E_FAIL;

        VariantInit(pVarResult);
        pVarResult->vt       = VT_DISPATCH;
        pVarResult->pdispVal = pPicture;   // caller releases via VARIANT clear
        return S_OK;
    });
XLL_COM_REGISTER(getButtonImage);

// ---------------------------------------------------------------------------
// Helpers: interact with a CustomTaskPane IDispatch.
// ---------------------------------------------------------------------------

static HRESULT TaskPane_GetVisible(IDispatch* pPane, bool& visible)
{
    LPOLESTR  name = const_cast<LPOLESTR>(L"Visible");
    DISPID    id   = 0;
    HRESULT   hr   = pPane->GetIDsOfNames(IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &id);
    if (FAILED(hr)) return hr;

    DISPPARAMS noParams = {};
    VARIANT    result   = {};
    hr = pPane->Invoke(id, IID_NULL, LOCALE_USER_DEFAULT,
                       DISPATCH_PROPERTYGET, &noParams, &result, nullptr, nullptr);
    if (SUCCEEDED(hr) && result.vt == VT_BOOL)
        visible = (result.boolVal != VARIANT_FALSE);
    VariantClear(&result);
    return hr;
}

static HRESULT TaskPane_SetVisible(IDispatch* pPane, bool visible)
{
    LPOLESTR  name = const_cast<LPOLESTR>(L"Visible");
    DISPID    id   = 0;
    HRESULT   hr   = pPane->GetIDsOfNames(IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &id);
    if (FAILED(hr)) return hr;

    VARIANT    arg     = {};
    arg.vt             = VT_BOOL;
    arg.boolVal        = visible ? VARIANT_TRUE : VARIANT_FALSE;
    DISPID     putId   = DISPID_PROPERTYPUT;
    DISPPARAMS params  = {};
    params.rgvarg            = &arg;
    params.cArgs             = 1;
    params.rgdispidNamedArgs = &putId;
    params.cNamedArgs        = 1;
    return pPane->Invoke(id, IID_NULL, LOCALE_USER_DEFAULT,
                         DISPATCH_PROPERTYPUT, &params, nullptr, nullptr, nullptr);
}

// Delete() removes the CTP from Excel's CustomTaskPanes collection, which
// triggers IOleObject::Close → TaskPaneControl::deactivate → ~ImGuiTaskPane.
static HRESULT TaskPane_Delete(IDispatch* pPane)
{
    LPOLESTR name = const_cast<LPOLESTR>(L"Delete");
    DISPID   id   = 0;
    HRESULT  hr   = pPane->GetIDsOfNames(IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &id);
    if (FAILED(hr)) return hr;
    DISPPARAMS noParams = {};
    return pPane->Invoke(id, IID_NULL, LOCALE_USER_DEFAULT,
                         DISPATCH_METHOD, &noParams, nullptr, nullptr, nullptr);
}

static HRESULT TaskPane_SetWidth(IDispatch* pPane, int widthPx)
{
    LPOLESTR  name = const_cast<LPOLESTR>(L"Width");
    DISPID    id   = 0;
    HRESULT   hr   = pPane->GetIDsOfNames(IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &id);
    if (FAILED(hr)) return hr;

    VARIANT    arg    = {};
    arg.vt            = VT_I4;
    arg.lVal          = widthPx;
    DISPID     putId  = DISPID_PROPERTYPUT;
    DISPPARAMS params = {};
    params.rgvarg            = &arg;
    params.cArgs             = 1;
    params.rgdispidNamedArgs = &putId;
    params.cNamedArgs        = 1;
    return pPane->Invoke(id, IID_NULL, LOCALE_USER_DEFAULT,
                         DISPATCH_PROPERTYPUT, &params, nullptr, nullptr, nullptr);
}

// ---------------------------------------------------------------------------
// OnTaskPaneClicked — destroy-and-recreate toggle.
//
// Strategy: Excel's CTP Visible property does not reliably re-drive
// DoVerb/activation on the hosted ActiveX control after the first hide.
// Instead we delete the entire CTP on close (which tears down the
// ImGuiTaskPane cleanly) and create a fresh one on open.
//
//   Ribbon press while pane is VISIBLE  → Delete() + release → pane gone.
//   Ribbon press while pane is HIDDEN   → stale pointer; drop, recreate.
//   Ribbon press while no pane          → create fresh pane.
//   User closes with pane X button      → same as "pane hidden" on next press.
// ---------------------------------------------------------------------------

auto onTaskPaneClicked = com::DispatchCallback<"OnTaskPaneClicked">(
    [](DISPPARAMS*, VARIANT*) -> HRESULT
    {
        std::cerr << "[xlCOM] OnTaskPaneClicked\n";

        if (g_taskPane)
        {
            bool visible = false;
            HRESULT hr = TaskPane_GetVisible(g_taskPane, visible);
            if (SUCCEEDED(hr) && visible)
            {
                // Pane is open — close it by deleting the CTP entirely.
                // This triggers TaskPaneControl::deactivate → ~ImGuiTaskPane.
                std::cerr << "[xlCOM]   deleting visible pane\n";
                TaskPane_Delete(g_taskPane);
                g_taskPane->Release();
                g_taskPane = nullptr;
                return S_OK;
            }
            // Pane is already hidden (user closed with X) or pointer is stale.
            // Drop it and fall through to recreate.
            std::cerr << "[xlCOM]   pane hidden/stale — recreating\n";
            g_taskPane->Release();
            g_taskPane = nullptr;
        }

        if (!g_ctpFactory)
        {
            std::cerr << "[xlCOM]   no CTP factory available\n";
            return E_FAIL;
        }

        // Resolve "CreateCTP" on the ICTPFactory dispatch interface.
        LPOLESTR methodName = const_cast<LPOLESTR>(L"CreateCTP");
        DISPID   dispId     = 0;
        HRESULT  hr = g_ctpFactory->GetIDsOfNames(IID_NULL, &methodName, 1,
                                                    LOCALE_USER_DEFAULT, &dispId);
        if (FAILED(hr)) return hr;

        // Arguments are passed in reverse order per IDispatch convention.
        // CreateCTP(CTPAxID As String, CTPTitle As String,
        //           [CTPParentWindow As Object]) As CustomTaskPane
        VARIANT args[3] = {};
        args[2].vt      = VT_BSTR;                          // CTPAxID  (1st param → last index)
        args[2].bstrVal = SysAllocString(kProgID_TaskPane);
        args[1].vt      = VT_BSTR;                          // CTPTitle (2nd param)
        args[1].bstrVal = SysAllocString(L"My Task Pane");
        args[0].vt      = VT_ERROR;                          // CTPParentWindow (optional)
        args[0].scode   = DISP_E_PARAMNOTFOUND;

        DISPPARAMS params  = {};
        params.rgvarg      = args;
        params.cArgs       = 3;

        VARIANT   result   = {};
        EXCEPINFO excep    = {};
        UINT      argErr   = 0;
        hr = g_ctpFactory->Invoke(dispId, IID_NULL, LOCALE_USER_DEFAULT,
                                   DISPATCH_METHOD, &params, &result, &excep, &argErr);

        if (hr == DISP_E_EXCEPTION)
        {
            if (excep.bstrDescription)
                fprintf(stderr, "[xlCOM] CreateCTP failed: %ls\n", excep.bstrDescription);
            SysFreeString(excep.bstrDescription);
            SysFreeString(excep.bstrSource);
            SysFreeString(excep.bstrHelpFile);
        }

        SysFreeString(args[2].bstrVal);
        SysFreeString(args[1].bstrVal);

        // Cache the pane and set it visible.
        // Setting Visible = true on a freshly created CTP triggers
        // DoVerb(OLEIVERB_INPLACEACTIVATE) → activateInPlace → new ImGuiTaskPane.
        if (SUCCEEDED(hr) && result.vt == VT_DISPATCH && result.pdispVal)
        {
            g_taskPane = result.pdispVal;
            g_taskPane->AddRef();
            hr = TaskPane_SetVisible(g_taskPane, true);
            if (SUCCEEDED(hr))
                TaskPane_SetWidth(g_taskPane, 600);
            std::cerr << "[xlCOM]   pane created, SetVisible hr=0x"
                      << std::hex << hr << std::dec << '\n';
        }
        VariantClear(&result);

        return hr;
    });
XLL_COM_REGISTER(onTaskPaneClicked);

