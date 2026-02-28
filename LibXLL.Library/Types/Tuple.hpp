/**
 * @file Tuple.hpp
 * @brief A heterogeneous, Excel-compatible tuple type backed by an XLOPER12 array.
 *
 * `xll::Tuple<Ts...>` is a run-time heterogeneous sequence whose element
 * types are declared at compile time (like `std::tuple`), but whose values
 * originate from Excel and therefore cannot be enforced at compile time.
 * The storage is a flat `xltypeMulti` XLOPER12 array of `xll::Any` elements,
 * managed via private inheritance from `Array<Any>`.
 *
 * ## Validity
 *
 * A `Tuple` is **structurally valid** when its underlying `xltypeMulti` array
 * contains exactly `sizeof...(Ts)` elements.  Normally-constructed objects are
 * always valid.  An object received from a raw Excel `XLOPER12` (e.g. via
 * `xll::Any`) may have the wrong number of elements and is then **invalid**.
 *
 *  - `valid()` — non-throwing structural check.
 *  - `validate()` — throws `std::invalid_argument` if the object is invalid.
 *
 * ## Element access
 *
 * Elements are accessed via `xll::get`, which returns the declared type
 * directly (not wrapped in `xll::Optional`).  It throws if:
 *   - The `Tuple` is structurally invalid (wrong size / xltype).
 *   - The runtime xltype of the requested element does not match the
 *     declared type `T` at that position.
 *
 * To handle potentially-invalid input from Excel without exceptions, use
 * `xll::cast<MyTuple>(any)`, which returns `xll::Optional<MyTuple>` —
 * engaged only when the `Any` holds a structurally valid tuple **and** every
 * element matches its declared type.  A successful cast therefore guarantees
 * that all `xll::get` calls on the result will succeed.
 *
 * @code
 * using MyTuple = xll::Tuple<xll::String, xll::Number, xll::Bool>;
 * MyTuple t { xll::String("hi"), xll::Number(3.14), xll::Bool(true) };
 *
 * auto s = xll::get<0>(t);              // xll::String  — throws if wrong type
 * auto n = xll::get<1>(t);              // xll::Number  — throws if wrong type
 * auto b = xll::get<xll::Bool>(t);      // xll::Bool    — throws if wrong type
 *
 * // Safe path from Excel:
 * xll::Any any = receive_from_excel();
 * if (auto opt = xll::cast<MyTuple>(any)) {
 *     auto s2 = xll::get<0>(*opt);
 * }
 * @endcode
 */

#pragma once

#include "Any.hpp"
#include "Array.hpp"
#include "Optional.hpp"
#include "../Utils/Concepts.hpp"
#include <cstddef>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace xll
{
    // =========================================================================
    // Internal helpers
    // =========================================================================

    namespace impl
    {
        /// Count how many times T appears in the pack Ts...
        template<typename T, typename... Ts>
        inline constexpr size_t count_type = (static_cast<size_t>(std::is_same_v<T, Ts>) + ...);

        /// Index of T in Ts... (first occurrence; T must appear exactly once).
        template<typename T, typename... Ts>
        inline constexpr size_t index_of = []<size_t... Is>(std::index_sequence<Is...>) constexpr {
            size_t idx = 0;
            ((std::is_same_v<T, Ts> ? (void)(idx = Is) : (void)0), ...);
            return idx;
        }(std::index_sequence_for<Ts...>{});

        /// Type at index I in the pack Ts...
        template<size_t I, typename... Ts>
        using type_at = std::tuple_element_t<I, std::tuple<Ts...>>;

        /// Forward declaration so Tuple<Ts...> can befriend it before it is defined.
        struct TupleAccess;

    }    // namespace impl

    // =========================================================================
    // xll::Tuple
    // =========================================================================

    /**
     * @brief Heterogeneous Excel-compatible tuple with compile-time declared element types.
     *
     * Wraps `Array<Any>` via private inheritance, exposing only the tuple
     * interface.  The declared types `Ts...` document the expected types of
     * each position; actual enforcement happens at run time through
     * `xll::get`, which throws on structural invalidity or element type mismatch.
     *
     * @tparam Ts  The declared element types, in order.  Each must satisfy
     *             `is_xll_type`.
     *
     * @see xll::get
     * @see xll::cast
     */
    template<typename... Ts>
        requires (is_xll_type<Ts> && ...)
    class Tuple : private Array<Any>
    {
        using Base = Array<Any>;

        static constexpr size_t arity = sizeof...(Ts);

    public:
        /// Excel type tag — always xltypeMulti.  Required by is_xll_type and
        /// needed so that xll::Any can store a Tuple via its converting constructor.
        static constexpr size_t excel_type = xltypeMulti;
        // ------------------------------------------------------------------
        // Constructors
        // ------------------------------------------------------------------

        /// Default constructor — all elements initialised to xll::Nil.
        Tuple() : Base(arity, 1) {}

        /**
         * @brief Constructs from an initializer list of `xll::Any` values.
         * @throws std::invalid_argument if `values.size() != sizeof...(Ts)`.
         */
        Tuple(std::initializer_list<Any> values) : Base(values, Horizontal{})
        {
            if (values.size() != arity)
                throw std::invalid_argument(
                    "xll::Tuple: initializer list has " +
                    std::to_string(values.size()) +
                    " element(s), expected " + std::to_string(arity));
        }

        // ------------------------------------------------------------------
        // Copy / move
        // ------------------------------------------------------------------

        /// @note Use parentheses for copy/move construction (e.g. `T b(a)`).
        ///       Brace-init (`T b { a }`) is ambiguous with the
        ///       `initializer_list<Any>` constructor when `Tuple` is
        ///       convertible to `Any`, and will throw at runtime if
        ///       `arity != 1`.
        Tuple(const Tuple&)            = default;
        Tuple(Tuple&&)                 = default;
        Tuple& operator=(const Tuple&) = default;
        Tuple& operator=(Tuple&&)      = default;
        ~Tuple()                       = default;

        // ------------------------------------------------------------------
        // Validity
        // ------------------------------------------------------------------

        /**
         * @brief Returns `true` iff the underlying array has exactly
         *        `sizeof...(Ts)` elements and xltype is `xltypeMulti`.
         *
         * Always `true` for normally constructed objects.  May be `false`
         * only for objects received from a raw Excel `XLOPER12` that
         * bypasses the xll type system.
         *
         * Use this as a non-throwing guard before calling `xll::get` on
         * objects received from raw Excel data.
         */
        [[nodiscard]] bool valid() const noexcept
        {
            const auto& raw = static_cast<const XLOPER12&>(
                static_cast<const Base&>(*this));
            constexpr int TYPE_MASK = ~(xlbitDLLFree | xlbitXLFree);
            if ((raw.xltype & TYPE_MASK) != xltypeMulti) return false;
            const size_t n = static_cast<size_t>(raw.val.array.rows) *
                             static_cast<size_t>(raw.val.array.columns);
            return n == arity;
        }

        /**
         * @brief Throws `std::invalid_argument` if the Tuple is not in a
         *        valid state (wrong xltype or wrong number of elements).
         *
         * Call this after receiving a Tuple from Excel to assert structural
         * integrity before performing element access.
         */
        void validate() const
        {
            if (!valid()) {
                const auto& raw = static_cast<const XLOPER12&>(
                    static_cast<const Base&>(*this));
                constexpr int TYPE_MASK = ~(xlbitDLLFree | xlbitXLFree);
                if ((raw.xltype & TYPE_MASK) != xltypeMulti)
                    throw std::invalid_argument(
                        "xll::Tuple: underlying XLOPER12 is not xltypeMulti");
                const size_t n = static_cast<size_t>(raw.val.array.rows) *
                                 static_cast<size_t>(raw.val.array.columns);
                throw std::invalid_argument(
                    "xll::Tuple: underlying array has " + std::to_string(n) +
                    " element(s), expected " + std::to_string(arity));
            }
        }

        // ------------------------------------------------------------------
        // Observers
        // ------------------------------------------------------------------

        /// Returns the XLOPER12 type tag (always xltypeMulti for a valid Tuple).
        [[nodiscard]] constexpr int xltype() const noexcept
        {
            return static_cast<const XLOPER12&>(static_cast<const Base&>(*this)).xltype;
        }

        /// Returns a pointer to the raw element storage (nullptr if empty).
        [[nodiscard]] constexpr const void* data() const noexcept
        {
            return static_cast<const XLOPER12&>(
                static_cast<const Base&>(*this)).val.array.lparray;
        }

        // ------------------------------------------------------------------
        // Friend access
        // A single non-template friend struct sidesteps the inline-friend-
        // template redefinition issue present in clang-19.
        // ------------------------------------------------------------------

        friend struct impl::TupleAccess;

    private:

        [[nodiscard]] const Any& element_at(size_t i) const noexcept
        {
            return static_cast<const Base&>(*this)[i];
        }
    };  // class Tuple

    // =========================================================================
    // impl::TupleAccess — definition (Tuple must be complete first)
    // =========================================================================

    namespace impl
    {
        struct TupleAccess
        {
            template<typename... Ts>
            static const Any& element_at(const Tuple<Ts...>& t, size_t i) noexcept
            {
                return t.element_at(i);
            }

            template<typename... Ts>
            static bool valid(const Tuple<Ts...>& t) noexcept
            {
                return t.valid();
            }

            template<typename... Ts>
            static constexpr size_t arity(const Tuple<Ts...>&) noexcept
            {
                return sizeof...(Ts);
            }
        };
    }

    // =========================================================================
    // xll::get — free function definitions
    // =========================================================================

    /**
     * @brief Retrieves the element at compile-time index @p I.
     *
     * Returns the declared type `T` at position `I` directly (not wrapped in
     * `xll::Optional`).
     *
     * @throws std::invalid_argument if the Tuple is structurally invalid
     *         (wrong xltype or wrong number of elements).
     * @throws std::invalid_argument if the runtime xltype of the element at
     *         position `I` does not match the declared type `T`.
     *
     * @tparam I  Zero-based index. Must be < `sizeof...(Ts)` (compile-time).
     */
    template<size_t I, typename... Ts>
        requires (I < sizeof...(Ts))
    [[nodiscard]]
    impl::type_at<I, Ts...> get(const Tuple<Ts...>& tuple)
    {
        tuple.validate();   // throws if structurally invalid

        using TTarget = impl::type_at<I, Ts...>;
        auto opt = xll::cast<TTarget>(impl::TupleAccess::element_at(tuple, I));
        if (!opt)
            throw std::invalid_argument(
                "xll::get<" + std::to_string(I) + ">: element type mismatch");
        return std::move(*opt);
    }

    /**
     * @brief Retrieves the element of declared type `TTarget`.
     *
     * `TTarget` must appear **exactly once** in `Ts...` — enforced at compile time.
     *
     * Returns the value of type `TTarget` directly (not wrapped in `xll::Optional`).
     *
     * @throws std::invalid_argument if the Tuple is structurally invalid
     *         (wrong xltype or wrong number of elements).
     * @throws std::invalid_argument if the runtime xltype of the element at
     *         the position of `TTarget` does not match.
     *
     * @tparam TTarget  Must appear exactly once in `Ts...`.
     */
    template<typename TTarget, typename... Ts>
        requires is_xll_type<TTarget> && (impl::count_type<TTarget, Ts...> == 1)
    [[nodiscard]]
    TTarget get(const Tuple<Ts...>& tuple)
    {
        tuple.validate();   // throws if structurally invalid

        constexpr size_t I = impl::index_of<TTarget, Ts...>;
        auto opt = xll::cast<TTarget>(impl::TupleAccess::element_at(tuple, I));
        if (!opt)
            throw std::invalid_argument(
                "xll::get<T>: element type mismatch at index " + std::to_string(I));
        return std::move(*opt);
    }

    // =========================================================================
    // sizeof check
    // =========================================================================

    static_assert(sizeof(Tuple<Number, String>) == sizeof(XLOPER12),
        "xll::Tuple must not add data members beyond XLOPER12");

    // =========================================================================
    // is_tuple_type — specialisation
    //
    // The concept itself is forward-declared in Concepts.hpp.
    // Here we specialise is_tuple_impl for Tuple<Ts...>.
    // =========================================================================

    namespace impl
    {
        template<typename... Ts>
        struct is_tuple_impl<Tuple<Ts...>> : std::true_type {};
    }

    // =========================================================================
    // xll::cast — Tuple specialisation
    //
    // Defined here (not in Any.hpp) to avoid a circular include:
    //   Any.hpp → Tuple.hpp → Any.hpp
    //
    // Validates both structure AND element types, so a successful cast
    // guarantees that every xll::get call on the result will succeed.
    // =========================================================================

    namespace impl
    {
        /// Core: checks each element against its declared type via cast.
        template<typename... Ts, size_t... Is>
        bool all_elements_match(const Tuple<Ts...>& t,
                                std::index_sequence<Is...>) noexcept
        {
            return (... && static_cast<bool>(
                xll::cast<type_at<Is, Ts...>>(TupleAccess::element_at(t, Is))));
        }

        /// Convenience overload: deduces Ts... and generates the index sequence.
        template<typename... Ts>
        bool all_elements_match(const Tuple<Ts...>& t) noexcept
        {
            return all_elements_match(t, std::index_sequence_for<Ts...>{});
        }
    }

    /**
     * @brief Casts an `xll::Any` to `xll::Tuple<Ts...>`.
     *
     * Returns an engaged `Optional<TTarget>` only when **all** of the
     * following hold:
     *   - The stored value has xltype == xltypeMulti.
     *   - The array contains exactly `sizeof...(Ts)` elements.
     *   - Every element at position `I` has an xltype compatible with the
     *     declared type `Ts[I]`.
     *
     * Returns `xll::None` if any check fails, so a successful cast guarantees
     * that every subsequent `xll::get` call on the result will succeed.
     *
     * @tparam TTarget  A specialisation of `xll::Tuple`.
     * @param  any      The `Any` object to cast from.
     * @return          `Optional<TTarget>` — fully validated on success,
     *                  `None` on any mismatch.
     *
     * @code
     * using Row = xll::Tuple<xll::String, xll::Number>;
     *
     * xll::Any a = Row { xll::String("x"), xll::Number(1.0) };
     * auto r = xll::cast<Row>(a);    // Optional<Row> — engaged
     *
     * xll::Any b = Row { xll::Number(1.0), xll::String("bad") };  // swapped types
     * auto s = xll::cast<Row>(b);    // Optional<Row> — None (element mismatch)
     *
     * xll::Any n = xll::Number(3.14);
     * auto t = xll::cast<Row>(n);    // Optional<Row> — None (wrong xltype)
     * @endcode
     */
    template<typename TTarget>
        requires is_tuple_type<TTarget>
    [[nodiscard]]
    Optional<TTarget> cast(const Any& any)
    {
        if (any.type() != xltypeMulti)
            return xll::None;

        const auto* t = std::launder(reinterpret_cast<const TTarget*>(&any));

        if (!impl::TupleAccess::valid(*t))
            return xll::None;

        if (!impl::all_elements_match(*t))
            return xll::None;

        return Optional<TTarget>(*t);
    }

}    // namespace xll

