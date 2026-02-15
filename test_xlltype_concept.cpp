// Test file to verify XllType concept works correctly
// This file demonstrates that Expected only accepts valid xll types

#include "LibXLL.Library/Types/Expected.hpp"
#include "LibXLL.Library/Types/String.hpp"
#include "LibXLL.Library/Types/Number.hpp"
#include "LibXLL.Library/Types/Error.hpp"
#include "LibXLL.Library/Types/Int.hpp"

// These should compile successfully - valid xll types
void test_valid_types() {
    xll::Expected<xll::String> exp1;
    xll::Expected<xll::Number> exp2;
    xll::Expected<xll::Int> exp3;
    xll::Expected<xll::String, xll::Error> exp4;
    xll::Expected<xll::Number, xll::String> exp5;
    xll::Expected<xll::String, xll::String> exp6; // Same type for value and error
}

// Uncomment the following to test that concept properly rejects invalid types:
// (These should fail to compile with XllType concept errors)

/*
void test_invalid_types() {
    // Should fail: int is not an xll type
    xll::Expected<int> exp1;

    // Should fail: std::string is not an xll type
    xll::Expected<xll::String, std::string> exp2;

    // Should fail: double is not an xll type
    xll::Expected<double> exp3;

    // Should fail: custom class is not an xll type
    struct MyClass {};
    xll::Expected<MyClass> exp4;
}
*/

int main() {
    test_valid_types();
    return 0;
}

