#pragma once

#include "Interfaces.hpp"
#include "String.hpp"
#include <functional>
#include <string>
#include <vector>

namespace com {

// ---------------------------------------------------------------------------
// Event policy types — each defines the callback signature for one COM event.
// ---------------------------------------------------------------------------

struct Connection {
    using Callback = std::function<
        void(IDispatch* application, ext_ConnectMode connectMode,
             IDispatch* addInInst,   SAFEARRAY**     custom)>;
};

struct Disconnection {
    using Callback = std::function<
        void(ext_DisconnectMode removeMode, SAFEARRAY** custom)>;
};

struct AddInsUpdate    { using Callback = std::function<void(SAFEARRAY** custom)>; };
struct StartupComplete { using Callback = std::function<void(SAFEARRAY** custom)>; };
struct BeginShutdown   { using Callback = std::function<void(SAFEARRAY** custom)>; };

// Event fired when Excel hands over the ICTPFactory for custom task panes.
// The factory is passed as IDispatch* to avoid importing the Office type library.
struct CTPFactory {
    using Callback = std::function<void(IDispatch* factory)>;
};

// Query-style event — the callback receives the RibbonID and returns the
// RibbonX XML as a com::String.  Returning an empty com::String defers to
// the next handler (or to the built-in ribbon.xml resource).
struct GetCustomUI {
    using Callback   = std::function<com::String(const com::String& ribbonId)>;
    using ReturnType = com::String;
};

// ---------------------------------------------------------------------------
// EventHandler<TEvent>
//
// Wraps a single callback for the given event.  All registered callbacks are
// stored in a per-event Meyers-singleton vector so they survive across Connect
// instances and are safe from static-init-order issues.
//
// Usage:
//   auto onConnection = com::OnConnection(
//       [](IDispatch* app, ext_ConnectMode mode, IDispatch*, SAFEARRAY**) { ... });
//   XLL_COM_REGISTER(onConnection);
// ---------------------------------------------------------------------------

template<typename TEvent>
class EventHandler
{
public:
    using Callback = typename TEvent::Callback;

    explicit EventHandler(Callback cb) : m_callback(std::move(cb)) {}

    // Called by Registrar (via XLL_COM_REGISTER) at static-init time.
    void Register() const { callbacks().push_back(m_callback); }

    // Called by Connect::<event> to fire all registered handlers.
    template<typename... Args>
    static void Execute(Args&&... args)
    {
        for (auto& cb : callbacks())
            cb(std::forward<Args>(args)...);
    }

private:
    Callback m_callback;

    static std::vector<Callback>& callbacks()
    {
        static std::vector<Callback> s_list;
        return s_list;
    }
};

// ---------------------------------------------------------------------------
// QueryHandler<TEvent>
//
// Like EventHandler, but for events that produce a return value.
// Execute calls each registered callback in registration order and returns
// the first non-empty result.  Returning an empty ReturnType defers to the
// next handler (or to the built-in default in Connect).
//
// TEvent must define both Callback and ReturnType.
// ---------------------------------------------------------------------------

template<typename TEvent>
class QueryHandler
{
public:
    using Callback   = typename TEvent::Callback;
    using ReturnType = typename TEvent::ReturnType;

    explicit QueryHandler(Callback cb) : m_callback(std::move(cb)) {}

    // Called by Registrar (via XLL_COM_REGISTER) at static-init time.
    void Register() const { callbacks().push_back(m_callback); }

    // Calls each registered callback; returns the first non-empty result,
    // or a default-constructed ReturnType if no handler provides one.
    template<typename... Args>
    static ReturnType Execute(Args&&... args)
    {
        for (auto& cb : callbacks())
        {
            ReturnType result = cb(std::forward<Args>(args)...);
            if (!result.empty()) return result;
        }
        return {};
    }

private:
    Callback m_callback;

    static std::vector<Callback>& callbacks()
    {
        static std::vector<Callback> s_list;
        return s_list;
    }
};

// ---------------------------------------------------------------------------
// Named aliases — mirrors the xll::OnOpen / xll::OnClose naming convention.
// ---------------------------------------------------------------------------

using OnConnection      = EventHandler<Connection>;
using OnDisconnection   = EventHandler<Disconnection>;
using OnAddInsUpdate    = EventHandler<AddInsUpdate>;
using OnStartupComplete = EventHandler<StartupComplete>;
using OnBeginShutdown   = EventHandler<BeginShutdown>;
using OnGetCustomUI     = QueryHandler<GetCustomUI>;
using OnCTPFactoryAvailable = EventHandler<CTPFactory>;

// ---------------------------------------------------------------------------
// Registrar — RAII wrapper instantiated by XLL_COM_REGISTER.
// Works with both EventHandler and QueryHandler (both expose .Register()).
// ---------------------------------------------------------------------------

template<typename THandler>
struct Registrar
{
    explicit Registrar(THandler& h) { h.Register(); }
};

}  // namespace com
