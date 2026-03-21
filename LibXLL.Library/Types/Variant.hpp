/**
 * @file Variant.hpp
 * @brief Type-safe discriminated union over xll types, built on the XLOPER12 memory layout.
 *
 * @section overview Design Overview
 *
 * xll::Variant<T, Ts...> is a discriminated union analogous to `std::variant`, but
 * constrained to xll types and built directly on the XLOPER12 binary layout.  Like every
 * other xll wrapper it inherits from XLOPER12 without adding any data members:
 *
 * ```
 * sizeof(Variant<T, Ts...>) == sizeof(XLOPER12)   (always 16 bytes)
 * ```
 *
 * This means a Variant can be passed directly to the Excel C API as a raw XLOPER12 pointer
 * and can be wrapped in xll::Optional or xll::Expected without boxing overhead.
 *
 * @section discriminant Discriminant
 *
 * The active alternative is identified by the inherited `XLOPER12::xltype` field, which
 * serves as the runtime discriminant.  The payload lives in `XLOPER12::val`.
 * Ownership flags (`xlbitDLLFree`, `xlbitXLFree`) are always masked before any type
 * comparison so that Excel-owned values work correctly.
 *
 * @section type_punning Type Punning and Pointer Provenance
 *
 * Changing the active alternative requires ending the lifetime of the current object and
 * beginning a new one.  The established library pattern is used throughout:
 *   - `std::destroy_at(std::launder(reinterpret_cast<U*>(this)))` to destroy
 *   - `std::construct_at(reinterpret_cast<U*>(this), ...)` to construct
 *   - `std::launder(reinterpret_cast<U*>(ptr))` to obtain a provenance-correct pointer
 *     after placement construction, before the first access
 *
 * @section constraints Type Constraints
 *
 * Every type in the parameter pack must:
 *   - Satisfy `is_xll_type` (inherit from XLOPER12, expose `excel_type`, fit in 16 bytes)
 *   - Not be `xll::Missing` (always excluded — Missing is only meaningful as a raw function
 *     argument, not as a storable value)
 *   - Be unique within the pack (`Variant<Bool, Bool>` is ill-formed)
 *
 * Valid examples:
 * ```cpp
 * xll::Variant<xll::Number, xll::String>
 * xll::Variant<xll::Number, xll::String, xll::Bool>
 * xll::Variant<xll::Number, xll::String, xll::Bool, xll::Int, xll::Error, xll::Nil>
 * ```
 *
 * @section excel_interop Excel Interoperability
 *
 * The canonical UDF pattern: receive a cell value as `xll::Any`, narrow it to a Variant
 * via `xll::cast`, then dispatch on the active alternative via `xll::visit`:
 *
 * ```cpp
 * // UDF parameter declared as xll::Any
 * xll::Optional<xll::Variant<xll::Number, xll::String>> typed =
 *     xll::cast<xll::Variant<xll::Number, xll::String>>(any_from_excel);
 *
 * if (!typed) return xll::ErrValue;   // xltype not in the declared list
 *
 * return xll::visit(xll::overload{
 *     [](const xll::Number& n) -> xll::Number { return n * 2.0; },
 *     [](const xll::String& s) -> xll::String { return s + " (processed)"; }
 * }, *typed);
 * ```
 *
 * @section free_functions Free Functions
 *
 * The following free functions mirror the `std::variant` API:
 *   - `xll::holds_alternative<U>(v)`  — query the active alternative
 *   - `xll::get<U>(v)`                — access with `std::bad_variant_access` on mismatch
 *   - `xll::get_if<U>(v)`             — access returning `Optional<U>` (never throws)
 *   - `xll::visit(visitor, v)`        — type-safe polymorphic dispatch
 *
 * @author Kenneth Troldal Balslev
 * @date 23/03/2025
 */

#pragma once

#include "Bool.hpp"
#include "Error.hpp"
#include "Int.hpp"
#include "Missing.hpp"
#include "Nil.hpp"
#include "Number.hpp"
#include "Optional.hpp"
#include "String.hpp"

#include <functional>
#include <utility>
#include <variant>

namespace xll
{
    namespace impl
    {
        /**
         * @brief Recursive variable template that is `true` iff every type in the pack is
         *        distinct.
         *
         * Base case (empty pack): trivially true.
         * Recursive case: `T` must differ from every type in `Ts...`, and the remainder
         * `Ts...` must also be mutually distinct.
         *
         */
        template<typename...>
        inline constexpr bool all_unique_v = true;

        /// @cond INTERNAL
        template<typename T, typename... Ts>
        inline constexpr bool all_unique_v<T, Ts...> =
            (!std::is_same_v<T, Ts> && ...) && all_unique_v<Ts...>;
        /// @endcond

        /**
         * @brief Concept that is satisfied iff every type in `Types...` is distinct.
         *
         * Used to prevent `Variant<Bool, Bool>` and similar duplicate-type instantiations
         * from compiling.
         *
         * @tparam Types  The pack of types to check for uniqueness.
         */
        template<typename... Types>
        concept unique_types = all_unique_v<Types...>;
    }    // namespace impl

    // =========================================================================
    // Forward declarations
    // =========================================================================

    /**
     * @brief Forward declaration of `xll::Variant`.
     *
     * Required so that the `impl::is_variant_specialization_impl` and
     * `impl::variant_member_impl` partial specialisations can reference the
     * complete class name before its definition.
     *
     * @tparam T   First (default) alternative.  Must satisfy `is_xll_type` and must not
     *             be `xll::Missing`.
     * @tparam Ts  Remaining alternatives.  Each must satisfy `is_xll_type`, must not be
     *             `xll::Missing`, and must be distinct from every other type in the pack.
     */
    template<typename T, typename... Ts>
    requires(is_xll_type<T> && (is_xll_type<Ts> && ...) &&
             impl::unique_types<T, Ts...> &&
             !std::same_as<T, xll::Missing> &&
             !(std::same_as<Ts, xll::Missing> || ...))
    class Variant;

    namespace impl
    {
        /// @cond INTERNAL

        /**
         * @brief Primary template — `T` is not a `Variant` specialisation.
         */
        template<typename>
        struct is_variant_specialization_impl : std::false_type {};

        /**
         * @brief Partial specialisation — matches any `Variant<T, Ts...>`.
         * @tparam T   First alternative of the matched Variant.
         * @tparam Ts  Remaining alternatives of the matched Variant.
         */
        template<typename T, typename... Ts>
        struct is_variant_specialization_impl<Variant<T, Ts...>> : std::true_type {};

        /// @endcond

        /**
         * @brief Concept satisfied iff `V` is a specialisation of `xll::Variant`.
         *
         * Used to constrain the `xll::visit` free-function wrapper so that it only
         * matches Variant arguments and not arbitrary XLOPER12-derived types.
         *
         * @tparam V  The type to test.
         */
        template<typename V>
        concept variant_specialization = is_variant_specialization_impl<V>::value;

        /// @cond INTERNAL

        /**
         * @brief Primary template — `U` is not a member of variant `V`.
         * @tparam U  Candidate type.
         * @tparam V  Candidate Variant specialisation.
         */
        template<typename U, typename V>
        struct variant_member_impl : std::false_type {};

        /**
         * @brief Partial specialisation — `U` is `true_type` iff it equals `T` or one
         *        of `Ts...`.
         *
         * @tparam U   The type to look up.
         * @tparam T   First alternative of the Variant.
         * @tparam Ts  Remaining alternatives of the Variant.
         */
        template<typename U, typename T, typename... Ts>
            requires (std::same_as<U, T> || (std::same_as<U, Ts> || ...))
        struct variant_member_impl<U, Variant<T, Ts...>> : std::true_type {};

        /// @endcond

        /**
         * @brief Concept satisfied iff `U` is one of the declared alternatives of `V`.
         *
         * Used to constrain `xll::get` and `xll::get_if` so that requesting a type that
         * is not in the Variant is a compile-time error rather than a runtime failure.
         *
         * @tparam U  The alternative type to look up.
         * @tparam V  A cv-ref-stripped `Variant` specialisation.
         */
        template<typename U, typename V>
        concept variant_member = variant_member_impl<U, V>::value;
    }    // namespace impl

    // =========================================================================
    // Free-function forward declarations
    // =========================================================================

    /**
     * @brief Returns `true` if the active alternative of @p v is `U`.
     *
     * The comparison masks `xlbitDLLFree` and `xlbitXLFree` so that Excel-owned
     * values (where Excel has set ownership bits on `xltype`) are handled correctly.
     *
     * @tparam U     The alternative type to test for.  Must be one of `T, Ts...`.
     * @tparam T     First alternative of the Variant (deduced).
     * @tparam Ts    Remaining alternatives (deduced).
     * @param  v     The Variant to inspect.
     * @return `true` if `(v.xltype & ~ownership_bits) == U::excel_type`, else `false`.
     */
    template<typename U, typename T, typename... Ts>
        requires (std::same_as<U, T> || (std::same_as<U, Ts> || ...))
    constexpr bool holds_alternative(const Variant<T, Ts...>& v) noexcept;

    /**
     * @brief Returns a reference to the active alternative, throwing on type mismatch.
     *
     * A single forwarding-reference overload handles all four cv/ref combinations
     * (`Variant&` → `U&`, `const Variant&` → `const U&`, `Variant&&` → `U&&`,
     * `const Variant&&` → `const U&&`).  The correct value category is propagated via
     * `std::forward_like` inside the member `get`.
     *
     * @tparam U        The alternative type to retrieve.  Must be one of `T, Ts...`.
     * @tparam TVariant The cv/ref-qualified Variant type (deduced).
     * @param  v        The Variant to access.
     * @return A reference to the stored `U` with the same cv/ref qualifications as `v`.
     * @throws std::bad_variant_access if the active alternative is not `U`.
     */
    template<typename U, typename TVariant>
        requires impl::variant_member<U, std::remove_cvref_t<TVariant>>
    [[nodiscard]] constexpr decltype(auto) get(TVariant&& v);

    /**
     * @brief Returns an `Optional<U>` containing the active value, or `xll::None`.
     *
     * Unlike `xll::get`, this function never throws.  It checks `holds_alternative<U>`
     * and returns a disengaged `Optional` on mismatch.
     *
     * Value category is forwarded: calling `get_if` on an lvalue copies the value into
     * the Optional; calling it on an rvalue moves the value into the Optional.
     *
     * @tparam U        The alternative type to retrieve.  Must be one of `T, Ts...`.
     * @tparam TVariant The cv/ref-qualified Variant type (deduced).
     * @param  v        The Variant to inspect.
     * @return `Optional<U>{ get<U>(v) }` if `holds_alternative<U>(v)`, else `Optional<U>{}`.
     */
    template<typename U, typename TVariant>
        requires impl::variant_member<U, std::remove_cvref_t<TVariant>>
    [[nodiscard]] constexpr Optional<U> get_if(TVariant&& v);

    /**
     * @brief Applies a visitor to the active alternative of a Variant.
     *
     * A single forwarding-reference overload handles all four cv/ref combinations.
     * The free function delegates to the member `visit`, which uses deducing-this to
     * propagate the correct value category to the alternative passed to the visitor.
     *
     * @tparam Visitor  A callable type whose call operator is invocable with every
     *                  alternative type in the Variant.  All overloads must return the
     *                  same type (or types with a common type).
     * @tparam TVariant The cv/ref-qualified Variant type (deduced).
     * @param  vis      The visitor callable.
     * @param  va       The Variant to visit.
     * @return The return value of `vis(active_alternative)`.
     * @throws std::bad_variant_access if the stored xltype matches none of the
     *         declared alternatives (indicates corrupted or externally-mutated storage).
     *
     * @note Use `xll::overload{...}` to build a per-type overload set:
     * ```cpp
     * xll::visit(xll::overload{
     *     [](const xll::Number& n) { ... },
     *     [](const xll::String& s) { ... }
     * }, variant);
     * ```
     */
    template<typename Visitor, typename TVariant>
        requires impl::variant_specialization<std::remove_cvref_t<TVariant>>
    [[nodiscard]] constexpr decltype(auto) visit(Visitor&& vis, TVariant&& va);

    // =========================================================================
    // xll::overload — helper for building per-type visitor overload sets
    // =========================================================================

    /**
     * @brief Aggregates multiple callable objects into a single overloaded call operator.
     *
     * Inherits `operator()` from every base type, making all overloads visible in one
     * type.  Primarily used with `xll::visit` to dispatch to per-alternative handlers:
     *
     * ```cpp
     * xll::Variant<xll::Number, xll::String> v = xll::String("hello");
     *
     * xll::visit(xll::overload{
     *     [](const xll::Number& n) { std::cout << "Number: " << n << "\n"; },
     *     [](const xll::String& s) { std::cout << "String: " << s << "\n"; }
     * }, v);
     * // prints: String: hello
     * ```
     *
     * @tparam Ts  The callable types to aggregate.  Typically lambda types.
     */
    template<class... Ts>
    struct overload : Ts...
    {
        using Ts::operator()...;
    };

    // =========================================================================
    // xll::Variant
    // =========================================================================

    /**
     * @brief Type-safe discriminated union over two or more xll types.
     *
     * `Variant<T, Ts...>` stores exactly one value drawn from the declared set of
     * alternatives at any given time.  The active alternative is identified by the
     * inherited `XLOPER12::xltype` field.
     *
     * ## Constraints
     *
     * Every type in `T, Ts...` must satisfy `is_xll_type`, must not be
     * `xll::Missing`, and must be distinct.  Violations are compile-time errors.
     *
     * ## Memory layout
     *
     * Variant inherits directly from XLOPER12 and adds no data members.
     * `sizeof(Variant<T,Ts...>) == sizeof(XLOPER12)` (16 bytes) is a hard invariant.
     *
     * ## Default alternative
     *
     * The default constructor initialises the first alternative `T`.
     *
     * ## Valueless state
     *
     * Unlike `std::variant`, there is no "valueless by exception" sentinel.  If the
     * construction of a replacement value throws during `emplace` or assignment, the
     * object is left in a destructed, invalid state (`is_valid()` returns `false`).
     * Callers should treat this as a fatal programming error.
     *
     * @tparam T   The first (default) alternative.  Must satisfy `is_xll_type` and
     *             must not be `xll::Missing`.
     * @tparam Ts  The remaining alternatives.  Each must satisfy `is_xll_type`, must
     *             not be `xll::Missing`, and must be distinct from every other type in
     *             the pack.
     *
     * @see xll::holds_alternative
     * @see xll::get
     * @see xll::get_if
     * @see xll::visit
     * @see xll::overload
     */
    template<typename T, typename... Ts>
    requires(is_xll_type<T> && (is_xll_type<Ts> && ...) &&
             impl::unique_types<T, Ts...> &&
             !std::same_as<T, xll::Missing> &&
             !(std::same_as<Ts, xll::Missing> || ...))
    class Variant : public XLOPER12
    {
    public:

        /**
         * @brief Satisfies the `has_crtp_base` marker expected by `is_xll_type`.
         *
         * The `is_xll_type` concept currently has this check commented out.  Declaring
         * the member here ensures that Variant will keep satisfying the concept if the
         * check is ever reinstated, and that `Optional<Variant<...>>` and
         * `Expected<Variant<...>>` continue to compile without modification.
         */
        static constexpr bool has_crtp_base = true;

        /**
         * @brief Bitmask of every `xltype` value this Variant can hold.
         *
         * Computed as the bitwise OR of `T::excel_type | Ts::excel_type | ...`.  Because
         * each concrete xll type has a single power-of-two xltype bit, the result is a
         * bitmask where any set bit indicates a supported alternative.
         *
         * A binary right fold with identity `0` is used so that the single-alternative
         * case (`Ts...` empty) is well-formed — a unary fold over `|` with an empty pack
         * would be ill-formed per the C++ standard.
         *
         * @note `(xltype & ~ownership_bits) & excel_type) != 0` is the correct membership
         *       test; see `is_valid()` and `holds_alternative`.
         */
        static constexpr size_t excel_type = T::excel_type | (Ts::excel_type | ... | static_cast<size_t>(0));

        /**
         * @brief Returns `true` iff the stored `xltype` (ownership bits masked) is one
         *        of the declared alternatives.
         *
         * Consistent with `impl::Base::is_valid()` across the library: strips
         * `xlbitDLLFree` and `xlbitXLFree` before comparing, so Excel-owned values
         * are handled correctly.
         *
         * @return `(static_cast<size_t>(xltype & MASK) & excel_type) != 0`
         *
         * @note A freshly default-constructed or assigned Variant always returns `true`.
         *       A Variant that became invalid due to a throwing `emplace` returns `false`.
         */
        [[nodiscard]] constexpr bool is_valid() const noexcept
        {
            constexpr auto MASK = static_cast<decltype(xltype)>(~(xlbitDLLFree | xlbitXLFree));
            return (static_cast<size_t>(xltype & MASK) & excel_type) != 0;
        }

        // ------------------------------------------------------------------
        // Constructors
        // ------------------------------------------------------------------

        /**
         * @brief Default constructor — initialises the first alternative `T`.
         *
         * Constructs a default `T` at `this` via `std::construct_at`.
         *
         * @post `index() == 0`
         * @post `holds_alternative<T>(*this) == true`
         * @post `is_valid() == true`
         */
        constexpr Variant() : XLOPER12()
        {
            std::construct_at(reinterpret_cast<T*>(this));
        }

        /**
         * @brief Converting copy constructor — stores a copy of @p u.
         *
         * `U` must be one of the declared alternatives `T, Ts...`.  The correct
         * xltype is set by `U`'s own constructor.
         *
         * @tparam U  The alternative type to store.  Must be in `T, Ts...`.
         * @param  u  The value to copy.
         *
         * @post `holds_alternative<U>(*this) == true`
         * @post `is_valid() == true`
         */
        template<typename U>
            requires(std::same_as<U, T> || (std::same_as<U, Ts> || ...))
        constexpr Variant(const U& u) : XLOPER12()    // NOLINT
        {
            std::construct_at(reinterpret_cast<U*>(this), u);
        }

        /**
         * @brief Converting move constructor — moves @p u into the Variant.
         *
         * `U` must be one of the declared alternatives `T, Ts...`.  For `xll::String`
         * this steals the heap buffer; for trivial types it is equivalent to the copy.
         *
         * @tparam U  The alternative type to store.  Must be in `T, Ts...`.
         * @param  u  The value to move from.
         *
         * @post `holds_alternative<U>(*this) == true`
         * @post `is_valid() == true`
         */
        template<typename U>
            requires(std::same_as<U, T> || (std::same_as<U, Ts> || ...))
        constexpr Variant(U&& u) noexcept : XLOPER12()   // NOLINT
        {
            std::construct_at(reinterpret_cast<U*>(this), std::forward<U>(u));
        }

        /**
         * @brief Copy constructor — copies the active alternative from @p v.
         *
         * Delegates to the member `visit` on @p v to dispatch to the correct
         * `std::construct_at` call.  For `xll::String`, the heap buffer is deep-copied.
         *
         * @param v  The source Variant.
         *
         * @post `index() == v.index()`
         * @post `is_valid() == true`
         */
        constexpr Variant(const Variant& v) : XLOPER12()
        {
            v.visit([this]<typename TAlternative>(const TAlternative& val) {
                using U = std::remove_cvref_t<TAlternative>;
                std::construct_at(reinterpret_cast<U*>(this), val);
            });
        }

        /**
         * @brief Move constructor — moves the active alternative out of @p v.
         *
         * Delegates to `std::move(v).visit(...)` so the visitor receives a `U&&`.
         * For `xll::String`, the heap buffer is stolen; @p v is left with
         * `val.str == nullptr`.
         *
         * @param v  The source Variant.  Left in a valid-but-moved-from state.
         *
         * @post `index() == (original index of v)`
         * @post `is_valid() == true`
         */
        constexpr Variant(Variant&& v) noexcept : XLOPER12()
        {
            std::move(v).visit([this]<typename TAlternative>(TAlternative&& val) {
                using U = std::remove_cvref_t<TAlternative>;
                std::construct_at(reinterpret_cast<U*>(this), std::forward<TAlternative>(val));
            });
        }

        // ------------------------------------------------------------------
        // Destructor
        // ------------------------------------------------------------------

        /**
         * @brief Destructor — destroys the active alternative and zeroes the storage.
         *
         * For each alternative type `U` that is in `T, Ts...`, checks at compile time
         * whether `U` is a declared alternative (`if constexpr`), then at runtime
         * whether `U` is currently active (`holds_alternative`), and if so calls
         * `std::destroy_at(std::launder(...))` to run `U`'s destructor.
         *
         * `std::launder` is required to obtain a provenance-correct pointer to the
         * object placed by a prior `std::construct_at` call (same rule as in
         * `xll::Expected` and `xll::Optional`).
         *
         * Finally, the underlying `XLOPER12` is zeroed so the Excel SDK never sees
         * a stale pointer in `val.str` or similar fields.
         */
        constexpr ~Variant()
        {
            if constexpr ((std::same_as<xll::Bool, T> || (std::same_as<xll::Bool, Ts> || ...)))
                if (xll::holds_alternative<xll::Bool>(*this))
                    std::destroy_at(std::launder(reinterpret_cast<xll::Bool*>(this)));

            if constexpr ((std::same_as<xll::Error, T> || (std::same_as<xll::Error, Ts> || ...)))
                if (xll::holds_alternative<xll::Error>(*this))
                    std::destroy_at(std::launder(reinterpret_cast<xll::Error*>(this)));

            if constexpr ((std::same_as<xll::Int, T> || (std::same_as<xll::Int, Ts> || ...)))
                if (xll::holds_alternative<xll::Int>(*this))
                    std::destroy_at(std::launder(reinterpret_cast<xll::Int*>(this)));

            if constexpr ((std::same_as<xll::Number, T> || (std::same_as<xll::Number, Ts> || ...)))
                if (xll::holds_alternative<xll::Number>(*this))
                    std::destroy_at(std::launder(reinterpret_cast<xll::Number*>(this)));

            if constexpr ((std::same_as<xll::String, T> || (std::same_as<xll::String, Ts> || ...)))
                if (xll::holds_alternative<xll::String>(*this))
                    std::destroy_at(std::launder(reinterpret_cast<xll::String*>(this)));

            if constexpr ((std::same_as<xll::Nil, T> || (std::same_as<xll::Nil, Ts> || ...)))
                if (xll::holds_alternative<xll::Nil>(*this))
                    std::destroy_at(std::launder(reinterpret_cast<xll::Nil*>(this)));

            static_cast<XLOPER12&>(*this) = XLOPER12();
        }

        // ------------------------------------------------------------------
        // Assignment operators
        // ------------------------------------------------------------------

        /**
         * @brief Copy assignment operator.
         *
         * Guards against self-assignment, destroys the current alternative via
         * `std::destroy_at(this)`, then copy-constructs the active alternative
         * of @p v at `this` using `v.visit(lambda)`.
         *
         * @param v  The source Variant.
         * @return   Reference to `*this`.
         *
         * @post `index() == v.index()`
         * @post `is_valid() == true`
         */
        constexpr Variant& operator=(const Variant& v)
        {
            if (this == &v) return *this;
            std::destroy_at(this);
            v.visit([this]<typename TAlternative>(const TAlternative& val) {
                using U = std::remove_cvref_t<TAlternative>;
                std::construct_at(reinterpret_cast<U*>(this), val);
            });
            return *this;
        }

        /**
         * @brief Move assignment operator.
         *
         * Guards against self-assignment, destroys the current alternative, then
         * move-constructs the active alternative of @p v via `std::move(v).visit(lambda)`.
         * For `xll::String`, ownership of the heap buffer is transferred.
         *
         * @param v  The source Variant.  Left in a valid-but-moved-from state.
         * @return   Reference to `*this`.
         *
         * @post `index() == (original index of v)`
         * @post `is_valid() == true`
         */
        constexpr Variant& operator=(Variant&& v) noexcept
        {
            if (this == &v) return *this;
            std::destroy_at(this);
            std::move(v).visit([this]<typename TAlternative>(TAlternative&& val) {
                using U = std::remove_cvref_t<TAlternative>;
                std::construct_at(reinterpret_cast<U*>(this), std::forward<TAlternative>(val));
            });
            return *this;
        }

        /**
         * @brief Converting copy assignment — stores a copy of @p u.
         *
         * When the active alternative is already `U`, delegates to `U::operator=(const U&)`
         * directly.  This correctly handles self-assignment (e.g. `v = xll::get<U>(v)`)
         * because `U`'s own assignment operator is responsible for aliasing safety.
         *
         * When the active alternative is a different type, `u` cannot alias `*this`, so
         * `std::destroy_at` + `std::construct_at` is safe.
         *
         * @tparam U  The alternative type to assign.  Must be in `T, Ts...`.
         * @param  u  The value to copy.
         * @return    Reference to `*this`.
         *
         * @post `holds_alternative<U>(*this) == true`
         * @post `is_valid() == true`
         */
        template<typename U>
            requires(std::same_as<U, T> || (std::same_as<U, Ts> || ...))
        constexpr Variant& operator=(const U& u)
        {
            if (xll::holds_alternative<U>(*this))
                xll::get<U>(*this) = u;             // U is the active type: delegate to U::operator=,
            else {                                  // which handles aliasing and self-assignment correctly
                std::destroy_at(this);              // u cannot alias *this here (active type != U)
                std::construct_at(reinterpret_cast<U*>(this), u);
            }
            return *this;
        }

        /**
         * @brief Converting move assignment — moves @p u into the Variant.
         *
         * Same aliasing logic as the copy overload.  For `xll::String`, the heap buffer
         * is stolen when the target is a different type; when the active alternative is
         * already `String`, `String::operator=(String&&)` handles the move.
         *
         * @tparam U  The alternative type to assign.  Must be in `T, Ts...`.
         * @param  u  The value to move from.
         * @return    Reference to `*this`.
         *
         * @post `holds_alternative<U>(*this) == true`
         * @post `is_valid() == true`
         */
        template<typename U>
            requires(std::same_as<U, T> || (std::same_as<U, Ts> || ...))
        constexpr Variant& operator=(U&& u) noexcept
        {
            if (xll::holds_alternative<U>(*this))
                xll::get<U>(*this) = std::forward<U>(u);  // U is the active type: delegate to U::operator=
            else {                                  // u cannot alias *this here (active type != U)
                std::destroy_at(this);
                std::construct_at(reinterpret_cast<U*>(this), std::forward<U>(u));
            }
            return *this;
        }

        // ------------------------------------------------------------------
        // Observers
        // ------------------------------------------------------------------

        /**
         * @brief Returns the zero-based index of the active alternative.
         *
         * Uses a short-circuit `||` fold over `T, Ts...`; each step increments a counter
         * until the active type is found, then stops — no wasted iterations.
         *
         * @return The position of the active type in `T, Ts...` (0-based), or
         *         `std::variant_npos` if `xltype` does not match any declared alternative
         *         (indicates corrupted or externally-mutated storage).
         *
         * @note `std::variant_npos` is `static_cast<std::size_t>(-1)`.
         */
        [[nodiscard]] constexpr std::size_t index() const noexcept
        {
            std::size_t i = 0;
            auto try_type = [&]<typename U>() -> bool {
                if (xll::holds_alternative<U>(*this)) return true;
                ++i;
                return false;
            };
            const bool found = (try_type.template operator()<T>() ||
                                (try_type.template operator()<Ts>() || ...));
            return found ? i : std::variant_npos;
        }

        // ------------------------------------------------------------------
        // Modifiers
        // ------------------------------------------------------------------

        /**
         * @brief Destroys the current value and constructs a new `U` in-place.
         *
         * Equivalent to `std::variant::emplace<U>`.  Destroys the active alternative
         * first, then constructs `U` from `args...` directly in the Variant's storage.
         *
         * @tparam U     The alternative type to emplace.  Must be in `T, Ts...`.
         * @tparam Args  Constructor argument types for `U`.
         * @param  args  Arguments forwarded to `U`'s constructor.
         * @return       A reference to the newly constructed `U`.
         *
         * @post `holds_alternative<U>(*this) == true`
         * @post `is_valid() == true`
         *
         * @warning If `U`'s constructor throws, the Variant is left in a destroyed,
         *          invalid state (`is_valid()` returns `false`).  This is analogous to
         *          `std::variant`'s "valueless by exception" state.
         *
         * @note The returned reference addresses the live `U` object inside the Variant.
         *       It remains valid until the next non-const operation on the Variant.
         */
        template<typename U, typename... Args>
            requires(std::same_as<U, T> || (std::same_as<U, Ts> || ...))
        constexpr U& emplace(Args&&... args)
        {
            std::destroy_at(this);
            std::construct_at(reinterpret_cast<U*>(this), std::forward<Args>(args)...);
            return *std::launder(reinterpret_cast<U*>(this));
        }

        /**
         * @brief Exchanges the contents of `*this` and @p other.
         *
         * Two paths are taken depending on the active alternatives:
         *
         * **Same-type path** — both hold the same alternative `U`:
         * Delegates to `U`'s own `swap` via the `using std::swap; swap(a, b)` idiom.
         * This avoids a destroy+construct cycle and, for `xll::String`, exchanges the
         * heap buffer pointers without any allocation.
         *
         * **Different-type path** — the two Variants hold different alternatives:
         * Moves through a temporary using the existing move constructor and move
         * assignment operator.
         *
         * @param other  The Variant to swap with.
         *
         * @post `this` holds what `other` previously held, and vice versa.
         *
         * @note The `noexcept` specification is `true` iff every alternative type is
         *       both nothrow-move-constructible and nothrow-swappable — the same
         *       condition as `std::variant::swap`.
         */
        constexpr void swap(Variant& other)
            noexcept((std::is_nothrow_move_constructible_v<T> && std::is_nothrow_swappable_v<T>) &&
                     ((std::is_nothrow_move_constructible_v<Ts> && std::is_nothrow_swappable_v<Ts>) && ...))
        {
            if (this == &other) return;

            constexpr auto MASK = static_cast<decltype(xltype)>(~(xlbitDLLFree | xlbitXLFree));
            if ((xltype & MASK) == (other.xltype & MASK))
            {
                auto do_swap = [&]<typename U>() {
                    if (xll::holds_alternative<U>(*this)) {
                        using std::swap;
                        swap(xll::get<U>(*this), xll::get<U>(other));
                    }
                };
                (do_swap.template operator()<T>(), (do_swap.template operator()<Ts>(), ...));
            }
            else
            {
                Variant tmp(std::move(other));
                other = std::move(*this);
                *this = std::move(tmp);
            }
        }

        // ------------------------------------------------------------------
        // Value access (member, via deducing-this)
        // ------------------------------------------------------------------

        /**
         * @brief Returns a reference to the stored `U`, throwing on type mismatch.
         *
         * Uses the C++23 "deducing this" feature (`this Self&& self`) so that a single
         * definition handles all four cv/ref combinations:
         *
         * | Call site         | `Self` deduced as     | Return type  |
         * |-------------------|-----------------------|--------------|
         * | `v.get<U>()`      | `Variant&`            | `U&`         |
         * | `cv.get<U>()`     | `const Variant&`      | `const U&`   |
         * | `std::move(v).get<U>()` | `Variant&&`    | `U&&`        |
         * | `std::move(cv).get<U>()` | `const Variant&&` | `const U&&` |
         *
         * `std::forward_like<Self>` propagates `Self`'s cv/ref to the return type.
         * `std::launder` ensures provenance-correct access to the object placed by
         * `std::construct_at`.
         *
         * @tparam U     The alternative type to retrieve.  Must be in `T, Ts...`.
         * @tparam Self  Deduced explicit-object parameter; controls the return value category.
         * @return       A reference to the stored `U` with the same cv/ref as `Self`.
         * @throws std::bad_variant_access if the active alternative is not `U`.
         *
         * @note Prefer calling via the free function `xll::get<U>(v)` at call sites,
         *       which delegates here and maintains the `std::variant`-like interface.
         */
        template<typename U, typename Self>
            requires(std::same_as<U, T> || (std::same_as<U, Ts> || ...))
        [[nodiscard]] constexpr decltype(auto) get(this Self&& self)
        {
            if (!xll::holds_alternative<U>(self)) throw std::bad_variant_access();
            using CvU = std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, const U, U>;
            return std::forward_like<Self>(*std::launder(reinterpret_cast<CvU*>(&self)));
        }

        /**
         * @brief Applies @p vis to the active alternative and returns the result.
         *
         * Uses the C++23 "deducing this" feature so that a single definition handles all
         * four cv/ref combinations.  `std::forward<Self>(self).template get<U>()` is
         * used to forward the correct value category to the visitor argument.
         *
         * The `if constexpr` guards ensure that only branches for types actually in
         * `T, Ts...` are instantiated; the `holds_alternative` runtime checks dispatch
         * to the single active branch.
         *
         * @tparam Visitor  A callable whose call operator is invocable with every
         *                  alternative type in `T, Ts...`.  All overloads must return
         *                  the same type.
         * @tparam Self     Deduced explicit-object parameter; controls the value category
         *                  of the alternative reference passed to @p vis.
         * @param  vis      The visitor callable, forwarded once to the active branch.
         * @return          The return value of `vis(active_alternative)`.
         * @throws std::bad_variant_access if `xltype` matches none of the declared
         *                  alternatives (indicates corrupted or externally-mutated storage).
         *
         * @note Prefer calling via the free function `xll::visit(vis, v)` at call sites.
         * @note Use `xll::overload{...}` to dispatch to per-type lambda handlers.
         */
        template<typename Visitor, typename Self>
        [[nodiscard]] constexpr decltype(auto) visit(this Self&& self, Visitor&& vis)
        {
            if constexpr (std::same_as<xll::Bool, T> || (std::same_as<xll::Bool, Ts> || ...))
                if (xll::holds_alternative<xll::Bool>(self))
                    return std::invoke(std::forward<Visitor>(vis), std::forward<Self>(self).template get<xll::Bool>());

            if constexpr (std::same_as<xll::Error, T> || (std::same_as<xll::Error, Ts> || ...))
                if (xll::holds_alternative<xll::Error>(self))
                    return std::invoke(std::forward<Visitor>(vis), std::forward<Self>(self).template get<xll::Error>());

            if constexpr (std::same_as<xll::Int, T> || (std::same_as<xll::Int, Ts> || ...))
                if (xll::holds_alternative<xll::Int>(self))
                    return std::invoke(std::forward<Visitor>(vis), std::forward<Self>(self).template get<xll::Int>());

            if constexpr (std::same_as<xll::Number, T> || (std::same_as<xll::Number, Ts> || ...))
                if (xll::holds_alternative<xll::Number>(self))
                    return std::invoke(std::forward<Visitor>(vis), std::forward<Self>(self).template get<xll::Number>());

            if constexpr (std::same_as<xll::String, T> || (std::same_as<xll::String, Ts> || ...))
                if (xll::holds_alternative<xll::String>(self))
                    return std::invoke(std::forward<Visitor>(vis), std::forward<Self>(self).template get<xll::String>());

            if constexpr (std::same_as<xll::Nil, T> || (std::same_as<xll::Nil, Ts> || ...))
                if (xll::holds_alternative<xll::Nil>(self))
                    return std::invoke(std::forward<Visitor>(vis), std::forward<Self>(self).template get<xll::Nil>());

            throw std::bad_variant_access();
        }
    };

    // =========================================================================
    // Free-function definitions
    // =========================================================================

    /**
     * @brief Returns `true` if the active alternative of @p v is `U`.
     *
     * Masks `xlbitDLLFree` and `xlbitXLFree` before comparing so that Excel-owned
     * values (with ownership bits set on `xltype`) are handled identically to
     * library-owned values.  This is the same masking performed by `Any::type()` and
     * `Base::is_valid()`.
     *
     * @tparam U     The alternative type to test.  Must be one of `T, Ts...`.
     * @tparam T     First alternative of the Variant (deduced).
     * @tparam Ts    Remaining alternatives (deduced).
     * @param  v     The Variant to inspect.
     * @return `(v.xltype & ~ownership_bits) == U::excel_type`
     */
    template<typename U, typename T, typename... Ts>
        requires (std::same_as<U, T> || (std::same_as<U, Ts> || ...))
    constexpr bool holds_alternative(const Variant<T, Ts...>& v) noexcept
    {
        constexpr auto MASK = static_cast<int>(~(xlbitDLLFree | xlbitXLFree));
        return (v.xltype & MASK) == static_cast<int>(U::excel_type);
    }

    /**
     * @brief Returns a reference to the active `U` in @p v, throwing on mismatch.
     *
     * Thin wrapper that delegates to the member `get<U>` via `std::forward<TVariant>(v)`,
     * which propagates the correct value category to `std::forward_like` inside the
     * member.  The four cv/ref combinations are handled by a single template:
     *
     * ```cpp
     * xll::Variant<xll::Number, xll::String> v = xll::Number(3.14);
     *
     * xll::Number&       n1 = xll::get<xll::Number>(v);              // U&
     * const xll::Number& n2 = xll::get<xll::Number>(std::as_const(v)); // const U&
     * xll::Number&&      n3 = xll::get<xll::Number>(std::move(v));   // U&&
     * ```
     *
     * @tparam U        The alternative type to retrieve.  Must be in `T, Ts...`.
     * @tparam TVariant The cv/ref-qualified Variant type (deduced from the argument).
     * @param  v        The Variant to access.
     * @return A reference to the stored `U` with the same cv/ref qualifications as @p v.
     * @throws std::bad_variant_access if the active alternative is not `U`.
     */
    template<typename U, typename TVariant>
        requires impl::variant_member<U, std::remove_cvref_t<TVariant>>
    [[nodiscard]] constexpr decltype(auto) get(TVariant&& v)
    {
        return std::forward<TVariant>(v).template get<U>();
    }

    /**
     * @brief Returns an `Optional<U>` containing the active value, or `xll::None`.
     *
     * Never throws.  When `holds_alternative<U>(v)` is true, constructs an engaged
     * `Optional<U>` from `get<U>(std::forward<TVariant>(v))`.  The forwarding ensures:
     *   - Lvalue Variants copy the value into the Optional.
     *   - Rvalue Variants move the value into the Optional (e.g. steals `String` buffer).
     *
     * ```cpp
     * xll::Variant<xll::Number, xll::String> v = xll::Number(42.0);
     *
     * auto opt_num = xll::get_if<xll::Number>(v);   // Optional<Number> — engaged
     * auto opt_str = xll::get_if<xll::String>(v);   // Optional<String> — xll::None
     *
     * if (opt_num) std::cout << static_cast<double>(*opt_num) << "\n";  // 42.0
     * ```
     *
     * @tparam U        The alternative type to retrieve.  Must be in `T, Ts...`.
     * @tparam TVariant The cv/ref-qualified Variant type (deduced).
     * @param  v        The Variant to inspect.
     * @return `Optional<U>{ get<U>(v) }` if `holds_alternative<U>(v)`, else `Optional<U>{}`.
     */
    template<typename U, typename TVariant>
        requires impl::variant_member<U, std::remove_cvref_t<TVariant>>
    [[nodiscard]] constexpr Optional<U> get_if(TVariant&& v)
    {
        if (holds_alternative<U>(v))
            return Optional<U>{ get<U>(std::forward<TVariant>(v)) };
        return Optional<U>{};
    }

    /**
     * @brief Applies @p vis to the active alternative of @p va.
     *
     * Thin wrapper that delegates to the member `visit` via `std::forward<TVariant>(va)`,
     * propagating the correct value category so the visitor receives:
     *   - `U&`         when @p va is an lvalue
     *   - `const U&`   when @p va is a const lvalue
     *   - `U&&`        when @p va is an rvalue (allows the visitor to move out)
     *   - `const U&&`  when @p va is a const rvalue
     *
     * ```cpp
     * xll::Variant<xll::Number, xll::String> v = xll::String("hello");
     *
     * // Visit with an overload set
     * std::string result = xll::visit(xll::overload{
     *     [](const xll::Number& n) { return std::to_string(static_cast<double>(n)); },
     *     [](const xll::String& s) { return std::string(s); }
     * }, v);
     * // result == "hello"
     *
     * // Visit on rvalue — move the String out
     * std::string stolen;
     * xll::visit(xll::overload{
     *     [&stolen](xll::String&& s) { stolen = std::string(std::move(s)); },
     *     [](auto&&) {}
     * }, std::move(v));
     * ```
     *
     * @tparam Visitor  A callable whose call operator is invocable with every
     *                  alternative in `T, Ts...`.  All overloads must return the same
     *                  type (or types with a common type).
     * @tparam TVariant The cv/ref-qualified Variant type (deduced).
     * @param  vis      The visitor callable.  Forwarded to the active-alternative branch.
     * @param  va       The Variant to visit.
     * @return The return value of `vis(active_alternative)`.
     * @throws std::bad_variant_access if the stored `xltype` matches none of the declared
     *         alternatives (indicates corrupted or externally-mutated storage).
     */
    template<typename Visitor, typename TVariant>
        requires impl::variant_specialization<std::remove_cvref_t<TVariant>>
    [[nodiscard]] constexpr decltype(auto) visit(Visitor&& vis, TVariant&& va)
    {
        return std::forward<TVariant>(va).visit(std::forward<Visitor>(vis));
    }

}    // namespace xll

