//
// Created by kenne on 21/03/2025.
//

#pragma once

#include "../Types/Int.hpp"
#include "../Types/Nil.hpp"
#include "../Types/Number.hpp"
#include "../Types/String.hpp"

namespace xll {

    struct FunctionArgs {

        XLOPER12 modulePath = xll::Nil();
        XLOPER12 procedureName = xll::Nil();
        XLOPER12 argTypes = xll::Nil();
        XLOPER12 functionName = xll::Nil();
        XLOPER12 argNames = xll::Nil();
        XLOPER12 functionType = xll::Nil();
        XLOPER12 functionCategory = xll::Nil();
        XLOPER12 shortcutKey = xll::Nil();
        XLOPER12 functionHelpTopic = xll::Nil();
        XLOPER12 functionDescription = xll::Nil();
        std::vector<XLOPER12> argumentHelp;

    };
}