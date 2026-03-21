//
// Created by kenne on 25/03/2025.
//

#include "catch_amalgamated.hpp"
#include <xlcall.hpp>
#include "../Types/Variant.hpp"

#include <string>
#include <type_traits>
#include <variant>   // std::variant_npos, std::bad_variant_access

// ---------------------------------------------------------------------------
// Common aliases used throughout the test file
// ---------------------------------------------------------------------------

using V2   = xll::Variant<xll::Number, xll::String>;
using V3   = xll::Variant<xll::Number, xll::String, xll::Int>;
using V4   = xll::Variant<xll::Number, xll::String, xll::Bool, xll::Int>;
using VFull = xll::Variant<xll::Number, xll::String, xll::Bool, xll::Int, xll::Error, xll::Nil>;

// ---------------------------------------------------------------------------
// Static / compile-time assertions
// ---------------------------------------------------------------------------

// Size invariant: Variant must stay at XLOPER12 size (16 bytes)
static_assert(sizeof(V2)    == sizeof(XLOPER12));
static_assert(sizeof(V3)    == sizeof(XLOPER12));
static_assert(sizeof(VFull) == sizeof(XLOPER12));

// Alignment invariant
static_assert(alignof(V2)    == alignof(XLOPER12));
static_assert(alignof(VFull) == alignof(XLOPER12));

// Must inherit from XLOPER12
static_assert(std::is_base_of_v<XLOPER12, V2>);
static_assert(std::is_base_of_v<XLOPER12, VFull>);

// has_crtp_base marker must be present
static_assert(V2::has_crtp_base);
static_assert(VFull::has_crtp_base);

// excel_type is the bitwise-OR of all alternative excel_types
static_assert(V2::excel_type == (xll::Number::excel_type | xll::String::excel_type));
static_assert(VFull::excel_type == (xll::Number::excel_type | xll::String::excel_type |
                                     xll::Bool::excel_type  | xll::Int::excel_type    |
                                     xll::Error::excel_type | xll::Nil::excel_type));

// Standard special-member availability
static_assert(std::is_default_constructible_v<V2>);
static_assert(std::is_copy_constructible_v<V2>);
static_assert(std::is_move_constructible_v<V2>);
static_assert(std::is_copy_assignable_v<V2>);
static_assert(std::is_move_assignable_v<V2>);

// V2 is constructible from either alternative
static_assert(std::is_constructible_v<V2, xll::Number>);
static_assert(std::is_constructible_v<V2, xll::String>);

// V2 is NOT constructible from a type not in its pack
static_assert(!std::is_constructible_v<V2, xll::Bool>);
static_assert(!std::is_constructible_v<V2, xll::Int>);
static_assert(!std::is_constructible_v<V2, xll::Nil>);
static_assert(!std::is_constructible_v<V2, xll::Error>);

// =============================================================================
// LAYOUT
// =============================================================================

TEST_CASE("Variant - sizeof equals XLOPER12", "[xll::Variant][layout]")
{
    STATIC_REQUIRE(sizeof(V2)    == sizeof(XLOPER12));
    STATIC_REQUIRE(sizeof(VFull) == sizeof(XLOPER12));
}

// =============================================================================
// CONSTRUCTION
// =============================================================================

TEST_CASE("Variant - Default construction initialises first alternative", "[xll::Variant][construction]")
{
    V2 v;
    REQUIRE(v.xltype == xltypeNum);
    REQUIRE(v.is_valid());
    REQUIRE(xll::holds_alternative<xll::Number>(v));
    REQUIRE_FALSE(xll::holds_alternative<xll::String>(v));
    REQUIRE(xll::get<xll::Number>(v) == 0.0);
}

TEST_CASE("Variant - Default construction: 3-alternative (first is Number)", "[xll::Variant][construction]")
{
    V3 v;
    REQUIRE(v.index() == 0);
    REQUIRE(xll::holds_alternative<xll::Number>(v));
}

TEST_CASE("Variant - Converting copy construction from each alternative", "[xll::Variant][construction]")
{
    SECTION("Number") {
        xll::Number n = 3.14;
        V2 v(n);
        REQUIRE(v.xltype == xltypeNum);
        REQUIRE(v.is_valid());
        REQUIRE(xll::holds_alternative<xll::Number>(v));
        REQUIRE(xll::get<xll::Number>(v) == 3.14);
    }

    SECTION("String") {
        xll::String s("Hello World");
        V2 v(s);
        REQUIRE(v.xltype == xltypeStr);
        REQUIRE(v.is_valid());
        REQUIRE(xll::holds_alternative<xll::String>(v));
        REQUIRE(xll::get<xll::String>(v) == "Hello World");
    }

    SECTION("Int in V3") {
        xll::Int i = 42;
        V3 v(i);
        REQUIRE(v.xltype == xltypeInt);
        REQUIRE(v.is_valid());
        REQUIRE(xll::holds_alternative<xll::Int>(v));
        REQUIRE(xll::get<xll::Int>(v) == 42);
    }

    SECTION("Bool in V4") {
        xll::Bool b = true;
        V4 v(b);
        REQUIRE(v.xltype == xltypeBool);
        REQUIRE(v.is_valid());
        REQUIRE(xll::holds_alternative<xll::Bool>(v));
        REQUIRE(xll::get<xll::Bool>(v) == true);
    }

    SECTION("Error in VFull") {
        VFull v(xll::ErrValue);
        REQUIRE(v.xltype == xltypeErr);
        REQUIRE(v.is_valid());
        REQUIRE(xll::holds_alternative<xll::Error>(v));
        REQUIRE(xll::get<xll::Error>(v) == xll::ErrValue);
    }

    SECTION("Nil in VFull") {
        xll::Nil nil;
        VFull v(nil);
        REQUIRE(v.xltype == xltypeNil);
        REQUIRE(v.is_valid());
        REQUIRE(xll::holds_alternative<xll::Nil>(v));
    }
}

TEST_CASE("Variant - Converting move construction from each alternative", "[xll::Variant][construction]")
{
    SECTION("Number rvalue") {
        V2 v(xll::Number(99.0));
        REQUIRE(xll::holds_alternative<xll::Number>(v));
        REQUIRE(xll::get<xll::Number>(v) == 99.0);
    }

    SECTION("String rvalue") {
        V2 v(xll::String("moved"));
        REQUIRE(xll::holds_alternative<xll::String>(v));
        REQUIRE(xll::get<xll::String>(v) == "moved");
    }

    SECTION("Int rvalue in V3") {
        V3 v(xll::Int(7));
        REQUIRE(xll::holds_alternative<xll::Int>(v));
        REQUIRE(xll::get<xll::Int>(v) == 7);
    }
}

TEST_CASE("Variant - Copy constructor", "[xll::Variant][copy]")
{
    SECTION("Copy of Number variant") {
        V2 a(xll::Number(1.5));
        V2 b(a);
        REQUIRE(xll::holds_alternative<xll::Number>(b));
        REQUIRE(xll::get<xll::Number>(b) == 1.5);
        // Original unchanged
        REQUIRE(xll::holds_alternative<xll::Number>(a));
        REQUIRE(xll::get<xll::Number>(a) == 1.5);
    }

    SECTION("Copy of String variant performs deep copy") {
        V2 a(xll::String("deep copy"));
        V2 b(a);
        REQUIRE(xll::holds_alternative<xll::String>(b));
        REQUIRE(xll::get<xll::String>(b) == "deep copy");
        // Modifying b does not affect a
        xll::get<xll::String>(b) = xll::String("changed");
        REQUIRE(xll::get<xll::String>(a) == "deep copy");
    }

    SECTION("Copy of Bool variant in VFull") {
        VFull a(xll::Bool(true));
        VFull b(a);
        REQUIRE(xll::holds_alternative<xll::Bool>(b));
        REQUIRE(xll::get<xll::Bool>(b) == true);
    }

    SECTION("Copy preserves index") {
        V3 a(xll::Int(5));
        V3 b(a);
        REQUIRE(a.index() == b.index());
        REQUIRE(xll::get<xll::Int>(b) == 5);
    }
}

TEST_CASE("Variant - Move constructor", "[xll::Variant][move]")
{
    SECTION("Move of Number variant") {
        V2 a(xll::Number(2.5));
        V2 b(std::move(a));
        REQUIRE(xll::holds_alternative<xll::Number>(b));
        REQUIRE(xll::get<xll::Number>(b) == 2.5);
    }

    SECTION("Move of String variant transfers ownership") {
        V2 a(xll::String("transfer me"));
        V2 b(std::move(a));
        REQUIRE(xll::holds_alternative<xll::String>(b));
        REQUIRE(xll::get<xll::String>(b) == "transfer me");
    }

    SECTION("Move of Error variant in VFull") {
        VFull a(xll::ErrName);
        VFull b(std::move(a));
        REQUIRE(xll::holds_alternative<xll::Error>(b));
        REQUIRE(xll::get<xll::Error>(b) == xll::ErrName);
    }
}

// =============================================================================
// ASSIGNMENT
// =============================================================================

TEST_CASE("Variant - Copy assignment operator", "[xll::Variant][assignment]")
{
    SECTION("Number <- Number") {
        V2 a(xll::Number(10.0));
        V2 b;
        b = a;
        REQUIRE(xll::holds_alternative<xll::Number>(b));
        REQUIRE(xll::get<xll::Number>(b) == 10.0);
    }

    SECTION("String <- String (deep copy)") {
        V2 a(xll::String("copy assign"));
        V2 b;
        b = a;
        REQUIRE(xll::holds_alternative<xll::String>(b));
        REQUIRE(xll::get<xll::String>(b) == "copy assign");
    }

    SECTION("Number <- String (type changes)") {
        V2 a(xll::String("switch"));
        V2 b(xll::Number(0.0));
        b = a;
        REQUIRE(xll::holds_alternative<xll::String>(b));
        REQUIRE(xll::get<xll::String>(b) == "switch");
    }

    SECTION("Self-assignment is a no-op") {
        V2 a(xll::Number(42.0));
        V2& self = a;
        a = self;   // self-assignment via reference
        REQUIRE(xll::holds_alternative<xll::Number>(a));
        REQUIRE(xll::get<xll::Number>(a) == 42.0);
    }
}

TEST_CASE("Variant - Move assignment operator", "[xll::Variant][assignment]")
{
    SECTION("Number <- Number rvalue") {
        V2 a(xll::Number(7.7));
        V2 b;
        b = std::move(a);
        REQUIRE(xll::holds_alternative<xll::Number>(b));
        REQUIRE(xll::get<xll::Number>(b) == 7.7);
    }

    SECTION("String <- String rvalue transfers ownership") {
        V2 a(xll::String("move assign"));
        V2 b;
        b = std::move(a);
        REQUIRE(xll::holds_alternative<xll::String>(b));
        REQUIRE(xll::get<xll::String>(b) == "move assign");
    }

    SECTION("Type change via move assignment (String -> Number)") {
        V2 a(xll::Number(3.0));
        V2 b(xll::String("before"));
        b = std::move(a);
        REQUIRE(xll::holds_alternative<xll::Number>(b));
        REQUIRE(xll::get<xll::Number>(b) == 3.0);
    }

    SECTION("Self-move-assignment is a no-op") {
        V2 a(xll::Number(1.0));
        V2& self = a;
        a = std::move(self);   // self-move-assignment via reference
        REQUIRE(xll::holds_alternative<xll::Number>(a));
        REQUIRE(xll::get<xll::Number>(a) == 1.0);
    }
}

TEST_CASE("Variant - Converting copy assignment", "[xll::Variant][assignment]")
{
    SECTION("Assign Number to Number variant (same active type)") {
        V2 v(xll::Number(1.0));
        xll::Number n = 99.0;
        v = n;
        REQUIRE(xll::holds_alternative<xll::Number>(v));
        REQUIRE(xll::get<xll::Number>(v) == 99.0);
    }

    SECTION("Assign String to Number variant (type changes)") {
        V2 v(xll::Number(1.0));
        xll::String s("hello");
        v = s;
        REQUIRE(xll::holds_alternative<xll::String>(v));
        REQUIRE(xll::get<xll::String>(v) == "hello");
    }

    SECTION("Assign Int to V3 variant") {
        V3 v;
        v = xll::Int(55);
        REQUIRE(xll::holds_alternative<xll::Int>(v));
        REQUIRE(xll::get<xll::Int>(v) == 55);
    }

    SECTION("Assign Bool to VFull variant") {
        VFull v;
        v = xll::Bool(false);
        REQUIRE(xll::holds_alternative<xll::Bool>(v));
        REQUIRE(xll::get<xll::Bool>(v) == false);
    }

    SECTION("Assign Error to VFull variant") {
        VFull v;
        v = xll::ErrDiv0;
        REQUIRE(xll::holds_alternative<xll::Error>(v));
        REQUIRE(xll::get<xll::Error>(v) == xll::ErrDiv0);
    }

    SECTION("Assign Nil to VFull variant") {
        VFull v(xll::Number(1.0));
        v = xll::Nil{};
        REQUIRE(xll::holds_alternative<xll::Nil>(v));
    }
}

TEST_CASE("Variant - Converting move assignment", "[xll::Variant][assignment]")
{
    SECTION("Move-assign Number (same active type)") {
        V2 v(xll::Number(0.0));
        v = xll::Number(3.14);
        REQUIRE(xll::holds_alternative<xll::Number>(v));
        REQUIRE(xll::get<xll::Number>(v) == 3.14);
    }

    SECTION("Move-assign String rvalue (type changes)") {
        V2 v(xll::Number(1.0));
        v = xll::String("rvalue string");
        REQUIRE(xll::holds_alternative<xll::String>(v));
        REQUIRE(xll::get<xll::String>(v) == "rvalue string");
    }
}

// =============================================================================
// OBSERVERS
// =============================================================================

TEST_CASE("Variant - is_valid()", "[xll::Variant][observers]")
{
    SECTION("Default-constructed is valid") {
        V2 v;
        REQUIRE(v.is_valid());
    }

    SECTION("All alternatives are valid") {
        REQUIRE(V2(xll::Number(1.0)).is_valid());
        REQUIRE(V2(xll::String("s")).is_valid());
        REQUIRE(VFull(xll::Bool(true)).is_valid());
        REQUIRE(VFull(xll::Int(0)).is_valid());
        REQUIRE(VFull(xll::ErrNA).is_valid());
        REQUIRE(VFull(xll::Nil{}).is_valid());
    }
}

TEST_CASE("Variant - index()", "[xll::Variant][observers]")
{
    SECTION("Default construction gives index 0") {
        V2 v;
        REQUIRE(v.index() == 0);
    }

    SECTION("Correct index for each alternative in V3") {
        REQUIRE(V3(xll::Number(0.0)).index() == 0);
        REQUIRE(V3(xll::String("")).index()  == 1);
        REQUIRE(V3(xll::Int(0)).index()      == 2);
    }

    SECTION("Correct index for each alternative in VFull") {
        REQUIRE(VFull(xll::Number(0.0)).index() == 0);
        REQUIRE(VFull(xll::String("")).index()  == 1);
        REQUIRE(VFull(xll::Bool(true)).index()  == 2);
        REQUIRE(VFull(xll::Int(0)).index()      == 3);
        REQUIRE(VFull(xll::ErrNull).index()     == 4);
        REQUIRE(VFull(xll::Nil{}).index()       == 5);
    }

    SECTION("index() after assignment changes correctly") {
        V3 v(xll::Number(1.0));
        REQUIRE(v.index() == 0);
        v = xll::String("hi");
        REQUIRE(v.index() == 1);
        v = xll::Int(3);
        REQUIRE(v.index() == 2);
    }
}

TEST_CASE("Variant - holds_alternative()", "[xll::Variant][observers]")
{
    SECTION("True for active alternative, false for others") {
        V3 vn(xll::Number(1.0));
        REQUIRE( xll::holds_alternative<xll::Number>(vn));
        REQUIRE_FALSE(xll::holds_alternative<xll::String>(vn));
        REQUIRE_FALSE(xll::holds_alternative<xll::Int>(vn));

        V3 vs(xll::String("x"));
        REQUIRE_FALSE(xll::holds_alternative<xll::Number>(vs));
        REQUIRE( xll::holds_alternative<xll::String>(vs));
        REQUIRE_FALSE(xll::holds_alternative<xll::Int>(vs));

        V3 vi(xll::Int(2));
        REQUIRE_FALSE(xll::holds_alternative<xll::Number>(vi));
        REQUIRE_FALSE(xll::holds_alternative<xll::String>(vi));
        REQUIRE( xll::holds_alternative<xll::Int>(vi));
    }

    SECTION("Changes after assignment") {
        V2 v(xll::Number(0.0));
        REQUIRE(xll::holds_alternative<xll::Number>(v));
        v = xll::String("changed");
        REQUIRE(xll::holds_alternative<xll::String>(v));
        REQUIRE_FALSE(xll::holds_alternative<xll::Number>(v));
    }
}

// =============================================================================
// MODIFIERS — emplace
// =============================================================================

TEST_CASE("Variant - emplace<U>(args...)", "[xll::Variant][modifiers]")
{
    SECTION("Emplace Number into default V2") {
        V2 v;
        xll::Number& ref = v.emplace<xll::Number>(xll::Number(55.5));
        REQUIRE(xll::holds_alternative<xll::Number>(v));
        REQUIRE(ref == 55.5);
        REQUIRE(xll::get<xll::Number>(v) == 55.5);
    }

    SECTION("Emplace String into Number variant (type changes)") {
        V2 v(xll::Number(1.0));
        xll::String& ref = v.emplace<xll::String>(xll::String("emplaced"));
        REQUIRE(xll::holds_alternative<xll::String>(v));
        REQUIRE(ref == "emplaced");
    }

    SECTION("Emplace Int into V3") {
        V3 v;
        v.emplace<xll::Int>(xll::Int(100));
        REQUIRE(xll::holds_alternative<xll::Int>(v));
        REQUIRE(xll::get<xll::Int>(v) == 100);
    }

    SECTION("Returned reference addresses live object") {
        V2 v;
        xll::Number& ref = v.emplace<xll::Number>(xll::Number(7.0));
        ref = xll::Number(8.0);   // modify through the returned ref
        REQUIRE(xll::get<xll::Number>(v) == 8.0);
    }
}

// =============================================================================
// MODIFIERS — swap
// =============================================================================

TEST_CASE("Variant - swap()", "[xll::Variant][modifiers]")
{
    SECTION("Swap two same-type variants (Number)") {
        V2 a(xll::Number(1.0));
        V2 b(xll::Number(2.0));
        a.swap(b);
        REQUIRE(xll::get<xll::Number>(a) == 2.0);
        REQUIRE(xll::get<xll::Number>(b) == 1.0);
    }

    SECTION("Swap two different-type variants (Number <-> String)") {
        V2 a(xll::Number(3.14));
        V2 b(xll::String("hello"));
        a.swap(b);
        REQUIRE(xll::holds_alternative<xll::String>(a));
        REQUIRE(xll::get<xll::String>(a) == "hello");
        REQUIRE(xll::holds_alternative<xll::Number>(b));
        REQUIRE(xll::get<xll::Number>(b) == 3.14);
    }

    SECTION("Self-swap is a no-op") {
        V2 v(xll::Number(9.9));
        v.swap(v);
        REQUIRE(xll::holds_alternative<xll::Number>(v));
        REQUIRE(xll::get<xll::Number>(v) == 9.9);
    }

    SECTION("Swap with String transfers content correctly") {
        V2 a(xll::String("AAA"));
        V2 b(xll::String("BBB"));
        a.swap(b);
        REQUIRE(xll::get<xll::String>(a) == "BBB");
        REQUIRE(xll::get<xll::String>(b) == "AAA");
    }
}

// =============================================================================
// VALUE ACCESS — get
// =============================================================================

TEST_CASE("Variant - get<U>() (free function)", "[xll::Variant][access]")
{
    SECTION("Returns reference to active Number") {
        V2 v(xll::Number(1.23));
        REQUIRE(xll::get<xll::Number>(v) == 1.23);
    }

    SECTION("Returned lvalue reference is modifiable") {
        V2 v(xll::Number(0.0));
        xll::get<xll::Number>(v) = xll::Number(42.0);
        REQUIRE(xll::get<xll::Number>(v) == 42.0);
    }

    SECTION("Returns const reference on const variant") {
        const V2 cv(xll::String("const"));
        const xll::String& s = xll::get<xll::String>(cv);
        REQUIRE(s == "const");
    }

    SECTION("Returns rvalue reference on rvalue variant") {
        V2 v(xll::Number(5.0));
        xll::Number n = xll::get<xll::Number>(std::move(v));
        REQUIRE(n == 5.0);
    }

    SECTION("Throws std::bad_variant_access on type mismatch") {
        V2 v(xll::Number(1.0));
        REQUIRE_THROWS_AS(xll::get<xll::String>(v), std::bad_variant_access);
    }

    SECTION("Throws on mismatch with const variant") {
        const V2 cv(xll::String("hi"));
        REQUIRE_THROWS_AS(xll::get<xll::Number>(cv), std::bad_variant_access);
    }

    SECTION("All alternatives in VFull are accessible") {
        REQUIRE(xll::get<xll::Number>(VFull(xll::Number(1.0))) == 1.0);
        REQUIRE(xll::get<xll::String>(VFull(xll::String("s"))) == "s");
        REQUIRE(xll::get<xll::Bool>(VFull(xll::Bool(true)))    == true);
        REQUIRE(xll::get<xll::Int>(VFull(xll::Int(3)))         == 3);
        REQUIRE(xll::get<xll::Error>(VFull(xll::ErrNum))       == xll::ErrNum);
    }
}

// =============================================================================
// VALUE ACCESS — get_if
// =============================================================================

TEST_CASE("Variant - get_if<U>() (free function)", "[xll::Variant][access]")
{
    SECTION("Returns engaged Optional for active type") {
        V2 v(xll::Number(7.0));
        auto opt = xll::get_if<xll::Number>(v);
        REQUIRE(opt.has_value());
        REQUIRE(opt.value() == 7.0);
    }

    SECTION("Returns disengaged Optional for inactive type") {
        V2 v(xll::Number(7.0));
        auto opt = xll::get_if<xll::String>(v);
        REQUIRE_FALSE(opt.has_value());
    }

    SECTION("Works with String as active type") {
        V2 v(xll::String("get_if test"));
        auto opt = xll::get_if<xll::String>(v);
        REQUIRE(opt.has_value());
        REQUIRE(opt.value() == "get_if test");
    }

    SECTION("Works on const variant") {
        const V2 cv(xll::Number(2.5));
        auto opt = xll::get_if<xll::Number>(cv);
        REQUIRE(opt.has_value());
        REQUIRE(opt.value() == 2.5);
    }

    SECTION("Rvalue get_if moves value into Optional") {
        V2 v(xll::Number(9.0));
        auto opt = xll::get_if<xll::Number>(std::move(v));
        REQUIRE(opt.has_value());
        REQUIRE(opt.value() == 9.0);
    }

    SECTION("All alternatives in VFull") {
        REQUIRE( xll::get_if<xll::Number>(VFull(xll::Number(1.0))).has_value());
        REQUIRE( xll::get_if<xll::String>(VFull(xll::String("x"))).has_value());
        REQUIRE( xll::get_if<xll::Bool>(VFull(xll::Bool(false))).has_value());
        REQUIRE( xll::get_if<xll::Int>(VFull(xll::Int(9))).has_value());
        REQUIRE( xll::get_if<xll::Error>(VFull(xll::ErrRef)).has_value());
        REQUIRE( xll::get_if<xll::Nil>(VFull(xll::Nil{})).has_value());

        // Wrong type returns None
        REQUIRE_FALSE(xll::get_if<xll::String>(VFull(xll::Number(1.0))).has_value());
        REQUIRE_FALSE(xll::get_if<xll::Number>(VFull(xll::String("x"))).has_value());
    }
}

// =============================================================================
// VISITATION — visit + overload
// =============================================================================

TEST_CASE("Variant - visit() dispatches to active alternative", "[xll::Variant][visit]")
{
    using namespace std::literals;

    auto type_name = xll::overload{
        [](const xll::Number&) { return "Number"s; },
        [](const xll::String&) { return "String"s; },
        [](const xll::Bool&)   { return "Bool"s;   },
        [](const xll::Int&)    { return "Int"s;     },
        [](const xll::Error&)  { return "Error"s;  },
        [](const xll::Nil&)    { return "Nil"s;     }
    };

    SECTION("Number variant") {
        VFull v(xll::Number(0.0));
        REQUIRE(xll::visit(type_name, v) == "Number");
    }

    SECTION("String variant") {
        VFull v(xll::String("x"));
        REQUIRE(xll::visit(type_name, v) == "String");
    }

    SECTION("Bool variant") {
        VFull v(xll::Bool(true));
        REQUIRE(xll::visit(type_name, v) == "Bool");
    }

    SECTION("Int variant") {
        VFull v(xll::Int(0));
        REQUIRE(xll::visit(type_name, v) == "Int");
    }

    SECTION("Error variant") {
        VFull v(xll::ErrNA);
        REQUIRE(xll::visit(type_name, v) == "Error");
    }

    SECTION("Nil variant") {
        VFull v(xll::Nil{});
        REQUIRE(xll::visit(type_name, v) == "Nil");
    }
}

TEST_CASE("Variant - visit() on const variant", "[xll::Variant][visit]")
{
    const V2 cv(xll::Number(2.0));
    double result = xll::visit(xll::overload{
        [](const xll::Number& n) { return static_cast<double>(n); },
        [](const xll::String&)   { return -1.0; }
    }, cv);
    REQUIRE(result == 2.0);
}

TEST_CASE("Variant - visit() on rvalue variant", "[xll::Variant][visit]")
{
    V2 v(xll::Number(5.0));
    double result = xll::visit(xll::overload{
        [](xll::Number&& n) { return static_cast<double>(n); },
        [](xll::String&&)   { return -1.0; }
    }, std::move(v));
    REQUIRE(result == 5.0);
}

TEST_CASE("Variant - visit() produces computed result from each alternative", "[xll::Variant][visit]")
{
    auto double_value = xll::overload{
        [](const xll::Number& n) { return static_cast<double>(n) * 2.0; },
        [](const xll::Int&    i) { return static_cast<double>(static_cast<int>(i)) * 2.0; },
        [](const xll::Bool&   b) { return static_cast<double>(static_cast<bool>(b) ? 1 : 0) * 2.0; },
        [](const xll::String&)   { return 0.0; },
        [](const xll::Error&)    { return 0.0; },
        [](const xll::Nil&)      { return 0.0; }
    };

    REQUIRE(xll::visit(double_value, VFull(xll::Number(3.0))) == 6.0);
    REQUIRE(xll::visit(double_value, VFull(xll::Int(4)))      == 8.0);
    REQUIRE(xll::visit(double_value, VFull(xll::Bool(true)))  == 2.0);
    REQUIRE(xll::visit(double_value, VFull(xll::Bool(false))) == 0.0);
}

TEST_CASE("Variant - visit() on two-alternative V2", "[xll::Variant][visit]")
{
    auto visitor = xll::overload{
        [](const xll::Number& n) { return static_cast<double>(n); },
        [](const xll::String&)   { return -1.0; }
    };

    V2 vn(xll::Number(10.0));
    REQUIRE(xll::visit(visitor, vn) == 10.0);

    V2 vs(xll::String("ignored"));
    REQUIRE(xll::visit(visitor, vs) == -1.0);
}

// =============================================================================
// EXCEL-OWNERSHIP BIT MASKING
// =============================================================================

TEST_CASE("Variant - Excel ownership bits are masked in holds_alternative", "[xll::Variant][excel_interop]")
{
    // Simulate a value where Excel has set xlbitXLFree on xltype
    V2 v(xll::Number(42.0));

    // Directly set the ownership bit as Excel would
    v.xltype |= static_cast<int>(xlbitXLFree);

    // holds_alternative must still recognise the type correctly
    REQUIRE(xll::holds_alternative<xll::Number>(v));
    REQUIRE_FALSE(xll::holds_alternative<xll::String>(v));
    REQUIRE(v.is_valid());

    // get must work even with the ownership bit set
    REQUIRE(xll::get<xll::Number>(v) == 42.0);
}

TEST_CASE("Variant - xlbitDLLFree is masked in holds_alternative", "[xll::Variant][excel_interop]")
{
    V2 v(xll::String("dll-owned"));
    v.xltype |= static_cast<int>(xlbitDLLFree);

    REQUIRE(xll::holds_alternative<xll::String>(v));
    REQUIRE(v.is_valid());
    REQUIRE(xll::get<xll::String>(v) == "dll-owned");
}

// =============================================================================
// MULTIPLE ALTERNATIVE COUNTS — smoke tests
// =============================================================================

TEST_CASE("Variant - Two-alternative specialization roundtrip", "[xll::Variant][multi]")
{
    V2 v;
    v = xll::Number(1.0);
    REQUIRE(xll::holds_alternative<xll::Number>(v));
    v = xll::String("two");
    REQUIRE(xll::holds_alternative<xll::String>(v));
    v = xll::Number(2.0);
    REQUIRE(xll::holds_alternative<xll::Number>(v));
}

TEST_CASE("Variant - Three-alternative specialization roundtrip", "[xll::Variant][multi]")
{
    V3 v;
    v = xll::String("start");
    REQUIRE(xll::holds_alternative<xll::String>(v));
    v = xll::Int(99);
    REQUIRE(xll::holds_alternative<xll::Int>(v));
    v = xll::Number(3.3);
    REQUIRE(xll::holds_alternative<xll::Number>(v));
}

TEST_CASE("Variant - Full six-alternative specialization cycles through all alternatives", "[xll::Variant][multi]")
{
    VFull v;
    // Number
    v = xll::Number(1.0);
    REQUIRE(xll::holds_alternative<xll::Number>(v));
    REQUIRE(v.index() == 0);
    // String
    v = xll::String("full");
    REQUIRE(xll::holds_alternative<xll::String>(v));
    REQUIRE(v.index() == 1);
    // Bool
    v = xll::Bool(true);
    REQUIRE(xll::holds_alternative<xll::Bool>(v));
    REQUIRE(v.index() == 2);
    // Int
    v = xll::Int(7);
    REQUIRE(xll::holds_alternative<xll::Int>(v));
    REQUIRE(v.index() == 3);
    // Error
    v = xll::ErrNull;
    REQUIRE(xll::holds_alternative<xll::Error>(v));
    REQUIRE(v.index() == 4);
    // Nil
    v = xll::Nil{};
    REQUIRE(xll::holds_alternative<xll::Nil>(v));
    REQUIRE(v.index() == 5);
}

// =============================================================================
// MEMBER visit (deducing-this)
// =============================================================================

TEST_CASE("Variant - member visit via deducing-this", "[xll::Variant][visit]")
{
    using namespace std::literals;

    V2 v(xll::String("member visit"));

    std::string result = v.visit(xll::overload{
        [](const xll::Number&) { return "number"s; },
        [](const xll::String&) { return "string"s; }
    });
    REQUIRE(result == "string");
}

TEST_CASE("Variant - member get via deducing-this", "[xll::Variant][access]")
{
    V2 v(xll::Number(6.28));
    REQUIRE(v.get<xll::Number>() == 6.28);
    REQUIRE_THROWS_AS(v.get<xll::String>(), std::bad_variant_access);
}


