//
// Created by kenne on 23/03/2025.
//

#pragma once

#include "../Utils/Ensure.hpp"
#include <iostream>
#include <type_traits>
#include <xlcall.hpp>

namespace xll::impl
{

    template<auto Value, auto... Values>
    constexpr bool contains_value = (... || (Value == Values));

    template<typename T>
    concept ExcelType = requires(T value) {
        requires std::is_integral_v<T>;
        requires(value == xltypeNum || value == xltypeStr || value == xltypeBool || value == xltypeRef || value == xltypeErr ||
                 value == xltypeFlow || value == xltypeMulti || value == xltypeMissing || value == xltypeNil || value == xltypeSRef ||
                 value == xltypeInt);
    };

    // Helper variables for testing specific types
    template<auto Value>
    constexpr bool is_excel_type = (Value == xltypeNum || Value == xltypeStr || Value == xltypeBool || Value == xltypeRef ||
                                    Value == xltypeErr || Value == xltypeFlow || Value == xltypeMulti || Value == xltypeMissing ||
                                    Value == xltypeNil || Value == xltypeSRef || Value == xltypeInt);

    template<typename TDerived, size_t ValueType, size_t... OtherTypes>
        requires is_excel_type<ValueType>
    class Base;

    template<typename TDerived, size_t ValueType, size_t... OtherTypes>
    void swap(Base<TDerived, ValueType, OtherTypes...>& lhs, Base<TDerived, ValueType, OtherTypes...>& rhs) noexcept;

    template<typename TDerived, size_t XLType, size_t... OtherTypes>
        requires is_excel_type<XLType>
    class Base : public XLOPER12
    {
        TDerived&       derived() { return static_cast<TDerived&>(*this); }
        TDerived const& derived() const { return static_cast<TDerived const&>(*this); }

        static constexpr size_t excel_type = XLType;

    protected:
        ~Base() = default;

        auto& value()
        {
            if constexpr (XLType == xltypeNum)
                return val.num;
            else if constexpr (XLType == xltypeStr)
                return val.str;
            else if constexpr (XLType == xltypeBool)
                return val.xbool;
            else if constexpr (XLType == xltypeErr)
                return val.err;
            else if constexpr (XLType == xltypeMulti)
                return val.array;
            else if constexpr (XLType == xltypeInt)
                return val.w;
            else
                throw std::bad_cast();    // Handle other types
        }

        auto value() const
        {
            if constexpr (XLType == xltypeNum)
                return val.num;
            else if constexpr (XLType == xltypeStr)
                return val.str;
            else if constexpr (XLType == xltypeBool)
                return val.xbool;
            else if constexpr (XLType == xltypeErr)
                return val.err;
            else if constexpr (XLType == xltypeMulti)
                return val.array;
            else if constexpr (XLType == xltypeInt)
                return val.w;
            else
                throw std::bad_cast();    // Handle other types
        }

    public:
        bool                  is_valid() const { return xltype == XLType; }
        static constexpr bool has_crtp_base = true;

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

        Base() : XLOPER12() { xltype = XLType; }

        /**
         * \brief Constructs a Base object from an XLOPER12 object.
         *
         * This constructor initializes a Base object using the provided XLOPER12 object.
         * It checks if the xltype of the provided XLOPER12 object matches the XLType of the Base class.
         * If they match, it copies the xltype and val from the provided XLOPER12 object.
         * Otherwise, it throws a runtime error indicating that the XLOPER12 type is not convertible.
         *
         * \param v The XLOPER12 object to construct the Base object from.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \throws std::runtime_error if the xltype of the provided XLOPER12 object does not match the XLType of the Base class.
         */
        explicit Base(const XLOPER12& v)    // NOLINT
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>>
            : Base()
        {
            switch (v.xltype == XLType) {
                case true:
                    xltype = v.xltype;
                    val    = v.val;
                    break;
                default:
                    throw std::runtime_error("XLOPER12 type not convertible to type");
            }
        }

        /**
         * \brief Copy constructor for the Base class.
         *
         * This constructor initializes a Base object by copying the contents of another Base object.
         * It checks if the xltype of the other Base object matches the XLType of the current Base class.
         * If they match, it copies the xltype and value from the other Base object.
         * Otherwise, it throws a runtime error indicating that the instance is invalid.
         *
         * \param other The Base object to copy from.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \throws std::runtime_error if the xltype of the other Base object does not match the XLType of the current Base class.
         */
        Base(const Base& other)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>>
            : Base()
        {
            ensure(other.is_valid());
            ensure(xltype == other.xltype);
            value() = other.value();

            // switch (other.xltype == XLType) {
            //     case true:
            //         xltype  = other.xltype;
            //         value() = other.value();
            //         break;
            //     default:
            //         throw std::runtime_error(std::format("Instance of {} is invalid", TDerived::type_name));
            //         break;
            // }
        }

        /**
         * \brief Move constructor for the Base class.
         *
         * This constructor initializes a Base object by moving the contents of another Base object.
         * It checks if the xltype of the other Base object matches the XLType of the current Base class.
         * If they match, it swaps the contents of the other Base object with the current Base object.
         * Otherwise, it throws a runtime error indicating that the instance is invalid.
         *
         * \param other The Base object to move from.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \throws std::runtime_error if the xltype of the other Base object does not match the XLType of the current Base class.
         */
        Base(Base&& other) noexcept
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
         * \brief Constructs a Base object from a value of type TValue.
         *
         * This constructor initializes a Base object using the provided value of type TValue.
         * It checks if the value_type of the Base class is an arithmetic type and if TValue is convertible to value_type.
         * If these conditions are met, it assigns the provided value to the internal value of the Base object.
         *
         * \tparam TValue The type of the value to construct the Base object from.
         * \param v The value to construct the Base object from.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \requires TValue must be convertible to value_type.
         */
        template<typename TValue>
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::convertible_to<TValue, value_type>
        Base(TValue v) : Base()    // NOLINT
        {
            value() = v;
        }

        /**
         * \brief Constructs a Base object from another Base object of a different derived type.
         *
         * This constructor initializes a Base object using the provided Base object of a different derived type.
         * It checks if the xltype of the provided Base object is one of the allowed types for the current Base class.
         * If the provided Base object is valid, it assigns its value to the current Base object.
         *
         * \tparam TThisDerived The derived type of the provided Base object.
         * \tparam ThisValueType The xltype of the provided Base object.
         * \tparam Others Additional allowed xltypes for the current Base class.
         * \param other The Base object to construct the current Base object from.
         * \requires The xltype of the provided Base object must be one of the allowed types for the current Base class.
         */
        template<typename TThisDerived, size_t ThisValueType, size_t... Others>
            requires contains_value<ThisValueType, OtherTypes...>
        Base(const Base<TThisDerived, ThisValueType, Others...>& other) : Base()    // NOLINT
        {
            ensure(other.is_valid());
            value() = other.template to<value_type>();
        }

        /**
         * \brief Copy assignment operator for the Base class.
         *
         * This operator assigns the contents of another Base object to this one.
         * It checks if both objects are valid and have the same Excel type.
         * If the validation checks pass, it copies the value from the other object.
         *
         * \param other The Base object to copy from.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \throws RuntimeError if either object is invalid or if the Excel types don't match.
         * \return Reference to this object after assignment.
         */
        Base& operator=(const Base& other)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>>
        {
            ensure(is_valid());
            ensure(other.is_valid());
            ensure(xltype == other.xltype);

            if (this == &other) return *this;
            val = other.val;
            return *this;
        }

        /**
         * \brief Move assignment operator for the Base class.
         *
         * This operator assigns the contents of another Base object to this one using move semantics.
         * It checks if the current object and the other object are the same.
         * If they are not the same, it swaps the contents of the other object with the current object.
         *
         * \param other The Base object to move from.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \return Reference to this object after assignment.
         */
        Base& operator=(Base&& other) noexcept
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>>
        {
            using std::swap;

            // ensure(is_valid());
            // ensure(other.is_valid());
            // ensure(xltype == other.xltype);

            if (this == &other) return *this;
            swap(other, *this);
            return *this;
        }
        /**
         * \brief Assignment operator for assigning a value of type TValue to the Base object.
         *
         * This operator assigns the provided value to the internal value of the Base object.
         * It checks if the current object is valid before performing the assignment.
         *
         * \tparam TValue The type of the value to assign to the Base object.
         * \param v The value to assign to the Base object.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \requires TValue must be convertible to value_type.
         * \return Reference to this object after assignment.
         */
        template<typename TValue>
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::convertible_to<TValue, value_type>
        Base& operator=(TValue v)
        {
            ensure(is_valid());

            value() = v;
            return *this;
        }

        /**
         * \brief Assignment operator for assigning a Base object of a different derived type to the current Base object.
         *
         * This operator assigns the value of the provided Base object to the internal value of the current Base object.
         * It checks if both the current object and the provided object are valid before performing the assignment.
         *
         * \tparam TThisDerived The derived type of the provided Base object.
         * \tparam ThisValueType The xltype of the provided Base object.
         * \tparam Others Additional allowed xltypes for the current Base class.
         * \param v The Base object to assign to the current Base object.
         * \requires The xltype of the provided Base object must be one of the allowed types for the current Base class.
         * \return Reference to this object after assignment.
         */
        template<typename TThisDerived, size_t ThisValueType, size_t... Others>
            requires contains_value<ThisValueType, OtherTypes...>
        Base& operator=(const Base<TThisDerived, ThisValueType, Others...>& v)
        {
            ensure(is_valid());
            ensure(v.is_valid());

            value() = v.template to<value_type>();
            return *this;
        }

        /**
         * \brief Equality operator for comparing a Base object with another object.
         *
         * This operator compares the value of the current Base object with the value of another object.
         * It checks if the value_type of the Base class is an arithmetic type, the Excel type is not an error type,
         * and the other object is not a fundamental type. Additionally, it ensures that the other object is either
         * convertible to the derived type or has an allowed Excel type.
         *
         * \tparam TOther The type of the other object to compare with.
         * \param lhs The current Base object.
         * \param rhs The other object to compare with.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \requires The Excel type of the Base class must not be an error type.
         * \requires The other object must not be a fundamental type.
         * \requires The other object must be convertible to the derived type or have an allowed Excel type.
         * \return True if the values of the two objects are equal, false otherwise.
         */
        template<typename TOther>
        friend bool operator==(const TDerived& lhs, const TOther& rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && (XLType != xltypeErr) && (!std::is_fundamental_v<TOther>) &&
                     (std::convertible_to<TOther, TDerived> || contains_value<TOther::excel_type, OtherTypes...>)
        {
            return lhs.value() == rhs.value();
        }

        /**
         * \brief Equality operator for comparing a Base object with a fundamental type value.
         *
         * This operator compares the value of the current Base object with a fundamental type value.
         * It checks if the value_type of the Base class is an arithmetic type, the Excel type is not an error type,
         * and the provided value is a fundamental type that is convertible to the value_type.
         *
         * \tparam TValue The type of the value to compare with.
         * \param lhs The current Base object.
         * \param rhs The fundamental type value to compare with.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \requires The Excel type of the Base class must not be an error type.
         * \requires The provided value must be a fundamental type.
         * \requires The provided value must be convertible to value_type.
         * \return True if the value of the Base object is equal to the provided value, false otherwise.
         */
        template<typename TValue>
        friend bool operator==(const TDerived& lhs, TValue rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::is_fundamental_v<TValue> &&
                     (XLType != xltypeErr) && std::convertible_to<TValue, value_type>
        {
            return lhs.value() == rhs;
        }

        /**
         * \brief Three-way comparison operator for comparing a Base object with another object.
         *
         * This operator performs a three-way comparison between the value of the current Base object and the value of another object.
         * It checks if the value_type of the Base class is an arithmetic type, the Excel type is not an error type,
         * and the other object is not a fundamental type. Additionally, it ensures that the other object is either
         * convertible to the derived type or has an allowed Excel type.
         *
         * \tparam TOther The type of the other object to compare with.
         * \param lhs The current Base object.
         * \param rhs The other object to compare with.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \requires The Excel type of the Base class must not be an error type.
         * \requires The other object must not be a fundamental type.
         * \requires The other object must be convertible to the derived type or have an allowed Excel type.
         * \return The result of the three-way comparison between the values of the two objects.
         */
        template<typename TOther>
        friend auto operator<=>(const TDerived& lhs, const TOther& rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && (XLType != xltypeErr) && (!std::is_fundamental_v<TOther>) &&
                     (std::convertible_to<TOther, TDerived> || contains_value<TOther::excel_type, OtherTypes...>)
        {
            return lhs.value() <=> rhs.value();
        }

        /**
         * \brief Three-way comparison operator for comparing a Base object with a fundamental type value.
         *
         * This operator performs a three-way comparison between the value of the current Base object and a fundamental type value.
         * It checks if the value_type of the Base class is an arithmetic type, the Excel type is not an error type,
         * and the provided value is a fundamental type that is convertible to the value_type.
         *
         * \tparam TValue The type of the value to compare with.
         * \param lhs The current Base object.
         * \param rhs The fundamental type value to compare with.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \requires The Excel type of the Base class must not be an error type.
         * \requires The provided value must be a fundamental type.
         * \requires The provided value must be convertible to value_type.
         * \return The result of the three-way comparison between the value of the Base object and the provided value.
         */
        template<typename TValue>
        friend auto operator<=>(const TDerived& lhs, TValue rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::is_fundamental_v<TValue> &&
                     (XLType != xltypeErr) && std::convertible_to<TValue, value_type>
        {
            return lhs.value() <=> rhs;
        }

        /**
         * \brief Unary plus operator for the Base class.
         *
         * This operator returns a new TDerived object with the value of the current Base object.
         * It checks if the value_type of the Base class is an arithmetic type and the Excel type is not an error type.
         *
         * \param v The current Base object.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \requires The Excel type of the Base class must not be an error type.
         * \return A new TDerived object with the value of the current Base object.
         */
        friend TDerived operator+(const TDerived& v)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && (XLType != xltypeErr)
        {
            return TDerived(+v.value());
        }

        /**
         * \brief Unary minus operator for the Base class.
         *
         * This operator returns a new TDerived object with the negated value of the current Base object.
         * It checks if the value_type of the Base class is an arithmetic type and the Excel type is not an error type.
         *
         * \param v The current Base object.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \requires The Excel type of the Base class must not be an error type.
         * \return A new TDerived object with the negated value of the current Base object.
         */
        friend TDerived operator-(const TDerived& v)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && (XLType != xltypeErr)
        {
            return TDerived(-v.value());
        }

        /**
         * \brief Addition operator for adding a Base object with another object.
         *
         * This operator performs addition between the value of the current Base object and the value of another object.
         * It checks if the value_type of the Base class is an arithmetic type, the Excel type is not an error type,
         * and the other object is not a fundamental type. Additionally, it ensures that the other object is either
         * convertible to the derived type or has an allowed Excel type.
         *
         * \tparam TOther The type of the other object to add.
         * \param lhs The current Base object.
         * \param rhs The other object to add.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \requires The Excel type of the Base class must not be an error type.
         * \requires The other object must not be a fundamental type.
         * \requires The other object must be convertible to the derived type or have an allowed Excel type.
         * \return A new TDerived object with the result of the addition.
         */
        template<typename TOther>
        friend TDerived operator+(const TDerived& lhs, const TOther& rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && (XLType != xltypeErr) && (!std::is_fundamental_v<TOther>) &&
                     (std::convertible_to<TOther, TDerived> || contains_value<TOther::excel_type, OtherTypes...>)
        {
            using result_t = std::common_type_t<value_type, typename TOther::value_type>;
            return TDerived(static_cast<result_t>(lhs.value()) + static_cast<result_t>(rhs));
        }

        /**
         * \brief Addition operator for adding a Base object with a fundamental type value.
         *
         * This operator performs addition between the value of the current Base object and a fundamental type value.
         * It checks if the value_type of the Base class is an arithmetic type, the Excel type is not an error type,
         * and the provided value is a fundamental type that is convertible to the value_type.
         *
         * \tparam TValue The type of the value to add.
         * \param lhs The current Base object.
         * \param rhs The fundamental type value to add.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \requires The Excel type of the Base class must not be an error type.
         * \requires The provided value must be a fundamental type.
         * \requires The provided value must be convertible to value_type.
         * \return A new TDerived object with the result of the addition.
         */
        template<typename TValue>
        friend TDerived operator+(const TDerived& lhs, TValue rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::is_fundamental_v<TValue> &&
                     (XLType != xltypeErr) && std::convertible_to<TValue, value_type>
        {
            using result_t = std::common_type_t<value_type, TValue>;
            auto result    = static_cast<result_t>(lhs.value()) + static_cast<result_t>(rhs);
            return TDerived(result);
        }

        /**
         * \brief Subtraction operator for subtracting another object from a Base object.
         *
         * This operator performs subtraction between the value of the current Base object and the value of another object.
         * It checks if the value_type of the Base class is an arithmetic type, the Excel type is not an error type,
         * and the other object is not a fundamental type. Additionally, it ensures that the other object is either
         * convertible to the derived type or has an allowed Excel type.
         *
         * \tparam TOther The type of the other object to subtract.
         * \param lhs The current Base object.
         * \param rhs The other object to subtract.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \requires The Excel type of the Base class must not be an error type.
         * \requires The other object must not be a fundamental type.
         * \requires The other object must be convertible to the derived type or have an allowed Excel type.
         * \return A new TDerived object with the result of the subtraction.
         */
        template<typename TOther>
        friend TDerived operator-(const TDerived& lhs, const TOther& rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && (XLType != xltypeErr) && (!std::is_fundamental_v<TOther>) &&
                     (std::convertible_to<TOther, TDerived> || contains_value<TOther::excel_type, OtherTypes...>)
        {
            using result_t = std::common_type_t<value_type, typename TOther::value_type>;
            return TDerived(static_cast<result_t>(lhs.value()) - static_cast<result_t>(rhs));
        }

        /**
         * \brief Subtraction operator for subtracting a fundamental type value from a Base object.
         *
         * This operator performs subtraction between the value of the current Base object and a fundamental type value.
         * It checks if the value_type of the Base class is an arithmetic type, the Excel type is not an error type,
         * and the provided value is a fundamental type that is convertible to the value_type.
         *
         * \tparam TValue The type of the value to subtract.
         * \param lhs The current Base object.
         * \param rhs The fundamental type value to subtract.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \requires The Excel type of the Base class must not be an error type.
         * \requires The provided value must be a fundamental type.
         * \requires The provided value must be convertible to value_type.
         * \return A new TDerived object with the result of the subtraction.
         */
        template<typename TValue>
        friend TDerived operator-(const Base& lhs, TValue rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::is_fundamental_v<TValue> &&
                     (XLType != xltypeErr) && std::convertible_to<TValue, value_type>
        {
            using result_t = std::common_type_t<value_type, TValue>;
            auto result    = static_cast<result_t>(lhs.value()) - static_cast<result_t>(rhs);
            return TDerived(result);
        }

        /**
         * \brief Multiplication operator for multiplying a Base object with another object.
         *
         * This operator performs multiplication between the value of the current Base object and the value of another object.
         * It checks if the value_type of the Base class is an arithmetic type, the Excel type is not an error type,
         * and the other object is not a fundamental type. Additionally, it ensures that the other object is either
         * convertible to the derived type or has an allowed Excel type.
         *
         * \tparam TOther The type of the other object to multiply.
         * \param lhs The current Base object.
         * \param rhs The other object to multiply.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \requires The Excel type of the Base class must not be an error type.
         * \requires The other object must not be a fundamental type.
         * \requires The other object must be convertible to the derived type or have an allowed Excel type.
         * \return A new TDerived object with the result of the multiplication.
         */
        template<typename TOther>
        friend TDerived operator*(const TDerived& lhs, const TOther& rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && (XLType != xltypeErr) && (!std::is_fundamental_v<TOther>) &&
                     (std::convertible_to<TOther, TDerived> || contains_value<TOther::excel_type, OtherTypes...>)
        {
            using result_t = std::common_type_t<value_type, typename TOther::value_type>;
            return TDerived(static_cast<result_t>(lhs.value()) * static_cast<result_t>(rhs));
        }

        /**
         * \brief Multiplication operator for multiplying a Base object with a fundamental type value.
         *
         * This operator performs multiplication between the value of the current Base object and a fundamental type value.
         * It checks if the value_type of the Base class is an arithmetic type, the Excel type is not an error type,
         * and the provided value is a fundamental type that is convertible to the value_type.
         *
         * \tparam TValue The type of the value to multiply.
         * \param lhs The current Base object.
         * \param rhs The fundamental type value to multiply.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \requires The Excel type of the Base class must not be an error type.
         * \requires The provided value must be a fundamental type.
         * \requires The provided value must be convertible to value_type.
         * \return A new TDerived object with the result of the multiplication.
         */
        template<typename TValue>
        friend TDerived operator*(const TDerived& lhs, TValue rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::is_fundamental_v<TValue> &&
                     (XLType != xltypeErr) && std::convertible_to<TValue, value_type>
        {
            using result_t = std::common_type_t<value_type, TValue>;
            auto result    = static_cast<result_t>(lhs.value()) * static_cast<result_t>(rhs);
            return TDerived(result);
        }

        /**
         * \brief Division operator for dividing a Base object by another object.
         *
         * This operator performs division between the value of the current Base object and the value of another object.
         * It checks if the value_type of the Base class is an arithmetic type, the Excel type is not an error type,
         * and the other object is not a fundamental type. Additionally, it ensures that the other object is either
         * convertible to the derived type or has an allowed Excel type.
         *
         * \tparam TOther The type of the other object to divide by.
         * \param lhs The current Base object.
         * \param rhs The other object to divide by.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \requires The Excel type of the Base class must not be an error type.
         * \requires The other object must not be a fundamental type.
         * \requires The other object must be convertible to the derived type or have an allowed Excel type.
         * \return A new TDerived object with the result of the division.
         */
        template<typename TOther>
        friend TDerived operator/(const TDerived& lhs, const TOther& rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && (XLType != xltypeErr) && (!std::is_fundamental_v<TOther>) &&
                     (std::convertible_to<TOther, TDerived> || contains_value<TOther::excel_type, OtherTypes...>)
        {
            using result_t = std::common_type_t<value_type, typename TOther::value_type>;
            return TDerived(static_cast<result_t>(lhs.value()) / static_cast<result_t>(rhs));
        }

        /**
         * \brief Division operator for dividing a Base object by a fundamental type value.
         *
         * This operator performs division between the value of the current Base object and a fundamental type value.
         * It checks if the value_type of the Base class is an arithmetic type, the Excel type is not an error type,
         * and the provided value is a fundamental type that is convertible to the value_type.
         *
         * \tparam TValue The type of the value to divide by.
         * \param lhs The current Base object.
         * \param rhs The fundamental type value to divide by.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \requires The Excel type of the Base class must not be an error type.
         * \requires The provided value must be a fundamental type.
         * \requires The provided value must be convertible to value_type.
         * \return A new TDerived object with the result of the division.
         */
        template<typename TValue>
        friend TDerived operator/(const TDerived& lhs, TValue rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::is_fundamental_v<TValue> &&
                     (XLType != xltypeErr) && std::convertible_to<TValue, value_type>
        {
            using result_t = std::common_type_t<value_type, TValue>;
            auto result    = static_cast<result_t>(lhs.value()) / static_cast<result_t>(rhs);
            return TDerived(result);
        }

        /**
         * \brief Addition assignment operator for adding another object to a Base object.
         *
         * This operator performs addition between the value of the current Base object and the value of another object,
         * and assigns the result to the current Base object. It checks if the value_type of the Base class is an arithmetic type,
         * the Excel type is not an error type, and the other object is not a fundamental type. Additionally, it ensures that the other
         * object is either convertible to the derived type or has an allowed Excel type.
         *
         * \tparam TOther The type of the other object to add.
         * \param rhs The other object to add.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \requires The Excel type of the Base class must not be an error type.
         * \requires The other object must not be a fundamental type.
         * \requires The other object must be convertible to the derived type or have an allowed Excel type.
         * \return Reference to this object after the addition.
         */
        template<typename TOther>
        TDerived& operator+=(const TOther& rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && (XLType != xltypeErr) && (!std::is_fundamental_v<TOther>) &&
                     (std::convertible_to<TOther, TDerived> || contains_value<TOther::excel_type, OtherTypes...>)
        {
            using result_t = std::common_type_t<value_type, typename TOther::value_type>;
            value()        = static_cast<result_t>(value()) + static_cast<result_t>(rhs);
            return derived();
        }

        /**
         * \brief Addition assignment operator for adding a fundamental type value to a Base object.
         *
         * This operator performs addition between the value of the current Base object and a fundamental type value,
         * and assigns the result to the current Base object. It checks if the value_type of the Base class is an arithmetic type,
         * the Excel type is not an error type, and the provided value is a fundamental type that is convertible to the value_type.
         *
         * \tparam TValue The type of the value to add.
         * \param rhs The fundamental type value to add.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \requires The Excel type of the Base class must not be an error type.
         * \requires The provided value must be a fundamental type.
         * \requires The provided value must be convertible to value_type.
         * \return Reference to this object after the addition.
         */
        template<typename TValue>
        TDerived& operator+=(TValue rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::is_fundamental_v<TValue> &&
                     (XLType != xltypeErr) && std::convertible_to<TValue, value_type>
        {
            using result_t = std::common_type_t<value_type, TValue>;
            value()        = static_cast<result_t>(value()) + static_cast<result_t>(rhs);
            return derived();
        }

        /**
         * \brief Subtraction assignment operator for subtracting another object from a Base object.
         *
         * This operator performs subtraction between the value of the current Base object and the value of another object,
         * and assigns the result to the current Base object. It checks if the value_type of the Base class is an arithmetic type,
         * the Excel type is not an error type, and the other object is not a fundamental type. Additionally, it ensures that the other
         * object is either convertible to the derived type or has an allowed Excel type.
         *
         * \tparam TOther The type of the other object to subtract.
         * \param rhs The other object to subtract.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \requires The Excel type of the Base class must not be an error type.
         * \requires The other object must not be a fundamental type.
         * \requires The other object must be convertible to the derived type or have an allowed Excel type.
         * \return Reference to this object after the subtraction.
         */
        template<typename TOther>
        TDerived& operator-=(const TOther& rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && (XLType != xltypeErr) && (!std::is_fundamental_v<TOther>) &&
                     (std::convertible_to<TOther, TDerived> || contains_value<TOther::excel_type, OtherTypes...>)
        {
            using result_t = std::common_type_t<value_type, typename TOther::value_type>;
            value()        = static_cast<result_t>(value()) - static_cast<result_t>(rhs);
            return derived();
        }

        /**
         * \brief Subtraction assignment operator for subtracting a fundamental type value from a Base object.
         *
         * This operator performs subtraction between the value of the current Base object and a fundamental type value,
         * and assigns the result to the current Base object. It checks if the value_type of the Base class is an arithmetic type,
         * the Excel type is not an error type, and the provided value is a fundamental type that is convertible to the value_type.
         *
         * \tparam TValue The type of the value to subtract.
         * \param rhs The fundamental type value to subtract.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \requires The Excel type of the Base class must not be an error type.
         * \requires The provided value must be a fundamental type.
         * \requires The provided value must be convertible to value_type.
         * \return Reference to this object after the subtraction.
         */
        template<typename TValue>
        TDerived& operator-=(TValue rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::is_fundamental_v<TValue> &&
                     (XLType != xltypeErr) && std::convertible_to<TValue, value_type>
        {
            using result_t = std::common_type_t<value_type, TValue>;
            value()        = static_cast<result_t>(value()) - static_cast<result_t>(rhs);
            return derived();
        }

        /**
         * \brief Multiplication assignment operator for multiplying a Base object with another object.
         *
         * This operator performs multiplication between the value of the current Base object and the value of another object,
         * and assigns the result to the current Base object. It checks if the value_type of the Base class is an arithmetic type,
         * the Excel type is not an error type, and the other object is not a fundamental type. Additionally, it ensures that the other
         * object is either convertible to the derived type or has an allowed Excel type.
         *
         * \tparam TOther The type of the other object to multiply.
         * \param rhs The other object to multiply.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \requires The Excel type of the Base class must not be an error type.
         * \requires The other object must not be a fundamental type.
         * \requires The other object must be convertible to the derived type or have an allowed Excel type.
         * \return Reference to this object after the multiplication.
         */
        template<typename TOther>
        TDerived& operator*=(const TOther& rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && (XLType != xltypeErr) && (!std::is_fundamental_v<TOther>) &&
                     (std::convertible_to<TOther, TDerived> || contains_value<TOther::excel_type, OtherTypes...>)
        {
            using result_t = std::common_type_t<value_type, typename TOther::value_type>;
            value()        = static_cast<result_t>(value()) * static_cast<result_t>(rhs);
            return derived();
        }

        /**
         * \brief Multiplication assignment operator for multiplying a Base object with a fundamental type value.
         *
         * This operator performs multiplication between the value of the current Base object and a fundamental type value,
         * and assigns the result to the current Base object. It checks if the value_type of the Base class is an arithmetic type,
         * the Excel type is not an error type, and the provided value is a fundamental type that is convertible to the value_type.
         *
         * \tparam TValue The type of the value to multiply.
         * \param rhs The fundamental type value to multiply.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \requires The Excel type of the Base class must not be an error type.
         * \requires The provided value must be a fundamental type.
         * \requires The provided value must be convertible to value_type.
         * \return Reference to this object after the multiplication.
         */
        template<typename TValue>
        TDerived& operator*=(TValue rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::is_fundamental_v<TValue> &&
                     (XLType != xltypeErr) && std::convertible_to<TValue, value_type>
        {
            using result_t = std::common_type_t<value_type, TValue>;
            value()        = static_cast<result_t>(value()) * static_cast<result_t>(rhs);
            return derived();
        }

        /**
         * \brief Division assignment operator for dividing a Base object by another object.
         *
         * This operator performs division between the value of the current Base object and the value of another object,
         * and assigns the result to the current Base object. It checks if the value_type of the Base class is an arithmetic type,
         * the Excel type is not an error type, and the other object is not a fundamental type. Additionally, it ensures that the other
         * object is either convertible to the derived type or has an allowed Excel type.
         *
         * \tparam TOther The type of the other object to divide by.
         * \param rhs The other object to divide by.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \requires The Excel type of the Base class must not be an error type.
         * \requires The other object must not be a fundamental type.
         * \requires The other object must be convertible to the derived type or have an allowed Excel type.
         * \return Reference to this object after the division.
         */
        template<typename TOther>
        TDerived& operator/=(const TOther& rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && (XLType != xltypeErr) && (!std::is_fundamental_v<TOther>) &&
                     (std::convertible_to<TOther, TDerived> || contains_value<TOther::excel_type, OtherTypes...>)
        {
            using result_t = std::common_type_t<value_type, typename TOther::value_type>;
            value()        = static_cast<result_t>(value()) / static_cast<result_t>(rhs);
            return derived();
        }

        /**
         * \brief Division assignment operator for dividing a Base object by a fundamental type value.
         *
         * This operator performs division between the value of the current Base object and a fundamental type value,
         * and assigns the result to the current Base object. It checks if the value_type of the Base class is an arithmetic type,
         * the Excel type is not an error type, and the provided value is a fundamental type that is convertible to the value_type.
         *
         * \tparam TValue The type of the value to divide by.
         * \param rhs The fundamental type value to divide by.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \requires The Excel type of the Base class must not be an error type.
         * \requires The provided value must be a fundamental type.
         * \requires The provided value must be convertible to value_type.
         * \return Reference to this object after the division.
         */
        template<typename TValue>
        TDerived& operator/=(TValue rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::is_fundamental_v<TValue> &&
                     (XLType != xltypeErr) && std::convertible_to<TValue, value_type>
        {
            using result_t = std::common_type_t<value_type, TValue>;
            value()        = static_cast<result_t>(value()) / static_cast<result_t>(rhs);
            return derived();
        }

        /**
         * \brief Conversion operator to convert a Base object to a specified arithmetic type.
         *
         * This operator converts the value of the current Base object to a specified arithmetic type T.
         * It checks if the value_type of the Base class and the specified type T are arithmetic types,
         * and if the value_type is convertible to the specified type T. Additionally, it ensures that
         * the specified type T is not bool.
         *
         * \tparam T The type to convert the Base object to. Defaults to value_type.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \requires The specified type T must be an arithmetic type.
         * \requires The value_type must be convertible to the specified type T.
         * \requires The specified type T must not be bool.
         * \return The value of the Base object converted to the specified type T.
         */
        template<typename T = value_type>
        operator T() const
            requires std::is_arithmetic_v<T> && std::is_arithmetic_v<std::remove_cvref_t<value_type>> &&
                     std::convertible_to<value_type, T> && (not std::same_as<T, bool>)
        {
            ensure(is_valid());
            return static_cast<T>(value());
        }

        /**
         * \brief Conversion operator to convert a Base object to a boolean value.
         *
         * This operator converts the value of the current Base object to a boolean value.
         * It checks if the value_type of the Base class is an arithmetic type and the Excel type is not an error type.
         *
         * \requires The value_type of the Base class must be an arithmetic type.
         * \requires The Excel type of the Base class must not be an error type.
         * \return The boolean representation of the value of the Base object.
         */
        explicit operator bool() const
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && (XLType != xltypeErr)
        {
            return static_cast<bool>(value());
        }

        /**
         * \brief Output stream operator for the Base class.
         *
         * This operator outputs the value of the current Base object to the provided output stream.
         * It checks if the xltype of the Base object matches the XLType of the class.
         * If the types do not match, it throws a bad_cast exception.
         *
         * \param os The output stream to write to.
         * \param v The Base object to output.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \throws std::bad_cast if the xltype of the Base object does not match the XLType of the class.
         * \return The output stream after writing the value of the Base object.
         */
        friend std::ostream& operator<<(std::ostream& os, const Base& v)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>>
        {
            if (v.xltype != XLType) throw std::bad_cast();
            os << v.value();
            return os;
        }

        /**
         * \brief Converts the value of the current Base object to a specified type.
         *
         * This function returns the value of the current Base object converted to the specified type TValue.
         * It checks if the value_type of the Base class is an arithmetic type.
         *
         * \tparam TValue The type to convert the value to.
         * \requires The value_type of the Base class must be an arithmetic type.
         * \return The value of the current Base object converted to the specified type TValue.
         */
        template<typename TValue>
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>>
        TValue to() const
        {
            return value();
        }



        static TDerived* lift(XLOPER12* op) { return static_cast<TDerived*>(op); }

        static TDerived& lift(XLOPER12& op) { return static_cast<TDerived&>(op); }

        static const TDerived& lift(const XLOPER12& op) { return static_cast<const TDerived&>(op); }
    };

    template<typename TDerived, size_t ValueType, size_t... OtherTypes>
    void swap(Base<TDerived, ValueType, OtherTypes...>& lhs, Base<TDerived, ValueType, OtherTypes...>& rhs) noexcept
    {
        using std::swap;
        swap(lhs.xltype, rhs.xltype);
        swap(lhs.val, rhs.val);
    }

}    // namespace xll::impl