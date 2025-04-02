//
// Created by kenne on 31/03/2025.
//

#pragma once

#include <utility>
#include "../Types/Variant.hpp"
#include "../Types/Expected.hpp"


namespace xll {

    template<typename TFunction>
    auto transform(TFunction&& f)
    {
        return [f = std::forward<TFunction>(f)]<impl::is_valid_type TValue>(const xll::Expected<TValue>& ex) {
            return ex.transform(f);
        };
    }
}