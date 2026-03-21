// DemoVariant.cpp
// Demonstrates xll::Variant — a type-safe discriminated union over Excel value
// types — and xll::cast<Variant<...>>() for receiving mixed-type Any values.
//
// Sections
//   1. Construction
//   2. Type queries  — holds_alternative, index, is_valid, excel_type
//   3. Value access  — get, get_if
//   4. visit         — polymorphic dispatch with generic lambdas and xll::overload
//   5. Mutation      — operator=(U), emplace
//   6. swap
//   7. xll::cast<Variant<...>>(any)
//   8. Practical example — UDF-like function using cast + visit

#include <Types/Any.hpp>
#include <Types/Bool.hpp>
#include <Types/Error.hpp>
#include <Types/Int.hpp>
#include <Types/Nil.hpp>
#include <Types/Number.hpp>
#include <Types/String.hpp>
#include <Types/Variant.hpp>

#include <iostream>
#include <string>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void separator(const char* title)
{
    std::cout << "\n"
              << "========================================================\n"
              << "  " << title << "\n"
              << "========================================================\n";
}

static void subsection(const char* title)
{
    std::cout << "\n--- " << title << " ---\n";
}

// ---------------------------------------------------------------------------
// Section 1: Construction
// ---------------------------------------------------------------------------

void demo_construction()
{
    separator("1. Construction");

    // ------------------------------------------------------------------
    subsection("Default construction — first alternative (Number) is default-initialised");
    {
        xll::Variant<xll::Number, xll::String> v;
        std::cout << "  is_valid()         = " << std::boolalpha << v.is_valid() << "\n";
        std::cout << "  index()            = " << v.index() << "  (0 = Number)\n";
        std::cout << "  holds<Number>?     = " << xll::holds_alternative<xll::Number>(v) << "\n";
        std::cout << "  value              = " << xll::get<xll::Number>(v) << "\n";
    }

    // ------------------------------------------------------------------
    subsection("Converting construction from each alternative");
    {
        xll::Variant<xll::Number, xll::String, xll::Bool, xll::Int> vn(xll::Number(3.14));
        xll::Variant<xll::Number, xll::String, xll::Bool, xll::Int> vs(xll::String("hello"));
        xll::Variant<xll::Number, xll::String, xll::Bool, xll::Int> vb(xll::Bool(true));
        xll::Variant<xll::Number, xll::String, xll::Bool, xll::Int> vi(xll::Int(42));

        std::cout << "  Number  index=" << vn.index() << " value=" << xll::get<xll::Number>(vn) << "\n";
        std::cout << "  String  index=" << vs.index() << " value=\"" << xll::get<xll::String>(vs) << "\"\n";
        std::cout << "  Bool    index=" << vb.index() << " value=" << static_cast<bool>(xll::get<xll::Bool>(vb)) << "\n";
        std::cout << "  Int     index=" << vi.index() << " value=" << static_cast<int>(xll::get<xll::Int>(vi)) << "\n";
    }

    // ------------------------------------------------------------------
    subsection("Copy construction — String buffer is deep-copied");
    {
        xll::Variant<xll::Number, xll::String> a(xll::String("copy me"));
        xll::Variant<xll::Number, xll::String> b(a);

        std::cout << "  a = \"" << xll::get<xll::String>(a) << "\"\n";
        std::cout << "  b = \"" << xll::get<xll::String>(b) << "\"\n";
        std::cout << "  independent buffers? "
                  << (a.val.str != b.val.str ? "yes" : "no") << "\n";
    }

    // ------------------------------------------------------------------
    subsection("Move construction — String buffer is stolen");
    {
        xll::Variant<xll::Number, xll::String> a(xll::String("move me"));
        xll::Variant<xll::Number, xll::String> b(std::move(a));

        std::cout << "  b = \"" << xll::get<xll::String>(b) << "\"\n";
        std::cout << "  a.val.str == nullptr (moved-from)? "
                  << (a.val.str == nullptr ? "yes" : "no") << "\n";
    }
}

// ---------------------------------------------------------------------------
// Section 2: Type queries
// ---------------------------------------------------------------------------

void demo_type_queries()
{
    separator("2. Type queries — holds_alternative, index, is_valid, excel_type");

    using V = xll::Variant<xll::Number, xll::String, xll::Bool, xll::Int>;

    // ------------------------------------------------------------------
    subsection("holds_alternative<U>()");
    {
        V v(xll::String("test"));
        std::cout << "  holds<Number>? " << xll::holds_alternative<xll::Number>(v) << "\n";
        std::cout << "  holds<String>? " << xll::holds_alternative<xll::String>(v) << "\n";
        std::cout << "  holds<Bool>?   " << xll::holds_alternative<xll::Bool>(v)   << "\n";
        std::cout << "  holds<Int>?    " << xll::holds_alternative<xll::Int>(v)    << "\n";
    }

    // ------------------------------------------------------------------
    subsection("index() — zero-based position in the type list");
    {
        std::cout << "  Number -> " << V(xll::Number(0.0)).index()         << "\n";
        std::cout << "  String -> " << V(xll::String("")).index()          << "\n";
        std::cout << "  Bool   -> " << V(xll::Bool(false)).index()         << "\n";
        std::cout << "  Int    -> " << V(xll::Int(0)).index()              << "\n";
    }

    // ------------------------------------------------------------------
    subsection("excel_type — bitmask of all constituent xltypes");
    {
        // xltypeNum=0x0001, xltypeStr=0x0002, xltypeBool=0x0004, xltypeInt=0x0800
        std::cout << std::hex;
        std::cout << "  V::excel_type = 0x" << V::excel_type << "\n";
        std::cout << std::dec;
        std::cout << "  (OR of: xltypeNum=0x1, xltypeStr=0x2, xltypeBool=0x4, xltypeInt=0x800)\n";
    }

    // ------------------------------------------------------------------
    subsection("is_valid() — true when stored xltype is one of the declared alternatives");
    {
        V v(xll::Number(1.0));
        std::cout << "  is_valid() = " << v.is_valid() << "\n";
    }
}

// ---------------------------------------------------------------------------
// Section 3: Value access — get, get_if
// ---------------------------------------------------------------------------

void demo_value_access()
{
    separator("3. Value access — get<U> and get_if<U>");

    using V = xll::Variant<xll::Number, xll::String, xll::Bool>;

    // ------------------------------------------------------------------
    subsection("get<U>() — returns a reference, throws on type mismatch");
    {
        V v(xll::Number(2.718));

        xll::Number& n = xll::get<xll::Number>(v);          // mutable lvalue ref
        std::cout << "  value before  = " << n << "\n";

        n = 3.14;                                             // mutate in-place
        std::cout << "  value after   = " << xll::get<xll::Number>(v) << "\n";

        try {
            [[maybe_unused]] auto& _ = xll::get<xll::String>(v);
            std::cout << "  ERROR: should have thrown\n";
        }
        catch (const std::bad_variant_access&) {
            std::cout << "  get<String>(v) threw std::bad_variant_access (expected)\n";
        }
    }

    // ------------------------------------------------------------------
    subsection("get<U>(Variant&&) — rvalue overload steals the buffer");
    {
        V v(xll::String("move out"));
        xll::String s = xll::get<xll::String>(std::move(v));   // U&&

        std::cout << "  moved string        = \"" << s << "\"\n";
        std::cout << "  source val.str null = " << (v.val.str == nullptr ? "yes" : "no") << "\n";
    }

    // ------------------------------------------------------------------
    subsection("get_if<U>() — returns Optional<U>, never throws");
    {
        V v(xll::Bool(true));

        auto maybe_bool   = xll::get_if<xll::Bool>(v);    // Optional<Bool>  — engaged
        auto maybe_number = xll::get_if<xll::Number>(v);  // Optional<Number> — empty

        std::cout << "  get_if<Bool>   has_value = " << maybe_bool.has_value() << "\n";
        std::cout << "  get_if<Bool>   value     = "
                  << std::boolalpha << static_cast<bool>(maybe_bool.value()) << "\n";
        std::cout << "  get_if<Number> has_value = " << maybe_number.has_value() << "\n";
        std::cout << "  get_if<Number> == None   = " << (maybe_number == xll::None) << "\n";
    }

    // ------------------------------------------------------------------
    subsection("get_if on rvalue Variant — moves into the Optional");
    {
        V v(xll::String("move via get_if"));
        auto opt = xll::get_if<xll::String>(std::move(v));   // Optional<String> via move

        std::cout << "  optional value     = \"" << opt.value() << "\"\n";
        std::cout << "  source val.str null = " << (v.val.str == nullptr ? "yes" : "no") << "\n";
    }
}

// ---------------------------------------------------------------------------
// Section 4: visit
// ---------------------------------------------------------------------------

void demo_visit()
{
    separator("4. visit — polymorphic dispatch");

    using V = xll::Variant<xll::Number, xll::String, xll::Bool, xll::Error>;

    // ------------------------------------------------------------------
    subsection("Generic lambda — single auto&& handler for all alternatives");
    {
        auto print = [](const auto& val) {
            using T = std::remove_cvref_t<decltype(val)>;
            if constexpr (std::is_same_v<T, xll::String>)
                std::cout << "  String : \"" << val << "\"\n";
            else if constexpr (std::is_same_v<T, xll::Error>)
                std::cout << "  Error  : " << val << "\n";
            else
                std::cout << "  value  : " << val << "\n";
        };

        xll::visit(print, V(xll::Number(42.0)));
        xll::visit(print, V(xll::String("hello, Variant")));
        xll::visit(print, V(xll::Bool(false)));
        xll::visit(print, V(xll::ErrDiv0));
    }

    // ------------------------------------------------------------------
    subsection("xll::overload — separate handler per alternative");
    {
        auto describe = xll::overload{
            [](const xll::Number& n) { std::cout << "  Number  = " << static_cast<double>(n) << "\n"; },
            [](const xll::String& s) { std::cout << "  String  = \"" << s << "\"\n"; },
            [](const xll::Bool&   b) { std::cout << "  Bool    = " << std::boolalpha << static_cast<bool>(b) << "\n"; },
            [](const xll::Error&  e) { std::cout << "  Error   = " << e << "\n"; }
        };

        for (V v : { V(xll::Number(1.5)), V(xll::String("world")),
                     V(xll::Bool(true)),  V(xll::ErrNA) })
            xll::visit(describe, v);
    }

    // ------------------------------------------------------------------
    subsection("Mutable visit — visitor modifies the active value in-place");
    {
        V v(xll::Number(10.0));

        // Variant& overload: visitor receives Number&
        xll::visit(xll::overload{
            [](xll::Number& n) { n = xll::Number(static_cast<double>(n) * 2.0); },
            [](auto&)          {}
        }, v);

        std::cout << "  10 * 2 = " << xll::get<xll::Number>(v) << "\n";
    }

    // ------------------------------------------------------------------
    subsection("Rvalue visit — visitor receives U&&, can move the value out");
    {
        V v(xll::String("steal me"));
        std::string stolen;

        xll::visit(xll::overload{
            [&stolen](xll::String&& s) { stolen = std::string(std::move(s)); },
            [](auto&&)                 {}
        }, std::move(v));

        std::cout << "  stolen = \"" << stolen << "\"\n";
        std::cout << "  source val.str null = " << (v.val.str == nullptr ? "yes" : "no") << "\n";
    }

    // ------------------------------------------------------------------
    subsection("visit return value — each handler returns std::string");
    {
        auto describe = [](const auto& val) -> std::string {
            using T = std::remove_cvref_t<decltype(val)>;
            if constexpr (std::is_same_v<T, xll::Number>)
                return "number("  + std::to_string(static_cast<double>(val)) + ")";
            else if constexpr (std::is_same_v<T, xll::String>)
                return "string(\"" + std::string(val) + "\")";
            else if constexpr (std::is_same_v<T, xll::Bool>)
                return std::string("bool(") + (static_cast<bool>(val) ? "true" : "false") + ")";
            else
                return "error("  + std::string(val.to_string()) + ")";
        };

        for (V v : { V(xll::Number(2.71)), V(xll::String("pi")),
                     V(xll::Bool(false)),  V(xll::ErrValue) })
            std::cout << "  " << xll::visit(describe, v) << "\n";
    }
}

// ---------------------------------------------------------------------------
// Section 5: Mutation — operator=(U), emplace
// ---------------------------------------------------------------------------

void demo_mutation()
{
    separator("5. Mutation — operator=(U) and emplace<U>()");

    using V = xll::Variant<xll::Number, xll::String, xll::Bool>;

    // ------------------------------------------------------------------
    subsection("operator=(U) — change the active alternative");
    {
        V v(xll::Number(1.0));
        std::cout << "  before : Number " << xll::get<xll::Number>(v) << "\n";

        v = xll::String("now a string");
        std::cout << "  after  : String \"" << xll::get<xll::String>(v) << "\"\n";
        std::cout << "  index  = " << v.index() << "  (1 = String)\n";
    }

    // ------------------------------------------------------------------
    subsection("operator=(Variant) — copy-assign from another Variant");
    {
        V a(xll::Number(99.0));
        V b(xll::String("overwrite me"));
        b = a;
        std::cout << "  b holds Number? " << xll::holds_alternative<xll::Number>(b) << "\n";
        std::cout << "  b value         = " << xll::get<xll::Number>(b) << "\n";
    }

    // ------------------------------------------------------------------
    subsection("operator=(Variant) — move-assign, String buffer transferred");
    {
        V a(xll::String("move assign"));
        V b(xll::Number(0.0));
        b = std::move(a);
        std::cout << "  b value         = \"" << xll::get<xll::String>(b) << "\"\n";
        std::cout << "  a.val.str null  = " << (a.val.str == nullptr ? "yes" : "no") << "\n";
    }

    // ------------------------------------------------------------------
    subsection("emplace<U>() — construct in-place, returns U&");
    {
        V v(xll::Number(0.0));
        xll::String& ref = v.emplace<xll::String>("emplaced!");

        std::cout << "  emplace returned  = \"" << ref << "\"\n";
        std::cout << "  variant holds     = \"" << xll::get<xll::String>(v) << "\"\n";
        // ref addresses the live String object inside v
        std::cout << "  same address?     = "
                  << (&ref == &xll::get<xll::String>(v) ? "yes" : "no") << "\n";
    }

    // ------------------------------------------------------------------
    subsection("emplace<U>() destroys the old value cleanly (no leak)");
    {
        V v(xll::String("will be replaced — buffer freed"));
        v.emplace<xll::Number>(3.14);       // old String's destructor runs first

        std::cout << "  index after emplace = " << v.index() << "  (0 = Number)\n";
        std::cout << "  new value           = " << xll::get<xll::Number>(v) << "\n";
    }
}

// ---------------------------------------------------------------------------
// Section 6: swap
// ---------------------------------------------------------------------------

void demo_swap()
{
    separator("6. swap");

    using V = xll::Variant<xll::Number, xll::String>;

    // ------------------------------------------------------------------
    subsection("Same-type swap — delegates to U::swap (no destroy+construct)");
    {
        V a(xll::Number(1.0));
        V b(xll::Number(2.0));
        a.swap(b);
        std::cout << "  a = " << xll::get<xll::Number>(a) << "\n";
        std::cout << "  b = " << xll::get<xll::Number>(b) << "\n";
    }

    // ------------------------------------------------------------------
    subsection("Same-type swap — String (buffer pointers exchanged, no allocation)");
    {
        V a(xll::String("Alice"));
        V b(xll::String("Bob"));
        const auto* a_buf_before = a.val.str;
        const auto* b_buf_before = b.val.str;
        a.swap(b);
        std::cout << "  a = \"" << xll::get<xll::String>(a) << "\"  (was \"Alice\")\n";
        std::cout << "  b = \"" << xll::get<xll::String>(b) << "\"  (was \"Bob\")\n";
        // Pointers were exchanged, not reallocated
        std::cout << "  a.val.str == old b_buf? " << (a.val.str == b_buf_before ? "yes" : "no") << "\n";
        std::cout << "  b.val.str == old a_buf? " << (b.val.str == a_buf_before ? "yes" : "no") << "\n";
    }

    // ------------------------------------------------------------------
    subsection("Different-type swap — moves through a temporary");
    {
        V a(xll::Number(42.0));
        V b(xll::String("hello"));
        a.swap(b);
        std::cout << "  a now holds String : \"" << xll::get<xll::String>(a) << "\"\n";
        std::cout << "  b now holds Number :  " << xll::get<xll::Number>(b) << "\n";
    }
}

// ---------------------------------------------------------------------------
// Section 7: xll::cast<Variant<...>>(any)
// ---------------------------------------------------------------------------

void demo_cast_from_any()
{
    separator("7. xll::cast<Variant<...>>(any)");

    std::cout << "\n"
              << "  In a real UDF the function parameter is xll::Any — a type-erased\n"
              << "  wrapper over whatever value Excel passes in.  xll::cast converts it\n"
              << "  to Optional<Variant<T,Ts...>>:\n"
              << "    engaged  when any.type() is one of T::excel_type | Ts::excel_type | ...\n"
              << "    xll::None when the xltype is not in the declared type list.\n";

    using NumOrStr = xll::Variant<xll::Number, xll::String>;

    // ------------------------------------------------------------------
    subsection("Successful cast — Any(Number) -> Optional<Variant<Number,String>>");
    {
        xll::Any any(xll::Number(1.41421));
        auto result = xll::cast<NumOrStr>(any);

        std::cout << "  has_value()    = " << result.has_value() << "\n";
        std::cout << "  holds<Number>? = " << xll::holds_alternative<xll::Number>(result.value()) << "\n";
        std::cout << "  value          = " << xll::get<xll::Number>(result.value()) << "\n";
    }

    // ------------------------------------------------------------------
    subsection("Successful cast — Any(String) -> Optional<Variant<Number,String>>");
    {
        xll::Any any(xll::String("from Excel"));
        auto result = xll::cast<NumOrStr>(any);

        std::cout << "  has_value()    = " << result.has_value() << "\n";
        std::cout << "  holds<String>? = " << xll::holds_alternative<xll::String>(result.value()) << "\n";
        std::cout << "  value          = \"" << xll::get<xll::String>(result.value()) << "\"\n";
    }

    // ------------------------------------------------------------------
    subsection("Failed cast — Bool not in Variant<Number,String>");
    {
        xll::Any any(xll::Bool(true));
        auto result = xll::cast<NumOrStr>(any);

        std::cout << "  has_value() = " << result.has_value() << "\n";
        std::cout << "  == None?    = " << (result == xll::None) << "\n";
    }

    // ------------------------------------------------------------------
    subsection("Failed cast — Error not in Variant<Number,String>");
    {
        auto result = xll::cast<NumOrStr>(xll::Any(xll::ErrDiv0));
        std::cout << "  has_value() = " << result.has_value() << "\n";
    }

    // ------------------------------------------------------------------
    subsection("visit directly on the cast result");
    {
        auto show = xll::overload{
            [](const xll::Number& n) { std::cout << "  Number  = " << static_cast<double>(n) << "\n"; },
            [](const xll::String& s) { std::cout << "  String  = \"" << s << "\"\n"; }
        };

        for (xll::Any any : { xll::Any(xll::Number(7.0)), xll::Any(xll::String("pi")) }) {
            if (auto v = xll::cast<NumOrStr>(any))
                xll::visit(show, *v);
        }
    }

    // ------------------------------------------------------------------
    subsection("Optional::transform chain on cast result");
    {
        // cast<NumOrStr> -> Optional<NumOrStr>
        // .transform(f)  -> Optional<String>  where f formats the Variant as a String
        auto format_variant = [](const NumOrStr& v) {
            return xll::visit(xll::overload{
                [](const xll::Number& n) {
                    return xll::String("num:" + std::to_string(static_cast<double>(n)));
                },
                [](const xll::String& s) {
                    return xll::String("str:") + xll::String(std::string(s));
                }
            }, v);
        };

        for (xll::Any any : { xll::Any(xll::Number(2.0)), xll::Any(xll::String("hello")),
                               xll::Any(xll::Bool(true)) }) {
            auto label = xll::cast<NumOrStr>(any).transform(format_variant);
            std::cout << "  " << (label ? std::string(label.value()) : "(type not in Variant)") << "\n";
        }
    }
}

// ---------------------------------------------------------------------------
// Section 8: Practical example — UDF-like function via xll::cast + visit
// ---------------------------------------------------------------------------

// Simulates an Excel UDF that accepts a cell value (Any) which may be either
// a Number or a String.  Numbers are doubled; Strings are upper-cased.
// Any other Excel type (Bool, Error, Nil, …) returns a "#UNSUPPORTED" message.
static std::string process_cell(const xll::Any& any)
{
    using NumOrStr = xll::Variant<xll::Number, xll::String>;

    auto result = xll::cast<NumOrStr>(any);
    if (!result.has_value())
        return "#UNSUPPORTED";

    return xll::visit(xll::overload{
        [](const xll::Number& n) -> std::string {
            return std::to_string(static_cast<double>(n) * 2.0);
        },
        [](const xll::String& s) -> std::string {
            std::string u = std::string(s);
            for (char& c : u)
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            return u;
        }
    }, result.value());
}

void demo_practical_udf()
{
    separator("8. Practical example — UDF-like function via xll::cast + visit");

    std::cout << "\n  process_cell() doubles Numbers and upper-cases Strings.\n"
              << "  Any other Excel type returns #UNSUPPORTED.\n\n";

    std::cout << "  Number  3.5       -> " << process_cell(xll::Any(xll::Number(3.5)))      << "\n";
    std::cout << "  Number  0.0       -> " << process_cell(xll::Any(xll::Number(0.0)))      << "\n";
    std::cout << "  String  \"hello\" -> " << process_cell(xll::Any(xll::String("hello"))) << "\n";
    std::cout << "  String  \"xll\"  -> " << process_cell(xll::Any(xll::String("xll")))   << "\n";
    std::cout << "  Bool    true      -> " << process_cell(xll::Any(xll::Bool(true)))       << "\n";
    std::cout << "  Error   #DIV/0!   -> " << process_cell(xll::Any(xll::ErrDiv0))          << "\n";
    std::cout << "  Nil               -> " << process_cell(xll::Any(xll::Nil{}))            << "\n";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    demo_construction();
    demo_type_queries();
    demo_value_access();
    demo_visit();
    demo_mutation();
    demo_swap();
    demo_cast_from_any();
    demo_practical_udf();

    std::cout << "\n"
              << "========================================================\n"
              << "  All demos completed successfully.\n"
              << "========================================================\n";

    return 0;
}

