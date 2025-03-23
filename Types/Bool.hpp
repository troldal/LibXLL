//
// Created by kenne on 23/03/2025.
//

#pragma once

#include "Base.hpp"
#include <cmath>

namespace xll
{
  class Bool : public impl::Base<Bool, xltypeBool, xltypeInt, xltypeNum>
  {
    using BASE = impl::Base<Bool, xltypeBool, xltypeInt, xltypeNum>;

  public:
    using BASE::BASE;
    using BASE::operator=;

    Bool() : Bool(false) {}

  };
}
