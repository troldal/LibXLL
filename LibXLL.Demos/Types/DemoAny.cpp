// DemoAny.cpp
// Demonstrates xll::Any — a type-erased container for any Excel value — and
// xll::cast<T>() with both the ExpectedPolicy (default) and OptionalPolicy.

#include <Types/Any.hpp>
#include <Types/Bool.hpp>
#include <Types/Error.hpp>
#include <Types/Int.hpp>
#include <Types/Missing.hpp>
#include <Types/Nil.hpp>
#include <Types/Number.hpp>
#include <Types/String.hpp>

#include <iostream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void separator(const char* title)
{
    std::cout << "\n";
    std::cout << "========================================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "========================================================\n";
}

static void subsection(const char* title)
{
    std::cout << "\n--- " << title << " ---\n";
}

// Returns a human-readable name for an xltype constant.
static const char* xltype_name(int t)
{
    switch (t) {
        case xltypeNum:     return "xltypeNum     (Number)";
        case xltypeStr:     return "xltypeStr     (String)";
        case xltypeInt:     return "xltypeInt     (Int)";
        case xltypeBool:    return "xltypeBool    (Bool)";
        case xltypeErr:     return "xltypeErr     (Error)";
        case xltypeNil:     return "xltypeNil     (Nil)";
        case xltypeMissing: return "xltypeMissing (Missing)";
        case xltypeMulti:   return "xltypeMulti   (Array)";
        default:            return "(unknown)";
    }
}

// ---------------------------------------------------------------------------
// Section 1: Construction and type queries
// ---------------------------------------------------------------------------

void demo_construction()
{
    separator("1. Construction and type queries");

    subsection("Default construction — holds Nil");
    {
        xll::Any<> any;
        std::cout << "  any.type()      = " << xltype_name(any.type()) << "\n";
        std::cout << "  any.empty()     = " << std::boolalpha << any.empty() << "\n";
        std::cout << "  holds<Nil>()    = " << any.holds<xll::Nil>() << "\n";
    }

    subsection("Construct from xll::Number");
    {
        xll::Any<> any { xll::Number(3.14) };
        std::cout << "  any.type()      = " << xltype_name(any.type()) << "\n";
        std::cout << "  holds<Number>() = " << any.holds<xll::Number>() << "\n";
        std::cout << "  holds<String>() = " << any.holds<xll::String>() << "\n";
        std::cout << "  empty()         = " << any.empty() << "\n";
    }

    subsection("Construct from xll::String (deep copy)");
    {
        xll::String s { "hello, Excel" };
        xll::Any<> any { s };
        std::cout << "  any.type()       = " << xltype_name(any.type()) << "\n";
        std::cout << "  holds<String>()  = " << any.holds<xll::String>() << "\n";
        // Buffers must be different allocations — Any made a deep copy
        std::cout << "  Deep copy?       = " << (any.val.str != s.val.str ? "yes" : "no") << "\n";
    }

    subsection("Construct from every supported xll type");
    {
        xll::Any<> num     { xll::Number(1.0)    };
        xll::Any<> str     { xll::String("hi")   };
        xll::Any<> i       { xll::Int(42)         };
        xll::Any<> b       { xll::Bool(true)      };
        xll::Any<> err     { xll::ErrDiv0         };
        xll::Any<> missing { xll::Missing{}       };
        xll::Any<> nil     { xll::Nil{}           };

        std::cout << "  Number  -> " << xltype_name(num.type())     << "\n";
        std::cout << "  String  -> " << xltype_name(str.type())     << "\n";
        std::cout << "  Int     -> " << xltype_name(i.type())       << "\n";
        std::cout << "  Bool    -> " << xltype_name(b.type())       << "\n";
        std::cout << "  Error   -> " << xltype_name(err.type())     << "\n";
        std::cout << "  Missing -> " << xltype_name(missing.type()) << "\n";
        std::cout << "  Nil     -> " << xltype_name(nil.type())     << "\n";
    }
}

// ---------------------------------------------------------------------------
// Section 2: xll::cast with ExpectedPolicy (default)
// ---------------------------------------------------------------------------

void demo_cast_expected()
{
    separator("2. xll::cast  — ExpectedPolicy (default)");

    subsection("Successful cast: Number -> Number");
    {
        xll::Any<> any { xll::Number(2.71828) };
        auto result = xll::cast<xll::Number>(any);

        if (result.has_value())
            std::cout << "  Value = " << static_cast<double>(result.value()) << "\n";
    }

    subsection("Failed cast: Number -> String  (yields ErrValue)");
    {
        xll::Any<> any { xll::Number(42.0) };
        auto result = xll::cast<xll::String>(any);

        if (!result.has_value())
            std::cout << "  Error = " << std::string(result.error().to_string()) << "\n";
    }

    subsection("Successful cast: String -> String");
    {
        xll::Any<> any { xll::String("world") };
        auto result = xll::cast<xll::String>(any);

        if (result.has_value())
            std::cout << "  Value = " << std::string(result.value()) << "\n";
    }

    subsection("Successful cast: Bool -> Bool");
    {
        xll::Any<> any { xll::Bool(true) };
        auto result = xll::cast<xll::Bool>(any);

        if (result.has_value())
            std::cout << "  Value = " << static_cast<bool>(result.value()) << "\n";
    }

    subsection("Successful cast: Error -> Error (preserves error code)");
    {
        const xll::Error errors[] = {
            xll::ErrNull, xll::ErrDiv0, xll::ErrValue,
            xll::ErrRef,  xll::ErrName, xll::ErrNum, xll::ErrNA
        };
        for (auto& e : errors) {
            xll::Any<> any { e };
            auto result = xll::cast<xll::Error>(any);
            std::cout << "  " << std::string(e.to_string())
                      << "  ->  "
                      << (result.has_value() ? std::string(result.value().to_string()) : "FAILED")
                      << "\n";
        }
    }
}

// ---------------------------------------------------------------------------
// Section 3: xll::cast with OptionalPolicy
// ---------------------------------------------------------------------------

void demo_cast_optional()
{
    separator("3. xll::cast  — OptionalPolicy");

    subsection("Successful cast: Number -> Optional<Number>");
    {
        xll::Any<xll::OptionalPolicy> any { xll::Number(9.81) };
        auto result = xll::cast<xll::Number>(any);   // returns Optional<Number>

        if (result.has_value())
            std::cout << "  Value = " << static_cast<double>(result.value()) << "\n";
    }

    subsection("Failed cast: Number -> Optional<String>  (yields None)");
    {
        xll::Any<xll::OptionalPolicy> any { xll::Number(1.0) };
        auto result = xll::cast<xll::String>(any);

        std::cout << "  has_value() = " << result.has_value() << "\n";
        std::cout << "  == None?    = " << (result == xll::None) << "\n";
    }

    subsection("Using AnyOptional convenience alias");
    {
        xll::AnyOptional any { xll::String("alias demo") };
        auto result = xll::cast<xll::String>(any);

        if (result.has_value())
            std::cout << "  Value = " << std::string(result.value()) << "\n";
    }
}

// ---------------------------------------------------------------------------
// Section 4: Monadic chaining after cast
// ---------------------------------------------------------------------------

void demo_monadic_chaining()
{
    separator("4. Monadic chaining after cast");

    subsection("ExpectedPolicy: transform on successful cast");
    {
        xll::Any<> any { xll::Number(4.0) };

        // cast to Number, then square it, then negate it
        auto result = xll::cast<xll::Number>(any)
            .transform([](xll::Number n) { return xll::Number(n.val.num * n.val.num); })
            .transform([](xll::Number n) { return xll::Number(-n.val.num); });

        std::cout << "  -(4^2) = " << static_cast<double>(result.value()) << "\n";
    }

    subsection("ExpectedPolicy: and_then — chain that can fail");
    {
        auto safe_sqrt = [](xll::Number n) -> xll::Expected<xll::Number, xll::Error> {
            if (n.val.num < 0.0) return xll::Unexpected(xll::ErrNum);
            return xll::Number(std::sqrt(n.val.num));
        };

        xll::Any<> good { xll::Number(16.0) };
        auto r1 = xll::cast<xll::Number>(good).and_then(safe_sqrt);
        std::cout << "  sqrt(16) = " << static_cast<double>(r1.value()) << "\n";

        xll::Any<> bad { xll::Number(-1.0) };
        auto r2 = xll::cast<xll::Number>(bad).and_then(safe_sqrt);
        std::cout << "  sqrt(-1) = " << std::string(r2.error().to_string()) << "\n";
    }

    subsection("ExpectedPolicy: or_else — recover from wrong type");
    {
        xll::Any<> any { xll::String("not a number") };

        auto result = xll::cast<xll::Number>(any)
            .or_else([](xll::Error) -> xll::Expected<xll::Number, xll::Error> {
                return xll::Number(0.0);   // fallback default
            });

        std::cout << "  Fallback value = " << static_cast<double>(result.value()) << "\n";
    }

    subsection("OptionalPolicy: transform chain");
    {
        xll::AnyOptional any { xll::Number(3.0) };

        auto result = xll::cast<xll::Number>(any)
            .transform([](xll::Number n) { return xll::Number(n.val.num * 2.0); })
            .transform([](xll::Number n) { return xll::Number(n.val.num + 1.0); });

        std::cout << "  3*2+1 = " << static_cast<double>(result.value()) << "\n";
    }

    subsection("OptionalPolicy: or_else — provide fallback for wrong type");
    {
        xll::AnyOptional any { xll::String("nope") };

        auto result = xll::cast<xll::Number>(any)
            .or_else([]() -> xll::Optional<xll::Number> {
                return xll::Number(99.0);
            });

        std::cout << "  Fallback value = " << static_cast<double>(result.value()) << "\n";
    }
}

// ---------------------------------------------------------------------------
// Section 5: Copy/move and value semantics
// ---------------------------------------------------------------------------

void demo_value_semantics()
{
    separator("5. Copy / move and value semantics");

    subsection("Copy constructor — String buffer is deep-copied");
    {
        xll::Any<> original { xll::String("deep copy test") };
        xll::Any<> copy     { original };

        std::cout << "  original type = " << xltype_name(original.type()) << "\n";
        std::cout << "  copy type     = " << xltype_name(copy.type())     << "\n";
        std::cout << "  Same buffer?  = " << (original.val.str == copy.val.str ? "yes (BUG)" : "no (correct)") << "\n";
    }

    subsection("Move constructor — transfers ownership");
    {
        xll::Any<> source { xll::Number(1.618) };
        xll::Any<> dest   { std::move(source) };

        std::cout << "  dest type     = " << xltype_name(dest.type()) << "\n";
        auto r = xll::cast<xll::Number>(dest);
        std::cout << "  dest value    = " << static_cast<double>(r.value()) << "\n";
    }

    subsection("Assignment — changing the stored type");
    {
        xll::Any<> any { xll::Number(1.0) };
        std::cout << "  Before: " << xltype_name(any.type()) << "\n";

        any = xll::String("reassigned");
        std::cout << "  After:  " << xltype_name(any.type()) << "\n";

        auto r = xll::cast<xll::String>(any);
        if (r.has_value())
            std::cout << "  Value:  " << std::string(r.value()) << "\n";
    }

    subsection("cast result is a copy — mutating it does not affect Any");
    {
        xll::Any<> any { xll::Number(1.0) };

        auto r = xll::cast<xll::Number>(any);
        r.value() = xll::Number(999.0);   // mutate the copy

        auto r2 = xll::cast<xll::Number>(any);
        std::cout << "  Original still = " << static_cast<double>(r2.value()) << "\n";
    }
}

// ---------------------------------------------------------------------------
// Section 6: swap
// ---------------------------------------------------------------------------

void demo_swap()
{
    separator("6. swap");

    xll::Any<> a { xll::Number(1.0) };
    xll::Any<> b { xll::String("swap me") };

    std::cout << "  Before swap:\n";
    std::cout << "    a = " << xltype_name(a.type()) << "\n";
    std::cout << "    b = " << xltype_name(b.type()) << "\n";

    a.swap(b);

    std::cout << "  After swap:\n";
    std::cout << "    a = " << xltype_name(a.type()) << "\n";
    std::cout << "    b = " << xltype_name(b.type()) << "\n";
}

// ---------------------------------------------------------------------------
// Section 7: Realistic use-case — heterogeneous Excel cell processor
// ---------------------------------------------------------------------------

// Simulates a function that receives an arbitrary Excel value and produces a
// formatted string, just as an XLL function might process a cell argument.
std::string format_cell(const xll::Any<>& cell)
{
    if (auto n = xll::cast<xll::Number>(cell); n.has_value())
        return "Number: " + std::to_string(n.value().val.num);

    if (auto s = xll::cast<xll::String>(cell); s.has_value())
        return "String: " + std::string(s.value());

    if (auto i = xll::cast<xll::Int>(cell); i.has_value())
        return "Int: " + std::to_string(static_cast<int>(i.value()));

    if (auto b = xll::cast<xll::Bool>(cell); b.has_value())
        return std::string("Bool: ") + (static_cast<bool>(b.value()) ? "TRUE" : "FALSE");

    if (auto e = xll::cast<xll::Error>(cell); e.has_value())
        return "Error: " + std::string(e.value().to_string());

    if (cell.holds<xll::Missing>())
        return "(missing argument)";

    return "(empty/nil)";
}

void demo_heterogeneous_dispatch()
{
    separator("7. Heterogeneous dispatch — simulated XLL cell processing");

    const std::vector<xll::Any<>> cells = {
        xll::Any<>{ xll::Number(3.14)              },
        xll::Any<>{ xll::String("Hello, World!")   },
        xll::Any<>{ xll::Int(42)                   },
        xll::Any<>{ xll::Bool(false)               },
        xll::Any<>{ xll::ErrNA                     },
        xll::Any<>{ xll::Missing{}                 },
        xll::Any<>{ xll::Nil{}                     },
    };

    for (std::size_t i = 0; i < cells.size(); ++i)
        std::cout << "  cell[" << i << "] -> " << format_cell(cells[i]) << "\n";
}

// ---------------------------------------------------------------------------
// Section 8: Using AnyExpected / AnyOptional convenience aliases
// ---------------------------------------------------------------------------

void demo_aliases()
{
    separator("8. AnyExpected and AnyOptional convenience aliases");

    subsection("AnyExpected (same as Any<ExpectedPolicy>)");
    {
        xll::AnyExpected a { xll::Number(6.28) };
        auto r = xll::cast<xll::Number>(a);
        // r is Expected<Number, Error>
        std::cout << "  Value = " << static_cast<double>(r.value()) << "\n";
    }

    subsection("AnyOptional (same as Any<OptionalPolicy>)");
    {
        xll::AnyOptional a { xll::String("optional demo") };
        auto r = xll::cast<xll::String>(a);
        // r is Optional<String>
        if (r.has_value())
            std::cout << "  Value = " << std::string(r.value()) << "\n";
    }

    subsection("Switching policy — same stored value, different cast return type");
    {
        // Store the same value in both policy variants
        xll::AnyExpected exp_any { xll::Int(7) };
        xll::AnyOptional opt_any { xll::Int(7) };

        auto r_exp = xll::cast<xll::Int>(exp_any);  // Expected<Int, Error>
        auto r_opt = xll::cast<xll::Int>(opt_any);  // Optional<Int>

        std::cout << "  Expected result: " << static_cast<int>(r_exp.value()) << "\n";
        std::cout << "  Optional result: " << static_cast<int>(r_opt.value()) << "\n";
    }
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "xll::Any — Feature Demo\n";

    demo_construction();
    demo_cast_expected();
    demo_cast_optional();
    demo_monadic_chaining();
    demo_value_semantics();
    demo_swap();
    demo_heterogeneous_dispatch();
    demo_aliases();

    std::cout << "\n";
    std::cout << "========================================================\n";
    std::cout << "  Demo complete.\n";
    std::cout << "========================================================\n";

    return 0;
}


