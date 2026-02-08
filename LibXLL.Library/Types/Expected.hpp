//
// Created by kenne on 31/03/2025.
//

#pragma once

#include <cassert>
#include <fxt.hpp>
#include "../ExcelSDK/xlcall.hpp"
#include "Error.hpp"
#include "Nil.hpp"
#include "Missing.hpp"
#include "Number.hpp"
#include "Int.hpp"
#include "Bool.hpp"

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
     * ## Design & Invariants
     *
     * This class inherits from XLOPER12 and maintains binary compatibility with it.
     * Both TValue and TError are also XLOPER12 wrappers, meaning all three types
     * have identical memory layout but different interfaces to enforce type safety.
     *
     * **Critical Invariants:**
     * 1. The `xltype` field MUST always be either `TValue::excel_type` OR `TError::excel_type`
     *    (memory management flags like xlbitXLFree and xlbitDLLFree may be OR'd with the base type)
     * 2. No additional data members are added to maintain binary compatibility with XLOPER12
     * 3. The class is marked `final` to prevent derived classes from adding data members
     * 4. If `has_value()` returns true, the object contains a valid TValue
     * 5. If `has_value()` returns false, the object contains a valid TError
     *
     * **XLOPER12 Wrapper Design:**
     * Since Expected, TValue, and TError all wrap XLOPER12 with identical layout,
     * operations like `value()` return `reinterpret_cast<TValue&>(*this)`, which
     * returns a reference to the *same memory* with a different type interface.
     * This means self-assignment through union members is possible and must be handled.
     *
     * @note The type parameters are constrained to exclude certain Excel types from being
     *       used as values, and the error type is currently limited to xll::Error.
     *
     * @see Unexpected
     * @see XLOPER12
     */
    template<typename TValue, typename TError = xll::Error>
        requires(not std::same_as<TValue, xll::Error> and not std::same_as<TValue, xll::Nil> and not std::same_as<TValue, xll::Missing>) and
                (std::same_as<TError, xll::Error>)    // or std::same_as<TError, xll::String>)
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
        constexpr Expected() : XLOPER12()
        {
            //xltype = TValue::excel_type;
            std::construct_at(reinterpret_cast<TValue*>(this));
        }

        /**
         * @brief Copy constructor.
         *
         * Constructs a new Expected object by copying the state and value from another Expected object.
         * If the source object contains a value, the new object will contain a copy of that value.
         * If the source object contains an error, the new object will contain a copy of that error.
         *
         * @param other The Expected object to copy from
         *
         * @note The xltype is set by the TValue or TError constructor, not explicitly here
         */
        constexpr Expected(const Expected& other) : XLOPER12()
        {
            if (other.has_value()) {
                std::construct_at(reinterpret_cast<TValue*>(this), other.value());
            }
            else {
                std::construct_at(reinterpret_cast<TError*>(this), other.error());
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
            }
            else {
                std::construct_at(reinterpret_cast<TError*>(this), std::move(other.error()));
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
        constexpr Expected(const TValue& t) : XLOPER12()    // NOLINT
        {
            //xltype = TValue::excel_type;
            std::construct_at(reinterpret_cast<TValue*>(this), t);
        }

        template<typename U, typename UBase = std::remove_cvref_t<U>>
            requires std::constructible_from<TValue, U> &&
                     (!std::same_as<TValue, UBase>) &&
                     (!std::same_as<Expected, UBase>) &&
                     (!requires { typename UBase::value_type; typename UBase::error_type; })
        constexpr Expected(const U& u) : XLOPER12()
        {
            //xltype = TValue::excel_type;
            std::construct_at(reinterpret_cast<TValue*>(this), u);
        }

        // ...existing code...

        constexpr Expected(TValue&& t) noexcept : XLOPER12()
        {
            //xltype = TValue::excel_type;
            std::construct_at(reinterpret_cast<TValue*>(this), std::move(t));
        }

        template<typename U, typename UBase = std::remove_cvref_t<U>>
            requires std::constructible_from<TValue, U> &&
                     (!std::same_as<TValue, UBase>) &&
                     (!std::same_as<Expected, UBase>) &&
                     (!requires { typename UBase::value_type; typename UBase::error_type; })
        constexpr Expected(U&& u) : XLOPER12()
        {
            //xltype = TValue::excel_type;
            std::construct_at(reinterpret_cast<TValue*>(this), std::forward<U>(u));
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
        constexpr Expected(const Unexpected<UError>& unexpected) : XLOPER12()
        {
            //xltype = TError::excel_type;
            std::construct_at(reinterpret_cast<TError*>(this), unexpected.error());
        }

        // ...existing code...

        template<typename UError = TError>
        constexpr Expected(Unexpected<UError>&& unexpected) noexcept : XLOPER12()
        {
            //xltype = TError::excel_type;
            std::construct_at(reinterpret_cast<TError*>(this), std::move(unexpected.error()));
        }

        /**
         * @brief Destructor for the Expected class.
         *
         * Properly destroys the contained object based on whether the Expected is in a
         * value or error state. Uses std::destroy_at to explicitly destroy the active
         * object without invoking undefined behavior.
         *
         * CRITICAL FIX: Removed the problematic XLOPER12() assignment that would
         * corrupt the union state after manual destruction.
         */
        constexpr ~Expected()
        {
            if (has_value())
                std::destroy_at(reinterpret_cast<TValue*>(this));
            else
                std::destroy_at(reinterpret_cast<TError*>(this));
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
            if (this == &other) return *this;

            // Manually destroy only the active union member
            if (has_value())
                std::destroy_at(reinterpret_cast<TValue*>(this));
            else
                std::destroy_at(reinterpret_cast<TError*>(this));

            if (other.has_value()) {
                std::construct_at(reinterpret_cast<TValue*>(this),
                                reinterpret_cast<const TValue&>(other));
            }
            else {
                std::construct_at(reinterpret_cast<TError*>(this),
                                reinterpret_cast<const TError&>(other));
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
        constexpr Expected& operator=(Expected&& other) noexcept
        {
            if (this == std::addressof(other)) return *this;

            // Manually destroy only the active union member
            if (has_value())
                std::destroy_at(reinterpret_cast<TValue*>(this));
            else
                std::destroy_at(reinterpret_cast<TError*>(this));

            if (other.has_value()) {
                std::construct_at(reinterpret_cast<TValue*>(this),
                                std::move(reinterpret_cast<TValue&>(other)));
            }
            else {
                std::construct_at(reinterpret_cast<TError*>(this),
                                std::move(reinterpret_cast<TError&>(other)));
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
        constexpr Expected& operator=(const TValue& other)
        {
            if (std::addressof(other) == reinterpret_cast<const TValue*>(this))
                return *this;

            // Manually destroy only the active union member
            if (has_value())
                std::destroy_at(reinterpret_cast<TValue*>(this));
            else
                std::destroy_at(reinterpret_cast<TError*>(this));

            //xltype = TValue::excel_type;
            std::construct_at(reinterpret_cast<TValue*>(this), other);
            return *this;
        }

        template<typename U, typename UBase = std::remove_cvref_t<U>>
            requires std::constructible_from<TValue, U> &&
                     (!std::same_as<TValue, UBase>) &&
                     (!std::same_as<Expected, UBase>) &&
                     (!requires { typename UBase::value_type; typename UBase::error_type; })
        constexpr Expected& operator=(const U& other)
        {

            // Manually destroy only the active union member
            if (has_value())
                std::destroy_at(reinterpret_cast<TValue*>(this));
            else
                std::destroy_at(reinterpret_cast<TError*>(this));

            //xltype = TValue::excel_type;
            std::construct_at(reinterpret_cast<TValue*>(this), other);
            return *this;
        }

        // ...existing code...

        constexpr Expected& operator=(TValue&& other)
            noexcept(std::is_nothrow_move_constructible_v<TValue> && std::is_nothrow_destructible_v<Expected>)
        {
            if (std::addressof(other) == reinterpret_cast<const TValue*>(this))
                return *this;

            // Manually destroy only the active union member
            if (has_value())
                std::destroy_at(reinterpret_cast<TValue*>(this));
            else
                std::destroy_at(reinterpret_cast<TError*>(this));

            //xltype = TValue::excel_type;
            std::construct_at(reinterpret_cast<TValue*>(this), std::move(other));
            return *this;
        }

        template<typename U, typename UBase = std::remove_cvref_t<U>>
            requires std::constructible_from<TValue, U> &&
                     (!std::same_as<TValue, UBase>) &&
                     (!std::same_as<Expected, UBase>) &&
                     (!requires { typename UBase::value_type; typename UBase::error_type; })
        constexpr Expected& operator=(U&& other)
            noexcept(std::is_nothrow_constructible_v<TValue, U> && std::is_nothrow_destructible_v<Expected>)
        {

            // Manually destroy only the active union member
            if (has_value())
                std::destroy_at(reinterpret_cast<TValue*>(this));
            else
                std::destroy_at(reinterpret_cast<TError*>(this));

            //xltype = TValue::excel_type;
            std::construct_at(reinterpret_cast<TValue*>(this), std::forward<U>(other));
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
        constexpr Expected& operator=(const Unexpected<UError>& unexpected)
        {
            // Manually destroy only the active union member
            if (has_value())
                std::destroy_at(reinterpret_cast<TValue*>(this));
            else
                std::destroy_at(reinterpret_cast<TError*>(this));

            //xltype = TError::excel_type;
            std::construct_at(reinterpret_cast<TError*>(this), unexpected.error());
            return *this;
        }

        // ...existing code...

        template<typename UError = TError>
        constexpr Expected& operator=(Unexpected<UError>&& unexpected)
            noexcept(std::is_nothrow_move_constructible_v<TError> && std::is_nothrow_destructible_v<Expected>)
        {
            // Manually destroy only the active union member
            if (has_value())
                std::destroy_at(reinterpret_cast<TValue*>(this));
            else
                std::destroy_at(reinterpret_cast<TError*>(this));

            //xltype = TError::excel_type;
            std::construct_at(reinterpret_cast<TError*>(this), std::move(unexpected.error()));
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
        [[nodiscard]]
        constexpr bool has_value() const noexcept
        {
            return (xltype & TValue::excel_type) != 0;
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
        constexpr explicit operator bool() const { return has_value(); }

        /**
         * @brief Retrieves the contained value if in a success state (const lvalue overload).
         *
         * Provides access to the underlying value stored in the Expected object when it's in
         * a success state. Throws an exception if called when the object is in an error state.
         * This overload returns a const reference to avoid unnecessary copies.
         *
         * @return Const reference to the contained value
         * @throws std::runtime_error if the Expected object is in an error state
         */
        [[nodiscard]]
        constexpr const TValue& value() const& noexcept(false)
        {
            if (has_value()) return reinterpret_cast<const TValue&>(*this);
            throw std::runtime_error("Expected::value() called on unexpected value");
        }

        /**
         * @brief Retrieves the contained value if in a success state (rvalue overload).
         *
         * Provides move access to the underlying value stored in the Expected object.
         * Enables efficient transfer of the value when the Expected is an rvalue.
         *
         * @return Rvalue reference to the contained value (enables moves)
         * @throws std::runtime_error if the Expected object is in an error state
         */
        [[nodiscard]]
        constexpr TValue&& value() && noexcept(false)
        {
            if (has_value()) return std::move(reinterpret_cast<TValue&>(*this));
            throw std::runtime_error("Expected::value() called on unexpected value");
        }

        /**
         * @brief Retrieves the contained value if in a success state (const rvalue overload).
         *
         * Provides const rvalue access to the underlying value.
         *
         * @return Const rvalue reference to the contained value
         * @throws std::runtime_error if the Expected object is in an error state
         */
        [[nodiscard]]
        constexpr const TValue&& value() const&& noexcept(false)
        {
            if (has_value()) return std::move(reinterpret_cast<const TValue&>(*this));
            throw std::runtime_error("Expected::value() called on unexpected value");
        }

        /**
         * @brief Retrieves the contained error if in a failure state (const lvalue overload).
         *
         * Provides access to the underlying error stored in the Expected object when it's in
         * a failure state. Returns a const reference to avoid unnecessary copies.
         * Throws an exception if:
         * 1. Called when the object is in a success state (contains a value)
         * 2. The error type doesn't match TError (type mismatch)
         *
         * @return Const reference to the contained error
         * @throws std::runtime_error if the Expected object is in a success state or type mismatch
         */
        [[nodiscard]]
        constexpr const TError& error() const& noexcept(false)
        {
            if (has_value())
                throw std::runtime_error("Expected::error() called on expected value");

            if (xltype != TError::excel_type)
                throw std::runtime_error("Type mismatch: Expected contains error of different type");

            return reinterpret_cast<const TError&>(*this);
        }

        /**
         * @brief Retrieves the contained error if in a failure state (rvalue overload).
         *
         * Provides move access to the underlying error. Enables efficient transfer of the error
         * when the Expected is an rvalue.
         *
         * @return Rvalue reference to the contained error (enables moves)
         * @throws std::runtime_error if the Expected object is in a success state or type mismatch
         */
        [[nodiscard]]
        constexpr TError&& error() && noexcept(false)
        {
            if (has_value())
                throw std::runtime_error("Expected::error() called on expected value");

            if (xltype != TError::excel_type)
                throw std::runtime_error("Type mismatch: Expected contains error of different type");

            return std::move(reinterpret_cast<TError&>(*this));
        }

        /**
         * @brief Retrieves the contained error if in a failure state (const rvalue overload).
         *
         * Provides const rvalue access to the underlying error.
         *
         * @return Const rvalue reference to the contained error
         * @throws std::runtime_error if the Expected object is in a success state or type mismatch
         */
        [[nodiscard]]
        constexpr const TError&& error() const&& noexcept(false)
        {
            if (has_value())
                throw std::runtime_error("Expected::error() called on expected value");

            if (xltype != TError::excel_type)
                throw std::runtime_error("Type mismatch: Expected contains error of different type");

            return std::move(reinterpret_cast<const TError&>(*this));
        }

        /**
         * @brief Dereference operator that retrieves the contained value (const lvalue overload).
         *
         * Provides a convenient shorthand for accessing the underlying value stored in the
         * Expected object when it's in a success state. Returns a const reference.
         *
         * @return Const reference to the contained value
         * @throws std::runtime_error if the Expected object is in an error state
         */
        [[nodiscard]]
        constexpr const TValue& operator*() const& { return value(); }

        /**
         * @brief Dereference operator that retrieves the contained value (rvalue overload).
         *
         * Enables move semantics when dereferencing an rvalue Expected.
         *
         * @return Rvalue reference to the contained value
         * @throws std::runtime_error if the Expected object is in an error state
         */
        [[nodiscard]]
        constexpr TValue&& operator*() && { return std::move(*this).value(); }

        /**
         * @brief Dereference operator that retrieves the contained value (const rvalue overload).
         *
         * @return Const rvalue reference to the contained value
         * @throws std::runtime_error if the Expected object is in an error state
         */
        [[nodiscard]]
        constexpr const TValue&& operator*() const&& { return std::move(*this).value(); }

        /**
         * @brief Arrow operator for pointer-like access to the contained value.
         *
         * Provides convenient member access to the contained value, following the
         * smart pointer interface pattern. Allows using -> on Expected just like
         * on std::optional or smart pointers.
         *
         * @return Pointer to the contained value
         *
         * @pre has_value() must be true
         * @note Does not perform has_value() check in release builds - undefined behavior if called on error state
         * @note Use only when you're certain the Expected contains a value
         */
        [[nodiscard]]
        constexpr const TValue* operator->() const noexcept
        {
            assert(has_value() && "operator-> called on Expected in error state");
            return std::addressof(reinterpret_cast<const TValue&>(*this));
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
            assert(has_value() && "operator-> called on Expected in error state");
            return std::addressof(reinterpret_cast<TValue&>(*this));
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
            if (has_value()) return fxt::expected<T, E>(value());

            // Type safety check before accessing error
            if (xltype != TError::excel_type)
                throw std::runtime_error("Type mismatch in to_expected: Expected contains error of different type");

            return fxt::unexpected<E>(error());
        }

        constexpr operator fxt::expected<TValue, xll::Error>() const
        {
            if (has_value()) return fxt::expected<TValue, xll::Error>(value());

            // Type safety check before accessing error
            if (xltype != TError::excel_type)
                throw std::runtime_error("Type mismatch in conversion: Expected contains error of different type");

            return fxt::unexpected<xll::Error>(error());
        }

        constexpr operator fxt::expected<TValue, xll::String>() const
        {
            if (has_value()) return fxt::expected<TValue, xll::String>(value());

            // Type safety check before accessing error
            if (xltype != TError::excel_type)
                throw std::runtime_error("Type mismatch in conversion: Expected contains error of different type");

            return fxt::unexpected<xll::String>(error().to_string());
        }

        template<typename UError>
            requires std::convertible_to<TError, UError> &&
                     (!std::same_as<UError, TError>)
        constexpr operator fxt::expected<TValue, UError>() const
        {
            if (has_value()) return fxt::expected<TValue, UError>(value());

            // Type safety check before accessing error
            if (xltype != TError::excel_type)
                throw std::runtime_error("Type mismatch in conversion: Expected contains error of different type");

            return fxt::unexpected<UError>(error());
        }

        /**
         * @brief Returns the contained value or a default value if in error state (const lvalue overload).
         *
         * This method provides a safe way to access the contained value without throwing
         * an exception when the Expected object is in an error state. If the object is in
         * a success state, it returns a copy of the contained value; otherwise, it returns
         * a value constructed from the provided default.
         *
         * @tparam U Type of the default value (deduced)
         * @param default_value The value to return if the Expected object is in an error state
         * @return The contained value if in success state, otherwise the default value
         *
         * @note Uses perfect forwarding to support any type convertible to TValue
         * @note Returns by value (copies from the Expected)
         */
        template<typename U = TValue>
            requires std::constructible_from<TValue, U>
        [[nodiscard]]
        constexpr TValue value_or(U&& default_value) const&
        {
            if (has_value()) return value();
            return TValue(std::forward<U>(default_value));
        }

        /**
         * @brief Returns the contained value or a default value if in error state (rvalue overload).
         *
         * This overload enables move semantics when the Expected is an rvalue, allowing
         * efficient transfer of the contained value without unnecessary copies.
         *
         * @tparam U Type of the default value (deduced)
         * @param default_value The value to return if the Expected object is in an error state
         * @return The contained value (moved) if in success state, otherwise the default value
         *
         * @note Uses perfect forwarding and move semantics for maximum efficiency
         */
        template<typename U = TValue>
            requires std::constructible_from<TValue, U>
        [[nodiscard]]
        constexpr TValue value_or(U&& default_value) &&
        {
            if (has_value()) return std::move(*this).value();
            return TValue(std::forward<U>(default_value));
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
        constexpr TError error_or(const TError& default_value) const
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
            requires std::invocable<TFunc, TValue&> &&
                     requires(TFunc f, TValue& v) {
                         {
                             std::invoke(f, v)
                         } -> std::convertible_to<Expected<typename TResult::value_type, TError>>;
                     }
        [[nodiscard]]
        constexpr auto and_then(this TSelf&& self, TFunc&& func) -> TResult
        {
            if (self.has_value()) return std::invoke(std::forward<TFunc>(func), self.value());

            // Type safety check before accessing error
            if (self.xltype != TError::excel_type)
                throw std::runtime_error("Type mismatch in and_then: Expected contains error of different type");

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
            requires std::invocable<TFunc, TValue>
        [[nodiscard]]
        constexpr auto transform(this TSelf&& self, TFunc&& func)
        {
            using result_type          = std::invoke_result_t<TFunc, TValue&>;
            using expected_result_type = xll::Expected<result_type>;

            if (self.has_value()) return expected_result_type(std::invoke(std::forward<TFunc>(func), self.value()));

            // Type safety check before accessing error
            if (self.xltype != TError::excel_type)
                throw std::runtime_error("Type mismatch in transform: Expected contains error of different type");

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
        [[nodiscard]]
        constexpr auto or_else(this TSelf&& self, TFunc&& func)
        {
            using result_type = std::invoke_result_t<TFunc, const xll::Error&>;

            if (!self.has_value()) {
                // Type safety check before accessing error
                if (self.xltype != TError::excel_type)
                    throw std::runtime_error("Type mismatch in or_else: Expected contains error of different type");

                return std::invoke(std::forward<TFunc>(func), self.error());
            }
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
        [[nodiscard]]
        constexpr auto transform_error(this TSelf&& self, TFunc&& func)
        {
            using result_type          = std::invoke_result_t<TFunc, const TError&>;
            using expected_result_type = xll::Expected<TValue, result_type>;

            if (!self.has_value()) {
                // Type safety check before accessing error
                if (self.xltype != TError::excel_type)
                    throw std::runtime_error("Type mismatch in transform_error: Expected contains error of different type");

                return expected_result_type(Unexpected(std::invoke(std::forward<TFunc>(func), self.error())));
            }
            return expected_result_type(self.value());
        }

        /**
         * @brief Constructs the value in-place, destroying any existing value or error.
         *
         * Destroys the currently contained value or error, then constructs a new value
         * in-place using the provided arguments. This is more efficient than assignment
         * when constructing complex objects.
         *
         * @tparam Args Types of arguments to forward to TValue's constructor
         * @param args Arguments to forward to the TValue constructor
         */
        template<typename... Args>
            requires std::constructible_from<TValue, Args...>
        constexpr void emplace(Args&&... args)
        {
            // Manually destroy only the active union member
            if (has_value())
                std::destroy_at(reinterpret_cast<TValue*>(this));
            else
                std::destroy_at(reinterpret_cast<TError*>(this));

            xltype = TValue::excel_type;
            std::construct_at(reinterpret_cast<TValue*>(this), std::forward<Args>(args)...);
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
            requires std::constructible_from<TError, Args...>
        constexpr void emplace_error(Args&&... args)
        {
            // Manually destroy only the active union member
            if (has_value())
                std::destroy_at(reinterpret_cast<TValue*>(this));
            else
                std::destroy_at(reinterpret_cast<TError*>(this));

            xltype = TError::excel_type;
            std::construct_at(reinterpret_cast<TError*>(this), std::forward<Args>(args)...);
        }

        /**
         * @brief Swaps the contents of two Expected objects.
         *
         * Efficiently exchanges the contents of this Expected with another.
         * Handles all four cases: value-value, error-error, value-error, error-value.
         *
         * @param other The Expected object to swap with
         *
         * @note noexcept specification depends on move constructibility of TValue and TError
         */
        constexpr void swap(Expected& other)
            noexcept(std::is_nothrow_move_constructible_v<TValue> &&
                     std::is_nothrow_move_constructible_v<TError>)
        {
            if (this == &other) return;

            if (has_value() && other.has_value()) {
                // Both contain values - swap them
                using std::swap;
                swap(reinterpret_cast<TValue&>(*this), reinterpret_cast<TValue&>(other));
            }
            else if (!has_value() && !other.has_value()) {
                // Both contain errors - swap them
                using std::swap;
                swap(reinterpret_cast<TError&>(*this), reinterpret_cast<TError&>(other));
            }
            else {
                // Mixed case - use move construction
                Expected temp(std::move(*this));
                *this = std::move(other);
                other = std::move(temp);
            }
        }

        /**
         * @brief Equality comparison operator.
         *
         * Compares two Expected objects for equality. Two Expected objects are equal if:
         * - Both contain values and the values are equal, OR
         * - Both contain errors and the errors are equal
         *
         * @param other The Expected object to compare with
         * @return true if both Expected objects are in the same state with equal contents
         */
        [[nodiscard]]
        friend constexpr bool operator==(const Expected& lhs, const Expected& rhs)
            requires std::equality_comparable<TValue> && std::equality_comparable<TError>
        {
            if (lhs.has_value() != rhs.has_value()) return false;
            if (lhs.has_value()) return lhs.value() == rhs.value();

            // Both have errors - check type compatibility first
            if (lhs.xltype != TError::excel_type || rhs.xltype != TError::excel_type)
                return false;

            return lhs.error() == rhs.error();
        }

        /**
         * @brief Equality comparison with a value.
         *
         * An Expected is equal to a value if it contains that value (is in success state).
         *
         * @param value The value to compare with
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
         * @param unexpected The Unexpected to compare with
         * @return true if the Expected contains an error equal to the given Unexpected's error
         */
        [[nodiscard]]
        friend constexpr bool operator==(const Expected& lhs, const Unexpected<TError>& rhs)
            requires std::equality_comparable<TError>
        {
            if (lhs.has_value()) return false;
            if (lhs.xltype != TError::excel_type) return false;
            return lhs.error() == rhs.error();
        }
    };

    namespace impl
    {
        // Helper trait to detect if a type is a specialization of xll::Expected
        template<typename T>
        struct is_expected_impl : std::false_type
        {
        };

        template<typename TValue, typename TError>
        struct is_expected_impl<Expected<TValue, TError>> : std::true_type
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
        constexpr ~Unexpected() = default;

        /**
         * @brief Accessor for the error value, const lvalue reference overload.
         *
         * Provides read-only access to the contained error value when the Unexpected
         * object is accessed as a const lvalue. This overload is marked noexcept
         * to indicate it won't throw exceptions.
         *
         * @return A const lvalue reference to the contained error
         */
        [[nodiscard]]
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
        [[nodiscard]]
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
        [[nodiscard]]
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
        [[nodiscard]]
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
     * @note The returned function uses perfect forwarding to preserve the function's value category
     * @note The impl::is_valid_type concept ensures type safety with Excel-compatible types
     *
     * @see Expected::and_then
     * @see operator|
     */
    template<typename TFunction>
    constexpr auto and_then(TFunction&& f)
    {
        return [f = std::forward<TFunction>(f)]<typename TValue, typename TError>(const xll::Expected<TValue, TError>& ex) {
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
    constexpr auto or_else(TFunction&& f)
    {
        return [f = std::forward<TFunction>(f)]<typename TValue>(const xll::Expected<TValue>& ex) { return ex.or_else(f); };
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
    constexpr auto transform(TFunction&& f)
    {
        return [f = std::forward<TFunction>(f)]<typename TValue>(const xll::Expected<TValue>& ex) { return ex.transform(f); };
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
    constexpr auto transform_error(TFunction&& f)
    {
        return
            [f = std::forward<TFunction>(f)]<typename TValue>(const xll::Expected<TValue>& ex) { return ex.transform_error(f); };
    }

    /**
     * @brief Creates a higher-order function for converting Expected objects to fxt::expected.
     *
     * This function returns a closure that applies the to_expected method of an Expected object,
     * converting it to a fxt::expected type. It enables point-free programming style and function
     * composition with the Expected monad, allowing conversions to be chained together with
     * the pipe operator.
     *
     * @tparam T The target value type for the fxt::expected (defaults to the source value type)
     * @tparam E The target error type for the fxt::expected (defaults to the source error type)
     *
     * @return A higher-order function that takes an Expected object and converts it to
     *         fxt::expected<T, E> using the to_expected method
     *
     * @note This enables more readable composition with the pipe operator, turning
     *       ex.to_expected<T, E>() into ex | to_expected<T, E>()
     * @note The target types T and E must be convertible from the source Expected's value and error types
     * @note If no template arguments are provided, it converts to fxt::expected with the same types
     *
     * @see Expected::to_expected
     * @see operator|
     * @see fxt::expected
     *
     * @example
     * @code
     * xll::Expected<xll::Number> ex = 42.0;
     * auto result = ex | to_expected<double, xll::Error>();
     * // result is fxt::expected<double, xll::Error>
     * @endcode
     */
    template<typename T = void, typename E = void>
    constexpr auto to_expected()
    {
        return []<typename TValue, typename TError>(const xll::Expected<TValue, TError>& ex) {
            if constexpr (std::same_as<T, void> && std::same_as<E, void>) {
                return ex.template to_expected<TValue, TError>();
            }
            else if constexpr (std::same_as<T, void>) {
                return ex.template to_expected<TValue, E>();
            }
            else if constexpr (std::same_as<E, void>) {
                return ex.template to_expected<T, TError>();
            }
            else {
                return ex.template to_expected<T, E>();
            }
        };
    }




}    // namespace xll
