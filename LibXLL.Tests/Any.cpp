// Tests for xll::Any, xll::cast, xll::ExpectedPolicy, xll::OptionalPolicy

#include "catch_amalgamated.hpp"
#include <xlcall.hpp>
#include "../Types/Any.hpp"

// =============================================================================
// LAYOUT / STATIC ASSERTIONS
// =============================================================================

TEST_CASE("Any - sizeof equals XLOPER12", "[xll::Any][layout]")
{
    STATIC_REQUIRE(sizeof(xll::Any<>)                    == sizeof(XLOPER12));
    STATIC_REQUIRE(sizeof(xll::Any<xll::ExpectedPolicy>) == sizeof(XLOPER12));
    STATIC_REQUIRE(sizeof(xll::Any<xll::OptionalPolicy>) == sizeof(XLOPER12));
    STATIC_REQUIRE(sizeof(xll::AnyExpected)              == sizeof(XLOPER12));
    STATIC_REQUIRE(sizeof(xll::AnyOptional)              == sizeof(XLOPER12));
}

TEST_CASE("Any - is_base_of XLOPER12", "[xll::Any][layout]")
{
    STATIC_REQUIRE(std::is_base_of_v<XLOPER12, xll::Any<>>);
    STATIC_REQUIRE(std::is_base_of_v<XLOPER12, xll::AnyOptional>);
}

TEST_CASE("Any - policy_type aliases", "[xll::Any][layout]")
{
    STATIC_REQUIRE(std::same_as<xll::Any<>::policy_type,            xll::OptionalPolicy>);
    STATIC_REQUIRE(std::same_as<xll::AnyExpected::policy_type,      xll::ExpectedPolicy>);
    STATIC_REQUIRE(std::same_as<xll::AnyOptional::policy_type,      xll::OptionalPolicy>);
}

// =============================================================================
// DEFAULT CONSTRUCTION
// =============================================================================

TEST_CASE("Any - default construction holds Nil", "[xll::Any][construction]")
{
    xll::Any<> any;
    REQUIRE(any.type()  == xltypeNil);
    REQUIRE(any.empty());
    REQUIRE(any.holds<xll::Nil>());
}

TEST_CASE("Any(OptionalPolicy) - default construction holds Nil", "[xll::Any][construction]")
{
    xll::Any<xll::OptionalPolicy> any;
    REQUIRE(any.type() == xltypeNil);
    REQUIRE(any.empty());
}

// =============================================================================
// CONSTRUCTION FROM XLL TYPES
// =============================================================================

TEST_CASE("Any - construct from Number (copy)", "[xll::Any][construction]")
{
    xll::Number n{3.14};
    xll::Any<> any{n};
    REQUIRE(any.type() == xltypeNum);
    REQUIRE(any.holds<xll::Number>());
    REQUIRE_FALSE(any.holds<xll::String>());
    REQUIRE_FALSE(any.empty());
}

TEST_CASE("Any - construct from Number (move)", "[xll::Any][construction]")
{
    xll::Any<> any{xll::Number(2.71828)};
    REQUIRE(any.type() == xltypeNum);
    REQUIRE(any.holds<xll::Number>());
}

TEST_CASE("Any - construct from Int", "[xll::Any][construction]")
{
    xll::Any<> any{xll::Int(42)};
    REQUIRE(any.type() == xltypeInt);
    REQUIRE(any.holds<xll::Int>());
}

TEST_CASE("Any - construct from Bool", "[xll::Any][construction]")
{
    xll::Any<> any{xll::Bool(true)};
    REQUIRE(any.type() == xltypeBool);
    REQUIRE(any.holds<xll::Bool>());
}

TEST_CASE("Any - construct from Error", "[xll::Any][construction]")
{
    xll::Any<> any{xll::ErrDiv0};
    REQUIRE(any.type() == xltypeErr);
    REQUIRE(any.holds<xll::Error>());
}

TEST_CASE("Any - construct from String performs deep copy", "[xll::Any][construction]")
{
    xll::String s{"hello"};
    xll::Any<> any{s};
    REQUIRE(any.type() == xltypeStr);
    REQUIRE(any.holds<xll::String>());
    // Deep copy: the string buffer pointers must differ
    REQUIRE(any.val.str != s.val.str);
}

TEST_CASE("Any - construct from String (move)", "[xll::Any][construction]")
{
    xll::Any<> any{xll::String("world")};
    REQUIRE(any.type() == xltypeStr);
    REQUIRE(any.holds<xll::String>());
}

TEST_CASE("Any - construct from Nil", "[xll::Any][construction]")
{
    xll::Any<> any{xll::Nil{}};
    REQUIRE(any.type() == xltypeNil);
    REQUIRE(any.empty());
}

TEST_CASE("Any - construct from Missing", "[xll::Any][construction]")
{
    xll::Any<> any{xll::Missing{}};
    REQUIRE(any.type() == xltypeMissing);
    REQUIRE(any.holds<xll::Missing>());
}

TEST_CASE("Any - construct from raw XLOPER12 (shallow copy)", "[xll::Any][construction]")
{
    XLOPER12 raw{};
    raw.xltype  = xltypeNum;
    raw.val.num = 6.28;

    xll::Any<> any{raw};
    REQUIRE(any.type() == xltypeNum);
    REQUIRE(any.holds<xll::Number>());
}

// =============================================================================
// COPY / MOVE CONSTRUCTION
// =============================================================================

TEST_CASE("Any - copy constructor (Number)", "[xll::Any][copy]")
{
    xll::Any<> a{xll::Number(1.0)};
    xll::Any<> b{a};
    REQUIRE(b.type() == xltypeNum);
    REQUIRE(b.holds<xll::Number>());
}

TEST_CASE("Any - copy constructor (String) is deep", "[xll::Any][copy]")
{
    xll::Any<> a{xll::String("deep")};
    xll::Any<> b{a};
    REQUIRE(b.type() == xltypeStr);
    REQUIRE(a.val.str != b.val.str);  // different allocations
}

TEST_CASE("Any - copy constructor (Nil)", "[xll::Any][copy]")
{
    xll::Any<> a;
    xll::Any<> b{a};
    REQUIRE(b.empty());
}

TEST_CASE("Any - copy constructor (Error)", "[xll::Any][copy]")
{
    xll::Any<> a{xll::ErrNA};
    xll::Any<> b{a};
    REQUIRE(b.holds<xll::Error>());
}

TEST_CASE("Any - copy constructor (Bool)", "[xll::Any][copy]")
{
    xll::Any<> a{xll::Bool(false)};
    xll::Any<> b{a};
    REQUIRE(b.holds<xll::Bool>());
}

TEST_CASE("Any - copy constructor (Int)", "[xll::Any][copy]")
{
    xll::Any<> a{xll::Int(7)};
    xll::Any<> b{a};
    REQUIRE(b.holds<xll::Int>());
}

TEST_CASE("Any - copy constructor (Missing)", "[xll::Any][copy]")
{
    xll::Any<> a{xll::Missing{}};
    xll::Any<> b{a};
    REQUIRE(b.holds<xll::Missing>());
}

TEST_CASE("Any - move constructor (Number)", "[xll::Any][move]")
{
    xll::Any<> a{xll::Number(9.81)};
    xll::Any<> b{std::move(a)};
    REQUIRE(b.holds<xll::Number>());
}

TEST_CASE("Any - move constructor (String)", "[xll::Any][move]")
{
    xll::Any<> a{xll::String("move me")};
    xll::Any<> b{std::move(a)};
    REQUIRE(b.holds<xll::String>());
}

// =============================================================================
// ASSIGNMENT OPERATORS
// =============================================================================

TEST_CASE("Any - copy assignment (Number)", "[xll::Any][assignment]")
{
    xll::Any<> a{xll::Number(5.0)};
    xll::Any<> b;
    b = a;
    REQUIRE(b.holds<xll::Number>());
}

TEST_CASE("Any - copy assignment (String) is deep", "[xll::Any][assignment]")
{
    xll::Any<> a{xll::String("assign")};
    xll::Any<> b;
    b = a;
    REQUIRE(b.holds<xll::String>());
    REQUIRE(a.val.str != b.val.str);
}

TEST_CASE("Any - copy assignment replaces existing value", "[xll::Any][assignment]")
{
    xll::Any<> a{xll::String("new")};
    xll::Any<> b{xll::Number(1.0)};
    b = a;
    REQUIRE(b.holds<xll::String>());
    REQUIRE_FALSE(b.holds<xll::Number>());
}

TEST_CASE("Any - self copy assignment", "[xll::Any][assignment]")
{
    xll::Any<> a{xll::Number(7.0)};
    a = a;  // NOLINT
    REQUIRE(a.holds<xll::Number>());
}

TEST_CASE("Any - move assignment", "[xll::Any][assignment]")
{
    xll::Any<> a{xll::Number(3.0)};
    xll::Any<> b;
    b = std::move(a);
    REQUIRE(b.holds<xll::Number>());
}

TEST_CASE("Any - assign from xll type (copy)", "[xll::Any][assignment]")
{
    xll::Any<> any;
    any = xll::Number(42.0);
    REQUIRE(any.holds<xll::Number>());
}

TEST_CASE("Any - assign from xll type (move)", "[xll::Any][assignment]")
{
    xll::Any<> any;
    any = xll::String("moved");
    REQUIRE(any.holds<xll::String>());
}

TEST_CASE("Any - re-assign changes type", "[xll::Any][assignment]")
{
    xll::Any<> any{xll::Number(1.0)};
    REQUIRE(any.holds<xll::Number>());

    any = xll::String("now a string");
    REQUIRE(any.holds<xll::String>());
    REQUIRE_FALSE(any.holds<xll::Number>());
}

// =============================================================================
// SWAP
// =============================================================================

TEST_CASE("Any - member swap", "[xll::Any][swap]")
{
    xll::Any<> a{xll::Number(1.0)};
    xll::Any<> b{xll::String("str")};
    a.swap(b);
    REQUIRE(a.holds<xll::String>());
    REQUIRE(b.holds<xll::Number>());
}

TEST_CASE("Any - non-member swap (ADL)", "[xll::Any][swap]")
{
    xll::Any<> a{xll::Bool(true)};
    xll::Any<> b{xll::Int(10)};
    using std::swap;
    swap(a, b);
    REQUIRE(a.holds<xll::Int>());
    REQUIRE(b.holds<xll::Bool>());
}

// =============================================================================
// holds() and type() queries
// =============================================================================

TEST_CASE("Any - holds<T> and type() queries", "[xll::Any][holds]")
{
    SECTION("Number") {
        xll::Any<> a{xll::Number(1.0)};
        REQUIRE( a.holds<xll::Number>());
        REQUIRE( a.type() == xltypeNum);
        REQUIRE_FALSE(a.holds<xll::Int>());
        REQUIRE_FALSE(a.holds<xll::String>());
        REQUIRE_FALSE(a.holds<xll::Bool>());
        REQUIRE_FALSE(a.holds<xll::Error>());
        REQUIRE_FALSE(a.holds<xll::Nil>());
        REQUIRE_FALSE(a.holds<xll::Missing>());
    }
    SECTION("String") {
        xll::Any<> a{xll::String("x")};
        REQUIRE( a.holds<xll::String>());
        REQUIRE( a.type() == xltypeStr);
        REQUIRE_FALSE(a.holds<xll::Number>());
    }
    SECTION("Int") {
        xll::Any<> a{xll::Int(0)};
        REQUIRE( a.holds<xll::Int>());
        REQUIRE( a.type() == xltypeInt);
        REQUIRE_FALSE(a.holds<xll::Number>());
    }
    SECTION("Bool") {
        xll::Any<> a{xll::Bool(false)};
        REQUIRE( a.holds<xll::Bool>());
        REQUIRE( a.type() == xltypeBool);
    }
    SECTION("Error") {
        xll::Any<> a{xll::ErrNull};
        REQUIRE( a.holds<xll::Error>());
        REQUIRE( a.type() == xltypeErr);
    }
    SECTION("Nil (default)") {
        xll::Any<> a;
        REQUIRE( a.holds<xll::Nil>());
        REQUIRE( a.empty());
        REQUIRE( a.type() == xltypeNil);
    }
    SECTION("Missing") {
        xll::Any<> a{xll::Missing{}};
        REQUIRE( a.holds<xll::Missing>());
        REQUIRE( a.type() == xltypeMissing);
    }
}

// =============================================================================
// xll::cast — ExpectedPolicy (default)
// =============================================================================

TEST_CASE("cast<Number> - ExpectedPolicy, correct type", "[xll::cast][ExpectedPolicy]")
{
    xll::AnyExpected any{xll::Number(3.14)};
    auto result = xll::cast<xll::Number>(any);

    STATIC_REQUIRE(std::same_as<decltype(result), xll::Expected<xll::Number, xll::Error>>);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == 3.14);
}

TEST_CASE("cast<Number> - ExpectedPolicy, wrong type yields ErrValue", "[xll::cast][ExpectedPolicy]")
{
    xll::AnyExpected any{xll::String("not a number")};
    auto result = xll::cast<xll::Number>(any);

    STATIC_REQUIRE(std::same_as<decltype(result), xll::Expected<xll::Number, xll::Error>>);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == xll::ErrValue);
}

TEST_CASE("cast<String> - ExpectedPolicy, correct type", "[xll::cast][ExpectedPolicy]")
{
    xll::AnyExpected any{xll::String("hello")};
    auto result = xll::cast<xll::String>(any);

    REQUIRE(result.has_value());
    REQUIRE(result.value() == xll::String("hello"));
}

TEST_CASE("cast<String> - ExpectedPolicy, wrong type yields ErrValue", "[xll::cast][ExpectedPolicy]")
{
    xll::AnyExpected any{xll::Number(1.0)};
    auto result = xll::cast<xll::String>(any);

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == xll::ErrValue);
}

TEST_CASE("cast<Int> - ExpectedPolicy, correct type", "[xll::cast][ExpectedPolicy]")
{
    xll::AnyExpected any{xll::Int(99)};
    auto result = xll::cast<xll::Int>(any);

    REQUIRE(result.has_value());
    REQUIRE(result.value() == 99);
}

TEST_CASE("cast<Int> - ExpectedPolicy, wrong type", "[xll::cast][ExpectedPolicy]")
{
    xll::AnyExpected any{xll::Bool(false)};
    auto result = xll::cast<xll::Int>(any);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == xll::ErrValue);
}

TEST_CASE("cast<Bool> - ExpectedPolicy, correct type", "[xll::cast][ExpectedPolicy]")
{
    xll::AnyExpected any{xll::Bool(true)};
    auto result = xll::cast<xll::Bool>(any);

    REQUIRE(result.has_value());
    REQUIRE(result.value() == true);
}

TEST_CASE("cast<Bool> - ExpectedPolicy, wrong type", "[xll::cast][ExpectedPolicy]")
{
    xll::AnyExpected any{xll::Number(0.0)};
    auto result = xll::cast<xll::Bool>(any);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == xll::ErrValue);
}

TEST_CASE("cast<Error> - ExpectedPolicy, correct type", "[xll::cast][ExpectedPolicy]")
{
    xll::AnyExpected any{xll::ErrDiv0};
    auto result = xll::cast<xll::Error>(any);

    REQUIRE(result.has_value());
    REQUIRE(result.value() == xll::ErrDiv0);
}

TEST_CASE("cast<Error> - ExpectedPolicy, wrong type", "[xll::cast][ExpectedPolicy]")
{
    xll::AnyExpected any{xll::Number(1.0)};
    auto result = xll::cast<xll::Error>(any);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == xll::ErrValue);
}

TEST_CASE("cast<Nil> - ExpectedPolicy, correct type", "[xll::cast][ExpectedPolicy]")
{
    xll::AnyExpected any{xll::Nil{}};
    auto result = xll::cast<xll::Nil>(any);
    REQUIRE(result.has_value());
}

TEST_CASE("cast<Nil> - ExpectedPolicy, wrong type", "[xll::cast][ExpectedPolicy]")
{
    xll::AnyExpected any{xll::Number(1.0)};
    auto result = xll::cast<xll::Nil>(any);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == xll::ErrValue);
}

TEST_CASE("cast<Missing> - ExpectedPolicy, correct type", "[xll::cast][ExpectedPolicy]")
{
    xll::AnyExpected any{xll::Missing{}};
    auto result = xll::cast<xll::Missing>(any);
    REQUIRE(result.has_value());
}

TEST_CASE("cast<Missing> - ExpectedPolicy, default AnyExpected (Nil) yields ErrValue", "[xll::cast][ExpectedPolicy]")
{
    xll::AnyExpected any;  // holds Nil
    auto result = xll::cast<xll::Missing>(any);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == xll::ErrValue);
}

// =============================================================================
// xll::cast — OptionalPolicy
// =============================================================================

TEST_CASE("cast<Number> - OptionalPolicy, correct type", "[xll::cast][OptionalPolicy]")
{
    xll::Any<xll::OptionalPolicy> any{xll::Number(2.71828)};
    auto result = xll::cast<xll::Number>(any);

    STATIC_REQUIRE(std::same_as<decltype(result), xll::Optional<xll::Number>>);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == Catch::Approx(2.71828));
}

TEST_CASE("cast<Number> - OptionalPolicy, wrong type yields None", "[xll::cast][OptionalPolicy]")
{
    xll::Any<xll::OptionalPolicy> any{xll::String("nope")};
    auto result = xll::cast<xll::Number>(any);

    STATIC_REQUIRE(std::same_as<decltype(result), xll::Optional<xll::Number>>);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result == xll::None);
}

TEST_CASE("cast<String> - OptionalPolicy, correct type", "[xll::cast][OptionalPolicy]")
{
    xll::Any<xll::OptionalPolicy> any{xll::String("opt")};
    auto result = xll::cast<xll::String>(any);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == xll::String("opt"));
}

TEST_CASE("cast<String> - OptionalPolicy, wrong type yields None", "[xll::cast][OptionalPolicy]")
{
    xll::Any<xll::OptionalPolicy> any{xll::Int(5)};
    auto result = xll::cast<xll::String>(any);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result == xll::None);
}

TEST_CASE("cast<Int> - OptionalPolicy, correct type", "[xll::cast][OptionalPolicy]")
{
    xll::Any<xll::OptionalPolicy> any{xll::Int(7)};
    auto result = xll::cast<xll::Int>(any);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == 7);
}

TEST_CASE("cast<Bool> - OptionalPolicy, correct type", "[xll::cast][OptionalPolicy]")
{
    xll::Any<xll::OptionalPolicy> any{xll::Bool(false)};
    auto result = xll::cast<xll::Bool>(any);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == false);
}

TEST_CASE("cast<Error> - OptionalPolicy, correct type", "[xll::cast][OptionalPolicy]")
{
    xll::Any<xll::OptionalPolicy> any{xll::ErrValue};
    auto result = xll::cast<xll::Error>(any);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == xll::ErrValue);
}

TEST_CASE("cast<Nil> - OptionalPolicy, empty Any", "[xll::cast][OptionalPolicy]")
{
    xll::Any<xll::OptionalPolicy> any;
    auto result = xll::cast<xll::Nil>(any);
    REQUIRE(result.has_value());
}

TEST_CASE("cast<Missing> - OptionalPolicy, correct type", "[xll::cast][OptionalPolicy]")
{
    xll::Any<xll::OptionalPolicy> any{xll::Missing{}};
    auto result = xll::cast<xll::Missing>(any);
    REQUIRE(result.has_value());
}

TEST_CASE("cast<Number> - OptionalPolicy, all other types yield None", "[xll::cast][OptionalPolicy]")
{
    SECTION("from Int")     { REQUIRE_FALSE(xll::cast<xll::Number>(xll::Any<xll::OptionalPolicy>{xll::Int(1)}).has_value()); }
    SECTION("from Bool")    { REQUIRE_FALSE(xll::cast<xll::Number>(xll::Any<xll::OptionalPolicy>{xll::Bool(true)}).has_value()); }
    SECTION("from Error")   { REQUIRE_FALSE(xll::cast<xll::Number>(xll::Any<xll::OptionalPolicy>{xll::ErrNull}).has_value()); }
    SECTION("from Nil")     { REQUIRE_FALSE(xll::cast<xll::Number>(xll::Any<xll::OptionalPolicy>{}).has_value()); }
    SECTION("from Missing") { REQUIRE_FALSE(xll::cast<xll::Number>(xll::Any<xll::OptionalPolicy>{xll::Missing{}}).has_value()); }
}

// =============================================================================
// Monadic chaining after cast
// =============================================================================

TEST_CASE("cast - ExpectedPolicy: monadic and_then", "[xll::cast][monadic]")
{
    xll::AnyExpected any{xll::Number(10.0)};

    auto result = xll::cast<xll::Number>(any)
        .and_then([](xll::Number& n) -> xll::Expected<xll::Number, xll::Error> {
            return xll::Number(n.val.num * 2.0);
        });

    REQUIRE(result.has_value());
    REQUIRE(result.value() == 20.0);
}

TEST_CASE("cast - ExpectedPolicy: monadic transform", "[xll::cast][monadic]")
{
    xll::AnyExpected any{xll::Number(5.0)};

    auto result = xll::cast<xll::Number>(any)
        .transform([](xll::Number n) { return xll::Number(n.val.num + 1.0); });

    REQUIRE(result.has_value());
    REQUIRE(result.value() == 6.0);
}

TEST_CASE("cast - ExpectedPolicy: error propagates through chain", "[xll::cast][monadic]")
{
    xll::AnyExpected any{xll::String("not a number")};

    int calls = 0;
    auto result = xll::cast<xll::Number>(any)
        .transform([&](xll::Number n) { ++calls; return n; });

    REQUIRE_FALSE(result.has_value());
    REQUIRE(calls == 0);
    REQUIRE(result.error() == xll::ErrValue);
}

TEST_CASE("cast - ExpectedPolicy: or_else recovers from type mismatch", "[xll::cast][monadic]")
{
    xll::AnyExpected any{xll::String("fallback")};

    auto result = xll::cast<xll::Number>(any)
        .or_else([](xll::Error) -> xll::Expected<xll::Number, xll::Error> {
            return xll::Number(0.0);
        });

    REQUIRE(result.has_value());
    REQUIRE(result.value() == 0.0);
}

TEST_CASE("cast - OptionalPolicy: monadic transform", "[xll::cast][monadic]")
{
    xll::Any<xll::OptionalPolicy> any{xll::Number(3.0)};

    auto result = xll::cast<xll::Number>(any)
        .transform([](xll::Number n) { return xll::Number(n.val.num * n.val.num); });

    REQUIRE(result.has_value());
    REQUIRE(result.value() == 9.0);
}

TEST_CASE("cast - OptionalPolicy: None propagates through chain", "[xll::cast][monadic]")
{
    xll::Any<xll::OptionalPolicy> any{xll::String("nope")};

    int calls = 0;
    auto result = xll::cast<xll::Number>(any)
        .transform([&](xll::Number n) { ++calls; return n; });

    REQUIRE_FALSE(result.has_value());
    REQUIRE(calls == 0);
}

TEST_CASE("cast - OptionalPolicy: or_else provides fallback", "[xll::cast][monadic]")
{
    xll::Any<xll::OptionalPolicy> any{xll::String("nope")};

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
    xll::Any<> any{xll::Number(1.0)};
    // Any<> uses OptionalPolicy — cast returns Optional<Number>
    auto r = xll::cast<xll::Number>(any);
    r.value() = xll::Number(999.0);  // mutate the copy

    // Re-cast: original Any is unchanged
    auto r2 = xll::cast<xll::Number>(any);
    REQUIRE(r2.value() == 1.0);
}

TEST_CASE("cast - String result is a deep copy", "[xll::cast][value_semantics]")
{
    xll::Any<> any{xll::String("original")};
    auto r = xll::cast<xll::String>(any);
    REQUIRE(r.has_value());
    // The returned String's buffer must differ from Any's buffer
    REQUIRE(r.value().val.str != any.val.str);
}

// =============================================================================
// Multiple casts on the same Any
// =============================================================================

TEST_CASE("Any - multiple casts succeed independently", "[xll::cast]")
{
    // Any<> uses OptionalPolicy — cast returns Optional<T>
    xll::Any<> any{xll::Number(2.0)};

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
// Type aliases
// =============================================================================

TEST_CASE("Any<> default - cast returns Optional (OptionalPolicy is default)", "[xll::Any][aliases]")
{
    xll::Any<> a{xll::Number(1.0)};
    auto r = xll::cast<xll::Number>(a);
    STATIC_REQUIRE(std::same_as<decltype(r), xll::Optional<xll::Number>>);
    REQUIRE(r.has_value());
}

TEST_CASE("Any<> default - failed cast returns None (not ErrValue)", "[xll::Any][aliases]")
{
    xll::Any<> a{xll::Number(1.0)};
    auto r = xll::cast<xll::String>(a);
    STATIC_REQUIRE(std::same_as<decltype(r), xll::Optional<xll::String>>);
    REQUIRE_FALSE(r.has_value());
    REQUIRE(r == xll::None);
}

TEST_CASE("AnyExpected type alias", "[xll::Any][aliases]")
{
    xll::AnyExpected a{xll::Number(1.0)};
    auto r = xll::cast<xll::Number>(a);
    STATIC_REQUIRE(std::same_as<decltype(r), xll::Expected<xll::Number, xll::Error>>);
    REQUIRE(r.has_value());
}

TEST_CASE("AnyOptional type alias", "[xll::Any][aliases]")
{
    xll::AnyOptional a{xll::Int(5)};
    auto r = xll::cast<xll::Int>(a);
    STATIC_REQUIRE(std::same_as<decltype(r), xll::Optional<xll::Int>>);
    REQUIRE(r.has_value());
}

// =============================================================================
// XLOPER12 raw construction
// =============================================================================

TEST_CASE("Any - construct from raw XLOPER12 numeric", "[xll::Any][construction]")
{
    XLOPER12 raw{};
    raw.xltype  = xltypeNum;
    raw.val.num = 6.28;

    xll::Any<> any{raw};
    REQUIRE(any.type() == xltypeNum);

    auto r = xll::cast<xll::Number>(any);
    REQUIRE(r.has_value());
    REQUIRE(r.value() == Catch::Approx(6.28));
}

// =============================================================================
// Error-specific cast values
// =============================================================================

TEST_CASE("cast<Error> preserves error code", "[xll::cast][ExpectedPolicy]")
{
    const xll::Error errors[] = { xll::ErrNull, xll::ErrDiv0, xll::ErrValue,
                                   xll::ErrRef,  xll::ErrName, xll::ErrNum, xll::ErrNA };
    for (auto& e : errors) {
        xll::Any<> any{e};
        auto r = xll::cast<xll::Error>(any);
        REQUIRE(r.has_value());
        REQUIRE(r.value() == e);
    }
}



