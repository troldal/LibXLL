//
// Created by kenne on 23/03/2025.
//

#pragma once

#include <iostream>
#include <xlcall.h>

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

    template<typename TDerived, size_t ValueType, size_t... OtherTypes>
        requires is_excel_type<ValueType>
    class Base : public XLOPER12
    {
        TDerived&        derived() { return static_cast<TDerived&>(*this); }
        TDerived const& derived() const { return static_cast<TDerived const&>(*this); }

    protected:
        ~Base() = default;

        auto& value()
        {
            if constexpr (ValueType == xltypeNum)
                return val.num;
            else if constexpr (ValueType == xltypeStr)
                return val.str;
            else if constexpr (ValueType == xltypeBool)
                return val.xbool;
            else if constexpr (ValueType == xltypeErr)
                return val.err;
            else if constexpr (ValueType == xltypeMulti)
                return val.array;
            else                 // xltypeInt, xltypeMissing, xltypeNil, or other
                return val.w;    // Default case
        }

        auto value() const
        {
            if constexpr (ValueType == xltypeNum)
                return val.num;
            else if constexpr (ValueType == xltypeStr)
                return val.str;
            else if constexpr (ValueType == xltypeBool)
                return val.xbool;
            else if constexpr (ValueType == xltypeErr)
                return val.err;
            else if constexpr (ValueType == xltypeMulti)
                return val.array;
            else                 // xltypeInt, xltypeMissing, xltypeNil, or other
                return val.w;    // Default case
        }

    public:
        // clang-format off
        using value_type =
            std::conditional_t<ValueType == xltypeNum, decltype(val.num),
            std::conditional_t<ValueType == xltypeStr, decltype(val.str),
            std::conditional_t<ValueType == xltypeBool, decltype(val.xbool),
            std::conditional_t<ValueType == xltypeRef, decltype(val.mref),
            std::conditional_t<ValueType == xltypeSRef, decltype(val.sref),
            std::conditional_t<ValueType == xltypeErr, decltype(val.err),
            std::conditional_t<ValueType == xltypeFlow, decltype(val.flow),
            std::conditional_t<ValueType == xltypeMulti, decltype(val.array),
            std::conditional_t<ValueType == xltypeInt, decltype(val.w), void>>>>>>>>>;
        // clang-format on

        Base() : XLOPER12() { xltype = ValueType; }

        explicit Base(const XLOPER12& v)    // NOLINT
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>>
            : Base()
        {
            switch (v.xltype == ValueType) {
                case true:
                    xltype = v.xltype;
                    val    = v.val;
                    break;
                default:
                    xltype = xltypeNil;
            }
        }

        Base(const Base& other)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>>
            : Base()
        {
            switch (other.xltype == ValueType) {
                case true:
                    xltype = other.xltype;
                    val    = other.val;
                    break;
                default:
                    xltype = xltypeNil;
            }
        }

        Base(Base&& other) noexcept
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>>
            : Base()
        {
            using std::swap;

            switch (other.xltype == ValueType) {
                case true:
                    swap(other, *this);
                    break;
                default:
                    xltype = xltypeNil;
            }
        }

        template<typename TValue>
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::convertible_to<TValue, value_type>
        Base(TValue v) : Base()    // NOLINT
        {
            value() = v;
        }

        template<typename TThisDerived, size_t ThisValueType, size_t... Others>
            requires contains_value<ThisValueType, OtherTypes...>
        Base(Base<TThisDerived, ThisValueType, Others...>& other) : Base()    // NOLINT
        {
            value() = other.template to<value_type>();
        }

        Base& operator=(const Base& other)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>>
        {
            if (this == &other) return *this;
            switch (other.xltype == ValueType) {
                case true:
                    xltype = other.xltype;
                    val    = other.val;
                    break;
                default:
                    xltype = xltypeNil;
            }
            return *this;
        }

        Base& operator=(Base&& other) noexcept
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>>
        {
            using std::swap;

            if (this == &other) return *this;
            switch (other.xltype == ValueType) {
                case true:
                    swap(other, *this);
                    break;
                default:
                    xltype = xltypeNil;
            }
            return *this;
        }

        template<typename TValue>
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::convertible_to<TValue, value_type>
        Base& operator=(TValue v)
        {
            value() = v;
            return *this;
        }

        template<typename TThisDerived, size_t ThisValueType, size_t... Others>
            requires contains_value<ThisValueType, OtherTypes...>
        Base& operator=(Base<TThisDerived, ThisValueType, Others...>& v)
        {
            value() = v.template to<value_type>();
            return *this;
        }

        friend std::ostream& operator<<(std::ostream& os, const Base& v)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>>
        {
            if (v.xltype != ValueType) throw std::bad_cast();
            os << v.value();
            return os;
        }

        friend bool operator==(const Base& lhs, const Base& rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>>
        {
            return lhs.value() == rhs.value();
        }

        template<typename TValue>
        friend bool operator==(const Base& lhs, TValue rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::convertible_to<TValue, value_type>
        {
            return lhs.value() == rhs;
        }

        friend std::strong_ordering operator<=>(const Base& lhs, const Base& rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>>
        {
            return lhs.value() <=> rhs.value();
        }

        template<typename TValue>
        friend std::strong_ordering operator<=>(const Base& lhs, TValue rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && std::convertible_to<TValue, value_type>
        {
            return lhs.value() <=> rhs;
        }

        friend TDerived operator+(const Base& lhs, const Base& rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> &&
                (ValueType != xltypeErr)
        {
            return TDerived(lhs.value() + rhs.value());
        }

        template<typename TValue>
        friend TDerived operator+(const Base& lhs, TValue rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> &&
                     (ValueType != xltypeErr) && std::convertible_to<TValue, value_type>
        {
            return TDerived(lhs.value() + rhs);
        }

        friend TDerived operator-(const Base& lhs, const Base& rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> && (ValueType != xltypeErr)
        {
            return TDerived(lhs.value() - rhs.value());
        }

        template<typename TValue>
        friend TDerived operator-(const Base& lhs, TValue rhs)
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>> &&
                     (ValueType != xltypeErr) && std::convertible_to<TValue, value_type>
        {
            return TDerived(lhs.value() - rhs);
        }

        template<typename TValue>
            requires std::is_arithmetic_v<std::remove_cvref_t<value_type>>
        TValue to() const
        {
            return value();
        }

        [[nodiscard]]
        bool has_value() const
        {
            return xltype != xltypeNil;
        }

        static TDerived* lift(XLOPER12* op)
        {
            return static_cast<TDerived*>(op);
        }

        static TDerived& lift(XLOPER12& op)
        {
            return static_cast<TDerived&>(op);
        }

        XLOPER12& operator*() { return *this; }

    };

    template<typename TDerived, size_t ValueType, size_t... OtherTypes>
    void swap(Base<TDerived, ValueType, OtherTypes...>& lhs, Base<TDerived, ValueType, OtherTypes...>& rhs) noexcept
    {
        using std::swap;
        swap(lhs.xltype, rhs.xltype);
        swap(lhs.val, rhs.val);
    }

}    // namespace xll::impl