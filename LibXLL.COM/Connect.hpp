#pragma once

#include "Utils/ParseGUID.hpp"
#include "COMInterfaces.hpp"

inline constexpr CLSID CLSID_Connect = detail::parseGUID(XLCOM_CONNECT_GUID);

// Implements the _IDTExtensibility2 interface required for Excel COM add-ins.
// _IDTExtensibility2 derives from IDispatch, which derives from IUnknown.
class Connect : public _IDTExtensibility2, public IRibbonExtensibility // NOLINT
{
public:
    Connect();
    ~Connect();

    // IUnknown
    STDMETHODIMP         QueryInterface(REFIID riid, void** ppvObject) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // IDispatch
    STDMETHODIMP GetTypeInfoCount(UINT* pctinfo) override;
    STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo) override;
    STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
                               LCID lcid, DISPID* rgDispId) override;
    STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
                        DISPPARAMS* pDispParams, VARIANT* pVarResult,
                        EXCEPINFO* pExcepInfo, UINT* puArgErr) override;

    // IDTExtensibility2
    STDMETHODIMP OnConnection(IDispatch* Application, ext_ConnectMode ConnectMode,
                              IDispatch* AddInInst, SAFEARRAY** custom) override;
    STDMETHODIMP OnDisconnection(ext_DisconnectMode RemoveMode,
                                 SAFEARRAY** custom) override;
    STDMETHODIMP OnAddInsUpdate(SAFEARRAY** custom) override;
    STDMETHODIMP OnStartupComplete(SAFEARRAY** custom) override;
    STDMETHODIMP OnBeginShutdown(SAFEARRAY** custom) override;

    // IRibbonExtensibility
    STDMETHODIMP GetCustomUI(BSTR RibbonID, BSTR* RibbonXml) override;

private:
    LONG       m_refCount;
    IRibbonUI* m_ribbonUI = nullptr;
};
