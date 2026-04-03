#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <new>

// IDTExtensibility2 — manually defined from the AddIn Designer Objects type library
// (LIBID: {AC0714F2-3D04-11D1-AE7D-00A0C90F26F4}).
// This replaces: #import "libid:AC0714F2-..." no_namespace raw_interfaces_only
// so the code compiles with both MSVC and clang-cl (which lacks #import support).

enum ext_ConnectMode : int
{
    ext_cm_AfterStartup = 0, // COM add-in was loaded after the application started. Typically this is if user has chosen to load an add-in from the COM Add-Ins dialog
    ext_cm_Startup      = 1, // COM add-in was loaded at startup.
    ext_cm_External     = 2, // COM add-in was loaded externally by another program or component.
    ext_cm_CommandLine  = 3, // COM add-in was loaded through the application's command line.
    ext_cm_Solution     = 4, // COM add-in was loaded when user loaded a solution that required it.
    ext_cm_UISetup      = 5  // COM add-in was started for the first time since being installed.
};

enum ext_DisconnectMode : int
{
    ext_dm_HostShutdown    = 0, // COM add-in was unloaded when the host application was closed.
    ext_dm_UserClosed      = 1, // COM add-in was unloaded when the user cleared its check box in the COM Add-Ins dialog box, or when the Connect property of the COMAddIn object corresponding to the COM add-in was set to false.
    ext_dm_UISetupComplete = 2, // COM add-in was unloaded after the environment setup completed and after the OnConnection method returns.
    ext_dm_SolutionClosed  = 3, // Only used with Visual Studio COM add-ins
};

// {B65AD801-ABAF-11D0-BB8B-00A0C90F2744}
static constexpr IID IID_IDTExtensibility2 =
    {0xB65AD801, 0xABAF, 0x11D0, {0xBB, 0x8B, 0x00, 0xA0, 0xC9, 0x0F, 0x27, 0x44}};

struct _IDTExtensibility2 : public IDispatch // NOLINT
{
public:
    virtual HRESULT STDMETHODCALLTYPE OnConnection(
        IDispatch *Application,
        ext_ConnectMode ConnectMode,
        IDispatch *AddInInst,
        SAFEARRAY **custom) = 0;
    virtual HRESULT STDMETHODCALLTYPE OnDisconnection(ext_DisconnectMode RemoveMode, SAFEARRAY **custom) = 0;
    virtual HRESULT STDMETHODCALLTYPE OnAddInsUpdate(SAFEARRAY **custom) = 0;
    virtual HRESULT STDMETHODCALLTYPE OnStartupComplete(SAFEARRAY **custom) = 0;
    virtual HRESULT STDMETHODCALLTYPE OnBeginShutdown(SAFEARRAY **custom) = 0;
};

// {000C0396-0000-0000-C000-000000000046}
static constexpr IID IID_IRibbonExtensibility =
    {0x000C0396, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

// IRibbonExtensibility — manually defined to avoid importing the large Office type library.
struct IRibbonExtensibility : public IDispatch // NOLINT
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetCustomUI(
        BSTR RibbonID,
        BSTR *RibbonXml) = 0;
};

// {000C03A7-0000-0000-C000-000000000046}
static constexpr IID IID_IRibbonUI =
    {0x000C03A7, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

// IRibbonUI — passed to the onLoad callback; used to invalidate ribbon controls.
// Vtable layout matches the Office 2007+ type library definition.
struct IRibbonUI : public IDispatch // NOLINT
{
    virtual HRESULT STDMETHODCALLTYPE Invalidate() = 0;
    virtual HRESULT STDMETHODCALLTYPE InvalidateControl(BSTR controlID) = 0;
    virtual HRESULT STDMETHODCALLTYPE InvalidateControlMso(BSTR controlID) = 0;
    virtual HRESULT STDMETHODCALLTYPE ActivateTab(BSTR controlID) = 0;
    virtual HRESULT STDMETHODCALLTYPE ActivateTabMso(BSTR controlID) = 0;
    virtual HRESULT STDMETHODCALLTYPE ActivateTabQ(BSTR controlID, BSTR ribbonID) = 0;
};

// {000C033E-0000-0000-C000-000000000046}
static constexpr IID IID_ICustomTaskPaneConsumer =
    {0x000C033E, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

// ICustomTaskPaneConsumer — implemented by the add-in so that Excel can hand
// over the ICTPFactory (as IDispatch*) when the add-in connects.
// Office queries for this interface via QueryInterface during OnConnection.
// The real Office definition derives from IDispatch (vtable slot 7).
struct ICustomTaskPaneConsumer : public IDispatch // NOLINT
{
    // CTPFactoryInst is an ICTPFactory*, passed as IDispatch* to avoid
    // pulling in the full Office type library.
    virtual HRESULT STDMETHODCALLTYPE CTPFactoryAvailable(IDispatch* CTPFactoryInst) = 0;
};

