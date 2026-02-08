//
// Created by kenne on 04/04/2025.
//

#include "catch_amalgamated.hpp"
#include <xlcall.hpp>
#include "../Types/Expected.hpp"
#include "../Types/Number.hpp"
#include "../Types/Int.hpp"
#include "../Types/Bool.hpp"
#include "../Types/String.hpp"
#include "../Types/Error.hpp"

// =============================================================================
// CONSTRUCTION TESTS
// =============================================================================

TEST_CASE("Expected - Default Construction", "[xll::Expected][construction]")
{
    SECTION("Default construct Expected<Number>") {
        xll::Expected<xll::Number> exp;
        REQUIRE(exp.has_value());
        REQUIRE(exp);
        REQUIRE(exp.value() == 0.0);
        REQUIRE(exp.xltype == xltypeNum);
    }

    SECTION("Default construct Expected<Int>") {
        xll::Expected<xll::Int> exp;
        REQUIRE(exp.has_value());
        REQUIRE(exp);
        REQUIRE(exp.value() == 0);
        REQUIRE(exp.xltype == xltypeInt);
    }

    SECTION("Default construct Expected<String>") {
        xll::Expected<xll::String> exp;
        REQUIRE(exp.has_value());
        REQUIRE(exp);
        REQUIRE(exp.xltype == xltypeStr);
    }
}

TEST_CASE("Expected - Value Construction", "[xll::Expected][construction]")
{
    SECTION("Construct from Number value") {
        xll::Expected<xll::Number> exp{xll::Number(3.14)};
        REQUIRE(exp.has_value());
        REQUIRE(exp);
        REQUIRE(exp.value() == 3.14);
        REQUIRE(exp.xltype == xltypeNum);
    }

    SECTION("Construct from Int value") {
        xll::Expected<xll::Int> exp{xll::Int(42)};
        REQUIRE(exp.has_value());
        REQUIRE(exp);
        REQUIRE(exp.value() == 42);
        REQUIRE(exp.xltype == xltypeInt);
    }

    SECTION("Construct from String value") {
        xll::Expected<xll::String> exp{xll::String("test")};
        REQUIRE(exp.has_value());
        REQUIRE(exp);
        REQUIRE(std::wstring(&exp.value().val.str[1]) == L"test");
        REQUIRE(exp.xltype == xltypeStr);
    }

    SECTION("Construct from convertible type") {
        xll::Expected<xll::Number> exp{42}; // int converts to Number
        REQUIRE(exp.has_value());
        REQUIRE(exp);
        REQUIRE(exp.value() == 42.0);
    }
}

TEST_CASE("Expected - Error Construction", "[xll::Expected][construction]")
{
    SECTION("Construct from Unexpected") {
        auto unexpected = xll::Unexpected<xll::Error>{xll::ErrDiv0};
        xll::Expected<xll::Number> exp{unexpected};
        REQUIRE_FALSE(exp.has_value());
        REQUIRE_FALSE(exp);
        REQUIRE(exp.error() == xll::ErrDiv0);
        REQUIRE(exp.xltype == xltypeErr);
    }

    SECTION("Construct from different error types") {
        xll::Expected<xll::Number> exp1{xll::Unexpected<xll::Error>{xll::ErrNull}};
        xll::Expected<xll::Number> exp2{xll::Unexpected<xll::Error>{xll::ErrValue}};
        xll::Expected<xll::Number> exp3{xll::Unexpected<xll::Error>{xll::ErrRef}};
        xll::Expected<xll::Number> exp4{xll::Unexpected<xll::Error>{xll::ErrName}};
        xll::Expected<xll::Number> exp5{xll::Unexpected<xll::Error>{xll::ErrNum}};
        xll::Expected<xll::Number> exp6{xll::Unexpected<xll::Error>{xll::ErrNA}};

        REQUIRE_FALSE(exp1.has_value());
        REQUIRE_FALSE(exp2.has_value());
        REQUIRE_FALSE(exp3.has_value());
        REQUIRE_FALSE(exp4.has_value());
        REQUIRE_FALSE(exp5.has_value());
        REQUIRE_FALSE(exp6.has_value());

        REQUIRE(exp1.error() == xll::ErrNull);
        REQUIRE(exp2.error() == xll::ErrValue);
        REQUIRE(exp3.error() == xll::ErrRef);
        REQUIRE(exp4.error() == xll::ErrName);
        REQUIRE(exp5.error() == xll::ErrNum);
        REQUIRE(exp6.error() == xll::ErrNA);
    }
}

TEST_CASE("Expected - Copy Construction", "[xll::Expected][construction]")
{
    SECTION("Copy Expected with value") {
        xll::Expected<xll::Number> exp1{xll::Number(3.14)};
        xll::Expected<xll::Number> exp2{exp1};

        REQUIRE(exp1.has_value());
        REQUIRE(exp2.has_value());
        REQUIRE(exp1.value() == exp2.value());
        REQUIRE(exp2.value() == 3.14);
    }

    SECTION("Copy Expected with error") {
        xll::Expected<xll::Number> exp1{xll::Unexpected<xll::Error>{xll::ErrDiv0}};
        xll::Expected<xll::Number> exp2{exp1};

        REQUIRE_FALSE(exp1.has_value());
        REQUIRE_FALSE(exp2.has_value());
        REQUIRE(exp1.error() == exp2.error());
        REQUIRE(exp2.error() == xll::ErrDiv0);
    }

    SECTION("Copy Expected<String> with value") {
        xll::Expected<xll::String> exp1{xll::String("hello")};
        xll::Expected<xll::String> exp2{exp1};

        REQUIRE(exp1.has_value());
        REQUIRE(exp2.has_value());
        REQUIRE(std::wstring(&exp2.value().val.str[1]) == L"hello");
        // Ensure deep copy
        REQUIRE(exp1.value().val.str != exp2.value().val.str);
    }
}

TEST_CASE("Expected - Move Construction", "[xll::Expected][construction]")
{
    SECTION("Move Expected with value") {
        xll::Expected<xll::Number> exp1{xll::Number(3.14)};
        xll::Expected<xll::Number> exp2{std::move(exp1)};

        REQUIRE(exp2.has_value());
        REQUIRE(exp2.value() == 3.14);
    }

    SECTION("Move Expected with error") {
        xll::Expected<xll::Number> exp1{xll::Unexpected<xll::Error>{xll::ErrDiv0}};
        xll::Expected<xll::Number> exp2{std::move(exp1)};

        REQUIRE_FALSE(exp2.has_value());
        REQUIRE(exp2.error() == xll::ErrDiv0);
    }

    SECTION("Move Expected<String>") {
        xll::Expected<xll::String> exp1{xll::String("test")};
        xll::Expected<xll::String> exp2{std::move(exp1)};

        REQUIRE(exp2.has_value());
        REQUIRE(std::wstring(&exp2.value().val.str[1]) == L"test");
        // Note: moved-from state is not guaranteed to have nullptr
    }
}

// =============================================================================
// ASSIGNMENT TESTS
// =============================================================================

TEST_CASE("Expected - Copy Assignment", "[xll::Expected][assignment]")
{
    SECTION("Assign value to value") {
        xll::Expected<xll::Number> exp1{xll::Number(3.14)};
        xll::Expected<xll::Number> exp2{xll::Number(2.718)};

        exp2 = exp1;
        REQUIRE(exp2.has_value());
        REQUIRE(exp2.value() == 3.14);
    }

    SECTION("Assign error to error") {
        xll::Expected<xll::Number> exp1{xll::Unexpected<xll::Error>{xll::ErrDiv0}};
        xll::Expected<xll::Number> exp2{xll::Unexpected<xll::Error>{xll::ErrValue}};

        exp2 = exp1;
        REQUIRE_FALSE(exp2.has_value());
        REQUIRE(exp2.error() == xll::ErrDiv0);
    }

    SECTION("Assign value to error") {
        xll::Expected<xll::Number> exp1{xll::Number(3.14)};
        xll::Expected<xll::Number> exp2{xll::Unexpected<xll::Error>{xll::ErrDiv0}};

        exp2 = exp1;
        REQUIRE(exp2.has_value());
        REQUIRE(exp2.value() == 3.14);
    }

    SECTION("Assign error to value") {
        xll::Expected<xll::Number> exp1{xll::Unexpected<xll::Error>{xll::ErrDiv0}};
        xll::Expected<xll::Number> exp2{xll::Number(3.14)};

        exp2 = exp1;
        REQUIRE_FALSE(exp2.has_value());
        REQUIRE(exp2.error() == xll::ErrDiv0);
    }

    SECTION("Self-assignment") {
        xll::Expected<xll::Number> exp{xll::Number(3.14)};
        exp = exp;
        REQUIRE(exp.has_value());
        REQUIRE(exp.value() == 3.14);
    }
}

TEST_CASE("Expected - Move Assignment", "[xll::Expected][assignment]")
{
    SECTION("Move assign value to value") {
        xll::Expected<xll::Number> exp1{xll::Number(3.14)};
        xll::Expected<xll::Number> exp2{xll::Number(2.718)};

        exp2 = std::move(exp1);
        REQUIRE(exp2.has_value());
        REQUIRE(exp2.value() == 3.14);
    }

    SECTION("Move assign error to value") {
        xll::Expected<xll::Number> exp1{xll::Unexpected<xll::Error>{xll::ErrDiv0}};
        xll::Expected<xll::Number> exp2{xll::Number(3.14)};

        exp2 = std::move(exp1);
        REQUIRE_FALSE(exp2.has_value());
        REQUIRE(exp2.error() == xll::ErrDiv0);
    }

    SECTION("Move assign with String") {
        xll::Expected<xll::String> exp1{xll::String("hello")};
        xll::Expected<xll::String> exp2{xll::String("world")};

        exp2 = std::move(exp1);
        REQUIRE(exp2.has_value());
        REQUIRE(std::wstring(&exp2.value().val.str[1]) == L"hello");
    }
}

TEST_CASE("Expected - Value Assignment", "[xll::Expected][assignment]")
{
    SECTION("Assign value type to Expected with value") {
        xll::Expected<xll::Number> exp{xll::Number(3.14)};
        exp = xll::Number(2.718);

        REQUIRE(exp.has_value());
        REQUIRE(exp.value() == 2.718);
    }

    SECTION("Assign value type to Expected with error") {
        xll::Expected<xll::Number> exp{xll::Unexpected<xll::Error>{xll::ErrDiv0}};
        exp = xll::Number(3.14);

        REQUIRE(exp.has_value());
        REQUIRE(exp.value() == 3.14);
    }

    SECTION("Assign convertible type") {
        xll::Expected<xll::Number> exp{xll::Number(3.14)};
        exp = 42; // int converts to Number

        REQUIRE(exp.has_value());
        REQUIRE(exp.value() == 42.0);
    }

    SECTION("Assign convertible type to error") {
        xll::Expected<xll::Number> exp{xll::Unexpected<xll::Error>{xll::ErrDiv0}};
        exp = 99; // int converts to Number, replacing error

        REQUIRE(exp.has_value());
        REQUIRE(exp.value() == 99.0);
    }

    SECTION("Move-assign convertible type") {
        xll::Expected<xll::Number> exp{xll::Number(3.14)};
        int value = 123;
        exp = std::move(value); // move int to Number

        REQUIRE(exp.has_value());
        REQUIRE(exp.value() == 123.0);
    }

    SECTION("Self-assignment of value") {
        xll::Expected<xll::Number> exp{xll::Number(3.14)};
        auto& val = exp.value();
        exp = val; // Self-assignment

        REQUIRE(exp.has_value());
        REQUIRE(exp.value() == 3.14);
    }
}

TEST_CASE("Expected - Unexpected Assignment", "[xll::Expected][assignment]")
{
    SECTION("Assign Unexpected to Expected with value") {
        xll::Expected<xll::Number> exp{xll::Number(3.14)};
        exp = xll::Unexpected<xll::Error>{xll::ErrDiv0};

        REQUIRE_FALSE(exp.has_value());
        REQUIRE(exp.error() == xll::ErrDiv0);
    }

    SECTION("Assign Unexpected to Expected with error") {
        xll::Expected<xll::Number> exp{xll::Unexpected<xll::Error>{xll::ErrValue}};
        exp = xll::Unexpected<xll::Error>{xll::ErrDiv0};

        REQUIRE_FALSE(exp.has_value());
        REQUIRE(exp.error() == xll::ErrDiv0);
    }
}

// =============================================================================
// VALUE ACCESS TESTS
// =============================================================================

TEST_CASE("Expected - Value Access", "[xll::Expected][access]")
{
    SECTION("value() on Expected with value") {
        xll::Expected<xll::Number> exp{xll::Number(3.14)};
        REQUIRE(exp.value() == 3.14);

        const auto& cexp = exp;
        REQUIRE(cexp.value() == 3.14);
    }

    SECTION("value() throws on Expected with error") {
        xll::Expected<xll::Number> exp{xll::Unexpected<xll::Error>{xll::ErrDiv0}};
        REQUIRE_THROWS(exp.value());

        const auto& cexp = exp;
        REQUIRE_THROWS(cexp.value());
    }

    SECTION("operator* on Expected with value") {
        xll::Expected<xll::Number> exp{xll::Number(3.14)};
        REQUIRE(*exp == 3.14);

        const auto& cexp = exp;
        REQUIRE(*cexp == 3.14);
    }

    SECTION("operator-> on Expected with value") {
        xll::Expected<xll::String> exp{xll::String("test")};
        REQUIRE(exp->xltype == xltypeStr);

        const auto& cexp = exp;
        REQUIRE(cexp->xltype == xltypeStr);
    }

    SECTION("Move value() rvalue reference") {
        xll::Expected<xll::String> exp{xll::String("test")};
        auto str = std::move(exp).value();
        REQUIRE(std::wstring(&str.val.str[1]) == L"test");
    }
}

TEST_CASE("Expected - Error Access", "[xll::Expected][access]")
{
    SECTION("error() on Expected with error") {
        xll::Expected<xll::Number> exp{xll::Unexpected<xll::Error>{xll::ErrDiv0}};
        REQUIRE(exp.error() == xll::ErrDiv0);

        const auto& cexp = exp;
        REQUIRE(cexp.error() == xll::ErrDiv0);
    }

    SECTION("error() throws on Expected with value") {
        xll::Expected<xll::Number> exp{xll::Number(3.14)};
        REQUIRE_THROWS(exp.error());

        const auto& cexp = exp;
        REQUIRE_THROWS(cexp.error());
    }

    SECTION("Move error() rvalue reference") {
        xll::Expected<xll::Number> exp{xll::Unexpected<xll::Error>{xll::ErrDiv0}};
        auto err = std::move(exp).error();
        REQUIRE(err == xll::ErrDiv0);
    }
}

TEST_CASE("Expected - Value Or Default", "[xll::Expected][access]")
{
    SECTION("value_or() with value") {
        xll::Expected<xll::Number> exp{xll::Number(3.14)};
        auto result = exp.value_or(xll::Number(0.0));
        REQUIRE(result == 3.14);
    }

    SECTION("value_or() with error") {
        xll::Expected<xll::Number> exp{xll::Unexpected<xll::Error>{xll::ErrDiv0}};
        auto result = exp.value_or(xll::Number(99.0));
        REQUIRE(result == 99.0);
    }

    SECTION("value_or() with move") {
        xll::Expected<xll::String> exp{xll::Unexpected<xll::Error>{xll::ErrDiv0}};
        auto result = std::move(exp).value_or(xll::String("default"));
        REQUIRE(std::wstring(&result.val.str[1]) == L"default");
    }
}

// =============================================================================
// EMPLACE TESTS
// =============================================================================

TEST_CASE("Expected - Emplace", "[xll::Expected][emplace]")
{
    SECTION("emplace() on Expected with value") {
        xll::Expected<xll::Number> exp{xll::Number(3.14)};
        exp.emplace(2.718);

        REQUIRE(exp.has_value());
        REQUIRE(exp.value() == 2.718);
    }

    SECTION("emplace() on Expected with error") {
        xll::Expected<xll::Number> exp{xll::Unexpected<xll::Error>{xll::ErrDiv0}};
        exp.emplace(3.14);

        REQUIRE(exp.has_value());
        REQUIRE(exp.value() == 3.14);
    }

    SECTION("emplace_error() on Expected with value") {
        xll::Expected<xll::Number> exp{xll::Number(3.14)};
        exp.emplace_error(xll::ErrDiv0);

        REQUIRE_FALSE(exp.has_value());
        REQUIRE(exp.error() == xll::ErrDiv0);
    }

    SECTION("emplace_error() on Expected with error") {
        xll::Expected<xll::Number> exp{xll::Unexpected<xll::Error>{xll::ErrValue}};
        exp.emplace_error(xll::ErrDiv0);

        REQUIRE_FALSE(exp.has_value());
        REQUIRE(exp.error() == xll::ErrDiv0);
    }
}

// =============================================================================
// MONADIC OPERATIONS TESTS
// =============================================================================

TEST_CASE("Expected - and_then", "[xll::Expected][monadic]")
{
    SECTION("and_then with value") {
        xll::Expected<xll::Number> exp{xll::Number(3.14)};

        auto result = exp.and_then([](xll::Number n) {
            return xll::Expected<xll::Number>{xll::Number(n.val.num * 2.0)};
        });

        REQUIRE(result.has_value());
        REQUIRE(result.value() == 6.28);
    }

    SECTION("and_then with error") {
        xll::Expected<xll::Number> exp{xll::Unexpected<xll::Error>{xll::ErrDiv0}};

        auto result = exp.and_then([](xll::Number n) {
            return xll::Expected<xll::Number>{xll::Number(n.val.num * 2.0)};
        });

        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == xll::ErrDiv0);
    }

    SECTION("and_then chain") {
        xll::Expected<xll::Number> exp{xll::Number(10.0)};

        auto result = exp
            .and_then([](xll::Number n) {
                return xll::Expected<xll::Number>{xll::Number(n.val.num / 2.0)};
            })
            .and_then([](xll::Number n) {
                return xll::Expected<xll::Number>{xll::Number(n.val.num + 5.0)};
            });

        REQUIRE(result.has_value());
        REQUIRE(result.value() == 10.0); // (10 / 2) + 5 = 10
    }

    SECTION("and_then with error propagation") {
        xll::Expected<xll::Number> exp{xll::Number(0.0)};

        auto result = exp.and_then([](xll::Number n) -> xll::Expected<xll::Number> {
            if (n.val.num == 0.0) {
                return xll::Unexpected<xll::Error>{xll::ErrDiv0};
            }
            return xll::Number(10.0 / n.val.num);
        });

        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == xll::ErrDiv0);
    }
}

TEST_CASE("Expected - or_else", "[xll::Expected][monadic]")
{
    SECTION("or_else with value") {
        xll::Expected<xll::Number> exp{xll::Number(3.14)};

        auto result = exp.or_else([](xll::Error) {
            return xll::Expected<xll::Number>{xll::Number(0.0)};
        });

        REQUIRE(result.has_value());
        REQUIRE(result.value() == 3.14);
    }

    SECTION("or_else with error") {
        xll::Expected<xll::Number> exp{xll::Unexpected<xll::Error>{xll::ErrDiv0}};

        auto result = exp.or_else([](xll::Error) {
            return xll::Expected<xll::Number>{xll::Number(99.0)};
        });

        REQUIRE(result.has_value());
        REQUIRE(result.value() == 99.0);
    }

    SECTION("or_else chain with error recovery") {
        xll::Expected<xll::Number> exp{xll::Unexpected<xll::Error>{xll::ErrDiv0}};

        auto result = exp
            .or_else([](xll::Error err) -> xll::Expected<xll::Number> {
                if (err == xll::ErrDiv0) {
                    return xll::Number(0.0); // Recover from division by zero
                }
                return xll::Unexpected<xll::Error>{err}; // Propagate other errors
            });

        REQUIRE(result.has_value());
        REQUIRE(result.value() == 0.0);
    }
}

TEST_CASE("Expected - transform", "[xll::Expected][monadic]")
{
    SECTION("transform with value") {
        xll::Expected<xll::Number> exp{xll::Number(3.14)};

        auto result = exp.transform([](xll::Number n) {
            return xll::Number(n.val.num * 2.0);
        });

        REQUIRE(result.has_value());
        REQUIRE(result.value() == 6.28);
    }

    SECTION("transform with error") {
        xll::Expected<xll::Number> exp{xll::Unexpected<xll::Error>{xll::ErrDiv0}};

        auto result = exp.transform([](xll::Number n) {
            return xll::Number(n.val.num * 2.0);
        });

        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == xll::ErrDiv0);
    }

    SECTION("transform chain") {
        xll::Expected<xll::Number> exp{xll::Number(5.0)};

        auto result = exp
            .transform([](xll::Number n) { return xll::Number(n.val.num * 2.0); })
            .transform([](xll::Number n) { return xll::Number(n.val.num + 10.0); });

        REQUIRE(result.has_value());
        REQUIRE(result.value() == 20.0); // (5 * 2) + 10 = 20
    }

    SECTION("transform type conversion") {
        xll::Expected<xll::Number> exp{xll::Number(3.14)};

        auto result = exp.transform([](xll::Number n) {
            return xll::Int(static_cast<int>(n.val.num));
        });

        REQUIRE(result.has_value());
        REQUIRE(result.value() == 3);
    }
}

TEST_CASE("Expected - transform_error", "[xll::Expected][monadic]")
{
    SECTION("transform_error with error") {
        xll::Expected<xll::Number> exp{xll::Unexpected<xll::Error>{xll::ErrDiv0}};

        auto result = exp.transform_error([](xll::Error) {
            return xll::ErrValue; // Transform error type
        });

        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == xll::ErrValue);
    }

    SECTION("transform_error with value") {
        xll::Expected<xll::Number> exp{xll::Number(3.14)};

        auto result = exp.transform_error([](xll::Error) {
            return xll::ErrValue;
        });

        REQUIRE(result.has_value());
        REQUIRE(result.value() == 3.14);
    }
}

// =============================================================================
// COMPARISON TESTS
// =============================================================================

TEST_CASE("Expected - Equality Comparison", "[xll::Expected][comparison]")
{
    SECTION("Compare two values") {
        xll::Expected<xll::Number> exp1{xll::Number(3.14)};
        xll::Expected<xll::Number> exp2{xll::Number(3.14)};
        xll::Expected<xll::Number> exp3{xll::Number(2.718)};

        REQUIRE(exp1 == exp2);
        REQUIRE_FALSE(exp1 != exp2);
        REQUIRE(exp1 != exp3);
        REQUIRE_FALSE(exp1 == exp3);
    }

    SECTION("Compare two errors") {
        xll::Expected<xll::Number> exp1{xll::Unexpected<xll::Error>{xll::ErrDiv0}};
        xll::Expected<xll::Number> exp2{xll::Unexpected<xll::Error>{xll::ErrDiv0}};
        xll::Expected<xll::Number> exp3{xll::Unexpected<xll::Error>{xll::ErrValue}};

        REQUIRE(exp1 == exp2);
        REQUIRE_FALSE(exp1 != exp2);
        REQUIRE(exp1 != exp3);
        REQUIRE_FALSE(exp1 == exp3);
    }

    SECTION("Compare value and error") {
        xll::Expected<xll::Number> exp1{xll::Number(3.14)};
        xll::Expected<xll::Number> exp2{xll::Unexpected<xll::Error>{xll::ErrDiv0}};

        REQUIRE(exp1 != exp2);
        REQUIRE_FALSE(exp1 == exp2);
    }

    SECTION("Compare with value directly") {
        xll::Expected<xll::Number> exp{xll::Number(3.14)};

        REQUIRE(exp == xll::Number(3.14));
        REQUIRE(xll::Number(3.14) == exp);
        REQUIRE(exp != xll::Number(2.718));
    }

    SECTION("Compare with Unexpected directly") {
        xll::Expected<xll::Number> exp{xll::Unexpected<xll::Error>{xll::ErrDiv0}};

        REQUIRE(exp == xll::Unexpected<xll::Error>{xll::ErrDiv0});
        REQUIRE(xll::Unexpected<xll::Error>{xll::ErrDiv0} == exp);
        REQUIRE(exp != xll::Unexpected<xll::Error>{xll::ErrValue});
    }
}

// =============================================================================
// SWAP TESTS
// =============================================================================

TEST_CASE("Expected - Swap", "[xll::Expected][swap]")
{
    SECTION("Swap two values") {
        xll::Expected<xll::Number> exp1{xll::Number(3.14)};
        xll::Expected<xll::Number> exp2{xll::Number(2.718)};

        exp1.swap(exp2);

        REQUIRE(exp1.has_value());
        REQUIRE(exp2.has_value());
        REQUIRE(exp1.value() == 2.718);
        REQUIRE(exp2.value() == 3.14);
    }

    SECTION("Swap two errors") {
        xll::Expected<xll::Number> exp1{xll::Unexpected<xll::Error>{xll::ErrDiv0}};
        xll::Expected<xll::Number> exp2{xll::Unexpected<xll::Error>{xll::ErrValue}};

        exp1.swap(exp2);

        REQUIRE_FALSE(exp1.has_value());
        REQUIRE_FALSE(exp2.has_value());
        REQUIRE(exp1.error() == xll::ErrValue);
        REQUIRE(exp2.error() == xll::ErrDiv0);
    }

    SECTION("Swap value and error") {
        xll::Expected<xll::Number> exp1{xll::Number(3.14)};
        xll::Expected<xll::Number> exp2{xll::Unexpected<xll::Error>{xll::ErrDiv0}};

        exp1.swap(exp2);

        REQUIRE_FALSE(exp1.has_value());
        REQUIRE(exp2.has_value());
        REQUIRE(exp1.error() == xll::ErrDiv0);
        REQUIRE(exp2.value() == 3.14);
    }

    SECTION("ADL swap") {
        xll::Expected<xll::Number> exp1{xll::Number(3.14)};
        xll::Expected<xll::Number> exp2{xll::Number(2.718)};

        using std::swap;
        swap(exp1, exp2);

        REQUIRE(exp1.value() == 2.718);
        REQUIRE(exp2.value() == 3.14);
    }
}

// =============================================================================
// INVALID STATE TESTS
// =============================================================================

TEST_CASE("Expected - Invalid XLOPER12 State", "[xll::Expected][invalid]")
{
    SECTION("has_value() with corrupted xltype") {
        xll::Expected<xll::Number> exp{xll::Number(3.14)};

        // Corrupt the xltype to something invalid
        exp.xltype = xltypeNil;

        // has_value() checks if xltype matches TValue::excel_type
        REQUIRE_FALSE(exp.has_value());
    }

    SECTION("has_value() with wrong value type") {
        xll::Expected<xll::Number> exp{xll::Number(3.14)};

        // Change xltype to a different value type
        exp.xltype = xltypeStr;

        // has_value() uses bitwise AND, so this depends on the types
        REQUIRE_FALSE(exp.has_value());
    }

    SECTION("value() with corrupted state throws") {
        xll::Expected<xll::Number> exp{xll::Number(3.14)};
        exp.xltype = xltypeNil; // Corrupt state

        // Should throw because has_value() returns false
        REQUIRE_THROWS(exp.value());
    }

    SECTION("error() with wrong error xltype throws") {
        xll::Expected<xll::Number> exp{xll::Unexpected<xll::Error>{xll::ErrDiv0}};

        // Corrupt xltype to non-error type
        exp.xltype = xltypeNum;

        // error() checks xltype == TError::excel_type
        REQUIRE_THROWS(exp.error());
    }

    SECTION("error() with corrupted xltype but !has_value()") {
        xll::Expected<xll::Number> exp{xll::Unexpected<xll::Error>{xll::ErrDiv0}};

        // Set xltype to something that makes has_value() false but isn't an error
        exp.xltype = xltypeNil;

        // Should throw due to type mismatch check
        REQUIRE_THROWS(exp.error());
    }

    SECTION("Construct from invalid XLOPER12") {
        // Expected should work with any valid TValue or TError construction
        // If the underlying type throws on invalid XLOPER12, Expected propagates it

        XLOPER12 invalid;
        invalid.xltype = xltypeNil;

        // Number constructor should throw on invalid XLOPER12
        REQUIRE_THROWS(xll::Expected<xll::Number>{xll::Number(invalid)});
    }
}

TEST_CASE("Expected - Type Aliases", "[xll::Expected][aliases]")
{
    SECTION("ExpNumber") {
        xll::ExpNumber exp{xll::Number(3.14)};
        REQUIRE(exp.has_value());
        REQUIRE(exp.value() == 3.14);
    }

    SECTION("ExpString") {
        xll::ExpString exp{xll::String("test")};
        REQUIRE(exp.has_value());
        REQUIRE(std::wstring(&exp.value().val.str[1]) == L"test");
    }

    SECTION("ExpInt") {
        xll::ExpInt exp{xll::Int(42)};
        REQUIRE(exp.has_value());
        REQUIRE(exp.value() == 42);
    }

    SECTION("ExpBool") {
        xll::ExpBool exp{xll::Bool(true)};
        REQUIRE(exp.has_value());
        REQUIRE(exp.value() == true);
    }
}

// =============================================================================
// EDGE CASES
// =============================================================================

TEST_CASE("Expected - Edge Cases", "[xll::Expected][edge]")
{
    SECTION("Multiple emplace calls") {
        xll::Expected<xll::Number> exp{xll::Number(1.0)};
        exp.emplace(2.0);
        exp.emplace(3.0);
        exp.emplace(4.0);

        REQUIRE(exp.has_value());
        REQUIRE(exp.value() == 4.0);
    }

    SECTION("Alternating value and error") {
        xll::Expected<xll::Number> exp{xll::Number(1.0)};

        exp = xll::Unexpected<xll::Error>{xll::ErrDiv0};
        REQUIRE_FALSE(exp.has_value());

        exp = xll::Number(2.0);
        REQUIRE(exp.has_value());

        exp = xll::Unexpected<xll::Error>{xll::ErrValue};
        REQUIRE_FALSE(exp.has_value());

        exp = xll::Number(3.0);
        REQUIRE(exp.has_value());
        REQUIRE(exp.value() == 3.0);
    }

    SECTION("Complex monadic chain") {
        auto result = xll::Expected<xll::Number>{xll::Number(10.0)}
            .transform([](xll::Number n) { return xll::Number(n.val.num * 2.0); })
            .and_then([](xll::Number n) -> xll::Expected<xll::Number> {
                if (n.val.num > 15.0) return xll::Number(n.val.num / 2.0);
                return xll::Unexpected<xll::Error>{xll::ErrValue};
            })
            .or_else([](xll::Error) {
                return xll::Expected<xll::Number>{xll::Number(999.0)};
            });

        REQUIRE(result.has_value());
        REQUIRE(result.value() == 10.0); // (10 * 2) / 2 = 10
    }

    SECTION("Boolean context in control flow") {
        xll::Expected<xll::Number> exp1{xll::Number(3.14)};
        xll::Expected<xll::Number> exp2{xll::Unexpected<xll::Error>{xll::ErrDiv0}};

        bool executed1 = false;
        bool executed2 = false;

        if (exp1) {
            executed1 = true;
        }

        if (exp2) {
            executed2 = true;
        }

        REQUIRE(executed1);
        REQUIRE_FALSE(executed2);
    }
}

TEST_CASE("Expected - Resource Management", "[xll::Expected][resources]")
{
    SECTION("String cleanup on value replacement") {
        xll::Expected<xll::String> exp{xll::String("initial")};
        auto old_ptr = exp.value().val.str;

        exp = xll::String("replaced");

        // New string should have different pointer
        REQUIRE(exp.value().val.str != old_ptr);
        REQUIRE(std::wstring(&exp.value().val.str[1]) == L"replaced");
    }

    SECTION("String cleanup on error assignment") {
        xll::Expected<xll::String> exp{xll::String("test")};

        exp = xll::Unexpected<xll::Error>{xll::ErrDiv0};

        REQUIRE_FALSE(exp.has_value());
        REQUIRE(exp.error() == xll::ErrDiv0);
    }

    SECTION("Error replacement") {
        xll::Expected<xll::Number> exp{xll::Unexpected<xll::Error>{xll::ErrDiv0}};

        exp = xll::Unexpected<xll::Error>{xll::ErrValue};
        REQUIRE(exp.error() == xll::ErrValue);

        exp = xll::Unexpected<xll::Error>{xll::ErrRef};
        REQUIRE(exp.error() == xll::ErrRef);
    }
}


