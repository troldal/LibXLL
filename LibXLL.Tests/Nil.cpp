// ============================================================
// Tests for xll::Nil
// Covers: construction, copy, assignment, equality, cross-type
//         interaction with xll::Missing, and type safety.
// ============================================================

#include "catch_amalgamated.hpp"
#include <xlcall.hpp>

#include "../Types/Bool.hpp"
#include "../Types/Int.hpp"
#include "../Types/Missing.hpp"
#include "../Types/Nil.hpp"
#include "../Types/Number.hpp"

#include <type_traits>

// ---------------------------------------------------------------------------
// Static / compile-time assertions
// ---------------------------------------------------------------------------

static_assert(sizeof(xll::Nil) == sizeof(XLOPER12));
static_assert(alignof(xll::Nil) == alignof(XLOPER12));

// The primary Excel type constant must be correct
static_assert(xll::Nil::excel_type == xltypeNil);

// Nil is NOT constructible from arithmetic types
static_assert(!std::is_constructible_v<xll::Nil, int>);
static_assert(!std::is_constructible_v<xll::Nil, double>);
static_assert(!std::is_constructible_v<xll::Nil, bool>);

// Bool/Int/Number ARE directly constructible from Nil (IS-A XLOPER12, explicit ctor),
// but they will throw at runtime because xltype != their expected type.
// They are NOT implicitly convertible from Nil.
static_assert(!std::is_constructible_v<xll::Nil, xll::Bool>);
static_assert(!std::is_constructible_v<xll::Nil, xll::Int>);
static_assert(!std::is_constructible_v<xll::Nil, xll::Number>);

// Nil IS constructible from Missing via Missing::operator Nil() –
// there is no Nil(const Missing&) constructor; the conversion comes from Missing's side
static_assert(std::is_constructible_v<xll::Nil, xll::Missing>);

// Nil is NOT implicitly convertible to arithmetic types
static_assert(!std::is_convertible_v<xll::Nil, int>);
static_assert(!std::is_convertible_v<xll::Nil, double>);
static_assert(!std::is_convertible_v<xll::Nil, bool>);

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST_CASE("Nil - default construction", "[xll::Nil][construction]")
{
    xll::Nil n;
    REQUIRE(n.xltype == xltypeNil);
}

TEST_CASE("Nil - copy construction", "[xll::Nil][construction]")
{
    xll::Nil a;
    xll::Nil b = a;
    REQUIRE(b.xltype == xltypeNil);
}

TEST_CASE("Nil - construction from Missing", "[xll::Nil][construction][cross-type]")
{
    // Nil has no constructor from Missing; conversion goes via Missing::operator Nil()
    xll::Missing m;
    xll::Nil     n = static_cast<xll::Nil>(m);
    REQUIRE(n.xltype == xltypeNil);
}

// ---------------------------------------------------------------------------
// Assignment
// ---------------------------------------------------------------------------

TEST_CASE("Nil - copy assignment", "[xll::Nil][assignment]")
{
    xll::Nil a;
    xll::Nil b;
    b = a;
    REQUIRE(b.xltype == xltypeNil);
}

TEST_CASE("Nil - self-assignment", "[xll::Nil][assignment]")
{
    xll::Nil  n;
    xll::Nil& ref = n;
    n = ref;    // exercises operator=(const Nil&) with self
    REQUIRE(n.xltype == xltypeNil);
}

TEST_CASE("Nil - assignment from Missing", "[xll::Nil][assignment][cross-type]")
{
    // Nil has no operator=(Missing); assignment requires explicit cast
    xll::Nil     n;
    xll::Missing m;
    n = static_cast<xll::Nil>(m);
    REQUIRE(n.xltype == xltypeNil);
}

// ---------------------------------------------------------------------------
// Equality
// ---------------------------------------------------------------------------

TEST_CASE("Nil - all Nil values are equal", "[xll::Nil][comparison]")
{
    xll::Nil a;
    xll::Nil b;
    REQUIRE(a == b);
    REQUIRE_FALSE(a != b);
}

// ---------------------------------------------------------------------------
// Cross-type interaction with Missing
// ---------------------------------------------------------------------------

TEST_CASE("Nil - Missing converts to Nil via operator Nil()", "[xll::Nil][cross-type]")
{
    xll::Missing m;
    xll::Nil     n = static_cast<xll::Nil>(m);
    REQUIRE(n.xltype == xltypeNil);
}

TEST_CASE("Nil - constructed from Missing retains xltypeNil", "[xll::Nil][cross-type]")
{
    xll::Missing m;
    xll::Nil     n = static_cast<xll::Nil>(m);
    REQUIRE(n.xltype == xltypeNil);
    // The original Missing is not affected
    REQUIRE(m.xltype == xltypeMissing);
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

TEST_CASE("Nil - sizeof equals XLOPER12", "[xll::Nil][layout]")
{
    REQUIRE(sizeof(xll::Nil) == sizeof(XLOPER12));
}








