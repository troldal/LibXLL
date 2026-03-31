#include "Connect.h"
#include <algorithm>
#include <string>
#include <unordered_map>

// Defined in dllmain.cpp — tracks live objects to support DllCanUnloadNow.
extern LONG g_lockCount;

Connect::Connect() : m_refCount(1)
{
    InterlockedIncrement(&g_lockCount);
}

// ---------------------------------------------------------------------------
// IUnknown
// ---------------------------------------------------------------------------

STDMETHODIMP Connect::QueryInterface(REFIID riid, void** ppvObject)
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

    if (riid == IID_IRibbonExtensibility)
    {
        *ppvObject = static_cast<IRibbonExtensibility*>(this);
        AddRef();
        return S_OK;
    }

    *ppvObject = nullptr;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) Connect::AddRef()
{
    return InterlockedIncrement(&m_refCount);
}

STDMETHODIMP_(ULONG) Connect::Release()
{
    const LONG ref = InterlockedDecrement(&m_refCount);
    if (ref == 0)
    {
        InterlockedDecrement(&g_lockCount);
        delete this;
    }
    return ref;
}

// ---------------------------------------------------------------------------
// IDispatch — handles ribbon callback dispatch.
// ---------------------------------------------------------------------------

struct WStrIHash
{
    size_t operator()(const std::wstring& s) const
    {
        std::wstring lower(s.size(), L'\0');
        std::transform(s.begin(), s.end(), lower.begin(), ::towlower);
        return std::hash<std::wstring>{}(lower);
    }
};

struct WStrIEqual
{
    bool operator()(const std::wstring& a, const std::wstring& b) const
    {
        return _wcsicmp(a.c_str(), b.c_str()) == 0;
    }
};

using DispatchMap = std::unordered_map<std::wstring, DISPID, WStrIHash, WStrIEqual>;

static const DispatchMap kDispatchMap =
{
    { L"OnButtonClicked", 1 },
};

STDMETHODIMP Connect::GetTypeInfoCount(UINT* pctinfo)
{
    if (!pctinfo) return E_POINTER;
    *pctinfo = 0;
    return S_OK;
}

STDMETHODIMP Connect::GetTypeInfo(UINT, LCID, ITypeInfo** ppTInfo)
{
    if (!ppTInfo) return E_POINTER;
    *ppTInfo = nullptr;
    return E_NOTIMPL;
}

STDMETHODIMP Connect::GetIDsOfNames(REFIID, LPOLESTR* rgszNames, UINT cNames,
                                     LCID, DISPID* rgDispId)
{
    if (!rgszNames || !rgDispId) return E_POINTER;

    HRESULT hr = S_OK;
    for (UINT i = 0; i < cNames; ++i)
    {
        const auto it = kDispatchMap.find(rgszNames[i]);
        if (it != kDispatchMap.end())
            rgDispId[i] = it->second;
        else
        {
            rgDispId[i] = DISPID_UNKNOWN;
            hr = DISP_E_UNKNOWNNAME;
        }
    }
    return hr;
}

STDMETHODIMP Connect::Invoke(DISPID dispIdMember, REFIID, LCID, WORD,
                              DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT*)
{
    if (dispIdMember == kDispatchMap.at(L"OnButtonClicked"))
    {
        MessageBoxW(nullptr,
                    L"Hello from xlCOM!",
                    L"xlCOM",
                    MB_OK | MB_ICONINFORMATION);
        return S_OK;
    }
    return DISP_E_MEMBERNOTFOUND;
}

// ---------------------------------------------------------------------------
// IDTExtensibility2
// ---------------------------------------------------------------------------

STDMETHODIMP Connect::OnConnection(IDispatch* /*Application*/,
                                    ext_ConnectMode /*ConnectMode*/,
                                    IDispatch* /*AddInInst*/,
                                    SAFEARRAY** /*custom*/)
{
    return S_OK;
}

STDMETHODIMP Connect::OnDisconnection(ext_DisconnectMode /*RemoveMode*/,
                                       SAFEARRAY** /*custom*/)
{
    return S_OK;
}

STDMETHODIMP Connect::OnAddInsUpdate(SAFEARRAY** /*custom*/)
{
    return S_OK;
}

STDMETHODIMP Connect::OnStartupComplete(SAFEARRAY** /*custom*/)
{
    return S_OK;
}

STDMETHODIMP Connect::OnBeginShutdown(SAFEARRAY** /*custom*/)
{
    return S_OK;
}

// ---------------------------------------------------------------------------
// IRibbonExtensibility
// ---------------------------------------------------------------------------

STDMETHODIMP Connect::GetCustomUI(BSTR /*RibbonID*/, BSTR* RibbonXml)
{
    if (!RibbonXml) return E_POINTER;

    static const wchar_t* xml =
        L"<customUI xmlns=\"http://schemas.microsoft.com/office/2006/01/customui\">"
        L"  <ribbon>"
        L"    <tabs>"
        L"      <tab id=\"xlCOMTab\" label=\"xlCOM\">"
        L"        <group id=\"xlCOMGroup\" label=\"xlCOM\">"
        L"          <button id=\"xlCOMButton\""
        L"                  label=\"Click Me\""
        L"                  size=\"large\""
        L"                  imageMso=\"HappyFace\""
        L"                  onAction=\"OnButtonClicked\" />"
        L"        </group>"
        L"      </tab>"
        L"    </tabs>"
        L"  </ribbon>"
        L"</customUI>";

    *RibbonXml = SysAllocString(xml);
    return *RibbonXml ? S_OK : E_OUTOFMEMORY;
}

