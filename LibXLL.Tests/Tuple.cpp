// Tests for xll::Tuple and xll::get

#include "catch_amalgamated.hpp"
#include <xlcall.hpp>
#include "../Types/Tuple.hpp"
#include <cmath>

// =============================================================================
// LAYOUT / STATIC ASSERTIONS
// =============================================================================

TEST_CASE("Tuple - sizeof equals XLOPER12", "[xll::Tuple][layout]")
{
    STATIC_REQUIRE(sizeof(xll::Tuple<xll::Number>)                  == sizeof(XLOPER12));
    STATIC_REQUIRE(sizeof(xll::Tuple<xll::Number, xll::String>)     == sizeof(XLOPER12));
    STATIC_REQUIRE(sizeof(xll::Tuple<xll::Number, xll::String,
                                     xll::Bool,   xll::Int>)        == sizeof(XLOPER12));
}

TEST_CASE("Tuple - xltype is xltypeMulti", "[xll::Tuple][layout]")
{
    xll::Tuple<xll::Number, xll::String> t;
    REQUIRE(t.xltype() == xltypeMulti);
}

// =============================================================================
// DEFAULT CONSTRUCTION
// =============================================================================

TEST_CASE("Tuple - default construction initialises all elements to Nil", "[xll::Tuple][construction]")
{
    using T = xll::Tuple<xll::Number, xll::String, xll::Bool>;
    T t;

    // Every position should hold Nil, so get returns None for all declared types.
    REQUIRE_FALSE(xll::get<0>(t).has_value());
    REQUIRE_FALSE(xll::get<1>(t).has_value());
    REQUIRE_FALSE(xll::get<2>(t).has_value());
}

TEST_CASE("Tuple - default construction of single-element tuple", "[xll::Tuple][construction]")
{
    xll::Tuple<xll::Number> t;
    REQUIRE_FALSE(xll::get<0>(t).has_value());
}

// =============================================================================
// INITIALIZER-LIST CONSTRUCTION
// =============================================================================

TEST_CASE("Tuple - initializer-list construction: Number", "[xll::Tuple][construction]")
{
    xll::Tuple<xll::Number> t { xll::Number(42.0) };
    auto n = xll::get<0>(t);
    REQUIRE(n.has_value());
    REQUIRE(n.value() == Catch::Approx(42.0));
}

TEST_CASE("Tuple - initializer-list construction: String", "[xll::Tuple][construction]")
{
    xll::Tuple<xll::String> t { xll::String("hello") };
    auto s = xll::get<0>(t);
    REQUIRE(s.has_value());
    REQUIRE(s.value() == xll::String("hello"));
}

TEST_CASE("Tuple - initializer-list construction: mixed types", "[xll::Tuple][construction]")
{
    using T = xll::Tuple<xll::Number, xll::String, xll::Bool, xll::Int>;
    T t { xll::Number(3.14), xll::String("hi"), xll::Bool(true), xll::Int(7) };

    auto n = xll::get<0>(t);
    auto s = xll::get<1>(t);
    auto b = xll::get<2>(t);
    auto i = xll::get<3>(t);

    REQUIRE(n.has_value());
    REQUIRE(s.has_value());
    REQUIRE(b.has_value());
    REQUIRE(i.has_value());

    REQUIRE(n.value() == Catch::Approx(3.14));
    REQUIRE(s.value() == xll::String("hi"));
    REQUIRE(static_cast<bool>(b.value()) == true);
    REQUIRE(static_cast<int>(i.value()) == 7);
}

TEST_CASE("Tuple - initializer-list wrong size throws out_of_range", "[xll::Tuple][construction]")
{
    using T = xll::Tuple<xll::Number, xll::String>;
    REQUIRE_THROWS_AS((T { xll::Number(1.0) }),                              std::out_of_range);
    REQUIRE_THROWS_AS((T { xll::Number(1.0), xll::String("a"), xll::Bool(true) }), std::out_of_range);
}

// =============================================================================
// COPY / MOVE
// =============================================================================

TEST_CASE("Tuple - copy constructor", "[xll::Tuple][copy_move]")
{
    using T = xll::Tuple<xll::Number, xll::String>;
    T original { xll::Number(1.5), xll::String("copy") };
    T copy { original };

    auto n = xll::get<0>(copy);
    auto s = xll::get<1>(copy);
    REQUIRE(n.has_value());
    REQUIRE(s.has_value());
    REQUIRE(n.value() == Catch::Approx(1.5));
    REQUIRE(s.value() == xll::String("copy"));

    // Deep copy: element buffers must differ.
    REQUIRE(original.data() != copy.data());
}

TEST_CASE("Tuple - copy assignment", "[xll::Tuple][copy_move]")
{
    using T = xll::Tuple<xll::Number, xll::String>;
    T a { xll::Number(2.0), xll::String("a") };
    T b { xll::Number(9.0), xll::String("b") };
    b = a;

    REQUIRE(xll::get<0>(b).value() == Catch::Approx(2.0));
    REQUIRE(xll::get<1>(b).value() == xll::String("a"));
}

TEST_CASE("Tuple - move constructor", "[xll::Tuple][copy_move]")
{
    using T = xll::Tuple<xll::Number, xll::String>;
    T original { xll::Number(7.0), xll::String("move") };
    const auto* old_ptr = original.data();

    T moved { std::move(original) };

    REQUIRE(xll::get<0>(moved).has_value());
    REQUIRE(xll::get<0>(moved).value() == Catch::Approx(7.0));
    REQUIRE(xll::get<1>(moved).value() == xll::String("move"));

    // The buffer was transferred, not copied.
    REQUIRE(moved.data() == old_ptr);
}

TEST_CASE("Tuple - move assignment", "[xll::Tuple][copy_move]")
{
    using T = xll::Tuple<xll::Number, xll::String>;
    T src { xll::Number(5.0), xll::String("src") };
    T dst { xll::Number(0.0), xll::String("dst") };
    const auto* old_ptr = src.data();

    dst = std::move(src);

    REQUIRE(dst.data() == old_ptr);
    REQUIRE(xll::get<0>(dst).value() == Catch::Approx(5.0));
}

// =============================================================================
// get<I> — index-based access
// =============================================================================

TEST_CASE("get<I> - returns correct value for each index", "[xll::Tuple][get_index]")
{
    using T = xll::Tuple<xll::Number, xll::String, xll::Bool, xll::Int, xll::Error>;
    T t { xll::Number(1.0), xll::String("s"), xll::Bool(false), xll::Int(3), xll::ErrDiv0 };

    REQUIRE(xll::get<0>(t).has_value());
    REQUIRE(xll::get<1>(t).has_value());
    REQUIRE(xll::get<2>(t).has_value());
    REQUIRE(xll::get<3>(t).has_value());
    REQUIRE(xll::get<4>(t).has_value());

    REQUIRE(xll::get<0>(t).value() == Catch::Approx(1.0));
    REQUIRE(xll::get<1>(t).value() == xll::String("s"));
    REQUIRE(static_cast<bool>(xll::get<2>(t).value()) == false);
    REQUIRE(static_cast<int>(xll::get<3>(t).value())  == 3);
    REQUIRE(xll::get<4>(t).value()                    == xll::ErrDiv0);
}

TEST_CASE("get<I> - returns None when runtime type mismatches declared type", "[xll::Tuple][get_index]")
{
    using T = xll::Tuple<xll::Number, xll::String>;
    // Provide String where Number is declared.
    T t { xll::String("mismatch"), xll::Number(0.0) };

    REQUIRE_FALSE(xll::get<0>(t).has_value());   // declared Number, holds String
    REQUIRE_FALSE(xll::get<1>(t).has_value());   // declared String, holds Number
    REQUIRE(xll::get<0>(t) == xll::None);
    REQUIRE(xll::get<1>(t) == xll::None);
}

TEST_CASE("get<I> - return type is Optional<declared-type>", "[xll::Tuple][get_index]")
{
    using T = xll::Tuple<xll::Number, xll::String, xll::Bool>;
    T t { xll::Number(1.0), xll::String("x"), xll::Bool(true) };

    STATIC_REQUIRE(std::same_as<decltype(xll::get<0>(t)), xll::Optional<xll::Number>>);
    STATIC_REQUIRE(std::same_as<decltype(xll::get<1>(t)), xll::Optional<xll::String>>);
    STATIC_REQUIRE(std::same_as<decltype(xll::get<2>(t)), xll::Optional<xll::Bool>>);
}

TEST_CASE("get<I> - nil elements after default construction return None", "[xll::Tuple][get_index]")
{
    xll::Tuple<xll::Number, xll::String, xll::Int> t;
    REQUIRE(xll::get<0>(t) == xll::None);
    REQUIRE(xll::get<1>(t) == xll::None);
    REQUIRE(xll::get<2>(t) == xll::None);
}

// =============================================================================
// get<T> — type-based access
// =============================================================================

TEST_CASE("get<T> - returns correct value by type", "[xll::Tuple][get_type]")
{
    using T = xll::Tuple<xll::Number, xll::String, xll::Bool>;
    T t { xll::Number(2.71), xll::String("world"), xll::Bool(true) };

    auto n = xll::get<xll::Number>(t);
    auto s = xll::get<xll::String>(t);
    auto b = xll::get<xll::Bool>(t);

    REQUIRE(n.has_value());
    REQUIRE(s.has_value());
    REQUIRE(b.has_value());

    REQUIRE(n.value() == Catch::Approx(2.71));
    REQUIRE(s.value() == xll::String("world"));
    REQUIRE(static_cast<bool>(b.value()) == true);
}

TEST_CASE("get<T> - return type is Optional<T>", "[xll::Tuple][get_type]")
{
    using T = xll::Tuple<xll::Number, xll::String>;
    T t { xll::Number(1.0), xll::String("x") };

    STATIC_REQUIRE(std::same_as<decltype(xll::get<xll::Number>(t)), xll::Optional<xll::Number>>);
    STATIC_REQUIRE(std::same_as<decltype(xll::get<xll::String>(t)), xll::Optional<xll::String>>);
}

TEST_CASE("get<T> - returns None on runtime type mismatch", "[xll::Tuple][get_type]")
{
    using T = xll::Tuple<xll::Number, xll::String>;
    // Swap: String where Number declared, Number where String declared.
    T t { xll::String("bad"), xll::Number(0.0) };

    REQUIRE_FALSE(xll::get<xll::Number>(t).has_value());
    REQUIRE_FALSE(xll::get<xll::String>(t).has_value());
}

TEST_CASE("get<T> - type not in Ts fails at compile time", "[xll::Tuple][get_type]")
{
    // This is a compile-time-only property verified by comment; STATIC_REQUIRE
    // cannot test for compilation failures, but we verify the constraint exists:
    STATIC_REQUIRE(xll::impl::count_type<xll::Bool,
                       xll::Number, xll::String> == 0);   // Bool not in pack → get<Bool> would fail
    STATIC_REQUIRE(xll::impl::count_type<xll::Number,
                       xll::Number, xll::Number> == 2);   // duplicate → get<Number> would fail
}

TEST_CASE("get<T> - duplicate type in Ts fails at compile time", "[xll::Tuple][get_type]")
{
    // Duplicate type: count_type == 2, so requires clause is unsatisfied.
    STATIC_REQUIRE(xll::impl::count_type<xll::Number,
                       xll::Number, xll::Number> != 1);
}

// =============================================================================
// ALL ERROR TYPES
// =============================================================================

TEST_CASE("Tuple - stores and retrieves all seven Excel error codes", "[xll::Tuple][error]")
{
    using T = xll::Tuple<xll::Error>;
    const xll::Error errors[] = {
        xll::ErrNull, xll::ErrDiv0, xll::ErrValue,
        xll::ErrRef,  xll::ErrName, xll::ErrNum, xll::ErrNA
    };
    for (const auto& e : errors) {
        T t { e };
        auto result = xll::get<0>(t);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == e);
    }
}

// =============================================================================
// MISSING AND NIL ELEMENTS
// =============================================================================

TEST_CASE("Tuple - Missing element round-trips", "[xll::Tuple][missing_nil]")
{
    xll::Tuple<xll::Missing> t { xll::Missing{} };
    auto m = xll::get<0>(t);
    REQUIRE(m.has_value());
}

TEST_CASE("Tuple - Nil element round-trips", "[xll::Tuple][missing_nil]")
{
    xll::Tuple<xll::Nil> t { xll::Nil{} };
    auto n = xll::get<0>(t);
    REQUIRE(n.has_value());
}

// =============================================================================
// MONADIC CHAINING ON get RESULTS
// =============================================================================

TEST_CASE("Tuple - get<I> result supports transform", "[xll::Tuple][monadic]")
{
    xll::Tuple<xll::Number, xll::String> t { xll::Number(4.0), xll::String("x") };

    auto result = xll::get<0>(t)
        .transform([](xll::Number n) { return xll::Number(n.val.num * n.val.num); });

    REQUIRE(result.has_value());
    REQUIRE(result.value() == Catch::Approx(16.0));
}

TEST_CASE("Tuple - get<I> None propagates through transform", "[xll::Tuple][monadic]")
{
    xll::Tuple<xll::Number, xll::String> t { xll::String("wrong"), xll::String("x") };

    int calls = 0;
    auto result = xll::get<0>(t)
        .transform([&](xll::Number n) { ++calls; return n; });

    REQUIRE_FALSE(result.has_value());
    REQUIRE(calls == 0);
}

TEST_CASE("Tuple - get<I> or_else provides fallback on None", "[xll::Tuple][monadic]")
{
    xll::Tuple<xll::Number> t;   // default: Nil → None

    auto result = xll::get<0>(t)
        .or_else([]() -> xll::Optional<xll::Number> { return xll::Number(99.0); });

    REQUIRE(result.has_value());
    REQUIRE(result.value() == Catch::Approx(99.0));
}

TEST_CASE("Tuple - get<T> result supports and_then chain", "[xll::Tuple][monadic]")
{
    using T = xll::Tuple<xll::Number, xll::String>;
    T t { xll::Number(9.0), xll::String("x") };

    auto safe_sqrt = [](xll::Number n) -> xll::Optional<xll::Number> {
        if (n.val.num < 0.0) return xll::None;
        return xll::Number(std::sqrt(n.val.num));
    };

    auto result = xll::get<xll::Number>(t).and_then(safe_sqrt);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == Catch::Approx(3.0));
}

// =============================================================================
// TYPE ALIAS USAGE (mirrors std::tuple idiom)
// =============================================================================

TEST_CASE("Tuple - type alias usage mirrors std::tuple", "[xll::Tuple][alias]")
{
    using Row = xll::Tuple<xll::String, xll::Number, xll::Bool>;

    Row r { xll::String("Alice"), xll::Number(42.0), xll::Bool(true) };

    REQUIRE(xll::get<xll::String>(r).value() == xll::String("Alice"));
    REQUIRE(xll::get<xll::Number>(r).value() == Catch::Approx(42.0));
    REQUIRE(static_cast<bool>(xll::get<xll::Bool>(r).value()) == true);

    // Index access matches type access
    REQUIRE(xll::get<0>(r).value() == xll::get<xll::String>(r).value());
    REQUIRE(xll::get<1>(r).value() == xll::get<xll::Number>(r).value());
}

// =============================================================================
// IMPL HELPERS
// =============================================================================

TEST_CASE("impl::count_type", "[xll::Tuple][impl]")
{
    STATIC_REQUIRE(xll::impl::count_type<xll::Number, xll::Number, xll::String> == 1);
    STATIC_REQUIRE(xll::impl::count_type<xll::Bool,   xll::Number, xll::String> == 0);
    STATIC_REQUIRE(xll::impl::count_type<xll::Number, xll::Number, xll::Number> == 2);
}

TEST_CASE("impl::index_of", "[xll::Tuple][impl]")
{
    STATIC_REQUIRE(xll::impl::index_of<xll::Number, xll::Number, xll::String, xll::Bool> == 0);
    STATIC_REQUIRE(xll::impl::index_of<xll::String, xll::Number, xll::String, xll::Bool> == 1);
    STATIC_REQUIRE(xll::impl::index_of<xll::Bool,   xll::Number, xll::String, xll::Bool> == 2);
}

TEST_CASE("impl::type_at", "[xll::Tuple][impl]")
{
    STATIC_REQUIRE(std::same_as<xll::impl::type_at<0, xll::Number, xll::String, xll::Bool>, xll::Number>);
    STATIC_REQUIRE(std::same_as<xll::impl::type_at<1, xll::Number, xll::String, xll::Bool>, xll::String>);
    STATIC_REQUIRE(std::same_as<xll::impl::type_at<2, xll::Number, xll::String, xll::Bool>, xll::Bool>);
}






