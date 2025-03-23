//
// Created by kenne on 21/03/2025.
//

#pragma once

#include <utility>

#include "../Types/Int.hpp"
#include "../Types/Nil.hpp"
#include "../Types/Number.hpp"
#include "../Types/String.hpp"
#include "../Utils/Traits.hpp"
#include "../xlFunctions/GetName.hpp"

#include <numeric>
#include <string>
#include <vector>

namespace xll
{

    template<typename TReturn>
    class Function
    {

        struct FunctionArgs
        {
            xll::String modulePath;
            xll::String procedureName;
            xll::String argTypes;
            xll::String functionName;
            xll::String argNames;
            XLOPER12    functionType        = xll::Nil();
            XLOPER12    functionCategory    = xll::Nil();
            XLOPER12    shortcutKey         = xll::Nil();
            XLOPER12    functionHelpTopic   = xll::Nil();
            XLOPER12    functionDescription = xll::Nil();

            std::vector<xll::String> argumentNames;
            std::vector<xll::String> argumentHelp;
        };

    public:
        // xll::String modulePath;
        // xll::String procedureName;
        // xll::String argTypes;
        // xll::String functionName;
        // xll::String argNames;
        // XLOPER12    functionType        = xll::Nil();
        // XLOPER12    functionCategory    = xll::Nil();
        // XLOPER12    shortcutKey         = xll::Nil();
        // XLOPER12    functionHelpTopic   = xll::Nil();
        // XLOPER12    functionDescription = xll::Nil();
        //
        // std::vector<xll::String> argumentNames;
        // std::vector<xll::String> argumentHelp;

        FunctionArgs args {};

        inline static std::vector<FunctionArgs> functionArgs {};

    public:
        enum Visibility { Hidden = 0, Visible = 1 };

        Function(String procedure, String functionText, Visibility vis = Hidden)
        {
            args.modulePath    = xll::get_name();
            args.procedureName = std::move(procedure);
            args.argTypes      = xll::String(traits::arg_traits<TReturn>::excel_type);
            args.functionName  = std::move(functionText);
            args.functionType  = xll::Int(vis == Hidden ? 0 : 1);
        }

        template<typename TArgument>
        Function& Argument(String arg_name, String arg_help)
        {
            using namespace xll::literals;

            auto arg = Arg<TArgument>(arg_name, arg_help);
            args.argTypes = args.argTypes + arg.type();
            args.argumentNames.emplace_back(std::move(arg.name()));
            args.argumentHelp.emplace_back(std::move(arg.help()));

            args.argNames = join(args.argumentNames, ","_xs);

            return *this;
        }

        int Register()
        {
            functionArgs.emplace_back(FunctionArgs());
            return 0;
        }


        Function& ThreadSafe()
        {
            using namespace xll::literals;

            args.argTypes = args.argTypes + "$"_xs;
            return *this;
        }

        xll::String join(const std::vector<xll::String>& strings, const xll::String& delimiter)
        {
            if (strings.empty()) return {};

            return std::accumulate(std::next(strings.begin()),
                                   strings.end(),
                                   strings[0],
                                   [&delimiter](const xll::String& a, const xll::String& b) { return a + delimiter + b; });
        }
    };
}    // namespace xll