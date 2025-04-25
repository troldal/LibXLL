//
// Created by kenne on 31/03/2025.
//

#pragma once

#include <fxt.hpp>

namespace xll
{

    // Forward declaration of the Unexpected class template
    template<typename TError>
        requires(std::same_as<TError, xll::Error>)    // or std::same_as<TError, xll::String>)
    class Unexpected;

    /**
     * @brief A class template that represents a value that could be either valid or an error.
     *
     * The Expected class is a monadic container similar to std::expected that
     * can either hold a value of type TValue (success state) or an error of type TError
     * (failure state). This class integrates with Excel's XLOPER12 data structure to
     * provide error handling for Excel add-in functions.
     *
     * @tparam TValue The type of value to store when in the success state
     * @tparam TError The type of error to store when in the failure state, defaults to xll::Error
     *
     * @note The type parameters are constrained to exclude certain Excel types from being
     *       used as values, and the error type is currently limited to xll::Error.
     *
     * @see Unexpected
     */
    template<typename TValue, typename TError = xll::Error>
        requires(not std::same_as<TValue, xll::Error> and not std::same_as<TValue, xll::Nil> and not std::same_as<TValue, xll::Missing>) and
                (std::same_as<TError, xll::Error>)    // or std::same_as<TError, xll::String>)
    class Expected final : public XLOPER12
    {
    public:
        using value_type      = TValue;
        using error_type      = TError;
        using unexpected_type = Unexpected<TError>;

        static constexpr size_t excel_type = TValue::excel_type;

        /**
         * @brief Default constructor.
         *
         * Initializes a new Expected object in a valid state containing a default-constructed value.
         * This constructor creates an Expected object with its internal state set to hold a value
         * of type TValue rather than an error state. The XLOPER12 base class is initialized through
         * its default constructor before setting the appropriate Excel type and value.
         */
        Expected() : XLOPER12()
        {
            xltype                           = TValue::excel_type;
            reinterpret_cast<TValue&>(*this) = TValue();
        }

        /**
         * @brief Copy constructor.
         *
         * Constructs a new Expected object by copying the state and value from another Expected object.
         * If the source object contains a value, the new object will contain a copy of that value.
         * If the source object contains an error, the new object will contain a copy of that error.
         *
         * @param other The Expected object to copy from
         */
        Expected(const Expected& other) : XLOPER12()
        {
            if (other.has_value()) {
                xltype                           = TValue::excel_type;
                reinterpret_cast<TValue&>(*this) = other.value();
            }
            else {
                xltype                           = TError::excel_type;
                reinterpret_cast<TError&>(*this) = other.error();
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
         */
        Expected(Expected&& other) noexcept : XLOPER12()
        {
            if (other.has_value()) {
                xltype                           = TValue::excel_type;
                reinterpret_cast<TValue&>(*this) = std::move(other.value());
            }
            else {
                xltype                           = TError::excel_type;
                reinterpret_cast<TError&>(*this) = std::move(other.error());
            }
        }

        /**
         * @brief Converting constructor from a value type.
         *
         * Constructs a new Expected object in a success state containing a copy of the provided value.
         * This constructor allows for implicit conversion from TValue to Expected<TValue, TError>,
         * making it easier to return values from functions that return Expected objects.
         *
         * @param t The value to store in the Expected object
         *
         * @note NOLINT annotation is used to suppress static analysis warnings about implicit conversions
         */
        Expected(const TValue& t) : XLOPER12()    // NOLINT
        {
            xltype                           = TValue::excel_type;
            reinterpret_cast<TValue&>(*this) = t;
        }

        /**
         * @brief Converting move constructor from a value type.
         *
         * Constructs a new Expected object in a success state containing the moved value.
         * This constructor allows for implicit conversion from an rvalue reference of TValue
         * to Expected<TValue, TError>, enabling move semantics when returning values from
         * functions that return Expected objects.
         *
         * @param t The value to move into the Expected object
         */
        Expected(TValue&& t) : XLOPER12()
        {
            xltype                           = TValue::excel_type;
            reinterpret_cast<TValue&>(*this) = std::move(t);
        }

        /**
         * @brief Converting constructor from an Unexpected object.
         *
         * Constructs a new Expected object in an error state containing a copy of the error
         * from the provided Unexpected object. This constructor allows for implicit conversion
         * from Unexpected<UError> to Expected<TValue, TError>, facilitating error propagation
         * in functions that return Expected objects.
         *
         * @tparam UError The error type of the Unexpected object, defaults to TError
         * @param unexpected The Unexpected object containing the error to be stored
         */
        template<typename UError = TError>
        Expected(const Unexpected<UError>& unexpected) : XLOPER12()
        {
            xltype                           = TError::excel_type;
            reinterpret_cast<TError&>(*this) = unexpected.error();
        }

        /**
         * @brief Converting move constructor from an Unexpected object.
         *
         * Constructs a new Expected object in an error state containing the moved error
         * from the provided Unexpected object. This constructor allows for implicit conversion
         * from an rvalue reference of Unexpected<UError> to Expected<TValue, TError>, enabling
         * move semantics when propagating errors in functions that return Expected objects.
         *
         * @tparam UError The error type of the Unexpected object, defaults to TError
         * @param unexpected The Unexpected object containing the error to be moved
         */
        template<typename UError = TError>
        Expected(Unexpected<UError>&& unexpected) : XLOPER12()
        {
            xltype                           = TError::excel_type;
            reinterpret_cast<TError&>(*this) = std::move(unexpected.error());
        }

        /**
         * @brief Converting constructor from a Nil object.
         *
         * Constructs a new Expected object in an error state when provided with a Nil object.
         * This constructor handles cases where a Nil value (representing empty or null values in Excel)
         * is converted to an Expected object by treating it as an error. It delegates to the
         * Unexpected constructor, creating an Expected object that contains a default-constructed error.
         *
         * @param _ The Nil object (parameter name is intentionally unused)
         */
        Expected(const Nil& _) : Expected(Unexpected(TError())) {}

        /**
         * @brief Converting constructor from a Missing object.
         *
         * Constructs a new Expected object in an error state when provided with a Missing object.
         * This constructor handles cases where a Missing value (representing missing arguments in Excel)
         * is converted to an Expected object by treating it as an error. It delegates to the
         * Unexpected constructor, creating an Expected object that contains a default-constructed error.
         *
         * @param _ The Missing object (parameter name is intentionally unused)
         */
        Expected(const Missing& _) : Expected(Unexpected(TError())) {}

        /**
         * @brief Destructor for the Expected class.
         *
         * Properly destroys the contained object based on whether the Expected is in a
         * value or error state. First calls the destructor of the appropriate type
         * (TValue or TError) using reinterpret_cast to access the underlying object.
         * Then resets the XLOPER12 base class to its default-constructed state to
         * ensure all memory is properly released and the object is left in a clean state.
         *
         * This destructor handles the special memory management needed when embedding
         * C++ objects within an XLOPER12 structure.
         */
        ~Expected()
        {
            if (has_value())
                reinterpret_cast<TValue&>(*this).~TValue();
            else
                reinterpret_cast<TError&>(*this).~TError();
            *static_cast<XLOPER12*>(this) = XLOPER12();
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
        Expected& operator=(const Expected& other)
        {
            if (this == &other) return *this;

            this->~Expected();
            if (other.has_value()) {
                xltype                           = TValue::excel_type;
                reinterpret_cast<TValue&>(*this) = other.value();
            }
            else {
                xltype                           = TError::excel_type;
                reinterpret_cast<TError&>(*this) = other.error();
            }

            return *this;
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
        Expected& operator=(Expected&& other) noexcept
        {
            if (this == &other) return *this;

            this->~Expected();
            if (other.has_value()) {
                xltype                           = TValue::excel_type;
                reinterpret_cast<TValue&>(*this) = std::move(other.value());
            }
            else {
                xltype                           = TError::excel_type;
                reinterpret_cast<TError&>(*this) = std::move(other.error());
            }

            return *this;
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
        Expected& operator=(const TValue& other)
        {
            if (reinterpret_cast<void*>(this) == reinterpret_cast<void*>(&other)) return *this;

            this->~Expected();
            xltype                           = TValue::excel_type;
            reinterpret_cast<TValue&>(*this) = other;
            return *this;
        }

        /**
         * @brief Move assignment operator from a value type.
         *
         * Assigns a value to this Expected object by moving it, changing the object to a success state.
         * If the current object and the value are the same object (self-assignment detection using pointer comparison),
         * no operation is performed to avoid undefined behavior.
         * Otherwise, the current object is destroyed, and then reconstructed with the moved value type.
         *
         * @param other The value to move into the Expected object
         * @return Reference to this object after assignment
         *
         * @note Uses reinterpret_cast for self-assignment detection since the compared objects have different types
         */
        Expected& operator=(TValue&& other)
        {
            if (reinterpret_cast<void*>(this) == reinterpret_cast<void*>(&other)) return *this;

            this->~Expected();
            xltype                           = TValue::excel_type;
            reinterpret_cast<TValue&>(*this) = std::move(other);
            return *this;
        }

        /**
         * @brief Assignment operator from an Unexpected object.
         *
         * Assigns an error to this Expected object, changing it to an error state containing
         * the error from the provided Unexpected object. The current object is destroyed,
         * and then reconstructed as an error object.
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
        Expected& operator=(const Unexpected<UError>& unexpected)
        {
            this->~Expected();
            xltype                           = TError::excel_type;
            reinterpret_cast<TError&>(*this) = unexpected.error();
            return *this;
        }

        /**
         * @brief Move assignment operator from an Unexpected object.
         *
         * Assigns an error to this Expected object by moving it, changing the object to an error state.
         * The current object is destroyed, and then reconstructed with the moved error type.
         * This operator enables efficient error propagation without unnecessary copying of error objects.
         *
         * @tparam UError The error type of the Unexpected object, defaults to TError
         * @param unexpected The Unexpected object containing the error to be moved
         * @return Reference to this object after assignment
         *
         * @note This templated assignment operator allows for implicit conversion from
         *       Unexpected<UError> to Expected<TValue, TError> when assigning, which
         *       facilitates efficient error propagation in code that uses Expected objects.
         */
        template<typename UError = TError>
        Expected& operator=(Unexpected<UError>&& unexpected)
        {
            this->~Expected();
            xltype                           = TError::excel_type;
            reinterpret_cast<TError&>(*this) = std::move(unexpected.error());
            return *this;
        }

        /**
         * @brief Assignment operator from a Nil object.
         *
         * Assigns an error state to this Expected object when provided with a Nil object.
         * This operator handles cases where a Nil value (representing empty or null values in Excel)
         * is assigned to an Expected object by treating it as an error. The current object is destroyed,
         * and then reconstructed with a default-constructed error object.
         *
         * @param _ The Nil object (parameter name is intentionally unused)
         * @return Reference to this object after assignment
         *
         * @note This operator maintains consistency with the Nil constructor, treating Nil values
         *       as errors rather than valid values within the Expected container.
         */
        Expected& operator=(const Nil& _)
        {
            this->~Expected();
            xltype                           = TError::excel_type;
            reinterpret_cast<TError&>(*this) = TError();
            return *this;
        }

        /**
         * @brief Assignment operator from a Missing object.
         *
         * Assigns an error state to this Expected object when provided with a Missing object.
         * This operator handles cases where a Missing value (representing missing arguments in Excel)
         * is assigned to an Expected object by treating it as an error. The current object is destroyed,
         * and then reconstructed with a default-constructed error object.
         *
         * @param _ The Missing object (parameter name is intentionally unused)
         * @return Reference to this object after assignment
         *
         * @note This operator maintains consistency with the Missing constructor, treating Missing values
         *       as errors rather than valid values within the Expected container.
         */
        Expected& operator=(const Missing& _)
        {
            this->~Expected();
            xltype                           = TError::excel_type;
            reinterpret_cast<TError&>(*this) = TError();
            return *this;
        }

        /**
         * @brief Checks if the Expected object contains a value (is in success state).
         *
         * Determines whether the Expected object is currently holding a value rather than an error.
         * This is done by comparing the current Excel type of the object (xltype) with the
         * expected Excel type for the value type (TValue::excel_type).
         *
         * @return true if the object contains a value (success state), false if it contains
         *         an error (failure state)
         *
         * @note This method is const and doesn't modify the state of the Expected object.
         */
        bool has_value() const
        {
            if (xltype & TValue::excel_type) return true;
            return false;
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
        explicit operator bool() const { return has_value(); }

        /**
         * @brief Retrieves the contained value if in a success state.
         *
         * Provides access to the underlying value stored in the Expected object when it's in
         * a success state. Throws an exception if called when the object is in an error state.
         * This method performs a runtime check to ensure safe access to the value.
         *
         * @return The contained value of type TValue
         * @throws std::runtime_error if the Expected object is in an error state
         *
         * @note Marked with [[nodiscard]] to warn if the return value is ignored
         * @note Uses reinterpret_cast to access the underlying value without type conversion
         */
        [[nodiscard]]
        TValue value() const
        {
            if (has_value()) return reinterpret_cast<const TValue&>(*this);
            throw std::runtime_error("Expected::value() called on unexpected value");
        }

        /**
         * @brief Retrieves the contained error if in a failure state.
         *
         * Provides access to the underlying error stored in the Expected object when it's in
         * a failure state. This method handles two failure cases:
         * 1. When the object contains an error of the expected type (TError)
         * 2. When the object contains an error of a different Excel type
         *
         * Throws an exception if called when the object is in a success state.
         *
         * @return The contained error of type TError
         * @throws std::runtime_error if the Expected object is in a success state (contains a value)
         *
         * @note Marked with [[nodiscard]] to warn if the return value is ignored
         * @note When the object contains an error but with a different Excel type than expected,
         *       a default-constructed error object is returned
         */
        [[nodiscard]]
        TError error() const
        {
            if (not has_value() and xltype == TError::excel_type) return reinterpret_cast<const TError&>(*this);
            if (not has_value() and xltype != TError::excel_type) return TError();
            throw std::runtime_error("Expected::error() called on expected value");
        }

        /**
         * @brief Dereference operator that retrieves the contained value.
         *
         * Provides a convenient shorthand for accessing the underlying value stored in the
         * Expected object when it's in a success state. This operator follows the common
         * pointer-like interface pattern used by other monadic types like std::optional
         * and std::expected.
         *
         * @return The contained value of type TValue
         * @throws std::runtime_error if the Expected object is in an error state
         *
         * @note Delegates to the value() method, which performs the necessary runtime checks
         *       and throws an exception if called when the object is in an error state
         * @note Marked const to allow usage with const Expected objects
         */
        TValue operator*() const { return value(); }

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
            requires std::convertible_to<TValue, T> && std::convertible_to<TError, E>
        fxt::expected<T, E> to_expected() const
        {
            if (has_value()) return fxt::expected<T, E>(value());
            return fxt::unexpected<E>(error());
        }

        operator fxt::expected<TValue, xll::Error>() const
        {
            if (has_value()) return fxt::expected<TValue, xll::Error>(value());
            return fxt::unexpected<xll::Error>(error());
        }

        operator fxt::expected<TValue, xll::String>() const
        {
            if (has_value()) return fxt::expected<TValue, xll::String>(value());
            return fxt::unexpected<xll::String>(error().to_string());
        }

        /**
         * @brief Returns the contained value or a default value if in error state.
         *
         * This method provides a safe way to access the contained value without throwing
         * an exception when the Expected object is in an error state. If the object is in
         * a success state, it returns the contained value; otherwise, it returns the provided
         * default value.
         *
         * @param default_value The value to return if the Expected object is in an error state
         * @return The contained value if in success state, otherwise the default value
         *
         * @note Marked with [[nodiscard]] to warn if the return value is ignored
         * @note Takes the default value by rvalue reference to enable move semantics,
         *       which can be more efficient for complex types
         * @note This is const-qualified to allow use with const Expected objects
         *
         * @see value()
         * @see has_value()
         */
        [[nodiscard]]
        TValue value_or(TValue&& default_value) const
        {
            if (has_value()) return value();
            return default_value;
        }

        /**
         * @brief Returns the contained error or a default error if in success state.
         *
         * This method provides a complementary function to value_or(), allowing safe access to
         * the error value without throwing an exception when the Expected object is in a success state.
         * If the object is in an error state, it returns the contained error; otherwise, it returns
         * the provided default error value.
         *
         * @param default_value The error value to return if the Expected object is in a success state
         * @return The contained error if in failure state, otherwise the default error value
         *
         * @note Marked with [[nodiscard]] to warn if the return value is ignored
         * @note Takes the default value by const reference to avoid unnecessary copying
         * @note This is const-qualified to allow use with const Expected objects
         *
         * @see error()
         * @see has_value()
         * @see value_or()
         */
        [[nodiscard]]
        TError error_or(const TError& default_value) const
        {
            if (not has_value()) return error();
            return default_value;
        }

        /**
         * @brief Chains another operation on the Expected object if it contains a value.
         *
         * This method implements monadic binding (similar to flatMap in other languages) for the Expected type.
         * It allows sequential composition of operations that may fail by only executing the provided function
         * if this Expected is in a success state. If this Expected contains an error, that error is propagated
         * without calling the function.
         *
         * @tparam TSelf The deduced type of the Expected instance (used with deducing this feature from C++23)
         * @tparam TFunc The type of the function to apply to the contained value
         * @tparam TResult The deduced return type of the function, must be an Expected type
         *
         * @param self The Expected object to operate on (deducing this parameter)
         * @param func A callable that takes the current value and returns a new Expected object
         * @return If this Expected contains a value, returns the result of applying func to that value;
         *         otherwise, returns a new Expected containing the original error
         *
         * @note Uses C++23's deducing this feature to support both lvalue and rvalue Expected objects
         * @note The function must return an Expected type with the same error type as this Expected
         * @note Perfect forwarding is used to preserve value categories and avoid unnecessary copies
         *
         * @see transform for non-monadic mapping of the value
         * @see or_else for handling the error case
         */
        template<typename TSelf, typename TFunc, typename TResult = std::invoke_result_t<TFunc, TValue&>>
            requires std::invocable<TFunc, TValue&> && requires(TFunc f, TValue& v) {
                {
                    std::invoke(f, v)
                } -> std::convertible_to<Expected<typename TResult::value_type, TError>>;
            }
        auto and_then(this TSelf&& self, TFunc&& func) -> TResult
        {
            if (self.has_value()) return std::invoke(std::forward<TFunc>(func), self.value());
            return TResult(Unexpected(std::forward<TSelf>(self).error()));
        }

        /**
         * @brief Transforms the value contained in the Expected object using a function.
         *
         * This method applies a non-monadic transformation to the value if the Expected object
         * is in a success state. If this Expected contains an error, that error is propagated
         * without calling the function. Unlike and_then(), this method wraps the result of the
         * function in a new Expected object automatically.
         *
         * @tparam TSelf The deduced type of the Expected instance (using C++23's deducing this)
         * @tparam TFunc The type of the function to apply to the contained value
         *
         * @param self The Expected object to operate on (deducing this parameter)
         * @param func A callable that takes the current value and returns a new value of any type
         * @return A new Expected object containing either the transformed value (if in success state)
         *         or the original error (if in error state)
         *
         * @note Uses C++23's deducing this feature to support both lvalue and rvalue Expected objects
         * @note Perfect forwarding is used to preserve value categories
         * @note Unlike and_then(), the function does not need to return an Expected type
         *
         * @see and_then for monadic binding operations
         * @see transform_error for transforming the error case
         */
        template<typename TSelf, typename TFunc>
            requires std::invocable<TFunc, TValue&>
        auto transform(this TSelf&& self, TFunc&& func)
        {
            using result_type          = std::invoke_result_t<TFunc, TValue&>;
            using expected_result_type = xll::Expected<result_type>;

            if (self.has_value()) return expected_result_type(std::invoke(std::forward<TFunc>(func), self.value()));
            return expected_result_type(Unexpected(self.error()));
        }

        /**
         * @brief Handles the error case by applying a function to transform the error to a new Expected.
         *
         * This method provides error handling by executing the provided function only when
         * this Expected is in an error state. If the Expected contains a value, that value
         * is preserved without calling the function.
         *
         * @tparam TSelf The deduced type of the Expected instance (using C++23's deducing this)
         * @tparam TFunc A function that takes an xll::Error and returns an Expected
         *
         * @param self The Expected object to operate on
         * @param func A callable that processes the error and returns a new Expected
         * @return The result of func if in error state; otherwise, a new Expected with the original value
         *
         * @note Complements and_then() which handles the success case
         * @note Uses C++23's deducing this feature for both lvalue and rvalue Expected objects
         */
        template<typename TSelf, typename TFunc>
            requires std::invocable<TFunc, const xll::Error&> &&
                     std::convertible_to<std::invoke_result_t<TFunc, const xll::Error&>, xll::Expected<TValue>>
        auto or_else(this TSelf&& self, TFunc&& func)
        {
            using result_type = std::invoke_result_t<TFunc, const xll::Error&>;

            if (!self.has_value()) return std::invoke(std::forward<TFunc>(func), self.error());
            return result_type(self.value());
        }

        /**
         * @brief Transforms the error contained in the Expected object using a function.
         *
         * This method applies a transformation function to the error if the Expected object
         * is in an error state. If the Expected contains a value, that value is preserved
         * without calling the function.
         *
         * @tparam TSelf The deduced type of the Expected instance (using C++23's deducing this)
         * @tparam TFunc The type of the function to apply to the contained error
         *
         * @param self The Expected object to operate on
         * @param func A callable that takes the current error and returns a new error of any type
         * @return A new Expected object containing either the original value (if in success state)
         *         or the transformed error (if in error state)
         */
        template<typename TSelf, typename TFunc>
            requires std::invocable<TFunc, const TError&>
        auto transform_error(this TSelf&& self, TFunc&& func)
        {
            using result_type          = std::invoke_result_t<TFunc, const TError&>;
            using expected_result_type = xll::Expected<TValue, result_type>;

            if (!self.has_value()) return expected_result_type(Unexpected(std::invoke(std::forward<TFunc>(func), self.error())));
            return expected_result_type(self.value());
        }
    };

    /**
     * @brief A class template that represents an unexpected error value.
     *
     * The Unexpected class is used as a wrapper for error values in the Expected monad.
     * This class is designed to work with the Expected class to represent failure cases.
     * It provides a clear and type-safe way to construct Expected objects in an error state.
     *
     * @tparam TError The type of error to store, currently restricted to xll::Error
     *
     * @note The template parameter is constrained to be exactly xll::Error for now,
     *       with a commented indication that xll::String might be supported in the future
     *
     * @see Expected
     */
    template<typename TError>
        requires(std::same_as<TError, xll::Error>)    // or std::same_as<TError, xll::String>)
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
        ~Unexpected() = default;

        /**
         * @brief Accessor for the error value, const lvalue reference overload.
         *
         * Provides read-only access to the contained error value when the Unexpected
         * object is accessed as a const lvalue. This overload is marked noexcept
         * to indicate it won't throw exceptions.
         *
         * @return A const lvalue reference to the contained error
         */
        constexpr const TError& error() const& noexcept { return error_; }

        /**
         * @brief Accessor for the error value, non-const lvalue reference overload.
         *
         * Provides read-write access to the contained error value when the Unexpected
         * object is accessed as a non-const lvalue. This overload is marked noexcept
         * to indicate it won't throw exceptions.
         *
         * @return A non-const lvalue reference to the contained error
         */
        constexpr TError& error() & noexcept { return error_; }

        /**
         * @brief Accessor for the error value, const rvalue reference overload.
         *
         * Provides read-only access to the contained error value when the Unexpected
         * object is accessed as a const rvalue. Returns a moved const reference to
         * enable efficient transfer of the error value. This overload is marked
         * noexcept to indicate it won't throw exceptions.
         *
         * @return A const rvalue reference to the contained error
         */
        constexpr const TError&& error() const&& noexcept { return std::move(error_); }

        /**
         * @brief Accessor for the error value, non-const rvalue reference overload.
         *
         * Provides access to the contained error value when the Unexpected object is
         * accessed as a non-const rvalue. Returns a moved reference to enable efficient
         * transfer of the error value. This overload is marked noexcept to indicate
         * it won't throw exceptions.
         *
         * @return An rvalue reference to the contained error
         */
        constexpr TError&& error() && noexcept { return std::move(error_); }

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
    using ExpNumber = Expected<Number>;

    /**
     * @brief Type alias for Expected<String> representing string values that may contain errors.
     *
     * This type provides a convenient shorthand for Expected<String>, representing
     * string values that might be in an error state.
     */
    using ExpString = Expected<String>;

    /**
     * @brief Type alias for Expected<Int> representing integer values that may contain errors.
     *
     * This type provides a convenient shorthand for Expected<Int>, representing
     * integer values that might be in an error state.
     */
    using ExpInt = Expected<Int>;

    /**
     * @brief Type alias for Expected<Bool> representing boolean values that may contain errors.
     *
     * This type provides a convenient shorthand for Expected<Bool>, representing
     * boolean values that might be in an error state.
     */
    using ExpBool = Expected<Bool>;

    /**
     * @brief Pipe operator for Expected, const lvalue reference overload.
     *
     * Allows piping an Expected object through a function using the | operator.
     * This enables a more readable functional programming style with the Expected monad,
     * supporting fluent chaining of operations.
     *
     * @tparam T The value type of the Expected object
     * @tparam E The error type of the Expected object
     * @tparam TFunc The type of function to apply to the Expected
     *
     * @param t The Expected object to pipe through the function
     * @param f The function to apply to the Expected object
     * @return The result of applying the function to the Expected object
     *
     * @note Used for const lvalue Expected objects
     */
    template<typename T, typename E, typename TFunc>
        requires std::invocable<TFunc, xll::Expected<T, E>>
    constexpr auto operator|(const xll::Expected<T, E>& t, TFunc&& f) -> std::invoke_result_t<TFunc, xll::Expected<T, E>>
    {
        return std::invoke(std::forward<TFunc>(f), t);
    }

    /**
     * @brief Pipe operator for Expected, rvalue reference overload.
     *
     * Allows piping a temporary Expected object through a function using the | operator.
     * This overload enables move semantics when the Expected object is a temporary,
     * avoiding unnecessary copies for better performance.
     *
     * @tparam T The value type of the Expected object
     * @tparam E The error type of the Expected object
     * @tparam TFunc The type of function to apply to the Expected
     *
     * @param t The temporary Expected object to pipe through the function
     * @param f The function to apply to the Expected object
     * @return The result of applying the function to the Expected object
     *
     * @note Used for rvalue Expected objects, preserving move semantics
     */
    template<typename T, typename E, typename TFunc>
        requires std::invocable<TFunc, xll::Expected<T, E>>
    constexpr auto operator|(xll::Expected<T, E>&& t, TFunc&& f) -> std::invoke_result_t<TFunc, xll::Expected<T, E>>
    {
        return std::invoke(std::forward<TFunc>(f), std::move(t));
    }

    /**
     * @brief Pipe operator for Expected, non-const lvalue reference overload.
     *
     * Allows piping a mutable Expected object through a function using the | operator.
     * This overload is useful when the function needs to modify the Expected object directly.
     *
     * @tparam T The value type of the Expected object
     * @tparam E The error type of the Expected object
     * @tparam TFunc The type of function to apply to the Expected
     *
     * @param t The mutable Expected object to pipe through the function
     * @param f The function to apply to the Expected object
     * @return The result of applying the function to the Expected object
     *
     * @note Used for non-const lvalue Expected objects
     */
    template<typename T, typename E, typename TFunc>
        requires std::invocable<TFunc, xll::Expected<T, E>&>
    constexpr auto operator|(xll::Expected<T, E>& t, TFunc&& f) -> std::invoke_result_t<TFunc, xll::Expected<T, E>&>
    {
        return std::invoke(std::forward<TFunc>(f), t);
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
     * @note The returned function uses perfect forwarding to preserve the function's value category
     * @note The impl::is_valid_type concept ensures type safety with Excel-compatible types
     *
     * @see Expected::and_then
     * @see operator|
     */
    template<typename TFunction>
    auto and_then(TFunction&& f)
    {
        return [f = std::forward<TFunction>(f)]<impl::is_valid_type TValue, typename TError>(const xll::Expected<TValue, TError>& ex) {
            return ex.and_then(f);
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
     * @note The returned function uses perfect forwarding to preserve the function's value category
     * @note The impl::is_valid_type concept ensures type safety with Excel-compatible types
     *
     * @see Expected::or_else
     * @see operator|
     * @see and_then for the complementary operation that handles the success case
     */
    template<typename TFunction>
    auto or_else(TFunction&& f)
    {
        return [f = std::forward<TFunction>(f)]<impl::is_valid_type TValue>(const xll::Expected<TValue>& ex) { return ex.or_else(f); };
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
     * @note The returned function uses perfect forwarding to preserve the function's value category
     * @note The impl::is_valid_type concept ensures type safety with Excel-compatible types
     * @note Unlike and_then(), this function automatically wraps the result in a new Expected
     *
     * @see Expected::transform
     * @see operator|
     * @see transform_error for the complementary operation that transforms errors
     * @see and_then for operations that return Expected objects directly
     */
    template<typename TFunction>
    auto transform(TFunction&& f)
    {
        return [f = std::forward<TFunction>(f)]<impl::is_valid_type TValue>(const xll::Expected<TValue>& ex) { return ex.transform(f); };
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
     * @note The returned function uses perfect forwarding to preserve the function's value category
     * @note The impl::is_valid_type concept ensures type safety with Excel-compatible types
     * @note Unlike or_else(), this function automatically wraps the result in a new Expected
     *
     * @see Expected::transform_error
     * @see operator|
     * @see transform for the complementary operation that transforms values
     * @see or_else for operations that return Expected objects directly
     */
    template<typename TFunction>
    auto transform_error(TFunction&& f)
    {
        return
            [f = std::forward<TFunction>(f)]<impl::is_valid_type TValue>(const xll::Expected<TValue>& ex) { return ex.transform_error(f); };
    }

}    // namespace xll