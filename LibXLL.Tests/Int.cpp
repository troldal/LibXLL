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

// Int IS directly constructible from Nil/Missing: both inherit from XLOPER12,
// and Base has an explicit Base(const XLOPER12&) constructor.
// However that constructor is explicit, so no IMPLICIT conversion is possible.
// Note: the constructor will throw at runtime if xltype != xltypeInt.
static_assert(std::is_constructible_v<xll::Int, xll::Nil>);
static_assert(std::is_constructible_v<xll::Int, xll::Missing>);
static_assert(!std::is_convertible_v<xll::Int, xll::Nil>);
static_assert(!std::is_convertible_v<xll::Int, xll::Missing>);

// Legal cross-type constructions (OtherTypes in Base<Int, xltypeInt, xltypeNum, xltypeBool>)
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

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST_CASE("Int – default construction", "[xll::Int][construction]")
{
    xll::Int i;
    REQUIRE(i.xltype == xltypeInt);
    REQUIRE(i.is_valid());
    REQUIRE(i == 0);
    REQUIRE(i.val.w == 0);
}

TEST_CASE("Int – construction from int", "[xll::Int][construction]")
{
    xll::Int i = 42;
    REQUIRE(i.xltype == xltypeInt);
    REQUIRE(i.is_valid());
    REQUIRE(i == 42);
    REQUIRE(i.val.w == 42);

    xll::Int neg = -7;
    REQUIRE(neg == -7);
    REQUIRE(neg.val.w == -7);
}

TEST_CASE("Int – construction from double (truncation)", "[xll::Int][construction]")
{
    xll::Int i = 3.9;
    // Standard C++ truncation towards zero
    REQUIRE(i == 3);
    REQUIRE(i.xltype == xltypeInt);

    xll::Int neg = -3.9;
    REQUIRE(neg == -3);
}

TEST_CASE("Int – construction from bool", "[xll::Int][construction]")
{
    xll::Int t = true;
    REQUIRE(t == 1);
    REQUIRE(t.xltype == xltypeInt);

    xll::Int f = false;
    REQUIRE(f == 0);
}

TEST_CASE("Int – construction from XLOPER12", "[xll::Int][construction]")
{
    XLOPER12 xl{};
    xl.xltype = xltypeInt;
    xl.val.w  = 99;

    xll::Int i(xl);
    REQUIRE(i.xltype == xltypeInt);
    REQUIRE(i.is_valid());
    REQUIRE(i == 99);
}

TEST_CASE("Int – construction from XLOPER12 with wrong type throws", "[xll::Int][construction]")
{
    XLOPER12 xl{};
    xl.xltype = xltypeNum;
    REQUIRE_THROWS(xll::Int(xl));
}

TEST_CASE("Int – explicit construction from Nil throws (wrong xltype)", "[xll::Int][construction]")
{
    xll::Nil n;
    REQUIRE_THROWS(xll::Int(static_cast<const XLOPER12&>(n)));
}

TEST_CASE("Int – explicit construction from Missing throws (wrong xltype)", "[xll::Int][construction]")
{
    xll::Missing m;
    REQUIRE_THROWS(xll::Int(static_cast<const XLOPER12&>(m)));
}

TEST_CASE("Int – copy construction", "[xll::Int][construction]")
{
    xll::Int src = 123;
    xll::Int dst = src;
    REQUIRE(dst == src);
    REQUIRE(dst == 123);
    REQUIRE(dst.xltype == xltypeInt);
    REQUIRE(dst.is_valid());
}

TEST_CASE("Int – copy construction from invalid object throws", "[xll::Int][construction]")
{
    xll::Int invalid;
    invalid.xltype = xltypeNil;
    REQUIRE_THROWS(xll::Int(invalid));
}

TEST_CASE("Int – move construction", "[xll::Int][construction]")
{
    xll::Int src = 77;
    xll::Int dst = std::move(src);
    REQUIRE(dst == 77);
    REQUIRE(dst.xltype == xltypeInt);
    REQUIRE(dst.is_valid());
}

TEST_CASE("Int – construction from xll::Bool", "[xll::Int][construction][cross-type]")
{
    xll::Int t(xll::Bool(true));
    REQUIRE(t == 1);
    REQUIRE(t.xltype == xltypeInt);

    xll::Int f(xll::Bool(false));
    REQUIRE(f == 0);
}

TEST_CASE("Int – construction from xll::Number (truncation)", "[xll::Int][construction][cross-type]")
{
    xll::Int i(xll::Number(3.14));
    REQUIRE(i == 3);
    REQUIRE(i.xltype == xltypeInt);

    xll::Int neg(xll::Number(-2.9));
    REQUIRE(neg == -2);
}

// ---------------------------------------------------------------------------
// Assignment
// ---------------------------------------------------------------------------

TEST_CASE("Int – assignment from int", "[xll::Int][assignment]")
{
    xll::Int i;
    i = 55;
    REQUIRE(i == 55);
    i = -1;
    REQUIRE(i == -1);
    REQUIRE(i.xltype == xltypeInt);
}

TEST_CASE("Int – assignment from double (truncation)", "[xll::Int][assignment]")
{
    xll::Int i;
    i = 7.9;
    REQUIRE(i == 7);
    REQUIRE(i.xltype == xltypeInt);
}

TEST_CASE("Int – copy assignment", "[xll::Int][assignment]")
{
    xll::Int src = 42;
    xll::Int dst;
    dst = src;
    REQUIRE(dst == src);
    REQUIRE(dst == 42);
    REQUIRE(dst.xltype == xltypeInt);
}

TEST_CASE("Int – copy self-assignment", "[xll::Int][assignment]")
{
    xll::Int i = 10;
    i          = i;    // NOLINT(self-assign)
    REQUIRE(i == 10);
    REQUIRE(i.xltype == xltypeInt);
}

TEST_CASE("Int – move assignment", "[xll::Int][assignment]")
{
    xll::Int src = 33;
    xll::Int dst;
    dst = std::move(src);
    REQUIRE(dst == 33);
    REQUIRE(dst.xltype == xltypeInt);
}

TEST_CASE("Int – assignment from xll::Bool", "[xll::Int][assignment][cross-type]")
{
    xll::Int i;
    i = xll::Bool(true);
    REQUIRE(i == 1);
    i = xll::Bool(false);
    REQUIRE(i == 0);
    REQUIRE(i.xltype == xltypeInt);
}

TEST_CASE("Int – assignment from xll::Number", "[xll::Int][assignment][cross-type]")
{
    xll::Int i;
    i = xll::Number(9.9);
    REQUIRE(i == 9);
    i = xll::Number(-4.1);
    REQUIRE(i == -4);
    REQUIRE(i.xltype == xltypeInt);
}

// ---------------------------------------------------------------------------
// Increment / Decrement
// ---------------------------------------------------------------------------

TEST_CASE("Int – pre-increment", "[xll::Int][arithmetic]")
{
    xll::Int i = 5;
    xll::Int& ref = ++i;
    REQUIRE(i == 6);
    REQUIRE(&ref == &i);    // returns *this
}

TEST_CASE("Int – post-increment", "[xll::Int][arithmetic]")
{
    xll::Int i   = 5;
    xll::Int old = i++;
    REQUIRE(i == 6);
    REQUIRE(old == 5);
}

TEST_CASE("Int – pre-decrement", "[xll::Int][arithmetic]")
{
    xll::Int i = 5;
    xll::Int& ref = --i;
    REQUIRE(i == 4);
    REQUIRE(&ref == &i);
}

TEST_CASE("Int – post-decrement", "[xll::Int][arithmetic]")
{
    xll::Int i   = 5;
    xll::Int old = i--;
    REQUIRE(i == 4);
    REQUIRE(old == 5);
}

TEST_CASE("Int – increment/decrement preserve xltype", "[xll::Int][arithmetic]")
{
    xll::Int i = 0;
    ++i; --i; i++; i--;
    REQUIRE(i.xltype == xltypeInt);
    REQUIRE(i.is_valid());
}

// ---------------------------------------------------------------------------
// Modulo
// ---------------------------------------------------------------------------

TEST_CASE("Int – operator%= with int", "[xll::Int][arithmetic]")
{
    xll::Int i = 10;
    i %= 3;
    REQUIRE(i == 1);
    REQUIRE(i.xltype == xltypeInt);
}

TEST_CASE("Int – operator% with int", "[xll::Int][arithmetic]")
{
    xll::Int i = 10;
    auto r = i % 3;
    REQUIRE(r == 1);
    REQUIRE(r.xltype == xltypeInt);
}

TEST_CASE("Int – operator% with xll::Int", "[xll::Int][arithmetic]")
{
    xll::Int a = 17;
    xll::Int b = 5;
    auto r = a % b;
    REQUIRE(r == 2);
    REQUIRE(r.xltype == xltypeInt);
}

TEST_CASE("Int – operator% with xll::Bool", "[xll::Int][arithmetic][cross-type]")
{
    xll::Int  i = 5;
    xll::Bool b = true;   // bool value 1
    auto r = i % b;
    REQUIRE(r == 0);      // 5 % 1 == 0
    REQUIRE(r.xltype == xltypeInt);
}

TEST_CASE("Int – negative modulo follows C++ semantics", "[xll::Int][arithmetic]")
{
    xll::Int i = -10;
    auto r = i % 3;
    REQUIRE(r == (-10 % 3));
}

// ---------------------------------------------------------------------------
// Arithmetic operators (from Base)
// ---------------------------------------------------------------------------

TEST_CASE("Int – unary plus and minus", "[xll::Int][arithmetic]")
{
    xll::Int i = 7;
    REQUIRE(+i == 7);
    REQUIRE(-i == -7);
    REQUIRE((+i).xltype == xltypeInt);
    REQUIRE((-i).xltype == xltypeInt);
}

TEST_CASE("Int – addition with int", "[xll::Int][arithmetic]")
{
    xll::Int i = 10;
    auto r = i + 5;
    REQUIRE(r == 15);
    REQUIRE(r.xltype == xltypeInt);
}

TEST_CASE("Int – addition with xll::Int", "[xll::Int][arithmetic]")
{
    xll::Int a = 3;
    xll::Int b = 4;
    auto r = a + b;
    REQUIRE(r == 7);
    REQUIRE(r.xltype == xltypeInt);
}

TEST_CASE("Int – addition with xll::Bool", "[xll::Int][arithmetic][cross-type]")
{
    xll::Int  i = 10;
    xll::Bool b = true;
    auto r = i + b;
    REQUIRE(r == 11);
    REQUIRE(r.xltype == xltypeInt);
}

TEST_CASE("Int – addition with xll::Number", "[xll::Int][arithmetic][cross-type]")
{
    xll::Int    i = 3;
    xll::Number n = 2.0;
    auto r = i + n;
    REQUIRE(r == 5);
    REQUIRE(r.xltype == xltypeInt);
}

TEST_CASE("Int – subtraction", "[xll::Int][arithmetic]")
{
    xll::Int a = 10;
    xll::Int b = 3;
    REQUIRE((a - b) == 7);
    REQUIRE((a - 4) == 6);
}

TEST_CASE("Int – multiplication", "[xll::Int][arithmetic]")
{
    xll::Int a = 6;
    xll::Int b = 7;
    REQUIRE((a * b) == 42);
    REQUIRE((a * 3) == 18);
}

TEST_CASE("Int – division", "[xll::Int][arithmetic]")
{
    xll::Int a = 20;
    xll::Int b = 4;
    REQUIRE((a / b) == 5);
    REQUIRE((a / 3) == 6);    // integer truncation
}

TEST_CASE("Int – compound assignment +=", "[xll::Int][arithmetic]")
{
    xll::Int i = 10;
    i += 5;
    REQUIRE(i == 15);
    i += xll::Int(3);
    REQUIRE(i == 18);
    REQUIRE(i.xltype == xltypeInt);
}

TEST_CASE("Int – compound assignment -=", "[xll::Int][arithmetic]")
{
    xll::Int i = 10;
    i -= 3;
    REQUIRE(i == 7);
    REQUIRE(i.xltype == xltypeInt);
}

TEST_CASE("Int – compound assignment *=", "[xll::Int][arithmetic]")
{
    xll::Int i = 5;
    i *= 4;
    REQUIRE(i == 20);
    REQUIRE(i.xltype == xltypeInt);
}

TEST_CASE("Int – compound assignment /=", "[xll::Int][arithmetic]")
{
    xll::Int i = 20;
    i /= 4;
    REQUIRE(i == 5);
    REQUIRE(i.xltype == xltypeInt);
}

// ---------------------------------------------------------------------------
// Comparison
// ---------------------------------------------------------------------------

TEST_CASE("Int – equality with int", "[xll::Int][comparison]")
{
    xll::Int i = 42;
    REQUIRE(i == 42);
    REQUIRE_FALSE(i == 0);
    REQUIRE(i != 0);
}

TEST_CASE("Int – equality with xll::Int", "[xll::Int][comparison]")
{
    xll::Int a = 5;
    xll::Int b = 5;
    xll::Int c = 6;
    REQUIRE(a == b);
    REQUIRE_FALSE(a == c);
    REQUIRE(a != c);
}

TEST_CASE("Int – equality with xll::Bool", "[xll::Int][comparison][cross-type]")
{
    xll::Int  i1 = 1;
    xll::Int  i0 = 0;
    xll::Bool t  = true;
    xll::Bool f  = false;
    REQUIRE(i1 == t);
    REQUIRE(i0 == f);
    REQUIRE_FALSE(i1 == f);
}

TEST_CASE("Int – equality with xll::Number", "[xll::Int][comparison][cross-type]")
{
    xll::Int    i = 3;
    xll::Number n = 3.0;
    REQUIRE(i == n);
    xll::Number m = 3.5;
    REQUIRE_FALSE(i == m);
}

TEST_CASE("Int – three-way comparison (spaceship)", "[xll::Int][comparison]")
{
    xll::Int a = 3;
    xll::Int b = 5;
    REQUIRE((a <=> b) < 0);
    REQUIRE((b <=> a) > 0);
    REQUIRE((a <=> a) == 0);
    REQUIRE(a < b);
    REQUIRE(b > a);
    REQUIRE(a <= a);
    REQUIRE(b >= b);
}

TEST_CASE("Int – three-way comparison with fundamental", "[xll::Int][comparison]")
{
    xll::Int i = 10;
    REQUIRE((i <=> 10) == 0);
    REQUIRE((i <=> 9) > 0);
    REQUIRE((i <=> 11) < 0);
}

// ---------------------------------------------------------------------------
// Conversion
// ---------------------------------------------------------------------------

TEST_CASE("Int – explicit conversion to bool", "[xll::Int][conversion]")
{
    xll::Int t = 7;
    xll::Int f = 0;
    REQUIRE(static_cast<bool>(t) == true);
    REQUIRE(static_cast<bool>(f) == false);
}

TEST_CASE("Int – implicit conversion to int", "[xll::Int][conversion]")
{
    xll::Int i  = 99;
    int      v  = i;
    REQUIRE(v == 99);
}

TEST_CASE("Int – implicit conversion to double", "[xll::Int][conversion]")
{
    xll::Int i  = 4;
    double   d  = i;
    REQUIRE(d == 4.0);
}

TEST_CASE("Int – .to<int>()", "[xll::Int][conversion]")
{
    REQUIRE(xll::Int(7).to<int>() == 7);
}

TEST_CASE("Int – .to<double>()", "[xll::Int][conversion]")
{
    REQUIRE(xll::Int(3).to<double>() == 3.0);
}

// ---------------------------------------------------------------------------
// Stream output
// ---------------------------------------------------------------------------

TEST_CASE("Int – stream output", "[xll::Int][stream]")
{
    std::ostringstream os;
    os << xll::Int(42);
    REQUIRE(os.str() == "42");
}

TEST_CASE("Int – stream output negative", "[xll::Int][stream]")
{
    std::ostringstream os;
    os << xll::Int(-5);
    REQUIRE(os.str() == "-5");
}

// ---------------------------------------------------------------------------
// Swap
// ---------------------------------------------------------------------------

TEST_CASE("Int – swap", "[xll::Int][swap]")
{
    xll::Int a = 10;
    xll::Int b = 20;
    using std::swap;
    swap(a, b);
    REQUIRE(a == 20);
    REQUIRE(b == 10);
    REQUIRE(a.xltype == xltypeInt);
    REQUIRE(b.xltype == xltypeInt);
}

// ---------------------------------------------------------------------------
// Validity
// ---------------------------------------------------------------------------

TEST_CASE("Int – is_valid reflects xltype", "[xll::Int][validity]")
{
    xll::Int i = 1;
    REQUIRE(i.is_valid());

    i.xltype = xltypeNil;
    REQUIRE_FALSE(i.is_valid());
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

TEST_CASE("Int – sizeof equals XLOPER12", "[xll::Int][layout]")
{
    REQUIRE(sizeof(xll::Int) == sizeof(XLOPER12));
}

