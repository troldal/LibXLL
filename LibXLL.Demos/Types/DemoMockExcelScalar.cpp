//
// Created by Copilot on 06/03/2026.
//
// Mock-Excel executable demonstrating scalar type validation in LibXLL.
//
// Loads scalar_validation.xll via MockExcel::XllSession, then calls each
// exported function with correct and incorrect XLOPER12 argument types.
// When the xltype doesn't match, the ensure() inside the xll:: scalar type
// operators throws std::runtime_error — demonstrating runtime type-safety.

#include <MockXL.hpp>
#include <Types.hpp>

#include <iostream>


// ============================================================================
// Helpers
// ============================================================================

// Runs a single named test case, catching and printing any exception.
void run_test(const char* description, auto callable)
{
    std::cout << "  " << description << "\n";
    try {
        callable();
    }
    catch (const std::exception& ex) {
        std::cout << "    CAUGHT EXCEPTION: " << ex.what() << "\n";
    }
    catch (...) {
        std::cout << "    CAUGHT UNKNOWN EXCEPTION\n";
    }
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char* argv[])
{
    std::cout << "=== Mock-Excel: Scalar Type Validation Demo ===\n\n";

    const std::string xll_path = (argc > 1) ? argv[1] : "scalar_validation.address.xll";

    // XllSession loads the XLL, calls xlAutoOpen, resolves xlAutoFree12.
    // Its destructor calls xlAutoClose and unloads the library.

    MockXL::Session session{ xll_path };
    std::cout << "Loaded: " << session.path().string() << "\n\n";

    // Prints the value held in an xll::Any by casting to each known type.
    auto print_any = [](const xll::Any& any) {
        if (auto v = xll::cast<xll::Number>(any))
            std::cout << "    Result: Number(" << v.value() << ")\n";
        else if (auto v = xll::cast<xll::Bool>(any))
            std::cout << "    Result: Bool(" << (v.value() ? "true" : "false") << ")\n";
        else if (auto v = xll::cast<xll::String>(any))
            std::cout << "    Result: String(\"" << v.value() << "\")\n";
        else if (auto v = xll::cast<xll::Int>(any))
            std::cout << "    Result: Int(" << v.value() << ")\n";
        else if (auto v = xll::cast<xll::Error>(any))
            std::cout << "    Result: Error(" << v.value() << ")\n";
        else if (xll::holds<xll::Missing>(any))
            std::cout << "    Result: Missing\n";
        else if (xll::holds<xll::Nil>(any))
            std::cout << "    Result: Nil\n";
        else
            std::cout << "    Result: <unknown xltype>\n";
    };

    // =================================================================
    // TEST GROUP 1: AddNumbers
    // =================================================================
    std::cout << "--- AddNumbers ---\n";

    run_test("1a. Number(3.0) + Number(4.0)  => expected: 7.0", [&] {
        xll::Number a{3.0}, b{4.0};
        print_any(session.call<"ADD.NUMBERS">(a, b));
    });

    run_test("1b. String(\"hello\") + Number(4.0)  => expected: EXCEPTION", [&] {
        xll::String a{"hello"};
        xll::Number b{4.0};
        print_any(session.call<"ADD.NUMBERS">(a, b));
    });

    run_test("1c. Bool(true) + Number(4.0)  => expected: EXCEPTION", [&] {
        xll::Bool   a{true};
        xll::Number b{4.0};
        print_any(session.call<"ADD.NUMBERS">(a, b));
    });

    run_test("1d. Error(#N/A) + Number(4.0)  => expected: EXCEPTION", [&] {
        xll::Error  a{xll::ErrNA};
        xll::Number b{4.0};
        print_any(session.call<"ADD.NUMBERS">(a, b));
    });

    run_test("1e. Int(5) + Number(4.0)  => expected: EXCEPTION", [&] {
        xll::Int    a{5};
        xll::Number b{4.0};
        print_any(session.call<"ADD.NUMBERS">(a, b));
    });

    std::cout << "\n";

    // =================================================================
    // TEST GROUP 2: NegateBool
    // =================================================================
    std::cout << "--- NegateBool ---\n";

    run_test("2a. Bool(true)  => expected: FALSE", [&] {
        xll::Bool v{true};
        print_any(session.call<"NEGATE.BOOL">(v));
    });

    run_test("2b. Number(1.0)  => expected: EXCEPTION", [&] {
        xll::Number v{1.0};
        print_any(session.call<"NEGATE.BOOL">(v));
    });

    run_test("2c. String(\"true\")  => expected: EXCEPTION", [&] {
        xll::String v{"true"};
        print_any(session.call<"NEGATE.BOOL">(v));
    });

    std::cout << "\n";

    // =================================================================
    // TEST GROUP 3: StringLength
    // =================================================================
    std::cout << "--- StringLength ---\n";

    run_test("3a. String(\"Hello\")  => expected: 5.0", [&] {
        xll::String v{"Hello"};
        print_any(session.call<"STRING.LENGTH">(v));
    });

    run_test("3b. Number(42.0)  => expected: EXCEPTION", [&] {
        xll::Number v{42.0};
        print_any(session.call<"STRING.LENGTH">(v));
    });

    run_test("3c. Bool(false)  => expected: EXCEPTION", [&] {
        xll::Bool v{false};
        print_any(session.call<"STRING.LENGTH">(v));
    });

    std::cout << "\n";

    // =================================================================
    // Summary
    // =================================================================
    std::cout << "=== Demo complete ===\n"
              << "The tests above demonstrate that LibXLL's basic/scalar types\n"
              << "throw exceptions when the underlying XLOPER12's xltype does not\n"
              << "match the expected type. This is the correct behaviour for\n"
              << "scalar types used as UDF parameters — the user cannot guarantee\n"
              << "the argument type in Excel, so runtime validation is essential.\n\n"
              << "To handle wrong types gracefully (without exceptions), use the\n"
              << "wrapper types: xll::Optional, xll::Expected, xll::Any, or xll::Variant.\n";

    return 0;
}

