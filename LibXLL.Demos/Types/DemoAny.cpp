// DemoAny.cpp
// Demonstrates xll::Any — a type-erased container for any Excel value — and
// xll::cast<T>() with both the OptionalPolicy (default) and ExpectedPolicy.

#include <Types/Any.hpp>
#include <Types/Array.hpp>
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
// (bare xll::Any uses the default OptionalPolicy)
// ---------------------------------------------------------------------------

void demo_construction()
{
    separator("1. Construction and type queries");

    subsection("Default construction — holds Nil");
    {
        xll::Any any;
        std::cout << "  any.type()      = " << xltype_name(any.type()) << "\n";
        std::cout << "  any.empty()     = " << std::boolalpha << any.empty() << "\n";
        std::cout << "  holds<Nil>()    = " << any.holds<xll::Nil>() << "\n";
    }

    subsection("Construct from xll::Number");
    {
        xll::Any any { xll::Number(3.14) };
        std::cout << "  any.type()      = " << xltype_name(any.type()) << "\n";
        std::cout << "  holds<Number>() = " << any.holds<xll::Number>() << "\n";
        std::cout << "  holds<String>() = " << any.holds<xll::String>() << "\n";
        std::cout << "  empty()         = " << any.empty() << "\n";
    }

    subsection("Construct from xll::String (deep copy)");
    {
        xll::String s { "hello, Excel" };
        xll::Any any { s };
        std::cout << "  any.type()       = " << xltype_name(any.type()) << "\n";
        std::cout << "  holds<String>()  = " << any.holds<xll::String>() << "\n";
        // Buffers must be different allocations — Any made a deep copy
        std::cout << "  Deep copy?       = " << (any.val.str != s.val.str ? "yes" : "no") << "\n";
    }

    subsection("Construct from every supported xll type");
    {
        xll::Any num     { xll::Number(1.0)    };
        xll::Any str     { xll::String("hi")   };
        xll::Any i       { xll::Int(42)         };
        xll::Any b       { xll::Bool(true)      };
        xll::Any err     { xll::ErrDiv0         };
        xll::Any missing { xll::Missing{}       };
        xll::Any nil     { xll::Nil{}           };

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
// Section 2: xll::cast — always returns Optional<TTarget>
// ---------------------------------------------------------------------------

void demo_cast()
{
    separator("2. xll::cast  — returns Optional<TTarget>");

    subsection("Successful cast: Number -> Optional<Number>");
    {
        xll::Any any { xll::Number(2.71828) };
        auto result = xll::cast<xll::Number>(any);   // returns Optional<Number>

        if (result.has_value())
            std::cout << "  Value = " << static_cast<double>(result.value()) << "\n";
    }

    subsection("Failed cast: Number -> Optional<String>  (yields None)");
    {
        xll::Any any { xll::Number(42.0) };
        auto result = xll::cast<xll::String>(any);

        std::cout << "  has_value() = " << result.has_value() << "\n";
        std::cout << "  == None?    = " << (result == xll::None) << "\n";
    }

    subsection("Successful cast: String -> Optional<String>");
    {
        xll::Any any { xll::String("world") };
        auto result = xll::cast<xll::String>(any);

        if (result.has_value())
            std::cout << "  Value = " << std::string(result.value()) << "\n";
    }

    subsection("Successful cast: Bool -> Optional<Bool>");
    {
        xll::Any any { xll::Bool(true) };
        auto result = xll::cast<xll::Bool>(any);

        if (result.has_value())
            std::cout << "  Value = " << static_cast<bool>(result.value()) << "\n";
    }

    subsection("Successful cast: Error -> Optional<Error> (preserves error code)");
    {
        const xll::Error errors[] = {
            xll::ErrNull, xll::ErrDiv0, xll::ErrValue,
            xll::ErrRef,  xll::ErrName, xll::ErrNum, xll::ErrNA
        };
        for (auto& e : errors) {
            xll::Any any { e };
            auto result = xll::cast<xll::Error>(any);
            std::cout << "  " << std::string(e.to_string())
                      << "  ->  "
                      << (result.has_value() ? std::string(result.value().to_string()) : "FAILED")
                      << "\n";
        }
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
// Section 3: Monadic chaining after cast
// ---------------------------------------------------------------------------

void demo_monadic_chaining()
{
    separator("3. Monadic chaining after cast");

    subsection("transform chain: square then negate");
    {
        xll::Any any { xll::Number(4.0) };

        auto result = xll::cast<xll::Number>(any)
            .transform([](xll::Number n) { return xll::Number(n.val.num * n.val.num); })
            .transform([](xll::Number n) { return xll::Number(-n.val.num); });

        std::cout << "  -(4^2) = " << static_cast<double>(result.value()) << "\n";
    }

    subsection("or_else — provide fallback for wrong type");
    {
        xll::Any any { xll::String("nope") };

        auto result = xll::cast<xll::Number>(any)
            .or_else([]() -> xll::Optional<xll::Number> {
                return xll::Number(99.0);
            });

        std::cout << "  Fallback value = " << static_cast<double>(result.value()) << "\n";
    }

    subsection("None propagates through transform — callable not invoked");
    {
        xll::Any any { xll::String("not a number") };
        int calls = 0;

        auto result = xll::cast<xll::Number>(any)
            .transform([&](xll::Number n) { ++calls; return n; });

        std::cout << "  has_value() = " << result.has_value() << "\n";
        std::cout << "  calls       = " << calls             << "  (should be 0)\n";
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
        xll::Any original { xll::String("deep copy test") };
        xll::Any copy     { original };

        std::cout << "  original type = " << xltype_name(original.type()) << "\n";
        std::cout << "  copy type     = " << xltype_name(copy.type())     << "\n";
        std::cout << "  Same buffer?  = " << (original.val.str == copy.val.str ? "yes (BUG)" : "no (correct)") << "\n";
    }

    subsection("Move constructor — transfers ownership");
    {
        xll::Any source { xll::Number(1.618) };
        xll::Any dest   { std::move(source) };

        std::cout << "  dest type     = " << xltype_name(dest.type()) << "\n";
        // Any cast returns Optional<Number>
        auto r = xll::cast<xll::Number>(dest);
        std::cout << "  dest value    = " << static_cast<double>(r.value()) << "\n";
    }

    subsection("Assignment — changing the stored type");
    {
        xll::Any any { xll::Number(1.0) };
        std::cout << "  Before: " << xltype_name(any.type()) << "\n";

        any = xll::String("reassigned");
        std::cout << "  After:  " << xltype_name(any.type()) << "\n";

        // Returns Optional<String>
        auto r = xll::cast<xll::String>(any);
        if (r.has_value())
            std::cout << "  Value:  " << std::string(r.value()) << "\n";
    }

    subsection("cast result is a copy — mutating it does not affect Any");
    {
        xll::Any any { xll::Number(1.0) };

        // Returns Optional<Number>
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

    xll::Any a { xll::Number(1.0) };
    xll::Any b { xll::String("swap me") };

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
// xll::cast returns Optional<T>.
std::string format_cell(const xll::Any& cell)
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

    const std::vector<xll::Any> cells = {
        xll::Any{ xll::Number(3.14)              },
        xll::Any{ xll::String("Hello, World!")   },
        xll::Any{ xll::Int(42)                   },
        xll::Any{ xll::Bool(false)               },
        xll::Any{ xll::ErrNA                     },
        xll::Any{ xll::Missing{}                 },
        xll::Any{ xll::Nil{}                     },
    };

    for (std::size_t i = 0; i < cells.size(); ++i)
        std::cout << "  cell[" << i << "] -> " << format_cell(cells[i]) << "\n";
}

// ---------------------------------------------------------------------------
// Section 8: AnyOptional convenience alias
// ---------------------------------------------------------------------------

void demo_aliases()
{
    separator("8. AnyOptional convenience alias");

    subsection("AnyOptional is an alias for Any — cast returns Optional<T>");
    {
        xll::AnyOptional a { xll::Number(6.28) };
        auto r = xll::cast<xll::Number>(a);
        std::cout << "  Value = " << static_cast<double>(r.value()) << "\n";
    }
}

// ---------------------------------------------------------------------------
// Section 9: xll::Any holding an xll::Array
// ---------------------------------------------------------------------------

void demo_array()
{
    separator("9. xll::Any holding an xll::Array");

    // ------------------------------------------------------------------
    // 9a. Store a homogeneous Array<Number> inside an Any<>
    // ------------------------------------------------------------------
    subsection("9a. Store Array<Number> in Any<> and cast it back");
    {
        // Build a 1×5 horizontal array of numbers.
        xll::Array<xll::Number> src { {1.0, 2.0, 3.0, 4.0, 5.0},
                                      xll::Array<xll::Number>::Horizontal{} };

        // Any<> can accept any xll type — Array<Number> satisfies is_xll_type.
        xll::Any any { src };

        std::cout << "  any.type()                        = " << xltype_name(any.type()) << "\n";
        std::cout << "  any.holds<xll::Array>()           = "
                  << any.holds<xll::Array>() << "\n";
        std::cout << "  any.holds<xll::Array<Number>>()   = "
                  << any.holds<xll::Array<xll::Number>>() << "\n";
        std::cout << "  any.holds<xll::Array<xll::String>>() = "
                  << any.holds<xll::Array<xll::String>>() << "\n";

        // Cast back.  Any<> uses OptionalPolicy, so result is Optional<Array<Number>>.
        auto result = xll::cast<xll::Array<xll::Number>>(any);
        if (result.has_value()) {
            const auto& arr = result.value();
            std::cout << "  rows=" << arr.rows() << "  cols=" << arr.cols() << "\n";
            std::cout << "  elements:";
            for (const auto& v : arr)
                std::cout << " " << static_cast<double>(v);
            std::cout << "\n";
        }
    }

    // ------------------------------------------------------------------
    // 9b. Store a 2-D Array<Number> inside an Any<> (deep copy)
    // ------------------------------------------------------------------
    subsection("9b. 2-D Array<Number> round-trip through Any<> (deep copy)");
    {
        // 2×3 matrix: { 1 2 3 / 4 5 6 }
        xll::Array<xll::Number> mat {
            {1.0, 2.0, 3.0, 4.0, 5.0, 6.0},
            xll::Array<xll::Number>::TwoDimensional{2, 3}
        };

        xll::Any any { mat };   // deep copy of the element buffer

        // Verify the element buffers are different allocations.
        std::cout << "  Same buffer? = "
                  << (any.val.array.lparray == mat.val.array.lparray
                      ? "yes (BUG)" : "no (correct)") << "\n";

        auto result = xll::cast<xll::Array<xll::Number>>(any);
        if (result.has_value()) {
            const auto& arr = result.value();
            std::cout << "  shape: " << arr.rows() << "×" << arr.cols() << "\n";
            for (size_t r = 0; r < arr.rows(); ++r) {
                std::cout << "  row " << r << ":";
                for (size_t c = 0; c < arr.cols(); ++c)
                    std::cout << " " << static_cast<double>(arr[r, c]);
                std::cout << "\n";
            }
        }
    }

    // ------------------------------------------------------------------
    // 9c. Store a heterogeneous Array<Any> inside an Any<>
    // ------------------------------------------------------------------
    subsection("9c. Heterogeneous Array<Any> stored in Any<>");
    {
        // Build an array whose elements are themselves Any objects of mixed types.
        xll::Array<xll::Any> hetero {
            {
                xll::Number(3.14),
                xll::String("hello"),
                xll::Bool(true),
                xll::Int(42),
                xll::ErrDiv0,
            },
            xll::Array<xll::Any>::Horizontal{}
        };

        xll::Any any { hetero };

        std::cout << "  any.type() = " << xltype_name(any.type()) << "\n";

        auto result = xll::cast<xll::Array<xll::Any>>(any);
        if (result.has_value()) {
            const auto& arr = result.value();
            std::cout << "  size = " << arr.size() << "\n";
            for (size_t i = 0; i < arr.size(); ++i) {
                std::cout << "  [" << i << "] " << xltype_name(arr[i].type()) << "\n";
            }
        }
    }

    // ------------------------------------------------------------------
    // 9d. Failed cast — Any holds Array<Number>, try to cast to Number
    // ------------------------------------------------------------------
    subsection("9d. Failed cast: Array<Number> stored in Any<> -> Optional<Number>");
    {
        xll::Any any { xll::Array<xll::Number>{ {7.0, 8.0, 9.0} } };

        auto result = xll::cast<xll::Number>(any);
        std::cout << "  has_value() = " << result.has_value() << "\n";
        std::cout << "  == None?    = " << (result == xll::None) << "\n";
    }

    // ------------------------------------------------------------------
    // 9e. Move an Array into Any<> — no heap allocation
    // ------------------------------------------------------------------
    subsection("9e. Move Array<Number> into Any<>");
    {
        xll::Array<xll::Number> src { {10.0, 20.0, 30.0} };
        const auto* old_ptr = src.val.array.lparray;

        xll::Any any { std::move(src) };

        // After the move the buffer pointer inside Any should be the same
        // allocation that src used to own.
        std::cout << "  Buffer transferred? = "
                  << (any.val.array.lparray == old_ptr ? "yes" : "no") << "\n";

        auto result = xll::cast<xll::Array<xll::Number>>(any);
        if (result.has_value()) {
            std::cout << "  elements:";
            for (const auto& v : result.value())
                std::cout << " " << static_cast<double>(v);
            std::cout << "\n";
        }
    }
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "xll::Any — Feature Demo\n";

    demo_construction();
    demo_cast();
    demo_monadic_chaining();
    demo_value_semantics();
    demo_swap();
    demo_heterogeneous_dispatch();
    demo_aliases();
    demo_array();

    std::cout << "\n";
    std::cout << "========================================================\n";
    std::cout << "  Demo complete.\n";
    std::cout << "========================================================\n";

    return 0;
}


