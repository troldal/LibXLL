// ============================================================
// Tests for xll::Missing
// Covers: construction, copy, assignment, equality, cross-type
//         interaction with xll::Nil, and type safety.
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

static_assert(sizeof(xll::Missing) == sizeof(XLOPER12));
static_assert(alignof(xll::Missing) == alignof(XLOPER12));

// The primary Excel type constant must be correct
static_assert(xll::Missing::excel_type == xltypeMissing);

// Missing is NOT constructible from arithmetic types
static_assert(!std::is_constructible_v<xll::Missing, int>);
static_assert(!std::is_constructible_v<xll::Missing, double>);
static_assert(!std::is_constructible_v<xll::Missing, bool>);

// Missing is NOT constructible from Bool, Int, or Number
static_assert(!std::is_constructible_v<xll::Missing, xll::Bool>);
static_assert(!std::is_constructible_v<xll::Missing, xll::Int>);
static_assert(!std::is_constructible_v<xll::Missing, xll::Number>);

// Missing is NOT constructible from Nil (one-way relationship: Missing -> Nil only)
static_assert(!std::is_constructible_v<xll::Missing, xll::Nil>);

// Missing IS implicitly convertible to Nil via operator xll::Nil()
static_assert(std::is_convertible_v<xll::Missing, xll::Nil>);

// Missing is NOT implicitly convertible to arithmetic types
static_assert(!std::is_convertible_v<xll::Missing, int>);
static_assert(!std::is_convertible_v<xll::Missing, double>);
static_assert(!std::is_convertible_v<xll::Missing, bool>);

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST_CASE("Missing - default construction", "[xll::Missing][construction]")
{
    xll::Missing m;
    REQUIRE(m.xltype == xltypeMissing);
}

TEST_CASE("Missing - copy construction", "[xll::Missing][construction]")
{
    xll::Missing a;
    xll::Missing b = a;
    REQUIRE(b.xltype == xltypeMissing);
}

// ---------------------------------------------------------------------------
// Assignment
// ---------------------------------------------------------------------------

TEST_CASE("Missing - copy assignment", "[xll::Missing][assignment]")
{
    xll::Missing a;
    xll::Missing b;
    b = a;
    REQUIRE(b.xltype == xltypeMissing);
}

TEST_CASE("Missing - self-assignment", "[xll::Missing][assignment]")
{
    xll::Missing m;
    xll::Missing& ref = m;
    m = ref;    // exercises operator=(const Missing&) with self
    REQUIRE(m.xltype == xltypeMissing);
}

// ---------------------------------------------------------------------------
// Equality
// ---------------------------------------------------------------------------

TEST_CASE("Missing - all Missing values are equal", "[xll::Missing][comparison]")
{
    xll::Missing a;
    xll::Missing b;
    REQUIRE(a == b);
    REQUIRE_FALSE(a != b);
}

// ---------------------------------------------------------------------------
// Cross-type interaction with Nil
// ---------------------------------------------------------------------------

TEST_CASE("Missing - implicit conversion to Nil", "[xll::Missing][cross-type]")
{
    xll::Missing m;
    xll::Nil     n = m;    // via Missing::operator Nil() – unambiguous now
    REQUIRE(n.xltype == xltypeNil);
    // The original Missing is not affected
    REQUIRE(m.xltype == xltypeMissing);
}

TEST_CASE("Missing - Nil constructed from Missing has correct type", "[xll::Missing][cross-type]")
{
    xll::Nil n = xll::Missing{};
    REQUIRE(n.xltype == xltypeNil);
}

TEST_CASE("Missing - Nil assigned from Missing has correct type", "[xll::Missing][cross-type]")
{
    xll::Nil n;
    n = xll::Missing{};
    REQUIRE(n.xltype == xltypeNil);
}

TEST_CASE("Missing - multiple copies all equal", "[xll::Missing][comparison]")
{
    xll::Missing a;
    xll::Missing b = a;
    xll::Missing c;
    c = b;
    REQUIRE(a == b);
    REQUIRE(b == c);
    REQUIRE(a == c);
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

TEST_CASE("Missing - sizeof equals XLOPER12", "[xll::Missing][layout]")
{
    REQUIRE(sizeof(xll::Missing) == sizeof(XLOPER12));
}





