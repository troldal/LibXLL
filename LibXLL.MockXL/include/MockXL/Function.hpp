/**
 * @file Function.hpp
 * @brief Type-safe wrapper around an XLL exported function pointer of arbitrary arity.
 *
 * XLL exported functions all share the same return type (`xll::Any*`) and calling
 * convention (`PASCAL` / `__stdcall` on Windows, plain on Linux), but differ in the
 * number of `xll::Any*` parameters.  This header provides the complete machinery
 * needed to store, identify, and invoke such a function pointer in a type-safe way:
 *
 * | Name                 | Purpose                                                        |
 * |----------------------|----------------------------------------------------------------|
 * | `XlArgT<I>`          | Maps any index `I` to the uniform parameter type `xll::Any*`. |
 * | `XllFnN<N>`          | Typed function-pointer type for arity `N`.                     |
 * | `XllFn`              | `std::variant` of `XllFnN<0>` … `XllFnN<MaxXllArity>`.        |
 * | `function_arity_v<F>`| Compile-time arity of a typed function-pointer type.           |
 * | `Function`           | Holds an `XllFn` and invokes it with a `std::vector<xll::Any>`.|
 * | `make_function()`    | Casts a `void*` to the correct `XllFnN<N>` and wraps it.      |
 *
 * @note On x86-64 (Windows and Linux) all calling conventions collapse to a single
 *       ABI, so the lambda-decay trick used throughout this file produces types that
 *       are binary-compatible with `PASCAL` (`__stdcall`).  This would **not** hold
 *       on 32-bit x86 Windows, where `__stdcall` and `__cdecl` differ.
 */
#pragma once

#include "Types/Any.hpp"
#include "Types/Nil.hpp"
#include <array>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <xlcall.hpp>

// Bring in the PASCAL calling-convention macro where needed.
#ifdef _WIN32
#    ifndef PASCAL
#        define PASCAL __stdcall
#    endif
#else
#    ifndef PASCAL
#        define PASCAL
#    endif
#endif

namespace MockXL
{

    using FAutoFree = std::function<void(const xll::Any*)>;

    // ----------------------------------------------------------------------------
    // XlArgT
    // ----------------------------------------------------------------------------

    /**
     * @brief Uniform parameter type for all XLL function-pointer positions.
     *
     * Every argument slot in an XLL exported function has the same type regardless
     * of its position, so this alias simply maps any index `I` to `xll::Any*`.
     * The index parameter exists solely to allow it to be used in a parameter pack
     * expansion such as `XlArgT<Is>...`.
     *
     * `xll::Any` inherits from `XLOPER12` with no added data members, so `xll::Any*`
     * and `LPXLOPER12` are layout-identical and can be used interchangeably at the
     * binary level.
     *
     * @tparam I Parameter position (unused at runtime; drives pack expansion only).
     */
    template<std::size_t I>
    using XlArgT = const xll::Any*;

    // ----------------------------------------------------------------------------
    // XllFnN
    // ----------------------------------------------------------------------------

    /**
     * @brief Helper that builds a typed function-pointer type from an index sequence.
     *
     * The primary template is left undefined; only the partial specialisation on
     * `std::index_sequence<Is...>` is used.
     *
     * @tparam Seq An `std::index_sequence` whose length determines the arity.
     */
    template<typename Seq>
    struct XllFnType;

    /**
     * @brief Partial specialisation that produces the actual function-pointer type.
     *
     * The unary `+` applied to a captureless lambda forces it to decay to a plain
     * function pointer.  On x86-64 all calling conventions are identical, so the
     * decayed type is binary-compatible with `PASCAL` (`__stdcall`).
     *
     * @tparam Is Index pack; `sizeof...(Is)` is the arity of the resulting type.
     */
    template<std::size_t... Is>
    struct XllFnType<std::index_sequence<Is...>>
    {
        using type = decltype(+[](XlArgT<Is>...) -> xll::Any* { return nullptr; });    ///< Typed function-pointer type for arity `sizeof...(Is)`.
    };

    /**
     * @brief Typed function-pointer type for an XLL exported function of arity `N`.
     *
     * Examples:
     * @code
     * XllFnN<0>  ->  xll::Any*(*)()
     * XllFnN<1>  ->  xll::Any*(*)(xll::Any*)
     * XllFnN<2>  ->  xll::Any*(*)(xll::Any*, xll::Any*)
     * @endcode
     *
     * @tparam N Number of `xll::Any*` parameters.
     */
    template<std::size_t N>
    using XllFnN = XllFnType<std::make_index_sequence<N>>::type;

    // ----------------------------------------------------------------------------
    // XllFn
    // ----------------------------------------------------------------------------

    /**
     * @brief Helper that builds `std::variant<XllFnN<0>, ..., XllFnN<MaxArity>>`.
     *
     * The primary template is left undefined; only the partial specialisation on
     * `std::index_sequence<Is...>` is used.
     *
     * @tparam Seq An `std::index_sequence<0, 1, ..., MaxArity>`.
     */
    template<typename Seq>
    struct XllFnVariantImpl;

    /** @brief Partial specialisation that expands the index sequence into the variant. */
    template<std::size_t... Is>
    struct XllFnVariantImpl<std::index_sequence<Is...>>
    {
        using type =
            std::variant<XllFnN<Is>...>;    ///< `std::variant` of all typed function-pointer types up to arity `sizeof...(Is) - 1`.
    };

    /// Maximum number of `xll::Any*` arguments that `Function` can dispatch.
    inline constexpr std::size_t MaxXllArity = 20;

    /**
     * @brief `std::variant` of typed XLL function-pointer types for arities 0 – `MaxXllArity`.
     *
     * The active index of the variant encodes the arity of the stored function pointer,
     * eliminating the need for a separate arity field or an untyped `void*`.
     */
    using XllFn = XllFnVariantImpl<std::make_index_sequence<MaxXllArity + 1>>::type;

    // ----------------------------------------------------------------------------
    // function_arity_v
    // ----------------------------------------------------------------------------

    /**
     * @brief Compile-time arity of a typed function-pointer type.
     *
     * An immediately-invoked generic lambda deduces the return type `R` and the
     * parameter pack `Args...` from the function-pointer type `F`, then returns
     * `sizeof...(Args)`.  `F{}` produces a null pointer of that type; it is value-
     * initialised and **never called** — it exists solely to trigger template-
     * argument deduction at compile time.
     *
     * @tparam F A typed function-pointer type, e.g. `XllFnN<3>`.
     *
     * Example:
     * @code
     * static_assert(function_arity_v<XllFnN<3>> == 3);
     * @endcode
     */
    template<typename F>
    inline constexpr std::size_t function_arity_v =
        []<typename R, typename... Args>(R (*)(Args...)) -> std::size_t { return sizeof...(Args); }(F {});

    // ----------------------------------------------------------------------------
    // Function
    // ----------------------------------------------------------------------------

    /**
     * @brief Type-safe, callable wrapper around an XLL exported function pointer.
     *
     * `Function` stores one `XllFn` variant, whose active index encodes the arity of
     * the held function pointer.  Calling `operator()` with a `std::vector<xll::Any>`
     * builds an internal pointer array (padding missing arguments with `xll::Nil`),
     * dispatches to the correctly-typed function pointer via `std::visit`, deep-copies
     * the result into an owning `xll::Any`, and optionally invokes `xlAutoFree12` on
     * the raw return value if the add-in set the `xlbitDLLFree` ownership flag.
     *
     * A default-constructed `Function` is in the invalid state: `operator bool()`
     * returns `false` and calling `operator()` is undefined behaviour.
     *
     * @see make_function()
     * @see XllFn
     */
    struct Function
    {
        XllFn fn;    ///< The stored typed function pointer; active index == arity.

        /**
         * @brief Returns the arity (number of parameters) of the stored function pointer.
         *
         * The arity is the active index of the `XllFn` variant, which is set when the
         * variant is constructed by `make_function()`.
         *
         * @return Number of `xll::Any*` parameters the stored function expects.
         */
        [[nodiscard]] std::size_t arity() const noexcept { return fn.index(); }

        /**
         * @brief Returns `true` if the stored function pointer is non-null.
         *
         * A default-constructed `Function` holds `XllFnN<0>{}` (a null pointer at
         * variant index 0) and therefore evaluates to `false`.
         */
        [[nodiscard]] explicit operator bool() const noexcept
        {
            return std::visit([](auto f) { return f != nullptr; }, fn);
        }

        /**
         * @brief Invokes the stored XLL function with the given arguments.
         *
         * Steps:
         * 1. Fills a fixed-size `xll::Any*` pointer array with pointers to each
         *    element of @p args, padding unused slots with a static `xll::Nil` sentinel.
         * 2. Dispatches via `std::visit` to the correctly-typed function pointer; the
         *    active variant index ensures exactly the right number of pointers are
         *    forwarded.
         * 3. Deep-copies the raw return value into an owning `xll::Any` via its copy
         *    constructor.
         * 4. If the add-in set `xlbitDLLFree` on the return value and @p autoFree is
         *    provided, calls `autoFree` so the add-in can release its memory.
         *
         * @param args     Argument values.  At most `MaxXllArity` elements are used;
         *                 any beyond that limit are silently ignored.
         * @param autoFree Optional `xlAutoFree12` callback; invoked when the return
         *                 value carries the `xlbitDLLFree` ownership flag.
         * @return An owning `xll::Any` containing the value returned by the function,
         *         or `xll::Nil` if the function returned a null pointer.
         */
        [[nodiscard]] xll::Any operator()(const std::vector<xll::Any>& args, const FAutoFree& autoFree = {}) const
        {
            // Build a fixed-size xll::Any* pointer array, padding with Nil.
            // The variant's active index encodes the arity, so exactly the right
            // number of pointers are passed to the underlying function pointer.
            static xll::Any                        s_nil {};
            std::array<xll::Any*, MaxXllArity + 1> p {};
            p.fill(&s_nil);
            const auto n = std::min(args.size(), MaxXllArity + 1);
            for (std::size_t i = 0; i < n; ++i) p[i] = const_cast<xll::Any*>(&args[i]);

            const LPXLOPER12 ret = std::visit(
                [&]<typename TFunc>(TFunc f) -> LPXLOPER12 {
                    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                        return f(p[Is]...);
                    }(std::make_index_sequence<function_arity_v<TFunc>> {});
                },
                fn);

            if (!ret) return xll::Nil {};
            auto result = xll::Any {*ret};
            if ((ret->xltype & xlbitDLLFree) && autoFree) autoFree(reinterpret_cast<xll::Any*>(ret));
            return result;
        }
    };

    // ----------------------------------------------------------------------------
    // make_function
    // ----------------------------------------------------------------------------

    /**
     * @brief Factory that casts a raw `void*` function pointer to the correct
     *        `XllFnN<arity>` type and wraps it in a `Function`.
     *
     * A compile-time dispatch table of `MaxXllArity + 1` factory lambdas is built
     * once (as a `static constexpr` array) on the first call.  Each entry casts
     * @p fp to `XllFnN<N>` and stores it in a `Function`.  Subsequent calls index
     * directly into the table at O(1) cost.
     *
     * @param fp     Raw function pointer obtained from `dlsym` / `GetProcAddress`.
     * @param arity  Number of `xll::Any*` parameters the function expects.
     * @return A `Function` wrapping the correctly-typed function pointer.
     * @throws std::out_of_range if `arity` is negative or greater than `MaxXllArity`.
     */
    inline Function make_function(void* fp, const int arity)
    {
        if (arity < 0 || static_cast<std::size_t>(arity) > MaxXllArity)
            throw std::out_of_range("[MockXL] unsupported arity: " + std::to_string(arity));

        using Factory                                               = Function (*)(void*);
        static constexpr std::array<Factory, MaxXllArity + 1> table = [] {
            std::array<Factory, MaxXllArity + 1> t {};
            [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                ((t[Is] = [](void* p) -> Function { return Function { reinterpret_cast<XllFnN<Is>>(p) }; }), ...);
            }(std::make_index_sequence<MaxXllArity + 1> {});
            return t;
        }();

        return table[static_cast<std::size_t>(arity)](fp);
    }

}    // namespace MockXL
