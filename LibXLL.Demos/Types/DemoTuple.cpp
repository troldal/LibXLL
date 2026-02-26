// DemoTuple.cpp
// Demonstrates xll::Tuple — a heterogeneous, Excel-compatible tuple type
// backed by an xltypeMulti XLOPER12 array.

#include <Types/Bool.hpp>
#include <Types/Error.hpp>
#include <Types/Int.hpp>
#include <Types/Missing.hpp>
#include <Types/Nil.hpp>
#include <Types/Number.hpp>
#include <Types/String.hpp>
#include <Types/Tuple.hpp>

#include <cmath>
#include <iostream>
#include <string>

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

template<typename T>
static std::string opt_str(const xll::Optional<T>& o)
{
    if (!o.has_value()) return "(None)";
    if constexpr (std::is_same_v<T, xll::Number>)
        return std::to_string(static_cast<double>(o.value()));
    else if constexpr (std::is_same_v<T, xll::Int>)
        return std::to_string(static_cast<int>(o.value()));
    else if constexpr (std::is_same_v<T, xll::Bool>)
        return static_cast<bool>(o.value()) ? "true" : "false";
    else if constexpr (std::is_same_v<T, xll::String>)
        return std::string(o.value());
    else if constexpr (std::is_same_v<T, xll::Error>)
        return std::string(o.value().to_string());
    else
        return "(value)";
}

// ---------------------------------------------------------------------------
// Section 1: Type alias and basic construction
// ---------------------------------------------------------------------------

void demo_construction()
{
    separator("1. Type alias and construction");

    subsection("Define a tuple type via alias — mirrors std::tuple");
    {
        using PersonRow = xll::Tuple<xll::String, xll::Number, xll::Bool>;

        PersonRow row { xll::String("Alice"), xll::Number(30.0), xll::Bool(true) };

        std::cout << "  PersonRow { String, Number, Bool }\n";
        std::cout << "  xltype() == xltypeMulti: "
                  << (row.xltype() == xltypeMulti ? "yes" : "no") << "\n";
        std::cout << "  sizeof(PersonRow) == sizeof(XLOPER12): "
                  << (sizeof(PersonRow) == sizeof(XLOPER12) ? "yes" : "no") << "\n";
    }

    subsection("Default construction — all elements are Nil");
    {
        using T = xll::Tuple<xll::Number, xll::String, xll::Bool>;
        T t;

        std::cout << "  get<0> (Number): " << opt_str(xll::get<0>(t)) << "  (expected None)\n";
        std::cout << "  get<1> (String): " << opt_str(xll::get<1>(t)) << "  (expected None)\n";
        std::cout << "  get<2> (Bool):   " << opt_str(xll::get<2>(t)) << "  (expected None)\n";
    }

    subsection("Initializer-list construction — wrong size throws");
    {
        using T = xll::Tuple<xll::Number, xll::String>;
        try {
            T bad { xll::Number(1.0) };   // only 1 element for a 2-element tuple
            std::cout << "  ERROR: no exception thrown\n";
        }
        catch (const std::out_of_range& e) {
            std::cout << "  Caught expected exception: " << e.what() << "\n";
        }
    }
}

// ---------------------------------------------------------------------------
// Section 2: get<I> — index-based access
// ---------------------------------------------------------------------------

void demo_get_by_index()
{
    separator("2. get<I>(tuple) — index-based access");

    using Row = xll::Tuple<xll::String, xll::Number, xll::Bool, xll::Int, xll::Error>;
    Row row {
        xll::String("hello"),
        xll::Number(3.14),
        xll::Bool(true),
        xll::Int(42),
        xll::ErrDiv0
    };

    subsection("Correct types at each index");
    std::cout << "  [0] String = " << opt_str(xll::get<0>(row)) << "\n";
    std::cout << "  [1] Number = " << opt_str(xll::get<1>(row)) << "\n";
    std::cout << "  [2] Bool   = " << opt_str(xll::get<2>(row)) << "\n";
    std::cout << "  [3] Int    = " << opt_str(xll::get<3>(row)) << "\n";
    std::cout << "  [4] Error  = " << opt_str(xll::get<4>(row)) << "\n";

    subsection("Runtime type mismatch → None");
    {
        // Provide a String where Number is declared at index 0.
        using T = xll::Tuple<xll::Number, xll::String>;
        T t { xll::String("oops"), xll::String("ok") };

        std::cout << "  get<0> declared Number, holds String: "
                  << opt_str(xll::get<0>(t)) << "  (expected None)\n";
    }

    subsection("Return type is always Optional<declared-type>");
    {
        using T = xll::Tuple<xll::Number, xll::String>;
        T t { xll::Number(1.0), xll::String("x") };

        // Demonstrate with auto — the type deduced is Optional<Number> / Optional<String>
        auto n = xll::get<0>(t);
        auto s = xll::get<1>(t);
        std::cout << "  get<0> has_value: " << n.has_value() << "\n";
        std::cout << "  get<1> has_value: " << s.has_value() << "\n";
    }
}

// ---------------------------------------------------------------------------
// Section 3: get<T> — type-based access
// ---------------------------------------------------------------------------

void demo_get_by_type()
{
    separator("3. get<T>(tuple) — type-based access");

    subsection("Retrieve by type");
    {
        using Row = xll::Tuple<xll::String, xll::Number, xll::Bool>;
        Row row { xll::String("world"), xll::Number(2.71), xll::Bool(false) };

        std::cout << "  get<String> = " << opt_str(xll::get<xll::String>(row)) << "\n";
        std::cout << "  get<Number> = " << opt_str(xll::get<xll::Number>(row)) << "\n";
        std::cout << "  get<Bool>   = " << opt_str(xll::get<xll::Bool>(row))   << "\n";
    }

    subsection("Type must appear exactly once — duplicate is a compile error");
    {
        // This is enforced at compile time via requires (count_type<T,Ts...> == 1).
        // Attempting xll::get<xll::Number> on Tuple<Number,Number> would not compile.
        std::cout << "  (duplicate-type get is a compile-time error — shown by comment)\n";
    }

    subsection("Type not present → compile error, not runtime None");
    {
        // xll::get<xll::Error> on Tuple<Number,String> would not compile.
        std::cout << "  (absent-type get is a compile-time error — shown by comment)\n";
    }
}

// ---------------------------------------------------------------------------
// Section 4: Monadic chaining on get results
// ---------------------------------------------------------------------------

void demo_monadic()
{
    separator("4. Monadic operations on get results");

    using Measurement = xll::Tuple<xll::String, xll::Number>;

    subsection("transform — square the number if present");
    {
        Measurement m { xll::String("area"), xll::Number(5.0) };
        auto result = xll::get<xll::Number>(m)
            .transform([](const xll::Number& n) { return xll::Number(n.val.num * n.val.num); });
        std::cout << "  5^2 = " << opt_str(result) << "  (expected 25)\n";
    }

    subsection("or_else — fallback when element is Nil (default-constructed)");
    {
        Measurement m;   // all Nil
        auto result = xll::get<xll::Number>(m)
            .or_else([]() -> xll::Optional<xll::Number> { return xll::Number(-1.0); });
        std::cout << "  fallback = " << opt_str(result) << "  (expected -1)\n";
    }

    subsection("and_then — safe square root");
    {
        auto safe_sqrt = [](const xll::Number& n) -> xll::Optional<xll::Number> {
            if (n.val.num < 0.0) return xll::None;
            return xll::Number(std::sqrt(n.val.num));
        };

        Measurement good { xll::String("radius"), xll::Number(9.0) };
        Measurement bad  { xll::String("negative"), xll::Number(-4.0) };

        std::cout << "  sqrt(9)  = " << opt_str(xll::get<xll::Number>(good).and_then(safe_sqrt))
                  << "  (expected 3)\n";
        std::cout << "  sqrt(-4) = " << opt_str(xll::get<xll::Number>(bad).and_then(safe_sqrt))
                  << "  (expected None)\n";
    }

    subsection("Chain: index → transform → or_else");
    {
        using T = xll::Tuple<xll::Number, xll::String>;
        T t { xll::Number(4.0), xll::String("label") };

        auto result = xll::get<0>(t)
            .transform([](const xll::Number& n) { return xll::Number(n.val.num * 2.0); })
            .or_else([]() -> xll::Optional<xll::Number> { return xll::Number(0.0); });
        std::cout << "  4 * 2 = " << opt_str(result) << "  (expected 8)\n";
    }
}

// ---------------------------------------------------------------------------
// Section 5: Copy and move semantics
// ---------------------------------------------------------------------------

void demo_value_semantics()
{
    separator("5. Copy and move semantics");

    using T = xll::Tuple<xll::String, xll::Number>;

    subsection("Copy — deep copy of String buffer");
    {
        T original { xll::String("original"), xll::Number(1.0) };
        T copy { original };

        std::cout << "  original String: " << opt_str(xll::get<0>(original)) << "\n";
        std::cout << "  copy     String: " << opt_str(xll::get<0>(copy))     << "\n";
        std::cout << "  buffers differ:  "
                  << (original.data() != copy.data() ? "yes" : "no") << "\n";
    }

    subsection("Move — buffer is transferred, not copied");
    {
        T src { xll::String("src"), xll::Number(2.0) };
        const void* old_ptr = src.data();
        T dst { std::move(src) };

        std::cout << "  dst String: " << opt_str(xll::get<0>(dst)) << "\n";
        std::cout << "  buffer transferred: " << (dst.data() == old_ptr ? "yes" : "no") << "\n";
    }
}

// ---------------------------------------------------------------------------
// Section 6: Realistic use-case — table row
// ---------------------------------------------------------------------------

void demo_table_row()
{
    separator("6. Realistic use-case — typed table rows");

    // Imagine a spreadsheet row: employee name, salary, active flag, dept code
    using EmployeeRow = xll::Tuple<xll::String, xll::Number, xll::Bool, xll::Int>;

    auto print_row = [](const EmployeeRow& row) {
        std::cout << "  Name:   " << opt_str(xll::get<xll::String>(row)) << "\n";
        std::cout << "  Salary: " << opt_str(xll::get<xll::Number>(row)) << "\n";
        std::cout << "  Active: " << opt_str(xll::get<xll::Bool>(row))   << "\n";
        std::cout << "  Dept:   " << opt_str(xll::get<xll::Int>(row))    << "\n";
    };

    subsection("Well-formed row");
    {
        EmployeeRow row {
            xll::String("Bob"),
            xll::Number(75000.0),
            xll::Bool(true),
            xll::Int(3)
        };
        print_row(row);
    }

    subsection("Partially wrong row — Salary is a String (bad Excel data)");
    {
        EmployeeRow row {
            xll::String("Carol"),
            xll::String("N/A"),   // declared Number, but Excel sent a String
            xll::Bool(true),
            xll::Int(5)
        };
        print_row(row);   // Salary will print (None)
    }

    subsection("Raise salary by 10% using monadic transform");
    {
        EmployeeRow row {
            xll::String("Dave"),
            xll::Number(50000.0),
            xll::Bool(true),
            xll::Int(2)
        };

        auto new_salary = xll::get<xll::Number>(row)
            .transform([](const xll::Number& n) { return xll::Number(n.val.num * 1.10); });

        std::cout << "  Old salary: " << opt_str(xll::get<xll::Number>(row)) << "\n";
        std::cout << "  New salary: " << opt_str(new_salary)                 << "\n";
    }
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "xll::Tuple — Feature Demo\n";

    demo_construction();
    demo_get_by_index();
    demo_get_by_type();
    demo_monadic();
    demo_value_semantics();
    demo_table_row();

    std::cout << "\n";
    std::cout << "========================================================\n";
    std::cout << "  Demo complete.\n";
    std::cout << "========================================================\n";

    return 0;
}

