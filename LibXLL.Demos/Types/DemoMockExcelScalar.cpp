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

    const std::string xll_path = (argc > 1) ? argv[1] : "scalar_validation.undefined.xll";

    // XllSession loads the XLL, calls xlAutoOpen, resolves xlAutoFree12.
    // Its destructor calls xlAutoClose and unloads the library.

    MockXL::Session session{ xll_path };
    std::cout << "Loaded: " << session.path().string() << "\n\n";

    // Register a mock handler for xlcAlert.
    // The real Excel would show a modal dialog; here we simply print the
    // message and type prefix to the console so automated runs stay non-interactive.
    session.register_handler(xlcAlert,
        [](const std::vector<xll::Any>& args, xll::Any& /*result*/) -> int
        {
            std::string msg  = "<no message>";
            int         type = 0;

            if (!args.empty())
                if (auto msg_opt = xll::cast<xll::String>(args[0]))
                    msg = static_cast<std::string>(*msg_opt);

            if (args.size() >= 2)
                if (auto type_opt = xll::cast<xll::Int>(args[1]))
                    type = static_cast<int>(*type_opt);

            const std::array<const char*, 4> prefix{
                "[NONE]", "[QUESTION]", "[INFORMATION]", "[ERROR]"
            };
            const char* p = (type >= 0 && type < 4)
                                ? prefix[static_cast<std::size_t>(type)]
                                : "[ALERT]";
            std::cout << "    " << p << " " << msg << "\n";
            return xlretSuccess;
        });

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
    // TEST GROUP 4: ShowAlert (xlcAlert mock)
    // =================================================================
    std::cout << "--- ShowAlert (xlcAlert) ---\n";

    run_test("4a. ShowAlert(\"Hello!\", 0)  => expected: [NONE] Hello!", [&] {
        xll::String msg{"Hello!"};
        xll::Int    type{0};
        print_any(session.call<"SHOW.ALERT">(msg, type));
    });

    run_test("4b. ShowAlert(\"Is this correct?\", 1)  => expected: [QUESTION]", [&] {
        xll::String msg{"Is this correct?"};
        xll::Int    type{1};
        print_any(session.call<"SHOW.ALERT">(msg, type));
    });

    run_test("4c. ShowAlert(\"FYI\", 2)  => expected: [INFORMATION]", [&] {
        xll::String msg{"FYI"};
        xll::Int    type{2};
        print_any(session.call<"SHOW.ALERT">(msg, type));
    });

    run_test("4d. ShowAlert(\"Something went wrong!\", 3)  => expected: [ERROR]", [&] {
        xll::String msg{"Something went wrong!"};
        xll::Int    type{3};
        print_any(session.call<"SHOW.ALERT">(msg, type));
    });

    run_test("4e. ShowAlert(Number(42), 0)  => expected: EXCEPTION (message not xltypeStr)", [&] {
        xll::Number msg{42.0};
        xll::Int    type{0};
        print_any(session.call<"SHOW.ALERT">(msg, type));
    });

    run_test("4f. ShowAlert(\"Hello\", Number(2.0))  => expected: EXCEPTION (type not xltypeInt)", [&] {
        xll::String  msg{"Hello"};
        xll::Number  type{2.0};
        print_any(session.call<"SHOW.ALERT">(msg, type));
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

