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

// Number IS directly constructible from Nil/Missing: both inherit from XLOPER12,
// and Base has an explicit Base(const XLOPER12&) constructor.
// However that constructor is explicit, so no IMPLICIT conversion is possible.
// Note: the constructor will throw at runtime if xltype != xltypeNum.
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

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST_CASE("Number – default construction", "[xll::Number][construction]")
{
    xll::Number n;
    REQUIRE(n.xltype == xltypeNum);
    REQUIRE(n.is_valid());
    REQUIRE(n == 0.0);
    REQUIRE(n.val.num == 0.0);
}

TEST_CASE("Number – construction from double", "[xll::Number][construction]")
{
    xll::Number n = 3.14;
    REQUIRE(n.xltype == xltypeNum);
    REQUIRE(n.is_valid());
    REQUIRE(n == 3.14);
    REQUIRE(n.val.num == 3.14);

    xll::Number neg = -2.718;
    REQUIRE(neg == -2.718);
}

TEST_CASE("Number – construction from int", "[xll::Number][construction]")
{
    xll::Number n = 42;
    REQUIRE(n.xltype == xltypeNum);
    REQUIRE(n == 42.0);
    REQUIRE(n.val.num == 42.0);
}

TEST_CASE("Number – construction from float", "[xll::Number][construction]")
{
    xll::Number n = 1.5f;
    REQUIRE(n.xltype == xltypeNum);
    REQUIRE(n == Catch::Approx(1.5));
}

TEST_CASE("Number – construction from bool", "[xll::Number][construction]")
{
    xll::Number t = true;
    REQUIRE(t == 1.0);

    xll::Number f = false;
    REQUIRE(f == 0.0);
}

TEST_CASE("Number – construction from XLOPER12", "[xll::Number][construction]")
{
    XLOPER12 xl{};
    xl.xltype  = xltypeNum;
    xl.val.num = 2.718;

    xll::Number n(xl);
    REQUIRE(n.xltype == xltypeNum);
    REQUIRE(n.is_valid());
    REQUIRE(n == 2.718);
}

TEST_CASE("Number – construction from XLOPER12 with wrong type throws", "[xll::Number][construction]")
{
    XLOPER12 xl{};
    xl.xltype = xltypeInt;
    REQUIRE_THROWS(xll::Number(xl));
}

TEST_CASE("Number – explicit construction from Nil throws (wrong xltype)", "[xll::Number][construction]")
{
    xll::Nil n;
    REQUIRE_THROWS(xll::Number(static_cast<const XLOPER12&>(n)));
}

TEST_CASE("Number – explicit construction from Missing throws (wrong xltype)", "[xll::Number][construction]")
{
    xll::Missing m;
    REQUIRE_THROWS(xll::Number(static_cast<const XLOPER12&>(m)));
}

TEST_CASE("Number – copy construction", "[xll::Number][construction]")
{
    xll::Number src = 3.14;
    xll::Number dst = src;
    REQUIRE(dst == src);
    REQUIRE(dst == 3.14);
    REQUIRE(dst.xltype == xltypeNum);
    REQUIRE(dst.is_valid());
}

TEST_CASE("Number – copy construction from invalid object throws", "[xll::Number][construction]")
{
    xll::Number invalid;
    invalid.xltype = xltypeNil;
    REQUIRE_THROWS(xll::Number(invalid));
}

TEST_CASE("Number – move construction", "[xll::Number][construction]")
{
    xll::Number src = 9.9;
    xll::Number dst = std::move(src);
    REQUIRE(dst == 9.9);
    REQUIRE(dst.xltype == xltypeNum);
    REQUIRE(dst.is_valid());
}

TEST_CASE("Number – construction from xll::Int", "[xll::Number][construction][cross-type]")
{
    xll::Number n(xll::Int(42));
    REQUIRE(n == 42.0);
    REQUIRE(n.xltype == xltypeNum);
}

TEST_CASE("Number – construction from xll::Bool", "[xll::Number][construction][cross-type]")
{
    xll::Number t(xll::Bool(true));
    REQUIRE(t == 1.0);
    REQUIRE(t.xltype == xltypeNum);

    xll::Number f(xll::Bool(false));
    REQUIRE(f == 0.0);
}

TEST_CASE("Number – construction from extreme double values", "[xll::Number][construction]")
{
    xll::Number mx = std::numeric_limits<double>::max();
    REQUIRE(mx == std::numeric_limits<double>::max());

    xll::Number mn = std::numeric_limits<double>::lowest();
    REQUIRE(mn == std::numeric_limits<double>::lowest());

    xll::Number eps = std::numeric_limits<double>::epsilon();
    REQUIRE(eps == std::numeric_limits<double>::epsilon());
}

TEST_CASE("Number – construction from NaN preserves NaN", "[xll::Number][construction]")
{
    xll::Number n = std::numeric_limits<double>::quiet_NaN();
    REQUIRE(std::isnan(n.val.num));
}

TEST_CASE("Number – construction from infinity", "[xll::Number][construction]")
{
    xll::Number pos_inf = std::numeric_limits<double>::infinity();
    REQUIRE(std::isinf(pos_inf.val.num));
    REQUIRE(pos_inf.val.num > 0.0);

    xll::Number neg_inf = -std::numeric_limits<double>::infinity();
    REQUIRE(std::isinf(neg_inf.val.num));
    REQUIRE(neg_inf.val.num < 0.0);
}

// ---------------------------------------------------------------------------
// Assignment
// ---------------------------------------------------------------------------

TEST_CASE("Number – assignment from double", "[xll::Number][assignment]")
{
    xll::Number n;
    n = 3.14;
    REQUIRE(n == 3.14);
    n = -1.0;
    REQUIRE(n == -1.0);
    REQUIRE(n.xltype == xltypeNum);
}

TEST_CASE("Number – assignment from int", "[xll::Number][assignment]")
{
    xll::Number n;
    n = 7;
    REQUIRE(n == 7.0);
    REQUIRE(n.xltype == xltypeNum);
}

TEST_CASE("Number – copy assignment", "[xll::Number][assignment]")
{
    xll::Number src = 2.718;
    xll::Number dst;
    dst = src;
    REQUIRE(dst == src);
    REQUIRE(dst == 2.718);
    REQUIRE(dst.xltype == xltypeNum);
}

TEST_CASE("Number – copy self-assignment", "[xll::Number][assignment]")
{
    xll::Number n = 1.0;
    n             = n;    // NOLINT(self-assign)
    REQUIRE(n == 1.0);
    REQUIRE(n.xltype == xltypeNum);
}

TEST_CASE("Number – move assignment", "[xll::Number][assignment]")
{
    xll::Number src = 5.5;
    xll::Number dst;
    dst = std::move(src);
    REQUIRE(dst == 5.5);
    REQUIRE(dst.xltype == xltypeNum);
}

TEST_CASE("Number – assignment from xll::Int", "[xll::Number][assignment][cross-type]")
{
    xll::Number n;
    n = xll::Int(10);
    REQUIRE(n == 10.0);
    REQUIRE(n.xltype == xltypeNum);
}

TEST_CASE("Number – assignment from xll::Bool", "[xll::Number][assignment][cross-type]")
{
    xll::Number n;
    n = xll::Bool(true);
    REQUIRE(n == 1.0);
    n = xll::Bool(false);
    REQUIRE(n == 0.0);
    REQUIRE(n.xltype == xltypeNum);
}

// ---------------------------------------------------------------------------
// Arithmetic operators
// ---------------------------------------------------------------------------

TEST_CASE("Number – unary plus and minus", "[xll::Number][arithmetic]")
{
    xll::Number n = 3.14;
    REQUIRE(+n == 3.14);
    REQUIRE(-n == -3.14);
    REQUIRE((+n).xltype == xltypeNum);
    REQUIRE((-n).xltype == xltypeNum);
}

TEST_CASE("Number – addition with double", "[xll::Number][arithmetic]")
{
    xll::Number n = 1.0;
    auto r = n + 2.0;
    REQUIRE(r == Catch::Approx(3.0));
    REQUIRE(r.xltype == xltypeNum);
}

TEST_CASE("Number – addition with xll::Number", "[xll::Number][arithmetic]")
{
    xll::Number a = 1.5;
    xll::Number b = 2.5;
    auto r = a + b;
    REQUIRE(r == Catch::Approx(4.0));
    REQUIRE(r.xltype == xltypeNum);
}

TEST_CASE("Number – addition with xll::Int", "[xll::Number][arithmetic][cross-type]")
{
    xll::Number n = 1.5;
    xll::Int    i = 2;
    auto r = n + i;
    REQUIRE(r == Catch::Approx(3.5));
    REQUIRE(r.xltype == xltypeNum);
}

TEST_CASE("Number – addition with xll::Bool", "[xll::Number][arithmetic][cross-type]")
{
    xll::Number n = 2.0;
    xll::Bool   b = true;
    auto r = n + b;
    REQUIRE(r == Catch::Approx(3.0));
    REQUIRE(r.xltype == xltypeNum);
}

TEST_CASE("Number – subtraction", "[xll::Number][arithmetic]")
{
    xll::Number a = 5.0;
    xll::Number b = 2.0;
    REQUIRE((a - b) == Catch::Approx(3.0));
    REQUIRE((a - 1.5) == Catch::Approx(3.5));
}

TEST_CASE("Number – multiplication", "[xll::Number][arithmetic]")
{
    xll::Number a = 3.0;
    xll::Number b = 4.0;
    REQUIRE((a * b) == Catch::Approx(12.0));
    REQUIRE((a * 2.0) == Catch::Approx(6.0));
}

TEST_CASE("Number – division", "[xll::Number][arithmetic]")
{
    xll::Number a = 10.0;
    xll::Number b = 4.0;
    REQUIRE((a / b) == Catch::Approx(2.5));
    REQUIRE((a / 2.5) == Catch::Approx(4.0));
}

TEST_CASE("Number – compound assignment +=", "[xll::Number][arithmetic]")
{
    xll::Number n = 1.0;
    n += 2.0;
    REQUIRE(n == Catch::Approx(3.0));
    n += xll::Number(0.5);
    REQUIRE(n == Catch::Approx(3.5));
    REQUIRE(n.xltype == xltypeNum);
}

TEST_CASE("Number – compound assignment -=", "[xll::Number][arithmetic]")
{
    xll::Number n = 5.0;
    n -= 2.0;
    REQUIRE(n == Catch::Approx(3.0));
    REQUIRE(n.xltype == xltypeNum);
}

TEST_CASE("Number – compound assignment *=", "[xll::Number][arithmetic]")
{
    xll::Number n = 3.0;
    n *= 4.0;
    REQUIRE(n == Catch::Approx(12.0));
    REQUIRE(n.xltype == xltypeNum);
}

TEST_CASE("Number – compound assignment /=", "[xll::Number][arithmetic]")
{
    xll::Number n = 10.0;
    n /= 4.0;
    REQUIRE(n == Catch::Approx(2.5));
    REQUIRE(n.xltype == xltypeNum);
}

TEST_CASE("Number – compound assignment with xll::Int", "[xll::Number][arithmetic][cross-type]")
{
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

// ---------------------------------------------------------------------------
// Comparison
// ---------------------------------------------------------------------------

TEST_CASE("Number – equality with double", "[xll::Number][comparison]")
{
    xll::Number n = 3.14;
    REQUIRE(n == 3.14);
    REQUIRE_FALSE(n == 3.0);
    REQUIRE(n != 0.0);
}

TEST_CASE("Number – equality with xll::Number", "[xll::Number][comparison]")
{
    xll::Number a = 2.5;
    xll::Number b = 2.5;
    xll::Number c = 3.5;
    REQUIRE(a == b);
    REQUIRE_FALSE(a == c);
    REQUIRE(a != c);
}

TEST_CASE("Number – equality with xll::Int", "[xll::Number][comparison][cross-type]")
{
    xll::Number n = 5.0;
    xll::Int    i = 5;
    REQUIRE(n == i);
    xll::Int j = 6;
    REQUIRE_FALSE(n == j);
}

TEST_CASE("Number – equality with xll::Bool", "[xll::Number][comparison][cross-type]")
{
    xll::Number one  = 1.0;
    xll::Number zero = 0.0;
    REQUIRE(one == xll::Bool(true));
    REQUIRE(zero == xll::Bool(false));
}

TEST_CASE("Number – three-way comparison (spaceship)", "[xll::Number][comparison]")
{
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

TEST_CASE("Number – three-way comparison with fundamental", "[xll::Number][comparison]")
{
    xll::Number n = 5.0;
    REQUIRE((n <=> 5.0) == 0);
    REQUIRE((n <=> 4.0) > 0);
    REQUIRE((n <=> 6.0) < 0);
}

TEST_CASE("Number – NaN comparison returns unordered (partial_ordering)", "[xll::Number][comparison]")
{
    xll::Number nan = std::numeric_limits<double>::quiet_NaN();
    xll::Number n   = 1.0;
    // NaN comparisons are unordered – none of <, ==, > hold
    REQUIRE_FALSE(nan == n);
    REQUIRE_FALSE(nan == nan);
}

// ---------------------------------------------------------------------------
// Conversion
// ---------------------------------------------------------------------------

TEST_CASE("Number – explicit conversion to bool", "[xll::Number][conversion]")
{
    REQUIRE(static_cast<bool>(xll::Number(1.0)) == true);
    REQUIRE(static_cast<bool>(xll::Number(0.0)) == false);
    REQUIRE(static_cast<bool>(xll::Number(-1.0)) == true);
}

TEST_CASE("Number – implicit conversion to double", "[xll::Number][conversion]")
{
    xll::Number n = 3.14;
    double      d = n;
    REQUIRE(d == 3.14);
}

TEST_CASE("Number – implicit conversion to int (truncation)", "[xll::Number][conversion]")
{
    xll::Number n  = 9.9;
    int         i  = n;
    REQUIRE(i == 9);
}

TEST_CASE("Number – implicit conversion to float", "[xll::Number][conversion]")
{
    xll::Number n = 1.5;
    float       f = n;
    REQUIRE(f == Catch::Approx(1.5f));
}

TEST_CASE("Number – .to<double>()", "[xll::Number][conversion]")
{
    REQUIRE(xll::Number(3.14).to<double>() == 3.14);
}

TEST_CASE("Number – .to<int>() truncates", "[xll::Number][conversion]")
{
    REQUIRE(xll::Number(7.9).to<int>() == 7);
    REQUIRE(xll::Number(-2.9).to<int>() == -2);
}

// ---------------------------------------------------------------------------
// Stream output
// ---------------------------------------------------------------------------

TEST_CASE("Number – stream output", "[xll::Number][stream]")
{
    std::ostringstream os;
    os << xll::Number(3.14);
    REQUIRE(!os.str().empty());
}

TEST_CASE("Number – stream output zero", "[xll::Number][stream]")
{
    std::ostringstream os;
    os << xll::Number(0.0);
    REQUIRE(!os.str().empty());
}

// ---------------------------------------------------------------------------
// Swap
// ---------------------------------------------------------------------------

TEST_CASE("Number – swap", "[xll::Number][swap]")
{
    xll::Number a = 1.0;
    xll::Number b = 2.0;
    using std::swap;
    swap(a, b);
    REQUIRE(a == 2.0);
    REQUIRE(b == 1.0);
    REQUIRE(a.xltype == xltypeNum);
    REQUIRE(b.xltype == xltypeNum);
}

// ---------------------------------------------------------------------------
// Validity
// ---------------------------------------------------------------------------

TEST_CASE("Number – is_valid reflects xltype", "[xll::Number][validity]")
{
    xll::Number n = 1.0;
    REQUIRE(n.is_valid());

    n.xltype = xltypeNil;
    REQUIRE_FALSE(n.is_valid());
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

TEST_CASE("Number – sizeof equals XLOPER12", "[xll::Number][layout]")
{
    REQUIRE(sizeof(xll::Number) == sizeof(XLOPER12));
}