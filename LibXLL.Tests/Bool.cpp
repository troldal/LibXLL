// ============================================================
// Tests for xll::Bool
// Covers: construction, assignment, arithmetic, comparison,
//         conversion, stream output, swap, and type safety.
// ============================================================

#include "catch_amalgamated.hpp"
#include <xlcall.hpp>

#include "../Types/Bool.hpp"
#include "../Types/Int.hpp"
#include "../Types/Missing.hpp"
#include "../Types/Nil.hpp"
#include "../Types/Number.hpp"

#include <concepts>
#include <sstream>
#include <type_traits>

// ---------------------------------------------------------------------------
// Static / compile-time assertions
// ---------------------------------------------------------------------------

static_assert(sizeof(xll::Bool) == sizeof(XLOPER12));
static_assert(alignof(xll::Bool) == alignof(XLOPER12));
static_assert(xll::Bool::has_crtp_base);
static_assert(xll::Bool::excel_type == xltypeBool);
static_assert(std::is_same_v<xll::Bool::value_type, decltype(XLOPER12{}.val.xbool)>);

// Bool IS directly constructible from Nil/Missing: both inherit from XLOPER12,
// and Base has an explicit Base(const XLOPER12&) constructor.
// However that constructor is explicit, so no IMPLICIT conversion is possible.
// Note: the constructor will throw at runtime if xltype != xltypeBool.
static_assert(std::is_constructible_v<xll::Bool, xll::Nil>);
static_assert(std::is_constructible_v<xll::Bool, xll::Missing>);
static_assert(!std::is_convertible_v<xll::Bool, xll::Nil>);
static_assert(!std::is_convertible_v<xll::Bool, xll::Missing>);

// Legal constructions from OtherTypes
static_assert(std::is_constructible_v<xll::Bool, xll::Int>);
static_assert(std::is_constructible_v<xll::Bool, xll::Number>);

// Legal constructions from fundamentals
static_assert(std::is_constructible_v<xll::Bool, bool>);
static_assert(std::is_constructible_v<xll::Bool, int>);
static_assert(std::is_constructible_v<xll::Bool, double>);

// operator bool is EXPLICIT – no implicit conversion to bool
static_assert(!std::is_convertible_v<xll::Bool, bool>);

// Implicit conversion to other arithmetic types via operator T()
static_assert(std::is_convertible_v<xll::Bool, int>);
static_assert(std::is_convertible_v<xll::Bool, double>);

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST_CASE("Bool – default construction", "[xll::Bool][construction]")
{
    xll::Bool b;
    REQUIRE(b.xltype == xltypeBool);
    REQUIRE(b.is_valid());
    REQUIRE(b == false);
    REQUIRE(b.val.xbool == 0);
}

TEST_CASE("Bool – construction from bool literal", "[xll::Bool][construction]")
{
    xll::Bool t = true;
    REQUIRE(t.xltype == xltypeBool);
    REQUIRE(t.is_valid());
    REQUIRE(t == true);
    REQUIRE(t.val.xbool != 0);

    xll::Bool f = false;
    REQUIRE(f.xltype == xltypeBool);
    REQUIRE(f.is_valid());
    REQUIRE(f == false);
    REQUIRE(f.val.xbool == 0);
}

TEST_CASE("Bool – construction from int", "[xll::Bool][construction]")
{
    xll::Bool b1 = 1;
    REQUIRE(b1 == true);
    REQUIRE(b1.xltype == xltypeBool);

    xll::Bool b0 = 0;
    REQUIRE(b0 == false);
    REQUIRE(b0.xltype == xltypeBool);

    xll::Bool bn = 42;
    REQUIRE(bn == true);
    REQUIRE(bn.xltype == xltypeBool);
}

TEST_CASE("Bool – construction from double", "[xll::Bool][construction]")
{
    xll::Bool bt = 3.14;
    REQUIRE(bt == true);
    REQUIRE(bt.xltype == xltypeBool);

    xll::Bool bf = 0.0;
    REQUIRE(bf == false);
    REQUIRE(bf.xltype == xltypeBool);
}

TEST_CASE("Bool – construction from XLOPER12", "[xll::Bool][construction]")
{
    XLOPER12 xl{};
    xl.xltype    = xltypeBool;
    xl.val.xbool = 1;

    xll::Bool b(xl);
    REQUIRE(b.xltype == xltypeBool);
    REQUIRE(b.is_valid());
    REQUIRE(b == true);
}

TEST_CASE("Bool – construction from XLOPER12 with wrong type throws", "[xll::Bool][construction]")
{
    XLOPER12 xl{};
    xl.xltype = xltypeNum;
    REQUIRE_THROWS(xll::Bool(xl));
}

TEST_CASE("Bool – explicit construction from Nil throws (wrong xltype)", "[xll::Bool][construction]")
{
    // Nil IS-A XLOPER12; the explicit XLOPER12 ctor is reachable but throws at runtime
    xll::Nil n;
    REQUIRE_THROWS(xll::Bool(static_cast<const XLOPER12&>(n)));
}

TEST_CASE("Bool – explicit construction from Missing throws (wrong xltype)", "[xll::Bool][construction]")
{
    xll::Missing m;
    REQUIRE_THROWS(xll::Bool(static_cast<const XLOPER12&>(m)));
}

TEST_CASE("Bool – copy construction", "[xll::Bool][construction]")
{
    xll::Bool src = true;
    xll::Bool dst = src;
    REQUIRE(dst == src);
    REQUIRE(dst == true);
    REQUIRE(dst.xltype == xltypeBool);
    REQUIRE(dst.is_valid());
}

TEST_CASE("Bool – copy construction from invalid object throws", "[xll::Bool][construction]")
{
    xll::Bool invalid;
    invalid.xltype = xltypeNil;
    REQUIRE_THROWS(xll::Bool(invalid));
}

TEST_CASE("Bool – move construction", "[xll::Bool][construction]")
{
    xll::Bool src  = true;
    xll::Bool dst  = std::move(src);
    REQUIRE(dst == true);
    REQUIRE(dst.xltype == xltypeBool);
    REQUIRE(dst.is_valid());
}

TEST_CASE("Bool – construction from xll::Int", "[xll::Bool][construction][cross-type]")
{
    xll::Bool bt(xll::Int(7));
    REQUIRE(bt == true);
    REQUIRE(bt.xltype == xltypeBool);

    xll::Bool bf(xll::Int(0));
    REQUIRE(bf == false);
    REQUIRE(bf.xltype == xltypeBool);
}

TEST_CASE("Bool – construction from xll::Number", "[xll::Bool][construction][cross-type]")
{
    xll::Bool bt(xll::Number(2.71828));
    REQUIRE(bt == true);
    REQUIRE(bt.xltype == xltypeBool);

    xll::Bool bf(xll::Number(0.0));
    REQUIRE(bf == false);
    REQUIRE(bf.xltype == xltypeBool);
}

// ---------------------------------------------------------------------------
// Assignment
// ---------------------------------------------------------------------------

TEST_CASE("Bool – assignment from bool", "[xll::Bool][assignment]")
{
    xll::Bool b;
    b = true;
    REQUIRE(b == true);
    b = false;
    REQUIRE(b == false);
    REQUIRE(b.xltype == xltypeBool);
}

TEST_CASE("Bool – assignment from int", "[xll::Bool][assignment]")
{
    xll::Bool b;
    b = 1;
    REQUIRE(b == true);
    b = 0;
    REQUIRE(b == false);
    REQUIRE(b.xltype == xltypeBool);
}

TEST_CASE("Bool – assignment from double", "[xll::Bool][assignment]")
{
    xll::Bool b;
    b = 3.14;
    REQUIRE(b == true);
    b = 0.0;
    REQUIRE(b == false);
    REQUIRE(b.xltype == xltypeBool);
}

TEST_CASE("Bool – copy assignment", "[xll::Bool][assignment]")
{
    xll::Bool src = true;
    xll::Bool dst;
    dst = src;
    REQUIRE(dst == src);
    REQUIRE(dst == true);
    REQUIRE(dst.xltype == xltypeBool);
}

TEST_CASE("Bool – copy self-assignment", "[xll::Bool][assignment]")
{
    xll::Bool b = true;
    b           = b;    // NOLINT(self-assign)
    REQUIRE(b == true);
    REQUIRE(b.xltype == xltypeBool);
}

TEST_CASE("Bool – move assignment", "[xll::Bool][assignment]")
{
    xll::Bool src = true;
    xll::Bool dst;
    dst = std::move(src);
    REQUIRE(dst == true);
    REQUIRE(dst.xltype == xltypeBool);
}

TEST_CASE("Bool – assignment from xll::Int", "[xll::Bool][assignment][cross-type]")
{
    xll::Bool b;
    b = xll::Int(5);
    REQUIRE(b == true);
    b = xll::Int(0);
    REQUIRE(b == false);
    REQUIRE(b.xltype == xltypeBool);
}

TEST_CASE("Bool – assignment from xll::Number", "[xll::Bool][assignment][cross-type]")
{
    xll::Bool b;
    b = xll::Number(1.0);
    REQUIRE(b == true);
    b = xll::Number(0.0);
    REQUIRE(b == false);
    REQUIRE(b.xltype == xltypeBool);
}

// ---------------------------------------------------------------------------
// Comparison
// ---------------------------------------------------------------------------

TEST_CASE("Bool – equality with bool", "[xll::Bool][comparison]")
{
    xll::Bool t = true;
    xll::Bool f = false;
    REQUIRE(t == true);
    REQUIRE(f == false);
    REQUIRE_FALSE(t == false);
    REQUIRE_FALSE(f == true);
}

TEST_CASE("Bool – equality with xll::Bool", "[xll::Bool][comparison]")
{
    xll::Bool a = true;
    xll::Bool b = true;
    xll::Bool c = false;
    REQUIRE(a == b);
    REQUIRE_FALSE(a == c);
    REQUIRE(a != c);
}

TEST_CASE("Bool – equality with xll::Int", "[xll::Bool][comparison][cross-type]")
{
    xll::Bool t = true;
    xll::Bool f = false;
    REQUIRE(t == xll::Int(1));
    REQUIRE(f == xll::Int(0));
    REQUIRE_FALSE(t == xll::Int(0));
}

TEST_CASE("Bool – equality with xll::Number", "[xll::Bool][comparison][cross-type]")
{
    xll::Bool t = true;
    xll::Bool f = false;
    REQUIRE(t == xll::Number(1.0));
    REQUIRE(f == xll::Number(0.0));
}

TEST_CASE("Bool – three-way comparison (spaceship)", "[xll::Bool][comparison]")
{
    xll::Bool t = true;
    xll::Bool f = false;
    REQUIRE((f <=> t) < 0);
    REQUIRE((t <=> f) > 0);
    REQUIRE((t <=> t) == 0);
    REQUIRE(f < t);
    REQUIRE(t > f);
    REQUIRE(t >= t);
    REQUIRE(f <= f);
    REQUIRE(f <= t);
    REQUIRE(t >= f);
}

TEST_CASE("Bool – three-way comparison with fundamental", "[xll::Bool][comparison]")
{
    xll::Bool t = true;
    REQUIRE((t <=> 1) == 0);
    REQUIRE((t <=> 0) > 0);
}

// ---------------------------------------------------------------------------
// Arithmetic
// ---------------------------------------------------------------------------

TEST_CASE("Bool – unary plus", "[xll::Bool][arithmetic]")
{
    xll::Bool t = true;
    auto      p = +t;
    REQUIRE(p == true);
    REQUIRE(p.xltype == xltypeBool);
}

TEST_CASE("Bool – unary minus", "[xll::Bool][arithmetic]")
{
    xll::Bool f = false;
    auto      n = -f;
    REQUIRE(n == false);
    REQUIRE(n.xltype == xltypeBool);
}

TEST_CASE("Bool – addition with Bool", "[xll::Bool][arithmetic]")
{
    xll::Bool t = true;
    xll::Bool f = false;
    auto      r = t + f;
    REQUIRE(r == true);
    REQUIRE(r.xltype == xltypeBool);
}

TEST_CASE("Bool – addition with fundamental", "[xll::Bool][arithmetic]")
{
    xll::Bool t = true;
    auto      r = t + 0;
    REQUIRE(r == true);
    REQUIRE(r.xltype == xltypeBool);
}

TEST_CASE("Bool – subtraction", "[xll::Bool][arithmetic]")
{
    xll::Bool t = true;
    xll::Bool f = false;
    auto      r = t - f;
    REQUIRE(r == true);
    REQUIRE(r.xltype == xltypeBool);
}

TEST_CASE("Bool – multiplication", "[xll::Bool][arithmetic]")
{
    xll::Bool t = true;
    xll::Bool f = false;
    REQUIRE((t * t) == true);
    REQUIRE((t * f) == false);
    REQUIRE((f * f) == false);
}

TEST_CASE("Bool – addition assignment with fundamental", "[xll::Bool][arithmetic]")
{
    xll::Bool b = false;
    b += 1;
    REQUIRE(b == true);
    REQUIRE(b.xltype == xltypeBool);
}

TEST_CASE("Bool – subtraction assignment", "[xll::Bool][arithmetic]")
{
    xll::Bool b = true;
    b -= 1;
    REQUIRE(b == false);
    REQUIRE(b.xltype == xltypeBool);
}

TEST_CASE("Bool – addition with xll::Int", "[xll::Bool][arithmetic][cross-type]")
{
    xll::Bool t = true;
    auto      r = t + xll::Int(0);
    REQUIRE(r == true);
    REQUIRE(r.xltype == xltypeBool);
}

TEST_CASE("Bool – addition with xll::Number", "[xll::Bool][arithmetic][cross-type]")
{
    xll::Bool f = false;
    auto      r = f + xll::Number(1.0);
    REQUIRE(r == true);
    REQUIRE(r.xltype == xltypeBool);
}

// ---------------------------------------------------------------------------
// Conversion
// ---------------------------------------------------------------------------

TEST_CASE("Bool – explicit conversion to bool", "[xll::Bool][conversion]")
{
    xll::Bool t = true;
    xll::Bool f = false;
    REQUIRE(static_cast<bool>(t) == true);
    REQUIRE(static_cast<bool>(f) == false);
}

TEST_CASE("Bool – implicit conversion to int", "[xll::Bool][conversion]")
{
    xll::Bool t  = true;
    int       i  = t;
    REQUIRE(i != 0);

    xll::Bool f  = false;
    int       j  = f;
    REQUIRE(j == 0);
}

TEST_CASE("Bool – implicit conversion to double", "[xll::Bool][conversion]")
{
    xll::Bool t  = true;
    double    d  = t;
    REQUIRE(d != 0.0);

    xll::Bool f  = false;
    double    e  = f;
    REQUIRE(e == 0.0);
}

TEST_CASE("Bool – .to<int>()", "[xll::Bool][conversion]")
{
    REQUIRE(xll::Bool(true).to<int>() != 0);
    REQUIRE(xll::Bool(false).to<int>() == 0);
}

TEST_CASE("Bool – .to<double>()", "[xll::Bool][conversion]")
{
    REQUIRE(xll::Bool(true).to<double>() != 0.0);
    REQUIRE(xll::Bool(false).to<double>() == 0.0);
}

// ---------------------------------------------------------------------------
// Stream output
// ---------------------------------------------------------------------------

TEST_CASE("Bool – stream output produces non-empty string", "[xll::Bool][stream]")
{
    std::ostringstream os;
    os << xll::Bool(true);
    REQUIRE(!os.str().empty());
}

// ---------------------------------------------------------------------------
// Swap
// ---------------------------------------------------------------------------

TEST_CASE("Bool – swap", "[xll::Bool][swap]")
{
    xll::Bool a = true;
    xll::Bool b = false;
    using std::swap;
    swap(a, b);
    REQUIRE(a == false);
    REQUIRE(b == true);
    REQUIRE(a.xltype == xltypeBool);
    REQUIRE(b.xltype == xltypeBool);
}

// ---------------------------------------------------------------------------
// Validity
// ---------------------------------------------------------------------------

TEST_CASE("Bool – is_valid reflects xltype", "[xll::Bool][validity]")
{
    xll::Bool b = true;
    REQUIRE(b.is_valid());

    b.xltype = xltypeNil;
    REQUIRE_FALSE(b.is_valid());
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

TEST_CASE("Bool – sizeof equals XLOPER12", "[xll::Bool][layout]")
{
    REQUIRE(sizeof(xll::Bool) == sizeof(XLOPER12));
}