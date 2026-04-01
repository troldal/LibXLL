#include "COM/Macros.hpp"
#include "Utils/ImageFromPNGBytes.hpp"
#include "Utils/IsDarkMode.hpp"
#include <cmrc/cmrc.hpp>
#include <iostream>

CMRC_DECLARE(foo);

// ---------------------------------------------------------------------------
// OnConnection — fires when Excel loads and connects the add-in.
// ---------------------------------------------------------------------------

auto onConnection = com::OnConnection(
    [](IDispatch* /*application*/, ext_ConnectMode connectMode,
       IDispatch* /*addInInst*/,   SAFEARRAY** /*custom*/)
    {
        std::cerr << "[xlCOM] OnConnection fired. "
                     "ConnectMode = " << static_cast<int>(connectMode) << '\n';
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
    });
XLL_COM_REGISTER(onDisconnection);

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
        MessageBoxW(nullptr,
                    L"Hello from xlCOM!",
                    L"xlCOM",
                    MB_OK | MB_ICONINFORMATION);
        return S_OK;
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

