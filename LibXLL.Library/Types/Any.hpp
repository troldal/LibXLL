/**
 * @file Any.hpp
 * @brief Type-safe, XLOPER12-compatible container for any Excel value.
 *
 * @section overview Overview
 *
 * `xll::Any` is a type-erased container that can hold any value representable
 * by the Excel C API's `XLOPER12` union.  It is modelled after `std::any` but
 * operates entirely within the XLOPER12 binary layout: it inherits from
 * `XLOPER12` directly and adds no data members, so its address can be passed
 * to and from Excel C API functions without conversion.
 *
 * The companion free function template `xll::cast<TTarget>(any)` retrieves
 * the stored value with a type check.  The return type — either
 * `xll::Optional<TTarget>` or `xll::Expected<TTarget, xll::Error>` — is
 * governed by a policy template parameter on `xll::Any`.
 *
 * @section policies Cast-result policies
 *
 * Two policies are provided:
 *
 * | Policy tag              | `cast` return type                       |
 * |-------------------------|------------------------------------------|
 * | `xll::ExpectedPolicy`   | `xll::Expected<TTarget, xll::Error>`     |
 * | `xll::OptionalPolicy`   | `xll::Optional<TTarget>`                 |
 *
 * `ExpectedPolicy` is the default.  When the stored xltype does not match
 * `TTarget`, `cast` returns:
 * - `xll::Unexpected(xll::ErrValue)` for `ExpectedPolicy`
 * - `xll::None`                       for `OptionalPolicy`
 *
 * Example:
 * @code
 * // Default (Expected) policy
 * xll::Any<> any = xll::Number(3.14);
 * auto r1 = xll::cast<xll::Number>(any);   // Expected<Number, Error> = 3.14
 * auto r2 = xll::cast<xll::String>(any);   // Expected<String, Error> = Unexpected(ErrValue)
 *
 * // Optional policy
 * xll::Any<xll::OptionalPolicy> any2 = xll::String("hello");
 * auto r3 = xll::cast<xll::String>(any2);  // Optional<String> = "hello"
 * auto r4 = xll::cast<xll::Number>(any2);  // Optional<Number> = None
 * @endcode
 *
 * @section type_punning Type punning and memory safety
 *
 * `xll::Any` stores a value by **copying the XLOPER12 base** of the source
 * object.  For types that own heap memory (e.g. `xll::String`, `xll::Array`)
 * this means a *shallow* copy — the stored pointer aliases the original
 * allocation.  The copy constructor and copy-assignment operator perform a
 * deep copy when possible (delegating to the source type's own copy logic via
 * `std::construct_at`).
 *
 * `xll::cast` uses `std::launder` after `reinterpret_cast`, exactly as
 * `xll::Expected` and `xll::Optional` do, to ensure valid pointer provenance.
 *
 * @note `xll::Any` does **not** manage the lifetime of heap-allocated payloads
 *       (String buffers, Array element arrays).  It is the caller's
 *       responsibility to ensure that the source object outlives any `Any`
 *       that was constructed from a shallow copy.  Use the dedicated
 *       constructors that accept owning xll types to obtain a deep copy.
 *
 * @author Kenneth Troldal Balslev
 * @date 24/02/2026
 */

#pragma once

#include "../ExcelSDK/xlcall.hpp"
#include "../Utils/Concepts.hpp"
#include "Array.hpp"
#include "Bool.hpp"
#include "Error.hpp"
#include "Expected.hpp"
#include "Int.hpp"
#include "Missing.hpp"
#include "Nil.hpp"
#include "Number.hpp"
#include "Optional.hpp"
#include "String.hpp"
#include "Variant.hpp"
#include <type_traits>

namespace xll
{
    // =========================================================================
    // Policy tags
    // =========================================================================

    /**
     * @brief Policy tag: `xll::cast` returns `xll::Expected<TTarget, xll::Error>`.
     *
     * On type mismatch the returned Expected is in the error state and contains
     * `xll::ErrValue` (#VALUE!).  This is the default policy.
     */
    struct ExpectedPolicy {};

    /**
     * @brief Policy tag: `xll::cast` returns `xll::Optional<TTarget>`.
     *
     * On type mismatch the returned Optional is in the disengaged state
     * (`xll::None`).
     */
    struct OptionalPolicy {};

    // =========================================================================
    // xll::Any
    // =========================================================================

    /**
     * @brief Type-erased Excel value container with configurable cast-result policy.
     *
     * `Any<TPolicy>` can hold any value that fits in an `XLOPER12` union.
     * It inherits directly from `XLOPER12` and adds no data members, preserving
     * binary compatibility with the Excel C API.
     *
     * @tparam TPolicy  Controls the return type of `xll::cast`.
     *                  Use `xll::ExpectedPolicy` (default) to get
     *                  `Expected<TTarget, Error>`, or `xll::OptionalPolicy`
     *                  to get `Optional<TTarget>`.
     */
    template<typename TPolicy = ExpectedPolicy>
        requires std::same_as<TPolicy, ExpectedPolicy> || std::same_as<TPolicy, OptionalPolicy>
    class Any final : public XLOPER12
    {

    public:
        using policy_type = TPolicy;

        // ------------------------------------------------------------------
        // Default constructor — stores xll::Nil (xltypeNil)
        // ------------------------------------------------------------------

        /**
         * @brief Constructs an Any holding `xll::Nil` (the "empty" XLOPER12 value).
         *
         * @post xltype == xltypeNil
         */
        constexpr Any() noexcept : XLOPER12()
        {
            std::construct_at(reinterpret_cast<xll::Nil*>(this));
        }

        // ------------------------------------------------------------------
        // Construct from any xll type (deep copy)
        // ------------------------------------------------------------------

        /**
         * @brief Constructs an Any by deep-copying any xll type.
         *
         * The source type @p TValue must satisfy `is_xll_type`.  Its copy
         * constructor is invoked via `std::construct_at` so that heap-owning
         * types (e.g. `xll::String`, `xll::Array<T>`) are deep-copied.
         *
         * @tparam TValue  Source xll type (deduced).
         * @param  value   The value to store.
         */
        template<typename TValue>
            requires is_xll_type<TValue> && (!std::same_as<std::remove_cvref_t<TValue>, Any>)
        constexpr Any(const TValue& value) : XLOPER12()    // NOLINT(google-explicit-constructor)
        {
            std::construct_at(reinterpret_cast<std::remove_cvref_t<TValue>*>(this), value);
        }

        template<typename TValue>
            requires is_xll_type<std::remove_cvref_t<TValue>> &&
                     (!std::same_as<std::remove_cvref_t<TValue>, Any>)
        constexpr Any(TValue&& value) noexcept : XLOPER12()    // NOLINT(google-explicit-constructor)
        {
            std::construct_at(reinterpret_cast<std::remove_cvref_t<TValue>*>(this), std::forward<TValue>(value));
        }

        // ------------------------------------------------------------------
        // Construct from raw XLOPER12 (shallow copy — use with care)
        // ------------------------------------------------------------------

        /**
         * @brief Constructs an Any from a raw `XLOPER12` (shallow copy).
         *
         * Copies the xltype and val union verbatim.  No deep copy is performed,
         * so the caller must ensure the source lifetime exceeds this object's.
         */
        constexpr explicit Any(const XLOPER12& raw) noexcept : XLOPER12(raw) {}

        // ------------------------------------------------------------------
        // Copy / move
        // ------------------------------------------------------------------

        /**
         * @brief Copy constructor — performs a type-aware deep copy.
         *
         * Inspects the stored xltype and invokes the copy constructor of the
         * corresponding xll type to ensure heap-owning payloads are deep-copied.
         */
        constexpr Any(const Any& other) : XLOPER12() { copy_from(other); }

        /**
         * @brief Move constructor — transfers ownership of heap payloads.
         */
        constexpr Any(Any&& other) noexcept : XLOPER12() { move_from(std::move(other)); }

        // ------------------------------------------------------------------
        // Destructor — type-aware cleanup
        // ------------------------------------------------------------------

        /**
         * @brief Destructor — destroys the currently held value.
         *
         * Dispatches to the correct destructor based on `xltype`.
         */
        constexpr ~Any() { destroy_current(); }

        // ------------------------------------------------------------------
        // Assignment operators
        // ------------------------------------------------------------------

        /**
         * @brief Copy assignment — copy-and-swap for exception safety.
         */
        constexpr Any& operator=(const Any& other)
        {
            Any temp(other);
            swap(temp);
            return *this;
        }

        /**
         * @brief Move assignment.
         */
        constexpr Any& operator=(Any&& other) noexcept
        {
            Any temp(std::move(other));
            swap(temp);
            return *this;
        }

        /**
         * @brief Assign from any xll type (copy).
         */
        template<typename TValue>
            requires is_xll_type<TValue> && (!std::same_as<std::remove_cvref_t<TValue>, Any>)
        constexpr Any& operator=(const TValue& value)
        {
            Any temp(value);
            swap(temp);
            return *this;
        }

        /**
         * @brief Assign from any xll type (move).
         */
        template<typename TValue>
            requires is_xll_type<std::remove_cvref_t<TValue>> &&
                     (!std::same_as<std::remove_cvref_t<TValue>, Any>)
        constexpr Any& operator=(TValue&& value) noexcept
        {
            Any temp(std::forward<TValue>(value));
            swap(temp);
            return *this;
        }

        // ------------------------------------------------------------------
        // Swap
        // ------------------------------------------------------------------

        /**
         * @brief Swaps two Any objects by swapping the underlying XLOPER12.
         *
         * Safe and efficient: because Any and all xll types have identical layout
         * to XLOPER12, swapping the base struct exchanges the complete object state.
         */
        constexpr void swap(Any& other) noexcept
        {
            using std::swap;
            swap(static_cast<XLOPER12&>(*this), static_cast<XLOPER12&>(other));
        }

        // ------------------------------------------------------------------
        // Type queries
        // ------------------------------------------------------------------

        /**
         * @brief Returns the raw `xltype` of the stored value (masked to remove
         *        Excel ownership flags).
         */
        [[nodiscard]]
        constexpr int type() const noexcept
        {
            return static_cast<int>(xltype & ~(xlbitDLLFree | xlbitXLFree));
        }

        /**
         * @brief Returns `true` when the stored type matches `TTarget::excel_type`.
         *
         * @tparam TTarget  The xll type to check against (must satisfy `is_xll_type`).
         */
        template<typename TTarget>
            requires is_xll_type<TTarget>
        [[nodiscard]]
        constexpr bool holds() const noexcept
        {
            return type() == static_cast<int>(TTarget::excel_type);
        }

        /**
         * @brief Returns `true` when the Any currently holds `xll::Nil`.
         */
        [[nodiscard]]
        constexpr bool empty() const noexcept { return type() == xltypeNil; }

    private:
        // ------------------------------------------------------------------
        // Internal helpers
        // ------------------------------------------------------------------

        /// Type-aware deep copy from another Any.
        constexpr void copy_from(const Any& src)
        {
            const int t = src.type();
            if (t == xltypeNum)
                std::construct_at(reinterpret_cast<xll::Number*>(this),
                    *std::launder(reinterpret_cast<const xll::Number*>(&src)));
            else if (t == xltypeStr)
                std::construct_at(reinterpret_cast<xll::String*>(this),
                    *std::launder(reinterpret_cast<const xll::String*>(&src)));
            else if (t == xltypeInt)
                std::construct_at(reinterpret_cast<xll::Int*>(this),
                    *std::launder(reinterpret_cast<const xll::Int*>(&src)));
            else if (t == xltypeBool)
                std::construct_at(reinterpret_cast<xll::Bool*>(this),
                    *std::launder(reinterpret_cast<const xll::Bool*>(&src)));
            else if (t == xltypeErr)
                std::construct_at(reinterpret_cast<xll::Error*>(this),
                    *std::launder(reinterpret_cast<const xll::Error*>(&src)));
            else if (t == xltypeMissing)
                std::construct_at(reinterpret_cast<xll::Missing*>(this));
            else    // xltypeNil or unknown — treat as Nil
                std::construct_at(reinterpret_cast<xll::Nil*>(this));
        }

        /// Type-aware move from another Any.
        constexpr void move_from(Any&& src) noexcept
        {
            const int t = src.type();
            if (t == xltypeNum)
                std::construct_at(reinterpret_cast<xll::Number*>(this),
                    std::move(*std::launder(reinterpret_cast<xll::Number*>(&src))));
            else if (t == xltypeStr)
                std::construct_at(reinterpret_cast<xll::String*>(this),
                    std::move(*std::launder(reinterpret_cast<xll::String*>(&src))));
            else if (t == xltypeInt)
                std::construct_at(reinterpret_cast<xll::Int*>(this),
                    std::move(*std::launder(reinterpret_cast<xll::Int*>(&src))));
            else if (t == xltypeBool)
                std::construct_at(reinterpret_cast<xll::Bool*>(this),
                    std::move(*std::launder(reinterpret_cast<xll::Bool*>(&src))));
            else if (t == xltypeErr)
                std::construct_at(reinterpret_cast<xll::Error*>(this),
                    std::move(*std::launder(reinterpret_cast<xll::Error*>(&src))));
            else if (t == xltypeMissing)
                std::construct_at(reinterpret_cast<xll::Missing*>(this));
            else
                std::construct_at(reinterpret_cast<xll::Nil*>(this));
        }

        /// Destroy the currently active object.
        constexpr void destroy_current() noexcept
        {
            const int t = type();
            if (t == xltypeNum)
                std::destroy_at(std::launder(reinterpret_cast<xll::Number*>(this)));
            else if (t == xltypeStr)
                std::destroy_at(std::launder(reinterpret_cast<xll::String*>(this)));
            else if (t == xltypeInt)
                std::destroy_at(std::launder(reinterpret_cast<xll::Int*>(this)));
            else if (t == xltypeBool)
                std::destroy_at(std::launder(reinterpret_cast<xll::Bool*>(this)));
            else if (t == xltypeErr)
                std::destroy_at(std::launder(reinterpret_cast<xll::Error*>(this)));
            else if (t == xltypeMissing)
                std::destroy_at(std::launder(reinterpret_cast<xll::Missing*>(this)));
            else    // xltypeNil or unknown
                std::destroy_at(std::launder(reinterpret_cast<xll::Nil*>(this)));
        }
    };

    // =========================================================================
    // Non-member swap
    // =========================================================================

    /**
     * @brief ADL-friendly non-member swap for xll::Any.
     */
    template<typename TPolicy>
    constexpr void swap(Any<TPolicy>& lhs, Any<TPolicy>& rhs) noexcept { lhs.swap(rhs); }

    // Verify that Any adds no data members beyond XLOPER12.
    static_assert(sizeof(Any<ExpectedPolicy>) == sizeof(XLOPER12),
        "Any<ExpectedPolicy> must not add data members");
    static_assert(sizeof(Any<OptionalPolicy>) == sizeof(XLOPER12),
        "Any<OptionalPolicy> must not add data members");

    // =========================================================================
    // xll::cast — type-safe retrieval
    // =========================================================================

    namespace impl
    {
        // Traits that map a policy tag to the return type of cast<TTarget>.
        template<typename TPolicy, typename TTarget>
        struct cast_result;

        template<typename TTarget>
        struct cast_result<ExpectedPolicy, TTarget> {
            using type = Expected<TTarget, xll::Error>;
        };

        template<typename TTarget>
        struct cast_result<OptionalPolicy, TTarget> {
            using type = Optional<TTarget>;
        };

        template<typename TPolicy, typename TTarget>
        using cast_result_t = typename cast_result<TPolicy, TTarget>::type;

        // ------------------------------------------------------------------
        // do_cast — core implementation, non-Array types
        // ------------------------------------------------------------------
        template<typename TTarget, typename TPolicy>
            requires is_xll_type<TTarget>
        cast_result_t<TPolicy, TTarget> do_cast(const Any<TPolicy>& any)
        {
            using Result = cast_result_t<TPolicy, TTarget>;

            const int stored = any.type();
            const int wanted = static_cast<int>(TTarget::excel_type);

            if (stored == wanted) {
                // Type matches — copy-construct TTarget from the stored XLOPER12.
                return Result(*std::launder(reinterpret_cast<const TTarget*>(&any)));
            }

            // Type mismatch — return empty / error depending on policy.
            if constexpr (std::same_as<TPolicy, ExpectedPolicy>)
                return Unexpected(xll::ErrValue);
            else
                return xll::None;
        }

    }    // namespace impl

    /**
     * @brief Retrieves the value stored in an `xll::Any` as `TTarget`.
     *
     * Checks whether the stored XLOPER12 type matches `TTarget::excel_type`.
     * On success, returns a copy of the value.  On failure, returns an error
     * or empty result according to the policy:
     *
     * - `ExpectedPolicy` → `Expected<TTarget, Error>` with `Unexpected(ErrValue)`
     * - `OptionalPolicy` → `Optional<TTarget>` with the disengaged state (`None`)
     *
     * @tparam TTarget  The xll type to cast to (must satisfy `is_xll_type`).
     * @tparam TPolicy  Deduced from the `Any` argument.
     * @param  any      The `Any` object to cast from.
     * @return          The cast result according to the policy.
     *
     * @code
     * xll::Any<> any = xll::Number(3.14);
     *
     * // Success
     * auto n = xll::cast<xll::Number>(any);
     * // n is Expected<Number, Error> containing 3.14
     *
     * // Failure
     * auto s = xll::cast<xll::String>(any);
     * // s is Expected<String, Error> containing Unexpected(ErrValue)
     *
     * // Optional policy
     * xll::Any<xll::OptionalPolicy> opt_any = xll::String("hi");
     * auto r = xll::cast<xll::String>(opt_any);
     * // r is Optional<String> containing "hi"
     * @endcode
     */
    template<typename TTarget, typename TPolicy>
        requires is_xll_type<TTarget>
    [[nodiscard]]
    auto cast(const Any<TPolicy>& any) -> impl::cast_result_t<TPolicy, TTarget>
    {
        return impl::do_cast<TTarget, TPolicy>(any);
    }

    /**
     * @brief Retrieves the value from a mutable `xll::Any`.
     *
     * Identical to the const overload; provided so that non-const `Any` objects
     * can be passed without an explicit `const_cast`.
     */
    template<typename TTarget, typename TPolicy>
        requires is_xll_type<TTarget>
    [[nodiscard]]
    auto cast(Any<TPolicy>& any) -> impl::cast_result_t<TPolicy, TTarget>
    {
        return impl::do_cast<TTarget, TPolicy>(any);
    }

    // =========================================================================
    // Convenience type aliases
    // =========================================================================

    /// `xll::Any` with `ExpectedPolicy` (default).
    using AnyExpected = Any<ExpectedPolicy>;

    /// `xll::Any` with `OptionalPolicy`.
    using AnyOptional = Any<OptionalPolicy>;

}    // namespace xll



