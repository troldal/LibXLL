#pragma once

#include "Interfaces.hpp"
#include "String.hpp"
#include "DispatchRegistry.hpp"
#include "../AddIn.hpp"
#include <fixed_string.hpp>
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
// RequiredInterface specialisations — map event policy types to the COM
// interface that must be listed in AddIn<...> for the event to be usable.
// ---------------------------------------------------------------------------

namespace detail {
    template<> struct RequiredInterface<GetCustomUI> { using type = IRibbonExtensibility; };
    template<> struct RequiredInterface<CTPFactory>  { using type = ICustomTaskPaneConsumer; };

    // Forward-declare HandlerPusher so EventHandler/QueryHandler can befriend it.
    template<typename TEvent, typename = void> struct HandlerPusher;
} // namespace detail

// ---------------------------------------------------------------------------
// EventHandler<TEvent>
//
// Wraps a single callback for the given event.  All registered callbacks are
// stored in a per-event Meyers-singleton vector so they survive across
// AddInServer instances and are safe from static-init-order issues.
//
// Usage (new API — preferred):
//   static const auto r = s_addIn.on<com::Connection>(
//       [](IDispatch* app, ext_ConnectMode mode, IDispatch*, SAFEARRAY**) { ... });
//
// Usage (legacy API — still supported):
//   auto onConnection = com::OnConnection([...]{...});
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

    // Called by AddInServer to fire all registered handlers.
    template<typename... Args>
    static void Execute(Args&&... args)
    {
        for (auto& cb : callbacks())
            cb(std::forward<Args>(args)...);
    }

    // Returns true if at least one handler is registered for this event.
    static bool hasHandler() { return !callbacks().empty(); }

private:
    Callback m_callback;

    static std::vector<Callback>& callbacks()
    {
        static std::vector<Callback> s_list;
        return s_list;
    }

    // Allow AddIn::on<>() to push via HandlerPusher.
    template<typename... Interfaces>
    friend class AddIn;
    template<typename, typename> friend struct detail::HandlerPusher;
};

// ---------------------------------------------------------------------------
// QueryHandler<TEvent>
//
// Like EventHandler, but for events that produce a return value.
// Execute calls each registered callback in registration order and returns
// the first non-empty result.  Returning an empty ReturnType defers to the
// next handler (or to the built-in default in AddInServer).
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

    // Returns true if at least one handler is registered for this event.
    static bool hasHandler() { return !callbacks().empty(); }

private:
    Callback m_callback;

    static std::vector<Callback>& callbacks()
    {
        static std::vector<Callback> s_list;
        return s_list;
    }

    // Allow AddIn::on<>() to push via HandlerPusher.
    template<typename... Interfaces>
    friend class AddIn;
    template<typename, typename> friend struct detail::HandlerPusher;
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

// ---------------------------------------------------------------------------
// Helper: detect whether TEvent uses EventHandler or QueryHandler,
// and push a callback into the appropriate Meyers-singleton vector.
// ---------------------------------------------------------------------------

namespace detail {

    // Primary: TEvent has only Callback → EventHandler (fire-and-forget).
    template<typename TEvent, typename>
    struct HandlerPusher
    {
        static void push(typename TEvent::Callback cb)
        {
            EventHandler<TEvent>::callbacks().push_back(std::move(cb));
        }
    };

    // Specialisation: TEvent also has ReturnType → QueryHandler.
    template<typename TEvent>
    struct HandlerPusher<TEvent, std::void_t<typename TEvent::ReturnType>>
    {
        static void push(typename TEvent::Callback cb)
        {
            QueryHandler<TEvent>::callbacks().push_back(std::move(cb));
        }
    };

} // namespace detail

// ---------------------------------------------------------------------------
// AddIn<Interfaces...>::on<TEvent>() implementation
// ---------------------------------------------------------------------------

template<typename... Interfaces>
template<typename TEvent>
detail::Reg AddIn<Interfaces...>::on(typename TEvent::Callback cb) const
{
    // Compile-time check: if the event requires an optional interface,
    // verify that it is present in the Interfaces... pack.
    using Required = typename detail::RequiredInterface<TEvent>::type;
    if constexpr (!std::is_void_v<Required>) {
        static_assert((std::is_same_v<Interfaces, Required> || ...),
            "This event requires a COM interface not listed in AddIn<...>.  "
            "Add the required interface to the AddIn template parameter list.");
    }
    detail::HandlerPusher<TEvent>::push(std::move(cb));
    return {};
}

// ---------------------------------------------------------------------------
// AddIn<Interfaces...>::dispatch<Name>() implementation
// ---------------------------------------------------------------------------

template<typename... Interfaces>
template<fixstr::fixed_string Name>
detail::Reg AddIn<Interfaces...>::dispatch(
    std::function<HRESULT(DISPPARAMS*, VARIANT*)> cb) const
{
    constexpr std::string_view sv(Name.data(), Name.size());
    std::wstring wideName(sv.begin(), sv.end());
    DispatchRegistry::instance().add(wideName, std::move(cb));
    return {};
}

}  // namespace com
