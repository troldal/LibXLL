// Tests for xll::Any and xll::cast

#include "catch_amalgamated.hpp"
#include <xlcall.hpp>
#include "../Types/Any.hpp"

// =============================================================================
// LAYOUT / STATIC ASSERTIONS
// =============================================================================

TEST_CASE("Any - sizeof equals XLOPER12", "[xll::Any][layout]")
{
    STATIC_REQUIRE(sizeof(xll::Any)         == sizeof(XLOPER12));
    // STATIC_REQUIRE(sizeof(xll::AnyOptional) == sizeof(XLOPER12));
}

TEST_CASE("Any - is_base_of XLOPER12", "[xll::Any][layout]")
{
    STATIC_REQUIRE(std::is_base_of_v<XLOPER12, xll::Any>);
    // STATIC_REQUIRE(std::is_base_of_v<XLOPER12, xll::AnyOptional>);
}


// =============================================================================
// DEFAULT CONSTRUCTION
// =============================================================================

TEST_CASE("Any - default construction holds Nil", "[xll::Any][construction]")
{
    xll::Any any;
    REQUIRE(any.type()  == xltypeNil);
    REQUIRE(any.empty());
    REQUIRE(xll::holds<xll::Nil>(any));
}


// =============================================================================
// CONSTRUCTION FROM XLL TYPES
// =============================================================================

TEST_CASE("Any - construct from Number (copy)", "[xll::Any][construction]")
{
    xll::Number n{3.14};
    xll::Any any{n};
    REQUIRE(any.type() == xltypeNum);
    REQUIRE(xll::holds<xll::Number>(any));
    REQUIRE_FALSE(xll::holds<xll::String>(any));
    REQUIRE_FALSE(any.empty());
}

TEST_CASE("Any - construct from Number (move)", "[xll::Any][construction]")
{
    xll::Any any{xll::Number(2.71828)};
    REQUIRE(any.type() == xltypeNum);
    REQUIRE(xll::holds<xll::Number>(any));
}

TEST_CASE("Any - construct from Int", "[xll::Any][construction]")
{
    xll::Any any{xll::Int(42)};
    REQUIRE(any.type() == xltypeInt);
    REQUIRE(xll::holds<xll::Int>(any));
}

TEST_CASE("Any - construct from Bool", "[xll::Any][construction]")
{
    xll::Any any{xll::Bool(true)};
    REQUIRE(any.type() == xltypeBool);
    REQUIRE(xll::holds<xll::Bool>(any));
}

TEST_CASE("Any - construct from Error", "[xll::Any][construction]")
{
    xll::Any any{xll::ErrDiv0};
    REQUIRE(any.type() == xltypeErr);
    REQUIRE(xll::holds<xll::Error>(any));
}

TEST_CASE("Any - construct from String performs deep copy", "[xll::Any][construction]")
{
    xll::String s{"hello"};
    xll::Any any{s};
    REQUIRE(any.type() == xltypeStr);
    REQUIRE(xll::holds<xll::String>(any));
    // Deep copy: the string buffer pointers must differ
    REQUIRE(any.val.str != s.val.str);
}

TEST_CASE("Any - construct from String (move)", "[xll::Any][construction]")
{
    xll::Any any{xll::String("world")};
    REQUIRE(any.type() == xltypeStr);
    REQUIRE(xll::holds<xll::String>(any));
}

TEST_CASE("Any - construct from Nil", "[xll::Any][construction]")
{
    xll::Any any{xll::Nil{}};
    REQUIRE(any.type() == xltypeNil);
    REQUIRE(any.empty());
}

TEST_CASE("Any - construct from Missing", "[xll::Any][construction]")
{
    xll::Any any{xll::Missing{}};
    REQUIRE(any.type() == xltypeMissing);
    REQUIRE(xll::holds<xll::Missing>(any));
}

TEST_CASE("Any - construct from raw XLOPER12 (shallow copy)", "[xll::Any][construction]")
{
    XLOPER12 raw{};
    raw.xltype  = xltypeNum;
    raw.val.num = 6.28;

    xll::Any any{raw};
    REQUIRE(any.type() == xltypeNum);
    REQUIRE(xll::holds<xll::Number>(any));
}

// =============================================================================
// COPY / MOVE CONSTRUCTION
// =============================================================================

TEST_CASE("Any - copy constructor (Number)", "[xll::Any][copy]")
{
    xll::Any a{xll::Number(1.0)};
    xll::Any b{a};
    REQUIRE(b.type() == xltypeNum);
    REQUIRE(xll::holds<xll::Number>(b));
}

TEST_CASE("Any - copy constructor (String) is deep", "[xll::Any][copy]")
{
    xll::Any a{xll::String("deep")};
    xll::Any b{a};
    REQUIRE(b.type() == xltypeStr);
    REQUIRE(a.val.str != b.val.str);  // different allocations
}

TEST_CASE("Any - copy constructor (Nil)", "[xll::Any][copy]")
{
    xll::Any a;
    xll::Any b{a};
    REQUIRE(b.empty());
}

TEST_CASE("Any - copy constructor (Error)", "[xll::Any][copy]")
{
    xll::Any a{xll::ErrNA};
    xll::Any b{a};
    REQUIRE(xll::holds<xll::Error>(b));
}

TEST_CASE("Any - copy constructor (Bool)", "[xll::Any][copy]")
{
    xll::Any a{xll::Bool(false)};
    xll::Any b{a};
    REQUIRE(xll::holds<xll::Bool>(b));
}

TEST_CASE("Any - copy constructor (Int)", "[xll::Any][copy]")
{
    xll::Any a{xll::Int(7)};
    xll::Any b{a};
    REQUIRE(xll::holds<xll::Int>(b));
}

TEST_CASE("Any - copy constructor (Missing)", "[xll::Any][copy]")
{
    xll::Any a{xll::Missing{}};
    xll::Any b{a};
    REQUIRE(xll::holds<xll::Missing>(b));
}

TEST_CASE("Any - move constructor (Number)", "[xll::Any][move]")
{
    xll::Any a{xll::Number(9.81)};
    xll::Any b{std::move(a)};
    REQUIRE(xll::holds<xll::Number>(b));
}

TEST_CASE("Any - move constructor (String)", "[xll::Any][move]")
{
    xll::Any a{xll::String("move me")};
    xll::Any b{std::move(a)};
    REQUIRE(xll::holds<xll::String>(b));
}

// =============================================================================
// ASSIGNMENT OPERATORS
// =============================================================================

TEST_CASE("Any - copy assignment (Number)", "[xll::Any][assignment]")
{
    xll::Any a{xll::Number(5.0)};
    xll::Any b;
    b = a;
    REQUIRE(xll::holds<xll::Number>(b));
}

TEST_CASE("Any - copy assignment (String) is deep", "[xll::Any][assignment]")
{
    xll::Any a{xll::String("assign")};
    xll::Any b;
    b = a;
    REQUIRE(xll::holds<xll::String>(b));
    REQUIRE(a.val.str != b.val.str);
}

TEST_CASE("Any - copy assignment replaces existing value", "[xll::Any][assignment]")
{
    xll::Any a{xll::String("new")};
    xll::Any b{xll::Number(1.0)};
    b = a;
    REQUIRE(xll::holds<xll::String>(b));
    REQUIRE_FALSE(xll::holds<xll::Number>(b));
}

TEST_CASE("Any - self copy assignment", "[xll::Any][assignment]")
{
    xll::Any a{xll::Number(7.0)};
    a = a;  // NOLINT
    REQUIRE(xll::holds<xll::Number>(a));
}

TEST_CASE("Any - move assignment", "[xll::Any][assignment]")
{
    xll::Any a{xll::Number(3.0)};
    xll::Any b;
    b = std::move(a);
    REQUIRE(xll::holds<xll::Number>(b));
}

TEST_CASE("Any - assign from xll type (copy)", "[xll::Any][assignment]")
{
    xll::Any any;
    any = xll::Number(42.0);
    REQUIRE(xll::holds<xll::Number>(any));
}

TEST_CASE("Any - assign from xll type (move)", "[xll::Any][assignment]")
{
    xll::Any any;
    any = xll::String("moved");
    REQUIRE(xll::holds<xll::String>(any));
}

TEST_CASE("Any - re-assign changes type", "[xll::Any][assignment]")
{
    xll::Any any{xll::Number(1.0)};
    REQUIRE(xll::holds<xll::Number>(any));

    any = xll::String("now a string");
    REQUIRE(xll::holds<xll::String>(any));
    REQUIRE_FALSE(xll::holds<xll::Number>(any));
}

// =============================================================================
// SWAP
// =============================================================================

TEST_CASE("Any - member swap", "[xll::Any][swap]")
{
    xll::Any a{xll::Number(1.0)};
    xll::Any b{xll::String("str")};
    a.swap(b);
    REQUIRE(xll::holds<xll::String>(a));
    REQUIRE(xll::holds<xll::Number>(b));
}

TEST_CASE("Any - non-member swap (ADL)", "[xll::Any][swap]")
{
    xll::Any a{xll::Bool(true)};
    xll::Any b{xll::Int(10)};
    using std::swap;
    swap(a, b);
    REQUIRE(xll::holds<xll::Int>(a));
    REQUIRE(xll::holds<xll::Bool>(b));
}

// =============================================================================
// holds() and type() queries
// =============================================================================

TEST_CASE("Any - holds<T> and type() queries", "[xll::Any][holds]")
{
    SECTION("Number") {
        xll::Any a{xll::Number(1.0)};
        REQUIRE( xll::holds<xll::Number>(a));
        REQUIRE( a.type() == xltypeNum);
        REQUIRE_FALSE(xll::holds<xll::Int>(a));
        REQUIRE_FALSE(xll::holds<xll::String>(a));
        REQUIRE_FALSE(xll::holds<xll::Bool>(a));
        REQUIRE_FALSE(xll::holds<xll::Error>(a));
        REQUIRE_FALSE(xll::holds<xll::Nil>(a));
        REQUIRE_FALSE(xll::holds<xll::Missing>(a));
    }
    SECTION("String") {
        xll::Any a{xll::String("x")};
        REQUIRE( xll::holds<xll::String>(a));
        REQUIRE( a.type() == xltypeStr);
        REQUIRE_FALSE(xll::holds<xll::Number>(a));
    }
    SECTION("Int") {
        xll::Any a{xll::Int(0)};
        REQUIRE( xll::holds<xll::Int>(a));
        REQUIRE( a.type() == xltypeInt);
        REQUIRE_FALSE(xll::holds<xll::Number>(a));
    }
    SECTION("Bool") {
        xll::Any a{xll::Bool(false)};
        REQUIRE( xll::holds<xll::Bool>(a));
        REQUIRE( a.type() == xltypeBool);
    }
    SECTION("Error") {
        xll::Any a{xll::ErrNull};
        REQUIRE( xll::holds<xll::Error>(a));
        REQUIRE( a.type() == xltypeErr);
    }
    SECTION("Nil (default)") {
        xll::Any a;
        REQUIRE( xll::holds<xll::Nil>(a));
        REQUIRE( a.empty());
        REQUIRE( a.type() == xltypeNil);
    }
    SECTION("Missing") {
        xll::Any a{xll::Missing{}};
        REQUIRE( xll::holds<xll::Missing>(a));
        REQUIRE( a.type() == xltypeMissing);
    }
}

// =============================================================================
// xll::cast — always returns Optional<TTarget>
// =============================================================================

TEST_CASE("cast<Number> - correct type returns engaged Optional", "[xll::cast]")
{
    xll::Any any{xll::Number(2.71828)};
    auto result = xll::cast<xll::Number>(any);

    STATIC_REQUIRE(std::same_as<decltype(result), xll::Optional<xll::Number>>);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == Catch::Approx(2.71828));
}

TEST_CASE("cast<Number> - wrong type returns None", "[xll::cast]")
{
    xll::Any any{xll::String("nope")};
    auto result = xll::cast<xll::Number>(any);

    STATIC_REQUIRE(std::same_as<decltype(result), xll::Optional<xll::Number>>);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result == xll::None);
}

TEST_CASE("cast<String> - correct type", "[xll::cast]")
{
    xll::Any any{xll::String("opt")};
    auto result = xll::cast<xll::String>(any);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == xll::String("opt"));
}

TEST_CASE("cast<String> - wrong type yields None", "[xll::cast]")
{
    xll::Any any{xll::Int(5)};
    auto result = xll::cast<xll::String>(any);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result == xll::None);
}

TEST_CASE("cast<Int> - correct type", "[xll::cast]")
{
    xll::Any any{xll::Int(99)};
    auto result = xll::cast<xll::Int>(any);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == 99);
}

TEST_CASE("cast<Int> - wrong type yields None", "[xll::cast]")
{
    xll::Any any{xll::Bool(false)};
    auto result = xll::cast<xll::Int>(any);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result == xll::None);
}

TEST_CASE("cast<Bool> - correct type", "[xll::cast]")
{
    xll::Any any{xll::Bool(true)};
    auto result = xll::cast<xll::Bool>(any);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == true);
}

TEST_CASE("cast<Bool> - wrong type yields None", "[xll::cast]")
{
    xll::Any any{xll::Number(0.0)};
    auto result = xll::cast<xll::Bool>(any);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result == xll::None);
}

TEST_CASE("cast<Error> - correct type", "[xll::cast]")
{
    xll::Any any{xll::ErrDiv0};
    auto result = xll::cast<xll::Error>(any);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == xll::ErrDiv0);
}

TEST_CASE("cast<Error> - wrong type yields None", "[xll::cast]")
{
    xll::Any any{xll::Number(1.0)};
    auto result = xll::cast<xll::Error>(any);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result == xll::None);
}

TEST_CASE("cast<Nil> - correct type", "[xll::cast]")
{
    xll::Any any{xll::Nil{}};
    auto result = xll::cast<xll::Nil>(any);
    REQUIRE(result.has_value());
}

TEST_CASE("cast<Nil> - wrong type yields None", "[xll::cast]")
{
    xll::Any any{xll::Number(1.0)};
    auto result = xll::cast<xll::Nil>(any);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result == xll::None);
}

TEST_CASE("cast<Missing> - correct type", "[xll::cast]")
{
    xll::Any any{xll::Missing{}};
    auto result = xll::cast<xll::Missing>(any);
    REQUIRE(result.has_value());
}

TEST_CASE("cast<Missing> - default Any (Nil) yields None", "[xll::cast]")
{
    xll::Any any;  // holds Nil
    auto result = xll::cast<xll::Missing>(any);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result == xll::None);
}

TEST_CASE("cast<Number> - all other types yield None", "[xll::cast]")
{
    SECTION("from Int")     { REQUIRE_FALSE(xll::cast<xll::Number>(xll::Any{xll::Int(1)}).has_value()); }
    SECTION("from Bool")    { REQUIRE_FALSE(xll::cast<xll::Number>(xll::Any{xll::Bool(true)}).has_value()); }
    SECTION("from Error")   { REQUIRE_FALSE(xll::cast<xll::Number>(xll::Any{xll::ErrNull}).has_value()); }
    SECTION("from Nil")     { REQUIRE_FALSE(xll::cast<xll::Number>(xll::Any{}).has_value()); }
    SECTION("from Missing") { REQUIRE_FALSE(xll::cast<xll::Number>(xll::Any{xll::Missing{}}).has_value()); }
}

// =============================================================================
// Monadic chaining after cast
// =============================================================================

TEST_CASE("cast - monadic transform", "[xll::cast][monadic]")
{
    xll::Any any{xll::Number(3.0)};

    auto result = xll::cast<xll::Number>(any)
        .transform([](xll::Number n) { return xll::Number(n.val.num * n.val.num); });

    REQUIRE(result.has_value());
    REQUIRE(result.value() == 9.0);
}

TEST_CASE("cast - None propagates through transform chain", "[xll::cast][monadic]")
{
    xll::Any any{xll::String("nope")};

    int calls = 0;
    auto result = xll::cast<xll::Number>(any)
        .transform([&](xll::Number n) { ++calls; return n; });

    REQUIRE_FALSE(result.has_value());
    REQUIRE(calls == 0);
}

TEST_CASE("cast - or_else provides fallback on type mismatch", "[xll::cast][monadic]")
{
    xll::Any any{xll::String("nope")};

    auto result = xll::cast<xll::Number>(any)
        .or_else([]() -> xll::Optional<xll::Number> { return xll::Number(99.0); });

    REQUIRE(result.has_value());
    REQUIRE(result.value() == 99.0);
}

// =============================================================================
// Value semantics — cast result is a copy, not an alias
// =============================================================================

TEST_CASE("cast - result is a copy, not an alias", "[xll::cast][value_semantics]")
{
    xll::Any any{xll::Number(1.0)};
    auto r = xll::cast<xll::Number>(any);
    r.value() = xll::Number(999.0);  // mutate the copy

    // Re-cast: original Any is unchanged
    auto r2 = xll::cast<xll::Number>(any);
    REQUIRE(r2.value() == 1.0);
}

TEST_CASE("cast - String result is a deep copy", "[xll::cast][value_semantics]")
{
    xll::Any any{xll::String("original")};
    auto r = xll::cast<xll::String>(any);
    REQUIRE(r.has_value());
    REQUIRE(r.value().val.str != any.val.str);
}

// =============================================================================
// Multiple casts on the same Any
// =============================================================================

TEST_CASE("Any - multiple casts succeed independently", "[xll::cast]")
{
    xll::Any any{xll::Number(2.0)};

    auto r1 = xll::cast<xll::Number>(any);
    auto r2 = xll::cast<xll::Number>(any);
    auto r3 = xll::cast<xll::String>(any);

    REQUIRE(r1.has_value());
    REQUIRE(r2.has_value());
    REQUIRE_FALSE(r3.has_value());
    REQUIRE(r1.value() == 2.0);
    REQUIRE(r2.value() == 2.0);
}

// =============================================================================
// Type alias
// =============================================================================

TEST_CASE("cast returns Optional<TTarget>", "[xll::Any][aliases]")
{
    xll::Any a{xll::Number(1.0)};
    auto r = xll::cast<xll::Number>(a);
    STATIC_REQUIRE(std::same_as<decltype(r), xll::Optional<xll::Number>>);
    REQUIRE(r.has_value());
}

TEST_CASE("failed cast returns None", "[xll::Any][aliases]")
{
    xll::Any a{xll::Number(1.0)};
    auto r = xll::cast<xll::String>(a);
    STATIC_REQUIRE(std::same_as<decltype(r), xll::Optional<xll::String>>);
    REQUIRE_FALSE(r.has_value());
    REQUIRE(r == xll::None);
}

// =============================================================================
// XLOPER12 raw construction
// =============================================================================

TEST_CASE("Any - construct from raw XLOPER12 numeric", "[xll::Any][construction]")
{
    XLOPER12 raw{};
    raw.xltype  = xltypeNum;
    raw.val.num = 6.28;

    xll::Any any{raw};
    REQUIRE(any.type() == xltypeNum);

    auto r = xll::cast<xll::Number>(any);
    REQUIRE(r.has_value());
    REQUIRE(r.value() == Catch::Approx(6.28));
}

// =============================================================================
// Error-specific cast values
// =============================================================================

TEST_CASE("cast<Error> preserves error code", "[xll::cast]")
{
    const xll::Error errors[] = { xll::ErrNull, xll::ErrDiv0, xll::ErrValue,
                                   xll::ErrRef,  xll::ErrName, xll::ErrNum, xll::ErrNA };
    for (auto& e : errors) {
        xll::Any any{e};
        auto r = xll::cast<xll::Error>(any);
        REQUIRE(r.has_value());
        REQUIRE(r.value() == e);
    }
}



