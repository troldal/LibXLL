#pragma once

// ---------------------------------------------------------------------------
// XLL_COM_REGISTER(handler)
//
// Registers a com::EventHandler, com::QueryHandler, or
// com::DispatchCallback at static-init time by constructing a
// com::Registrar<T> in namespace scope.  __COUNTER__ ensures a unique
// variable name even when the macro is used multiple times in the same TU.
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
#include "../COMServer.hpp"

#define COM_CONCAT2(a, b) a##b
#define COM_CONCAT(a, b)  COM_CONCAT2(a, b)

#define XLL_COM_REGISTER(handler)                               \
    static com::Registrar<decltype(handler)>                    \
        COM_CONCAT(s_com_reg_, __COUNTER__) { handler }         \



