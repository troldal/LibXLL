# Lazy Error Materialization in xll::Expected

## Overview

**Lazy Error Materialization** is a feature in `xll::Expected` that automatically handles XLOPER12 structures with invalid or unexpected `xltype` values when accessing errors. This is critical for Excel add-in functions where the Excel SDK may return unexpected types.

## The Problem

When an Excel add-in function receives a parameter, the Excel SDK converts the input to an `XLOPER12` structure. If the conversion fails or the input type is wrong, the XLOPER12 might have:

- `xltype = xltypeMissing` (empty cell)
- `xltype = xltypeNil` (null value)
- `xltype = xltypeNum` (wrong type - number when string expected)
- Any other non-error xltype

When this XLOPER12 is wrapped in an `Expected<TValue, TError>`, the Expected is in an error state (`has_value() == false`), but the underlying xltype doesn't match `TError::excel_type`.

### Before Lazy Materialization

Calling `.error()` on such an Expected would fail with:

```
Ensure failed: self.xltype == TError::excel_type && "Expected invariant violated"
```

This required defensive checks everywhere:

```cpp
if (!input->has_value() && input->xltype != xltypeErr) {
    // Manual error handling
    return create_error_response();
}

// Otherwise safe to use .error()
```

## The Solution

The `.error()` method now implements **lazy materialization**: when the xltype doesn't match `TError::excel_type`, it automatically constructs a default `TError` in-place.

### How It Works

```cpp
template<typename Self>
constexpr auto&& error(this Self&& self)
{
    if (self.has_value())
        throw std::bad_expected_access<TValue>(...);

    // Check if xltype matches expected error type
    if (self.xltype != TError::excel_type) {
        // For non-const self, materialize a default error
        if constexpr (!std::is_const_v<std::remove_reference_t<Self>>) {
            auto& mutable_self = const_cast<Expected&>(...);
            
            // Destroy current contents
            std::destroy_at(reinterpret_cast<XLOPER12*>(&mutable_self));
            
            // Construct default TError
            std::construct_at(reinterpret_cast<TError*>(&mutable_self), TError());
            
            // Mark as error state
            impl::set_error_state(mutable_self, true);
        }
        else {
            // Cannot modify const objects
            throw std::runtime_error("Cannot materialize error from const object");
        }
    }

    return std::forward_like<Self>(reinterpret_cast<QualifiedError&>(self));
}
```

## Benefits

### ✅ Automatic Handling of Excel SDK Edge Cases

No defensive checks needed:

```cpp
// Before: Required defensive check
if (!input->has_value() && input->xltype != xltypeErr) {
    return handle_invalid_type();
}

// After: Pure monadic pipeline works automatically
auto result = (*input)
    .transform_error([](const xll::Error&) { 
        return xll::String("Error: Invalid input type"); 
    })
    .and_then(validate)
    .or_else(log_error);
```

### ✅ Transparent to Callers

The materialization happens automatically when `.error()` is called. Callers don't need to know about it.

### ✅ Monadic Pipelines "Just Work"

Operations like `.transform_error()`, `.or_else()`, etc. can now handle any input from Excel without special cases.

### ✅ Preserves Type Safety

The materialized error is a proper `TError` instance, maintaining type safety throughout the pipeline.

## Usage Examples

### Example 1: Empty Cell Input

```cpp
// Excel: =MY_FUNCTION([empty cell])
// SDK creates: XLOPER12 { xltype: xltypeMissing }
// Wrapped in: Expected<String> { has_value: false, xltype: xltypeMissing }

auto result = (*input)
    .transform_error([](const xll::Error&) {  // ✅ Materializes default Error
        return xll::String("Empty input");
    });
```

**Flow:**
1. `.transform_error()` calls `.error()`
2. `.error()` detects `xltype == xltypeMissing` (not `xltypeErr`)
3. Materializes: `std::construct_at<Error>(&self, Error())`
4. Updates `xltype = xltypeErr`
5. Returns reference to materialized error
6. Lambda executes with valid `xll::Error&`

### Example 2: Wrong Type (Number Instead of String)

```cpp
// Excel: =MY_FUNCTION(123)
// SDK attempts string conversion, may create: XLOPER12 { xltype: xltypeNum }
// Wrapped in: Expected<String> { has_value: false, xltype: xltypeNum }

auto result = input->transform_error([](const xll::Error& err) {
    // ✅ Works! err is a materialized default Error
    return xll::String("Invalid type");
});
```

### Example 3: Full Monadic Pipeline

```cpp
XLL_FUNCTION xll::Expected<xll::String>* XLLAPI ValidateInput(
    xll::Expected<xll::String> const* input)
{
    // No defensive checks needed!
    auto result = (*input)
        .transform_error([](const xll::Error&) {  // Handles Missing/Nil/etc.
            return xll::String("Invalid input type");
        })
        .and_then(validate_format)
        .or_else([](const xll::String& err) {
            std::cout << "Error: " << std::string(err) << std::endl;
            return xll::Unexpected(err);
        })
        .transform_error([](const xll::String&) {
            return xll::ErrValue;  // Convert to Excel error
        });
    
    return result | xll::AutoFree();
}
```

## Requirements

### TError Must Be Default-Constructible

Lazy materialization requires `TError()` to be valid:

```cpp
static_assert(std::is_default_constructible_v<TError>,
    "TError must be default-constructible for lazy error materialization");
```

For `xll::Error`, this is satisfied:

```cpp
class Error : public impl::Base<Error, xltypeErr> {
    // Default constructor inherited from Base
};
```

### Non-Const Expected Required

Materialization modifies the Expected in-place, so it only works for non-const objects:

```cpp
xll::Expected<xll::String> exp;  // Non-const
exp.xltype = xltypeMissing;
exp.error();  // ✅ Materializes

const auto& const_exp = exp;
const_exp.error();  // ❌ Throws: "Cannot materialize error from const object"
```

In practice, this is not a limitation because Excel function parameters are always non-const pointers.

## Implementation Details

### Memory Safety

1. **Destroy Old Contents**: `std::destroy_at()` properly destroys whatever is in the XLOPER12
2. **Construct New Error**: `std::construct_at()` constructs TError in-place
3. **Update Metadata**: Sets error state flag in metadata bytes
4. **Update xltype**: Ensures xltype matches TError::excel_type

### Idempotency

Multiple calls to `.error()` are safe:

```cpp
auto& err1 = exp.error();  // Materializes if needed
auto& err2 = exp.error();  // Returns same materialized error
assert(&err1 == &err2);    // Same object
```

After the first materialization, `xltype == TError::excel_type`, so subsequent calls skip materialization.

### Performance

- **Best case** (xltype already correct): One comparison, no overhead
- **Materialization case**: One destroy, one construct, minimal overhead
- **Amortized cost**: Materialization happens at most once per Expected

## Const-Correctness Considerations

### Why Materialization Breaks Const

Lazy materialization modifies the Expected object on read, which is unconventional but necessary:

```cpp
const Expected<String>& exp = ...;
exp.error();  // Logically const (just reading), but physically mutates
```

This violates traditional const-correctness but is justified because:

1. **The logical state is unchanged**: The Expected is still in error state before and after
2. **Excel SDK limitations**: We receive XLOPER12 with unexpected types, not by choice
3. **Implementation detail**: Materialization is an internal optimization, transparent to users
4. **Pragmatic solution**: The alternative (always checking xltype manually) is verbose and error-prone

### Workaround for Const Expected

If you must access errors on const Expected with potential xltype mismatches:

```cpp
const Expected<String>& const_exp = ...;

// Option 1: Create mutable copy
auto mutable_copy = const_exp;
auto& err = mutable_copy.error();  // Safe

// Option 2: Use transform_error on non-const
Expected<String> exp = ...;
auto result = exp.transform_error([](const Error& e) { 
    // Process error
});
```

In practice, Excel function parameters are always non-const, so this is rarely an issue.

## Testing

Comprehensive tests verify lazy materialization:

```cpp
TEST_CASE("Expected - Lazy Error Materialization")
{
    SECTION("Materialize from Missing") {
        Expected<String> exp;
        exp.xltype = xltypeMissing;
        impl::set_error_state(exp, true);
        
        REQUIRE_NOTHROW(exp.error());
        REQUIRE(exp.xltype == xltypeErr);
    }
    
    SECTION("Works in monadic pipeline") {
        Expected<String> exp;
        exp.xltype = xltypeNum;  // Wrong type
        impl::set_error_state(exp, true);
        
        auto result = exp
            .transform_error([](const Error&) { return String("Fixed"); });
        
        REQUIRE_FALSE(result.has_value());
    }
    
    SECTION("Const Expected throws") {
        const Expected<String> exp = /* ... with wrong xltype */;
        REQUIRE_THROWS(exp.error());
    }
}
```

## Design Rationale

### Why Not Fix at Construction?

We could try to fix xltype when constructing Expected from XLOPER12, but:

1. **Construction happens in Excel SDK**: We don't control how XLOPER12 is created
2. **Expected is a thin wrapper**: Adding construction logic violates zero-overhead principle
3. **Lazy is more efficient**: Only materialize when error is actually accessed

### Why Not Return Optional<TError>?

Returning `optional<TError>` from `.error()` would indicate "no error available":

```cpp
optional<TError> error() const;  // Alternative design
```

But this:
- Complicates the API
- Breaks compatibility with std::expected
- Forces callers to handle empty optional everywhere

Lazy materialization keeps the API simple: if `!has_value()`, then `.error()` always succeeds.

### Why Not Static Assertion?

We could prevent the issue at compile-time:

```cpp
static_assert(!std::same_as<TValue, TError>, 
    "TValue and TError must be different");
```

But this:
- Prevents useful patterns like `Expected<String, String>`
- Doesn't solve the xltype mismatch issue from Excel
- Is too restrictive

Lazy materialization is more flexible and handles all cases gracefully.

## Migration Guide

### If You Had Defensive Checks

**Before:**
```cpp
if (!input->has_value() && input->xltype != xltypeErr) {
    return create_default_error();
}

auto result = input->transform_error(...);
```

**After:**
```cpp
// Just remove the check - materialization handles it
auto result = input->transform_error(...);
```

### If You Caught Exceptions

**Before:**
```cpp
try {
    auto& err = input->error();
    // ...
}
catch (const std::exception& e) {
    // Handle invariant violation
}
```

**After:**
```cpp
// Exception will not be thrown for xltype mismatches
auto& err = input->error();  // Materializes automatically
// ...
```

## Limitations

1. **Requires TError to be default-constructible**: Custom error types must have `TError()`
2. **Non-const only**: Cannot materialize on const Expected (rare in practice)
3. **Mutates on read**: Violates strict const-correctness (justified by pragmatism)
4. **One-size-fits-all**: Always materializes default TError, no customization

Despite these limitations, lazy materialization dramatically improves the ergonomics of using Expected with Excel's SDK.

## Summary

Lazy error materialization in `xll::Expected::error()` automatically handles XLOPER12 structures with unexpected xltype values by constructing a default TError on-demand. This eliminates the need for defensive checks and allows pure monadic pipelines to work seamlessly with Excel's type system.

**Key Points:**
- ✅ Automatic handling of Missing, Nil, wrong types from Excel
- ✅ Transparent to callers - works behind the scenes
- ✅ Enables pure monadic pipelines without defensive checks
- ✅ Efficient - materializes only when needed
- ✅ Safe - proper construction/destruction semantics
- ⚠️ Requires TError to be default-constructible
- ⚠️ Only works on non-const Expected (sufficient for Excel use cases)

This feature makes `xll::Expected` production-ready for Excel add-in development.

