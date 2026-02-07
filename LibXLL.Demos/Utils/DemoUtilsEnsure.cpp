//
// Created by Copilot on 07/02/2026.
//
// Demonstrates the usage of the ensure macro for runtime assertions

#include <iostream>
#include <Utils/Ensure.hpp>

void test_basic_ensure() {
    std::cout << "Testing basic ensure with valid condition...\n";
    int x = 10;
    ensure(x > 0);
    std::cout << "  ✓ Passed: x > 0\n\n";
}

void test_ensure_with_message() {
    std::cout << "Testing ensure with custom message...\n";
    int balance = 100;
    ensure(balance >= 0, "Account balance cannot be negative");
    std::cout << "  ✓ Passed: balance >= 0\n\n";
}

void test_failing_ensure() {
    std::cout << "Testing failing ensure (will throw exception)...\n";
    try {
        int value = -5;
        ensure(value >= 0, "Value must be non-negative");
        std::cout << "  This line should not be reached\n";
    } catch (const std::runtime_error& e) {
        std::cout << "  ✓ Exception caught as expected:\n";
        std::cout << "    " << e.what() << "\n\n";
    }
}

void test_complex_condition() {
    std::cout << "Testing ensure with complex condition...\n";
    int x = 5, y = 10;
    ensure(x < y && y < 20);
    std::cout << "  ✓ Passed: x < y && y < 20\n\n";
}

void test_pointer_check() {
    std::cout << "Testing ensure with pointer check...\n";
    int* ptr = new int(42);
    ensure(ptr != nullptr, "Pointer must not be null");
    std::cout << "  ✓ Passed: ptr != nullptr\n";
    std::cout << "  Value: " << *ptr << "\n\n";
    delete ptr;
}

void test_string_comparison() {
    std::cout << "Testing ensure with string comparison...\n";
    std::string name = "Alice";
    ensure(!name.empty(), "Name cannot be empty");
    std::cout << "  ✓ Passed: !name.empty()\n";
    std::cout << "  Name: " << name << "\n\n";
}

void test_multiple_failures() {
    std::cout << "Testing multiple failure scenarios...\n";

    // Test 1: Division by zero check
    try {
        int divisor = 0;
        ensure(divisor != 0, "Cannot divide by zero");
    } catch (const std::runtime_error& e) {
        std::cout << "  ✓ Test 1 caught: " << e.what() << "\n";
    }

    // Test 2: Range check
    try {
        int index = 100;
        int size = 10;
        ensure(index < size, "Index out of bounds");
    } catch (const std::runtime_error& e) {
        std::cout << "  ✓ Test 2 caught: " << e.what() << "\n";
    }

    // Test 3: Without custom message
    try {
        bool condition = false;
        ensure(condition);
    } catch (const std::runtime_error& e) {
        std::cout << "  ✓ Test 3 caught: " << e.what() << "\n\n";
    }
}

int main() {
    std::cout << "=== Ensure Macro Demo ===\n\n";

    try {
        test_basic_ensure();
        test_ensure_with_message();
        test_failing_ensure();
        test_complex_condition();
        test_pointer_check();
        test_string_comparison();
        test_multiple_failures();

        std::cout << "=== All tests completed successfully ===\n";
    } catch (const std::exception& e) {
        std::cerr << "Unexpected exception: " << e.what() << "\n";
        return 1;
    }

    return 0;
}


