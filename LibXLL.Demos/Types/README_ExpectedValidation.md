# XLL.EXPECTED - Excel Add-in Demo

## Overview

This Excel add-in demonstrates the use of `xll::Expected<xll::String, xll::String>` for robust error handling in Excel functions. The add-in provides a single function `XLL.EXPECTED` that validates string input against a specific format.

## Function Description

### `XLL.EXPECTED(Input)`

Validates a string against a predefined format and returns either "SUCCESS" or an Excel error.

**Parameters:**
- `Input` (String): The string to validate

**Returns:**
- `"SUCCESS"` if the string passes all validation rules
- `#VALUE!` Excel error if validation fails (error details are logged to console)

## Validation Rules

The function validates that the input string:

1. **Not Empty**: String must contain at least one character
2. **Length**: Must be between 5 and 20 characters (inclusive)
3. **First Character**: Must start with an uppercase letter (A-Z)
4. **Allowed Characters**: Contains only alphanumeric characters (a-z, A-Z, 0-9) and hyphens (-)
5. **Hyphen Position**: Must not start or end with a hyphen

## Building the Add-in

The add-in is built as part of the LibXLL.Demos project. The CMake configuration creates an `.xll` file named `expected_validation.xll`.

```bash
# Configure CMake (example using Clang)
cmake -S . -B cmake-build-debug-clang-cl -G Ninja -DCMAKE_CXX_COMPILER=clang-cl

# Build the add-in
cmake --build cmake-build-debug-clang-cl --target xllDemoExpectedValidation
```

The output file will be located at:
```
cmake-build-debug-clang-cl/LibXLL.Demos/expected_validation.xll
```

## Usage in Excel

1. **Load the Add-in**:
   - Open Excel
   - Go to `File > Options > Add-ins`
   - Click `Go...` next to "Manage: Excel Add-ins"
   - Click `Browse...` and select `expected_validation.xll`
   - Click `OK`

2. **Use the Function**:
   ```excel
   =XLL.EXPECTED("Hello123")    → Returns: "SUCCESS"
   =XLL.EXPECTED("Test")        → Returns: #VALUE! (too short)
   =XLL.EXPECTED("hello")       → Returns: #VALUE! (must start with uppercase)
   =XLL.EXPECTED("Test@123")    → Returns: #VALUE! (@ not allowed)
   =XLL.EXPECTED("-Test123")    → Returns: #VALUE! (cannot start with hyphen)
   ```

## Example Valid Inputs

- `Product-1`
- `ABC123`
- `User-Account-1`
- `TestString`
- `HELLO-WORLD`

## Example Invalid Inputs

| Input | Reason for Failure |
|-------|-------------------|
| `Test` | Too short (< 5 characters) |
| `hello` | Doesn't start with uppercase |
| `Test@123` | Contains invalid character (@) |
| `-Test` | Starts with hyphen |
| `Test-` | Ends with hyphen |
| `ThisIsAVeryLongStringThatExceedsTheLimit` | Too long (> 20 characters) |
| `` | Empty string |

## Error Logging

When validation fails, the error message is logged to the console (stdout). This is useful for debugging and monitoring. To view the console output:

1. **During Development**: 
   - Run Excel from a command prompt to see console output
   - Or use a tool like DebugView to capture OutputDebugString messages

2. **Error Message Format**:
   ```
   Validation failed: Error: String must be at least 5 characters
   ```

## Technical Details

### Architecture

This demo showcases several key features of the LibXLL library:

1. **`xll::Expected<TValue, TError>`**: A type-safe wrapper for operations that can fail, similar to `std::expected` in C++23.

2. **Metadata-based State Tracking**: Since both `TValue` and `TError` are `xll::String` (same type), the implementation uses metadata bits in the XLOPER12 structure to distinguish between value and error states.

3. **Zero-overhead Abstraction**: The `Expected` class inherits from `XLOPER12` without adding data members, ensuring binary layout compatibility with Excel's native data structures.

### Implementation Highlights

```cpp
// Validation returns Expected<String, String>
xll::Expected<xll::String, xll::String> validate_string_format(const xll::String& input) {
    if (str.length() < 5) {
        return xll::Unexpected(xll::String("Error: String must be at least 5 characters"));
    }
    // ... more validation ...
    return xll::String("SUCCESS");
}

// Excel function checks the result
if (result.has_value()) {
    return xll::Variant<xll::String, xll::Error>(result.value()) | xll::AutoFree();
} else {
    std::cout << "Validation failed: " << std::string(result.error()) << std::endl;
    return xll::Variant<xll::String, xll::Error>(xll::ErrValue) | xll::AutoFree();
}
```

### Thread Safety

The function is marked as `ThreadSafe()`, allowing Excel to call it from multiple threads simultaneously during multi-threaded calculation.

## Extending the Demo

You can modify the validation rules by editing the `validate_string_format` function in `xllDemoExpectedValidation.cpp`. For example:

```cpp
// Add custom validation rule
if (!std::regex_match(str, std::regex("^[A-Z][A-Za-z0-9-]*$"))) {
    return xll::Unexpected(xll::String("Error: Invalid format pattern"));
}
```

## Related Demos

- **DemoExpectedSameType.cpp**: Console application demonstrating `Expected<String, String>` with monadic operations
- **xllDemoTypesExpected.cpp**: Excel add-in with various `Expected` examples using different types

## Troubleshooting

### Add-in Won't Load
- Ensure you're using a compatible Excel version (Excel 2007 or later for .xll add-ins)
- Check that all dependencies are available
- Verify the build configuration matches your Excel architecture (32-bit vs 64-bit)

### Function Not Found
- Make sure the add-in is properly loaded in Excel's Add-ins dialog
- Try restarting Excel after loading the add-in

### Unexpected Errors
- Check the console output for detailed error messages
- Verify that input is a text string, not a number or other type

## License

This demo is part of the LibXLL library. See the main project LICENSE file for details.

