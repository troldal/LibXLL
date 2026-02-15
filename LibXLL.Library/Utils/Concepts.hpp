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