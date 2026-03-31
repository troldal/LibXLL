#include "Connect.h"
#include <stdio.h>

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

// Tracks all live objects and IClassFactory::LockServer calls.
// DllCanUnloadNow returns S_OK only when this reaches zero.
LONG     g_lockCount = 0;
HMODULE  g_hModule   = nullptr;

// ---------------------------------------------------------------------------
// IClassFactory for Connect
// ---------------------------------------------------------------------------

class ConnectClassFactory : public IClassFactory // NOLINT
{
public:
    ConnectClassFactory() : m_refCount(1) {}

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

    // IClassFactory
    STDMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppv) override
    {
        if (!ppv)      return E_POINTER;
        if (pUnkOuter) return CLASS_E_NOAGGREGATION;

        Connect* pConnect = new(std::nothrow) Connect();
        if (!pConnect) return E_OUTOFMEMORY;

        const HRESULT hr = pConnect->QueryInterface(riid, ppv);
        pConnect->Release();
        return hr;
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

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}

// ---------------------------------------------------------------------------
// COM server exports
// ---------------------------------------------------------------------------

#pragma comment(linker, "/EXPORT:DllGetClassObject,PRIVATE")
#pragma comment(linker, "/EXPORT:DllCanUnloadNow,PRIVATE")
#pragma comment(linker, "/EXPORT:DllRegisterServer,PRIVATE")
#pragma comment(linker, "/EXPORT:DllUnregisterServer,PRIVATE")

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    if (!ppv) return E_POINTER;

    if (rclsid == CLSID_Connect)
    {
        ConnectClassFactory* pFactory = new(std::nothrow) ConnectClassFactory();
        if (!pFactory) return E_OUTOFMEMORY;

        const HRESULT hr = pFactory->QueryInterface(riid, ppv);
        pFactory->Release();
        return hr;
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllCanUnloadNow()
{
    return (g_lockCount == 0) ? S_OK : S_FALSE;
}

// ---------------------------------------------------------------------------
// Registration helpers
// ---------------------------------------------------------------------------

namespace
{
    // Write a REG_SZ value, creating the key if necessary.
    HRESULT SetRegString(HKEY hRoot, const wchar_t* subKey,
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
    HRESULT SetRegDword(HKEY hRoot, const wchar_t* subKey,
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
    void DeleteRegKey(HKEY hRoot, const wchar_t* subKey)
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
}

// ---------------------------------------------------------------------------
// DllRegisterServer / DllUnregisterServer
// ---------------------------------------------------------------------------

STDAPI DllRegisterServer()
{
    // Retrieve the full path to this DLL.
    wchar_t dllPath[MAX_PATH] = {};
    if (!GetModuleFileNameW(g_hModule, dllPath, MAX_PATH))
        return HRESULT_FROM_WIN32(GetLastError());

    // Build CLSID string: {5C6D7E8F-9A0B-4C1D-8E2F-3A4B5C6D7E8F}
    wchar_t clsidStr[64] = {};
    StringFromGUID2(CLSID_Connect, clsidStr, static_cast<int>(_countof(clsidStr)));

    // HKCR\CLSID\{...}  —  friendly name
    wchar_t key[512] = {};
    swprintf_s(key, L"CLSID\\%s", clsidStr);
    HRESULT hr = SetRegString(HKEY_CLASSES_ROOT, key, nullptr, L"xlCOM Connect");
    if (FAILED(hr)) return hr;

    // HKCR\CLSID\{...}\InprocServer32
    swprintf_s(key, L"CLSID\\%s\\InprocServer32", clsidStr);
    hr = SetRegString(HKEY_CLASSES_ROOT, key, nullptr, dllPath);
    if (FAILED(hr)) return hr;
    hr = SetRegString(HKEY_CLASSES_ROOT, key, L"ThreadingModel", L"Apartment");
    if (FAILED(hr)) return hr;

    // HKCR\CLSID\{...}\ProgID
    swprintf_s(key, L"CLSID\\%s\\ProgID", clsidStr);
    hr = SetRegString(HKEY_CLASSES_ROOT, key, nullptr, L"xlCOM.Connect");
    if (FAILED(hr)) return hr;

    // HKCR\xlCOM.Connect  —  ProgID → CLSID mapping
    hr = SetRegString(HKEY_CLASSES_ROOT, L"xlCOM.Connect", nullptr, L"xlCOM Connect");
    if (FAILED(hr)) return hr;
    hr = SetRegString(HKEY_CLASSES_ROOT, L"xlCOM.Connect\\CLSID", nullptr, clsidStr);
    if (FAILED(hr)) return hr;

    // HKCU\Software\Microsoft\Office\Excel\Addins\xlCOM.Connect
    // LoadBehavior = 3  →  Connected + Load at Startup
    const wchar_t* addinKey =
        L"Software\\Microsoft\\Office\\Excel\\Addins\\xlCOM.Connect";
    hr = SetRegString(HKEY_CURRENT_USER, addinKey, L"FriendlyName", L"xlCOM");
    if (FAILED(hr)) return hr;
    hr = SetRegString(HKEY_CURRENT_USER, addinKey, L"Description",
                      L"xlCOM Excel COM Add-in sample");
    if (FAILED(hr)) return hr;
    hr = SetRegDword(HKEY_CURRENT_USER, addinKey, L"LoadBehavior", 3);
    if (FAILED(hr)) return hr;

    return S_OK;
}

STDAPI DllUnregisterServer()
{
    wchar_t clsidStr[64] = {};
    StringFromGUID2(CLSID_Connect, clsidStr, static_cast<int>(_countof(clsidStr)));

    wchar_t key[512] = {};

    // Remove COM class registration
    swprintf_s(key, L"CLSID\\%s", clsidStr);
    DeleteRegKey(HKEY_CLASSES_ROOT, key);

    // Remove ProgID
    DeleteRegKey(HKEY_CLASSES_ROOT, L"xlCOM.Connect");

    // Remove Excel add-in entry
    DeleteRegKey(HKEY_CURRENT_USER,
                 L"Software\\Microsoft\\Office\\Excel\\Addins\\xlCOM.Connect");

    return S_OK;
}
