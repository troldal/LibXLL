/**
 * @file Optional.hpp
 * @brief std::optional-like monadic container for xll types, built on the XLOPER12 memory layout.
 *
 * @section design Design Overview
 *
 * xll::Optional<TValue> is modelled after std::optional but operates entirely within the
 * constraints of the Excel SDK's XLOPER12 binary layout.  Like xll::Expected, it inherits
 * directly from XLOPER12, adds no data members, and uses the last 2 bytes of the XLOPER12.val
 * union as metadata (magic sentinel + "has-value" flag) to track the engaged/disengaged state.
 *
 * The "disengaged" (empty) state is represented by a default-constructed xll::Nil object
 * occupying the same storage.  This means:
 *   - sizeof(Optional<T>) == sizeof(XLOPER12)  (always 16 bytes)
 *   - An Optional can be passed directly to Excel as a raw XLOPER12 pointer.
 *   - The disengaged state is visible to Excel as xltypeNil.
 *
 * @section nullopt_analogue xll::Nullopt and xll::None
 *
 * xll::Nullopt is a tag type (analogous to std::nullopt_t) used to explicitly construct
 * or assign an Optional to the empty state.  The global constant xll::None (analogous to
 * std::nullopt) is the only instance you ever need to use:
 *
 * ```cpp
 * xll::Optional<xll::Number> a = xll::None;   // empty
 * xll::Optional<xll::Number> b = 42.0;        // engaged
 * b = xll::None;                               // now empty again
 * ```
 *
 * @section monadic Monadic Operations
 *
 * Optional supports the same monadic operations as std::optional (C++23 style):
 *   - transform(f)   – apply f to the value if present, return Optional<result>
 *   - and_then(f)    – f must itself return an Optional; propagates the empty state
 *   - or_else(f)     – call f (taking no arguments, returning Optional) if empty
 *
 * @section pointer_provenance Pointer Provenance and std::launder
 *
 * The same rules as xll::Expected apply: after constructing TValue via std::construct_at we
 * must use std::launder to obtain a pointer with valid provenance before dereferencing.
 *
 * @author Kenneth Troldal Balslev
 * @date 24/02/2026
 */

#pragma once

#include "../ExcelSDK/xlcall.hpp"
#include "Bool.hpp"
#include "Int.hpp"
#include "Nil.hpp"
#include "Number.hpp"
#include "String.hpp"
#include "Utils/Concepts.hpp"
#include "Utils/Ensure.hpp"
#include "Utils/Metadata.hpp"
#include <cstddef>
#include <optional>        // for std::bad_optional_access
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace xll
{
    // =========================================================================
    // Nullopt tag type — analogous to std::nullopt_t
    // =========================================================================

    /**
     * @brief Tag type used to construct or assign an xll::Optional to the empty state.
     *
     * Analogous to std::nullopt_t.  Use the global constant xll::None rather than
     * constructing a Nullopt object directly.
     *
     * @see xll::None
     */
    struct Nullopt
    {
        /// Explicit constructor — prevents accidental construction from, e.g., 0.
        constexpr explicit Nullopt(int) noexcept {}
    };

    /**
     * @brief Global constant representing the empty (disengaged) state for xll::Optional.
     *
     * Analogous to std::nullopt.  Use this to construct or assign an Optional to the
     * empty state:
     *
     * ```cpp
     * xll::Optional<xll::Number> opt = xll::None;
     * opt = xll::None;  // reset to empty
     * ```
     */
    inline constexpr Nullopt None{0};

    // =========================================================================
    // xll::Optional
    // =========================================================================

    /**
     * @brief A std::optional-like container for xll types that maintains XLOPER12 binary layout.
     *
     * Optional<TValue> either holds a value of type TValue (engaged state) or is empty
     * (disengaged state).  In the disengaged state the underlying XLOPER12 contains a
     * default-constructed xll::Nil, making the empty state visible to Excel as xltypeNil.
     *
     * ## Type Constraints
     *
     * TValue must satisfy the is_xll_type concept:
     *   - Inherits from XLOPER12 (directly or via impl::Base<…>)
     *   - Has a static excel_type member
     *   - sizeof(TValue) == sizeof(XLOPER12)
     *
     * Valid types: xll::Number, xll::String, xll::Int, xll::Bool, xll::Error,
     *              xll::Nil, xll::Array<T>, xll::Variant<T,Ts…>
     *
     * ## Memory Layout
     *
     * ```
     * Engaged:    [xltype=TValue::excel_type][val: TValue data        ][0xE7][0x00]
     * Disengaged: [xltype=xltypeNil         ][val: Nil data (all zero)][0xE7][0x01]
     * ```
     *
     * The last 2 bytes of the val union store metadata:
     *   - Byte [-2] = 0xE7  (magic sentinel)
     *   - Byte [-1] = 0x00  (engaged) or 0x01 (disengaged)
     *
     * @tparam TValue The type of value to optionally hold.
     */
    template<typename TValue>
        requires is_xll_type<TValue>
    class Optional final : public XLOPER12
    {
        static_assert(sizeof(TValue) <= sizeof(XLOPER12),
            "TValue is too large to fit in XLOPER12");
        static_assert(alignof(TValue) <= alignof(XLOPER12),
            "TValue alignment is incompatible with XLOPER12");
        static_assert(!std::is_polymorphic_v<TValue>,
            "TValue cannot have virtual functions (vtable would be corrupted)");

        // ------------------------------------------------------------------
        // Internal helpers — mirror the pattern from xll::Expected
        // ------------------------------------------------------------------

        /// Mark this object as engaged (has-value) in metadata.
        constexpr void set_engaged() noexcept
        {
            impl::set_error_state(*this, false);   // "not error" == engaged
        }

        /// Mark this object as disengaged (empty) in metadata.
        constexpr void set_disengaged() noexcept
        {
            impl::set_error_state(*this, true);    // "error" == disengaged
        }

        /// True if the metadata says this object is disengaged.
        [[nodiscard]]
        constexpr bool is_disengaged() const noexcept
        {
            if (impl::has_metadata(*this))
                return impl::is_error_state(*this);
            // Fallback for raw XLOPER12 from Excel: xltypeNil → disengaged.
            return (xltype & ~(xlbitDLLFree | xlbitXLFree)) == xltypeNil;
        }

    public:
        using value_type = TValue;

        // ------------------------------------------------------------------
        // Constructors
        // ------------------------------------------------------------------

        /**
         * @brief Default constructor — creates an empty (disengaged) Optional.
         *
         * The underlying storage is initialised with a default-constructed xll::Nil.
         *
         * @post has_value() == false
         * @post xltype == xltypeNil
         */
        constexpr Optional() : XLOPER12()
        {
            std::construct_at(reinterpret_cast<xll::Nil*>(this));
            set_disengaged();
        }

        /**
         * @brief Construct an empty Optional from xll::None (Nullopt tag).
         *
         * Provided so that `Optional<T> opt = xll::None;` compiles.
         *
         * @post has_value() == false
         */
        constexpr Optional(Nullopt) noexcept : Optional() {}  // NOLINT

        /**
         * @brief Copy constructor.
         *
         * If @p other is engaged, copies the TValue.
         * If @p other is disengaged, copies the empty state.
         */
        constexpr Optional(const Optional& other) : XLOPER12()
        {
            if (other.has_value()) {
                std::construct_at(reinterpret_cast<TValue*>(this),
                    *std::launder(reinterpret_cast<const TValue*>(&other)));
                set_engaged();
            }
            else {
                std::construct_at(reinterpret_cast<xll::Nil*>(this));
                set_disengaged();
            }
        }

        /**
         * @brief Move constructor.
         *
         * If @p other is engaged, moves the TValue.
         * If @p other is disengaged, creates an empty Optional.
         */
        constexpr Optional(Optional&& other) noexcept : XLOPER12()
        {
            if (other.has_value()) {
                std::construct_at(reinterpret_cast<TValue*>(this),
                    std::move(*std::launder(reinterpret_cast<TValue*>(&other))));
                set_engaged();
            }
            else {
                std::construct_at(reinterpret_cast<xll::Nil*>(this));
                set_disengaged();
            }
        }

        /**
         * @brief Converting constructor from TValue (copy).
         *
         * Constructs an engaged Optional containing a copy of @p t.
         *
         * @post has_value() == true
         */
        constexpr Optional(const TValue& t) : XLOPER12()    // NOLINT
        {
            std::construct_at(reinterpret_cast<TValue*>(this), t);
            set_engaged();
        }

        /**
         * @brief Converting constructor from TValue (move).
         *
         * Constructs an engaged Optional by moving @p t.
         *
         * @post has_value() == true
         */
        constexpr Optional(TValue&& t) noexcept : XLOPER12()    // NOLINT
        {
            std::construct_at(reinterpret_cast<TValue*>(this), std::move(t));
            set_engaged();
        }

        /**
         * @brief Converting constructor from a type U constructible to TValue (copy).
         *
         * Allows e.g. `Optional<Number> opt = 3.14;`.
         */
        template<typename U, typename UBase = std::remove_cvref_t<U>>
            requires std::constructible_from<TValue, U> &&
                     (!std::same_as<TValue,   UBase>) &&
                     (!std::same_as<Optional, UBase>) &&
                     (!std::same_as<Nullopt,  UBase>)
        constexpr Optional(const U& u) : XLOPER12()    // NOLINT
        {
            std::construct_at(reinterpret_cast<TValue*>(this), u);
            set_engaged();
        }

        /**
         * @brief Converting constructor from a type U constructible to TValue (move).
         */
        template<typename U, typename UBase = std::remove_cvref_t<U>>
            requires std::constructible_from<TValue, U> &&
                     (!std::same_as<TValue,   UBase>) &&
                     (!std::same_as<Optional, UBase>) &&
                     (!std::same_as<Nullopt,  UBase>)
        constexpr Optional(U&& u) noexcept(std::is_nothrow_constructible_v<TValue, U>) : XLOPER12()    // NOLINT
        {
            std::construct_at(reinterpret_cast<TValue*>(this), std::forward<U>(u));
            set_engaged();
        }

        // ------------------------------------------------------------------
        // Destructor
        // ------------------------------------------------------------------

        /**
         * @brief Destructor.
         *
         * Destroys whichever object (TValue or xll::Nil) currently occupies the storage,
         * then clears metadata for Excel compatibility.
         */
        constexpr ~Optional()
        {
            if (has_value())
                std::destroy_at(std::launder(reinterpret_cast<TValue*>(this)));
            else
                std::destroy_at(std::launder(reinterpret_cast<xll::Nil*>(this)));

            impl::clear_metadata(*this);
        }

        // ------------------------------------------------------------------
        // Assignment operators
        // ------------------------------------------------------------------

        /**
         * @brief Copy assignment operator.
         */
        constexpr Optional& operator=(const Optional& other)
        {
            Optional temp(other);
            swap(temp);
            return *this;
        }

        /**
         * @brief Move assignment operator.
         */
        constexpr Optional& operator=(Optional&& other) noexcept
        {
            Optional temp(std::move(other));
            swap(temp);
            return *this;
        }

        /**
         * @brief Assign xll::None — resets the Optional to the empty state.
         */
        constexpr Optional& operator=(Nullopt) noexcept
        {
            Optional temp;   // default-constructed = disengaged
            swap(temp);
            return *this;
        }

        /**
         * @brief Assign a TValue — sets the Optional to the engaged state.
         */
        constexpr Optional& operator=(const TValue& value)
        {
            Optional temp(value);
            swap(temp);
            return *this;
        }

        /**
         * @brief Assign a TValue (move) — sets the Optional to the engaged state.
         */
        constexpr Optional& operator=(TValue&& value)
            noexcept(std::is_nothrow_move_constructible_v<TValue>)
        {
            Optional temp(std::move(value));
            swap(temp);
            return *this;
        }

        /**
         * @brief Assign from a type U convertible to TValue (copy).
         */
        template<typename U, typename UBase = std::remove_cvref_t<U>>
            requires std::constructible_from<TValue, U> &&
                     (!std::same_as<TValue,   UBase>) &&
                     (!std::same_as<Optional, UBase>) &&
                     (!std::same_as<Nullopt,  UBase>)
        constexpr Optional& operator=(const U& u)
        {
            Optional temp(u);
            swap(temp);
            return *this;
        }

        /**
         * @brief Assign from a type U convertible to TValue (move).
         */
        template<typename U, typename UBase = std::remove_cvref_t<U>>
            requires std::constructible_from<TValue, U> &&
                     (!std::same_as<TValue,   UBase>) &&
                     (!std::same_as<Optional, UBase>) &&
                     (!std::same_as<Nullopt,  UBase>)
        constexpr Optional& operator=(U&& u)
            noexcept(std::is_nothrow_constructible_v<TValue, U>)
        {
            Optional temp(std::forward<U>(u));
            swap(temp);
            return *this;
        }

        // ------------------------------------------------------------------
        // State queries
        // ------------------------------------------------------------------

        /**
         * @brief Returns true if the Optional holds a value (is engaged).
         *
         * Uses metadata when present; falls back to xltype == xltypeNil for raw
         * XLOPER12 objects received from Excel.
         */
        [[nodiscard]]
        constexpr bool has_value() const noexcept { return !is_disengaged(); }

        /**
         * @brief Boolean conversion — true iff engaged.
         */
        [[nodiscard]]
        constexpr explicit operator bool() const noexcept { return has_value(); }

        // ------------------------------------------------------------------
        // Value access
        // ------------------------------------------------------------------

        /**
         * @brief Returns a forwarding reference to the contained value.
         *
         * Uses C++23 deducing this to cover all cv-ref overloads in one template.
         * Uses std::launder to obtain valid pointer provenance after std::construct_at.
         *
         * @throws std::bad_optional_access if the Optional is disengaged.
         */
        template<typename Self>
        [[nodiscard]]
        constexpr auto&& value(this Self&& self)
        {
            if (!self.has_value())
                throw std::bad_optional_access{};

            using QualifiedValue = std::conditional_t<
                std::is_const_v<std::remove_reference_t<Self>>,
                const TValue,
                TValue>;

            auto* ptr = std::launder(reinterpret_cast<QualifiedValue*>(&self));
            return std::forward_like<Self>(*ptr);
        }

        /**
         * @brief Dereference operator — unchecked access to the contained value.
         *
         * @pre has_value() == true (behaviour is undefined if disengaged).
         */
        template<typename Self>
        [[nodiscard]]
        constexpr auto&& operator*(this Self&& self)
        {
            ensure(self.has_value(), "Dereferencing a disengaged xll::Optional");
            using QualifiedValue = std::conditional_t<
                std::is_const_v<std::remove_reference_t<Self>>,
                const TValue,
                TValue>;
            return std::forward_like<Self>(*std::launder(reinterpret_cast<QualifiedValue*>(&self)));
        }

        /**
         * @brief Arrow operator — pointer-like access to the contained value.
         *
         * @pre has_value() == true
         */
        [[nodiscard]]
        constexpr const TValue* operator->() const noexcept
        {
            ensure(has_value(), "operator-> called on a disengaged xll::Optional");
            return std::launder(reinterpret_cast<const TValue*>(this));
        }

        [[nodiscard]]
        constexpr TValue* operator->() noexcept
        {
            ensure(has_value(), "operator-> called on a disengaged xll::Optional");
            return std::launder(reinterpret_cast<TValue*>(this));
        }

        /**
         * @brief Returns the contained value or a default if disengaged.
         *
         * Uses C++23 deducing this: copies if called on lvalue, moves if called on rvalue.
         *
         * @tparam U Type of the default value (must be constructible to TValue).
         */
        template<typename Self, typename U = TValue>
            requires std::constructible_from<TValue, U>
        [[nodiscard]]
        constexpr TValue value_or(this Self&& self, U&& default_value)
        {
            return self.has_value()
                ? std::forward_like<Self>(self.value())
                : TValue(std::forward<U>(default_value));
        }

        // ------------------------------------------------------------------
        // Modifiers
        // ------------------------------------------------------------------

        /**
         * @brief Resets the Optional to the disengaged state.
         *
         * Equivalent to `opt = xll::None;`.
         */
        constexpr void reset() noexcept
        {
            Optional temp;   // disengaged
            swap(temp);
        }

        /**
         * @brief Constructs the value in-place, destroying any existing value first.
         *
         * Provides basic exception safety: if construction throws, the Optional is left
         * in the disengaged state.
         *
         * @tparam Args Argument types forwarded to TValue's constructor.
         */
        template<typename... Args>
            requires std::constructible_from<TValue, Args...>
        constexpr TValue& emplace(Args&&... args)
        {
            // Destroy whatever is currently in storage.
            if (has_value())
                std::destroy_at(std::launder(reinterpret_cast<TValue*>(this)));
            else
                std::destroy_at(std::launder(reinterpret_cast<xll::Nil*>(this)));

            try {
                std::construct_at(reinterpret_cast<TValue*>(this), std::forward<Args>(args)...);
                set_engaged();
            }
            catch (...) {
                std::construct_at(reinterpret_cast<xll::Nil*>(this));
                set_disengaged();
                throw;
            }

            return *std::launder(reinterpret_cast<TValue*>(this));
        }

        /**
         * @brief Clears the metadata tag for Excel compatibility.
         *
         * Call this before passing the Optional to Excel APIs so that Excel sees a plain
         * XLOPER12 without our metadata bytes.
         */
        constexpr void clear_metadata() noexcept { impl::clear_metadata(*this); }

        // ------------------------------------------------------------------
        // Swap
        // ------------------------------------------------------------------

        /**
         * @brief Swaps two Optional objects.
         *
         * Because Optional and TValue have identical layout to XLOPER12, swapping the
         * XLOPER12 base is sufficient — it exchanges xltype, the entire val union
         * (including TValue/Nil data), and the metadata bytes in one operation.
         */
        constexpr void swap(Optional& other)
            noexcept(std::is_nothrow_move_constructible_v<TValue>)
        {
            using std::swap;
            swap(static_cast<XLOPER12&>(*this), static_cast<XLOPER12&>(other));
        }

        // ------------------------------------------------------------------
        // Monadic operations  (std::optional C++23 interface)
        // ------------------------------------------------------------------

        /**
         * @brief Applies @p func to the contained value and wraps the result in an Optional.
         *
         * If the Optional is disengaged, returns a disengaged Optional<result_type>.
         * func must return an xll type or a type convertible to one.
         *
         * ```cpp
         * Optional<Number> opt = 42.0;
         * auto s = opt.transform([](Number& n) { return xll::String(std::to_string(n.val.num)); });
         * // s is Optional<String> containing "42"
         * ```
         */
        template<typename Self, typename Func>
            requires std::invocable<Func, TValue&>
        [[nodiscard]]
        constexpr auto transform(this Self&& self, Func&& func)
        {
            using ResultValue  = std::invoke_result_t<Func, TValue&>;
            using ResultOptional = Optional<ResultValue>;

            if (self.has_value())
                return ResultOptional(std::invoke(std::forward<Func>(func), self.value()));

            return ResultOptional(None);
        }

        /**
         * @brief Monadic bind — @p func must itself return an Optional<U>.
         *
         * If engaged, calls func(value()) and returns the result.
         * If disengaged, returns a disengaged Optional of the same result type.
         *
         * ```cpp
         * Optional<Number> opt = 10.0;
         * auto result = opt.and_then([](Number& n) -> Optional<Number> {
         *     if (n.val.num == 0) return xll::None;
         *     return Number(1.0 / n.val.num);
         * });
         * ```
         */
        template<typename Self, typename Func,
                 typename Result = std::invoke_result_t<Func, TValue&>>
            requires std::invocable<Func, TValue&> &&
                     requires { typename Result::value_type; } &&   // Result is Optional-like
                     std::same_as<Result, Optional<typename Result::value_type>>
        [[nodiscard]]
        constexpr Result and_then(this Self&& self, Func&& func)
        {
            if (self.has_value())
                return std::invoke(std::forward<Func>(func), self.value());

            return Result(None);
        }

        /**
         * @brief Calls @p func (no arguments) and returns its result if *this is disengaged.
         *
         * If engaged, returns *this (copied/moved).
         * func must return an Optional<TValue>.
         *
         * ```cpp
         * Optional<Number> opt = xll::None;
         * auto result = opt.or_else([]() -> Optional<Number> { return 0.0; });
         * ```
         */
        template<typename Self, typename Func,
                 typename Result = std::invoke_result_t<Func>>
            requires std::invocable<Func> &&
                     std::same_as<Result, Optional>
        [[nodiscard]]
        constexpr Optional or_else(this Self&& self, Func&& func)
        {
            if (!self.has_value())
                return std::invoke(std::forward<Func>(func));

            return Optional(std::forward_like<Self>(self.value()));
        }

        // ------------------------------------------------------------------
        // Equality
        // ------------------------------------------------------------------

        /**
         * @brief Two Optionals are equal if both are disengaged, or both are engaged
         *        with equal values.
         */
        [[nodiscard]]
        friend constexpr bool operator==(const Optional& lhs, const Optional& rhs)
            requires std::equality_comparable<TValue>
        {
            if (lhs.has_value() != rhs.has_value()) return false;
            if (!lhs.has_value()) return true;   // both disengaged
            return lhs.value() == rhs.value();
        }

        /**
         * @brief An Optional equals a TValue if it is engaged and its value equals @p rhs.
         */
        [[nodiscard]]
        friend constexpr bool operator==(const Optional& lhs, const TValue& rhs)
            requires std::equality_comparable<TValue>
        {
            return lhs.has_value() && lhs.value() == rhs;
        }

        /**
         * @brief An Optional equals xll::None iff it is disengaged.
         */
        [[nodiscard]]
        friend constexpr bool operator==(const Optional& lhs, Nullopt) noexcept
        {
            return !lhs.has_value();
        }

        // ------------------------------------------------------------------
        // Iterator support (C++26 range interface)
        //
        // An Optional<TValue> is treated as a range of zero or one elements.
        // If has_value() is true, the range contains the single TValue element;
        // otherwise it is empty.  This mirrors the C++26 iterator support
        // proposed for std::optional / std::expected.
        // ------------------------------------------------------------------

        /**
         * @brief Iterator for xll::Optional treated as a single-element range.
         *
         * Satisfies std::contiguous_iterator (pointer-based), so the resulting range
         * automatically satisfies std::ranges::contiguous_range.
         * The end sentinel is represented by a null pointer.
         */
        class iterator
        {
        public:
            using iterator_category = std::contiguous_iterator_tag;
            using iterator_concept  = std::contiguous_iterator_tag;
            using value_type        = TValue;
            using difference_type   = std::ptrdiff_t;
            using pointer           = TValue*;
            using reference         = TValue&;

            constexpr iterator() noexcept : ptr_(nullptr) {}
            constexpr explicit iterator(TValue* p) noexcept : ptr_(p) {}

            constexpr reference operator*()  const noexcept { return *ptr_; }
            constexpr pointer   operator->() const noexcept { return  ptr_; }

            constexpr iterator& operator++()    noexcept { ptr_ = nullptr; return *this; }
            constexpr iterator  operator++(int) noexcept { iterator t(*this); ++(*this); return t; }
            constexpr iterator& operator--()    noexcept { return *this; }
            constexpr iterator  operator--(int) noexcept { return *this; }

            constexpr iterator& operator+=(difference_type n) noexcept { if (n != 0) ptr_ = nullptr; return *this; }
            constexpr iterator& operator-=(difference_type n) noexcept { if (n != 0) ptr_ = nullptr; return *this; }
            constexpr iterator  operator+(difference_type n) const noexcept { iterator t(*this); t += n; return t; }
            constexpr iterator  operator-(difference_type n) const noexcept { iterator t(*this); t -= n; return t; }
            friend constexpr iterator operator+(difference_type n, iterator it) noexcept { return it + n; }
            constexpr difference_type operator-(const iterator& rhs) const noexcept
            {
                if (ptr_ == rhs.ptr_) return 0;
                return ptr_ == nullptr ? 1 : -1;
            }
            constexpr reference operator[](difference_type n) const noexcept { return *(*this + n); }

            constexpr bool operator==(const iterator& rhs) const noexcept
            {
                if (ptr_ == nullptr && rhs.ptr_ == nullptr) return true;
                return ptr_ == rhs.ptr_;
            }
            constexpr bool operator!=(const iterator& rhs) const noexcept { return !(*this == rhs); }
            constexpr auto operator<=>(const iterator& rhs) const noexcept
            {
                auto as_int = [](TValue* p) -> int { return p == nullptr ? 1 : 0; };
                return as_int(ptr_) <=> as_int(rhs.ptr_);
            }

            constexpr pointer base() const noexcept { return ptr_; }

        private:
            TValue* ptr_;
        };

        /**
         * @brief Const iterator for xll::Optional treated as a single-element range.
         */
        class const_iterator
        {
        public:
            using iterator_category = std::contiguous_iterator_tag;
            using iterator_concept  = std::contiguous_iterator_tag;
            using value_type        = TValue;
            using difference_type   = std::ptrdiff_t;
            using pointer           = const TValue*;
            using reference         = const TValue&;

            constexpr const_iterator() noexcept : ptr_(nullptr) {}
            constexpr explicit const_iterator(const TValue* p) noexcept : ptr_(p) {}
            constexpr const_iterator(iterator it) noexcept : ptr_(it.base()) {} // NOLINT implicit

            constexpr reference      operator*()  const noexcept { return *ptr_; }
            constexpr pointer        operator->() const noexcept { return  ptr_; }

            constexpr const_iterator& operator++()    noexcept { ptr_ = nullptr; return *this; }
            constexpr const_iterator  operator++(int) noexcept { const_iterator t(*this); ++(*this); return t; }
            constexpr const_iterator& operator--()    noexcept { return *this; }
            constexpr const_iterator  operator--(int) noexcept { return *this; }

            constexpr const_iterator& operator+=(difference_type n) noexcept { if (n != 0) ptr_ = nullptr; return *this; }
            constexpr const_iterator& operator-=(difference_type n) noexcept { if (n != 0) ptr_ = nullptr; return *this; }
            constexpr const_iterator  operator+(difference_type n) const noexcept { const_iterator t(*this); t += n; return t; }
            constexpr const_iterator  operator-(difference_type n) const noexcept { const_iterator t(*this); t -= n; return t; }
            friend constexpr const_iterator operator+(difference_type n, const_iterator it) noexcept { return it + n; }
            constexpr difference_type operator-(const const_iterator& rhs) const noexcept
            {
                if (ptr_ == rhs.ptr_) return 0;
                return ptr_ == nullptr ? 1 : -1;
            }
            constexpr reference operator[](difference_type n) const noexcept { return *(*this + n); }

            constexpr bool operator==(const const_iterator& rhs) const noexcept
            {
                if (ptr_ == nullptr && rhs.ptr_ == nullptr) return true;
                return ptr_ == rhs.ptr_;
            }
            constexpr bool operator!=(const const_iterator& rhs) const noexcept { return !(*this == rhs); }
            constexpr auto operator<=>(const const_iterator& rhs) const noexcept
            {
                auto as_int = [](const TValue* p) -> int { return p == nullptr ? 1 : 0; };
                return as_int(ptr_) <=> as_int(rhs.ptr_);
            }

            constexpr pointer base() const noexcept { return ptr_; }

        private:
            const TValue* ptr_;
        };

        /**
         * @brief Returns an iterator to the beginning of the range.
         *
         * If has_value() is true, returns an iterator to the contained TValue.
         * Otherwise returns an end iterator (equivalent to end()).
         */
        [[nodiscard]]
        constexpr iterator begin() noexcept
        {
            if (has_value())
                return iterator(std::launder(reinterpret_cast<TValue*>(this)));
            return iterator(nullptr);
        }

        [[nodiscard]]
        constexpr const_iterator begin() const noexcept
        {
            if (has_value())
                return const_iterator(std::launder(reinterpret_cast<const TValue*>(this)));
            return const_iterator(nullptr);
        }

        [[nodiscard]]
        constexpr const_iterator cbegin() const noexcept { return begin(); }

        /**
         * @brief Returns an end iterator (always the nullptr-based sentinel).
         */
        [[nodiscard]]
        constexpr iterator end() noexcept { return iterator(nullptr); }

        [[nodiscard]]
        constexpr const_iterator end() const noexcept { return const_iterator(nullptr); }

        [[nodiscard]]
        constexpr const_iterator cend() const noexcept { return end(); }

        /**
         * @brief Returns the number of elements in the range (0 or 1).
         */
        [[nodiscard]]
        constexpr std::size_t size() const noexcept { return has_value() ? 1u : 0u; }

        /**
         * @brief Returns true if the range is empty (Optional is disengaged).
         */
        [[nodiscard]]
        constexpr bool empty() const noexcept { return !has_value(); }

        /**
         * @brief Returns a pointer to the contained value, or nullptr if empty.
         */
        [[nodiscard]]
        constexpr TValue* data() noexcept
        {
            return has_value() ? std::launder(reinterpret_cast<TValue*>(this)) : nullptr;
        }

        [[nodiscard]]
        constexpr const TValue* data() const noexcept
        {
            return has_value() ? std::launder(reinterpret_cast<const TValue*>(this)) : nullptr;
        }
    };

    // ------------------------------------------------------------------
    // Non-member swap
    // ------------------------------------------------------------------

    /**
     * @brief ADL-friendly non-member swap for xll::Optional.
     */
    template<typename TValue>
    constexpr void swap(Optional<TValue>& lhs, Optional<TValue>& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

    // ------------------------------------------------------------------
    // Pipe operator and higher-order helpers (mirror xll::Expected style)
    // ------------------------------------------------------------------

    namespace impl
    {
        template<typename T>
        struct is_optional_impl : std::false_type {};

        template<typename T>
        struct is_optional_impl<Optional<T>> : std::true_type {};

        template<typename T>
        inline constexpr bool is_optional_v = is_optional_impl<std::remove_cvref_t<T>>::value;
    }

    /**
     * @brief Concept satisfied only by specialisations of xll::Optional.
     */
    template<typename T>
    concept IsOptional = impl::is_optional_v<T>;

    /**
     * @brief Pipe operator for chaining operations on xll::Optional.
     *
     * Enables fluent style:
     * ```cpp
     * auto r = opt | transform([](Number& n){ return n.val.num * 2; })
     *              | and_then([](Number& n) -> Optional<Number> { ... });
     * ```
     */
    template<typename TOptional, typename Callable>
        requires std::invocable<Callable, TOptional> &&
                 IsOptional<std::remove_cvref_t<TOptional>>
    [[nodiscard]]
    constexpr auto operator|(TOptional&& opt, Callable&& func)
        -> decltype(std::invoke(std::forward<Callable>(func), std::forward<TOptional>(opt)))
    {
        return std::invoke(std::forward<Callable>(func), std::forward<TOptional>(opt));
    }

    /**
     * @brief Higher-order helper for transform, usable with operator|.
     *
     * ```cpp
     * auto r = opt | xll::transform([](Number& n){ return n.val.num * 2.0; });
     * ```
     *
     * @note Defined as a free function in the xll namespace.  Because xll::Expected
     *       already defines a free xll::transform() for Expected objects, this overload
     *       is selected only when the left-hand side is an xll::Optional.
     */
    template<typename TFunction>
    [[nodiscard]]
    constexpr auto opt_transform(TFunction&& f)
    {
        return [f = std::forward<TFunction>(f)]<typename Self>(Self&& opt) {
            return std::forward<Self>(opt).transform(f);
        };
    }

    /**
     * @brief Higher-order helper for and_then, usable with operator|.
     */
    template<typename TFunction>
    [[nodiscard]]
    constexpr auto opt_and_then(TFunction&& f)
    {
        return [f = std::forward<TFunction>(f)]<typename Self>(Self&& opt) {
            return std::forward<Self>(opt).and_then(f);
        };
    }

    /**
     * @brief Higher-order helper for or_else, usable with operator|.
     */
    template<typename TFunction>
    [[nodiscard]]
    constexpr auto opt_or_else(TFunction&& f)
    {
        return [f = std::forward<TFunction>(f)]<typename Self>(Self&& opt) {
            return std::forward<Self>(opt).or_else(f);
        };
    }

    // ------------------------------------------------------------------
    // Convenience type aliases
    // ------------------------------------------------------------------

    using OptNumber = Optional<Number>;  ///< Optional<Number>
    using OptString = Optional<String>;  ///< Optional<String>
    using OptInt    = Optional<Int>;     ///< Optional<Int>
    using OptBool   = Optional<Bool>;    ///< Optional<Bool>

}    // namespace xll

// Opt-in to std::ranges::view for xll::Optional.
// An Optional is treated as an owning range of 0 or 1 elements; it is a
// view (O(1) copy/move) but NOT a borrowed range (iterators dangle if the
// Optional is destroyed).
namespace std::ranges
{
    template<typename TValue>
    inline constexpr bool enable_view<xll::Optional<TValue>> = true;
}    // namespace std::ranges
