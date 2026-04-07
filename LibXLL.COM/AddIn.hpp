// ---------------------------------------------------------------------------
// AddIn.hpp — declares the COM add-in identity for this DLL.
//
// This file is part of the LibXLL.COM library.
//
// Each DLL that uses LibXLL.COM must construct exactly one static
// com::AddIn<Interfaces...> instance (typically in Handlers.cpp) to declare
// its GUID, ProgID, friendly name, description, and the set of optional
// COM interfaces the add-in server should expose to Excel.
//
// The constructor self-registers into per-DLL global pointers so that the
// COM server exports (DllGetClassObject, DllRegisterServer, etc.) can
// retrieve the identity and factory at runtime.
//
// Because the global pointers are `inline` variables in a header-only
// library, each DLL image gets its own copy — there is no cross-DLL
// conflict when multiple add-in DLLs are loaded into the same process.
//
// Usage:
//   static const com::AddIn<IRibbonExtensibility, ICustomTaskPaneConsumer>
//       s_addin(
//           "1E0739E0-A1B5-4AB4-953C-2D8959196A13",
//           L"xlCOM.ImGui.Connect",
//           L"xlCOM ImGui",
//           L"xlCOM Excel COM Add-in (Dear ImGui)"
//       );
//
//   // Register event handlers using XLL_COM_EVENT (= static const auto):
//   XLL_COM_EVENT r1 = s_addin.on<com::Connection>(
//       [](IDispatch* app, ext_ConnectMode mode, IDispatch*, SAFEARRAY**) {
//           // ...
//       });
//
//   XLL_COM_EVENT r2 = s_addin.on<com::GetCustomUI>(
//       [](const com::String& ribbonId) -> com::String { ... });
//
//   XLL_COM_EVENT r3 = s_addin.dispatch<"OnButtonClicked">(
//       [](DISPPARAMS*, VARIANT*) -> HRESULT { ... });
// ---------------------------------------------------------------------------

#pragma once

#include "Utils/ParseGUID.hpp"
#include "COM/Interfaces.hpp"
#include <fixed_string.hpp>
#include <functional>
#include <type_traits>

// Forward-declare the server template so AddIn can reference it.
template<typename... Interfaces>
class AddInServer;

namespace com {

// ---------------------------------------------------------------------------
// detail — internal helpers
// ---------------------------------------------------------------------------

namespace detail {

    // Per-DLL global pointer to the add-in identity — set during static init.
    struct AddInBase;
    inline const AddInBase* g_addIn = nullptr;

    // Per-DLL type-erased factory function — creates the right
    // AddInServer<Interfaces...> for DllGetClassObject.
    inline HRESULT (*g_createServer)(REFIID, void**) = nullptr;

    // Dummy type returned by on<>() and dispatch<>() so the call can be
    // used as a static variable initialiser at namespace scope.
    struct Reg { };

    // ---------------------------------------------------------------------------
    // RequiredInterface<TEvent> — maps an event policy type to the COM
    // interface that must be present in the AddIn template parameter list.
    // `void` means the event is always available (_IDTExtensibility2).
    // ---------------------------------------------------------------------------
    template<typename TEvent> struct RequiredInterface      { using type = void; };

} // namespace detail

// ---------------------------------------------------------------------------
// AddInBase — non-template base storing the add-in identity.
// ---------------------------------------------------------------------------

namespace detail {

struct AddInBase
{
    AddInBase(const char* guid, const wchar_t* progId,
              const wchar_t* friendlyName, const wchar_t* description)
        : m_clsid(::detail::parseGUID(guid))
        , m_progId(progId)
        , m_friendlyName(friendlyName)
        , m_description(description)
    {
        g_addIn = this;
    }

    [[nodiscard]] const CLSID&   clsid()        const { return m_clsid; }
    [[nodiscard]] const wchar_t* progId()        const { return m_progId; }
    [[nodiscard]] const wchar_t* friendlyName()  const { return m_friendlyName; }
    [[nodiscard]] const wchar_t* description()   const { return m_description; }

private:
    CLSID          m_clsid;
    const wchar_t* m_progId;
    const wchar_t* m_friendlyName;
    const wchar_t* m_description;
};

} // namespace detail

// ---------------------------------------------------------------------------
// AddIn<Interfaces...> — declares the COM add-in identity and the set of
// optional COM interfaces the add-in server should expose.
//
// _IDTExtensibility2 is always implemented.  Optional interfaces:
//   IRibbonExtensibility     — Ribbon customisation via GetCustomUI
//   ICustomTaskPaneConsumer  — Custom task pane support via CTPFactoryAvailable
//
// Template parameters are the raw COM interface types (e.g. IRibbonExtensibility),
// NOT the event policy types (e.g. com::GetCustomUI).
// ---------------------------------------------------------------------------

template<typename... Interfaces>
class AddIn : public detail::AddInBase
{
public:
    AddIn(const char* guid, const wchar_t* progId,
          const wchar_t* friendlyName, const wchar_t* description)
        : AddInBase(guid, progId, friendlyName, description)
    {
        // Register the type-erased factory that creates
        // AddInServer<Interfaces...> for DllGetClassObject.
        detail::g_createServer = [](REFIID riid, void** ppv) -> HRESULT {
            auto* obj = new(std::nothrow) ::AddInServer<Interfaces...>();
            if (!obj) return E_OUTOFMEMORY;
            HRESULT hr = obj->QueryInterface(riid, ppv);
            obj->Release();
            return hr;
        };
    }

    // --- Event handler registration ----------------------------------------

    // Register an event/query handler.  TEvent is the event policy type
    // (e.g. com::Connection, com::GetCustomUI).  The callback signature
    // must match TEvent::Callback.
    //
    // Returns a dummy Reg so it can be used as:
    //   static const auto rN = s_addIn.on<com::Connection>([...]{...});
    //
    // Compile-time check: if the event requires an optional interface
    // (e.g. GetCustomUI requires IRibbonExtensibility), a static_assert
    // fires when that interface is not in the template parameter list.
    template<typename TEvent>
    detail::Reg on(typename TEvent::Callback cb) const;

    // --- Named dispatch callback registration ------------------------------

    // Register a named IDispatch callback (e.g. ribbon button callbacks).
    //   static const auto rN = s_addIn.dispatch<"OnButtonClicked">([...]{...});
    template<fixstr::fixed_string Name>
    detail::Reg dispatch(std::function<HRESULT(DISPPARAMS*, VARIANT*)> cb) const;
};

// Convenience accessor — returns the non-template base.
inline const detail::AddInBase& addIn()
{
    return *detail::g_addIn;
}

} // namespace com

