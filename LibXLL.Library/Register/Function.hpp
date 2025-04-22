//
// Created by kenne on 21/03/2025.
//

#pragma once

#include <utility>

#include "Arg.hpp"

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

    namespace impl
    {
        struct FunctionArgs
        {
            xll::String modulePath;
            xll::String returnType;
            xll::String procedureName;
            xll::String argTypes;
            xll::String functionName;
            xll::String argNames;
            xll::Int    visibility = 1;
            xll::Variant functionCategory = xll::Nil();
            xll::String shortcutKey;
            xll::String functionHelpTopic;
            xll::String functionDescription;
            xll::String threadSafety;

            std::vector<xll::String> argumentNames;
            std::vector<xll::String> argumentHelp;
        };

        xll::String ProcedureName(const impl::FunctionArgs&);
        xll::String FunctionSignature(const impl::FunctionArgs&);
        xll::String FunctionName(const impl::FunctionArgs&);
        xll::String FunctionArguments(const impl::FunctionArgs&);
        xll::Int    FunctionVisibility(const impl::FunctionArgs&);
        xll::Variant FunctionCategory(const impl::FunctionArgs&);
        xll::String FunctionDescription(const impl::FunctionArgs&);
        xll::String FunctionHelp(const impl::FunctionArgs&);
        std::vector<xll::Variant> All(const impl::FunctionArgs&);
    }

    // template<typename TReturn>
    class Function
    {
        friend xll::String impl::ProcedureName(const impl::FunctionArgs&);
        friend xll::String impl::FunctionSignature(const impl::FunctionArgs&);
        friend xll::String impl::FunctionName(const impl::FunctionArgs&);
        friend xll::String impl::FunctionArguments(const impl::FunctionArgs&);
        friend xll::Int    impl::FunctionVisibility(const impl::FunctionArgs&);
        friend xll::Variant FunctionCategory(const impl::FunctionArgs&);
        friend xll::String FunctionDescription(const impl::FunctionArgs&);
        friend xll::String FunctionHelp(const impl::FunctionArgs&);
        friend std::vector<xll::Variant> All(const impl::FunctionArgs&);

    public:

        impl::FunctionArgs args {};

        inline static std::vector<impl::FunctionArgs> functionArgs {};

        // std::string m_text {};

    public:
        // enum Visibility { Hidden = 0, Visible = 1 };

        Function() = default;
        // Function() : m_text {"Hello, World!"} {}

        explicit Function(const xll::String& funcName)
        {
            args.functionName = funcName;
        }




        xll::String join(const std::vector<xll::String>& strings, const xll::String& delimiter)
        {
            if (strings.empty()) return {};

            return std::accumulate(std::next(strings.begin()),
                                   strings.end(),
                                   strings[0],
                                   [&delimiter](const xll::String& a, const xll::String& b) { return a + delimiter + b; });
        }


        template<typename TReturn>
        Function& Result()
        {
            args.returnType = xll::String(traits::arg_traits<TReturn>::excel_type);
            return *this;
        }

        Function& Procedure(const xll::String& procedureName)
        {
            args.procedureName = procedureName;
            return *this;
        }

        template<typename TArgument>
        Function& Parameter(const xll::String& name, const xll::String& help)
        {
            using namespace xll::literals;

            auto arg = Arg<TArgument>(name, help);
            args.argTypes = args.argTypes + arg.type();
            args.argumentNames.emplace_back(std::move(arg.name()));
            args.argumentHelp.emplace_back(std::move(arg.help()));

            args.argNames = join(args.argumentNames, ","_xs);

            return *this;
        }

        Function& Hidden()
        {
            args.visibility = 0;
            return *this;
        }

        Function& Category(const xll::String& category)
        {
            args.functionCategory = category;
            return *this;
        }

        Function& Description(const xll::String& functionDescription)
        {
            args.functionDescription = functionDescription;
            return *this;
        }

        Function& Help(const xll::String& functionHelp)
        {
            args.functionHelpTopic = functionHelp;
            return *this;
        }

        Function Register()
        {
            functionArgs.emplace_back(args);
            return {};
        }

        Function& ThreadSafe()
        {
            using namespace xll::literals;

            args.threadSafety = "$"_xs;
            return *this;
        }
    };

    template<typename TReturn>
    auto Result()
    {
        return [](Function&& lhs) { return lhs.Result<TReturn>(); };
    }

    inline auto Procedure(const xll::String& procedureName)
    {
        return [procedureName](Function&& lhs) { return lhs.Procedure(procedureName); };
    }

    template<typename TArgument>
    auto Parameter(const xll::String& name, const xll::String& help)
    {
        return [name,help](Function&& lhs) { return lhs.Parameter<TArgument>(name, help); };
    }

    inline auto Hidden()
    {
        return [](Function&& lhs) { return lhs.Hidden(); };
    }

    inline auto Category(const xll::String& category)
    {
        return [category](Function&& lhs) { return lhs.Category(category); };
    }

    inline auto Description(const xll::String& functionDescription)
    {
        return [functionDescription](Function&& lhs) { return lhs.Description(functionDescription); };
    }

    inline auto Help(const xll::String& functionHelp)
    {
        return [functionHelp](Function&& lhs) { return lhs.Help(functionHelp + "!0"); };
    }

    inline auto Register()
    {
        return [](auto&& lhs) { return lhs.Register(); };
    }

    inline auto ThreadSafe()
    {
        return [](Function&& lhs) { return lhs.ThreadSafe(); };
    }

    namespace impl
    {
        inline xll::String ProcedureName(const impl::FunctionArgs& args) { return args.procedureName; }

        inline xll::String FunctionSignature(const impl::FunctionArgs& args) { return args.returnType + args.argTypes + args.threadSafety; }

        inline xll::String FunctionName(const impl::FunctionArgs& args) { return args.functionName; }

        inline xll::String FunctionArguments(const impl::FunctionArgs& args) { return args.argNames; }

        inline xll::Int FunctionVisibility(const impl::FunctionArgs& args) { return args.visibility; }

        inline xll::Variant FunctionCategory(const impl::FunctionArgs& args) { return args.functionCategory; }

        inline xll::String FunctionDescription(const impl::FunctionArgs& args) { return args.functionDescription; }

        inline xll::String FunctionHelp(const impl::FunctionArgs& args) { return args.functionHelpTopic; }

        inline std::vector<xll::Variant> All(const impl::FunctionArgs& args)
        {
            auto result = std::vector<xll::Variant> {};
            result.emplace_back(xll::get_name());
            result.emplace_back(ProcedureName(args));
            result.emplace_back(FunctionSignature(args));
            result.emplace_back(FunctionName(args));
            result.emplace_back(FunctionArguments(args));
            result.emplace_back(FunctionVisibility(args));
            result.emplace_back(FunctionCategory(args));
            result.emplace_back(xll::Nil());
            result.emplace_back(FunctionHelp(args));
            result.emplace_back(FunctionDescription(args));

            for (auto item : args.argumentHelp) result.emplace_back(item);

            // Addressing a bug in Excel that removes the last two characters of a string.
            xll::get<xll::String>(result.back()) = xll::get<xll::String>(result.back()) + "  ";

            return result;
        }
    }    // namespace impl

}    // namespace xll