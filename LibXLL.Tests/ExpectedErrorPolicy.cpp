//
// Comprehensive test of error policy functionality for xll::Expected
//
#include "catch_amalgamated.hpp"
#include "../Types/Expected.hpp"
#include "../Types/Number.hpp"
#include "../Types/String.hpp"
#include "../Types/Error.hpp"

// Custom error policy for testing
struct CustomTestPolicy {
    static constexpr xll::String create() {
        return xll::String("Custom test error");
    }
};

TEST_CASE("Expected - Error Policy Basic", "[xll::Expected][error_policy]")
{
    SECTION("Default error policy uses default constructor") {
        xll::Expected<xll::Number, xll::Error> exp;
        REQUIRE(exp.has_value());

        // Verify the type alias includes error_policy
        using policy_type = decltype(exp)::error_policy;
        static_assert(std::same_as<policy_type, xll::DefaultErrorPolicy<xll::Error>>);
    }

    SECTION("StringErrorPolicy with compile-time string") {
        using MyExpected = xll::Expected<
            xll::Number,
            xll::String,
            xll::StringErrorPolicy<"Test error message">
        >;

        MyExpected exp;
        REQUIRE(exp.has_value());

        // Verify the policy type
        using policy_type = MyExpected::error_policy;
        static_assert(std::same_as<policy_type, xll::StringErrorPolicy<"Test error message">>);
    }

    SECTION("ErrorCodePolicy with specific error code") {
        using MyExpected = xll::Expected<
            xll::Number,
            xll::Error,
            xll::ErrorCodePolicy<xll::ErrValue>
        >;

        MyExpected exp;
        REQUIRE(exp.has_value());

        // Verify the policy type
        using policy_type = MyExpected::error_policy;
        static_assert(std::same_as<policy_type, xll::ErrorCodePolicy<xll::ErrValue>>);
    }

    SECTION("Custom error policy") {
        using MyExpected = xll::Expected<
            xll::Number,
            xll::String,
            CustomTestPolicy
        >;

        MyExpected exp;
        REQUIRE(exp.has_value());

        // Verify the policy type
        using policy_type = MyExpected::error_policy;
        static_assert(std::same_as<policy_type, CustomTestPolicy>);
    }
}

TEST_CASE("Expected - Error Policy in emplace()", "[xll::Expected][error_policy][emplace]")
{
    SECTION("Default policy creates default error on emplace failure") {
        xll::Expected<xll::Number, xll::Error> exp;

        bool caught = false;
        try {
            exp.emplace([](){ throw std::runtime_error("test"); return 42.0; }());
        } catch (...) {
            caught = true;
        }

        REQUIRE(caught);
        REQUIRE_FALSE(exp.has_value());
        // Should contain default-constructed Error (ErrNull)
        REQUIRE(exp.error() == xll::ErrNull);
    }

    SECTION("StringErrorPolicy creates custom string on emplace failure") {
        using MyExpected = xll::Expected<
            xll::Number,
            xll::String,
            xll::StringErrorPolicy<"Emplace failed">
        >;

        MyExpected exp;

        bool caught = false;
        try {
            exp.emplace([](){ throw std::runtime_error("test"); return 42.0; }());
        } catch (...) {
            caught = true;
        }

        REQUIRE(caught);
        REQUIRE_FALSE(exp.has_value());
        REQUIRE(std::wstring(&exp.error().val.str[1]) == L"Emplace failed");
    }

    SECTION("ErrorCodePolicy creates specific error on emplace failure") {
        using MyExpected = xll::Expected<
            xll::Number,
            xll::Error,
            xll::ErrorCodePolicy<xll::ErrValue>
        >;

        MyExpected exp;

        bool caught = false;
        try {
            exp.emplace([](){ throw std::runtime_error("test"); return 42.0; }());
        } catch (...) {
            caught = true;
        }

        REQUIRE(caught);
        REQUIRE_FALSE(exp.has_value());
        REQUIRE(exp.error() == xll::ErrValue);
    }
}

TEST_CASE("Expected - Error Policy doesn't affect explicit construction", "[xll::Expected][error_policy]")
{
    SECTION("Unexpected construction still works normally") {
        using MyExpected = xll::Expected<
            xll::Number,
            xll::String,
            xll::StringErrorPolicy<"Default error">
        >;

        // Explicit error construction via Unexpected
        MyExpected exp{xll::Unexpected(xll::String("Explicit error"))};

        REQUIRE_FALSE(exp.has_value());
        REQUIRE(std::wstring(&exp.error().val.str[1]) == L"Explicit error");
    }

    SECTION("Value construction works normally") {
        using MyExpected = xll::Expected<
            xll::Number,
            xll::String,
            xll::StringErrorPolicy<"Default error">
        >;

        MyExpected exp{xll::Number(42.0)};

        REQUIRE(exp.has_value());
        REQUIRE(exp.value() == 42.0);
    }
}

TEST_CASE("Expected - Multiple policies for same types", "[xll::Expected][error_policy]")
{
    SECTION("Different policies create different Expected types") {
        using Policy1Expected = xll::Expected<
            xll::Number,
            xll::String,
            xll::StringErrorPolicy<"Policy 1 error">
        >;

        using Policy2Expected = xll::Expected<
            xll::Number,
            xll::String,
            xll::StringErrorPolicy<"Policy 2 error">
        >;

        // These are different types despite same value/error types
        static_assert(!std::same_as<Policy1Expected, Policy2Expected>);
    }
}

TEST_CASE("Expected - Type aliases use default policy", "[xll::Expected][error_policy]")
{
    SECTION("ExpNumber uses default policy") {
        xll::ExpNumber exp;
        using policy_type = decltype(exp)::error_policy;
        static_assert(std::same_as<policy_type, xll::DefaultErrorPolicy<xll::Error>>);
    }

    SECTION("ExpString uses default policy") {
        xll::ExpString exp;
        using policy_type = decltype(exp)::error_policy;
        static_assert(std::same_as<policy_type, xll::DefaultErrorPolicy<xll::Error>>);
    }
}

