// ============================================================
// Tests for xll::Int
// Covers: construction, assignment, arithmetic (incl. ++/-- and %),
//         comparison, conversion, stream output, swap, and type safety.
// ============================================================

#include "catch_amalgamated.hpp"
#include <xlcall.hpp>

#include "../Types/Bool.hpp"
#include "../Types/Int.hpp"
#include "../Types/Missing.hpp"
#include "../Types/Nil.hpp"
#include "../Types/Number.hpp"

#include <sstream>
#include <type_traits>

// ---------------------------------------------------------------------------
// Static / compile-time assertions
// ---------------------------------------------------------------------------

static_assert(sizeof(xll::Int) == sizeof(XLOPER12));
static_assert(alignof(xll::Int) == alignof(XLOPER12));
static_assert(xll::Int::has_crtp_base);
static_assert(xll::Int::excel_type == xltypeInt);
static_assert(std::is_same_v<xll::Int::value_type, decltype(XLOPER12{}.val.w)>);

// Int IS directly constructible from Nil/Missing (IS-A XLOPER12, explicit ctor),
// but NOT implicitly convertible. Throws at runtime due to xltype mismatch.
static_assert(std::is_constructible_v<xll::Int, xll::Nil>);
static_assert(std::is_constructible_v<xll::Int, xll::Missing>);
static_assert(!std::is_convertible_v<xll::Int, xll::Nil>);
static_assert(!std::is_convertible_v<xll::Int, xll::Missing>);

// Legal cross-type constructions (OtherTypes: xltypeNum, xltypeBool)
static_assert(std::is_constructible_v<xll::Int, xll::Number>);
static_assert(std::is_constructible_v<xll::Int, xll::Bool>);

// Legal constructions from fundamentals
static_assert(std::is_constructible_v<xll::Int, int>);
static_assert(std::is_constructible_v<xll::Int, double>);
static_assert(std::is_constructible_v<xll::Int, bool>);

// operator bool is EXPLICIT
static_assert(!std::is_convertible_v<xll::Int, bool>);

// Implicit conversion to arithmetic types via operator T()
static_assert(std::is_convertible_v<xll::Int, int>);
static_assert(std::is_convertible_v<xll::Int, double>);

// =============================================================================
// CONSTRUCTION TESTS
// =============================================================================

TEST_CASE("Int - Construction", "[xll::Int][construction]")
{
    SECTION("Default construction") {
        xll::Int i;
        REQUIRE(i.xltype == xltypeInt);
        REQUIRE(i.is_valid());
        REQUIRE(i == 0);
        REQUIRE(i.val.w == 0);
    }

    SECTION("Construction from int") {
        xll::Int i = 42;
        REQUIRE(i.xltype == xltypeInt);
        REQUIRE(i.is_valid());
        REQUIRE(i == 42);
        REQUIRE(i.val.w == 42);

        xll::Int neg = -7;
        REQUIRE(neg == -7);
        REQUIRE(neg.val.w == -7);
    }

    SECTION("Construction from double (truncation towards zero)") {
        xll::Int i = 3.9;
        REQUIRE(i == 3);
        REQUIRE(i.xltype == xltypeInt);

        xll::Int neg = -3.9;
        REQUIRE(neg == -3);
    }

    SECTION("Construction from bool") {
        xll::Int t = true;
        REQUIRE(t == 1);
        REQUIRE(t.xltype == xltypeInt);

        xll::Int f = false;
        REQUIRE(f == 0);
    }

    SECTION("Construction from XLOPER12") {
        XLOPER12 xl{};
        xl.xltype = xltypeInt;
        xl.val.w  = 99;
        xll::Int i(xl);
        REQUIRE(i.xltype == xltypeInt);
        REQUIRE(i.is_valid());
        REQUIRE(i == 99);
    }

    SECTION("Construction from XLOPER12 with wrong type throws") {
        XLOPER12 xl{};
        xl.xltype = xltypeNum;
        REQUIRE_THROWS(xll::Int(xl));
    }

    SECTION("Construction from Nil throws (wrong xltype)") {
        xll::Nil n;
        REQUIRE_THROWS(xll::Int(static_cast<const XLOPER12&>(n)));
    }

    SECTION("Construction from Missing throws (wrong xltype)") {
        xll::Missing m;
        REQUIRE_THROWS(xll::Int(static_cast<const XLOPER12&>(m)));
    }

    SECTION("Copy construction") {
        xll::Int src = 123;
        xll::Int dst = src;
        REQUIRE(dst == src);
        REQUIRE(dst == 123);
        REQUIRE(dst.xltype == xltypeInt);
        REQUIRE(dst.is_valid());
    }

    SECTION("Copy construction from invalid object throws") {
        xll::Int invalid;
        invalid.xltype = xltypeNil;
        REQUIRE_THROWS(xll::Int(invalid));
    }

    SECTION("Move construction") {
        xll::Int src = 77;
        xll::Int dst = std::move(src);
        REQUIRE(dst == 77);
        REQUIRE(dst.xltype == xltypeInt);
        REQUIRE(dst.is_valid());
    }

    SECTION("Construction from xll::Bool") {
        xll::Int t(xll::Bool(true));
        REQUIRE(t == 1);
        REQUIRE(t.xltype == xltypeInt);

        xll::Int f(xll::Bool(false));
        REQUIRE(f == 0);
    }

    SECTION("Construction from xll::Number (truncation)") {
        xll::Int i(xll::Number(3.14));
        REQUIRE(i == 3);
        REQUIRE(i.xltype == xltypeInt);

        xll::Int neg(xll::Number(-2.9));
        REQUIRE(neg == -2);
    }
}

// =============================================================================
// ASSIGNMENT TESTS
// =============================================================================

TEST_CASE("Int - Assignment", "[xll::Int][assignment]")
{
    SECTION("Assignment from int") {
        xll::Int i;
        i = 55;
        REQUIRE(i == 55);
        i = -1;
        REQUIRE(i == -1);
        REQUIRE(i.xltype == xltypeInt);
    }

    SECTION("Assignment from double (truncation)") {
        xll::Int i;
        i = 7.9;
        REQUIRE(i == 7);
        REQUIRE(i.xltype == xltypeInt);
    }

    SECTION("Copy assignment") {
        xll::Int src = 42;
        xll::Int dst;
        dst = src;
        REQUIRE(dst == src);
        REQUIRE(dst == 42);
        REQUIRE(dst.xltype == xltypeInt);
    }

    SECTION("Copy self-assignment") {
        xll::Int i = 10;
        i          = i;    // NOLINT(self-assign)
        REQUIRE(i == 10);
        REQUIRE(i.xltype == xltypeInt);
    }

    SECTION("Move assignment") {
        xll::Int src = 33;
        xll::Int dst;
        dst = std::move(src);
        REQUIRE(dst == 33);
        REQUIRE(dst.xltype == xltypeInt);
    }

    SECTION("Assignment from xll::Bool") {
        xll::Int i;
        i = xll::Bool(true);
        REQUIRE(i == 1);
        i = xll::Bool(false);
        REQUIRE(i == 0);
        REQUIRE(i.xltype == xltypeInt);
    }

    SECTION("Assignment from xll::Number") {
        xll::Int i;
        i = xll::Number(9.9);
        REQUIRE(i == 9);
        i = xll::Number(-4.1);
        REQUIRE(i == -4);
        REQUIRE(i.xltype == xltypeInt);
    }
}

// =============================================================================
// ARITHMETIC TESTS
// =============================================================================

TEST_CASE("Int - Increment and Decrement", "[xll::Int][arithmetic]")
{
    SECTION("Pre-increment returns reference to self") {
        xll::Int  i   = 5;
        xll::Int& ref = ++i;
        REQUIRE(i == 6);
        REQUIRE(&ref == &i);
    }

    SECTION("Post-increment returns old value") {
        xll::Int i   = 5;
        xll::Int old = i++;
        REQUIRE(i == 6);
        REQUIRE(old == 5);
    }

    SECTION("Pre-decrement returns reference to self") {
        xll::Int  i   = 5;
        xll::Int& ref = --i;
        REQUIRE(i == 4);
        REQUIRE(&ref == &i);
    }

    SECTION("Post-decrement returns old value") {
        xll::Int i   = 5;
        xll::Int old = i--;
        REQUIRE(i == 4);
        REQUIRE(old == 5);
    }

    SECTION("Increment/decrement preserve xltype and validity") {
        xll::Int i = 0;
        ++i; --i; i++; i--;
        REQUIRE(i.xltype == xltypeInt);
        REQUIRE(i.is_valid());
    }
}

TEST_CASE("Int - Modulo", "[xll::Int][arithmetic]")
{
    SECTION("operator%= with int") {
        xll::Int i = 10;
        i %= 3;
        REQUIRE(i == 1);
        REQUIRE(i.xltype == xltypeInt);
    }

    SECTION("operator% with int") {
        xll::Int i = 10;
        auto     r = i % 3;
        REQUIRE(r == 1);
        REQUIRE(r.xltype == xltypeInt);
    }

    SECTION("operator% with xll::Int") {
        xll::Int a = 17;
        xll::Int b = 5;
        auto     r = a % b;
        REQUIRE(r == 2);
        REQUIRE(r.xltype == xltypeInt);
    }

    SECTION("operator% with xll::Bool") {
        xll::Int  i = 5;
        xll::Bool b = true;    // bool value 1
        auto      r = i % b;
        REQUIRE(r == 0);       // 5 % 1 == 0
        REQUIRE(r.xltype == xltypeInt);
    }

    SECTION("Negative modulo follows C++ truncation semantics") {
        xll::Int i = -10;
        auto     r = i % 3;
        REQUIRE(r == (-10 % 3));
    }
}

TEST_CASE("Int - Arithmetic Operators", "[xll::Int][arithmetic]")
{
    SECTION("Unary plus and minus") {
        xll::Int i = 7;
        REQUIRE(+i == 7);
        REQUIRE(-i == -7);
        REQUIRE((+i).xltype == xltypeInt);
        REQUIRE((-i).xltype == xltypeInt);
    }

    SECTION("Addition with int") {
        xll::Int i = 10;
        auto     r = i + 5;
        REQUIRE(r == 15);
        REQUIRE(r.xltype == xltypeInt);
    }

    SECTION("Addition with xll::Int") {
        xll::Int a = 3;
        xll::Int b = 4;
        auto     r = a + b;
        REQUIRE(r == 7);
        REQUIRE(r.xltype == xltypeInt);
    }

    SECTION("Addition with xll::Bool") {
        xll::Int  i = 10;
        xll::Bool b = true;
        auto      r = i + b;
        REQUIRE(r == 11);
        REQUIRE(r.xltype == xltypeInt);
    }

    SECTION("Addition with xll::Number") {
        xll::Int    i = 3;
        xll::Number n = 2.0;
        auto        r = i + n;
        REQUIRE(r == 5);
        REQUIRE(r.xltype == xltypeInt);
    }

    SECTION("Subtraction") {
        xll::Int a = 10;
        xll::Int b = 3;
        REQUIRE((a - b) == 7);
        REQUIRE((a - 4) == 6);
    }

    SECTION("Multiplication") {
        xll::Int a = 6;
        xll::Int b = 7;
        REQUIRE((a * b) == 42);
        REQUIRE((a * 3) == 18);
    }

    SECTION("Division (integer truncation)") {
        xll::Int a = 20;
        xll::Int b = 4;
        REQUIRE((a / b) == 5);
        REQUIRE((a / 3) == 6);
    }

    SECTION("Compound assignment +=") {
        xll::Int i = 10;
        i += 5;
        REQUIRE(i == 15);
        i += xll::Int(3);
        REQUIRE(i == 18);
        REQUIRE(i.xltype == xltypeInt);
    }

    SECTION("Compound assignment -=") {
        xll::Int i = 10;
        i -= 3;
        REQUIRE(i == 7);
        REQUIRE(i.xltype == xltypeInt);
    }

    SECTION("Compound assignment *=") {
        xll::Int i = 5;
        i *= 4;
        REQUIRE(i == 20);
        REQUIRE(i.xltype == xltypeInt);
    }

    SECTION("Compound assignment /=") {
        xll::Int i = 20;
        i /= 4;
        REQUIRE(i == 5);
        REQUIRE(i.xltype == xltypeInt);
    }
}

// =============================================================================
// COMPARISON TESTS
// =============================================================================

TEST_CASE("Int - Comparison", "[xll::Int][comparison]")
{
    SECTION("Equality with int") {
        xll::Int i = 42;
        REQUIRE(i == 42);
        REQUIRE_FALSE(i == 0);
        REQUIRE(i != 0);
    }

    SECTION("Equality with xll::Int") {
        xll::Int a = 5;
        xll::Int b = 5;
        xll::Int c = 6;
        REQUIRE(a == b);
        REQUIRE_FALSE(a == c);
        REQUIRE(a != c);
    }

    SECTION("Equality with xll::Bool") {
        xll::Int  i1 = 1;
        xll::Int  i0 = 0;
        xll::Bool t  = true;
        xll::Bool f  = false;
        REQUIRE(i1 == t);
        REQUIRE(i0 == f);
        REQUIRE_FALSE(i1 == f);
    }

    SECTION("Equality with xll::Number") {
        xll::Int    i = 3;
        xll::Number n = 3.0;
        REQUIRE(i == n);
        xll::Number m = 3.5;
        REQUIRE_FALSE(i == m);
    }

    SECTION("Three-way comparison (spaceship)") {
        xll::Int a = 3;
        xll::Int b = 5;
        REQUIRE(bool((a <=> b) < 0));
        REQUIRE(bool((b <=> a) > 0));
        REQUIRE(bool((a <=> a) == 0));
        REQUIRE(a < b);
        REQUIRE(b > a);
        REQUIRE(a <= a);
        REQUIRE(b >= b);
    }

    SECTION("Three-way comparison with fundamental") {
        xll::Int i = 10;
        REQUIRE(bool((i <=> 10) == 0));
        REQUIRE(bool((i <=> 9)  > 0));
        REQUIRE(bool((i <=> 11) < 0));
    }
}

// =============================================================================
// CONVERSION TESTS
// =============================================================================

TEST_CASE("Int - Conversion", "[xll::Int][conversion]")
{
    SECTION("Explicit conversion to bool") {
        REQUIRE(static_cast<bool>(xll::Int(7)) == true);
        REQUIRE(static_cast<bool>(xll::Int(0)) == false);
    }

    SECTION("Implicit conversion to int") {
        int v = xll::Int(99);
        REQUIRE(v == 99);
    }

    SECTION("Implicit conversion to double") {
        double d = xll::Int(4);
        REQUIRE(d == 4.0);
    }

    SECTION(".to<int>()") {
        REQUIRE(xll::Int(7).to<int>() == 7);
    }

    SECTION(".to<double>()") {
        REQUIRE(xll::Int(3).to<double>() == 3.0);
    }
}

// =============================================================================
// STREAM OUTPUT TESTS
// =============================================================================

TEST_CASE("Int - Stream Output", "[xll::Int][stream]")
{
    SECTION("Positive value") {
        std::ostringstream os;
        os << xll::Int(42);
        REQUIRE(os.str() == "42");
    }

    SECTION("Negative value") {
        std::ostringstream os;
        os << xll::Int(-5);
        REQUIRE(os.str() == "-5");
    }
}

// =============================================================================
// SWAP TESTS
// =============================================================================

TEST_CASE("Int - Swap", "[xll::Int][swap]")
{
    SECTION("Swap exchanges values and preserves xltype") {
        xll::Int a = 10;
        xll::Int b = 20;
        using std::swap;
        swap(a, b);
        REQUIRE(a == 20);
        REQUIRE(b == 10);
        REQUIRE(a.xltype == xltypeInt);
        REQUIRE(b.xltype == xltypeInt);
    }
}

// =============================================================================
// VALIDITY AND LAYOUT TESTS
// =============================================================================

TEST_CASE("Int - Validity and Layout", "[xll::Int][validity][layout]")
{
    SECTION("is_valid reflects xltype") {
        xll::Int i = 1;
        REQUIRE(i.is_valid());
        i.xltype = xltypeNil;
        REQUIRE_FALSE(i.is_valid());
    }

    SECTION("sizeof equals XLOPER12") {
        REQUIRE(sizeof(xll::Int) == sizeof(XLOPER12));
    }
}

