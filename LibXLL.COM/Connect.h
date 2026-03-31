#pragma once
#include "imports.h"

// {1E0739E0-A1B5-4AB4-953C-2D8959196A13} Unique for this add-in
static const CLSID CLSID_Connect =
    {0x1E0739E0, 0xA1B5, 0x4AB4, {0x95, 0x3C, 0x2D, 0x89, 0x59, 0x19, 0x6A, 0x13}};

// Implements the _IDTExtensibility2 interface required for Excel COM add-ins.
// _IDTExtensibility2 derives from IDispatch, which derives from IUnknown.
class Connect : public _IDTExtensibility2, public IRibbonExtensibility // NOLINT
{
public:
    Connect();

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
    LONG m_refCount;
};
