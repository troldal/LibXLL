#pragma once

// ============================================================================
// MockXL::XllCaller
// ============================================================================
//
// Type-safe wrapper around an XLL function pointer of arbitrary arity.
//
// XLL exported functions all share the same return type (LPXLOPER12) and
// calling convention (PASCAL/__stdcall on Windows, plain on Linux), but
// differ in the number of LPXLOPER12 parameters.  This header provides:
//
//   XllFnN<N>        — the typed function-pointer type for arity N
//   XllFn            — std::variant<XllFnN<0>, ..., XllFnN<MaxXllArity>>
//   function_arity   — trait to read the arity out of a typed fn-ptr type
//   XllCaller        — holds an XllFn and invokes it with a flat pointer array
//   make_xll_caller  — factory: casts a void* to XllFnN<arity> and wraps it
//
// The variant's active index encodes the arity, so no separate integer field
// or raw void* is required.

#include <array>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <xlcall.hpp>

// Bring in the PASCAL calling-convention macro where needed.
#ifdef _WIN32
#  ifndef PASCAL
#    define PASCAL __stdcall
#  endif
#else
#  ifndef PASCAL
#    define PASCAL
#  endif
#endif

namespace MockXL {

// ----------------------------------------------------------------------------
// XlArgT — always LPXLOPER12; the index just drives parameter-pack length.
// ----------------------------------------------------------------------------
template<std::size_t I>
using XlArgT = LPXLOPER12;

// ----------------------------------------------------------------------------
// XllFnN<N> — typed function-pointer type for arity N
//   XllFnN<0>  ->  LPXLOPER12 (PASCAL*)()
//   XllFnN<2>  ->  LPXLOPER12 (PASCAL*)(LPXLOPER12, LPXLOPER12)  etc.
// ----------------------------------------------------------------------------
template<typename Seq>
struct XllFnType;

template<std::size_t... Is>
struct XllFnType<std::index_sequence<Is...>> {
    using type = LPXLOPER12(PASCAL*)(XlArgT<Is>...);
};

template<std::size_t N>
using XllFnN = typename XllFnType<std::make_index_sequence<N>>::type;

// ----------------------------------------------------------------------------
// XllFn — std::variant<XllFnN<0>, XllFnN<1>, ..., XllFnN<MaxXllArity>>
// ----------------------------------------------------------------------------
template<typename Seq>
struct XllFnVariantImpl;

template<std::size_t... Is>
struct XllFnVariantImpl<std::index_sequence<Is...>> {
    using type = std::variant<XllFnN<Is>...>;
};

inline constexpr std::size_t MaxXllArity = 20;
using XllFn = typename XllFnVariantImpl<std::make_index_sequence<MaxXllArity + 1>>::type;

// ----------------------------------------------------------------------------
// function_arity — extracts the arity from a typed function-pointer type
// ----------------------------------------------------------------------------
template<typename>
struct function_arity;

template<typename R, typename... Args>
struct function_arity<R(PASCAL*)(Args...)>
    : std::integral_constant<std::size_t, sizeof...(Args)> {};

template<typename F>
inline constexpr std::size_t function_arity_v = function_arity<F>::value;

// ----------------------------------------------------------------------------
// XllCaller — stores a typed XLL function pointer and invokes it
// ----------------------------------------------------------------------------

/**
 * @brief Stores a typed XLL function pointer and invokes it with a flat
 * LPXLOPER12 array.  The variant's active index encodes the arity, so no
 * separate arity field or raw void* is needed.
 */
struct Function {
    XllFn fn;

    [[nodiscard]] std::size_t arity() const noexcept { return fn.index(); }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        // A default-constructed variant holds index 0 with a null pointer.
        return std::visit([](auto f) { return f != nullptr; }, fn);
    }

    LPXLOPER12 operator()(LPXLOPER12* p) const
    {
        return std::visit(
            [&](auto f) -> LPXLOPER12 {
                return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                    return f(p[Is]...);
                }(std::make_index_sequence<function_arity_v<decltype(f)>>{});
            },
            fn);
    }
};

// ----------------------------------------------------------------------------
// make_xll_caller — factory: cast void* to XllFnN<arity> and wrap in XllCaller
// ----------------------------------------------------------------------------

/**
 * @brief Builds an XllCaller for a void* function pointer of the given arity.
 *
 * The factory table is built once at compile time; each entry casts @p fp to
 * the correct XllFnN<N> and stores it in the variant.
 *
 * @throws std::out_of_range if arity > MaxXllArity.
 */
inline Function make_function(void* fp, int arity)
{
    if (arity < 0 || static_cast<std::size_t>(arity) > MaxXllArity)
        throw std::out_of_range("[MockXL] unsupported arity: " + std::to_string(arity));

    using Factory = Function(*)(void*);
    static constexpr std::array<Factory, MaxXllArity + 1> table = [] {
        std::array<Factory, MaxXllArity + 1> t{};
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            ((t[Is] = [](void* p) -> Function {
                return Function{ reinterpret_cast<XllFnN<Is>>(p) };
            }), ...);
        }(std::make_index_sequence<MaxXllArity + 1>{});
        return t;
    }();

    return table[static_cast<std::size_t>(arity)](fp);
}

} // namespace MockXL

