/**
 * \file Base.hpp
 * \brief CRTP base class for Excel primitive type wrappers using type punning with XLOPER12
 *
 * This file implements a CRTP (Curiously Recurring Template Pattern) base class that wraps
 * Excel's primitive types (Number, Int, Bool, Error, String, etc.) by inheriting from XLOPER12.
 *
 * \section design Design Philosophy
 *
 * \subsection type_punning Type Punning via Inheritance
 * The Base class inherits from XLOPER12 without adding any data members. This design enables
 * zero-overhead type punning: a Base<Derived, xltypeNum> object IS an XLOPER12 object with
 * compile-time knowledge of which union member is active.
 *
 * Memory layout guarantee:
 * \code
 * sizeof(Base<D, xltypeNum>) == sizeof(XLOPER12)  // No size overhead
 * static_cast<XLOPER12*>(base_ptr) // Safe and lossless
 * \endcode
 *
 * This allows derived classes to be passed directly to Excel SDK functions expecting XLOPER12
 * pointers, eliminating the need for conversion functions and enabling seamless C API interop.
 *
 * \subsection crtp CRTP Pattern
 * The Curiously Recurring Template Pattern allows the base class to return derived types from
 * operations while avoiding virtual function overhead:
 * \code
 * Number a(5.0), b(3.0);
 * Number c = a + b;  // Returns Number, not Base
 * \endcode
 *
 * \subsection dry DRY Without Dynamic Polymorphism
 * Common functionality (arithmetic, comparison, conversion) is implemented once in Base and
 * inherited by all derived types (Number, Int, Bool, etc.) without using virtual functions,
 * which would change the memory layout and break XLOPER12 compatibility.
 *
 * The protected destructor follows C++ Core Guideline C.35: "A base class destructor should
 * be either public and virtual, or protected and non-virtual." Since XLOPER12 is a plain C
 * struct with no destructor, and we avoid dynamic polymorphism for memory layout reasons,
 * the destructor is protected to prevent deletion through base pointers.
 *
 * \section safety Type Safety
 *
 * All operations are constrained via C++20 concepts to ensure:
 * - Only arithmetic types support arithmetic operations
 * - Error types are excluded from comparisons and arithmetic
 * - Cross-type operations only allow compatible Excel types
 * - Runtime validation ensures xltype matches the template parameter
 *
 * \author Kenneth Balslev
 * \date 2025-03-23
 */

#pragma once

#include "../Utils/Ensure.hpp"
#include <iostream>
#include <type_traits>
#include <xlcall.hpp>

namespace xll::impl
{
    /**
     * \brief Compile-time check if a value is contained in a parameter pack
     * \tparam Value The value to search for
     * \tparam Values The parameter pack to search in
     * \return true if Value matches any value in Values, false otherwise
     */
    template<auto Value, auto... Values>
    constexpr bool contains_value = (... || (Value == Values));

    /**
     * \brief Concept constraining types to valid Excel type constants
     * \tparam T The type to check
     *
     * Ensures T is an integral type that matches one of the Excel xltype constants.
     */
    template<typename T>
    concept ExcelType = requires(T value) {
        requires std::is_integral_v<T>;
        requires(value == xltypeNum || value == xltypeStr || value == xltypeBool || value == xltypeRef || value == xltypeErr ||
                 value == xltypeFlow || value == xltypeMulti || value == xltypeMissing || value == xltypeNil || value == xltypeSRef ||
                 value == xltypeInt);
    };

    /**
     * \brief Compile-time validation that a value is a valid Excel type constant
     * \tparam Value The value to validate
     * \return true if Value is a valid Excel xltype constant, false otherwise
     */
    template<auto Value>
    constexpr bool is_excel_type = (Value == xltypeNum || Value == xltypeStr || Value == xltypeBool || Value == xltypeRef ||
                                    Value == xltypeErr || Value == xltypeFlow || Value == xltypeMulti || Value == xltypeMissing ||
                                    Value == xltypeNil || Value == xltypeSRef || Value == xltypeInt);

    // Forward declaration
    template<typename TDerived, size_t ValueType, size_t... OtherTypes>
        requires is_excel_type<ValueType>
    class Base;

    /**
     * \brief Free swap function for Base objects (ADL-enabled)
     * \tparam TDerived The CRTP derived class type
     * \tparam ValueType The primary Excel type for this specialization
     * \tparam OtherTypes Additional allowed Excel types for cross-type operations
     * \param lhs First object to swap
     * \param rhs Second object to swap
     *
     * This function provides efficient swapping of Base objects by directly swapping
     * the POD members (xltype and val union). It is noexcept and constexpr-compatible.
     *
     * \note This function does NOT validate object state before swapping, following
     * standard library conventions for swap operations. Both objects are assumed to
     * be in valid states (caller's responsibility).
     *
     * \warning The noexcept guarantee is critical for exception safety in algorithms
     * and should not be removed.
     */
    template<typename TDerived, size_t ValueType, size_t... OtherTypes>
    constexpr void swap(Base<TDerived, ValueType, OtherTypes...>& lhs, Base<TDerived, ValueType, OtherTypes...>& rhs) noexcept;

    /**
     * \brief CRTP base class for Excel primitive type wrappers using type punning
     *
     * \tparam TDerived The derived class (CRTP pattern)
     * \tparam XLType The primary Excel type this specialization represents (e.g., xltypeNum)
     * \tparam OtherTypes Additional Excel types allowed for cross-type operations
     *
     * This class inherits from XLOPER12 without adding data members, enabling zero-overhead
     * type punning. A Base object IS an XLOPER12 object with compile-time type information
     * about which union member is active.
     *
     * \section memory_layout Memory Layout and Type Punning
     *
     * By inheriting from XLOPER12 and adding no data members:
     * - sizeof(Base) == sizeof(XLOPER12)
     * - Pointer/reference conversions are safe and lossless
     * - Derived classes can be passed directly to Excel SDK C functions
     *
     * \section crtp_design CRTP Design
     *
     * The CRTP pattern enables:
     * - Type-safe operations returning the derived type
     * - No virtual function overhead
     * - Compile-time polymorphism
     *
     * \section thread_safety Thread Safety
     *
     * All const operations are thread-safe for read access.
     * Concurrent modification requires external synchronization.
     *
     * \note The destructor is protected per Core Guideline C.35 since XLOPER12
     * has no destructor and we avoid virtual functions to preserve memory layout.
     */
    template<typename TDerived, size_t XLType, size_t... OtherTypes>
        requires is_excel_type<XLType>
    class Base : public XLOPER12
    {
        /**
         * \brief CRTP helper to get derived class reference
         * \return Reference to the derived class instance
         */
        TDerived&       derived() { return static_cast<TDerived&>(*this); }

        /**
         * \brief CRTP helper to get const derived class reference
         * \return Const reference to the derived class instance
         */
        TDerived const& derived() const { return static_cast<TDerived const&>(*this); }

    protected:
        /**
         * \brief Protected destructor per C++ Core Guideline C.35
         *
         * The destructor is protected because:
         * 1. XLOPER12 is a plain C struct with no destructor
         * 2. We avoid virtual functions to preserve memory layout
         * 3. Prevents deletion through base class pointers
         *
         * Following C.35: "A base class destructor should be either public and virtual,
         * or protected and non-virtual."
         */
        constexpr ~Base() = default;

        /**
         * \brief Access the active union member with perfect forwarding
         * \tparam Self Deduced type (uses C++23 explicit object parameter)
         * \param self The object instance
         * \return Forwarding reference to the appropriate union member
         *
         * Uses C++23's "deducing this" to provide a single function that handles
         * both const and non-const access with perfect forwarding, eliminating
         * the need for separate overloads.
         *
         * \throws std::bad_cast if XLType doesn't match any known Excel type
         *
         * \note This is the central abstraction for type-safe union access.
         * It uses compile-time dispatch to select the correct union member.
         */
        template<typename Self>
        constexpr auto&& value(this Self&& self)
        {
            if constexpr (XLType == xltypeNum)
                return std::forward<Self>(self).val.num;
            else if constexpr (XLType == xltypeStr)
                return std::forward<Self>(self).val.str;
            else if constexpr (XLType == xltypeBool)
                return std::forward<Self>(self).val.xbool;
            else if constexpr (XLType == xltypeErr)
                return std::forward<Self>(self).val.err;
            else if constexpr (XLType == xltypeMulti)
                return std::forward<Self>(self).val.array;
            else if constexpr (XLType == xltypeInt)
                return std::forward<Self>(self).val.w;
            else
                throw std::bad_cast();
        }

    public:
        /**
         * \brief Compile-time flag indicating this class uses CRTP
         *
         * Used by template metaprogramming to detect CRTP-based classes.
         */
        static constexpr bool has_crtp_base = true;

        /**
         * \brief The Excel type this specialization represents
         *
         * Compile-time constant accessible as TOther::excel_type in templates.
         */
        static constexpr size_t excel_type = XLType;

        /**
         * \brief Validates that xltype matches the template parameter
         * \return true if the object is in a valid state, false otherwise
         *
         * Checks that the runtime xltype field (ignoring memory management bits)
         * matches the compile-time XLType template parameter.
         *
         * This is the fundamental invariant: a Base<D, xltypeNum> object should
         * always have xltype == xltypeNum (ignoring xlbitDLLFree and xlbitXLFree).
         *
         * \note This function is marked [[nodiscard]] because ignoring the result
         * defeats the purpose of validation.
         */
        [[nodiscard]]
        constexpr bool is_valid() const {
            constexpr auto TYPE_MASK = static_cast<decltype(xltype)>(~(xlbitDLLFree | xlbitXLFree));
            return (xltype & TYPE_MASK) == static_cast<decltype(xltype)>(XLType);
        }

        /** \brief Returns xltype with xlbitDLLFree and xlbitXLFree masked off. */
        [[nodiscard]]
        static constexpr auto base_xltype(decltype(XLOPER12::xltype) t) noexcept {
            constexpr auto TYPE_MASK = static_cast<decltype(t)>(~(xlbitDLLFree | xlbitXLFree));
            return t & TYPE_MASK;
        }

        /**
         * \brief Type alias for the active union member's type
         *
         * Provides compile-time access to the C++ type of the value stored in
         * the XLOPER12 union for this specialization.
         *
         * Examples:
         * - Base<D, xltypeNum>::value_type == double
         * - Base<D, xltypeInt>::value_type == int
         * - Base<D, xltypeBool>::value_type == BOOL
         *
         * This alias is used throughout the class for type-safe operations.
         */
        // clang-format off
        using value_type =
            std::conditional_t<XLType == xltypeNum, decltype(val.num),
            std::conditional_t<XLType == xltypeStr, decltype(val.str),
            std::conditional_t<XLType == xltypeBool, decltype(val.xbool),
            std::conditional_t<XLType == xltypeRef, decltype(val.mref),
            std::conditional_t<XLType == xltypeSRef, decltype(val.sref),
            std::conditional_t<XLType == xltypeErr, decltype(val.err),
            std::conditional_t<XLType == xltypeFlow, decltype(val.flow),
            std::conditional_t<XLType == xltypeMulti, decltype(val.array),
            std::conditional_t<XLType == xltypeInt, decltype(val.w), void>>>>>>>>>;
        // clang-format on

        /**
         * \brief Default constructor initializing xltype
         *
         * Constructs an XLOPER12 via default initialization, then sets xltype to XLType.
         * The union member (val) is left in an indeterminate state and must be initialized
         * before use.
         *
         * \post xltype == XLType
         * \post is_valid() == true
         *
         * \note This is safe because we're only setting the discriminator (xltype).
         * Derived class constructors or assignment operators must initialize the value.
         */
        constexpr Base() : XLOPER12() { xltype = XLType; }

        /**
         * \brief Constructs from an XLOPER12 lvalue reference (internal use only)
         * \param v The XLOPER12 object to copy from
         *
         * This constructor is intended for internal library use only, not public API.
         * It performs a shallow copy of the xltype and val union from an existing XLOPER12.
         *
         * \pre v.xltype must equal XLType (enforced by ensure())
         * \post xltype == XLType && val == v.val
         * \throws Runtime error if v.xltype != XLType
         *
         * \warning This constructor only allows copying the exact same Excel type.
         * Attempting to construct from a mismatched type is a design error and will throw.
         *
         * \note The requirement that v.xltype == XLType ensures type safety - only
         * XLOPER12 objects of the matching type can be used to construct this object.
         * Any mismatch indicates an internal programming error.
         */
        constexpr explicit Base(const XLOPER12& v)    // NOLINT
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>>
            : Base()
        {
            ensure(base_xltype(v.xltype) == static_cast<decltype(xltype)>(XLType), "XLOPER12 type not convertible to type");
            xltype = v.xltype;
            val    = v.val;
        }

        /**
         * \brief Copy constructor for arithmetic types
         * \param other The Base object to copy from
         *
         * Performs a validated copy of another Base object. This constructor:
         * 1. Validates the source object's state
         * 2. Verifies type consistency (xltype == other.xltype)
         * 3. Copies the value through the protected value() accessor
         *
         * \pre other.is_valid() == true
         * \pre xltype == other.xltype (always true for same specialization)
         * \post value() == other.value()
         * \throws Runtime error if preconditions are violated
         *
         * \note Only available for arithmetic value_type (xltypeNum, xltypeInt, xltypeBool, xltypeErr).
         * String and complex types require different copy semantics.
         */
        constexpr Base(const Base& other)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>>
            : Base()
        {
            ensure(other.is_valid());
            ensure(base_xltype(xltype) == base_xltype(other.xltype));
            value() = other.value();
        }

        /**
         * \brief Move constructor for arithmetic types
         * \param other The Base object to move from
         *
         * Performs a move by swapping contents with the default-constructed object.
         * This is equivalent to a copy for POD types but provides move semantics for
         * consistency with the C++ standard.
         *
         * \post other is left in a valid but unspecified state (still has xltype == XLType)
         *
         * \note Marked noexcept for exception safety guarantees required by standard containers.
         * Cannot validate state due to noexcept requirement - caller must ensure validity.
         *
         * \warning Validation is commented out because this constructor must be noexcept.
         * Moving from an invalid object results in undefined behavior (caller's responsibility).
         */
        constexpr Base(Base&& other) noexcept
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>>
            : Base()
        {
            // Can't do this, as the constructor needs to be noexcept
            // ensure(other.is_valid());
            // ensure(xltype == other.xltype);

            using std::swap;
            swap(other, *this);
        }

        /**
         * \brief Value constructor from arithmetic type
         * \tparam TValue The type of value to construct from
         * \param v The value to initialize with
         *
         * Constructs a Base object from a fundamental arithmetic value.
         * This enables natural initialization syntax:
         * \code
         * Number num(3.14);
         * Int integer(42);
         * Bool boolean(true);
         * \endcode
         *
         * \pre TValue must be convertible to value_type
         * \post value() == static_cast<value_type>(v)
         *
         * \note Standard C++ conversion rules apply. Narrowing conversions
         * (e.g., double to int) are allowed and follow normal C++ semantics.
         */
        template<typename TValue>
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::convertible_to<TValue, value_type>
        constexpr Base(TValue v) : Base()    // NOLINT
        {
            value() = v;
        }

        /**
         * \brief Cross-type constructor from compatible Base specialization
         * \tparam TThisDerived The derived type of the source object
         * \tparam ThisValueType The Excel type of the source object
         * \tparam Others Additional allowed types for the source object
         * \param other The source Base object to construct from
         *
         * Enables conversion between compatible Excel types, for example:
         * \code
         * Base<D1, xltypeNum, xltypeInt> num(3.14);  // Allows num or int
         * Base<D2, xltypeInt> int_val(num);          // Convert from num
         * \endcode
         *
         * \pre ThisValueType must be in OtherTypes... (enforced by requires clause)
         * \pre other.is_valid() == true
         * \post value() == other.to<value_type>()
         * \throws Runtime error if other is not in a valid state
         *
         * \note Uses the source object's to<>() method for type conversion,
         * which delegates to the implicit conversion operator.
         */
        template<typename TThisDerived, size_t ThisValueType, size_t... Others>
            requires contains_value<ThisValueType, OtherTypes...>
        constexpr Base(const Base<TThisDerived, ThisValueType, Others...>& other) : Base()    // NOLINT
        {
            ensure(other.is_valid());
            value() = other.template to<value_type>();
        }

        /**
         * \brief Copy assignment operator for arithmetic types
         * \param other The Base object to copy from
         * \return Reference to *this
         *
         * Performs a validated copy assignment with self-assignment check.
         * For POD union members, this is a simple bit copy.
         *
         * \pre is_valid() == true
         * \pre other.is_valid() == true
         * \pre xltype == other.xltype (always true for same specialization)
         * \post val == other.val
         * \throws Runtime error if preconditions are violated
         *
         * \note Self-assignment is handled efficiently via pointer comparison.
         */
        constexpr Base& operator=(const Base& other) {
            if (this == &other) return *this;

            ensure(is_valid());
            ensure(other.is_valid());
            ensure(base_xltype(xltype) == base_xltype(other.xltype));

            val = other.val;  // Simple POD copy
            return *this;
        }

        /**
         * \brief Move assignment operator for arithmetic types
         * \param other The Base object to move from
         * \return Reference to *this
         *
         * Performs move assignment by copying POD members and resetting the source.
         * For POD types, move is equivalent to copy, but we reset the source to
         * maintain moved-from state semantics.
         *
         * \pre is_valid() == true (not enforced due to noexcept)
         * \pre other.is_valid() == true (not enforced due to noexcept)
         * \post xltype == other.xltype (pre-move value)
         * \post val == other.val (pre-move value)
         * \post other is in valid but unspecified state
         *
         * \note Marked noexcept for exception safety guarantees.
         * Self-assignment check prevents unnecessary work.
         */
        constexpr Base& operator=(Base&& other) noexcept
        {
            if (this == &other) return *this;

            xltype = other.xltype;
            val = other.val;

            // Optionally reset other to moved-from state
            other.xltype = XLType;
            // other.val can stay as-is for POD types

            return *this;
        }

        /**
         * \brief Assignment operator from arithmetic value
         * \tparam TValue The type of value to assign
         * \param v The value to assign
         * \return Reference to *this
         *
         * Enables natural assignment syntax:
         * \code
         * Number num(3.14);
         * num = 2.71;  // Uses this operator
         * \endcode
         *
         * \pre is_valid() == true
         * \pre TValue must be convertible to value_type
         * \post value() == static_cast<value_type>(v)
         * \throws Runtime error if object is not in valid state
         *
         * \note Standard C++ conversion rules apply for the assignment.
         */
        template<typename TValue>
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::convertible_to<TValue, value_type>
        constexpr Base& operator=(TValue v)
        {
            ensure(is_valid());
            value() = v;
            return *this;
        }

        /**
         * \brief Cross-type assignment from compatible Base specialization
         * \tparam TThisDerived The derived type of the source object
         * \tparam ThisValueType The Excel type of the source object
         * \tparam Others Additional allowed types for the source object
         * \param v The source Base object to assign from
         * \return Reference to *this
         *
         * Enables assignment between compatible Excel types:
         * \code
         * Base<D1, xltypeNum, xltypeInt> num(3.14);
         * Base<D2, xltypeInt> int_val(42);
         * num = int_val;  // Convert int to num
         * \endcode
         *
         * \pre is_valid() == true
         * \pre v.is_valid() == true
         * \pre ThisValueType must be in OtherTypes... (enforced by requires clause)
         * \post value() == v.to<value_type>()
         * \throws Runtime error if preconditions are violated
         *
         * \note Uses the source object's to<>() method for type conversion.
         */
        template<typename TThisDerived, size_t ThisValueType, size_t... Others>
            requires contains_value<ThisValueType, OtherTypes...>
        constexpr Base& operator=(const Base<TThisDerived, ThisValueType, Others...>& v)
        {
            ensure(is_valid());
            ensure(v.is_valid());

            value() = v.template to<value_type>();
            return *this;
        }

        /**
         * \brief Equality operator for cross-type Base comparison
         * \tparam TOther The type of the right-hand operand
         * \param lhs The left-hand Base object
         * \param rhs The right-hand Base object (different specialization)
         * \return true if values are equal, false otherwise
         *
         * Compares values of two Base objects with potentially different Excel types.
         * Standard arithmetic conversion rules apply:
         * \code
         * Base<D1, xltypeNum> num(3.0);
         * Base<D2, xltypeInt> int_val(3);
         * num == int_val;  // true (3.0 == 3)
         * \endcode
         *
         * \note Error types (xltypeErr) are excluded from comparisons as they
         * have no meaningful ordering or equality semantics.
         *
         * \note Only compares values, not type identity. Different Excel types
         * with equal numeric values are considered equal.
         *
         * \note Uses .value() on both sides, which internally calls the
         * protected value() accessor. No validation is performed.
         */
        template<typename TOther>
        constexpr friend bool operator==(const TDerived& lhs, const TOther& rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && (XLType != xltypeErr) && (!std::is_fundamental_v<TOther>) &&
                     (std::convertible_to<TOther, TDerived> || contains_value<TOther::excel_type, OtherTypes...>)
        {
            ensure(lhs.is_valid());
            using result_t = std::common_type_t<value_type, typename TOther::value_type>;
            return static_cast<result_t>(lhs.value()) == static_cast<result_t>(rhs);
        }

        /**
         * \brief Equality operator for comparison with fundamental types
         * \tparam TValue The fundamental type to compare with
         * \param lhs The left-hand Base object
         * \param rhs The right-hand fundamental value
         * \return true if values are equal, false otherwise
         *
         * Enables natural comparison with fundamental values:
         * \code
         * Number num(3.14);
         * if (num == 3.14) { }  // Uses this operator
         * if (3.14 == num) { }  // Also works via ADL
         * \endcode
         *
         * \note The std::is_fundamental_v constraint prevents overload ambiguity
         * with the cross-type Base overload.
         *
         * \note Standard arithmetic conversion rules apply.
         */
        template<typename TValue>
        constexpr friend bool operator==(const TDerived& lhs, TValue rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::is_fundamental_v<TValue> &&
                     (XLType != xltypeErr) && std::convertible_to<TValue, value_type>
        {
            ensure(lhs.is_valid());
            return lhs.value() == rhs;
        }

        /**
         * \brief Three-way comparison operator for cross-type Base comparison
         * \tparam TOther The type of the right-hand operand
         * \param lhs The left-hand Base object
         * \param rhs The right-hand Base object (different specialization)
         * \return std::strong_ordering or std::partial_ordering depending on types
         *
         * Performs three-way comparison between two Base objects:
         * \code
         * Base<D1, xltypeNum> a(5.0), b(3.0);
         * auto cmp = a <=> b;  // std::partial_ordering::greater
         * if (a > b) { }       // Auto-generated from <=>
         * \endcode
         *
         * Return type depends on operands:
         * - Both integral → std::strong_ordering
         * - Any floating-point → std::partial_ordering (handles NaN)
         * - Mixed types → std::common_comparison_category_t
         *
         * \note C++20 automatically generates <, <=, >, >= from this operator.
         *
         * \note Error types are excluded from ordering (no meaningful semantics).
         */
        template<typename TOther>
        constexpr friend auto operator<=>(const TDerived& lhs, const TOther& rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && (XLType != xltypeErr) && (!std::is_fundamental_v<TOther>) &&
                     (std::convertible_to<TOther, TDerived> || contains_value<TOther::excel_type, OtherTypes...>)
        {
            ensure(lhs.is_valid());
            using result_t = std::common_type_t<value_type, typename TOther::value_type>;
            return static_cast<result_t>(lhs.value()) <=> static_cast<result_t>(rhs);
        }

        /**
         * \brief Three-way comparison operator for comparison with fundamental types
         * \tparam TValue The fundamental type to compare with
         * \param lhs The left-hand Base object
         * \param rhs The right-hand fundamental value
         * \return std::strong_ordering or std::partial_ordering depending on types
         *
         * Enables natural three-way comparison with fundamental values:
         * \code
         * Number num(3.14);
         * if (num < 5.0) { }    // Auto-generated from <=>
         * if (5.0 >= num) { }   // Also works via ADL
         * \endcode
         *
         * \note Works symmetrically via ADL (Argument-Dependent Lookup).
         */
        template<typename TValue>
        constexpr friend auto operator<=>(const TDerived& lhs, TValue rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::is_fundamental_v<TValue> &&
                     (XLType != xltypeErr) && std::convertible_to<TValue, value_type>
        {
            ensure(lhs.is_valid());
            return lhs.value() <=> rhs;
        }

        /**
         * \brief Unary plus operator
         * \param v The Base object
         * \return A new TDerived object with the same value
         *
         * Applies unary + to the underlying value:
         * \code
         * Number num(3.14);
         * auto result = +num;  // result == 3.14 (new object)
         * \endcode
         *
         * For arithmetic types, unary + performs integral promotion but typically
         * doesn't change the value. Returns a new object (value semantics).
         *
         * \note Mirrors built-in behavior for fundamental types.
         */
        constexpr friend TDerived operator+(const TDerived& v)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && (XLType != xltypeErr)
        {
            ensure(v.is_valid());
            return TDerived(+v.value());
        }

        /**
         * \brief Unary minus operator
         * \param v The Base object
         * \return A new TDerived object with the negated value
         *
         * Applies unary - to the underlying value:
         * \code
         * Number num(3.14);
         * auto result = -num;  // result == -3.14 (new object)
         * \endcode
         *
         * \note Returns a new object (non-mutating).
         */
        constexpr friend TDerived operator-(const TDerived& v)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && (XLType != xltypeErr)
        {
            ensure(v.is_valid());
            return TDerived(-v.value());
        }

        /**
         * \brief Addition operator for cross-type Base objects
         * \tparam TOther The type of the right-hand Base operand
         * \param lhs The left-hand Base object
         * \param rhs The right-hand Base object (possibly different specialization)
         * \return A new TDerived object containing the sum
         *
         * Performs type-safe addition with automatic type promotion using std::common_type_t.
         * The RHS uses implicit conversion operator (static_cast<result_t>(rhs)) to extract
         * its value, maintaining encapsulation.
         *
         * Example:
         * \code
         * Base<D1, xltypeNum> a(3.14);
         * Base<D2, xltypeInt> b(5);
         * auto result = a + b;  // result_t = double, returns 8.14
         * \endcode
         *
         * \note Result may be narrowed when constructing TDerived if value_type differs
         * from result_t (standard C++ conversion rules apply).
         */
        template<typename TOther>
        constexpr friend TDerived operator+(const TDerived& lhs, const TOther& rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && (XLType != xltypeErr) && (!std::is_fundamental_v<TOther>) &&
                     (std::convertible_to<TOther, TDerived> || contains_value<TOther::excel_type, OtherTypes...>)
        {
            ensure(lhs.is_valid());
            using result_t = std::common_type_t<value_type, typename TOther::value_type>;
            return TDerived(static_cast<result_t>(lhs.value()) + static_cast<result_t>(rhs));
        }

        /**
         * \brief Addition operator with fundamental type
         * \tparam TValue The fundamental type to add
         * \param lhs The left-hand Base object
         * \param rhs The right-hand fundamental value
         * \return A new TDerived object containing the sum
         *
         * Enables natural arithmetic with fundamental types (works symmetrically via ADL):
         * \code
         * Number num(3.14);
         * auto r1 = num + 2.0;  // Returns Number(5.14)
         * auto r2 = 2.0 + num;  // Also works via ADL
         * \endcode
         */
        template<typename TValue>
        constexpr friend TDerived operator+(const TDerived& lhs, TValue rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::is_fundamental_v<TValue> &&
                     (XLType != xltypeErr) && std::convertible_to<TValue, value_type>
        {
            ensure(lhs.is_valid());
            using result_t = std::common_type_t<value_type, TValue>;
            return TDerived(static_cast<result_t>(lhs.value()) + static_cast<result_t>(rhs));
        }

        /**
         * \brief Subtraction operator for cross-type Base objects
         * \tparam TOther The type of the right-hand Base operand
         * \param lhs The left-hand Base object
         * \param rhs The right-hand Base object to subtract
         * \return A new TDerived object containing the difference
         *
         * Performs type-safe subtraction with automatic type promotion.
         * Follows same pattern as operator+ with std::common_type_t.
         */
        template<typename TOther>
        constexpr friend TDerived operator-(const TDerived& lhs, const TOther& rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && (XLType != xltypeErr) && (!std::is_fundamental_v<TOther>) &&
                     (std::convertible_to<TOther, TDerived> || contains_value<TOther::excel_type, OtherTypes...>)
        {
            ensure(lhs.is_valid());
            using result_t = std::common_type_t<value_type, typename TOther::value_type>;
            return TDerived(static_cast<result_t>(lhs.value()) - static_cast<result_t>(rhs));
        }

        /**
         * \brief Subtraction operator with fundamental type
         * \tparam TValue The fundamental type to subtract
         * \param lhs The left-hand Base object
         * \param rhs The right-hand fundamental value to subtract
         * \return A new TDerived object containing the difference
         *
         * Enables natural arithmetic: `num - 2.0` and `2.0 - num` both work via ADL.
         */
        template<typename TValue>
        constexpr friend TDerived operator-(const Base& lhs, TValue rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::is_fundamental_v<TValue> &&
                     (XLType != xltypeErr) && std::convertible_to<TValue, value_type>
        {
            ensure(lhs.is_valid());
            using result_t = std::common_type_t<value_type, TValue>;
            return TDerived(static_cast<result_t>(lhs.value()) - static_cast<result_t>(rhs));
        }

        /**
         * \brief Multiplication operator for cross-type Base objects
         * \tparam TOther The type of the right-hand Base operand
         * \param lhs The left-hand Base object
         * \param rhs The right-hand Base object to multiply
         * \return A new TDerived object containing the product
         *
         * Performs type-safe multiplication with automatic type promotion.
         */
        template<typename TOther>
        constexpr friend TDerived operator*(const TDerived& lhs, const TOther& rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && (XLType != xltypeErr) && (!std::is_fundamental_v<TOther>) &&
                     (std::convertible_to<TOther, TDerived> || contains_value<TOther::excel_type, OtherTypes...>)
        {
            ensure(lhs.is_valid());
            using result_t = std::common_type_t<value_type, typename TOther::value_type>;
            return TDerived(static_cast<result_t>(lhs.value()) * static_cast<result_t>(rhs));
        }

        /**
         * \brief Multiplication operator with fundamental type
         * \tparam TValue The fundamental type to multiply
         * \param lhs The left-hand Base object
         * \param rhs The right-hand fundamental value
         * \return A new TDerived object containing the product
         */
        template<typename TValue>
        constexpr friend TDerived operator*(const TDerived& lhs, TValue rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::is_fundamental_v<TValue> &&
                     (XLType != xltypeErr) && std::convertible_to<TValue, value_type>
        {
            ensure(lhs.is_valid());
            using result_t = std::common_type_t<value_type, TValue>;
            return TDerived(static_cast<result_t>(lhs.value()) * static_cast<result_t>(rhs));
        }

        /**
         * \brief Division operator for cross-type Base objects
         * \tparam TOther The type of the right-hand Base operand
         * \param lhs The left-hand Base object
         * \param rhs The right-hand Base object to divide by
         * \return A new TDerived object containing the quotient
         *
         * Performs type-safe division with automatic type promotion.
         * \warning Does not check for division by zero (standard C++ behavior).
         */
        template<typename TOther>
        constexpr friend TDerived operator/(const TDerived& lhs, const TOther& rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && (XLType != xltypeErr) && (!std::is_fundamental_v<TOther>) &&
                     (std::convertible_to<TOther, TDerived> || contains_value<TOther::excel_type, OtherTypes...>)
        {
            ensure(lhs.is_valid());
            using result_t = std::common_type_t<value_type, typename TOther::value_type>;
            return TDerived(static_cast<result_t>(lhs.value()) / static_cast<result_t>(rhs));
        }

        /**
         * \brief Division operator with fundamental type
         * \tparam TValue The fundamental type to divide by
         * \param lhs The left-hand Base object
         * \param rhs The right-hand fundamental value to divide by
         * \return A new TDerived object containing the quotient
         *
         * \warning Does not check for division by zero.
         */
        template<typename TValue>
        constexpr friend TDerived operator/(const TDerived& lhs, TValue rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::is_fundamental_v<TValue> &&
                     (XLType != xltypeErr) && std::convertible_to<TValue, value_type>
        {
            ensure(lhs.is_valid());
            using result_t = std::common_type_t<value_type, TValue>;
            return TDerived(static_cast<result_t>(lhs.value()) / static_cast<result_t>(rhs));
        }

        /**
         * \brief Addition assignment operator for cross-type Base objects
         * \tparam TOther The type of the right-hand Base operand
         * \param rhs The Base object to add
         * \return Reference to *this (derived type)
         *
         * Performs in-place addition with type promotion. The RHS uses implicit
         * conversion operator to extract its value.
         *
         * \note Result may be narrowed when assigned back to value() if value_type
         * differs from result_t.
         */
        template<typename TOther>
        constexpr TDerived& operator+=(const TOther& rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && (XLType != xltypeErr) && (!std::is_fundamental_v<TOther>) &&
                     (std::convertible_to<TOther, TDerived> || contains_value<TOther::excel_type, OtherTypes...>)
        {
            ensure(is_valid());
            using result_t = std::common_type_t<value_type, typename TOther::value_type>;
            value()        = static_cast<result_t>(value()) + static_cast<result_t>(rhs);
            return derived();
        }

        /**
         * \brief Addition assignment operator with fundamental type
         * \tparam TValue The fundamental type to add
         * \param rhs The fundamental value to add
         * \return Reference to *this (derived type)
         *
         * Enables natural compound assignment: `num += 2.0`
         */
        template<typename TValue>
        constexpr TDerived& operator+=(TValue rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::is_fundamental_v<TValue> &&
                     (XLType != xltypeErr) && std::convertible_to<TValue, value_type>
        {
            ensure(is_valid());
            using result_t = std::common_type_t<value_type, TValue>;
            value()        = static_cast<result_t>(value()) + static_cast<result_t>(rhs);
            return derived();
        }

        /**
         * \brief Subtraction assignment operator for cross-type Base objects
         * \tparam TOther The type of the right-hand Base operand
         * \param rhs The Base object to subtract
         * \return Reference to *this (derived type)
         */
        template<typename TOther>
        constexpr TDerived& operator-=(const TOther& rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && (XLType != xltypeErr) && (!std::is_fundamental_v<TOther>) &&
                     (std::convertible_to<TOther, TDerived> || contains_value<TOther::excel_type, OtherTypes...>)
        {
            ensure(is_valid());
            using result_t = std::common_type_t<value_type, typename TOther::value_type>;
            value()        = static_cast<result_t>(value()) - static_cast<result_t>(rhs);
            return derived();
        }

        /**
         * \brief Subtraction assignment operator with fundamental type
         * \tparam TValue The fundamental type to subtract
         * \param rhs The fundamental value to subtract
         * \return Reference to *this (derived type)
         */
        template<typename TValue>
        constexpr TDerived& operator-=(TValue rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::is_fundamental_v<TValue> &&
                     (XLType != xltypeErr) && std::convertible_to<TValue, value_type>
        {
            ensure(is_valid());
            using result_t = std::common_type_t<value_type, TValue>;
            value()        = static_cast<result_t>(value()) - static_cast<result_t>(rhs);
            return derived();
        }

        /**
         * \brief Multiplication assignment operator for cross-type Base objects
         * \tparam TOther The type of the right-hand Base operand
         * \param rhs The Base object to multiply by
         * \return Reference to *this (derived type)
         */
        template<typename TOther>
        constexpr TDerived& operator*=(const TOther& rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && (XLType != xltypeErr) && (!std::is_fundamental_v<TOther>) &&
                     (std::convertible_to<TOther, TDerived> || contains_value<TOther::excel_type, OtherTypes...>)
        {
            ensure(is_valid());
            using result_t = std::common_type_t<value_type, typename TOther::value_type>;
            value()        = static_cast<result_t>(value()) * static_cast<result_t>(rhs);
            return derived();
        }

        /**
         * \brief Multiplication assignment operator with fundamental type
         * \tparam TValue The fundamental type to multiply by
         * \param rhs The fundamental value to multiply by
         * \return Reference to *this (derived type)
         */
        template<typename TValue>
        constexpr TDerived& operator*=(TValue rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::is_fundamental_v<TValue> &&
                     (XLType != xltypeErr) && std::convertible_to<TValue, value_type>
        {
            ensure(is_valid());
            using result_t = std::common_type_t<value_type, TValue>;
            value()        = static_cast<result_t>(value()) * static_cast<result_t>(rhs);
            return derived();
        }

        /**
         * \brief Division assignment operator for cross-type Base objects
         * \tparam TOther The type of the right-hand Base operand
         * \param rhs The Base object to divide by
         * \return Reference to *this (derived type)
         *
         * \warning Does not check for division by zero.
         */
        template<typename TOther>
        constexpr TDerived& operator/=(const TOther& rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && (XLType != xltypeErr) && (!std::is_fundamental_v<TOther>) &&
                     (std::convertible_to<TOther, TDerived> || contains_value<TOther::excel_type, OtherTypes...>)
        {
            ensure(is_valid());
            using result_t = std::common_type_t<value_type, typename TOther::value_type>;
            value()        = static_cast<result_t>(value()) / static_cast<result_t>(rhs);
            return derived();
        }

        /**
         * \brief Division assignment operator with fundamental type
         * \tparam TValue The fundamental type to divide by
         * \param rhs The fundamental value to divide by
         * \return Reference to *this (derived type)
         *
         * \warning Does not check for division by zero.
         */
        template<typename TValue>
        constexpr TDerived& operator/=(TValue rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::is_fundamental_v<TValue> &&
                     (XLType != xltypeErr) && std::convertible_to<TValue, value_type>
        {
            ensure(is_valid());
            using result_t = std::common_type_t<value_type, TValue>;
            value()        = static_cast<result_t>(value()) / static_cast<result_t>(rhs);
            return derived();
        }

        /**
         * \brief Implicit conversion operator to arithmetic types
         * \tparam T The target arithmetic type (defaults to value_type)
         * \return The value converted to type T
         *
         * Enables implicit conversion to compatible arithmetic types:
         * \code
         * Number num(3.14);
         * double d = num;        // Implicit conversion
         * int i = num;           // Implicit conversion (narrowing)
         * float f = num;         // Implicit conversion
         * \endcode
         *
         * \pre is_valid() == true
         * \post Returns static_cast<T>(value())
         * \throws Runtime error if object is not in valid state
         *
         * \note Excludes bool to prevent implicit conversion in boolean contexts
         * (use explicit operator bool() instead).
         *
         * \note This operator is used internally by cross-type arithmetic operators
         * to extract values while respecting encapsulation (protected value() access).
         *
         * \note Standard C++ conversion rules apply. Narrowing conversions are allowed.
         */
        template<typename T = value_type>
        constexpr operator T() const // NOLINT
            requires std::is_arithmetic_v<T> && std::is_arithmetic_v<std::remove_cvref_t<value_type>> &&
                     std::convertible_to<value_type, T> && (not std::same_as<T, bool>)
        {
            ensure(is_valid());
            return static_cast<T>(value());
        }

        /**
         * \brief Explicit conversion operator to bool
         * \return The boolean representation of the value (0 = false, non-zero = true)
         *
         * Requires explicit conversion to bool to prevent accidental implicit conversions:
         * \code
         * Number num(0.0);
         * if (num) { }                    // ❌ Compile error - must be explicit
         * if (static_cast<bool>(num)) { } // ✅ OK
         * if (bool(num)) { }              // ✅ OK
         * \endcode
         *
         * \pre is_valid() == true
         * \throws Runtime error if object is not in valid state
         *
         * \note The explicit keyword prevents accidental use in boolean contexts,
         * forcing intentional conversion and avoiding ambiguity with operator T().
         *
         * \note Follows standard C++ truthiness: 0 is false, non-zero is true.
         */
        constexpr explicit operator bool() const
        {
            ensure(is_valid());
            return static_cast<bool>(value());
        }

        /**
         * \brief Stream output operator
         * \param os The output stream
         * \param v The Base object to output
         * \return The output stream (for chaining)
         *
         * Outputs the underlying value to a stream:
         * \code
         * Number num(3.14);
         * std::cout << num;           // Outputs: 3.14
         * std::cout << num << "\n";   // Chaining works
         * \endcode
         *
         * \pre v.is_valid() == true
         * \throws Runtime error if object is not in valid state
         *
         * \note Not marked constexpr because std::ostream operations are not constexpr.
         *
         * \note Validation uses ensure() for consistency with other operations.
         */
        friend std::ostream& operator<<(std::ostream& os, const Base& v)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>>
        {
            ensure(v.is_valid());
            os << v.value();
            return os;
        }

        /**
         * \brief Convert to specified arithmetic type
         * \tparam TValue The target type to convert to
         * \return The value converted to TValue
         *
         * Provides explicit conversion to arithmetic types by delegating to the
         * implicit conversion operator:
         * \code
         * Number num(3.14);
         * auto i = num.to<int>();     // Explicit conversion to int (3)
         * auto f = num.to<float>();   // Explicit conversion to float
         * auto d = num.to<double>();  // Explicit conversion to double
         * \endcode
         *
         * \note Delegates to operator T() which validates and performs conversion.
         * This avoids code duplication and ensures consistent behavior.
         *
         * \note Excludes bool to avoid ambiguity (use explicit operator bool() instead).
         *
         * \note Used internally by cross-type constructors and assignment operators
         * for type conversion between compatible Excel types.
         */
        template<typename TValue>
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> &&
                     std::is_arithmetic_v<TValue> &&
                     std::convertible_to<value_type, TValue> &&
                     (not std::same_as<TValue, bool>)
        constexpr TValue to() const
        {
            return static_cast<TValue>(*this);  // Delegates to operator T()
        }

        /**
         * \brief Dereference operator for accessing the underlying XLOPER12 object.
         *
         * This operator allows accessing the Base object as the underlying XLOPER12 object.
         * It returns a reference to the current object cast as an XLOPER12 reference,
         * enabling direct access to the Excel API structure.
         *
         * This is useful when interfacing with Excel SDK functions that require
         * raw XLOPER12 references.
         *
         * \return A reference to the underlying XLOPER12 object.
         */
        // constexpr XLOPER12& operator*() noexcept { return *this; }

        /**
         * \brief Const dereference operator for accessing the underlying XLOPER12 object.
         *
         * This operator allows accessing the Base object as the underlying XLOPER12 object
         * in a const context. It returns a const reference to the current object cast as
         * an XLOPER12 const reference.
         *
         * This is useful when interfacing with Excel SDK functions that require
         * raw XLOPER12 const references.
         *
         * \return A const reference to the underlying XLOPER12 object.
         */
        // constexpr const XLOPER12& operator*() const noexcept { return *this; }

        /**
         * \brief Lifts a pointer to an XLOPER12 object to a pointer to a TDerived object.
         *
         * This function casts a pointer to an XLOPER12 object to a pointer to a TDerived object.
         *
         * \param op Pointer to the XLOPER12 object.
         * \return Pointer to the TDerived object.
         */
        // constexpr static TDerived* lift(XLOPER12* op) { return static_cast<TDerived*>(op); }

        /**
         * \brief Lifts a const pointer to an XLOPER12 object to a const pointer to a TDerived object.
         *
         * This function casts a const pointer to an XLOPER12 object to a const pointer to a TDerived object.
         *
         * \param op Const pointer to the XLOPER12 object.
         * \return Const pointer to the TDerived object.
         */
        // constexpr static const TDerived* lift(const XLOPER12* op) { return static_cast<const TDerived*>(op); }

        /**
         * \brief Lifts a reference to an XLOPER12 object to a reference to a TDerived object.
         *
         * This function casts a reference to an XLOPER12 object to a reference to a TDerived object.
         *
         * \param op Reference to the XLOPER12 object.
         * \return Reference to the TDerived object.
         */
        // constexpr static TDerived& lift(XLOPER12& op) { return static_cast<TDerived&>(op); }

        /**
         * \brief Lifts a const reference to an XLOPER12 object to a const reference to a TDerived object.
         *
         * This function casts a const reference to an XLOPER12 object to a const reference to a TDerived object.
         *
         * \param op Const reference to the XLOPER12 object.
         * \return Const reference to the TDerived object.
         */
        // constexpr static const TDerived& lift(const XLOPER12& op) { return static_cast<const TDerived&>(op); }
    };

    /**
     * \brief Free swap function for Base objects (implementation)
     *
     * Swaps two Base objects by directly swapping their POD members.
     * This function is ADL-enabled (Argument-Dependent Lookup) for use with std::swap.
     *
     * Implementation details:
     * - Swaps xltype discriminator
     * - Swaps val union (POD bit copy)
     * - No validation performed (follows standard library conventions)
     * - noexcept guarantee preserved for algorithm compatibility
     *
     * \note This implementation intentionally skips validation for performance.
     * Both objects are assumed to be in valid states (caller's responsibility).
     * This matches standard library swap behavior for POD types.
     *
     * \note The noexcept guarantee is critical for exception safety in
     * standard algorithms and containers. Do not remove it.
     *
     * Example usage:
     * \code
     * Number a(3.14), b(2.71);
     * using std::swap;
     * swap(a, b);  // Calls this function via ADL
     * std::swap(a, b);  // Also calls this function
     * \endcode
     */
    template<typename TDerived, size_t ValueType, size_t... OtherTypes>
    constexpr void swap(Base<TDerived, ValueType, OtherTypes...>& lhs, Base<TDerived, ValueType, OtherTypes...>& rhs) noexcept
    {
        using std::swap;
        swap(lhs.xltype, rhs.xltype);
        swap(lhs.val, rhs.val);
    }

}    // namespace xll::impl