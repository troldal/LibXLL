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
 * Elements are accessed via `xll::get`:
 *
 * @code
 * using MyTuple = xll::Tuple<xll::String, xll::Number, xll::Bool>;
 * MyTuple t { xll::String("hi"), xll::Number(3.14), xll::Bool(true) };
 *
 * auto s = xll::get<0>(t);              // Optional<String>
 * auto n = xll::get<1>(t);              // Optional<Number>
 * auto b = xll::get<xll::Bool>(t);      // Optional<Bool>
 * @endcode
 */

#pragma once

#include "Any.hpp"
#include "Array.hpp"
#include "Optional.hpp"
#include "../Utils/Concepts.hpp"
#include <cstddef>
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
     * `xll::get`, which returns `xll::Optional<T>`.
     *
     * @tparam Ts  The declared element types, in order.  Each must satisfy
     *             `is_xll_type`.
     *
     * @see xll::get
     */
    template<typename... Ts>
        requires (is_xll_type<Ts> && ...)
    class Tuple : private Array<Any>
    {
        using Base = Array<Any>;

        static constexpr size_t arity = sizeof...(Ts);

    public:
        // ------------------------------------------------------------------
        // Constructors
        // ------------------------------------------------------------------

        /// Default constructor — all elements initialised to xll::Nil.
        Tuple() : Base(arity, 1) {}

        /**
         * @brief Constructs from an initializer list of `xll::Any` values.
         * @throws std::out_of_range if `values.size() != sizeof...(Ts)`.
         */
        Tuple(std::initializer_list<Any> values) : Base(values, Horizontal{})
        {
            if (values.size() != arity)
                throw std::out_of_range(
                    "xll::Tuple initializer list size does not match the declared arity");
        }

        // ------------------------------------------------------------------
        // Copy / move
        // ------------------------------------------------------------------

        Tuple(const Tuple&)            = default;
        Tuple(Tuple&&)                 = default;
        Tuple& operator=(const Tuple&) = default;
        Tuple& operator=(Tuple&&)      = default;
        ~Tuple()                       = default;

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
        };
    }

    // =========================================================================
    // xll::get — free function definitions
    // =========================================================================

    /**
     * @brief Retrieves the element at compile-time index @p I.
     *
     * Returns `Optional<T>` where `T` is the declared type at position `I`.
     * Returns `xll::None` if the runtime xltype of the element does not match.
     *
     * @tparam I     Zero-based index. Must be < `sizeof...(Ts)` (compile-time).
     */
    template<size_t I, typename... Ts>
        requires (I < sizeof...(Ts))
    [[nodiscard]]
    Optional<impl::type_at<I, Ts...>> get(const Tuple<Ts...>& tuple) noexcept
    {
        using TTarget = impl::type_at<I, Ts...>;
        return xll::cast<TTarget>(impl::TupleAccess::element_at(tuple, I));
    }

    /**
     * @brief Retrieves the element of declared type `TTarget`.
     *
     * `TTarget` must appear **exactly once** in `Ts...` — enforced at compile time.
     * Returns `xll::None` if the runtime xltype of the element does not match.
     *
     * @tparam TTarget  Must appear exactly once in `Ts...`.
     */
    template<typename TTarget, typename... Ts>
        requires is_xll_type<TTarget> && (impl::count_type<TTarget, Ts...> == 1)
    [[nodiscard]]
    Optional<TTarget> get(const Tuple<Ts...>& tuple) noexcept
    {
        constexpr size_t I = impl::index_of<TTarget, Ts...>;
        return xll::cast<TTarget>(impl::TupleAccess::element_at(tuple, I));
    }

    // =========================================================================
    // sizeof check
    // =========================================================================

    static_assert(sizeof(Tuple<Number, String>) == sizeof(XLOPER12),
        "xll::Tuple must not add data members beyond XLOPER12");

}    // namespace xll

