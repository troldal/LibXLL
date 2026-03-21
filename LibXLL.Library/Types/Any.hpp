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
 * @section cast Cast result
 *
 * `xll::cast<TTarget>(any)` always returns `xll::Optional<TTarget>`.
 * On type mismatch the returned Optional is in the disengaged state (`xll::None`).
 *
 * Example:
 * @code
 * xll::Any any = xll::Number(3.14);
 * auto r1 = xll::cast<xll::Number>(any);   // Optional<Number> = 3.14
 * auto r2 = xll::cast<xll::String>(any);   // Optional<String> = None
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
    // Internal helpers
    // =========================================================================

    namespace impl
    {
        /// Primary: not a typed array.
        template<typename T>
        struct is_typed_array_impl : std::false_type {};

        /// Partial specialisation: Array<T> — detected as a typed array.
        /// We forward-declare the specialisation here; Any is defined below.
        template<typename TValue>
        struct is_typed_array_impl<Array<TValue>> : std::true_type {};

        // ------------------------------------------------------------------
        // accepted_xltypes — extracts all xltypes accepted by a Base<> type
        // ------------------------------------------------------------------

        /// Primary template: no match — empty list of accepted types.
        template<typename T>
        struct accepted_xltypes_impl
        {
            static constexpr bool contains(int) noexcept { return false; }
        };

        /// Partial specialisation matching Base<TDerived, XLType, OtherTypes...>.
        /// Captures the primary XLType and all OtherTypes in a single pack.
        template<typename TDerived, size_t XLType, size_t... OtherTypes>
        struct accepted_xltypes_impl<xll::impl::Base<TDerived, XLType, OtherTypes...>>
        {
            static constexpr bool contains(int xltype) noexcept
            {
                return ((xltype == static_cast<int>(XLType)) || ... ||
                        (xltype == static_cast<int>(OtherTypes)));
            }
        };

        /// Helper that recovers the concrete Base<TDerived, XLType, OtherTypes...>
        /// that T inherits from, without requiring access to any private member of T.
        /// The function is never called; only its return type is used via decltype.
        template<typename TDerived, size_t XLType, size_t... OtherTypes>
        auto deduce_base(const xll::impl::Base<TDerived, XLType, OtherTypes...>*) ->
            xll::impl::Base<TDerived, XLType, OtherTypes...>;

        /// The concrete Base<...> type that T inherits from.
        template<typename T>
        using base_of_t = decltype(impl::deduce_base(static_cast<const T*>(nullptr)));

        /// Detect whether T has a Base<> parent by checking if deduce_base matches.
        template<typename T>
        concept has_base_type = requires(const T* p) { impl::deduce_base(p); };

        /// Detect whether T is a Variant<...> — satisfies is_xll_type but does not
        /// inherit from impl::Base (so has_base_type is false).
        template<typename T>
        concept is_variant_type = is_xll_type<T> && !has_base_type<T>;

        /// Overload for plain Base<>-derived types (Number, String, Bool, etc.).
        template<typename T>
            requires has_base_type<T>
        constexpr bool xltype_convertible_to(int xltype) noexcept
        {
            return accepted_xltypes_impl<base_of_t<T>>::contains(xltype);
        }

        /// Overload for Variant<T,Ts...> types.
        /// excel_type is the OR of all constituent xltypes; test membership via bitmask.
        template<typename T>
            requires is_variant_type<T>
        constexpr bool xltype_convertible_to(int xltype) noexcept
        {
            return (static_cast<size_t>(xltype) & T::excel_type) != 0;
        }

        /// Bit mask for Excel ownership flags that must be stripped from xltype
        /// before any type comparison.
        static constexpr unsigned int xltype_ownership_bits =
            static_cast<unsigned int>(xlbitDLLFree) |
            static_cast<unsigned int>(xlbitXLFree);

    }    // namespace impl

    /// `true` when `T` is `Array<U>` for any `U`.
    /// Used by `holds()` to select the element-type-checking path.
    template<typename T>
    inline constexpr bool is_typed_array = impl::is_typed_array_impl<T>::value;

    // =========================================================================
    // xll::Any
    // =========================================================================

    /**
     * @brief Type-erased Excel value container.
     *
     * `Any` can hold any value that fits in an `XLOPER12` union.
     * It inherits directly from `XLOPER12` and adds no data members, preserving
     * binary compatibility with the Excel C API.
     *
     * `xll::cast<TTarget>(any)` retrieves the stored value as `xll::Optional<TTarget>`.
     * On type mismatch the returned Optional is in the disengaged state (`xll::None`).
     */
    class Any final : public XLOPER12
    {
    public:

        static constexpr size_t excel_type = xltypeNum
                                            | xltypeStr
                                            | xltypeBool
                                            | xltypeRef
                                            | xltypeErr
                                            | xltypeFlow
                                            | xltypeMulti
                                            | xltypeMissing
                                            | xltypeNil
                                            | xltypeSRef
                                            | xltypeInt;

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
            requires is_xll_type<std::remove_cvref_t<TValue>> && (!std::same_as<std::remove_cvref_t<TValue>, Any>)
        constexpr Any(TValue&& value) noexcept(std::is_nothrow_move_constructible_v<std::remove_cvref_t<TValue>>) : XLOPER12()    // NOLINT(google-explicit-constructor)
        {
            std::construct_at(reinterpret_cast<std::remove_cvref_t<TValue>*>(this), std::forward<TValue>(value));
        }

        // ------------------------------------------------------------------
        // Construct from raw XLOPER12 (type-aware deep copy)
        // ------------------------------------------------------------------

        /**
         * @brief Constructs an Any from a raw `XLOPER12` by performing a type-aware deep copy.
         *
         * The `XLOPER12` is reinterpreted as an `Any` (safe because `Any` inherits
         * from `XLOPER12` with no added members) and then `copy_from` is invoked,
         * which deep-copies heap-owning payloads (e.g. String buffers, Array element
         * arrays) in the same way as the copy constructor.
         *
         * Unknown or unsupported xltype values are treated as `xll::Nil`.
         */
        constexpr explicit Any(const XLOPER12& raw) : XLOPER12()
        {
            copy_from(*std::launder(reinterpret_cast<const Any*>(&raw)));
        }

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
         *
         * Destroys the current value, then transfers ownership from `other`.
         * After the call, `other` holds `xll::Nil`.
         * Self-assignment is safe.
         */
        constexpr Any& operator=(Any&& other) noexcept
        {
            if (this != &other) {
                destroy_current();
                move_from(std::move(other));
            }
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
            return static_cast<int>(static_cast<unsigned int>(xltype) & ~impl::xltype_ownership_bits);
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
        /// @pre *this must have no active object lifetime (i.e. called only from
        ///      a constructor on uninitialised storage).
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
            else if (t == xltypeMulti)
                std::construct_at(reinterpret_cast<xll::Array<Any>*>(this),
                    *std::launder(reinterpret_cast<const xll::Array<Any>*>(&src)));
            else if (t == xltypeMissing)
                std::construct_at(reinterpret_cast<xll::Missing*>(this));
            else    // xltypeNil or unknown — treat as Nil
                std::construct_at(reinterpret_cast<xll::Nil*>(this));
        }

        /// Type-aware move from another Any.
        /// @pre *this must have no active object lifetime (i.e. called only from
        ///      a constructor on uninitialised storage).
        /// @post src holds xll::Nil — its heap payload has been transferred to
        ///       *this, and src is reset so that its destructor is a no-op.
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
            else if (t == xltypeMulti)
                std::construct_at(reinterpret_cast<xll::Array<Any>*>(this),
                    std::move(*std::launder(reinterpret_cast<xll::Array<Any>*>(&src))));
            else if (t == xltypeMissing)
                std::construct_at(reinterpret_cast<xll::Missing*>(this));
            else
                std::construct_at(reinterpret_cast<xll::Nil*>(this));

            // Reset source to Nil so its destructor does not attempt to free
            // the heap allocation that has just been transferred to *this.
            // destroy_current() dispatches to the correct destructor for the
            // live type in src (e.g. ~String(), ~Array<Any>()) rather than
            // blindly calling ~Nil() regardless of what is actually stored.
            src.destroy_current();
            std::construct_at(reinterpret_cast<xll::Nil*>(&src));
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
            else if (t == xltypeMulti)
                std::destroy_at(std::launder(reinterpret_cast<xll::Array<Any>*>(this)));
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
    constexpr void swap(Any& lhs, Any& rhs) noexcept { lhs.swap(rhs); }

    // Verify that Any adds no data members beyond XLOPER12.
    static_assert(sizeof(Any) == sizeof(XLOPER12),
        "Any must not add data members");

    // =========================================================================
    // xll::holds — type queries as free functions
    // =========================================================================

    /**
     * @brief Returns `true` when `any` holds a value of type `TTarget`.
     *
     * For plain xll types (Number, String, Bool, etc.) this checks only the
     * xltype tag.
     *
     * For a fully specialised `xll::Array<T>` where `T` is not `Any`, this
     * verifies that **every element** in the stored array has an xltype that
     * is *convertible to* `T` — i.e. the element's xltype is the primary type
     * or any of the `OtherTypes` accepted by `T`'s `Base` class.  For example,
     * `holds<Array<Number>>(any)` returns `true` if all elements are
     * `xltypeNum`, `xltypeInt`, or `xltypeBool`, because `xll::Number` accepts
     * all three.
     *
     * @tparam TTarget  The xll type to check against (must satisfy `is_xll_type`).
     * @param  any      The `Any` object to inspect.
     *
     * @code
     * xll::Any any = xll::Number(3.14);
     * xll::holds<xll::Number>(any);   // true
     * xll::holds<xll::String>(any);   // false
     * @endcode
     */
    template<typename TTarget>
        requires is_xll_type<TTarget>
    [[nodiscard]]
    constexpr bool holds(const Any& any) noexcept
    {
        if constexpr (is_typed_array<TTarget>) {
            // A single scalar of the element type is treated as a 1×1 array.
            // (Not applicable for Array<Any> since Any has no excel_type.)
            if constexpr (!std::same_as<typename TTarget::value_type, Any>) {
                if (any.type() == static_cast<int>(TTarget::value_type::excel_type)) return true;
            }

            // Array<Any>: only check that xltype is xltypeMulti — Any accepts every element type.
            if (any.type() != xltypeMulti) return false;
            if constexpr (std::same_as<typename TTarget::value_type, Any>) {
                return true;
            }
            else {
                // Array<T>: additionally verify every element is convertible to T.
                const auto* elems = static_cast<const XLOPER12*>(any.val.array.lparray);
                const size_t n    = static_cast<size_t>(any.val.array.rows) *
                                    static_cast<size_t>(any.val.array.columns);
                for (size_t i = 0; i < n; ++i) {
                    const int elem_type = static_cast<int>(
                        elems[i].xltype & ~impl::xltype_ownership_bits);
                    if (!impl::xltype_convertible_to<typename TTarget::value_type>(elem_type))
                        return false;
                }
                return true;
            }
        }
        else if constexpr (impl::is_variant_type<TTarget>) {
            // Variant::excel_type is a composite bitmask — strip the xltypeMissing
            // sentinel and test whether any.type() is one of the declared member types.
            return impl::xltype_convertible_to<TTarget>(any.type());
        }
        else {
            return any.type() == static_cast<int>(TTarget::excel_type);
        }
    }

    /**
     * @brief Returns `true` when `any` holds an array of any element type.
     *
     * This overload accepts the unspecialised `xll::Array` template as a
     * template template argument.  It only checks that the stored xltype is
     * `xltypeMulti`, without inspecting element types.
     *
     * @code
     * xll::holds<xll::Array>(any);   // true iff any holds an array
     * @endcode
     */
    template<template<typename> class TArrayTemplate>
        requires std::same_as<TArrayTemplate<Any>, Array<Any>>
    [[nodiscard]]
    constexpr bool holds(const Any& any) noexcept
    {
        return any.type() == xltypeMulti;
    }

    // =========================================================================
    // xll::cast — type-safe retrieval
    // =========================================================================

    /**
     * @brief Retrieves a typed array value stored in an `xll::Any` as `Array<TValue>`.
     *
     * Handles two cases that the generic reinterpret-cast path cannot deliver correctly:
     *
     * **Case 1 — scalar held as TValue (1×1 array semantics)**
     * `holds<Array<TValue>>` returns `true` for a scalar whose `xltype` matches
     * `TValue::excel_type`, reflecting Excel's own rule that a single cell value is
     * a valid 1×1 array.  This overload constructs a proper `Array<TValue>` with
     * `rows=1, cols=1` using `Array`'s public constructor, rather than relying on
     * `Array<TValue>`'s copy constructor being invoked on non-Array memory (which
     * would be technically undefined behaviour).
     *
     * **Case 2 — actual `xltypeMulti` array**
     * Each element is cast individually via `cast<TValue>(elem_any)`.  This avoids
     * the crash that occurred in the old generic path when `Array<TValue>`'s copy
     * constructor called `ensure(is_valid())` on elements whose `xltype` did not
     * match `TValue::excel_type` exactly (e.g. `xltypeInt` elements in an
     * `Array<Number>`).  A type mismatch on any element returns `xll::None` cleanly.
     *
     * **`Array<Any>` special case**
     * `holds<Array<Any>>` only fires for `xltypeMulti`, and the live object at `&any`
     * is a real `Array<Any>` placed via `std::construct_at<Array<Any>>` in
     * `copy_from`.  The reinterpret path is valid here; no scalar handling is needed.
     *
     * @tparam TValue   Element type of the target array (deduced).
     * @param  any      The `Any` object to cast from.
     * @return          `Optional<Array<TValue>>` — engaged on success, `None` on mismatch.
     *
     * @code
     * // Scalar → 1×1 array
     * xll::Any any = xll::Number(3.14);
     * auto a = xll::cast<xll::Array<xll::Number>>(any);  // Optional<Array<Number>>{{{3.14}}}
     *
     * // Actual array
     * xll::Any arr = xll::Array<xll::Number>({ 1.0, 2.0, 3.0 });
     * auto b = xll::cast<xll::Array<xll::Number>>(arr);  // Optional<Array<Number>> engaged
     * auto c = xll::cast<xll::Array<xll::String>>(arr);  // None — element type mismatch
     * @endcode
     */
    template<typename TTarget>
        requires is_typed_array<TTarget> && is_xll_type<TTarget>
    [[nodiscard]]
    Optional<TTarget> cast(const Any& any)
    {
        using TValue = typename TTarget::value_type;

        if (!holds<TTarget>(any)) return xll::None;

        if constexpr (std::same_as<TValue, Any>) {
            // The live object at &any is a valid Array<Any> — the reinterpret path
            // is correct here; std::launder is valid because copy_from places the
            // object via std::construct_at<Array<Any>>.
            return Optional<TTarget>(
                *std::launder(reinterpret_cast<const TTarget*>(&any)));
        }
        else {
            // ── Case 1: scalar → 1×1 Array<TValue> ──────────────────────────
            // holds<Array<TValue>> returns true for a scalar whose xltype equals
            // TValue::excel_type. Construct the array via its public constructor
            // to avoid UB from calling Array<TValue>'s copy ctor on Number memory.
            if (any.type() != xltypeMulti) {
                auto elem = cast<TValue>(any);
                if (!elem) return xll::None;
                return TTarget({ std::move(*elem) });  // 1-element Horizontal
            }

            // ── Case 2: actual xltypeMulti array → element-by-element cast ──
            // The live object at &any is Array<Any> (placed by copy_from via
            // std::construct_at<Array<Any>>). Its lparray holds live Any objects
            // placed by Array<Any>'s own copy constructor.
            const auto& src = *std::launder(reinterpret_cast<const Array<Any>*>(&any));
            const auto nRows = static_cast<size_t>(src.val.array.rows);
            const auto nCols = static_cast<size_t>(src.val.array.columns);
            const size_t n   = nRows * nCols;

            if (n == 0) return TTarget{};

            TTarget result(nRows, nCols);
            for (size_t i = 0; i < n; ++i) {
                const Any& elem_any =
                    *std::launder(reinterpret_cast<const Any*>(&src.val.array.lparray[i]));
                auto elem = cast<TValue>(elem_any);
                if (!elem) return xll::None;  // element type mismatch — fail cleanly
                result[i] = std::move(*elem);
            }
            return result;
        }
    }

    /**
     * @brief Retrieves the value stored in an `xll::Any` as `TTarget`.
     *
     * The type check is delegated to `xll::holds<TTarget>(any)`:
     * - For plain xll types the check is a simple xltype equality comparison.
     *
     * On success, returns a deep copy of the value wrapped in `Optional<TTarget>`.
     * On type mismatch, returns the disengaged state (`xll::None`).
     *
     * @note  For `Array<T>` types use the dedicated `cast<Array<T>>` overload, which
     *        handles the scalar-as-1×1-array case and per-element type conversion.
     *        This overload is excluded from matching typed arrays by the
     *        `!is_typed_array<TTarget>` constraint.
     *
     * @tparam TTarget  The xll type to cast to (must satisfy `is_xll_type`,
     *                  must not be a typed array, tuple, or string-enum type).
     * @param  any      The `Any` object to cast from.
     * @return          `Optional<TTarget>` — engaged on success, `None` on mismatch.
     *
     * @code
     * xll::Any any = xll::Number(3.14);
     * auto n = xll::cast<xll::Number>(any);   // Optional<Number> = 3.14
     * auto s = xll::cast<xll::String>(any);   // Optional<String> = None
     * @endcode
     */
    template<typename TTarget>
        requires is_xll_type<TTarget> && (!is_tuple_type<TTarget>) && (!is_string_enum_type<TTarget>) && (!is_typed_array<TTarget>)
    [[nodiscard]]
    Optional<TTarget> cast(const Any& any)
    {
        return holds<TTarget>(any)
            ? Optional<TTarget>(*std::launder(reinterpret_cast<const TTarget*>(&any)))
            : xll::None;
    }



}    // namespace xll







