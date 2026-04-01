#pragma once

#include "DispatchRegistry.hpp"
#include <fixed_string.hpp>
#include <functional>
#include <string>

namespace com {

// ---------------------------------------------------------------------------
// DispatchCallback<fixstr::fixed_string Name>
//
// Associates a compile-time callback name (e.g. "OnRibbonLoad") with a
// runtime std::function<HRESULT(DISPPARAMS*, VARIANT*)>.
//
// At static-init time (via XLL_COM_REGISTER), Register() inserts the
// callback into the global DispatchRegistry under the widened name.
// Connect::GetIDsOfNames and Connect::Invoke then resolve everything
// through the registry — no hand-maintained if-chains needed.
//
// Usage:
//   auto onRibbonLoad = com::DispatchCallback<"OnRibbonLoad">(
//       [](DISPPARAMS* params, VARIANT*) -> HRESULT { ... });
//   XLL_COM_REGISTER(onRibbonLoad);
// ---------------------------------------------------------------------------

template<fixstr::fixed_string Name>
class DispatchCallback
{
public:
    using Callback = std::function<HRESULT(DISPPARAMS*, VARIANT*)>;

    explicit DispatchCallback(Callback cb) : m_callback(std::move(cb)) {}

    // Called by Registrar (via XLL_COM_REGISTER) at static-init time.
    void Register() const
    {
        DispatchRegistry::instance().add(wideName(), m_callback);
    }

    // The compile-time name as a string_view.
    static constexpr std::string_view name()
    {
        return std::string_view(Name.data(), Name.size());
    }

private:
    Callback m_callback;

    // Widen the ASCII callback name to std::wstring for the registry.
    static std::wstring wideName()
    {
        constexpr std::string_view sv(Name.data(), Name.size());
        return std::wstring(sv.begin(), sv.end());
    }
};

} // namespace com

