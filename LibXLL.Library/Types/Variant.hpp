//
// Created by kenne on 23/03/2025.
//

#pragma once

#include "Bool.hpp"
#include "Error.hpp"
#include "Int.hpp"
#include "Missing.hpp"
#include "Nil.hpp"
#include "Number.hpp"
#include "String.hpp"

#include <functional>
#include <variant>

namespace xll
{
    namespace impl
    {
        template<typename T>
        concept is_valid_type = std::same_as<T, xll::Bool> || std::same_as<T, Error> || std::same_as<T, Int> || std::same_as<T, Number> ||
                                std::same_as<T, String> || std::same_as<T, Nil> || std::same_as<T, Missing>;

        template<typename... Types>
        concept unique_types = (sizeof...(Types) == 1) || ((std::is_same_v<Types, Types> + ...) == sizeof...(Types));

        template<typename... Types>
        concept contains_one_nil = ((std::is_same_v<Types, xll::Nil> + ...) == 1);
    }    // namespace impl

    template<typename T, typename... Ts>
    requires(impl::is_valid_type<T> && (impl::is_valid_type<Ts> && ...) &&
             impl::contains_one_nil<T, Ts...> &&
             impl::unique_types<T, Ts...> &&
             !std::same_as<T, xll::Missing> &&
             !(std::same_as<Ts, xll::Missing> || ...))
    class Variant;

    template<typename U, typename T, typename... Ts>
        requires (std::same_as<U, T> || (std::same_as<U, Ts> || ...))
    constexpr bool holds_alternative(const Variant<T, Ts...>& v);

    template<typename U, typename T, typename... Ts>
        requires (std::same_as<U, T> || (std::same_as<U, Ts> || ...))
    constexpr U& get(Variant<T, Ts...>& v);

    template<typename U, typename T, typename... Ts>
        requires (std::same_as<U, T> || (std::same_as<U, Ts> || ...))
    constexpr const U& get(const Variant<T, Ts...>& v);

    template<typename Visitor, typename T, typename... Ts>
    constexpr auto visit(Visitor&& vis, const Variant<T, Ts...>& va);

    template<class... Ts>
    struct overload : Ts...
    {
        using Ts::operator()...;
    };

    template<typename T, typename... Ts>
    requires(impl::is_valid_type<T> && (impl::is_valid_type<Ts> && ...) &&
             impl::contains_one_nil<T, Ts...> &&
             impl::unique_types<T, Ts...> &&
             !std::same_as<T, xll::Missing> &&
             !(std::same_as<Ts, xll::Missing> || ...))
    class Variant : public XLOPER12
    {
    public:

        // Bit pattern for the xltype field of the XLOPER12 structure
        static constexpr size_t excel_type = xltypeMissing | T::excel_type | (Ts::excel_type | ...);

        constexpr Variant() : XLOPER12()
        {
            xltype = T().xltype;
            reinterpret_cast<T&>(*this) = T();
        }

        template<typename U>
            requires(std::same_as<U, xll::Missing> || std::same_as<U, T> || (std::same_as<U, Ts> || ...))
        constexpr Variant(const U& u) : Variant()    // NOLINT
        {
            if constexpr (std::same_as<U, xll::Missing>) {
                xltype                             = xll::Nil().xltype;
                reinterpret_cast<xll::Nil&>(*this) = xll::Nil();
            }
            else {
                xltype                      = u.xltype;
                reinterpret_cast<U&>(*this) = u;
            }
        }

        template<typename U>
            requires(std::same_as<U, xll::Missing> || std::same_as<U, T> || (std::same_as<U, Ts> || ...))
        constexpr Variant(U&& u) noexcept : Variant()   // NOLINT
        {
            if constexpr (std::same_as<U, xll::Missing>) {
                xltype                             = xll::Nil().xltype;
                reinterpret_cast<xll::Nil&>(*this) = xll::Nil();
            }
            else {
                xltype                      = u.xltype;
                reinterpret_cast<U&>(*this) = std::move(u);
            }
        }

        constexpr Variant(const Variant& v) : Variant()
        {
            xltype = v.xltype;

            if constexpr ((std::same_as<xll::Bool, T> || (std::same_as<xll::Bool, Ts> || ...)))
                if (xll::holds_alternative<xll::Bool>(v))
                    xll::get<xll::Bool>(*this) = xll::get<xll::Bool>(v);

            if constexpr ((std::same_as<xll::Error, T> || (std::same_as<xll::Error, Ts> || ...)))
                if (xll::holds_alternative<xll::Error>(v))
                    xll::get<xll::Error>(*this) = xll::get<xll::Error>(v);

            if constexpr ((std::same_as<xll::Int, T> || (std::same_as<xll::Int, Ts> || ...)))
                if (xll::holds_alternative<xll::Int>(v))
                    xll::get<xll::Int>(*this) = xll::get<xll::Int>(v);

            if constexpr ((std::same_as<xll::Number, T> || (std::same_as<xll::Number, Ts> || ...)))
                if (xll::holds_alternative<xll::Number>(v))
                    xll::get<xll::Number>(*this) = xll::get<xll::Number>(v);

            if constexpr ((std::same_as<xll::String, T> || (std::same_as<xll::String, Ts> || ...)))
                if (xll::holds_alternative<xll::String>(v))
                    xll::get<xll::String>(*this) = xll::get<xll::String>(v);

            if constexpr ((std::same_as<xll::Nil, T> || (std::same_as<xll::Nil, Ts> || ...)))
                if (xll::holds_alternative<xll::Nil>(v))
                    xll::get<xll::Nil>(*this) = xll::get<xll::Nil>(v);
        }

        constexpr Variant(Variant&& v) noexcept : Variant()
        {
            xltype = v.xltype;

            if constexpr ((std::same_as<xll::Bool, T> || (std::same_as<xll::Bool, Ts> || ...)))
                if (xll::holds_alternative<xll::Bool>(v))
                    xll::get<xll::Bool>(*this) = xll::get<xll::Bool>(v);

            if constexpr ((std::same_as<xll::Error, T> || (std::same_as<xll::Error, Ts> || ...)))
                if (xll::holds_alternative<xll::Error>(v))
                    xll::get<xll::Error>(*this) = xll::get<xll::Error>(v);

            if constexpr ((std::same_as<xll::Int, T> || (std::same_as<xll::Int, Ts> || ...)))
                if (xll::holds_alternative<xll::Int>(v))
                    xll::get<xll::Int>(*this) = xll::get<xll::Int>(v);

            if constexpr ((std::same_as<xll::Number, T> || (std::same_as<xll::Number, Ts> || ...)))
                if (xll::holds_alternative<xll::Number>(v))
                    xll::get<xll::Number>(*this) = xll::get<xll::Number>(v);

            if constexpr ((std::same_as<xll::String, T> || (std::same_as<xll::String, Ts> || ...)))
                if (xll::holds_alternative<xll::String>(v))
                    xll::get<xll::String>(*this) = xll::get<xll::String>(v);

            if constexpr ((std::same_as<xll::Nil, T> || (std::same_as<xll::Nil, Ts> || ...)))
                if (xll::holds_alternative<xll::Nil>(v))
                    xll::get<xll::Nil>(*this) = xll::get<xll::Nil>(v);
        }

        constexpr ~Variant()
        {
            if constexpr ((std::same_as<xll::Bool, T> || (std::same_as<xll::Bool, Ts> || ...)))
                if (xll::holds_alternative<xll::Bool>(*this))
                    xll::get<xll::Bool>(*this).~Bool();

            if constexpr ((std::same_as<xll::Error, T> || (std::same_as<xll::Error, Ts> || ...)))
                if (xll::holds_alternative<xll::Error>(*this))
                    xll::get<xll::Error>(*this).~Error();

            if constexpr ((std::same_as<xll::Int, T> || (std::same_as<xll::Int, Ts> || ...)))
                if (xll::holds_alternative<xll::Int>(*this))
                    xll::get<xll::Int>(*this).~Int();

            if constexpr ((std::same_as<xll::Number, T> || (std::same_as<xll::Number, Ts> || ...)))
                if (xll::holds_alternative<xll::Number>(*this))
                    xll::get<xll::Number>(*this).~Number();

            if constexpr ((std::same_as<xll::String, T> || (std::same_as<xll::String, Ts> || ...)))
                if (xll::holds_alternative<xll::String>(*this))
                    xll::get<xll::String>(*this).~String();

            if constexpr ((std::same_as<xll::Nil, T> || (std::same_as<xll::Nil, Ts> || ...)))
                if (xll::holds_alternative<xll::Nil>(*this))
                    xll::get<xll::Nil>(*this).~Nil();

            static_cast<XLOPER12&>(*this) = XLOPER12();
        }

        constexpr Variant& operator=(const Variant& v)
        {
            this->~Variant();
            static_cast<XLOPER12&>(*this) = XLOPER12();
            xltype = v.xltype;

            if constexpr ((std::same_as<xll::Bool, T> || (std::same_as<xll::Bool, Ts> || ...)))
                if (xll::holds_alternative<xll::Bool>(v))
                    xll::get<xll::Bool>(*this) = xll::get<xll::Bool>(v);

            if constexpr ((std::same_as<xll::Error, T> || (std::same_as<xll::Error, Ts> || ...)))
                if (xll::holds_alternative<xll::Error>(v))
                    xll::get<xll::Error>(*this) = xll::get<xll::Error>(v);

            if constexpr ((std::same_as<xll::Int, T> || (std::same_as<xll::Int, Ts> || ...)))
                if (xll::holds_alternative<xll::Int>(v))
                    xll::get<xll::Int>(*this) = xll::get<xll::Int>(v);

            if constexpr ((std::same_as<xll::Number, T> || (std::same_as<xll::Number, Ts> || ...)))
                if (xll::holds_alternative<xll::Number>(v))
                    xll::get<xll::Number>(*this) = xll::get<xll::Number>(v);

            if constexpr ((std::same_as<xll::String, T> || (std::same_as<xll::String, Ts> || ...)))
                if (xll::holds_alternative<xll::String>(v))
                    xll::get<xll::String>(*this) = xll::get<xll::String>(v);

            if constexpr ((std::same_as<xll::Nil, T> || (std::same_as<xll::Nil, Ts> || ...)))
                if (xll::holds_alternative<xll::Nil>(v))
                    xll::get<xll::Nil>(*this) = xll::get<xll::Nil>(v);

            return *this;
        }

        constexpr Variant& operator=(Variant&& v) noexcept
        {
            this->~Variant();
            static_cast<XLOPER12&>(*this) = XLOPER12();
            xltype = v.xltype;

            if constexpr ((std::same_as<xll::Bool, T> || (std::same_as<xll::Bool, Ts> || ...)))
                if (xll::holds_alternative<xll::Bool>(v))
                    xll::get<xll::Bool>(*this) = xll::get<xll::Bool>(v);

            if constexpr ((std::same_as<xll::Error, T> || (std::same_as<xll::Error, Ts> || ...)))
                if (xll::holds_alternative<xll::Error>(v))
                    xll::get<xll::Error>(*this) = xll::get<xll::Error>(v);

            if constexpr ((std::same_as<xll::Int, T> || (std::same_as<xll::Int, Ts> || ...)))
                if (xll::holds_alternative<xll::Int>(v))
                    xll::get<xll::Int>(*this) = xll::get<xll::Int>(v);

            if constexpr ((std::same_as<xll::Number, T> || (std::same_as<xll::Number, Ts> || ...)))
                if (xll::holds_alternative<xll::Number>(v))
                    xll::get<xll::Number>(*this) = xll::get<xll::Number>(v);

            if constexpr ((std::same_as<xll::String, T> || (std::same_as<xll::String, Ts> || ...)))
                if (xll::holds_alternative<xll::String>(v))
                    xll::get<xll::String>(*this) = xll::get<xll::String>(v);

            if constexpr ((std::same_as<xll::Nil, T> || (std::same_as<xll::Nil, Ts> || ...)))
                if (xll::holds_alternative<xll::Nil>(v))
                    xll::get<xll::Nil>(*this) = xll::get<xll::Nil>(v);

            return *this;
        }

        template<typename U>
            requires(std::same_as<U, xll::Missing> || std::same_as<U, T> || (std::same_as<U, Ts> || ...))
        constexpr Variant& operator=(const U& u)
        {
            this->~Variant();
            static_cast<XLOPER12&>(*this)          = XLOPER12();

            if constexpr (std::same_as<U, xll::Missing>) {
                xltype                             = xll::Nil().xltype;
                reinterpret_cast<xll::Nil&>(*this) = xll::Nil();
            }
            else {
                xltype                      = u.xltype;
                reinterpret_cast<U&>(*this) = u;
            }

            return *this;
        }

        template<typename U>
            requires(std::same_as<U, xll::Missing> || std::same_as<U, T> || (std::same_as<U, Ts> || ...))
        constexpr Variant& operator=(U&& u) noexcept
        {
            this->~Variant();
            static_cast<XLOPER12&>(*this) = XLOPER12();

            if constexpr (std::same_as<U, xll::Missing>) {
                xltype                             = xll::Nil().xltype;
                reinterpret_cast<xll::Nil&>(*this) = xll::Nil();
            }
            else {
                xltype                      = u.xltype;
                reinterpret_cast<U&>(*this) = std::move(u);
            }

            return *this;
        }
        // template<typename T>
        //     requires impl::is_valid_type<T>
        // Variant& operator=(T&& t) noexcept
        // {
        //     this->~Variant();
        //     static_cast<XLOPER12&>(*this) = XLOPER12();
        //     T::lift(*static_cast<XLOPER12*>(this)) = T();
        //     xll::get<T>(*this) = t;
        //     return *this;
        // }

        // [[nodiscard]]
        // auto index() const
        // {
        //     return xltype;
        // }

        // XLOPER12& operator*() { return static_cast<XLOPER12&>(*this); }
    };


    template<typename U, typename T, typename... Ts>
        requires (std::same_as<U, T> || (std::same_as<U, Ts> || ...))
    constexpr bool holds_alternative(const Variant<T, Ts...>& v)
    {
        if (std::same_as<U, Bool> && v.xltype == xltypeBool) return true;
        if (std::same_as<U, Error> && v.xltype == xltypeErr) return true;
        if (std::same_as<U, Int> && v.xltype == xltypeInt) return true;
        if (std::same_as<U, Number> && v.xltype == xltypeNum) return true;
        if (std::same_as<U, String> && v.xltype == xltypeStr) return true;
        if (std::same_as<U, Nil> && (v.xltype == xltypeNil || v.xltype == xltypeMissing)) return true;

        return false;
    }

    template<typename U, typename T, typename... Ts>
        requires (std::same_as<U, T> || (std::same_as<U, Ts> || ...))
    constexpr U& get(Variant<T, Ts...>& v)
    {
        if (not holds_alternative<U>(v)) throw std::bad_variant_access();
        return reinterpret_cast<U&>(v);
    }

    template<typename U, typename T, typename... Ts>
        requires (std::same_as<U, T> || (std::same_as<U, Ts> || ...))
    constexpr const U& get(const Variant<T, Ts...>& v)
    {
        if (not holds_alternative<U>(v)) throw std::bad_variant_access();
        return reinterpret_cast<const U&>(v);
    }

    template<typename Visitor, typename T, typename... Ts>
    constexpr auto visit(Visitor&& vis, const Variant<T, Ts...>& va)
    {
        if constexpr (std::same_as<xll::Bool, T> || (std::same_as<xll::Bool, Ts> || ...))
            if (holds_alternative<xll::Bool>(va))
                return std::invoke(std::forward<Visitor>(vis), get<xll::Bool>(va));

        if constexpr (std::same_as<xll::Error, T> || (std::same_as<xll::Error, Ts> || ...))
            if (holds_alternative<xll::Error>(va))
                return std::invoke(std::forward<Visitor>(vis), get<xll::Error>(va));

        if constexpr (std::same_as<xll::Int, T> || (std::same_as<xll::Int, Ts> || ...))
            if (holds_alternative<xll::Int>(va))
                return std::invoke(std::forward<Visitor>(vis), get<xll::Int>(va));

        if constexpr (std::same_as<xll::Number, T> || (std::same_as<xll::Number, Ts> || ...))
            if (holds_alternative<xll::Number>(va))
                return std::invoke(std::forward<Visitor>(vis), get<xll::Number>(va));

        if constexpr (std::same_as<xll::String, T> || (std::same_as<xll::String, Ts> || ...))
            if (holds_alternative<xll::String>(va))
                return std::invoke(std::forward<Visitor>(vis), get<xll::String>(va));

        if constexpr (std::same_as<xll::Nil, T> || (std::same_as<xll::Nil, Ts> || ...))
            if (holds_alternative<xll::Nil>(va))
                return std::invoke(std::forward<Visitor>(vis), get<xll::Nil>(va));

        throw std::bad_variant_access();
    }

}    // namespace xll