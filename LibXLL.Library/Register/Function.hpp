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
        struct ProcedureArgs
        {
            xll::String modulePath;
            xll::String returnType;
            xll::String procedureName;
            xll::String argTypes;
            xll::String functionName;
            xll::String argNames;
            xll::Int    visibility = 1;
            xll::Variant<xll::Nil, xll::String, xll::Int, xll::Number> functionCategory = xll::Nil();
            xll::String shortcutKey;
            xll::String functionHelpTopic;
            xll::String functionDescription;
            xll::String threadSafety;

            std::vector<xll::String> argumentNames;
            std::vector<xll::String> argumentHelp;
        };

        xll::String ProcedureName(const impl::ProcedureArgs&);
        xll::String FunctionSignature(const impl::ProcedureArgs&);
        xll::String FunctionName(const impl::ProcedureArgs&);
        xll::String FunctionArguments(const impl::ProcedureArgs&);
        xll::Int    FunctionVisibility(const impl::ProcedureArgs&);
        xll::Variant<xll::Nil, xll::String, xll::Int, xll::Number> FunctionCategory(const impl::ProcedureArgs&);
        xll::String FunctionDescription(const impl::ProcedureArgs&);
        xll::String FunctionHelp(const impl::ProcedureArgs&);
        xll::Variant<xll::Nil, xll::String, xll::Int, xll::Number> ShortcutKey(const impl::ProcedureArgs&);
        std::vector<xll::Variant<xll::Nil, xll::String, xll::Int, xll::Number>> All(const impl::ProcedureArgs&);
    }

    // -----------------------------------------------------------------------
    // FunctionBase — common data and builder methods shared by Function and
    // Command.  The deducing-this pattern lets each method return a reference
    // to the most-derived type, keeping builder chains well-typed.
    // -----------------------------------------------------------------------
    class FunctionBase
    {
    public:
        impl::ProcedureArgs args {};

        inline static std::vector<impl::ProcedureArgs> functionArgs {};

        /// Sets the name of the exported C/C++ procedure in the DLL.
        template<typename Self>
        auto& Procedure(this Self& self, const xll::String& procedureName)
        {
            self.args.procedureName = procedureName;
            return self;
        }

        /// Sets the function-wizard / Macro-dialog category.
        template<typename Self>
        auto& Category(this Self& self, const xll::String& category)
        {
            self.args.functionCategory = category;
            return self;
        }

        /// Sets the description shown in the Function Wizard / Macro dialog.
        template<typename Self>
        auto& Description(this Self& self, const xll::String& description)
        {
            self.args.functionDescription = description;
            return self;
        }

        /**
         * @brief Sets the help topic URL.
         * @param help  URL of the help page (without the `!0` suffix —
         *              that is appended automatically by the free `Help()`
         *              pipeline wrapper).
         */
        template<typename Self>
        auto& Help(this Self& self, const xll::String& help)
        {
            self.args.functionHelpTopic = help;
            return self;
        }
    };

    // template<typename TReturn>
    class Function : public FunctionBase
    {
        friend xll::String impl::ProcedureName(const impl::ProcedureArgs&);
        friend xll::String impl::FunctionSignature(const impl::ProcedureArgs&);
        friend xll::String impl::FunctionName(const impl::ProcedureArgs&);
        friend xll::String impl::FunctionArguments(const impl::ProcedureArgs&);
        friend xll::Int    impl::FunctionVisibility(const impl::ProcedureArgs&);
        friend xll::Variant<xll::Nil, xll::String, xll::Int, xll::Number> FunctionCategory(const impl::ProcedureArgs&);
        friend xll::String FunctionDescription(const impl::ProcedureArgs&);
        friend xll::String FunctionHelp(const impl::ProcedureArgs&);
        friend std::vector<xll::Variant<xll::Nil, xll::String, xll::Int, xll::Number>> All(const impl::ProcedureArgs&);

    public:
        Function() = default;

        explicit Function(const xll::String& funcName)
        {
            args.functionName = funcName;
        }

        template<typename TReturn>
        Function& Result()
        {
            args.returnType = xll::String(traits::arg_traits<TReturn>::excel_type);
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
        return [procedureName](auto&& lhs) { return lhs.Procedure(procedureName); };
    }

    template<typename TArgument>
    auto Parameter(const xll::String& name, const xll::String& help)
    {
        return [name, help](Function&& lhs) { return lhs.Parameter<TArgument>(name, help); };
    }

    inline auto Hidden()
    {
        return [](Function&& lhs) { return lhs.Hidden(); };
    }

    inline auto Category(const xll::String& category)
    {
        return [category](auto&& lhs) { return lhs.Category(category); };
    }

    inline auto Description(const xll::String& functionDescription)
    {
        return [functionDescription](auto&& lhs) { return lhs.Description(functionDescription); };
    }

    inline auto Help(const xll::String& functionHelp)
    {
        return [functionHelp](auto&& lhs) { return lhs.Help(functionHelp + "!0"); };
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
        inline xll::String ProcedureName(const impl::ProcedureArgs& args) { return args.procedureName; }

        inline xll::String FunctionSignature(const impl::ProcedureArgs& args) { return args.returnType + args.argTypes + args.threadSafety; }

        inline xll::String FunctionName(const impl::ProcedureArgs& args) { return args.functionName; }

        inline xll::String FunctionArguments(const impl::ProcedureArgs& args) { return args.argNames; }

        inline xll::Int FunctionVisibility(const impl::ProcedureArgs& args) { return args.visibility; }

        inline xll::Variant<xll::Nil, xll::String, xll::Int, xll::Number> FunctionCategory(const impl::ProcedureArgs& args) { return args.functionCategory; }

        inline xll::String FunctionDescription(const impl::ProcedureArgs& args) { return args.functionDescription; }

        inline xll::String FunctionHelp(const impl::ProcedureArgs& args) { return args.functionHelpTopic; }

        inline xll::Variant<xll::Nil, xll::String, xll::Int, xll::Number> ShortcutKey(const impl::ProcedureArgs& args)
        {
            if (args.shortcutKey.empty()) return xll::Nil();
            return args.shortcutKey;
        }

        inline std::vector<xll::Variant<xll::Nil, xll::String, xll::Int, xll::Number>> All(const impl::ProcedureArgs& args)
        {
            auto result = std::vector<xll::Variant<xll::Nil, xll::String, xll::Int, xll::Number>> {};
            result.emplace_back(*xll::get_name());
            result.emplace_back(ProcedureName(args));
            result.emplace_back(FunctionSignature(args));
            result.emplace_back(FunctionName(args));
            result.emplace_back(FunctionArguments(args));
            result.emplace_back(FunctionVisibility(args));
            result.emplace_back(FunctionCategory(args));
            result.emplace_back(ShortcutKey(args));
            result.emplace_back(FunctionHelp(args));
            result.emplace_back(FunctionDescription(args));

            for (auto item : args.argumentHelp) result.emplace_back(item);

            // Addressing a bug in Excel that removes the last two characters of a string.
            xll::get<xll::String>(result.back()) = xll::get<xll::String>(result.back()) + "  ";

            return result;
        }
    }    // namespace impl

    // -----------------------------------------------------------------------
    // Command — builder for Excel XLL command macros (macro type 2).
    //
    // Commands differ from Functions in three ways:
    //   • visibility is 2 (command/macro) rather than 1 (worksheet function).
    //   • shortcutKey is meaningful; returnType and argTypes are not used.
    //   • There are no parameters — Excel does not pass arguments to commands.
    // -----------------------------------------------------------------------
    class Command : public FunctionBase
    {
    public:
        Command() = default;

        /**
         * @brief Constructs a Command with the given name.
         *
         * Sets `visibility` to 2 (Excel macro type for commands) so that
         * `xlfRegister` registers the procedure as a command rather than a
         * worksheet function.
         *
         * @param commandName  The name Excel users use to run the command.
         */
        explicit Command(const xll::String& commandName)
        {
            args.functionName = commandName;
            args.visibility   = 2;
        }

        /**
         * @brief Sets the keyboard shortcut used to run this command.
         *
         * @param key  A single uppercase letter (e.g. `"A"`).  Excel runs the
         *             command when the user presses Ctrl+Shift+<key>.
         */
        Command& ShortcutKey(const xll::String& key)
        {
            args.shortcutKey = key;
            return *this;
        }

        /// Pushes the completed command description into the shared
        /// registration queue so that `xlAutoOpen` can register it with Excel.
        void Register()
        {
            functionArgs.emplace_back(args);
        }
    };

    // -----------------------------------------------------------------------
    // Free-function pipeline wrapper — ShortcutKey (Command only)
    // -----------------------------------------------------------------------

    /**
     * @brief Pipeline wrapper that sets the keyboard shortcut of a `Command`.
     *
     * @param key  A single uppercase letter.  Excel runs the command when the
     *             user presses Ctrl+Shift+<key>.
     *
     * Usage:
     * @code
     * auto cmd = xll::Command("MY.CMD")
     *          | xll::Procedure("MyCmd")
     *          | xll::ShortcutKey("M");
     * XLL_REGISTER(cmd);
     * @endcode
     */
    inline auto ShortcutKey(const xll::String& key)
    {
        return [key](Command&& lhs) { return lhs.ShortcutKey(key); };
    }

}    // namespace xll

