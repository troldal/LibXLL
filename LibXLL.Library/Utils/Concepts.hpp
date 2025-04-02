//
// Created by kenne on 31/03/2025.
//

#pragma once

#include <memory>
#include <type_traits>

namespace xll {

    // Helper concept to detect unique_ptr
    template<typename T>
    concept is_unique_ptr = requires(T t) { typename std::unique_ptr<typename T::element_type, typename T::deleter_type>; };
}