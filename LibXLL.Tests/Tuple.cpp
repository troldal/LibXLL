// Tests for xll::Tuple and xll::get

#include "catch_amalgamated.hpp"
#include <xlcall.hpp>
#include "../Types/Tuple.hpp"
#include "../Types/Any.hpp"
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

    // Every position holds Nil, which mismatches the declared types → throws.
    REQUIRE_THROWS_AS(xll::get<0>(t), std::invalid_argument);
    REQUIRE_THROWS_AS(xll::get<1>(t), std::invalid_argument);
    REQUIRE_THROWS_AS(xll::get<2>(t), std::invalid_argument);
}

TEST_CASE("Tuple - default construction of single-element tuple", "[xll::Tuple][construction]")
{
    xll::Tuple<xll::Number> t;
    REQUIRE_THROWS_AS(xll::get<0>(t), std::invalid_argument);
}

// =============================================================================
// INITIALIZER-LIST CONSTRUCTION
// =============================================================================

TEST_CASE("Tuple - initializer-list construction: Number", "[xll::Tuple][construction]")
{
    xll::Tuple<xll::Number> t { xll::Number(42.0) };
    auto n = xll::get<0>(t);
    REQUIRE(static_cast<double>(n) == Catch::Approx(42.0));
}

TEST_CASE("Tuple - initializer-list construction: String", "[xll::Tuple][construction]")
{
    xll::Tuple<xll::String> t { xll::String("hello") };
    auto s = xll::get<0>(t);
    REQUIRE(s == xll::String("hello"));
}

TEST_CASE("Tuple - initializer-list construction: mixed types", "[xll::Tuple][construction]")
{
    using T = xll::Tuple<xll::Number, xll::String, xll::Bool, xll::Int>;
    T t { xll::Number(3.14), xll::String("hi"), xll::Bool(true), xll::Int(7) };

    REQUIRE(static_cast<double>(xll::get<0>(t)) == Catch::Approx(3.14));
    REQUIRE(xll::get<1>(t)                      == xll::String("hi"));
    REQUIRE(static_cast<bool>(xll::get<2>(t))   == true);
    REQUIRE(static_cast<int>(xll::get<3>(t))    == 7);
}

TEST_CASE("Tuple - initializer-list wrong size throws invalid_argument", "[xll::Tuple][construction]")
{
    using T = xll::Tuple<xll::Number, xll::String>;
    REQUIRE_THROWS_AS((T { xll::Number(1.0) }),                                        std::invalid_argument);
    REQUIRE_THROWS_AS((T { xll::Number(1.0), xll::String("a"), xll::Bool(true) }),     std::invalid_argument);
}

// =============================================================================
// COPY / MOVE
// =============================================================================

TEST_CASE("Tuple - copy constructor", "[xll::Tuple][copy_move]")
{
    using T = xll::Tuple<xll::Number, xll::String>;
    T original { xll::Number(1.5), xll::String("copy") };
    T copy(original);

    REQUIRE(static_cast<double>(xll::get<0>(copy)) == Catch::Approx(1.5));
    REQUIRE(xll::get<1>(copy)                      == xll::String("copy"));

    // Deep copy: element buffers must differ.
    REQUIRE(original.data() != copy.data());
}

TEST_CASE("Tuple - copy assignment", "[xll::Tuple][copy_move]")
{
    using T = xll::Tuple<xll::Number, xll::String>;
    T a { xll::Number(2.0), xll::String("a") };
    T b { xll::Number(9.0), xll::String("b") };
    b = a;

    REQUIRE(static_cast<double>(xll::get<0>(b)) == Catch::Approx(2.0));
    REQUIRE(xll::get<1>(b)                      == xll::String("a"));
}

TEST_CASE("Tuple - move constructor", "[xll::Tuple][copy_move]")
{
    using T = xll::Tuple<xll::Number, xll::String>;
    T original { xll::Number(7.0), xll::String("move") };
    const auto* old_ptr = original.data();

    T moved(std::move(original));

    REQUIRE(static_cast<double>(xll::get<0>(moved)) == Catch::Approx(7.0));
    REQUIRE(xll::get<1>(moved)                      == xll::String("move"));

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
    REQUIRE(static_cast<double>(xll::get<0>(dst)) == Catch::Approx(5.0));
}

// =============================================================================
// valid() and validate()
// =============================================================================

TEST_CASE("Tuple - valid() is true for normally constructed objects", "[xll::Tuple][valid]")
{
    xll::Tuple<xll::Number, xll::String> t { xll::Number(1.0), xll::String("x") };
    REQUIRE(t.valid());
    REQUIRE_NOTHROW(t.validate());
}

TEST_CASE("Tuple - valid() and validate() via cast from Any with correct size", "[xll::Tuple][valid]")
{
    using T = xll::Tuple<xll::Number, xll::String>;
    xll::Any any = T { xll::Number(1.0), xll::String("x") };
    auto opt = xll::cast<T>(any);
    REQUIRE(opt.has_value());
    REQUIRE(opt->valid());
}

TEST_CASE("Tuple - cast returns None from Any with wrong element count", "[xll::Tuple][valid]")
{
    using T = xll::Tuple<xll::Number, xll::String>;
    // Array with only one element — wrong size for T.
    xll::Any any = xll::Array<xll::Any>({ xll::Number(1.0) });
    auto opt = xll::cast<T>(any);
    REQUIRE_FALSE(opt.has_value());
}

TEST_CASE("Tuple - cast returns None from Any with wrong xltype", "[xll::Tuple][valid]")
{
    using T = xll::Tuple<xll::Number, xll::String>;
    xll::Any any = xll::Number(3.14);
    auto opt = xll::cast<T>(any);
    REQUIRE_FALSE(opt.has_value());
}

// =============================================================================
// get<I> — index-based access
// =============================================================================

TEST_CASE("get<I> - returns correct value for each index", "[xll::Tuple][get_index]")
{
    using T = xll::Tuple<xll::Number, xll::String, xll::Bool, xll::Int, xll::Error>;
    T t { xll::Number(1.0), xll::String("s"), xll::Bool(false), xll::Int(3), xll::ErrDiv0 };

    REQUIRE(static_cast<double>(xll::get<0>(t)) == Catch::Approx(1.0));
    REQUIRE(xll::get<1>(t)                      == xll::String("s"));
    REQUIRE(static_cast<bool>(xll::get<2>(t))   == false);
    REQUIRE(static_cast<int>(xll::get<3>(t))    == 3);
    REQUIRE(xll::get<4>(t)                      == xll::ErrDiv0);
}

TEST_CASE("get<I> - throws when runtime type mismatches declared type", "[xll::Tuple][get_index]")
{
    using T = xll::Tuple<xll::Number, xll::String>;
    // Provide String where Number is declared, and vice versa.
    T t { xll::String("mismatch"), xll::Number(0.0) };

    REQUIRE_THROWS_AS(xll::get<0>(t), std::invalid_argument);   // declared Number, holds String
    REQUIRE_THROWS_AS(xll::get<1>(t), std::invalid_argument);   // declared String, holds Number
}

TEST_CASE("get<I> - return type is the declared type (not Optional)", "[xll::Tuple][get_index]")
{
    using T = xll::Tuple<xll::Number, xll::String, xll::Bool>;
    T t { xll::Number(1.0), xll::String("x"), xll::Bool(true) };

    STATIC_REQUIRE(std::same_as<decltype(xll::get<0>(t)), xll::Number>);
    STATIC_REQUIRE(std::same_as<decltype(xll::get<1>(t)), xll::String>);
    STATIC_REQUIRE(std::same_as<decltype(xll::get<2>(t)), xll::Bool>);
}

TEST_CASE("get<I> - throws for nil elements after default construction", "[xll::Tuple][get_index]")
{
    xll::Tuple<xll::Number, xll::String, xll::Int> t;
    REQUIRE_THROWS_AS(xll::get<0>(t), std::invalid_argument);
    REQUIRE_THROWS_AS(xll::get<1>(t), std::invalid_argument);
    REQUIRE_THROWS_AS(xll::get<2>(t), std::invalid_argument);
}

// =============================================================================
// get<T> — type-based access
// =============================================================================

TEST_CASE("get<T> - returns correct value by type", "[xll::Tuple][get_type]")
{
    using T = xll::Tuple<xll::Number, xll::String, xll::Bool>;
    T t { xll::Number(2.71), xll::String("world"), xll::Bool(true) };

    REQUIRE(static_cast<double>(xll::get<xll::Number>(t)) == Catch::Approx(2.71));
    REQUIRE(xll::get<xll::String>(t)                      == xll::String("world"));
    REQUIRE(static_cast<bool>(xll::get<xll::Bool>(t))     == true);
}

TEST_CASE("get<T> - return type is the declared type (not Optional)", "[xll::Tuple][get_type]")
{
    using T = xll::Tuple<xll::Number, xll::String>;
    T t { xll::Number(1.0), xll::String("x") };

    STATIC_REQUIRE(std::same_as<decltype(xll::get<xll::Number>(t)), xll::Number>);
    STATIC_REQUIRE(std::same_as<decltype(xll::get<xll::String>(t)), xll::String>);
}

TEST_CASE("get<T> - throws on runtime type mismatch", "[xll::Tuple][get_type]")
{
    using T = xll::Tuple<xll::Number, xll::String>;
    // Swap: String where Number declared, Number where String declared.
    T t { xll::String("bad"), xll::Number(0.0) };

    REQUIRE_THROWS_AS(xll::get<xll::Number>(t), std::invalid_argument);
    REQUIRE_THROWS_AS(xll::get<xll::String>(t), std::invalid_argument);
}

TEST_CASE("get<T> - type not in Ts fails at compile time", "[xll::Tuple][get_type]")
{
    STATIC_REQUIRE(xll::impl::count_type<xll::Bool,
                       xll::Number, xll::String> == 0);   // Bool not in pack → get<Bool> would fail
    STATIC_REQUIRE(xll::impl::count_type<xll::Number,
                       xll::Number, xll::Number> == 2);   // duplicate → get<Number> would fail
}

TEST_CASE("get<T> - duplicate type in Ts fails at compile time", "[xll::Tuple][get_type]")
{
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
        REQUIRE(xll::get<0>(t) == e);
    }
}

// =============================================================================
// MISSING AND NIL ELEMENTS
// =============================================================================

TEST_CASE("Tuple - Missing element round-trips", "[xll::Tuple][missing_nil]")
{
    xll::Tuple<xll::Missing> t { xll::Missing{} };
    REQUIRE_NOTHROW(xll::get<0>(t));
}

TEST_CASE("Tuple - Nil element round-trips", "[xll::Tuple][missing_nil]")
{
    xll::Tuple<xll::Nil> t { xll::Nil{} };
    REQUIRE_NOTHROW(xll::get<0>(t));
}

// =============================================================================
// SAFE PATH VIA cast<Tuple>(any)
// =============================================================================

TEST_CASE("Tuple - cast<Tuple>(any) engaged for valid input", "[xll::Tuple][cast]")
{
    using Row = xll::Tuple<xll::String, xll::Number>;
    xll::Any any = Row { xll::String("Alice"), xll::Number(42.0) };

    auto opt = xll::cast<Row>(any);
    REQUIRE(opt.has_value());
    REQUIRE(xll::get<xll::String>(*opt) == xll::String("Alice"));
    REQUIRE(static_cast<double>(xll::get<xll::Number>(*opt)) == Catch::Approx(42.0));
}

TEST_CASE("Tuple - cast<Tuple>(any) returns None for wrong xltype", "[xll::Tuple][cast]")
{
    using Row = xll::Tuple<xll::String, xll::Number>;
    xll::Any any = xll::String("not a tuple");
    REQUIRE_FALSE(xll::cast<Row>(any).has_value());
}

TEST_CASE("Tuple - cast<Tuple>(any) returns None for wrong element count", "[xll::Tuple][cast]")
{
    using Row = xll::Tuple<xll::String, xll::Number>;
    xll::Any any = xll::Array<xll::Any>({ xll::String("only one") });
    REQUIRE_FALSE(xll::cast<Row>(any).has_value());
}

TEST_CASE("Tuple - cast returns None for element type mismatch", "[xll::Tuple][cast]")
{
    // Structurally valid (correct size) but element types are swapped.
    using Row = xll::Tuple<xll::String, xll::Number>;
    xll::Any any = Row { xll::Number(1.0), xll::String("swapped") };

    // cast validates both structure AND element types — must return None.
    auto opt = xll::cast<Row>(any);
    REQUIRE_FALSE(opt.has_value());
}

TEST_CASE("Tuple - get throws for element type mismatch on raw Tuple", "[xll::Tuple][cast]")
{
    // Bypass cast and reinterpret directly to test that get throws on type mismatch.
    using Row = xll::Tuple<xll::String, xll::Number>;
    xll::Any any = Row { xll::Number(1.0), xll::String("swapped") };

    const auto* raw = std::launder(reinterpret_cast<const Row*>(&any));
    REQUIRE_THROWS_AS(xll::get<xll::String>(*raw), std::invalid_argument);
    REQUIRE_THROWS_AS(xll::get<xll::Number>(*raw), std::invalid_argument);
}

// =============================================================================
// TYPE ALIAS USAGE (mirrors std::tuple idiom)
// =============================================================================

TEST_CASE("Tuple - type alias usage mirrors std::tuple", "[xll::Tuple][alias]")
{
    using Row = xll::Tuple<xll::String, xll::Number, xll::Bool>;
    Row r { xll::String("Alice"), xll::Number(42.0), xll::Bool(true) };

    REQUIRE(xll::get<xll::String>(r)              == xll::String("Alice"));
    REQUIRE(static_cast<double>(xll::get<xll::Number>(r)) == Catch::Approx(42.0));
    REQUIRE(static_cast<bool>(xll::get<xll::Bool>(r))     == true);

    // Index access matches type access.
    REQUIRE(xll::get<0>(r) == xll::get<xll::String>(r));
    REQUIRE(static_cast<double>(xll::get<1>(r)) ==
            static_cast<double>(xll::get<xll::Number>(r)));
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




