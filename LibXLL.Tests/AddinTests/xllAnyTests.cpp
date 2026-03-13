// xllAnyTests.cpp
// Catch2 test suite for the xllAny.xll addin.
//
// Loads xllAny.xll via MockXL::Session and exercises each exported function.
// The XLL path is injected at build time by CMake via the XLL_PATH macro.
//
// Test groups:
//   [echo]          — ANY.ECHO round-trips every xll type through the XLL.
//   [type_code]     — ANY.TYPE.CODE returns the correct xltype code.
//   [negate_number] — ANY.NEGATE.NUMBER negates Numbers; returns #VALUE! for others.
//   [concat]        — ANY.CONCAT concatenates Strings; returns #VALUE! for others.

#ifndef XLL_PATH
#  error "XLL_PATH must be defined by the build system (see CMakeLists.txt)"
#endif

#include <catch_amalgamated.hpp>
#include <MockXL.hpp>
#include <Types.hpp>

// ============================================================================
// Global session — created once for the entire test run so we pay the
// load/unload cost only once.
// ============================================================================

static MockXL::Session* g_session = nullptr;

static MockXL::Session& session()
{
    REQUIRE(g_session != nullptr);
    return *g_session;
}

int main(int argc, char* argv[])
{
    MockXL::Session sess{ XLL_PATH };
    g_session = &sess;
    const int result = Catch::Session().run(argc, argv);
    g_session = nullptr;
    return result;
}

// ============================================================================
// Helpers
// ============================================================================

static double numValue(const xll::Any& a)
{
    auto r = xll::cast<xll::Number>(a);
    REQUIRE(r.has_value());
    return static_cast<double>(r.value());
}

static std::string strValue(const xll::Any& a)
{
    auto r = xll::cast<xll::String>(a);
    REQUIRE(r.has_value());
    return static_cast<std::string>(r.value());
}

static bool isError(const xll::Any& a, const xll::Error& expected)
{
    auto r = xll::cast<xll::Error>(a);
    return r.has_value() && r.value() == expected;
}

// ============================================================================
// ANY.ECHO
// ============================================================================

TEST_CASE("ANY.ECHO — Number round-trips", "[xll::Any][echo]")
{
    xll::Number v{ 3.14 };
    auto result = session().call<"ANY.ECHO">(v);
    REQUIRE(xll::holds<xll::Number>(result));
    REQUIRE(numValue(result) == Catch::Approx(3.14));
}

TEST_CASE("ANY.ECHO — negative Number round-trips", "[xll::Any][echo]")
{
    xll::Number v{ -273.15 };
    auto result = session().call<"ANY.ECHO">(v);
    REQUIRE(xll::holds<xll::Number>(result));
    REQUIRE(numValue(result) == Catch::Approx(-273.15));
}

TEST_CASE("ANY.ECHO — String round-trips", "[xll::Any][echo]")
{
    xll::String v{ "hello" };
    auto result = session().call<"ANY.ECHO">(v);
    REQUIRE(xll::holds<xll::String>(result));
    REQUIRE(strValue(result) == "hello");
}

TEST_CASE("ANY.ECHO — empty String round-trips", "[xll::Any][echo]")
{
    xll::String v{ "" };
    auto result = session().call<"ANY.ECHO">(v);
    REQUIRE(xll::holds<xll::String>(result));
    REQUIRE(strValue(result).empty());
}

TEST_CASE("ANY.ECHO — Bool(true) round-trips", "[xll::Any][echo]")
{
    xll::Bool v{ true };
    auto result = session().call<"ANY.ECHO">(v);
    REQUIRE(xll::holds<xll::Bool>(result));
    REQUIRE(xll::cast<xll::Bool>(result).value() == true);
}

TEST_CASE("ANY.ECHO — Bool(false) round-trips", "[xll::Any][echo]")
{
    xll::Bool v{ false };
    auto result = session().call<"ANY.ECHO">(v);
    REQUIRE(xll::holds<xll::Bool>(result));
    REQUIRE(xll::cast<xll::Bool>(result).value() == false);
}

TEST_CASE("ANY.ECHO — Int round-trips", "[xll::Any][echo]")
{
    xll::Int v{ 42 };
    auto result = session().call<"ANY.ECHO">(v);
    REQUIRE(xll::holds<xll::Int>(result));
    REQUIRE(xll::cast<xll::Int>(result).value() == 42);
}

TEST_CASE("ANY.ECHO — Error(#N/A) round-trips", "[xll::Any][echo]")
{
    auto result = session().call<"ANY.ECHO">(xll::ErrNA);
    REQUIRE(xll::holds<xll::Error>(result));
    REQUIRE(xll::cast<xll::Error>(result).value() == xll::ErrNA);
}

TEST_CASE("ANY.ECHO — Error(#DIV/0!) round-trips", "[xll::Any][echo]")
{
    auto result = session().call<"ANY.ECHO">(xll::ErrDiv0);
    REQUIRE(xll::holds<xll::Error>(result));
    REQUIRE(xll::cast<xll::Error>(result).value() == xll::ErrDiv0);
}

TEST_CASE("ANY.ECHO — Nil round-trips", "[xll::Any][echo]")
{
    xll::Nil v{};
    auto result = session().call<"ANY.ECHO">(v);
    REQUIRE(xll::holds<xll::Nil>(result));
    REQUIRE(result.empty());
}

// ============================================================================
// ANY.TYPE.CODE
// ============================================================================

TEST_CASE("ANY.TYPE.CODE — Number yields xltypeNum", "[xll::Any][type_code]")
{
    xll::Number v{ 1.0 };
    auto result = session().call<"ANY.TYPE.CODE">(v);
    REQUIRE(numValue(result) == Catch::Approx(static_cast<double>(xltypeNum)));
}

TEST_CASE("ANY.TYPE.CODE — String yields xltypeStr", "[xll::Any][type_code]")
{
    xll::String v{ "x" };
    auto result = session().call<"ANY.TYPE.CODE">(v);
    REQUIRE(numValue(result) == Catch::Approx(static_cast<double>(xltypeStr)));
}

TEST_CASE("ANY.TYPE.CODE — Bool yields xltypeBool", "[xll::Any][type_code]")
{
    xll::Bool v{ true };
    auto result = session().call<"ANY.TYPE.CODE">(v);
    REQUIRE(numValue(result) == Catch::Approx(static_cast<double>(xltypeBool)));
}

TEST_CASE("ANY.TYPE.CODE — Int yields xltypeInt", "[xll::Any][type_code]")
{
    xll::Int v{ 7 };
    auto result = session().call<"ANY.TYPE.CODE">(v);
    REQUIRE(numValue(result) == Catch::Approx(static_cast<double>(xltypeInt)));
}

TEST_CASE("ANY.TYPE.CODE — Error yields xltypeErr", "[xll::Any][type_code]")
{
    auto result = session().call<"ANY.TYPE.CODE">(xll::ErrDiv0);
    REQUIRE(numValue(result) == Catch::Approx(static_cast<double>(xltypeErr)));
}

TEST_CASE("ANY.TYPE.CODE — Nil yields xltypeNil", "[xll::Any][type_code]")
{
    xll::Nil v{};
    auto result = session().call<"ANY.TYPE.CODE">(v);
    REQUIRE(numValue(result) == Catch::Approx(static_cast<double>(xltypeNil)));
}

// ============================================================================
// ANY.NEGATE.NUMBER
// ============================================================================

TEST_CASE("ANY.NEGATE.NUMBER — negates a positive Number", "[xll::Any][negate_number]")
{
    xll::Number v{ 5.0 };
    auto result = session().call<"ANY.NEGATE.NUMBER">(v);
    REQUIRE(xll::holds<xll::Number>(result));
    REQUIRE(numValue(result) == Catch::Approx(-5.0));
}

TEST_CASE("ANY.NEGATE.NUMBER — negates a negative Number", "[xll::Any][negate_number]")
{
    xll::Number v{ -2.5 };
    auto result = session().call<"ANY.NEGATE.NUMBER">(v);
    REQUIRE(xll::holds<xll::Number>(result));
    REQUIRE(numValue(result) == Catch::Approx(2.5));
}

TEST_CASE("ANY.NEGATE.NUMBER — negates zero", "[xll::Any][negate_number]")
{
    xll::Number v{ 0.0 };
    auto result = session().call<"ANY.NEGATE.NUMBER">(v);
    REQUIRE(xll::holds<xll::Number>(result));
    REQUIRE(numValue(result) == Catch::Approx(0.0));
}

TEST_CASE("ANY.NEGATE.NUMBER — String yields #VALUE!", "[xll::Any][negate_number]")
{
    xll::String v{ "not a number" };
    auto result = session().call<"ANY.NEGATE.NUMBER">(v);
    REQUIRE(isError(result, xll::ErrValue));
}

TEST_CASE("ANY.NEGATE.NUMBER — Bool yields #VALUE!", "[xll::Any][negate_number]")
{
    xll::Bool v{ true };
    auto result = session().call<"ANY.NEGATE.NUMBER">(v);
    REQUIRE(isError(result, xll::ErrValue));
}

TEST_CASE("ANY.NEGATE.NUMBER — Int yields #VALUE!", "[xll::Any][negate_number]")
{
    xll::Int v{ 3 };
    auto result = session().call<"ANY.NEGATE.NUMBER">(v);
    REQUIRE(isError(result, xll::ErrValue));
}

TEST_CASE("ANY.NEGATE.NUMBER — Error yields #VALUE!", "[xll::Any][negate_number]")
{
    auto result = session().call<"ANY.NEGATE.NUMBER">(xll::ErrNA);
    REQUIRE(isError(result, xll::ErrValue));
}

// ============================================================================
// ANY.CONCAT
// ============================================================================

TEST_CASE("ANY.CONCAT — concatenates two non-empty Strings", "[xll::Any][concat]")
{
    xll::String a{ "foo" }, b{ "bar" };
    auto result = session().call<"ANY.CONCAT">(a, b);
    REQUIRE(xll::holds<xll::String>(result));
    REQUIRE(strValue(result) == "foobar");
}

TEST_CASE("ANY.CONCAT — empty left String", "[xll::Any][concat]")
{
    xll::String a{ "" }, b{ "bar" };
    auto result = session().call<"ANY.CONCAT">(a, b);
    REQUIRE(strValue(result) == "bar");
}

TEST_CASE("ANY.CONCAT — empty right String", "[xll::Any][concat]")
{
    xll::String a{ "foo" }, b{ "" };
    auto result = session().call<"ANY.CONCAT">(a, b);
    REQUIRE(strValue(result) == "foo");
}

TEST_CASE("ANY.CONCAT — both empty Strings", "[xll::Any][concat]")
{
    xll::String a{ "" }, b{ "" };
    auto result = session().call<"ANY.CONCAT">(a, b);
    REQUIRE(xll::holds<xll::String>(result));
    REQUIRE(strValue(result).empty());
}

TEST_CASE("ANY.CONCAT — Number left yields #VALUE!", "[xll::Any][concat]")
{
    xll::Number a{ 1.0 };
    xll::String b{ "bar" };
    auto result = session().call<"ANY.CONCAT">(a, b);
    REQUIRE(isError(result, xll::ErrValue));
}

TEST_CASE("ANY.CONCAT — Number right yields #VALUE!", "[xll::Any][concat]")
{
    xll::String a{ "foo" };
    xll::Number b{ 2.0 };
    auto result = session().call<"ANY.CONCAT">(a, b);
    REQUIRE(isError(result, xll::ErrValue));
}

TEST_CASE("ANY.CONCAT — both Numbers yield #VALUE!", "[xll::Any][concat]")
{
    xll::Number a{ 1.0 }, b{ 2.0 };
    auto result = session().call<"ANY.CONCAT">(a, b);
    REQUIRE(isError(result, xll::ErrValue));
}

TEST_CASE("ANY.CONCAT — Bool left yields #VALUE!", "[xll::Any][concat]")
{
    xll::Bool   a{ true };
    xll::String b{ "x" };
    auto result = session().call<"ANY.CONCAT">(a, b);
    REQUIRE(isError(result, xll::ErrValue));
}


