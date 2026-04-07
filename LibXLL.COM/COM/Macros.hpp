#pragma once

// ---------------------------------------------------------------------------
// XLL_COM_EVENT
//
// Convenience storage-class specifier for event and dispatch handler
// registrations at namespace scope.  Because on<>() and dispatch<>() must
// appear as part of a declaration (bare expression statements are illegal
// outside a function), every registration needs a variable to bind to.
// This macro makes that intent explicit and searchable:
//
//   XLL_COM_EVENT r_connection = s_addin.on<com::Connection>([...]{...});
//   XLL_COM_EVENT r_customUI   = s_addin.on<com::GetCustomUI>([...]{...});
//   XLL_COM_EVENT r_clicked    = s_addin.dispatch<"OnButtonClicked">([...]{...});
// ---------------------------------------------------------------------------

#define XLL_COM_EVENT static const auto

// ---------------------------------------------------------------------------
// XLL_COM_REGISTER(handler)
//
// Legacy macro — registers a com::EventHandler, com::QueryHandler, or
// com::DispatchCallback at static-init time by constructing a
// com::Registrar<T> in namespace scope.
//
// The preferred API is now AddIn::on<>() and AddIn::dispatch<>(), which
// provide compile-time interface/event validation.  XLL_COM_REGISTER
// remains supported for backward compatibility.
//
// Examples:
//   auto onConnection = com::OnConnection([](IDispatch* app, ...) { ... });
//   XLL_COM_REGISTER(onConnection);
//
//   auto onClick = com::DispatchCallback<"OnButtonClicked">(
//       [](DISPPARAMS*, VARIANT*) -> HRESULT { ... });
//   XLL_COM_REGISTER(onClick);
// ---------------------------------------------------------------------------

#include "ComAuto.hpp"
#include "DispatchCallback.hpp"
#include "Server.hpp"

#define COM_CONCAT2(a, b) a##b
#define COM_CONCAT(a, b)  COM_CONCAT2(a, b)

#define XLL_COM_REGISTER(handler)                               \
    static com::Registrar<decltype(handler)>                    \
        COM_CONCAT(s_com_reg_, __COUNTER__) { handler }
