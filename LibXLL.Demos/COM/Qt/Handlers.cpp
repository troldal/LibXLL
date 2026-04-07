// ---------------------------------------------------------------------------
// Content type — Qt Widgets
// ---------------------------------------------------------------------------

#include "QtTaskPane.hpp"
// #include "QtQuickTaskPane.hpp"  // Qt Quick (QML) alternative

#include "ActiveX/TaskPaneControl.hpp"
#include "COM/Macros.hpp"
#include "Utils/ImageFromPNGBytes.hpp"
#include "Utils/IsDarkMode.hpp"
#include <cmrc/cmrc.hpp>
#include <iostream>

CMRC_DECLARE(foo);


// ---------------------------------------------------------------------------
// Task pane COM identity — CLSID, ProgID, and friendly name for this add-in's
// TaskPaneControl instantiation.  Must match the values passed to
// xlcom_configure_addin() in CMakeLists.txt.
// ---------------------------------------------------------------------------

struct QtPaneTraits
{
    static constexpr CLSID   clsid        = detail::parseGUID("7C2D4F8E-3E1B-4A5C-9D6E-8F0B2A1C3D5E");
    static constexpr wchar_t progId[]     = L"xlCOM.Qt.TaskPane";
    static constexpr wchar_t friendlyName[] = L"xlCOM Qt Task Pane Control";
};

// ---------------------------------------------------------------------------
// Explicit instantiation of the TaskPaneControl with the chosen content type.
// This causes the compiler to emit the full COM class and factory in this TU.
// ---------------------------------------------------------------------------

template class TaskPaneControl<QtTaskPane, QtPaneTraits>;
template class TaskPaneControlFactory<QtTaskPane, QtPaneTraits>;

// ---------------------------------------------------------------------------
// Install COM server hooks so that DllGetClassObject, DllRegisterServer, and
// DllUnregisterServer know about the TaskPaneControl ActiveX class.
// Runs at DLL load time (static initialisation).
// ---------------------------------------------------------------------------

static const bool s_taskPaneHooked =
    detail::registerTaskPaneHooks<QtTaskPane, QtPaneTraits>();

// ---------------------------------------------------------------------------
// Add-in COM identity — CLSID, ProgID, friendly name, and description.
// Must match the values passed to xlcom_configure_addin() in CMakeLists.txt.
// ---------------------------------------------------------------------------

static const com::AddIn<IRibbonExtensibility, ICustomTaskPaneConsumer> s_addin(
    "1E0739E4-A1B5-4AB4-953C-2D8959196A13",
    L"xlCOM.Qt.Connect",
    L"xlCOM Qt",
    L"xlCOM Excel COM Add-in (Qt Widgets)"
);

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

XLL_COM_EVENT r_connection = s_addin.on<com::Connection>(
    [](IDispatch* application, ext_ConnectMode connectMode,
       IDispatch* /*addInInst*/, SAFEARRAY** /*custom*/)
    {
        std::cerr << "[xlCOM-Qt] OnConnection fired. "
                     "ConnectMode = " << static_cast<int>(connectMode) << '\n';

        if (application)
        {
            application->AddRef();
            g_excelApp = application;
        }
    });

// ---------------------------------------------------------------------------
// OnDisconnection — fires when Excel unloads the add-in.
// ---------------------------------------------------------------------------

XLL_COM_EVENT r_disconnection = s_addin.on<com::Disconnection>(
    [](ext_DisconnectMode disconnectMode, SAFEARRAY** /*custom*/)
    {
        std::cerr << "[xlCOM-Qt] OnDisconnection fired. "
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
        QtTaskPane::shutdown();
    });

// ---------------------------------------------------------------------------
// OnCTPFactoryAvailable — fired by ICustomTaskPaneConsumer::CTPFactoryAvailable.
// Stores the factory for use in ribbon callbacks.
// ---------------------------------------------------------------------------

XLL_COM_EVENT r_ctpFactory = s_addin.on<com::CTPFactory>(
    [](IDispatch* factory)
    {
        if (factory)
        {
            factory->AddRef();
            g_ctpFactory = factory;
        }
    });

// ---------------------------------------------------------------------------
// OnGetCustomUI — returns the RibbonX XML from the embedded resource.
// ---------------------------------------------------------------------------

XLL_COM_EVENT r_customUI = s_addin.on<com::GetCustomUI>(
    [](const com::String& /*ribbonId*/) -> com::String
    {
        auto fs   = cmrc::foo::get_filesystem();
        auto file = fs.open("Resources/XML/ribbon.xml");
        return com::String(std::string(file.begin(), file.end()));
    });

// ---------------------------------------------------------------------------
// IDispatch callbacks — registered by name via DispatchCallback<"Name">.
// Connect::GetIDsOfNames and Connect::Invoke resolve these automatically
// through the DispatchRegistry.
// ---------------------------------------------------------------------------

XLL_COM_EVENT r_onButtonClicked = s_addin.dispatch<"OnButtonClicked">(
    [](DISPPARAMS*, VARIANT*) -> HRESULT
    {
        if (!g_excelApp) return E_FAIL;

        LPOLESTR methodName = const_cast<LPOLESTR>(L"Run");
        DISPID   dispId     = 0;
        HRESULT  hr = g_excelApp->GetIDsOfNames(IID_NULL, &methodName, 1,
                                                  LOCALE_USER_DEFAULT, &dispId);
        if (FAILED(hr)) return hr;

        com::String macroName(L"QT.STATUS");
        VARIANT arg  = {};
        arg.vt       = VT_BSTR;
        arg.bstrVal  = macroName.get();

        DISPPARAMS params = {};
        params.rgvarg      = &arg;
        params.cArgs       = 1;

        return g_excelApp->Invoke(dispId, IID_NULL, LOCALE_USER_DEFAULT,
                                   DISPATCH_METHOD, &params, nullptr, nullptr, nullptr);
    });

XLL_COM_EVENT r_onRibbonLoad = s_addin.dispatch<"OnRibbonLoad">(
    [](DISPPARAMS* pDispParams, VARIANT*) -> HRESULT
    {
        if (pDispParams && pDispParams->cArgs >= 1 &&
            pDispParams->rgvarg[0].vt == VT_DISPATCH &&
            pDispParams->rgvarg[0].pdispVal)
        {
            std::cerr << "[xlCOM-Qt] OnRibbonLoad fired — IRibbonUI captured.\n";
        }
        return S_OK;
    });

XLL_COM_EVENT r_getButtonImage = s_addin.dispatch<"GetButtonImage">(
    [](DISPPARAMS*, VARIANT* pVarResult) -> HRESULT
    {
        if (!pVarResult) return E_POINTER;

        const char* resource = isDarkMode()
            ? "Resources/Images/button-help_dark48px.png"
            : "Resources/Images/button-help_light48px.png";

        auto fs   = cmrc::foo::get_filesystem();
        auto file = fs.open(resource);

        IPictureDisp* pPicture = ImageFromPNGBytes(file);
        if (!pPicture) return E_FAIL;

        VariantInit(pVarResult);
        pVarResult->vt       = VT_DISPATCH;
        pVarResult->pdispVal = pPicture;
        return S_OK;
    });

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

XLL_COM_EVENT r_taskPaneClicked = s_addin.dispatch<"OnTaskPaneClicked">(
    [](DISPPARAMS*, VARIANT*) -> HRESULT
    {
        std::cerr << "[xlCOM-Qt] OnTaskPaneClicked\n";

        if (g_taskPane)
        {
            bool visible = false;
            HRESULT hr = TaskPane_GetVisible(g_taskPane, visible);
            if (SUCCEEDED(hr) && visible)
            {
                std::cerr << "[xlCOM-Qt]   deleting visible pane\n";
                TaskPane_Delete(g_taskPane);
                g_taskPane->Release();
                g_taskPane = nullptr;
                return S_OK;
            }
            std::cerr << "[xlCOM-Qt]   pane hidden/stale — deleting and recreating\n";
            TaskPane_Delete(g_taskPane);
            g_taskPane->Release();
            g_taskPane = nullptr;
        }

        if (!g_ctpFactory)
        {
            std::cerr << "[xlCOM-Qt]   no CTP factory available\n";
            return E_FAIL;
        }

        LPOLESTR methodName = const_cast<LPOLESTR>(L"CreateCTP");
        DISPID   dispId     = 0;
        HRESULT  hr = g_ctpFactory->GetIDsOfNames(IID_NULL, &methodName, 1,
                                                    LOCALE_USER_DEFAULT, &dispId);
        if (FAILED(hr)) return hr;

        VARIANT args[3] = {};
        args[2].vt      = VT_BSTR;
        args[2].bstrVal = SysAllocString(QtPaneTraits::progId);
        args[1].vt      = VT_BSTR;
        args[1].bstrVal = SysAllocString(L"My Task Pane");
        args[0].vt      = VT_ERROR;
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
                fprintf(stderr, "[xlCOM-Qt] CreateCTP failed: %ls\n", excep.bstrDescription);
            SysFreeString(excep.bstrDescription);
            SysFreeString(excep.bstrSource);
            SysFreeString(excep.bstrHelpFile);
        }

        SysFreeString(args[2].bstrVal);
        SysFreeString(args[1].bstrVal);

        if (SUCCEEDED(hr) && result.vt == VT_DISPATCH && result.pdispVal)
        {
            g_taskPane = result.pdispVal;
            g_taskPane->AddRef();
            hr = TaskPane_SetVisible(g_taskPane, true);
            if (SUCCEEDED(hr))
                TaskPane_SetWidth(g_taskPane, 600);
            std::cerr << "[xlCOM-Qt]   pane created, SetVisible hr=0x"
                      << std::hex << hr << std::dec << '\n';
        }
        VariantClear(&result);

        return hr;
    });

