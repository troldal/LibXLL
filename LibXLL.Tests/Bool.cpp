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

// Bool IS directly constructible from Nil/Missing (IS-A XLOPER12, explicit ctor),
// but NOT implicitly convertible (explicit ctor blocks it). Throws at runtime.
static_assert(std::is_constructible_v<xll::Bool, xll::Nil>);
static_assert(std::is_constructible_v<xll::Bool, xll::Missing>);
static_assert(!std::is_convertible_v<xll::Bool, xll::Nil>);
static_assert(!std::is_convertible_v<xll::Bool, xll::Missing>);

// Legal cross-type constructions (OtherTypes)
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

// =============================================================================
// CONSTRUCTION TESTS
// =============================================================================

TEST_CASE("Bool - Construction", "[xll::Bool][construction]")
{
    SECTION("Default construction") {
        xll::Bool b;
        REQUIRE(b.xltype == xltypeBool);
        REQUIRE(b.is_valid());
        REQUIRE(b == false);
        REQUIRE(b.val.xbool == 0);
    }

    SECTION("Construction from bool literal") {
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

    SECTION("Construction from int") {
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

    SECTION("Construction from double") {
        xll::Bool bt = 3.14;
        REQUIRE(bt == true);
        REQUIRE(bt.xltype == xltypeBool);

        xll::Bool bf = 0.0;
        REQUIRE(bf == false);
        REQUIRE(bf.xltype == xltypeBool);
    }

    SECTION("Construction from XLOPER12") {
        XLOPER12 xl{};
        xl.xltype    = xltypeBool;
        xl.val.xbool = 1;
        xll::Bool b(xl);
        REQUIRE(b.xltype == xltypeBool);
        REQUIRE(b.is_valid());
        REQUIRE(b == true);
    }

    SECTION("Construction from XLOPER12 with wrong type throws") {
        XLOPER12 xl{};
        xl.xltype = xltypeNum;
        REQUIRE_THROWS(xll::Bool(xl));
    }

    SECTION("Construction from Nil throws (wrong xltype)") {
        xll::Nil n;
        REQUIRE_THROWS(xll::Bool(static_cast<const XLOPER12&>(n)));
    }

    SECTION("Construction from Missing throws (wrong xltype)") {
        xll::Missing m;
        REQUIRE_THROWS(xll::Bool(static_cast<const XLOPER12&>(m)));
    }

    SECTION("Copy construction") {
        xll::Bool src = true;
        xll::Bool dst = src;
        REQUIRE(dst == src);
        REQUIRE(dst == true);
        REQUIRE(dst.xltype == xltypeBool);
        REQUIRE(dst.is_valid());
    }

    SECTION("Copy construction from invalid object throws") {
        xll::Bool invalid;
        invalid.xltype = xltypeNil;
        REQUIRE_THROWS(xll::Bool(invalid));
    }

    SECTION("Move construction") {
        xll::Bool src = true;
        xll::Bool dst = std::move(src);
        REQUIRE(dst == true);
        REQUIRE(dst.xltype == xltypeBool);
        REQUIRE(dst.is_valid());
    }

    SECTION("Construction from xll::Int") {
        xll::Bool bt(xll::Int(7));
        REQUIRE(bt == true);
        REQUIRE(bt.xltype == xltypeBool);

        xll::Bool bf(xll::Int(0));
        REQUIRE(bf == false);
        REQUIRE(bf.xltype == xltypeBool);
    }

    SECTION("Construction from xll::Number") {
        xll::Bool bt(xll::Number(2.71828));
        REQUIRE(bt == true);
        REQUIRE(bt.xltype == xltypeBool);

        xll::Bool bf(xll::Number(0.0));
        REQUIRE(bf == false);
        REQUIRE(bf.xltype == xltypeBool);
    }
}

// =============================================================================
// ASSIGNMENT TESTS
// =============================================================================

TEST_CASE("Bool - Assignment", "[xll::Bool][assignment]")
{
    SECTION("Assignment from bool") {
        xll::Bool b;
        b = true;
        REQUIRE(b == true);
        b = false;
        REQUIRE(b == false);
        REQUIRE(b.xltype == xltypeBool);
    }

    SECTION("Assignment from int") {
        xll::Bool b;
        b = 1;
        REQUIRE(b == true);
        b = 0;
        REQUIRE(b == false);
        REQUIRE(b.xltype == xltypeBool);
    }

    SECTION("Assignment from double") {
        xll::Bool b;
        b = 3.14;
        REQUIRE(b == true);
        b = 0.0;
        REQUIRE(b == false);
        REQUIRE(b.xltype == xltypeBool);
    }

    SECTION("Copy assignment") {
        xll::Bool src = true;
        xll::Bool dst;
        dst = src;
        REQUIRE(dst == src);
        REQUIRE(dst == true);
        REQUIRE(dst.xltype == xltypeBool);
    }

    SECTION("Copy self-assignment") {
        xll::Bool  b   = true;
        xll::Bool& ref = b;
        b = ref;    // exercises operator=(const Bool&) with self
        REQUIRE(b == true);
        REQUIRE(b.xltype == xltypeBool);
    }

    SECTION("Move assignment") {
        xll::Bool src = true;
        xll::Bool dst;
        dst = std::move(src);
        REQUIRE(dst == true);
        REQUIRE(dst.xltype == xltypeBool);
    }

    SECTION("Assignment from xll::Int") {
        xll::Bool b;
        b = xll::Int(5);
        REQUIRE(b == true);
        b = xll::Int(0);
        REQUIRE(b == false);
        REQUIRE(b.xltype == xltypeBool);
    }

    SECTION("Assignment from xll::Number") {
        xll::Bool b;
        b = xll::Number(1.0);
        REQUIRE(b == true);
        b = xll::Number(0.0);
        REQUIRE(b == false);
        REQUIRE(b.xltype == xltypeBool);
    }
}

// =============================================================================
// COMPARISON TESTS
// =============================================================================

TEST_CASE("Bool - Comparison", "[xll::Bool][comparison]")
{
    SECTION("Equality with bool") {
        xll::Bool t = true;
        xll::Bool f = false;
        REQUIRE(t == true);
        REQUIRE(f == false);
        REQUIRE_FALSE(t == false);
        REQUIRE_FALSE(f == true);
    }

    SECTION("Equality with xll::Bool") {
        xll::Bool a = true;
        xll::Bool b = true;
        xll::Bool c = false;
        REQUIRE(a == b);
        REQUIRE_FALSE(a == c);
        REQUIRE(a != c);
    }

    SECTION("Equality with xll::Int") {
        xll::Bool t = true;
        xll::Bool f = false;
        REQUIRE(t == xll::Int(1));
        REQUIRE(f == xll::Int(0));
        REQUIRE_FALSE(t == xll::Int(0));
    }

    SECTION("Equality with xll::Number") {
        xll::Bool t = true;
        xll::Bool f = false;
        REQUIRE(t == xll::Number(1.0));
        REQUIRE(f == xll::Number(0.0));
    }

    SECTION("Three-way comparison (spaceship) with xll::Bool") {
        xll::Bool t = true;
        xll::Bool f = false;
        REQUIRE(bool((f <=> t) < 0));
        REQUIRE(bool((t <=> f) > 0));
        REQUIRE(bool((t <=> t) == 0));
        REQUIRE(f < t);
        REQUIRE(t > f);
        REQUIRE(t >= t);
        REQUIRE(f <= f);
        REQUIRE(f <= t);
        REQUIRE(t >= f);
    }

    SECTION("Three-way comparison with fundamental") {
        xll::Bool t = true;
        REQUIRE(bool((t <=> 1) == 0));
        REQUIRE(bool((t <=> 0) > 0));
    }
}

// =============================================================================
// ARITHMETIC TESTS
// =============================================================================

TEST_CASE("Bool - Arithmetic", "[xll::Bool][arithmetic]")
{
    SECTION("Unary plus") {
        xll::Bool t = true;
        auto      p = +t;
        REQUIRE(p == true);
        REQUIRE(p.xltype == xltypeBool);
    }

    SECTION("Unary minus") {
        xll::Bool f = false;
        auto      n = -f;
        REQUIRE(n == false);
        REQUIRE(n.xltype == xltypeBool);
    }

    SECTION("Addition with xll::Bool") {
        xll::Bool t = true;
        xll::Bool f = false;
        auto      r = t + f;
        REQUIRE(r == true);
        REQUIRE(r.xltype == xltypeBool);
    }

    SECTION("Addition with fundamental") {
        xll::Bool t = true;
        auto      r = t + 0;
        REQUIRE(r == true);
        REQUIRE(r.xltype == xltypeBool);
    }

    SECTION("Subtraction") {
        xll::Bool t = true;
        xll::Bool f = false;
        auto      r = t - f;
        REQUIRE(r == true);
        REQUIRE(r.xltype == xltypeBool);
    }

    SECTION("Multiplication") {
        xll::Bool t = true;
        xll::Bool f = false;
        REQUIRE((t * t) == true);
        REQUIRE((t * f) == false);
        REQUIRE((f * f) == false);
    }

    SECTION("Addition assignment with fundamental") {
        xll::Bool b = false;
        b += 1;
        REQUIRE(b == true);
        REQUIRE(b.xltype == xltypeBool);
    }

    SECTION("Subtraction assignment") {
        xll::Bool b = true;
        b -= 1;
        REQUIRE(b == false);
        REQUIRE(b.xltype == xltypeBool);
    }

    SECTION("Addition with xll::Int") {
        xll::Bool t = true;
        auto      r = t + xll::Int(0);
        REQUIRE(r == true);
        REQUIRE(r.xltype == xltypeBool);
    }

    SECTION("Addition with xll::Number") {
        xll::Bool f = false;
        auto      r = f + xll::Number(1.0);
        REQUIRE(r == true);
        REQUIRE(r.xltype == xltypeBool);
    }
}

// =============================================================================
// CONVERSION TESTS
// =============================================================================

TEST_CASE("Bool - Conversion", "[xll::Bool][conversion]")
{
    SECTION("Explicit conversion to bool") {
        REQUIRE(static_cast<bool>(xll::Bool(true))  == true);
        REQUIRE(static_cast<bool>(xll::Bool(false)) == false);
    }

    SECTION("Implicit conversion to int") {
        int i = xll::Bool(true);
        int j = xll::Bool(false);
        REQUIRE(i != 0);
        REQUIRE(j == 0);
    }

    SECTION("Implicit conversion to double") {
        double d = xll::Bool(true);
        double e = xll::Bool(false);
        REQUIRE(d != 0.0);
        REQUIRE(e == 0.0);
    }

    SECTION(".to<int>()") {
        REQUIRE(xll::Bool(true).to<int>()  != 0);
        REQUIRE(xll::Bool(false).to<int>() == 0);
    }

    SECTION(".to<double>()") {
        REQUIRE(xll::Bool(true).to<double>()  != 0.0);
        REQUIRE(xll::Bool(false).to<double>() == 0.0);
    }
}

// =============================================================================
// STREAM OUTPUT TESTS
// =============================================================================

TEST_CASE("Bool - Stream Output", "[xll::Bool][stream]")
{
    SECTION("Output produces non-empty string") {
        std::ostringstream os;
        os << xll::Bool(true);
        REQUIRE(!os.str().empty());
    }
}

// =============================================================================
// SWAP TESTS
// =============================================================================

TEST_CASE("Bool - Swap", "[xll::Bool][swap]")
{
    SECTION("Swap exchanges values and preserves xltype") {
        xll::Bool a = true;
        xll::Bool b = false;
        using std::swap;
        swap(a, b);
        REQUIRE(a == false);
        REQUIRE(b == true);
        REQUIRE(a.xltype == xltypeBool);
        REQUIRE(b.xltype == xltypeBool);
    }
}

// =============================================================================
// VALIDITY AND LAYOUT TESTS
// =============================================================================

TEST_CASE("Bool - Validity and Layout", "[xll::Bool][validity][layout]")
{
    SECTION("is_valid reflects xltype") {
        xll::Bool b = true;
        REQUIRE(b.is_valid());
        b.xltype = xltypeNil;
        REQUIRE_FALSE(b.is_valid());
    }

    SECTION("sizeof equals XLOPER12") {
        REQUIRE(sizeof(xll::Bool) == sizeof(XLOPER12));
    }
}

