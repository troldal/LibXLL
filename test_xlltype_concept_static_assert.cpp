// Test to verify XllType concept works correctly
#include "LibXLL.Library/Types/Expected.hpp"
#include "LibXLL.Library/Types/String.hpp"
#include "LibXLL.Library/Types/Number.hpp"
#include "LibXLL.Library/Types/Error.hpp"
#include <type_traits>

// Test that valid xll types satisfy the concept
static_assert(xll::is_xll_type<xll::String>, "String should satisfy XllType");
static_assert(xll::is_xll_type<xll::Number>, "Number should satisfy XllType");
static_assert(xll::is_xll_type<xll::Error>, "Error should satisfy XllType");
static_assert(xll::is_xll_type<xll::Int>, "Int should satisfy XllType");
static_assert(xll::is_xll_type<xll::Bool>, "Bool should satisfy XllType");

// Test that invalid types do NOT satisfy the concept
static_assert(!xll::is_xll_type<int>, "int should NOT satisfy XllType");
static_assert(!xll::is_xll_type<double>, "double should NOT satisfy XllType");
static_assert(!xll::is_xll_type<std::string>, "std::string should NOT satisfy XllType");

struct CustomType {};
static_assert(!xll::is_xll_type<CustomType>, "CustomType should NOT satisfy XllType");

int main() {
    return 0;
}

