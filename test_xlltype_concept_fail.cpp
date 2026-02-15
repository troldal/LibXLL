// Test file to verify XllType concept rejects invalid types
// This file should FAIL to compile

#include "LibXLL.Library/Types/Expected.hpp"
#include "LibXLL.Library/Types/String.hpp"

// Force template instantiation to trigger concept check
template class xll::Expected<int>;

// This should fail to compile - int is not an xll type
void test_invalid_int_type() {
    xll::Expected<int> exp1;
}

int main() {
    test_invalid_int_type();
    return 0;
}


