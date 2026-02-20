//
// Created by kenne on 04/04/2025.
//
#include "catch_amalgamated.hpp"
#include <xlcall.hpp>

#include "../Types/Bool.hpp"
#include "../Types/Int.hpp"
#include "../Types/Missing.hpp"
#include "../Types/Nil.hpp"
#include "../Types/Number.hpp"

#include <cmath>
#include <limits>
#include <sstream>
#include <type_traits>

// ---------------------------------------------------------------------------
// Static / compile-time assertions
// ---------------------------------------------------------------------------

static_assert(sizeof(xll::Number) == sizeof(XLOPER12));
static_assert(alignof(xll::Number) == alignof(XLOPER12));
static_assert(xll::Number::has_crtp_base);
static_assert(xll::Number::excel_type == xltypeNum);
static_assert(std::is_same_v<xll::Number::value_type, decltype(XLOPER12{}.val.num)>);

// Number IS directly constructible from Nil/Missing (IS-A XLOPER12, explicit ctor),
// but NOT implicitly convertible. Throws at runtime due to xltype mismatch.
static_assert(std::is_constructible_v<xll::Number, xll::Nil>);
static_assert(std::is_constructible_v<xll::Number, xll::Missing>);
static_assert(!std::is_convertible_v<xll::Number, xll::Nil>);
static_assert(!std::is_convertible_v<xll::Number, xll::Missing>);

// Legal cross-type constructions (OtherTypes: xltypeInt, xltypeBool)
static_assert(std::is_constructible_v<xll::Number, xll::Int>);
static_assert(std::is_constructible_v<xll::Number, xll::Bool>);

// Legal constructions from fundamentals
static_assert(std::is_constructible_v<xll::Number, double>);
static_assert(std::is_constructible_v<xll::Number, int>);
static_assert(std::is_constructible_v<xll::Number, float>);
static_assert(std::is_constructible_v<xll::Number, bool>);

// operator bool is EXPLICIT
static_assert(!std::is_convertible_v<xll::Number, bool>);

// Implicit conversion to arithmetic types via operator T()
static_assert(std::is_convertible_v<xll::Number, double>);
static_assert(std::is_convertible_v<xll::Number, int>);
static_assert(std::is_convertible_v<xll::Number, float>);

// =============================================================================
// CONSTRUCTION TESTS
// =============================================================================

TEST_CASE("Number - Construction", "[xll::Number][construction]")
{
    SECTION("Default construction") {
        xll::Number n;
        REQUIRE(n.xltype == xltypeNum);
        REQUIRE(n.is_valid());
        REQUIRE(n == 0.0);
        REQUIRE(n.val.num == 0.0);
    }

    SECTION("Construction from double") {
        xll::Number n = 3.14;
        REQUIRE(n.xltype == xltypeNum);
        REQUIRE(n.is_valid());
        REQUIRE(n == 3.14);
        REQUIRE(n.val.num == 3.14);

        xll::Number neg = -2.718;
        REQUIRE(neg == -2.718);
    }

    SECTION("Construction from int") {
        xll::Number n = 42;
        REQUIRE(n.xltype == xltypeNum);
        REQUIRE(n == 42.0);
        REQUIRE(n.val.num == 42.0);
    }

    SECTION("Construction from float") {
        xll::Number n = 1.5f;
        REQUIRE(n.xltype == xltypeNum);
        REQUIRE(n == Catch::Approx(1.5));
    }

    SECTION("Construction from bool") {
        xll::Number t = true;
        REQUIRE(t == 1.0);

        xll::Number f = false;
        REQUIRE(f == 0.0);
    }

    SECTION("Construction from XLOPER12") {
        XLOPER12 xl{};
        xl.xltype  = xltypeNum;
        xl.val.num = 2.718;
        xll::Number n(xl);
        REQUIRE(n.xltype == xltypeNum);
        REQUIRE(n.is_valid());
        REQUIRE(n == 2.718);
    }

    SECTION("Construction from XLOPER12 with wrong type throws") {
        XLOPER12 xl{};
        xl.xltype = xltypeInt;
        REQUIRE_THROWS(xll::Number(xl));
    }

    SECTION("Construction from Nil throws (wrong xltype)") {
        xll::Nil n;
        REQUIRE_THROWS(xll::Number(static_cast<const XLOPER12&>(n)));
    }

    SECTION("Construction from Missing throws (wrong xltype)") {
        xll::Missing m;
        REQUIRE_THROWS(xll::Number(static_cast<const XLOPER12&>(m)));
    }

    SECTION("Copy construction") {
        xll::Number src = 3.14;
        xll::Number dst = src;
        REQUIRE(dst == src);
        REQUIRE(dst == 3.14);
        REQUIRE(dst.xltype == xltypeNum);
        REQUIRE(dst.is_valid());
    }

    SECTION("Copy construction from invalid object throws") {
        xll::Number invalid;
        invalid.xltype = xltypeNil;
        REQUIRE_THROWS(xll::Number(invalid));
    }

    SECTION("Move construction") {
        xll::Number src = 9.9;
        xll::Number dst = std::move(src);
        REQUIRE(dst == 9.9);
        REQUIRE(dst.xltype == xltypeNum);
        REQUIRE(dst.is_valid());
    }

    SECTION("Construction from xll::Int") {
        xll::Number n(xll::Int(42));
        REQUIRE(n == 42.0);
        REQUIRE(n.xltype == xltypeNum);
    }

    SECTION("Construction from xll::Bool") {
        xll::Number t(xll::Bool(true));
        REQUIRE(t == 1.0);
        REQUIRE(t.xltype == xltypeNum);

        xll::Number f(xll::Bool(false));
        REQUIRE(f == 0.0);
    }

    SECTION("Construction from extreme double values") {
        xll::Number mx  = std::numeric_limits<double>::max();
        xll::Number mn  = std::numeric_limits<double>::lowest();
        xll::Number eps = std::numeric_limits<double>::epsilon();
        REQUIRE(mx  == std::numeric_limits<double>::max());
        REQUIRE(mn  == std::numeric_limits<double>::lowest());
        REQUIRE(eps == std::numeric_limits<double>::epsilon());
    }

    SECTION("Construction from NaN preserves NaN") {
        xll::Number n = std::numeric_limits<double>::quiet_NaN();
        REQUIRE(std::isnan(n.val.num));
    }

    SECTION("Construction from infinity") {
        xll::Number pos = std::numeric_limits<double>::infinity();
        REQUIRE(std::isinf(pos.val.num));
        REQUIRE(pos.val.num > 0.0);

        xll::Number neg = -std::numeric_limits<double>::infinity();
        REQUIRE(std::isinf(neg.val.num));
        REQUIRE(neg.val.num < 0.0);
    }
}

// =============================================================================
// ASSIGNMENT TESTS
// =============================================================================

TEST_CASE("Number - Assignment", "[xll::Number][assignment]")
{
    SECTION("Assignment from double") {
        xll::Number n;
        n = 3.14;
        REQUIRE(n == 3.14);
        n = -1.0;
        REQUIRE(n == -1.0);
        REQUIRE(n.xltype == xltypeNum);
    }

    SECTION("Assignment from int") {
        xll::Number n;
        n = 7;
        REQUIRE(n == 7.0);
        REQUIRE(n.xltype == xltypeNum);
    }

    SECTION("Copy assignment") {
        xll::Number src = 2.718;
        xll::Number dst;
        dst = src;
        REQUIRE(dst == src);
        REQUIRE(dst == 2.718);
        REQUIRE(dst.xltype == xltypeNum);
    }

    SECTION("Copy self-assignment") {
        xll::Number n = 1.0;
        n             = n;    // NOLINT(self-assign)
        REQUIRE(n == 1.0);
        REQUIRE(n.xltype == xltypeNum);
    }

    SECTION("Move assignment") {
        xll::Number src = 5.5;
        xll::Number dst;
        dst = std::move(src);
        REQUIRE(dst == 5.5);
        REQUIRE(dst.xltype == xltypeNum);
    }

    SECTION("Assignment from xll::Int") {
        xll::Number n;
        n = xll::Int(10);
        REQUIRE(n == 10.0);
        REQUIRE(n.xltype == xltypeNum);
    }

    SECTION("Assignment from xll::Bool") {
        xll::Number n;
        n = xll::Bool(true);
        REQUIRE(n == 1.0);
        n = xll::Bool(false);
        REQUIRE(n == 0.0);
        REQUIRE(n.xltype == xltypeNum);
    }
}

// =============================================================================
// ARITHMETIC TESTS
// =============================================================================

TEST_CASE("Number - Arithmetic Operators", "[xll::Number][arithmetic]")
{
    SECTION("Unary plus and minus") {
        xll::Number n = 3.14;
        REQUIRE(+n == 3.14);
        REQUIRE(-n == -3.14);
        REQUIRE((+n).xltype == xltypeNum);
        REQUIRE((-n).xltype == xltypeNum);
    }

    SECTION("Addition with double") {
        xll::Number n = 1.0;
        auto        r = n + 2.0;
        REQUIRE(r == Catch::Approx(3.0));
        REQUIRE(r.xltype == xltypeNum);
    }

    SECTION("Addition with xll::Number") {
        xll::Number a = 1.5;
        xll::Number b = 2.5;
        auto        r = a + b;
        REQUIRE(r == Catch::Approx(4.0));
        REQUIRE(r.xltype == xltypeNum);
    }

    SECTION("Addition with xll::Int") {
        xll::Number n = 1.5;
        xll::Int    i = 2;
        auto        r = n + i;
        REQUIRE(r == Catch::Approx(3.5));
        REQUIRE(r.xltype == xltypeNum);
    }

    SECTION("Addition with xll::Bool") {
        xll::Number n = 2.0;
        xll::Bool   b = true;
        auto        r = n + b;
        REQUIRE(r == Catch::Approx(3.0));
        REQUIRE(r.xltype == xltypeNum);
    }

    SECTION("Subtraction") {
        xll::Number a = 5.0;
        xll::Number b = 2.0;
        REQUIRE((a - b)   == Catch::Approx(3.0));
        REQUIRE((a - 1.5) == Catch::Approx(3.5));
    }

    SECTION("Multiplication") {
        xll::Number a = 3.0;
        xll::Number b = 4.0;
        REQUIRE((a * b)   == Catch::Approx(12.0));
        REQUIRE((a * 2.0) == Catch::Approx(6.0));
    }

    SECTION("Division") {
        xll::Number a = 10.0;
        xll::Number b = 4.0;
        REQUIRE((a / b)   == Catch::Approx(2.5));
        REQUIRE((a / 2.5) == Catch::Approx(4.0));
    }

    SECTION("Compound assignment +=") {
        xll::Number n = 1.0;
        n += 2.0;
        REQUIRE(n == Catch::Approx(3.0));
        n += xll::Number(0.5);
        REQUIRE(n == Catch::Approx(3.5));
        REQUIRE(n.xltype == xltypeNum);
    }

    SECTION("Compound assignment -=") {
        xll::Number n = 5.0;
        n -= 2.0;
        REQUIRE(n == Catch::Approx(3.0));
        REQUIRE(n.xltype == xltypeNum);
    }

    SECTION("Compound assignment *=") {
        xll::Number n = 3.0;
        n *= 4.0;
        REQUIRE(n == Catch::Approx(12.0));
        REQUIRE(n.xltype == xltypeNum);
    }

    SECTION("Compound assignment /=") {
        xll::Number n = 10.0;
        n /= 4.0;
        REQUIRE(n == Catch::Approx(2.5));
        REQUIRE(n.xltype == xltypeNum);
    }

    SECTION("Compound assignment with xll::Int") {
        xll::Number n = 10.0;
        n += xll::Int(5);
        REQUIRE(n == Catch::Approx(15.0));
        n -= xll::Int(3);
        REQUIRE(n == Catch::Approx(12.0));
        n *= xll::Int(2);
        REQUIRE(n == Catch::Approx(24.0));
        n /= xll::Int(4);
        REQUIRE(n == Catch::Approx(6.0));
        REQUIRE(n.xltype == xltypeNum);
    }
}

// =============================================================================
// COMPARISON TESTS
// =============================================================================

TEST_CASE("Number - Comparison", "[xll::Number][comparison]")
{
    SECTION("Equality with double") {
        xll::Number n = 3.14;
        REQUIRE(n == 3.14);
        REQUIRE_FALSE(n == 3.0);
        REQUIRE(n != 0.0);
    }

    SECTION("Equality with xll::Number") {
        xll::Number a = 2.5;
        xll::Number b = 2.5;
        xll::Number c = 3.5;
        REQUIRE(a == b);
        REQUIRE_FALSE(a == c);
        REQUIRE(a != c);
    }

    SECTION("Equality with xll::Int") {
        xll::Number n = 5.0;
        xll::Int    i = 5;
        REQUIRE(n == i);
        xll::Int j = 6;
        REQUIRE_FALSE(n == j);
    }

    SECTION("Equality with xll::Bool") {
        xll::Number one  = 1.0;
        xll::Number zero = 0.0;
        REQUIRE(one  == xll::Bool(true));
        REQUIRE(zero == xll::Bool(false));
    }

    SECTION("Three-way comparison (spaceship)") {
        xll::Number a = 1.0;
        xll::Number b = 2.0;
        REQUIRE((a <=> b) < 0);
        REQUIRE((b <=> a) > 0);
        REQUIRE((a <=> a) == 0);
        REQUIRE(a < b);
        REQUIRE(b > a);
        REQUIRE(a <= a);
        REQUIRE(b >= b);
    }

    SECTION("Three-way comparison with fundamental") {
        xll::Number n = 5.0;
        REQUIRE((n <=> 5.0) == 0);
        REQUIRE((n <=> 4.0) > 0);
        REQUIRE((n <=> 6.0) < 0);
    }

    SECTION("NaN comparisons are unordered (partial_ordering)") {
        xll::Number nan = std::numeric_limits<double>::quiet_NaN();
        xll::Number n   = 1.0;
        REQUIRE_FALSE(nan == n);
        REQUIRE_FALSE(nan == nan);
    }
}

// =============================================================================
// CONVERSION TESTS
// =============================================================================

TEST_CASE("Number - Conversion", "[xll::Number][conversion]")
{
    SECTION("Explicit conversion to bool") {
        REQUIRE(static_cast<bool>(xll::Number(1.0))  == true);
        REQUIRE(static_cast<bool>(xll::Number(0.0))  == false);
        REQUIRE(static_cast<bool>(xll::Number(-1.0)) == true);
    }

    SECTION("Implicit conversion to double") {
        double d = xll::Number(3.14);
        REQUIRE(d == 3.14);
    }

    SECTION("Implicit conversion to int (truncation)") {
        int i = xll::Number(9.9);
        REQUIRE(i == 9);
    }

    SECTION("Implicit conversion to float") {
        float f = xll::Number(1.5);
        REQUIRE(f == Catch::Approx(1.5f));
    }

    SECTION(".to<double>()") {
        REQUIRE(xll::Number(3.14).to<double>() == 3.14);
    }

    SECTION(".to<int>() truncates") {
        REQUIRE(xll::Number(7.9).to<int>()  == 7);
        REQUIRE(xll::Number(-2.9).to<int>() == -2);
    }
}

// =============================================================================
// STREAM OUTPUT TESTS
// =============================================================================

TEST_CASE("Number - Stream Output", "[xll::Number][stream]")
{
    SECTION("Non-zero value produces non-empty string") {
        std::ostringstream os;
        os << xll::Number(3.14);
        REQUIRE(!os.str().empty());
    }

    SECTION("Zero produces non-empty string") {
        std::ostringstream os;
        os << xll::Number(0.0);
        REQUIRE(!os.str().empty());
    }
}

// =============================================================================
// SWAP TESTS
// =============================================================================

TEST_CASE("Number - Swap", "[xll::Number][swap]")
{
    SECTION("Swap exchanges values and preserves xltype") {
        xll::Number a = 1.0;
        xll::Number b = 2.0;
        using std::swap;
        swap(a, b);
        REQUIRE(a == 2.0);
        REQUIRE(b == 1.0);
        REQUIRE(a.xltype == xltypeNum);
        REQUIRE(b.xltype == xltypeNum);
    }
}

// =============================================================================
// VALIDITY AND LAYOUT TESTS
// =============================================================================

TEST_CASE("Number - Validity and Layout", "[xll::Number][validity][layout]")
{
    SECTION("is_valid reflects xltype") {
        xll::Number n = 1.0;
        REQUIRE(n.is_valid());
        n.xltype = xltypeNil;
        REQUIRE_FALSE(n.is_valid());
    }

    SECTION("sizeof equals XLOPER12") {
        REQUIRE(sizeof(xll::Number) == sizeof(XLOPER12));
    }
}

