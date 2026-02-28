//
// Created by kenne on 31/03/2025.
//

#pragma once

#include "../ExcelSDK/xlcall.hpp"
#include <memory>
#include <type_traits>

namespace xll
{

    // Helper concept to detect unique_ptr
    template<typename T>
    concept is_unique_ptr = requires(T t) { typename std::unique_ptr<typename T::element_type, typename T::deleter_type>; };

    // =========================================================================
    // is_tuple_type — forward declaration
    //
    // The full specialisation lives in Tuple.hpp (after Tuple is defined).
    // The default is false so Any.hpp can safely use !is_tuple_type<T> without
    // including Tuple.hpp, avoiding a circular dependency.
    // =========================================================================

    namespace impl
    {
        template<typename T>
        struct is_tuple_impl : std::false_type {};

        template<typename T>
        struct is_string_enum_impl : std::false_type {};
    }

    template<typename T>
    concept is_tuple_type = impl::is_tuple_impl<std::remove_cvref_t<T>>::value;

    /// Satisfied only by specialisations of xll::StringEnum.
    template<typename T>
    concept is_string_enum_type = impl::is_string_enum_impl<std::remove_cvref_t<T>>::value;

    /**
     * @brief Concept that checks if a type is a valid xll type for use in Expected.
     *
     * This concept ensures that types used in Expected<TValue, TError> are proper xll types
     * that inherit from impl::Base and have the required static members for Excel integration.
     *
     * Valid xll types include:
     * - xll::String
     * - xll::Number
     * - xll::Int
     * - xll::Bool
     * - xll::Error
     * - xll::Missing
     * - xll::Nil
     * - xll::Array<T>
     * - xll::Variant<T, Ts...>
     *
     * @tparam T The type to check
     *
     * @note This concept checks for:
     *       - Inheritance from XLOPER12
     *       - Presence of has_crtp_base static member (indicating impl::Base inheritance)
     *       - Presence of excel_type static member (defining the xltype constant)
     *       - Proper size and alignment constraints
     */
    template<typename T>
    concept is_xll_type = requires {
        // Must inherit from XLOPER12 (fundamental requirement)
        requires std::is_base_of_v<XLOPER12, T>;

        // Must have the CRTP base marker (indicates impl::Base<...> inheritance)
        // requires T::has_crtp_base == true;

        // Must have excel_type static member defining the xltype
        { T::excel_type } -> std::convertible_to<size_t>;

        // Must fit within XLOPER12 constraints
        requires sizeof(T) == sizeof(XLOPER12);
        requires alignof(T) <= alignof(XLOPER12);

        // Must not have virtual functions (would corrupt vtable during type punning)
        requires !std::is_polymorphic_v<T>;
    };
}    // namespace xll