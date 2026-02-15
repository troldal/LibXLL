//
// Created by Copilot on 13/02/2026.
//
// Demonstrates xll::Expected with same TValue and TError types (String, String)
// This showcases the metadata-based implementation that allows flexible type combinations

#include <iostream>
#include <string>
#include <Types/Expected.hpp>
#include <Types/String.hpp>

using namespace xll;

// ============================================================================
// Example Functions Using Expected<String, String>
// ============================================================================

/**
 * @brief Validates and normalizes a product code.
 * @return Success: normalized code (uppercase), Error: error message
 */
Expected<String, String> validate_product_code(const std::string& code) {
    if (code.empty()) {
        return Unexpected(String("Error: Product code cannot be empty"));
    }

    if (code.length() < 3) {
        return Unexpected(String("Error: Product code must be at least 3 characters"));
    }

    if (code.length() > 10) {
        return Unexpected(String("Error: Product code must be at most 10 characters"));
    }

    // Normalize to uppercase
    std::string normalized = code;
    for (auto& c : normalized) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    return String(normalized.c_str());
}

/**
 * @brief Fetches product name from database (simulated).
 * @return Success: product name, Error: error message
 */
Expected<String, String> fetch_product_name(const String& product_code) {
    std::string code = std::string(product_code);

    // Simulate database lookup
    if (code == "PROD001") {
        return String("Premium Widget");
    }
    else if (code == "PROD002") {
        return String("Standard Widget");
    }
    else if (code == "PROD003") {
        return String("Deluxe Widget");
    }
    else {
        return Unexpected(String("Error: Product code '" + code + "' not found in database"));
    }
}

/**
 * @brief Formats product name for display.
 * @return Success: formatted name, Error: error message
 */
Expected<String, String> format_product_name(const String& name) {
    std::string product_name = std::string(name);

    if (product_name.empty()) {
        return Unexpected(String("Error: Cannot format empty product name"));
    }

    // Format as: "Product: [NAME]"
    std::string formatted = "Product: [" + product_name + "]";
    return String(formatted.c_str());
}

/**
 * @brief Validates product name length.
 * @return Success: validated name, Error: error message
 */
Expected<String, String> validate_name_length(const String& name) {
    std::string product_name = std::string(name);

    if (product_name.length() > 50) {
        return Unexpected(String("Error: Product name too long (max 50 characters)"));
    }

    return name;
}

// ============================================================================
// Demo Functions
// ============================================================================

void demo_state_detection() {
    std::cout << "=== State Detection Demo ===\n\n";
    std::cout << "Demonstrating that value/error state is correctly detected\n";
    std::cout << "even when both types are xll::String.\n\n";

    // Value state
    Expected<String, String> value_result = validate_product_code("PROD001");
    std::cout << "Test 1 - Valid code:\n";
    std::cout << "  has_value(): " << (value_result.has_value() ? "true" : "false") << "\n";
    std::cout << "  operator bool(): " << (value_result ? "true" : "false") << "\n";
    if (value_result) {
        std::cout << "  value: \"" << std::string(value_result.value()) << "\"\n";
    }
    std::cout << "\n";

    // Error state
    Expected<String, String> error_result = validate_product_code("X");
    std::cout << "Test 2 - Invalid code (too short):\n";
    std::cout << "  has_value(): " << (error_result.has_value() ? "true" : "false") << "\n";
    std::cout << "  operator bool(): " << (error_result ? "true" : "false") << "\n";
    if (!error_result) {
        std::cout << "  error: \"" << std::string(error_result.error()) << "\"\n";
    }
    std::cout << "\n";

    // Another error state
    Expected<String, String> error_result2 = validate_product_code("");
    std::cout << "Test 3 - Invalid code (empty):\n";
    std::cout << "  has_value(): " << (error_result2.has_value() ? "true" : "false") << "\n";
    if (!error_result2) {
        std::cout << "  error: \"" << std::string(error_result2.error()) << "\"\n";
    }
    std::cout << "\n";
}

void demo_transform() {
    std::cout << "=== transform() Demo ===\n\n";
    std::cout << "transform() applies a function to the value if in success state.\n";
    std::cout << "If in error state, the error is propagated unchanged.\n\n";

    // Success case: transform is applied
    std::cout << "Test 1 - Transform success case:\n";
    auto result1 = validate_product_code("prod001")
        .transform([](auto&& code) {
            std::string s = std::string(std::forward<decltype(code)>(code));
            std::string prefixed = "CODE-" + s;
            return String(prefixed.c_str());
        });

    if (result1) {
        std::cout << "  Original: prod001\n";
        std::cout << "  Transformed: \"" << std::string(result1.value()) << "\"\n";
    }
    std::cout << "\n";

    // Error case: error is propagated
    std::cout << "Test 2 - Transform error case:\n";
    auto result2 = validate_product_code("AB")
        .transform([](auto&& code) {
            std::string s = "CODE-" + std::string(std::forward<decltype(code)>(code));
            return String(s.c_str());
        });

    if (!result2) {
        std::cout << "  Error propagated: \"" << std::string(result2.error()) << "\"\n";
        std::cout << "  (Transform function was NOT called)\n";
    }
    std::cout << "\n";
}

void demo_and_then() {
    std::cout << "=== and_then() Demo ===\n\n";
    std::cout << "and_then() chains operations that return Expected.\n";
    std::cout << "If in error state, the error is propagated without calling the function.\n\n";

    // Success case: chain multiple operations
    std::cout << "Test 1 - Successful chain:\n";
    auto result1 = validate_product_code("prod001")
        .and_then([](auto&& code) {
            std::cout << "  Step 1: Validated code = \"" << std::string(std::forward<decltype(code)>(code)) << "\"\n";
            return fetch_product_name(std::forward<decltype(code)>(code));
        })
        .and_then([](auto&& name) {
            std::cout << "  Step 2: Fetched name = \"" << std::string(std::forward<decltype(name)>(name)) << "\"\n";
            return format_product_name(std::forward<decltype(name)>(name));
        });

    if (result1) {
        std::cout << "  Final result: \"" << std::string(result1.value()) << "\"\n";
    }
    std::cout << "\n";

    // Failure in first step
    std::cout << "Test 2 - Failure in validation (first step):\n";
    auto result2 = validate_product_code("X")
        .and_then([](auto&& code) {
            std::cout << "  This should NOT be printed!\n";
            return fetch_product_name(std::forward<decltype(code)>(code));
        });

    if (!result2) {
        std::cout << "  Error: \"" << std::string(result2.error()) << "\"\n";
        std::cout << "  (Second function was NOT called)\n";
    }
    std::cout << "\n";

    // Failure in second step
    std::cout << "Test 3 - Failure in database lookup (second step):\n";
    auto result3 = validate_product_code("prod999")
        .and_then([](auto&& code) {
            std::cout << "  Step 1: Validated code = \"" << std::string(std::forward<decltype(code)>(code)) << "\"\n";
            return fetch_product_name(std::forward<decltype(code)>(code));
        })
        .and_then([](auto&& name) {
            std::cout << "  This should NOT be printed!\n";
            return format_product_name(std::forward<decltype(name)>(name));
        });

    if (!result3) {
        std::cout << "  Error: \"" << std::string(result3.error()) << "\"\n";
    }
    std::cout << "\n";
}

void demo_or_else() {
    std::cout << "=== or_else() Demo ===\n\n";
    std::cout << "or_else() handles errors by providing recovery logic.\n";
    std::cout << "If in success state, the value is preserved unchanged.\n\n";

    // Success case: or_else is NOT called
    std::cout << "Test 1 - Success case (or_else NOT called):\n";
    auto result1 = validate_product_code("PROD001")
        .or_else([](auto&& error) {
            std::cout << "  This should NOT be printed!\n";
            return Expected<String, String>(String("FALLBACK"));
        });

    if (result1) {
        std::cout << "  Value: \"" << std::string(result1.value()) << "\"\n";
        std::cout << "  (or_else function was NOT called)\n";
    }
    std::cout << "\n";

    // Error case: or_else provides fallback
    std::cout << "Test 2 - Error case with successful recovery:\n";
    auto result2 = validate_product_code("X")
        .or_else([](auto&& error) {
            std::cout << "  Handling error: \"" << std::string(std::forward<decltype(error)>(error)) << "\"\n";
            std::cout << "  Providing fallback value\n";
            return Expected<String, String>(String("DEFAULT"));
        });

    if (result2) {
        std::cout << "  Recovered value: \"" << std::string(result2.value()) << "\"\n";
    }
    std::cout << "\n";

    // Error case: or_else propagates different error
    std::cout << "Test 3 - Error case with error transformation:\n";
    auto result3 = validate_product_code("")
        .or_else([](auto&& error) {
            std::cout << "  Original error: \"" << std::string(std::forward<decltype(error)>(error)) << "\"\n";
            // Transform error message
            std::string critical = "CRITICAL: " + std::string(std::forward<decltype(error)>(error));
            return Expected<String, String>(
                Unexpected(String(critical.c_str()))
            );
        });

    if (!result3) {
        std::cout << "  New error: \"" << std::string(result3.error()) << "\"\n";
    }
    std::cout << "\n";
}

void demo_transform_error() {
    std::cout << "=== transform_error() Demo ===\n\n";
    std::cout << "transform_error() transforms the error if in error state.\n";
    std::cout << "If in success state, the value is preserved unchanged.\n\n";

    // Success case: error transform is NOT applied
    std::cout << "Test 1 - Success case (transform_error NOT called):\n";
    auto result1 = validate_product_code("PROD001")
        .transform_error([](auto&& error) {
            std::cout << "  This should NOT be printed!\n";
            return String("MODIFIED ERROR");
        });

    if (result1) {
        std::cout << "  Value: \"" << std::string(result1.value()) << "\"\n";
        std::cout << "  (transform_error was NOT called)\n";
    }
    std::cout << "\n";

    // Error case: error is transformed
    std::cout << "Test 2 - Error case (transform_error applied):\n";
    auto result2 = validate_product_code("AB")
        .transform_error([](auto&& error) {
            // Add timestamp prefix (simulated)
            std::string prefixed = "[2026-02-13 10:30:45] " + std::string(std::forward<decltype(error)>(error));
            return String(prefixed.c_str());
        });

    if (!result2) {
        std::cout << "  Original error: \"Error: Product code must be at least 3 characters\"\n";
        std::cout << "  Transformed error: \"" << std::string(result2.error()) << "\"\n";
    }
    std::cout << "\n";

    // Chain multiple error transformations
    std::cout << "Test 3 - Multiple error transformations:\n";
    auto result3 = validate_product_code("")
        .transform_error([](auto&& error) {
            std::string prefixed = "[VALIDATION] " + std::string(std::forward<decltype(error)>(error));
            return String(prefixed.c_str());
        })
        .transform_error([](auto&& error) {
            std::string prefixed = "[LOGGED] " + std::string(std::forward<decltype(error)>(error));
            return String(prefixed.c_str());
        });

    if (!result3) {
        std::cout << "  Final error: \"" << std::string(result3.error()) << "\"\n";
    }
    std::cout << "\n";
}

void demo_complex_pipeline() {
    std::cout << "=== Complex Pipeline Demo ===\n\n";
    std::cout << "Combining all operations in a real-world scenario.\n\n";

    auto process_product = [](const std::string& code) {
        std::cout << "Processing product code: \"" << code << "\"\n\n";

        auto result = validate_product_code(code)
            .and_then([](auto&& validated_code) {
                std::cout << "  ✓ Validation passed: " << std::string(std::forward<decltype(validated_code)>(validated_code)) << "\n";
                return fetch_product_name(std::forward<decltype(validated_code)>(validated_code));
            })
            .and_then([](auto&& product_name) {
                std::cout << "  ✓ Database lookup succeeded: " << std::string(std::forward<decltype(product_name)>(product_name)) << "\n";
                return validate_name_length(std::forward<decltype(product_name)>(product_name));
            })
            .transform([](auto&& validated_name) {
                std::cout << "  ✓ Name validation passed\n";
                std::string approved = "[APPROVED] " + std::string(std::forward<decltype(validated_name)>(validated_name));
                return String(approved.c_str());
            })
            .or_else([](auto&& error) {
                std::cout << "  ✗ Error occurred, attempting recovery...\n";
                // Try to provide a default value
                std::string error_str = std::string(std::forward<decltype(error)>(error));
                if (error_str.find("not found") != std::string::npos) {
                    return Expected<String, String>(String("[UNKNOWN PRODUCT]"));
                }
                // Otherwise propagate the error with context
                std::string fatal = "FATAL: " + error_str;
                return Expected<String, String>(
                    Unexpected(String(fatal.c_str()))
                );
            })
            .transform_error([](auto&& error) {
                std::string logged = "[ERROR LOG] " + std::string(std::forward<decltype(error)>(error));
                return String(logged.c_str());
            });

        if (result) {
            std::cout << "✓ Final result: \"" << std::string(result.value()) << "\"\n";
        } else {
            std::cout << "✗ Final error: \"" << std::string(result.error()) << "\"\n";
        }
        std::cout << "\n";
    };

    // Test various scenarios
    process_product("prod001");    // Complete success
    process_product("prod999");    // Database not found (recovers)
    process_product("X");          // Validation fails
}

// ============================================================================
// Main Demo Runner
// ============================================================================

int main() {
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  xll::Expected<String, String> Demo                       ║\n";
    std::cout << "║  Demonstrating metadata-based same-type Expected          ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n\n";

    try {
        demo_state_detection();
        std::cout << "────────────────────────────────────────────────────────────\n\n";

        demo_transform();
        std::cout << "────────────────────────────────────────────────────────────\n\n";

        demo_and_then();
        std::cout << "────────────────────────────────────────────────────────────\n\n";

        demo_or_else();
        std::cout << "────────────────────────────────────────────────────────────\n\n";

        demo_transform_error();
        std::cout << "────────────────────────────────────────────────────────────\n\n";

        demo_complex_pipeline();

        std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
        std::cout << "║  ✓ All demos completed successfully!                      ║\n";
        std::cout << "╚═══════════════════════════════════════════════════════════╝\n";

        return 0;
    }
    catch (const std::exception& ex) {
        std::cout << "✗ Exception: " << ex.what() << "\n";
        return 1;
    }
}



