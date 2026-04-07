// ---------------------------------------------------------------------------
// COMServer.hpp — COM server boilerplate for Excel COM add-ins.
//
// This file is part of the LibXLL.COM library.  Users should NOT modify it;
// all add-in behaviour is defined through handlers (see Handlers.cpp).
//
// Everything is marked inline (or lives inside a class body) so the entire
// server implementation can live in a single header.  It is pulled in
// transitively via COM/Macros.hpp — users never include it directly.
// ---------------------------------------------------------------------------

#pragma once

#include "../AddIn.hpp"
#include "ComAuto.hpp"
#include "DispatchRegistry.hpp"
#include "Interfaces.hpp"
#include "String.hpp"
#include <type_traits>

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

// Tracks all live objects and IClassFactory::LockServer calls.
// DllCanUnloadNow returns S_OK only when this reaches zero.
inline LONG    g_lockCount = 0;
inline HMODULE g_hModule   = nullptr;

// ---------------------------------------------------------------------------
// Extension hooks — let external headers (e.g. ActiveX controls) plug
// additional class factories and registration logic into the COM server
// without modifying this boilerplate.
// ---------------------------------------------------------------------------

// Called by DllGetClassObject when the add-in CLSID doesn't match.
// Return CLASS_E_CLASSNOTAVAILABLE to fall through.
inline HRESULT (*g_pfnExtraGetClassObject)(REFCLSID, REFIID, LPVOID*) = nullptr;

// Called at the end of DllRegisterServer; receives the DLL path.
inline HRESULT (*g_pfnExtraRegister)(const wchar_t* dllPath) = nullptr;

// Called at the end of DllUnregisterServer.
inline void (*g_pfnExtraUnregister)() = nullptr;

// ---------------------------------------------------------------------------
// Interface implementation mixins — each provides the COM method body for
// one optional interface.  AddInServer inherits from these as needed.
// ---------------------------------------------------------------------------

namespace mixin {

struct RibbonImpl : public IRibbonExtensibility
{
    STDMETHODIMP GetCustomUI(BSTR RibbonID, BSTR* RibbonXml) override
    {
        if (!RibbonXml) return E_POINTER;

        com::String result = com::OnGetCustomUI::Execute(com::String(RibbonID));
        if (result.empty()) return E_FAIL;

        *RibbonXml = result.release();
        return S_OK;
    }
};

struct TaskPaneImpl : public ICustomTaskPaneConsumer
{
    STDMETHODIMP CTPFactoryAvailable(IDispatch* CTPFactoryInst) override
    {
        com::OnCTPFactoryAvailable::Execute(CTPFactoryInst);
        return S_OK;
    }
};

// Map a COM interface type to its implementation mixin.
template<typename T> struct ImplFor;
template<> struct ImplFor<IRibbonExtensibility>    { using type = RibbonImpl; };
template<> struct ImplFor<ICustomTaskPaneConsumer> { using type = TaskPaneImpl; };

// Map a COM interface type to its IID.
template<typename T> struct IIDOf;
template<> struct IIDOf<IRibbonExtensibility>    { static constexpr const IID& value = IID_IRibbonExtensibility; };
template<> struct IIDOf<ICustomTaskPaneConsumer> { static constexpr const IID& value = IID_ICustomTaskPaneConsumer; };

} // namespace mixin

// ---------------------------------------------------------------------------
// AddInServer<Interfaces...> — implements _IDTExtensibility2 plus any
// optional COM interfaces listed in the template parameter pack.
//
// Every virtual method delegates to the handler infrastructure
// (com::EventHandler / com::QueryHandler / com::DispatchRegistry) so that
// users never need to touch this class.
// ---------------------------------------------------------------------------

template<typename... Interfaces>
class AddInServer : public _IDTExtensibility2,
                    public mixin::ImplFor<Interfaces>::type... // NOLINT
{
public:
    AddInServer() : m_refCount(1)
    {
        InterlockedIncrement(&g_lockCount);
    }

    ~AddInServer()
    {
        if (m_ribbonUI)
        {
            m_ribbonUI->Release();
            m_ribbonUI = nullptr;
        }
    }

    // --- IUnknown ---------------------------------------------------------

    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) override
    {
        if (!ppvObject)
            return E_POINTER;

        if (riid == IID_IUnknown || riid == IID_IDispatch ||
            riid == IID_IDTExtensibility2)
        {
            *ppvObject = static_cast<_IDTExtensibility2*>(this);
            AddRef();
            return S_OK;
        }

        // Try each optional interface via fold expression.
        if ((tryMatch<Interfaces>(riid, ppvObject) || ...))
            return S_OK;

        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() override
    {
        return InterlockedIncrement(&m_refCount);
    }

    STDMETHODIMP_(ULONG) Release() override
    {
        const LONG ref = InterlockedDecrement(&m_refCount);
        if (ref == 0)
        {
            InterlockedDecrement(&g_lockCount);
            delete this;
        }
        return ref;
    }

    // --- IDispatch --------------------------------------------------------

    STDMETHODIMP GetTypeInfoCount(UINT* pctinfo) override
    {
        if (!pctinfo) return E_POINTER;
        *pctinfo = 0;
        return S_OK;
    }

    STDMETHODIMP GetTypeInfo(UINT, LCID, ITypeInfo** ppTInfo) override
    {
        if (!ppTInfo) return E_POINTER;
        *ppTInfo = nullptr;
        return E_NOTIMPL;
    }

    STDMETHODIMP GetIDsOfNames(REFIID, LPOLESTR* rgszNames, UINT cNames,
                                LCID, DISPID* rgDispId) override
    {
        if (!rgszNames || !rgDispId) return E_POINTER;

        const auto& registry = com::DispatchRegistry::instance();
        HRESULT hr = S_OK;
        for (UINT i = 0; i < cNames; ++i)
        {
            rgDispId[i] = registry.getDispId(rgszNames[i]);
            if (rgDispId[i] == DISPID_UNKNOWN)
                hr = DISP_E_UNKNOWNNAME;
        }
        return hr;
    }

    STDMETHODIMP Invoke(DISPID dispIdMember, REFIID, LCID, WORD,
                         DISPPARAMS* pDispParams, VARIANT* pVarResult,
                         EXCEPINFO*, UINT*) override
    {
        return com::DispatchRegistry::instance().invoke(dispIdMember,
                                                        pDispParams, pVarResult);
    }

    // --- IDTExtensibility2 ------------------------------------------------

    STDMETHODIMP OnConnection(IDispatch* Application,
                               ext_ConnectMode ConnectMode,
                               IDispatch* AddInInst,
                               SAFEARRAY** custom) override
    {
        com::OnConnection::Execute(Application, ConnectMode, AddInInst, custom);
        return S_OK;
    }

    STDMETHODIMP OnDisconnection(ext_DisconnectMode RemoveMode,
                                  SAFEARRAY** custom) override
    {
        com::OnDisconnection::Execute(RemoveMode, custom);
        return S_OK;
    }

    STDMETHODIMP OnAddInsUpdate(SAFEARRAY** custom) override
    {
        com::OnAddInsUpdate::Execute(custom);
        return S_OK;
    }

    STDMETHODIMP OnStartupComplete(SAFEARRAY** custom) override
    {
        com::OnStartupComplete::Execute(custom);
        return S_OK;
    }

    STDMETHODIMP OnBeginShutdown(SAFEARRAY** custom) override
    {
        com::OnBeginShutdown::Execute(custom);
        return S_OK;
    }

private:
    LONG       m_refCount;
    IRibbonUI* m_ribbonUI = nullptr;

    // Helper for QueryInterface fold expression.
    template<typename T>
    bool tryMatch(REFIID riid, void** ppv)
    {
        if (riid == mixin::IIDOf<T>::value)
        {
            *ppv = static_cast<T*>(this);
            AddRef();
            return true;
        }
        return false;
    }
};

// ---------------------------------------------------------------------------
// IClassFactory for AddInServer
// ---------------------------------------------------------------------------

class AddInServerFactory : public IClassFactory // NOLINT
{
public:
    AddInServerFactory() : m_refCount(1) {}

    // IUnknown
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

    STDMETHODIMP_(ULONG) AddRef() override
    {
        return InterlockedIncrement(&m_refCount);
    }

    STDMETHODIMP_(ULONG) Release() override
    {
        const LONG ref = InterlockedDecrement(&m_refCount);
        if (ref == 0) delete this;
        return ref;
    }

    // IClassFactory — delegates to the type-erased factory registered
    // by AddIn<Interfaces...>'s constructor.
    STDMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppv) override
    {
        if (!ppv)      return E_POINTER;
        if (pUnkOuter) return CLASS_E_NOAGGREGATION;

        if (!com::detail::g_createServer)
            return E_UNEXPECTED;

        return com::detail::g_createServer(riid, ppv);
    }

    STDMETHODIMP LockServer(BOOL fLock) override
    {
        if (fLock) InterlockedIncrement(&g_lockCount);
        else       InterlockedDecrement(&g_lockCount);
        return S_OK;
    }

private:
    LONG m_refCount;
};

// ---------------------------------------------------------------------------
// DllMain
// ---------------------------------------------------------------------------

inline BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}

// ---------------------------------------------------------------------------
// Registration helpers
// ---------------------------------------------------------------------------

namespace detail
{
    // Write a REG_SZ value, creating the key if necessary.
    inline HRESULT SetRegString(HKEY hRoot, const wchar_t* subKey,
                                const wchar_t* valueName, const wchar_t* value)
    {
        HKEY hKey = nullptr;
        LSTATUS st = RegCreateKeyExW(hRoot, subKey, 0, nullptr,
                                     REG_OPTION_NON_VOLATILE, KEY_SET_VALUE,
                                     nullptr, &hKey, nullptr);
        if (st != ERROR_SUCCESS) return HRESULT_FROM_WIN32(st);

        st = RegSetValueExW(hKey, valueName, 0, REG_SZ,
                            reinterpret_cast<const BYTE*>(value),
                            static_cast<DWORD>((wcslen(value) + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);
        return HRESULT_FROM_WIN32(st);
    }

    // Write a REG_DWORD value, creating the key if necessary.
    inline HRESULT SetRegDword(HKEY hRoot, const wchar_t* subKey,
                               const wchar_t* valueName, DWORD value)
    {
        HKEY hKey = nullptr;
        LSTATUS st = RegCreateKeyExW(hRoot, subKey, 0, nullptr,
                                     REG_OPTION_NON_VOLATILE, KEY_SET_VALUE,
                                     nullptr, &hKey, nullptr);
        if (st != ERROR_SUCCESS) return HRESULT_FROM_WIN32(st);

        st = RegSetValueExW(hKey, valueName, 0, REG_DWORD,
                            reinterpret_cast<const BYTE*>(&value), sizeof(DWORD));
        RegCloseKey(hKey);
        return HRESULT_FROM_WIN32(st);
    }

    // Recursively delete a registry key and all its subkeys.
    inline void DeleteRegKey(HKEY hRoot, const wchar_t* subKey)
    {
        HKEY hKey = nullptr;
        if (RegOpenKeyExW(hRoot, subKey, 0, KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS)
            return;

        wchar_t child[256];
        while (true)
        {
            DWORD len = static_cast<DWORD>(_countof(child));
            if (RegEnumKeyExW(hKey, 0, child, &len,
                              nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
                break;

            wchar_t fullChild[512];
            swprintf_s(fullChild, L"%s\\%s", subKey, child);
            DeleteRegKey(hRoot, fullChild);
        }
        RegCloseKey(hKey);
        RegDeleteKeyW(hRoot, subKey);
    }
} // namespace detail

// ---------------------------------------------------------------------------
// COM server exports
//
// MSVC:      #pragma comment(linker) exports the symbols — no attribute needed
//            (and __declspec(dllexport) conflicts with the SDK forward-decls).
// clang-cl:  __declspec(dllexport) is required to force emission of the inline
//            bodies; the redeclaration warning from combaseapi.h is suppressed.
// MinGW/GCC: __declspec(dllexport) is needed because #pragma comment is ignored.
// ---------------------------------------------------------------------------

#if defined(_MSC_VER) && !defined(__clang__)
   // Pure MSVC — use linker pragmas, no dllexport (avoids C2375).
#  define XLLCOM_EXPORT
#  pragma comment(linker, "/EXPORT:DllGetClassObject,PRIVATE")
#  pragma comment(linker, "/EXPORT:DllCanUnloadNow,PRIVATE")
#  pragma comment(linker, "/EXPORT:DllRegisterServer,PRIVATE")
#  pragma comment(linker, "/EXPORT:DllUnregisterServer,PRIVATE")
#else
   // clang-cl and MinGW/GCC — dllexport forces emission of inline bodies.
#  define XLLCOM_EXPORT __declspec(dllexport)
#  if defined(__clang__) && defined(_MSC_VER)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wdll-attribute-on-redeclaration"
#  endif
#endif

extern "C" inline XLLCOM_EXPORT
HRESULT STDAPICALLTYPE DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    if (!ppv) return E_POINTER;

    if (rclsid == com::addIn().clsid())
    {
        auto* pFactory = new(std::nothrow) AddInServerFactory();
        if (!pFactory) return E_OUTOFMEMORY;

        const HRESULT hr = pFactory->QueryInterface(riid, ppv);
        pFactory->Release();
        return hr;
    }

    // Extension hook — let additional class factories handle the request.
    if (g_pfnExtraGetClassObject)
        return g_pfnExtraGetClassObject(rclsid, riid, ppv);

    return CLASS_E_CLASSNOTAVAILABLE;
}

extern "C" inline XLLCOM_EXPORT
HRESULT STDAPICALLTYPE DllCanUnloadNow()
{
    return (g_lockCount == 0) ? S_OK : S_FALSE;
}

extern "C" inline XLLCOM_EXPORT
HRESULT STDAPICALLTYPE DllRegisterServer()
{
    // Retrieve the full path to this DLL.
    wchar_t dllPath[MAX_PATH] = {};
    if (!GetModuleFileNameW(g_hModule, dllPath, MAX_PATH))
        return HRESULT_FROM_WIN32(GetLastError());

    // Build CLSID string: {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
    wchar_t clsidStr[64] = {};
    StringFromGUID2(com::addIn().clsid(), clsidStr, static_cast<int>(_countof(clsidStr)));

    // HKCR\CLSID\{...}  —  friendly name
    wchar_t key[512] = {};
    swprintf_s(key, L"CLSID\\%s", clsidStr);
    HRESULT hr = detail::SetRegString(HKEY_CLASSES_ROOT, key, nullptr,
                                      com::addIn().friendlyName());
    if (FAILED(hr)) return hr;

    // HKCR\CLSID\{...}\InprocServer32
    swprintf_s(key, L"CLSID\\%s\\InprocServer32", clsidStr);
    hr = detail::SetRegString(HKEY_CLASSES_ROOT, key, nullptr, dllPath);
    if (FAILED(hr)) return hr;
    hr = detail::SetRegString(HKEY_CLASSES_ROOT, key, L"ThreadingModel", L"Apartment");
    if (FAILED(hr)) return hr;

    // HKCR\CLSID\{...}\ProgID
    swprintf_s(key, L"CLSID\\%s\\ProgID", clsidStr);
    hr = detail::SetRegString(HKEY_CLASSES_ROOT, key, nullptr, com::addIn().progId());
    if (FAILED(hr)) return hr;

    // HKCR\{ProgID}  —  ProgID → CLSID mapping
    hr = detail::SetRegString(HKEY_CLASSES_ROOT, com::addIn().progId(), nullptr,
                              com::addIn().friendlyName());
    if (FAILED(hr)) return hr;
    wchar_t progIdClsidKey[512] = {};
    swprintf_s(progIdClsidKey, L"%s\\CLSID", com::addIn().progId());
    hr = detail::SetRegString(HKEY_CLASSES_ROOT, progIdClsidKey, nullptr, clsidStr);
    if (FAILED(hr)) return hr;

    // HKCU\Software\Microsoft\Office\Excel\Addins\{ProgID}
    // LoadBehavior = 3  →  Connected + Load at Startup
    wchar_t addinKey[512] = {};
    swprintf_s(addinKey, L"Software\\Microsoft\\Office\\Excel\\Addins\\%s",
               com::addIn().progId());
    hr = detail::SetRegString(HKEY_CURRENT_USER, addinKey, L"FriendlyName",
                              com::addIn().friendlyName());
    if (FAILED(hr)) return hr;
    hr = detail::SetRegString(HKEY_CURRENT_USER, addinKey, L"Description",
                              com::addIn().description());
    if (FAILED(hr)) return hr;
    hr = detail::SetRegDword(HKEY_CURRENT_USER, addinKey, L"LoadBehavior", 3);
    if (FAILED(hr)) return hr;

    // Extension hook — register additional CLSIDs.
    if (g_pfnExtraRegister)
    {
        hr = g_pfnExtraRegister(dllPath);
        if (FAILED(hr)) return hr;
    }

    return S_OK;
}

extern "C" inline XLLCOM_EXPORT
HRESULT STDAPICALLTYPE DllUnregisterServer()
{
    wchar_t clsidStr[64] = {};
    StringFromGUID2(com::addIn().clsid(), clsidStr, static_cast<int>(_countof(clsidStr)));

    wchar_t key[512] = {};

    // Remove COM class registration
    swprintf_s(key, L"CLSID\\%s", clsidStr);
    detail::DeleteRegKey(HKEY_CLASSES_ROOT, key);

    // Remove ProgID
    detail::DeleteRegKey(HKEY_CLASSES_ROOT, com::addIn().progId());

    // Remove Excel add-in entry
    wchar_t addinKey[512] = {};
    swprintf_s(addinKey, L"Software\\Microsoft\\Office\\Excel\\Addins\\%s",
               com::addIn().progId());
    detail::DeleteRegKey(HKEY_CURRENT_USER, addinKey);

    // Extension hook — unregister additional CLSIDs.
    if (g_pfnExtraUnregister)
        g_pfnExtraUnregister();

    return S_OK;
}

#if defined(__clang__) && defined(_MSC_VER)
#  pragma clang diagnostic pop
#endif

#undef XLLCOM_EXPORT

