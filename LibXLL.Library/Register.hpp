//
// Created by kenne on 28/03/2025.
//

#pragma once

#include "Utils/Pipe.hpp"

#include "Macros/Defines.hpp"
#include "Register/Function.hpp"
#include "Register/AddIn.hpp"
#include "Register/Registry.hpp"

#define XLL_FUNCTION extern "C" XLL_EXPORTS

#define XLL_REGISTER(func) extern const xll::AddIn func##_registered(func)
