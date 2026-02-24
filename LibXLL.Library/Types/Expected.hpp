/**
 * @file Expected.hpp
 * @brief Monadic error handling for Excel add-ins using Expected<TValue, TError>
 *
 * @section type_punning Type Punning and XLOPER12 Foundation
 *
 * This file implements a sophisticated type-punning design where all types (Expected, TValue, TError)
 * are fundamentally XLOPER12 objects with identical binary layout but different type-safe interfaces.
 *
 * **Critical Design Principles:**
 *
 * 1. **Binary Layout Identity**: Expected, TValue, and TError all inherit from XLOPER12 without
 *    adding any data members. This means `sizeof(Expected<T,E>) == sizeof(XLOPER12)` and they
 *    can be safely reinterpret_cast between each other.
 *
 * 2. **Type Punning via reinterpret_cast**: We use reinterpret_cast to view the same memory as
 *    different types (Expected ↔ TValue ↔ TError). This is safe because:
 *    - All types have identical size and alignment
 *    - We use std::launder after std::construct_at to obtain valid pointers
 *    - We carefully manage object lifetimes with std::construct_at/std::destroy_at
 *
 * 3. **Metadata-Based State Tracking**: Instead of relying solely on xltype, we use the last 2 bytes
 *    of the XLOPER12.val union to store metadata (magic sentinel + state flag). This allows:
 *    - Same type for TValue and TError (e.g., Expected<String, String>)
 *    - Detection of raw XLOPER12 objects from Excel
 *    - Proper state tracking independent of xltype
 *
 * 4. **Excel Compatibility**: The metadata is cleared before passing to Excel, and we can accept
 *    raw XLOPER12 objects from Excel and wrap them in Expected without copying.
 *
 * @section pointer_provenance Pointer Provenance and std::launder
 *
 * Because we construct objects via std::construct_at and access them through reinterpret_cast,
 * we must use std::launder to avoid undefined behavior from invalid pointer provenance:
 *
 * ```cpp
 * // Construction (no launder needed - pointer goes TO construct_at)
 * std::construct_at(reinterpret_cast<TValue*>(this), value);
 *
 * // Access (launder needed - pointer used AFTER construction)
 * auto* ptr = std::launder(reinterpret_cast<TValue*>(this));
 * return *ptr;  // Safe - pointer has valid provenance
 * ```
 *
 * @section memory_safety Memory Safety and Object Lifetimes
 *
 * - Every constructor calls std::construct_at to create the appropriate object
 * - The destructor calls std::destroy_at on the laundered pointer to the actual type
 * - Assignment operators use copy-and-swap for exception safety
 * - Corrupting metadata (e.g., setting xltype without proper construction) leads to undefined behavior
 *
 * @section examples Comprehensive Examples
 *
 * **Example 1: Basic Type Punning**
 * ```cpp
 * // All fundamentally XLOPER12 with identical layout:
 * Expected<Number> exp = 42.0;
 * Number* num = std::launder(reinterpret_cast<Number*>(&exp));  // Valid - same layout
 * XLOPER12* raw = &exp;  // Valid - Expected IS-A XLOPER12
 * ```
 *
 * **Example 2: Construction and Destruction with Laundering**
 * ```cpp
 * Expected<String> exp;  // Default constructor
 * // 1. XLOPER12() initializes base
 * // 2. std::construct_at(reinterpret_cast<String*>(this)) creates String
 * // 3. String constructor sets xltype = xltypeStr, allocates buffer
 * // 4. Metadata set to value state
 *
 * exp.value();  // Access with laundering
 * // 1. Check has_value() - reads metadata
 * // 2. std::launder(reinterpret_cast<String*>(this)) - get valid pointer
 * // 3. Return reference to laundered String
 *
 * // Destructor
 * // 1. Check has_value() - reads metadata (value state)
 * // 2. std::launder(reinterpret_cast<String*>(this)) - get valid pointer
 * // 3. std::destroy_at on laundered pointer - calls String destructor, frees buffer
 * // 4. Clear metadata
 * ````
 *
 * **Example 3: Same Type for Value and Error**
 * ```cpp
 * // Metadata enables this pattern:
 * Expected<String, String> validate(const String& input) {
 *     if (input.empty())
 *         return Unexpected(String("Error: empty input"));  // Error state
 *     return String("Valid: ") + input;  // Value state
 * }
 *
 * auto result = validate("");
 * // xltype == xltypeStr for both value and error
 * // Metadata distinguishes: has_value() uses metadata, not xltype
 * ```
 *
 * **Example 4: Monadic Chaining with Type Punning**
 * ```cpp
 * Expected<Number> calculate() {
 *     return Expected<String>("42")
 *         .transform([](String& s) { return std::stod(std::string(s)); })  // String→double
 *         .and_then([](double d) { return Expected<Number>(d * 2); })      // double→Expected<Number>
 *         .transform([](Number& n) { return n.val.num + 10; });            // Number→double
 * }
 *
 * // Type punning at each step:
 * // - Expected<String>: XLOPER12 with xltypeStr
 * // - Expected<double>: XLOPER12 with xltypeNum (via Number)
 * // - Expected<Number>: XLOPER12 with xltypeNum
 * // All same size, all safely reinterpret_cast-able
 * ```
 *
 * @author Kenneth Troldal Balslev
 * @date 31/03/2025
 */

#pragma once

#include "../ExcelSDK/xlcall.hpp"
#include "Bool.hpp"
#include "Error.hpp"
#include "Int.hpp"
#include "Missing.hpp"
#include "Nil.hpp"
#include "Number.hpp"
#include "String.hpp"
#include "Utils/Concepts.hpp"
#include "Utils/Metadata.hpp"
#include "Variant.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fxt.hpp>

namespace xll
{

    // Forward declaration of the Unexpected class template
    template<typename TError>
    class Unexpected;

    /**
     * @brief Concept that checks if a type is a valid error policy.
     *
     * An error policy must provide a static `create()` method that returns a TError instance.
     * This is used to create default error values when construction fails in Expected.
     *
     * @tparam TPolicy The policy type to check
     * @tparam TError The error type the policy creates
     */
    template<typename TPolicy, typename TError>
    concept ErrorPolicy = requires {
        { TPolicy::create() } -> std::same_as<TError>;
    };

    /**
     * @brief Default error policy that uses default construction.
     *
     * This policy creates error values using the default constructor of TError.
     * It's used as the default policy for Expected when no custom policy is specified.
     *
     * @tparam TError The error type to create
     *
     * @example
     * @code
     * Expected<Number, Error> exp;  // Uses DefaultErrorPolicy<Error>
     * // On construction failure, creates Error{}
     * @endcode
     */
    template<typename TError>
    struct DefaultErrorPolicy {
        static constexpr TError create() {
            return TError{};
        }
    };

    /**
     * @brief A class template that represents a value that could be either valid or an error.
     *
     * The Expected class is a monadic container similar to std::expected that
     * can either hold a value of type TValue (success state) or an error of type TError
     * (failure state). This class integrates with Excel's XLOPER12 data structure to
     * provide error handling for Excel add-in functions.
     *
     * @tparam TValue The type of value to store when in the success state (must satisfy XllType concept)
     * @tparam TError The type of error to store when in the failure state (must satisfy XllType concept), defaults to xll::Error
     * @tparam TErrorPolicy Policy for creating default error values on construction failure, defaults to DefaultErrorPolicy<TError>
     *
     * ## Type Constraints
     *
     * Both TValue and TError must satisfy the XllType concept, which enforces that they are valid
     * xll types that inherit from impl::Base<...> and ultimately from XLOPER12. Valid types include:
     * - xll::String
     * - xll::Number
     * - xll::Int
     * - xll::Bool
     * - xll::Error
     * - xll::Missing
     * - xll::Nil
     * - xll::Array<T>
     * - xll::Variant<T, Ts...>
     *
     * The TErrorPolicy must satisfy the ErrorPolicy concept, providing a static `create()` method
     * that returns a TError instance. This is used when construction fails in methods like `emplace()`.
     *
     * Attempting to instantiate Expected with non-xll types (e.g., int, std::string, custom classes)
     * will result in a compile-time error due to the XllType concept constraint.
     *
     * ## Error Policy System
     *
     * The error policy template parameter allows customization of default error values created when
     * operations like `emplace()` fail. This is particularly useful when default construction of
     * TError is insufficient or when you want specific error messages for different contexts.
     *
     * **Built-in Policies:**
     * - `DefaultErrorPolicy<TError>`: Uses default construction (e.g., `TError{}`)
     * - `StringErrorPolicy<"message">`: Creates String errors with a compile-time message
     * - `ErrorCodePolicy<ErrorCode>`: Creates specific Excel error codes (e.g., #VALUE!, #DIV/0!)
     *
     * **Example - Default Policy:**
     * ```cpp
     * Expected<Number, Error> exp;  // Uses DefaultErrorPolicy<Error>
     * exp.emplace(throw_something);  // On failure, creates Error{} (ErrNull)
     * ```
     *
     * **Example - String Error Policy:**
     * ```cpp
     * using ValidationExpected = Expected<Number, String, StringErrorPolicy<"Invalid input">>;
     * ValidationExpected exp;
     * exp.emplace(invalid_data);  // On failure, creates String("Invalid input")
     * ```
     *
     * **Example - Error Code Policy:**
     * ```cpp
     * using CalcExpected = Expected<Number, Error, ErrorCodePolicy<ErrValue>>;
     * CalcExpected exp;
     * exp.emplace(bad_calc);  // On failure, creates ErrValue (#VALUE!)
     * ```
     *
     * **Example - Custom Policy:**
     * ```cpp
     * struct MyPolicy {
     *     static constexpr String create() { return String("Custom error"); }
     * };
     * Expected<Number, String, MyPolicy> exp;
     * exp.emplace(data);  // On failure, creates String("Custom error")
     * ```
     *
     * The policy is only used when operations need to create a default error (like emplace()
     * catching an exception). Explicit error construction via Unexpected still works normally.
     *
     * ## Type Punning Architecture
     *
     * **Fundamental Design Principle:**
     * Expected<TValue, TError>, TValue, and TError are ALL fundamentally XLOPER12 objects.
     * They have identical binary layout (same size, same alignment, same memory representation)
     * but provide different type-safe interfaces. This allows us to safely reinterpret_cast
     * between these types.
     *
     * ```
     * Memory Layout (all identical):
     * Expected<String>: [xltype][val union (8 bytes)][padding]  = 16 bytes
     * String:           [xltype][val union (8 bytes)][padding]  = 16 bytes
     * Error:            [xltype][val union (8 bytes)][padding]  = 16 bytes
     * XLOPER12:         [xltype][val union (8 bytes)][padding]  = 16 bytes
     * ```
     *
     * **Type Safety Through Interfaces:**
     * - Expected provides has_value(), value(), error(), monadic operations
     * - String provides string-specific operations, ensures xltype == xltypeStr
     * - Error provides error-specific operations, ensures xltype == xltypeErr
     * - All enforce invariants through their constructors/destructors
     *
     * **Critical Invariants:**
     * 1. The object uses metadata stored in the last 2 bytes of the XLOPER12 val union
     *    to track whether it's in value or error state (metadata byte [-2] = 0xE7 sentinel,
     *    byte [-1] = 0 for value, 1 for error)
     * 2. No additional data members are added to maintain binary compatibility with XLOPER12
     * 3. The class is marked `final` to prevent derived classes from adding data members
     * 4. If `has_value()` returns true, the object contains a properly constructed TValue
     * 5. If `has_value()` returns false, the object contains a properly constructed TError
     * 6. Metadata is cleared before Excel consumption to ensure compatibility
     * 7. Object lifetime managed via std::construct_at/std::destroy_at with proper laundering
     *
     * ## Memory Safety and Pointer Provenance
     *
     * **Construction Pattern:**
     * ```cpp
     * Expected() : XLOPER12() {
     *     std::construct_at(reinterpret_cast<TValue*>(this), TValue{});
     *     // No launder needed here - we just set metadata, not access TValue
     *     impl::set_error_state(*this, false);
     * }
     * ```
     *
     * **Access Pattern:**
     * ```cpp
     * TValue& value() {
     *     // Must use std::launder to get valid pointer to constructed object
     *     auto* ptr = std::launder(reinterpret_cast<TValue*>(this));
     *     return *ptr;  // Safe - valid provenance
     * }
     * ```
     *
     * **Why std::launder is Required:**
     * After calling std::construct_at with a TValue*, the compiler knows a TValue exists.
     * But when we later reinterpret_cast from Expected*, the compiler doesn't know that
     * pointer points to the TValue. std::launder tells the compiler "yes, a TValue really
     * exists here" and provides a pointer with valid provenance.
     *
     * ## Metadata-Based State Tracking
     *
     * Unlike relying solely on xltype to distinguish value from error, we use metadata:
     * - Allows Expected<String, String> (same type for value and error)
     * - Detects raw XLOPER12 from Excel (no metadata = infer from xltype)
     * - Survives type punning (metadata independent of which "view" we use)
     * - Excel-safe (cleared before passing to Excel APIs)
     *
     * ## Excel Integration
     *
     * Excel sees only the XLOPER12 base:
     * ```cpp
     * Expected<Number> exp = 42.0;
     * exp.clear_metadata();  // Remove our metadata
     * Excel::API::call(&exp);  // Excel sees XLOPER12 with xltypeNum
     * ```
     *
     * Excel returns raw XLOPER12:
     * ```cpp
     * XLOPER12* raw = Excel::API::get_value();
     * Expected<Number> exp = *reinterpret_cast<Expected<Number>*>(raw);
     * // Works because Expected IS-A XLOPER12, just adds interface
     * ```
     *
     * ## Undefined Behavior Warnings
     *
     * **Safe Operations:**
     * ✅ All public API (constructors, value(), error(), monadic operations)
     * ✅ Reinterpret_cast between Expected/TValue/TError (identical layout)
     * ✅ Passing to Excel after clear_metadata()
     * ✅ Wrapping raw XLOPER12 from Excel
     *
     * **Undefined Behavior:**
     * ❌ Manually modifying xltype without proper construction
     * ❌ Calling impl::set_error_state with mismatched object type
     * ❌ Accessing TValue when has_value() is false
     * ❌ Accessing TError when has_value() is true
     * ❌ Destroying with corrupted metadata (wrong type will be destroyed)
     *
     * @note With metadata-based state tracking, TValue and TError can be the same type.
     *       The Unexpected wrapper disambiguates constructors.
     *
     * @see Unexpected
     * @see XLOPER12
     * @see std::launder
     * @see std::construct_at
     */
    template<typename TValue, typename TError = xll::Error, typename TErrorPolicy = DefaultErrorPolicy<TError>>
        requires is_xll_type<TValue> && is_xll_type<TError> && ErrorPolicy<TErrorPolicy, TError>
    class Expected final : public XLOPER12
    {
        // Safety checks to ensure Expected can be stored in XLOPER12
        static_assert(sizeof(TValue) <= sizeof(XLOPER12),
            "TValue is too large to fit in XLOPER12");
        static_assert(sizeof(TError) <= sizeof(XLOPER12),
            "TError is too large to fit in XLOPER12");
        static_assert(alignof(TValue) <= alignof(XLOPER12),
            "TValue alignment is incompatible with XLOPER12");
        static_assert(alignof(TError) <= alignof(XLOPER12),
            "TError alignment is incompatible with XLOPER12");
        static_assert(!std::is_polymorphic_v<TValue>,
            "TValue cannot have virtual functions (vtable would be corrupted)");
        static_assert(!std::is_polymorphic_v<TError>,
            "TError cannot have virtual functions (vtable would be corrupted)");

    public:
        using value_type      = TValue;
        using error_type      = TError;
        using error_policy    = TErrorPolicy;
        using unexpected_type = Unexpected<TError>;

        /**
         * @brief Default constructor - creates Expected in value state with default-constructed TValue.
         *
         * Initializes a new Expected object in a value state containing a default-constructed value.
         * The XLOPER12 base class is default-initialized first, then we construct the TValue in-place.
         *
         * **Type Punning Details:**
         * 1. `XLOPER12()` default-initializes the base (xltype = 0, val = {0})
         * 2. `std::construct_at(reinterpret_cast<TValue*>(this), ...)` constructs TValue in the same memory
         * 3. The TValue constructor will set xltype appropriately (e.g., xltypeNum for Number)
         * 4. We then set metadata to mark this as "value state"
         *
         * **Memory Layout After Construction:**
         * ```
         * this -> [xltype=TValue::excel_type][val union with TValue data][metadata: 0xE7, 0x00]
         *         ^                                                         ^
         *         |                                                         +-- Value state marker
         *         +-- Set by TValue constructor
         * ```
         *
         * **Why No std::launder:**
         * We don't access the constructed TValue object here - we only set metadata on *this
         * viewed as Expected/XLOPER12. Laundering is only needed when dereferencing the TValue pointer.
         *
         * @post has_value() returns true
         * @post xltype == TValue::excel_type
         * @post Metadata indicates value state
         */
        constexpr Expected() : XLOPER12()
        {
            std::construct_at(reinterpret_cast<TValue*>(this));
            impl::set_error_state(*this, false);
        }

        /**
         * @brief Copy constructor - deep copies another Expected's state and value/error.
         *
         * Constructs a new Expected object by copying the state and value from another Expected object.
         * If the source object contains a value, the new object will contain a copy of that value.
         * If the source object contains an error, the new object will contain a copy of that error.
         *
         * **Type Punning Details:**
         * 1. Check `other.has_value()` to determine which type is stored
         * 2. Call `other.value()` or `other.error()` which internally use std::launder
         * 3. Construct the appropriate type (TValue or TError) in *this
         * 4. The constructed object sets xltype, metadata is copied implicitly
         *
         * **Memory Safety:**
         * - `other.value()` and `other.error()` already handle laundering internally
         * - We receive a properly laundered reference to copy from
         * - `std::construct_at` creates a new object in *this with proper lifetime
         * - No additional laundering needed in constructor (we don't access the new object)
         *
         * @param other The Expected object to copy from (may be Expected<TValue> or convertible)
         *
         * @post has_value() == other.has_value()
         * @post If other has value, *this contains a copy of other's value
         * @post If other has error, *this contains a copy of other's error
         * @post xltype matches the contained type (TValue or TError)
         */
        constexpr Expected(const Expected& other) : XLOPER12()
        {
            if (other.has_value()) {
                std::construct_at(reinterpret_cast<TValue*>(this), other.value());
                impl::set_error_state(*this, false);
            }
            else {
                std::construct_at(reinterpret_cast<TError*>(this), other.error());
                impl::set_error_state(*this, true);
            }
        }

        /**
         * @brief Move constructor.
         *
         * Constructs a new Expected object by moving the state and value from another Expected object.
         * If the source object contains a value, the new object will contain the moved value.
         * If the source object contains an error, the new object will contain the moved error.
         * This operation is noexcept, guaranteeing that it won't throw exceptions during the move.
         *
         * @param other The Expected object to move from
         *
         * @note The xltype is set by the TValue or TError constructor, not explicitly here
         */
        constexpr Expected(Expected&& other) noexcept : XLOPER12()
        {
            if (other.has_value()) {
                std::construct_at(reinterpret_cast<TValue*>(this), std::move(other.value()));
                impl::set_error_state(*this, false);
            }
            else {
                std::construct_at(reinterpret_cast<TError*>(this), std::move(other.error()));
                impl::set_error_state(*this, true);
            }
        }

        /**
         * @brief Converting constructor from a value type.
         *
         * Constructs a new Expected object in a success state containing a copy of the provided value.
         * This constructor allows for implicit conversion from TValue to Expected<TValue, TError>,
         * making it easier to return values from functions that return Expected objects.
         * Metadata is set to mark this as a value state.
         *
         * @param t The value to store in the Expected object
         *
         * @note NOLINT annotation is used to suppress static analysis warnings about implicit conversions
         */
        constexpr Expected(const TValue& t) : XLOPER12()    // NOLINT
        {
            std::construct_at(reinterpret_cast<TValue*>(this), t);
            impl::set_error_state(*this, false);
        }

        template<typename U, typename UBase = std::remove_cvref_t<U>>
            requires std::constructible_from<TValue, U> &&
                     (!std::same_as<TValue, UBase>) &&
                     (!std::same_as<Expected, UBase>) &&
                     (!requires { typename UBase::value_type; typename UBase::error_type; })
        constexpr Expected(const U& u) : XLOPER12()
        {
            std::construct_at(reinterpret_cast<TValue*>(this), u);
            impl::set_error_state(*this, false);
        }


        constexpr Expected(TValue&& t) noexcept : XLOPER12()
        {
            std::construct_at(reinterpret_cast<TValue*>(this), std::move(t));
            impl::set_error_state(*this, false);
        }

        template<typename U, typename UBase = std::remove_cvref_t<U>>
            requires std::constructible_from<TValue, U> &&
                     (!std::same_as<TValue, UBase>) &&
                     (!std::same_as<Expected, UBase>) &&
                     (!requires { typename UBase::value_type; typename UBase::error_type; })
        constexpr Expected(U&& u) noexcept(std::is_nothrow_constructible_v<TValue, U>) : XLOPER12()
        {
            std::construct_at(reinterpret_cast<TValue*>(this), std::forward<U>(u));
            impl::set_error_state(*this, false);
        }

        /**
         * @brief Converting constructor from an Unexpected object.
         *
         * Constructs a new Expected object in an error state containing a copy of the error
         * from the provided Unexpected object. This constructor allows for implicit conversion
         * from Unexpected<UError> to Expected<TValue, TError>, facilitating error propagation
         * in functions that return Expected objects.
         * Metadata is set to mark this as an error state.
         *
         * @tparam UError The error type of the Unexpected object, defaults to TError
         * @param unexpected The Unexpected object containing the error to be stored
         */
        template<typename UError = TError>
        constexpr Expected(const Unexpected<UError>& unexpected) : XLOPER12()
        {
            std::construct_at(reinterpret_cast<TError*>(this), unexpected.error());
            impl::set_error_state(*this, true);
        }

        template<typename UError = TError>
        constexpr Expected(Unexpected<UError>&& unexpected) noexcept : XLOPER12()
        {
            std::construct_at(reinterpret_cast<TError*>(this), std::move(unexpected.error()));
            impl::set_error_state(*this, true);
        }

        /**
         * @brief Destructor - properly destroys the contained object based on state.
         *
         * Properly destroys the contained object based on whether the Expected is in a
         * value or error state. Uses std::destroy_at to explicitly destroy the active
         * object without invoking undefined behavior.
         *
         * **Critical Type Punning Operation:**
         * The destructor must determine which type (TValue or TError) actually exists in
         * memory and destroy it correctly. This is complex because:
         * - The same memory can contain TValue OR TError (union-like behavior)
         * - TValue and TError may need different cleanup (e.g., String frees memory, Number doesn't)
         * - We must call the correct destructor based on runtime state (has_value())
         *
         * **Destruction Sequence:**
         * 1. Check metadata via has_value() to determine what's stored
         * 2. If value state: reinterpret_cast to TValue*, launder it, destroy TValue
         * 3. If error state: reinterpret_cast to TError*, launder it, destroy TError
         * 4. Clear metadata for Excel safety
         *
         * **Why std::launder is REQUIRED:**
         * ```cpp
         * // Without launder (WRONG):
         * std::destroy_at(reinterpret_cast<TValue*>(this));
         * // Compiler sees: "destroy TValue at address of Expected"
         * // But pointer provenance is wrong - compiler doesn't know TValue is there
         *
         * // With launder (CORRECT):
         * std::destroy_at(std::launder(reinterpret_cast<TValue*>(this)));
         * // Compiler gets valid pointer to the actual TValue object
         * // Correct destructor called with proper pointer provenance
         * ```
         *
         * **Memory Management Examples:**
         * - TValue = String: String destructor frees heap-allocated character buffer
         * - TValue = Number: Number destructor is trivial (no cleanup needed)
         * - TError = Error: Error destructor is trivial (no cleanup needed)
         *
         * The destructor relies on metadata to determine which type to destroy:
         * - If metadata indicates value state, destroys as TValue
         * - If metadata indicates error state, destroys as TError
         * - If no metadata is present, infers from xltype
         *
         * @warning If the object's internal state is corrupted (e.g., by directly modifying
         *          xltype or calling impl::set_error_state with a mismatched type), behavior
         *          is undefined. This may result in memory leaks (destroying wrong type) or
         *          crashes (accessing invalid memory). Only use the public API to maintain
         *          object invariants.
         *
         * **Corruption Scenario Example:**
         * ```cpp
         * Expected<String> exp;  // Contains String with allocated memory
         * exp.xltype = xltypeMissing;  // CORRUPTION!
         * impl::set_error_state(exp, true);  // Says "error" but contains String
         * // Destructor thinks it contains Error, won't free String's memory → LEAK
         * ```
         *
         * @note Metadata is automatically cleared after destruction for Excel compatibility
         * @note The destructor is noexcept (as all destructors should be)
         * @note Uses const_cast internally because std::launder/std::destroy_at need non-const
         */
        constexpr ~Expected()
        {
            // Use std::launder to obtain valid pointers to objects created via std::construct_at
            if (has_value())
                std::destroy_at(std::launder(reinterpret_cast<TValue*>(this)));
            else
                std::destroy_at(std::launder(reinterpret_cast<TError*>(this)));

            impl::clear_metadata(*this);  // Safe for Excel consumption
        }

        /**
         * @brief Copy assignment operator.
         *
         * Assigns the state and value from another Expected object to this object.
         * If this and other are the same object, no operation is performed.
         * Otherwise, the current object is destroyed, and then reconstructed based on
         * the state of the other object:
         * - If the source object contains a value, this object will contain a copy of that value.
         * - If the source object contains an error, this object will contain a copy of that error.
         *
         * @param other The Expected object to copy from
         * @return Reference to this object after assignment
         */
        constexpr Expected& operator=(const Expected& other)
        {
            Expected temp(other);  // Copy in temporary (may throw)
            swap(temp);            // No-throw swap
            return *this;          // temp destroyed, cleaning up old state
        }

        /**
         * @brief Move assignment operator.
         *
         * Assigns the state and value from another Expected object to this object using move semantics.
         * If this and other are the same object, no operation is performed.
         * Otherwise, the current object is destroyed, and then reconstructed based on
         * the state of the other object:
         * - If the source object contains a value, this object will contain the moved value.
         * - If the source object contains an error, this object will contain the moved error.
         *
         * This operation is marked as noexcept, guaranteeing that it won't throw exceptions
         * during the move assignment.
         *
         * @param other The Expected object to move from
         * @return Reference to this object after assignment
         */
        constexpr Expected& operator=(Expected&& other) noexcept
        {
            Expected temp(std::move(other));  // Move construct temporary (noexcept)
            swap(temp);                        // No-throw swap
            return *this;                      // temp destroyed, cleaning up old state
        }

        /**
         * @brief Assignment operator from a value type.
         *
         * Assigns a value to this Expected object, changing it to a success state containing the provided value.
         * If the current object and the value are the same object (self-assignment detection using pointer comparison),
         * no operation is performed to avoid undefined behavior.
         * Otherwise, the current object is destroyed, and then reconstructed with the value type.
         *
         * @param other The value to store in the Expected object
         * @return Reference to this object after assignment
         *
         * @note Uses reinterpret_cast for self-assignment detection since the compared objects have different types
         */
        constexpr Expected& operator=(const TValue& other)
        {
            Expected temp(other);  // Copy-construct temporary (may throw)
            swap(temp);            // No-throw swap
            return *this;          // temp destroyed, cleaning up old state
        }

        template<typename U, typename UBase = std::remove_cvref_t<U>>
            requires std::constructible_from<TValue, U> &&
                     (!std::same_as<TValue, UBase>) &&
                     (!std::same_as<Expected, UBase>) &&
                     (!requires { typename UBase::value_type; typename UBase::error_type; })
        constexpr Expected& operator=(const U& other)
        {
            Expected temp(other);  // Copy-construct temporary (may throw)
            swap(temp);            // No-throw swap
            return *this;          // temp destroyed, cleaning up old state
        }

        constexpr Expected& operator=(TValue&& other)
            noexcept(std::is_nothrow_move_constructible_v<TValue>)  // Correct now!
        {
            Expected temp(std::move(other));
            swap(temp);
            return *this;
        }

        template<typename U, typename UBase = std::remove_cvref_t<U>>
            requires std::constructible_from<TValue, U> &&
                     (!std::same_as<TValue, UBase>) &&
                     (!std::same_as<Expected, UBase>) &&
                     (!requires { typename UBase::value_type; typename UBase::error_type; })
        constexpr Expected& operator=(U&& other)
            noexcept(std::is_nothrow_constructible_v<TValue, U>)  // Correct now!
        {
            Expected temp(std::forward<U>(other));
            swap(temp);
            return *this;
        }

        /**
         * @brief Assignment operator from an Unexpected object.
         *
         * Assigns an error to this Expected object, changing it to an error state containing
         * the error from the provided Unexpected object. Uses the copy-and-swap idiom to
         * provide strong exception safety guarantee.
         *
         * @tparam UError The error type of the Unexpected object, defaults to TError
         * @param unexpected The Unexpected object containing the error to be stored
         * @return Reference to this object after assignment
         *
         * @note This templated assignment operator allows for implicit conversion from
         *       Unexpected<UError> to Expected<TValue, TError> when assigning, facilitating
         *       error propagation in code that uses Expected objects.
         */
        template<typename UError = TError>
        constexpr Expected& operator=(const Unexpected<UError>& unexpected)
            noexcept(std::is_nothrow_copy_constructible_v<TError>)
        {
            Expected temp(unexpected);  // Copy-construct temporary (may throw)
            swap(temp);                  // No-throw swap
            return *this;                // temp destroyed, cleaning up old state
        }

        template<typename UError = TError>
        constexpr Expected& operator=(Unexpected<UError>&& unexpected)
            noexcept(std::is_nothrow_move_constructible_v<TError>)
        {
            Expected temp(std::move(unexpected));  // Move-construct temporary (may throw)
            swap(temp);                             // No-throw swap
            return *this;                           // temp destroyed, cleaning up old state
        }

        /**
         * @brief Checks if the Expected object contains a value (is in success state).
         *
         * Determines whether the Expected object is currently holding a value rather than an error.
         * This implementation uses a two-tier approach: metadata-based (preferred) and xltype-based (fallback).
         *
         * **Metadata-Based Detection (Primary):**
         * If metadata is present (magic byte 0xE7), we trust it as the source of truth:
         * - Metadata byte [-1] == 0: Value state
         * - Metadata byte [-1] == 1: Error state
         *
         * This allows Expected<String, String> to work (same type for value/error).
         *
         * **xltype-Based Detection (Fallback):**
         * If metadata is NOT present (raw XLOPER12 from Excel), we infer from xltype:
         * - xltype matches TValue::excel_type: Value state
         * - xltype doesn't match TValue::excel_type: Error state
         *
         * **Use Cases:**
         *
         * 1. **Expected we created:** Has metadata, uses metadata for accurate state
         * ```cpp
         * Expected<String> exp("hello");  // Metadata: 0xE7, 0x00 (value)
         * exp.has_value() → true  // Reads metadata
         * ```
         *
         * 2. **Raw XLOPER12 from Excel:** No metadata, infers from xltype
         * ```cpp
         * XLOPER12* raw = Excel::get_param();  // xltype = xltypeMissing
         * Expected<String>* exp = reinterpret_cast<Expected<String>*>(raw);
         * exp->has_value() → false  // xltypeMissing doesn't match xltypeStr
         * ```
         *
         * 3. **Expected<String, String>:** Requires metadata (xltype alone is ambiguous)
         * ```cpp
         * Expected<String, String> exp("value");  // Metadata: 0xE7, 0x00
         * exp.has_value() → true  // Must use metadata, xltype is xltypeStr in both cases
         * ```
         *
         * **Type Punning Context:**
         * This function treats the Expected object as an XLOPER12 (via inheritance) to read
         * xltype and metadata bytes. It doesn't matter if the actual contained object is
         * TValue or TError - we're just reading the discriminator, not accessing the object.
         *
         * @return true if the object contains a value (success state), false if it contains
         *         an error (failure state)
         *
         * @note This method is const and doesn't modify the state of the Expected object
         * @note Metadata takes precedence over xltype when present
         * @note For raw XLOPER12 from Excel, xltype mismatch indicates error state
         *
         * @see impl::has_metadata()
         * @see impl::is_error_state()
         * @see impl::is_error_from_xltype()
         */
        [[nodiscard]]
        constexpr bool has_value() const noexcept
        {

            auto error_state = xll::impl::is_error_state(*this);

            // If metadata is present, trust it
            if (impl::has_metadata(*this)) {
                return !impl::is_error_state(*this);
            }

            // Fallback: infer from xltype
            // If xltype doesn't match TValue, it's in error state (raw XLOPER12 from Excel)
            return !impl::is_error_from_xltype<TValue>(*this);
        }

        /**
         * @brief Boolean conversion operator that checks if the Expected object contains a value.
         *
         * Allows the Expected object to be used in boolean contexts, such as if statements or
         * conditional expressions. Returns true if the object is in a success state (contains a value),
         * and false if it's in an error state.
         *
         * @return true if the object contains a value (success state), false if it contains
         *         an error (failure state)
         *
         * @note This operator is marked explicit to prevent unintended implicit conversions to bool.
         *       It delegates to the has_value() method for the actual state check.
         */
        [[nodiscard]]
        constexpr explicit operator bool() const noexcept { return has_value(); }

        /**
         * @brief Retrieves the contained value if in a success state.
         *
         * Returns a reference to the contained TValue object, properly handling const-qualification
         * and value category through C++23's deducing this feature.
         *
         * **Type Punning Operation:**
         * This function performs a critical type-punning operation where we view the Expected
         * object as a TValue object:
         *
         * ```cpp
         * Expected<String>* exp_ptr = this;  // Pointer to Expected
         * String* str_ptr = reinterpret_cast<String*>(exp_ptr);  // View as String
         * String* valid_ptr = std::launder(str_ptr);  // Get valid pointer
         * return *valid_ptr;  // Access the String
         * ```
         *
         * This works because Expected and String have identical memory layout (both are XLOPER12).
         * The String was constructed in this memory via std::construct_at in the constructor.
         *
         * **Why std::launder is CRITICAL:**
         * After constructing with `std::construct_at(reinterpret_cast<TValue*>(this), ...)`,
         * the compiler knows a TValue exists. But when we later access through Expected*,
         * the compiler's pointer provenance analysis doesn't know that Expected* points to TValue.
         *
         * std::launder tells the compiler: "A TValue object really does exist at this address,
         * give me a pointer with valid provenance to access it."
         *
         * Without laundering: Undefined behavior - optimizer may assume Expected* and TValue*
         * don't alias, leading to incorrect optimizations.
         *
         * **Perfect Forwarding with std::forward_like:**
         * Uses C++23's deducing this to automatically handle all reference qualifiers:
         * - const& when called on const lvalue: `const Expected<T>& exp; exp.value()`
         * - & when called on non-const lvalue: `Expected<T>& exp; exp.value()`
         * - && when called on rvalue: `Expected<T>{}.value()` or `std::move(exp).value()`
         *
         * This single template replaces multiple overloads while maintaining the same
         * behavior through perfect forwarding.
         *
         * @tparam Self The deduced type of the Expected instance (preserves cv-qualifiers and value category)
         * @param self The Expected object to operate on (deducing this parameter)
         * @return Forwarded reference to the contained value (preserving const and value category)
         *
         * @throws std::bad_expected_access<TError> if the Expected object is in an error state
         *
         * @pre has_value() must be true, otherwise exception is thrown
         * @post Returned reference is valid as long as the Expected object lives
         *
         * @note The error thrown contains a copy of the error (obtained via laundered access)
         * @note For unchecked access without exception, use operator*()
         *
         * @see operator*() for unchecked access
         * @see std::launder for pointer provenance
         * @see std::forward_like for perfect forwarding in C++23
         */
        template<typename Self>
        [[nodiscard]]
        constexpr auto&& value(this Self&& self)
        {
            if (!self.has_value())
                throw std::bad_expected_access<TError>(*std::launder(reinterpret_cast<const TError*>(&self)));

            using QualifiedValue = std::conditional_t<
                std::is_const_v<std::remove_reference_t<Self>>,
                const TValue,
                TValue
            >;

            // Use std::launder to obtain a valid pointer to the object created by std::construct_at
            auto* ptr = std::launder(reinterpret_cast<QualifiedValue*>(&self));
            return std::forward_like<Self>(*ptr);
        }



        /**
         * @brief Retrieves the contained error if in a failure state.
         *
         * Returns a copy of the error by value. If the Expected is in an error state
         * but the xltype doesn't match TError::excel_type (which can happen when
         * Excel passes Missing, Nil, or other unexpected types), it returns a
         * default-constructed TError instead.
         *
         * **Type Punning Operation:**
         * Similar to value(), but returns by value instead of reference:
         * 1. Check has_value() - throw if in value state
         * 2. Check if xltype matches TError::excel_type
         * 3. If match: launder pointer to TError, dereference and copy
         * 4. If no match: return default-constructed TError (handles Excel edge cases)
         *
         * **Why Return By Value:**
         * Unlike value() which returns a reference, error() returns by value because:
         * - Enables graceful handling of xltype mismatches (return default TError)
         * - Simplifies lifetime management (no dangling reference issues)
         * - Common pattern in error handling (errors are typically small and copyable)
         *
         * **Excel Integration Scenario:**
         * Excel may pass XLOPER12 with unexpected xltype when parameter is missing or invalid:
         * ```cpp
         * Expected<String> param = get_from_excel();  // Excel passed xltypeMissing
         * // has_value() returns false (xltype doesn't match String::excel_type)
         * // error() is called but xltype != xltypeErr
         * // Returns default Error{} instead of crashing
         * ```
         *
         * This is critical for parameter handling where Excel's SDK may return XLOPER12
         * structures with xltype set to xltypeMissing, xltypeNil, etc., which need to be
         * converted to proper errors when accessed.
         *
         * **std::launder Usage:**
         * When xltype matches TError::excel_type, we know a proper TError was constructed.
         * We use std::launder to get a valid pointer before dereferencing and copying.
         *
         * @return Copy of the contained error, or default-constructed TError if xltype doesn't match
         *
         * @throws std::bad_expected_access<TValue> if the Expected object is in a success state
         *
         * @pre has_value() must be false, otherwise exception is thrown
         *
         * @note Requires TError to be copy-constructible and default-constructible
         * @note The exception thrown contains a copy of the value (obtained via laundered access)
         */
        [[nodiscard]]
        constexpr TError error() const
        {
            if (has_value())
                throw std::bad_expected_access<TValue>(*std::launder(reinterpret_cast<const TValue*>(this)));

            // If xltype matches TError, return a copy
            if (xltype == TError::excel_type) {
                // Use std::launder to get valid pointer to the TError object created via std::construct_at
                return *std::launder(reinterpret_cast<const TError*>(this));
            }

            // Otherwise, return a default-constructed TError
            return TErrorPolicy::create();
        }

        /**
         * @brief Dereference operator that retrieves the contained value.
         *
         * Uses C++23 deducing this to provide a convenient shorthand for accessing
         * the underlying value. Automatically handles all reference qualifiers through
         * delegation to value().
         *
         * @tparam Self The deduced type of the Expected instance
         * @param self The Expected object to operate on (deducing this parameter)
         * @return Forwarded reference to the contained value
         * @throws std::runtime_error if the Expected object is in an error state
         */
        template<typename Self>
        [[nodiscard]]
        constexpr auto&& operator*(this Self&& self)
        {
            return self.value();
        }

        /**
         * @brief Arrow operator for pointer-like access to the contained value.
         *
         * Provides convenient member access to the contained value, following the
         * smart pointer interface pattern. Allows using -> on Expected just like
         * on std::optional or smart pointers.
         *
         * @return Pointer to the contained value
         *
         * @pre has_value() must be true (undefined behavior otherwise)
         * @note Does not perform runtime checking - use value() for safe checked access
         * @note Debug builds will assert if called on Expected in error state
         * @note Follows std::optional and smart pointer conventions
         */
        [[nodiscard]]
        constexpr const TValue* operator->() const noexcept
        {
            ensure(has_value() && "operator-> called on Expected in error state");
            // Use std::launder to get valid pointer to the TValue object created via std::construct_at
            return std::launder(reinterpret_cast<const TValue*>(this));
        }

        /**
         * @brief Arrow operator for pointer-like access to the contained value (non-const).
         *
         * @return Pointer to the contained value
         * @pre has_value() must be true
         * @note Does not perform has_value() check in release builds - undefined behavior if called on error state
         */
        [[nodiscard]]
        constexpr TValue* operator->() noexcept
        {
            ensure(has_value() && "operator-> called on Expected in error state");
            // Use std::launder to get valid pointer to the TValue object created via std::construct_at
            return std::launder(reinterpret_cast<TValue*>(this));
        }

        /**
         * @brief Converts the Expected object to a fxt::expected object.
         *
         * This method transforms an xll::Expected object into a fxt::expected object,
         * preserving its state (success or error) and value/error content. The template
         * parameters allow for optional type conversion during the transformation.
         *
         * @tparam T The target value type for the resulting fxt::expected, defaults to TValue
         * @tparam E The target error type for the resulting fxt::expected, defaults to TError
         *
         * @return A fxt::expected<T, E> object containing either the converted value (if in success state)
         *         or the converted error (if in error state)
         *
         * @note This method requires that TValue be convertible to T and TError be convertible to E
         * @note Marked with [[nodiscard]] to warn if the return value is ignored
         *
         * @see fxt::expected
         * @see fxt::unexpected
         */
        template<typename T = TValue, typename E = TError>
            requires std::convertible_to<TValue, T> &&
                     std::convertible_to<TError, E>
        constexpr fxt::expected<T, E> to_expected() const
        {
            return has_value()
                ? fxt::expected<T, E>(value())
                : fxt::unexpected<E>(error());
        }

        constexpr operator fxt::expected<TValue, xll::Error>() const
        {
            return has_value()
                ? fxt::expected<TValue, xll::Error>(value())
                : fxt::unexpected<xll::Error>(error());
        }

        // constexpr operator fxt::expected<TValue, xll::String>() const
        // {
        //     if (has_value()) return fxt::expected<TValue, xll::String>(value());
        //
        //     // Type safety check before accessing error
        //     if (xltype != TError::excel_type)
        //         throw std::runtime_error("Type mismatch in conversion: Expected contains error of different type");
        //
        //     return fxt::unexpected<xll::String>(error().to_string());
        // }

        template<typename UError>
            requires std::convertible_to<TError, UError> &&
                     (!std::same_as<UError, TError>)
        constexpr operator fxt::expected<TValue, UError>() const
        {
            return has_value()
                ? fxt::expected<TValue, UError>(value())
                : fxt::unexpected<UError>(error());
        }

        /**
         * @brief Clears the metadata tag from the XLOPER12 for Excel compatibility.
         *
         * This function should be called before passing the Expected object to Excel APIs
         * to ensure the metadata bytes don't interfere with Excel's interpretation of the data.
         *
         * @note This is typically not needed in user code as the destructor automatically
         *       clears metadata. This is provided for cases where the XLOPER12 needs to be
         *       passed to Excel while the Expected object is still alive.
         *
         * @warning After calling this, has_value() will fall back to xltype checking,
         *          which may not work correctly if TValue and TError have the same xltype.
         */
        constexpr void clear_metadata() noexcept
        {
            impl::clear_metadata(*this);
        }

        /**
         * @brief Returns the contained value or a default value if in error state.
         *
         * Uses C++23 deducing this to automatically handle both const lvalue and rvalue
         * overloads. When called on a const lvalue, the value is copied; when called on
         * an rvalue, the value is moved for efficiency.
         *
         * @tparam Self The deduced type of the Expected instance
         * @tparam U Type of the default value (deduced)
         * @param self The Expected object to operate on (deducing this parameter)
         * @param default_value The value to return if in error state
         * @return The contained value (copied or moved) if in success state, otherwise the default value
         *
         * @note Uses std::forward_like for automatic copy/move selection
         * @note Perfect forwarding on default_value supports any type convertible to TValue
         */
        template<typename Self, typename U = TValue>
            requires std::constructible_from<TValue, U>
        [[nodiscard]]
        constexpr TValue value_or(this Self&& self, U&& default_value)
        {
            return self.has_value()
                ? std::forward_like<Self>(self.value())
                : TValue(std::forward<U>(default_value));
        }

        /**
         * @brief Returns the contained error or a default error if in success state.
         *
         * If the Expected is in an error state, returns the error (by value).
         * If in success state, returns the provided default error.
         *
         * @tparam U Type of the default error (deduced)
         * @param default_value The error to return if in success state
         * @return The contained error if in error state, otherwise the default error
         *
         * @note Perfect forwarding on default_value supports any type convertible to TError
         */
        template<typename U = TError>
            requires std::constructible_from<TError, U>
        [[nodiscard]]
        constexpr TError error_or(U&& default_value) const
        {
            return !has_value()
                ? error()
                : TError(std::forward<U>(default_value));
        }

        /**
         * @brief Chains another operation on the Expected object if it contains a value.
         *
         * This method implements monadic binding (similar to flatMap in other languages) for the Expected type.
         * It allows sequential composition of operations that may fail by only executing the provided function
         * if this Expected is in a success state. If this Expected contains an error, that error is propagated
         * without calling the function.
         *
         * **Type Punning and Perfect Forwarding:**
         * Uses C++23's deducing this to forward the Expected object correctly:
         * - Lvalue Expected: Copies value to function, copies error if propagating
         * - Rvalue Expected: Moves value to function, moves error if propagating
         *
         * The error propagation uses std::forward_like to maintain value category:
         * ```cpp
         * // Lvalue Expected
         * Expected<T> exp = ...;
         * exp.and_then(f);  // f receives T&, error copied if needed
         *
         * // Rvalue Expected
         * std::move(exp).and_then(f);  // f receives T&&, error moved if needed
         * Expected<T>{}.and_then(f);   // f receives T&&, error moved if needed
         * ```
         *
         * **Memory Safety:**
         * - value() call internally uses std::launder
         * - error() call internally uses std::launder
         * - Result Expected is properly constructed via its constructors
         *
         * @tparam Self The deduced type of the Expected instance (used with deducing this feature from C++23)
         * @tparam Func The type of the function to apply to the contained value
         * @tparam Result The deduced return type of the function, must be an Expected type
         *
         * @param self The Expected object to operate on (deducing this parameter)
         * @param func A callable that takes the current value and returns a new Expected object
         *
         * @return If this Expected contains a value, returns the result of applying func to that value;
         *         otherwise, returns a new Expected containing the original error
         *
         * @note Uses C++23's deducing this feature to support both lvalue and rvalue Expected objects
         * @note The function must return an Expected type with the same error type as this Expected
         * @note Uses std::forward_like for correct error forwarding (copy for lvalues, move for rvalues)
         *
         * @see transform for non-monadic mapping of the value
         * @see or_else for handling the error case
         */
        template<typename Self, typename Func, typename Result = std::invoke_result_t<Func, TValue&>>
            requires std::invocable<Func, TValue&> &&
                     requires(Func f, TValue& v) {
                         {
                             std::invoke(f, v)
                         } -> std::convertible_to<Expected<typename Result::value_type, TError>>;
                     }
        [[nodiscard]]
        constexpr auto and_then(this Self&& self, Func&& func) -> Result
        {
            return self.has_value()
                ? std::invoke(std::forward<Func>(func), self.value())
                : Result(Unexpected(std::forward_like<Self>(self.error())));
        }

        /**
         * @brief Transforms the value contained in the Expected object using a function.
         *
         * This method applies a non-monadic transformation to the value if the Expected object
         * is in a success state. If this Expected contains an error, that error is propagated
         * without calling the function. Unlike and_then(), this method wraps the result of the
         * function in a new Expected object automatically.
         *
         * @tparam Self The deduced type of the Expected instance (using C++23's deducing this)
         * @tparam Func The type of the function to apply to the contained value
         *
         * @param self The Expected object to operate on (deducing this parameter)
         * @param func A callable that takes the current value and returns a new value of any type
         * @return A new Expected object containing either the transformed value (if in success state)
         *         or the original error (if in error state)
         *
         * @note Uses C++23's deducing this feature to support both lvalue and rvalue Expected objects
         * @note Uses std::forward_like for correct error forwarding (copy for lvalues, move for rvalues)
         * @note Unlike and_then(), the function does not need to return an Expected type
         *
         * @see and_then for monadic binding operations
         * @see transform_error for transforming the error case
         */
        template<typename Self, typename Func>
            requires std::invocable<Func, TValue>
        [[nodiscard]]
        constexpr auto transform(this Self&& self, Func&& func)
        {
            using result_type          = std::invoke_result_t<Func, TValue&>;
            using expected_result_type = xll::Expected<result_type, TError>;

            return self.has_value()
                ? expected_result_type(std::invoke(std::forward<Func>(func), self.value()))
                : expected_result_type(Unexpected(std::forward_like<Self>(self.error())));
        }

        /**
         * @brief Handles the error case by applying a function to transform the error to a new Expected.
         *
         * This method provides error handling by executing the provided function only when
         * this Expected is in an error state. If the Expected contains a value, that value
         * is preserved without calling the function.
         *
         * @tparam Self The deduced type of the Expected instance (using C++23's deducing this)
         * @tparam Func A function that takes a TError and returns an Expected
         *
         * @param self The Expected object to operate on
         * @param func A callable that processes the error and returns a new Expected
         * @return The result of func if in error state; otherwise, a new Expected with the original value
         *
         * @note Complements and_then() which handles the success case
         * @note Uses C++23's deducing this feature for both lvalue and rvalue Expected objects
         * @note Uses std::forward_like for correct error forwarding (copy for lvalues, move for rvalues)
         */
        template<typename Self, typename Func>
            requires std::invocable<Func, const TError&> &&
                     std::convertible_to<std::invoke_result_t<Func, const TError&>, xll::Expected<TValue, TError>>
        [[nodiscard]]
        constexpr auto or_else(this Self&& self, Func&& func)
        {
            using result_type = std::invoke_result_t<Func, const TError&>;

            return !self.has_value()
                ? std::invoke(std::forward<Func>(func), std::forward_like<Self>(self.error()))
                : result_type(self.value());
        }

        /**
         * @brief Transforms the error contained in the Expected object using a function.
         *
         * This method applies a transformation function to the error if the Expected object
         * is in an error state. If the Expected contains a value, that value is preserved
         * without calling the function.
         *
         * @tparam Self The deduced type of the Expected instance (using C++23's deducing this)
         * @tparam Func The type of the function to apply to the contained error
         *
         * @param self The Expected object to operate on
         * @param func A callable that takes the current error and returns a new error of any type
         * @return A new Expected object containing either the original value (if in success state)
         *         or the transformed error (if in error state)
         *
         * @note Uses C++23's deducing this feature for both lvalue and rvalue Expected objects
         * @note Uses std::forward_like for correct error forwarding (copy for lvalues, move for rvalues)
         */
        template<typename Self, typename Func>
            requires std::invocable<Func, const TError&>
        [[nodiscard]]
        constexpr auto transform_error(this Self&& self, Func&& func)
        {
            using result_type          = std::invoke_result_t<Func, const TError&>;
            using expected_result_type = xll::Expected<TValue, result_type>;

            return !self.has_value()
                ? expected_result_type(Unexpected(std::invoke(std::forward<Func>(func), std::forward_like<Self>(self.error()))))
                : expected_result_type(self.value());
        }

        /**
         * @brief Constructs the value in-place, destroying any existing value or error.
         *
         * Destroys the currently contained value or error, then constructs a new value
         * in-place using the provided arguments. If construction throws, the Expected
         * is left in an error state containing an error created by the TErrorPolicy.
         *
         * @tparam Args Types of arguments to forward to TValue's constructor
         * @param args Arguments to forward to the TValue constructor
         *
         * @throws Any exception thrown by TValue's constructor (Expected will contain error from policy)
         * @note Provides basic exception safety - if construction throws, Expected contains error from TErrorPolicy::create()
         * @note The error policy determines what error value is created on construction failure
         */
        template<typename... Args>
            requires std::constructible_from<TValue, Args...>
        constexpr void emplace(Args&&... args)
        {
            // Destroy the active member - use std::launder to get valid pointer
            if (has_value())
                std::destroy_at(std::launder(reinterpret_cast<TValue*>(this)));
            else
                std::destroy_at(std::launder(reinterpret_cast<TError*>(this)));

            // Attempt to construct new value
            try {
                std::construct_at(reinterpret_cast<TValue*>(this), std::forward<Args>(args)...);
                impl::set_error_state(*this, false);  // Mark as value state
            }
            catch (...) {
                // Construction failed - use error policy to create default error
                std::construct_at(reinterpret_cast<TError*>(this), TErrorPolicy::create());
                impl::set_error_state(*this, true);  // Mark as error state
                throw;  // Rethrow original exception
            }
        }

        /**
         * @brief Constructs the error in-place, destroying any existing value or error.
         *
         * Destroys the currently contained value or error, then constructs a new error
         * in-place using the provided arguments. This is more efficient than assignment
         * when constructing complex error objects.
         *
         * @tparam Args Types of arguments to forward to TError's constructor
         * @param args Arguments to forward to the TError constructor
         */
        template<typename... Args>
            requires std::constructible_from<TError, Args...> &&
                     std::is_default_constructible_v<TValue>  // ✅ Removed "nothrow" requirement
        constexpr void emplace_error(Args&&... args)
        {
            // Destroy the active member - use std::launder to get valid pointer
            if (has_value())
                std::destroy_at(std::launder(reinterpret_cast<TValue*>(this)));
            else
                std::destroy_at(std::launder(reinterpret_cast<TError*>(this)));

            try {
                std::construct_at(reinterpret_cast<TError*>(this), std::forward<Args>(args)...);
                impl::set_error_state(*this, true);  // Mark as error state
            }
            catch (...) {
                std::construct_at(reinterpret_cast<TValue*>(this));
                impl::set_error_state(*this, false);  // Mark as value state
                throw;
            }
        }

        /**
         * @brief Swaps the contents of two Expected objects.
         *
         * Efficiently exchanges the contents of this Expected with another.
         * Handles all four cases: value-value, error-error, value-error, error-value.
         *
         * **Type Punning Insight:**
         * This function demonstrates why the XLOPER12-based design is so powerful.
         * We can swap two Expected objects by simply swapping their XLOPER12 bases:
         *
         * ```cpp
         * swap(static_cast<XLOPER12&>(*this), static_cast<XLOPER12&>(other));
         * ```
         *
         * This single operation swaps:
         * - xltype (the type discriminator)
         * - val (the entire 8-byte union containing data)
         * - Metadata (stored in val's last 2 bytes)
         *
         * **Why This Works:**
         * Because Expected, TValue, and TError are all fundamentally XLOPER12 objects
         * with identical layout, swapping the XLOPER12 base is equivalent to swapping
         * the entire object, regardless of whether it contains TValue or TError.
         *
         * **Memory Safety:**
         * No laundering needed because:
         * - We're not constructing or destroying objects
         * - We're not accessing TValue or TError members
         * - We're just byte-swapping the XLOPER12 structure
         * - Object lifetimes remain valid throughout
         *
         * **Efficiency:**
         * Swapping XLOPER12 is typically 16 bytes (xltype + val union), which is:
         * - More efficient than destroy-construct-construct pattern
         * - No exception safety concerns (noexcept if moves are noexcept)
         * - No memory allocation
         * - Just a few register moves
         *
         * @param other The Expected object to swap with
         *
         * @note noexcept specification depends on move constructibility of TValue and TError,
         *       but in practice swapping XLOPER12 is always noexcept
         * @note After swap, this contains other's previous value and vice versa
         * @note Both objects remain in valid states (no partial swap)
         */
        constexpr void swap(Expected& other)
            noexcept(std::is_nothrow_move_constructible_v<TValue> &&
                     std::is_nothrow_move_constructible_v<TError>)
        {
            // Swap the entire XLOPER12 base structure
            // This is safe and efficient because Expected, TValue, and TError all have
            // identical binary layout to XLOPER12 (no additional data members).
            // Swapping the base swaps both xltype and the entire union in one operation.
            using std::swap;
            swap(static_cast<XLOPER12&>(*this), static_cast<XLOPER12&>(other));
        }

        /**
         * @brief Equality comparison operator.
         *
         * Compares two Expected objects for equality. Two Expected objects are equal if:
         * - Both contain values and the values are equal, OR
         * - Both contain errors and the errors are equal
         *
         * @param lhs The left Expected object
         * @param rhs The right Expected object
         * @return true if both Expected objects are in the same state with equal contents
         *
         * @note Delegates validation to value() and error() accessors
         * @note If class invariants are maintained, no additional type checking is needed
         */
        [[nodiscard]]
        friend constexpr bool operator==(const Expected& lhs, const Expected& rhs)
            requires std::equality_comparable<TValue> && std::equality_comparable<TError>
        {
            if (lhs.has_value() != rhs.has_value()) return false;

            return lhs.has_value()
                ? lhs.value() == rhs.value()
                : lhs.error() == rhs.error();
        }

        /**
         * @brief Equality comparison with a value.
         *
         * An Expected is equal to a value if it contains that value (is in success state).
         *
         * @param lhs The Expected object
         * @param rhs The value to compare with
         * @return true if the Expected contains a value equal to the given value
         */
        [[nodiscard]]
        friend constexpr bool operator==(const Expected& lhs, const TValue& rhs)
            requires std::equality_comparable<TValue>
        {
            return lhs.has_value() && lhs.value() == rhs;
        }

        /**
         * @brief Equality comparison with an Unexpected.
         *
         * An Expected is equal to an Unexpected if it contains that error (is in error state).
         *
         * @param lhs The Expected object
         * @param rhs The Unexpected to compare with
         * @return true if the Expected contains an error equal to the given Unexpected's error
         */
        [[nodiscard]]
        friend constexpr bool operator==(const Expected& lhs, const Unexpected<TError>& rhs)
            requires std::equality_comparable<TError>
        {
            return !lhs.has_value() && lhs.error() == rhs.error();
        }
    };

    namespace impl
    {
        // Helper trait to detect if a type is a specialization of xll::Expected
        template<typename T>
        struct is_expected_impl : std::false_type
        {
        };

        template<typename TValue, typename TError, typename TErrorPolicy>
        struct is_expected_impl<Expected<TValue, TError, TErrorPolicy>> : std::true_type
        {
        };

        template<typename T>
        inline constexpr bool is_expected_v = is_expected_impl<std::remove_cvref_t<T>>::value;
    }

    /**
     * @brief Concept to check if a type is a specialization of xll::Expected.
     *
     * This concept evaluates to true if and only if the type T (after removing cv-qualifiers
     * and references) is a specialization of xll::Expected. It will not match other
     * expected-like types such as std::expected or fxt::expected.
     *
     * @tparam T The type to check
     *
     * @note This concept can be used in requires clauses, if constexpr conditions, or
     *       as a constraint on template parameters.
     * @note The concept automatically removes cv-qualifiers and references from T before checking.
     *
     * @example
     * @code
     * template<IsExpected E>
     * void process(E&& expected) {
     *     // This function only accepts xll::Expected types
     * }
     *
     * xll::Expected<xll::Number> ex = 42.0;
     * process(ex); // OK
     *
     * std::expected<int, xll::Error> stdEx = 5;
     * process(stdEx); // Compile error - not xll::Expected
     *
     * int x = 5;
     * process(x); // Compile error
     * @endcode
     */
    template<typename T>
    concept IsExpected = impl::is_expected_v<T>;

    /**
     * @brief Non-member swap function for Expected objects.
     *
     * Provides ADL-friendly swap that delegates to the member swap function.
     * Enables use with standard algorithms and generic code.
     *
     * @param lhs First Expected object to swap
     * @param rhs Second Expected object to swap
     */
    template<typename TValue, typename TError>
    constexpr void swap(Expected<TValue, TError>& lhs, Expected<TValue, TError>& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }


    /**
     * @brief A class template that represents an unexpected error value.
     *
     * The Unexpected class is a wrapper for error values in the Expected monad.
     * This class is designed to work with the Expected class to represent failure cases
     * and provides constructor disambiguation when TValue and TError are the same type.
     *
     * **Purpose in Type System:**
     * When Expected<String, String> is used (same type for value and error), constructors
     * would be ambiguous:
     * ```cpp
     * Expected<String, String> exp("error");  // Value or error?
     * ```
     *
     * Unexpected resolves this:
     * ```cpp
     * Expected<String, String> exp("success");  // Value (direct construction)
     * Expected<String, String> exp(Unexpected("error"));  // Error (wrapped)
     * ```
     *
     * **Not an XLOPER12 Type:**
     * Unlike Expected, TValue, and TError which are all XLOPER12 objects, Unexpected
     * is a simple wrapper class that stores a TError member. It's used only for
     * construction disambiguation and error propagation.
     *
     * **Memory Layout:**
     * ```
     * Unexpected<Error>: [TError member (16 bytes)]  = 16 bytes
     * vs.
     * Expected<...>:     [XLOPER12 base (16 bytes)]  = 16 bytes
     * ```
     * They have same size but different purposes and layouts.
     *
     * @tparam TError The type of error to store (typically an XLOPER12-derived type like Error)
     *
     * @note With metadata-based Expected, any XLOPER12-derived type can be TError
     * @note Unexpected is copyable and movable if TError is
     * @note Provides equality comparison based on contained error
     *
     * @see Expected
     * @see Expected's constructors taking Unexpected
     */
    template<typename TError>
    class Unexpected
    {
    public:
        using error_type = TError;

        /**
         * @brief Constructs an Unexpected object from a const reference to an error.
         *
         * Creates a new Unexpected object by copying the provided error value.
         * This constructor is marked explicit to prevent implicit conversions.
         *
         * @param error The error value to store in this Unexpected object
         */
        constexpr explicit Unexpected(const TError& error) : error_(error) {}

        /**
         * @brief Constructs an Unexpected object from an rvalue reference to an error.
         *
         * Creates a new Unexpected object by moving the provided error value.
         * This constructor is marked explicit to prevent implicit conversions
         * and uses move semantics for potentially better performance.
         *
         * @param error The error value to move into this Unexpected object
         */
        constexpr explicit Unexpected(TError&& error) : error_(std::move(error)) {}

        /**
         * @brief Copy constructor for the Unexpected class.
         *
         * Creates a new Unexpected object by copying the error value from another Unexpected object.
         * This constructor is defaulted to use the compiler-generated implementation
         * that performs a member-wise copy of the error_ field.
         *
         * @param _ The Unexpected object to copy from
         */
        constexpr Unexpected(const Unexpected& _) = default;

        /**
         * @brief Copy assignment operator for the Unexpected class.
         *
         * Assigns the error value from another Unexpected object to this object.
         * This operator is defaulted to use the compiler-generated implementation
         * that performs a member-wise copy assignment of the error_ field.
         *
         * @param _ The Unexpected object to copy from
         * @return Reference to this object after assignment
         */
        constexpr Unexpected& operator=(const Unexpected& _) = default;

        /**
         * @brief Move constructor for the Unexpected class.
         *
         * Creates a new Unexpected object by moving the error value from another Unexpected object.
         * This constructor is defaulted to use the compiler-generated implementation
         * that performs a member-wise move of the error_ field.
         * The noexcept specifier guarantees that this operation won't throw exceptions.
         *
         * @param _ The Unexpected object to move from
         */
        constexpr Unexpected(Unexpected&& _) noexcept = default;

        /**
         * @brief Move assignment operator for the Unexpected class.
         *
         * Assigns the error value from another Unexpected object to this object using move semantics.
         * This operator is defaulted to use the compiler-generated implementation
         * that performs a member-wise move assignment of the error_ field.
         * The noexcept specifier guarantees that this operation won't throw exceptions.
         *
         * @param _ The Unexpected object to move from
         * @return Reference to this object after assignment
         */
        constexpr Unexpected& operator=(Unexpected&& _) noexcept = default;

        /**
         * @brief Destructor for the Unexpected class.
         *
         * This defaulted destructor enables proper cleanup of Unexpected objects.
         * Since the class doesn't manage any resources directly (the contained error
         * object handles its own cleanup), a default implementation is sufficient.
         *
         * @note Marked as default to use the compiler-generated implementation
         */
        constexpr ~Unexpected() = default;

        /**
         * @brief Accessor for the error value with perfect forwarding.
         *
         * Uses C++23 deducing this to automatically handle all reference qualifiers:
         * - const& when called on const lvalue
         * - & when called on non-const lvalue
         * - const&& when called on const rvalue
         * - && when called on non-const rvalue
         *
         * This single template replaces four overloads while maintaining the same
         * behavior through perfect forwarding using std::forward_like.
         *
         * @tparam Self The deduced type of the Unexpected instance (preserves cv-qualifiers and value category)
         * @param self The Unexpected object to operate on (deducing this parameter)
         * @return Forwarded reference to the contained error (preserving const and value category)
         */
        template<typename Self>
        [[nodiscard]]
        constexpr auto&& error(this Self&& self) noexcept
        {
            return std::forward_like<Self>(self.error_);
        }

        /**
         * @brief Equality comparison operator for Unexpected objects.
         *
         * Compares two Unexpected objects for equality by comparing their contained error values.
         * Two Unexpected objects are equal if their error values are equal.
         *
         * @param lhs The left Unexpected object
         * @param rhs The right Unexpected object
         * @return true if both Unexpected objects contain equal errors
         *
         * @note Requires TError to be equality comparable
         * @note The noexcept specification depends on whether TError comparison is noexcept
         * @note Uses public error() accessor in noexcept specification for GCC compatibility
         */
        [[nodiscard]]
        friend constexpr bool operator==(const Unexpected& lhs, const Unexpected& rhs)
            noexcept(noexcept(lhs.error() == rhs.error()))
            requires std::equality_comparable<TError>
        {
            return lhs.error_ == rhs.error_;
        }

        /**
         * @brief Swaps the contents of two Unexpected objects.
         *
         * This friend function provides a specialized swap implementation for Unexpected objects
         * that exchanges their error values using std::swap. It follows the common C++ idiom of
         * using ADL (Argument Dependent Lookup) by first bringing std::swap into scope and then
         * calling an unqualified swap.
         *
         * @param lhs First Unexpected object to swap
         * @param rhs Second Unexpected object to swap
         *
         * @note This function is marked as constexpr to enable compile-time evaluation when possible
         * @note The noexcept specification depends on whether the underlying error type can be
         *       swapped without throwing exceptions (std::is_nothrow_swappable_v<TError>)
         */
        friend constexpr void swap(Unexpected& lhs, Unexpected& rhs) noexcept(std::is_nothrow_swappable_v<TError>)
        {
            using std::swap;
            swap(lhs.error_, rhs.error_);
        }

    private:
        TError error_;
    };

    /**
     * @brief Deduction guide for the Unexpected class template.
     *
     * This deduction guide allows for automatic template parameter deduction when
     * constructing an Unexpected object. When an Unexpected object is created with
     * a value of type TError, the compiler can automatically deduce the template
     * parameter without requiring explicit specification.
     *
     * @tparam TError The type of error to be stored in the Unexpected object
     *
     * @note This enables more concise code like `Unexpected(Error())` instead of
     *       `Unexpected<Error>(Error())`, reducing verbosity and improving readability
     * @note Particularly useful in the context of Expected's error handling where
     *       it allows for more natural error creation syntax
     */
    template<typename TError>
    Unexpected(TError) -> Unexpected<TError>;

    /**
     * @brief Type alias for Expected<Number> representing numeric values that may contain errors.
     *
     * This type provides a convenient shorthand for Expected<Number>, representing
     * numeric values that might be in an error state.
     */
    using ExpNumber = Expected<Number, xll::Error>;

    /**
     * @brief Type alias for Expected<String> representing string values that may contain errors.
     *
     * This type provides a convenient shorthand for Expected<String>, representing
     * string values that might be in an error state.
     */
    using ExpString = Expected<String, xll::Error>;

    /**
     * @brief Type alias for Expected<Int> representing integer values that may contain errors.
     *
     * This type provides a convenient shorthand for Expected<Int>, representing
     * integer values that might be in an error state.
     */
    using ExpInt = Expected<Int, xll::Error>;

    /**
     * @brief Type alias for Expected<Bool> representing boolean values that may contain errors.
     *
     * This type provides a convenient shorthand for Expected<Bool>, representing
     * boolean values that might be in an error state.
     */
    using ExpBool = Expected<Bool, xll::Error>;

    /**
     * @brief Pipe operator for chaining operations on xll::Expected objects.
     *
     * Allows piping an Expected object through a callable using the | operator.
     * This enables a more readable functional programming style with the Expected monad,
     * supporting fluent chaining of operations. The operator uses perfect forwarding to
     * preserve the value category of both the Expected object and the callable.
     *
     * @tparam TExpected The Expected type (must be a specialization of xll::Expected)
     * @tparam Callable The type of callable to apply to the Expected object
     *
     * @param expected The Expected object to pipe through the callable (forwarding reference)
     * @param function The callable to apply to the Expected object (forwarding reference)
     * @return The result of invoking the callable with the Expected object
     *
     * @note The IsExpected concept ensures only xll::Expected specializations are accepted
     * @note Uses perfect forwarding to support lvalue, rvalue, const, and non-const Expected objects
     * @note The callable must be invocable with the Expected object
     *
     * @example
     * @code
     * xll::Expected<xll::Number> ex = 42.0;
     * auto result = ex | and_then([](auto val) { return xll::Expected<xll::Number>(val * 2); })
     *                  | transform([](auto val) { return val + 10; });
     * @endcode
     */
    template<typename TExpected, typename Callable>
        requires std::invocable<Callable, TExpected> &&
                 IsExpected<std::remove_cvref_t<TExpected>>
    [[nodiscard]]
    constexpr auto operator|(TExpected&& expected, Callable&& function)
        -> decltype(std::invoke(std::forward<Callable>(function), std::forward<TExpected>(expected)))
    {
        return std::invoke(std::forward<Callable>(function), std::forward<TExpected>(expected));
    }

    /**
     * @brief Creates a higher-order function for monadic binding on Expected objects.
     *
     * This function returns a closure that applies the provided function using the and_then
     * method of an Expected object. It enables point-free programming style and function
     * composition with the Expected monad, allowing operations to be chained together with
     * the pipe operator.
     *
     * @tparam TFunction The type of the function to be applied to the Expected's value
     *
     * @param f The function to apply to the value inside an Expected if it's in a success state.
     *          This function should take a value of type TValue and return an Expected.
     *
     * @return A higher-order function that takes an Expected object and applies the function f
     *         to its value using and_then, propagating errors without invoking f
     *
     * @note This enables more readable composition with the pipe operator, turning
     *       ex.and_then(f) into ex | and_then(f)
     * @note The returned function uses perfect forwarding to preserve the Expected's value category
     * @note Supports both lvalue and rvalue Expected objects (moves rvalues for efficiency)
     *
     * @see Expected::and_then
     * @see operator|
     */
    template<typename TFunction>
    [[nodiscard]]
    constexpr auto and_then(TFunction&& f)
    {
        return [f = std::forward<TFunction>(f)]<typename Self>(Self&& ex) {
            return std::forward<Self>(ex).and_then(f);
        };
    }

    /**
     * @brief Creates a higher-order function for error handling on Expected objects.
     *
     * This function returns a closure that applies the provided function using the or_else
     * method of an Expected object. It enables point-free programming style and function
     * composition with the Expected monad, providing a clean way to handle errors in a
     * pipeline of operations.
     *
     * @tparam TFunction The type of the function to be applied to the Expected's error
     *
     * @param f The function to apply to the error inside an Expected if it's in an error state.
     *          This function should take an error of type TError and return an Expected.
     *
     * @return A higher-order function that takes an Expected object and applies the function f
     *         to its error using or_else, preserving values without invoking f
     *
     * @note This enables more readable composition with the pipe operator, turning
     *       ex.or_else(f) into ex | or_else(f)
     * @note The returned function uses perfect forwarding to preserve the Expected's value category
     * @note Supports both lvalue and rvalue Expected objects (moves rvalues for efficiency)
     *
     * @see Expected::or_else
     * @see operator|
     * @see and_then for the complementary operation that handles the success case
     */
    template<typename TFunction>
    [[nodiscard]]
    constexpr auto or_else(TFunction&& f)
    {
        return [f = std::forward<TFunction>(f)]<typename Self>(Self&& ex) {
            return std::forward<Self>(ex).or_else(f);
        };
    }

    /**
     * @brief Creates a higher-order function for transforming values within Expected objects.
     *
     * This function returns a closure that applies the provided function using the transform
     * method of an Expected object. It enables point-free programming style and function
     * composition with the Expected monad, allowing value transformations to be chained
     * together with the pipe operator.
     *
     * @tparam TFunction The type of the function to be applied to the Expected's value
     *
     * @param f The function to apply to the value inside an Expected if it's in a success state.
     *          This function should take a value of type TValue and return a new value of any type.
     *
     * @return A higher-order function that takes an Expected object and applies the function f
     *         to its value using transform, propagating errors without invoking f
     *
     * @note This enables more readable composition with the pipe operator, turning
     *       ex.transform(f) into ex | transform(f)
     * @note The returned function uses perfect forwarding to preserve the Expected's value category
     * @note Supports both lvalue and rvalue Expected objects (moves rvalues for efficiency)
     * @note Unlike and_then(), this function automatically wraps the result in a new Expected
     *
     * @see Expected::transform
     * @see operator|
     * @see transform_error for the complementary operation that transforms errors
     * @see and_then for operations that return Expected objects directly
     */
    template<typename TFunction>
    [[nodiscard]]
    constexpr auto transform(TFunction&& f)
    {
        return [f = std::forward<TFunction>(f)]<typename Self>(Self&& ex) {
            return std::forward<Self>(ex).transform(f);
        };
    }

    /**
     * @brief Creates a higher-order function for transforming errors within Expected objects.
     *
     * This function returns a closure that applies the provided function using the transform_error
     * method of an Expected object. It enables point-free programming style and function
     * composition with the Expected monad, allowing error transformations to be chained
     * together with the pipe operator.
     *
     * @tparam TFunction The type of the function to be applied to the Expected's error
     *
     * @param f The function to apply to the error inside an Expected if it's in an error state.
     *          This function should take an error of type TError and return a new error of any type.
     *
     * @return A higher-order function that takes an Expected object and applies the function f
     *         to its error using transform_error, preserving values without invoking f
     *
     * @note This enables more readable composition with the pipe operator, turning
     *       ex.transform_error(f) into ex | transform_error(f)
     * @note The returned function uses perfect forwarding to preserve the Expected's value category
     * @note Supports both lvalue and rvalue Expected objects (moves rvalues for efficiency)
     * @note Unlike or_else(), this function automatically wraps the result in a new Expected
     *
     * @see Expected::transform_error
     * @see operator|
     * @see transform for the complementary operation that transforms values
     * @see or_else for operations that return Expected objects directly
     */
    template<typename TFunction>
    [[nodiscard]]
    constexpr auto transform_error(TFunction&& f)
    {
        return [f = std::forward<TFunction>(f)]<typename Self>(Self&& ex) {
            return std::forward<Self>(ex).transform_error(f);
        };
    }

    /**
     * @brief Creates a higher-order function for converting Expected objects to fxt::expected.
     *
     * This function returns a closure that converts an xll::Expected object to a fxt::expected type.
     * It enables point-free programming style and function composition with the Expected monad,
     * allowing conversions to be chained together with the pipe operator.
     *
     * @tparam T The target value type for the fxt::expected (defaults to void = use source type)
     * @tparam E The target error type for the fxt::expected (defaults to void = use source type)
     *
     * @return A higher-order function that takes an Expected object and converts it to
     *         fxt::expected<T, E> using perfect forwarding
     *
     * @note Supports perfect forwarding - rvalue Expected objects are moved, not copied
     * @note Uses void as sentinel value meaning "use source type"
     * @note The returned lambda uses Self&& to preserve const and value category
     *
     * @see Expected::to_expected
     * @see operator|
     * @see fxt::expected
     *
     * @example
     * @code
     * xll::Expected<xll::Number> ex = 42.0;
     *
     * // Convert to fxt::expected with same types
     * auto result1 = ex | to_expected();
     * // result1 is fxt::expected<xll::Number, xll::Error>
     *
     * // Convert only error type
     * auto result2 = ex | to_expected<void, std::string>();
     * // result2 is fxt::expected<xll::Number, std::string>
     *
     * // Convert both types
     * auto result3 = ex | to_expected<double, xll::Error>();
     * // result3 is fxt::expected<double, xll::Error>
     *
     * // Move from rvalue
     * auto result4 = create_expected() | to_expected();
     * // ✅ Expected is moved, not copied
     * @endcode
     */
    template<typename T = void, typename E = void>
    [[nodiscard]]
    constexpr auto to_expected()
    {
        return []<typename Self>(Self&& ex) {
            // Extract the source types from the Expected object
            using TValue = typename std::remove_cvref_t<Self>::value_type;
            using TError = typename std::remove_cvref_t<Self>::error_type;

            if constexpr (std::same_as<T, void> && std::same_as<E, void>) {
                // Both void: use source types
                return std::forward<Self>(ex).template to_expected<TValue, TError>();
            }
            else if constexpr (std::same_as<T, void>) {
                // T is void: use source value type, specified error type
                return std::forward<Self>(ex).template to_expected<TValue, E>();
            }
            else if constexpr (std::same_as<E, void>) {
                // E is void: use specified value type, source error type
                return std::forward<Self>(ex).template to_expected<T, TError>();
            }
            else {
                // Both specified: use both specified types
                return std::forward<Self>(ex).template to_expected<T, E>();
            }
        };
    }
}    // namespace xll
