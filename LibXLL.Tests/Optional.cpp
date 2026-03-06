//
// Tests for xll::Optional, xll::Nullopt, and xll::None
//

#include "catch_amalgamated.hpp"
#include <xlcall.hpp>
#include "../Types/Optional.hpp"
#include "../Types/Number.hpp"
#include "../Types/Int.hpp"
#include "../Types/Bool.hpp"
#include "../Types/String.hpp"
#include "../Types/Error.hpp"

// =============================================================================
// CONSTRUCTION
// =============================================================================

TEST_CASE("Optional - Default Construction (disengaged)", "[xll::Optional][construction]")
{
    xll::Optional<xll::Number> opt;
    REQUIRE_FALSE(opt.has_value());
    REQUIRE_FALSE(static_cast<bool>(opt));
    REQUIRE(opt.xltype == xltypeNil);
}

TEST_CASE("Optional - Construct from xll::None", "[xll::Optional][construction]")
{
    xll::Optional<xll::Number> opt = xll::None;
    REQUIRE_FALSE(opt.has_value());
    REQUIRE(opt == xll::None);
}

TEST_CASE("Optional - Construct from TValue (copy)", "[xll::Optional][construction]")
{
    xll::Number n = 42.0;
    xll::Optional<xll::Number> opt(n);
    REQUIRE(opt.has_value());
    REQUIRE(opt.xltype == xltypeNum);
    REQUIRE(opt.value() == 42.0);
}

TEST_CASE("Optional - Construct from TValue (move)", "[xll::Optional][construction]")
{
    xll::Optional<xll::Number> opt(xll::Number(3.14));
    REQUIRE(opt.has_value());
    REQUIRE(opt.value() == 3.14);
}

TEST_CASE("Optional - Construct from convertible type (double -> Number)", "[xll::Optional][construction]")
{
    xll::Optional<xll::Number> opt = 99.0;
    REQUIRE(opt.has_value());
    REQUIRE(opt.value() == 99.0);
}

TEST_CASE("Optional - Construct Optional<String>", "[xll::Optional][construction]")
{
    xll::Optional<xll::String> opt = xll::String("hello");
    REQUIRE(opt.has_value());
    REQUIRE(opt.xltype == xltypeStr);
}

// =============================================================================
// COPY / MOVE
// =============================================================================

TEST_CASE("Optional - Copy constructor (engaged)", "[xll::Optional][copy]")
{
    xll::Optional<xll::Number> a = 7.0;
    xll::Optional<xll::Number> b = a;
    REQUIRE(b.has_value());
    REQUIRE(b.value() == 7.0);
}

TEST_CASE("Optional - Copy constructor (disengaged)", "[xll::Optional][copy]")
{
    xll::Optional<xll::Number> a;
    xll::Optional<xll::Number> b = a;
    REQUIRE_FALSE(b.has_value());
}

TEST_CASE("Optional - Move constructor (engaged)", "[xll::Optional][move]")
{
    xll::Optional<xll::Number> a = 5.0;
    xll::Optional<xll::Number> b = std::move(a);
    REQUIRE(b.has_value());
    REQUIRE(b.value() == 5.0);
}

TEST_CASE("Optional - Move constructor (disengaged)", "[xll::Optional][move]")
{
    xll::Optional<xll::Number> a;
    xll::Optional<xll::Number> b = std::move(a);
    REQUIRE_FALSE(b.has_value());
}

// =============================================================================
// ASSIGNMENT
// =============================================================================

TEST_CASE("Optional - Assign xll::None (reset)", "[xll::Optional][assignment]")
{
    xll::Optional<xll::Number> opt = 1.0;
    REQUIRE(opt.has_value());
    opt = xll::None;
    REQUIRE_FALSE(opt.has_value());
    REQUIRE(opt == xll::None);
}

TEST_CASE("Optional - Assign TValue", "[xll::Optional][assignment]")
{
    xll::Optional<xll::Number> opt;
    opt = xll::Number(2.0);
    REQUIRE(opt.has_value());
    REQUIRE(opt.value() == 2.0);
}

TEST_CASE("Optional - Copy assignment", "[xll::Optional][assignment]")
{
    xll::Optional<xll::Number> a = 8.0;
    xll::Optional<xll::Number> b;
    b = a;
    REQUIRE(b.has_value());
    REQUIRE(b.value() == 8.0);
}

TEST_CASE("Optional - Move assignment", "[xll::Optional][assignment]")
{
    xll::Optional<xll::Number> a = 9.0;
    xll::Optional<xll::Number> b;
    b = std::move(a);
    REQUIRE(b.has_value());
    REQUIRE(b.value() == 9.0);
}

TEST_CASE("Optional - Self assignment", "[xll::Optional][assignment]")
{
    xll::Optional<xll::Number> opt = 3.0;
    opt = opt;   // NOLINT (intentional self-assign)
    REQUIRE(opt.has_value());
    REQUIRE(opt.value() == 3.0);
}

// =============================================================================
// VALUE ACCESS
// =============================================================================

TEST_CASE("Optional - value() throws on disengaged", "[xll::Optional][access]")
{
    xll::Optional<xll::Number> opt;
    REQUIRE_THROWS_AS(opt.value(), std::bad_optional_access);
}

TEST_CASE("Optional - operator* (engaged)", "[xll::Optional][access]")
{
    xll::Optional<xll::Number> opt = 11.0;
    REQUIRE(*opt == 11.0);
}

TEST_CASE("Optional - operator-> (engaged)", "[xll::Optional][access]")
{
    xll::Optional<xll::Number> opt = 12.0;
    REQUIRE(opt->val.num == 12.0);
}

TEST_CASE("Optional - value_or (engaged)", "[xll::Optional][access]")
{
    xll::Optional<xll::Number> opt = 5.0;
    REQUIRE(opt.value_or(xll::Number(0.0)) == 5.0);
}

TEST_CASE("Optional - value_or (disengaged)", "[xll::Optional][access]")
{
    xll::Optional<xll::Number> opt;
    REQUIRE(opt.value_or(xll::Number(-1.0)) == -1.0);
}

// =============================================================================
// MODIFIERS
// =============================================================================

TEST_CASE("Optional - reset()", "[xll::Optional][modifiers]")
{
    xll::Optional<xll::Number> opt = 1.0;
    opt.reset();
    REQUIRE_FALSE(opt.has_value());
}

TEST_CASE("Optional - emplace()", "[xll::Optional][modifiers]")
{
    xll::Optional<xll::Number> opt;
    auto& ref = opt.emplace(xll::Number(77.0));
    REQUIRE(opt.has_value());
    REQUIRE(ref == 77.0);
}

TEST_CASE("Optional - emplace replaces existing value", "[xll::Optional][modifiers]")
{
    xll::Optional<xll::Number> opt = 1.0;
    opt.emplace(xll::Number(2.0));
    REQUIRE(opt.has_value());
    REQUIRE(opt.value() == 2.0);
}

// =============================================================================
// SWAP
// =============================================================================

TEST_CASE("Optional - swap (both engaged)", "[xll::Optional][swap]")
{
    xll::Optional<xll::Number> a = 1.0;
    xll::Optional<xll::Number> b = 2.0;
    a.swap(b);
    REQUIRE(a.value() == 2.0);
    REQUIRE(b.value() == 1.0);
}

TEST_CASE("Optional - swap (engaged with disengaged)", "[xll::Optional][swap]")
{
    xll::Optional<xll::Number> a = 3.0;
    xll::Optional<xll::Number> b;
    a.swap(b);
    REQUIRE_FALSE(a.has_value());
    REQUIRE(b.has_value());
    REQUIRE(b.value() == 3.0);
}

TEST_CASE("Optional - non-member swap", "[xll::Optional][swap]")
{
    xll::Optional<xll::Number> a = 4.0;
    xll::Optional<xll::Number> b = 5.0;
    swap(a, b);
    REQUIRE(a.value() == 5.0);
    REQUIRE(b.value() == 4.0);
}

// =============================================================================
// EQUALITY
// =============================================================================

TEST_CASE("Optional - equality: two disengaged", "[xll::Optional][equality]")
{
    xll::Optional<xll::Number> a;
    xll::Optional<xll::Number> b;
    REQUIRE(a == b);
}

TEST_CASE("Optional - equality: engaged with same value", "[xll::Optional][equality]")
{
    xll::Optional<xll::Number> a = 6.0;
    xll::Optional<xll::Number> b = 6.0;
    REQUIRE(a == b);
}

TEST_CASE("Optional - equality: engaged with different value", "[xll::Optional][equality]")
{
    xll::Optional<xll::Number> a = 6.0;
    xll::Optional<xll::Number> b = 7.0;
    REQUIRE_FALSE(a == b);
}

TEST_CASE("Optional - equality: engaged vs disengaged", "[xll::Optional][equality]")
{
    xll::Optional<xll::Number> a = 1.0;
    xll::Optional<xll::Number> b;
    REQUIRE_FALSE(a == b);
}

TEST_CASE("Optional - equality with TValue", "[xll::Optional][equality]")
{
    xll::Optional<xll::Number> opt = 3.0;
    REQUIRE(opt == xll::Number(3.0));
    REQUIRE_FALSE(opt == xll::Number(4.0));
}

TEST_CASE("Optional - equality with xll::None", "[xll::Optional][equality]")
{
    xll::Optional<xll::Number> engaged = 1.0;
    xll::Optional<xll::Number> empty;
    REQUIRE(empty == xll::None);
    REQUIRE_FALSE(engaged == xll::None);
}

// =============================================================================
// MONADIC OPERATIONS
// =============================================================================

TEST_CASE("Optional - transform (engaged)", "[xll::Optional][monadic]")
{
    xll::Optional<xll::Number> opt = 10.0;
    auto result = opt.transform([](xll::Number& n) { return xll::Number(n.val.num * 2.0); });
    REQUIRE(result.has_value());
    REQUIRE(result.value() == 20.0);
}

TEST_CASE("Optional - transform (disengaged)", "[xll::Optional][monadic]")
{
    xll::Optional<xll::Number> opt;
    auto result = opt.transform([](xll::Number& n) { return xll::Number(n.val.num * 2.0); });
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("Optional - and_then (engaged, returns value)", "[xll::Optional][monadic]")
{
    xll::Optional<xll::Number> opt = 4.0;
    auto result = opt.and_then([](xll::Number& n) -> xll::Optional<xll::Number> {
        if (n.val.num == 0.0) return xll::None;
        return xll::Number(1.0 / n.val.num);
    });
    REQUIRE(result.has_value());
    REQUIRE(result.value() == 0.25);
}

TEST_CASE("Optional - and_then (engaged, returns empty)", "[xll::Optional][monadic]")
{
    xll::Optional<xll::Number> opt = 0.0;
    auto result = opt.and_then([](xll::Number& n) -> xll::Optional<xll::Number> {
        if (n.val.num == 0.0) return xll::None;
        return xll::Number(1.0 / n.val.num);
    });
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("Optional - and_then (disengaged)", "[xll::Optional][monadic]")
{
    xll::Optional<xll::Number> opt;
    bool called = false;
    auto result = opt.and_then([&called](xll::Number& n) -> xll::Optional<xll::Number> {
        called = true;
        return n;
    });
    REQUIRE_FALSE(called);
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("Optional - or_else (disengaged, provides fallback)", "[xll::Optional][monadic]")
{
    xll::Optional<xll::Number> opt;
    auto result = opt.or_else([]() -> xll::Optional<xll::Number> { return xll::Number(99.0); });
    REQUIRE(result.has_value());
    REQUIRE(result.value() == 99.0);
}

TEST_CASE("Optional - or_else (engaged, skips fallback)", "[xll::Optional][monadic]")
{
    xll::Optional<xll::Number> opt = 5.0;
    bool called = false;
    auto result = opt.or_else([&called]() -> xll::Optional<xll::Number> {
        called = true;
        return xll::None;
    });
    REQUIRE_FALSE(called);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == 5.0);
}

// =============================================================================
// PIPE OPERATOR
// =============================================================================

TEST_CASE("Optional - pipe operator with opt_transform", "[xll::Optional][pipe]")
{
    xll::Optional<xll::Number> opt = 3.0;
    auto result = opt
        | xll::opt_transform([](xll::Number& n) { return xll::Number(n.val.num + 1.0); })
        | xll::opt_transform([](xll::Number& n) { return xll::Number(n.val.num * 2.0); });
    REQUIRE(result.has_value());
    REQUIRE(result.value() == 8.0);   // (3+1)*2
}

TEST_CASE("Optional - pipe operator propagates empty", "[xll::Optional][pipe]")
{
    xll::Optional<xll::Number> opt;
    bool called = false;
    auto result = opt
        | xll::opt_transform([&called](xll::Number& n) {
            called = true;
            return xll::Number(n.val.num * 2.0);
        });
    REQUIRE_FALSE(called);
    REQUIRE_FALSE(result.has_value());
}

// =============================================================================
// XLOPER12 MEMORY LAYOUT
// =============================================================================

TEST_CASE("Optional - sizeof equals XLOPER12", "[xll::Optional][layout]")
{
    REQUIRE(sizeof(xll::Optional<xll::Number>) == sizeof(XLOPER12));
    REQUIRE(sizeof(xll::Optional<xll::String>) == sizeof(XLOPER12));
    REQUIRE(sizeof(xll::Optional<xll::Int>)    == sizeof(XLOPER12));
    REQUIRE(sizeof(xll::Optional<xll::Bool>)   == sizeof(XLOPER12));
}

TEST_CASE("Optional - disengaged xltype is xltypeNil", "[xll::Optional][layout]")
{
    xll::Optional<xll::Number> opt;
    REQUIRE((opt.xltype & ~(xlbitDLLFree | xlbitXLFree)) == xltypeNil);
}

TEST_CASE("Optional - engaged xltype matches TValue", "[xll::Optional][layout]")
{
    xll::Optional<xll::Number> opt = 1.0;
    REQUIRE((opt.xltype & ~(xlbitDLLFree | xlbitXLFree)) == xltypeNum);
}

// =============================================================================
// STRING OPTIONAL (exercises non-trivial destructor path)
// =============================================================================

TEST_CASE("Optional - String value copy/move", "[xll::Optional][string]")
{
    xll::Optional<xll::String> a = xll::String("world");
    REQUIRE(a.has_value());

    xll::Optional<xll::String> b = a;
    REQUIRE(b.has_value());

    xll::Optional<xll::String> c = std::move(a);
    REQUIRE(c.has_value());
}

TEST_CASE("Optional - String reset", "[xll::Optional][string]")
{
    xll::Optional<xll::String> opt = xll::String("test");
    REQUIRE(opt.has_value());
    opt.reset();
    REQUIRE_FALSE(opt.has_value());
}

// =============================================================================
// ITERATOR / RANGE TESTS  (C++26-style single-element range)
// =============================================================================

#include <algorithm>
#include <ranges>
#include <vector>


