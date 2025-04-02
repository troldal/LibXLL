//
// Created by kenne on 31/03/2025.
//

#pragma once

namespace xll {

    template<typename TValue>
    class Expected : public XLOPER12
    {
        using value_type = TValue;

    public:

        Expected() : XLOPER12()
        {
            TValue::lift(*static_cast<XLOPER12*>(this)) = TValue();
        }

        Expected(const Expected& other) : XLOPER12()
        {
            if (other.has_value())
                TValue::lift(*static_cast<XLOPER12*>(this)) = TValue::lift(static_cast<const XLOPER12&>(other));
            else
                xll::Error::lift(*static_cast<XLOPER12*>(this)) = other.error();
        }

        Expected(Expected&& other) noexcept : XLOPER12()
        {
            if (other.has_value())
                TValue::lift(*static_cast<XLOPER12*>(this)) = reinterpret_cast<TValue&&>(other);
            else
                xll::Error::lift(*static_cast<XLOPER12*>(this)) = other.error();
        }


        Expected(const TValue& t) : XLOPER12()    // NOLINT
        {
            TValue::lift(*static_cast<XLOPER12*>(this)) = t;
        }

        Expected(TValue&& t) : XLOPER12()
        {
            TValue::lift(*static_cast<XLOPER12*>(this)) = std::forward<TValue>(t);
        }

        Expected(const xll::Error& err) : XLOPER12()    // NOLINT
        {
            xll::Error::lift(*static_cast<XLOPER12*>(this)) = err;
        }

        ~Expected()
        {
            if (has_value()) reinterpret_cast<TValue&>(*this).~TValue();
        }

        Expected& operator=(const Expected& other)
        {
            if (this == &other) return *this;
            if (other.has_value())
                TValue::lift(*static_cast<XLOPER12*>(this)) = TValue::lift(static_cast<const XLOPER12&>(other));
            else
                xll::Error::lift(*static_cast<XLOPER12*>(this)) = other.error();
            return *this;
        }

        Expected& operator=(Expected&& other) noexcept
        {
            if (this == &other) return *this;
            if (other.has_value())
                TValue::lift(*static_cast<XLOPER12*>(this)) = reinterpret_cast<TValue&&>(other);
            else
                xll::Error::lift(*static_cast<XLOPER12*>(this)) = other.error();
            return *this;
        }

        Expected& operator=(const TValue& other)
        {
            this->~Expected();
            TValue::lift(*static_cast<XLOPER12*>(this)) = other;
            return *this;
        }

        Expected& operator=(TValue&& other)
        {
            this->~Expected();
            TValue::lift(*static_cast<XLOPER12*>(this)) = std::forward<TValue>(other);
            return *this;
        }

        Expected& operator=(const xll::Error& err)
        {
            this->~Expected();
            xll::Error::lift(*static_cast<XLOPER12*>(this)) = err;
            return *this;
        }

        bool has_value() const
        {
            if (xltype == TValue().xltype) return true;
            return false;
        }

        explicit operator bool() const
        {
            return has_value();
        }

        TValue& value()
        {
            return TValue::lift(*static_cast<XLOPER12*>(this));
        }

        const TValue& value() const
        {
            return TValue::lift(*reinterpret_cast<const XLOPER12*>(this));
        }

        TValue& operator*()
        {
            return value();
        }

        const TValue& operator*() const
        {
            return value();
        }

        TValue* operator->()
        {
            return &value();
        }

        const TValue* operator->() const
        {
            return &value();
        }

        [[nodiscard]]
        xll::Error& error()
        {
            if (!has_value() && xltype != xltypeErr) static_cast<xll::Error&>(*this) = xll::ErrValue;
            return xll::Error::lift(*static_cast<XLOPER12*>(this));
        }

        [[nodiscard]]
        xll::Error error() const
        {
            if (!has_value() && xltype != xltypeErr) return xll::ErrValue;
            return xll::Error::lift(*static_cast<const XLOPER12*>(this));
        }

        [[nodiscard]]
        TValue value_or(TValue&& default_value) const
        {
            if (has_value()) return value();
            return default_value;
        }

        [[nodiscard]]
        xll::Error error_or(const xll::Error& default_value) const
        {
            if (!has_value()) return error();
            return default_value;
        }

        template<typename TSelf, typename TFunc>
            requires std::invocable<TFunc, TValue&> && requires(TFunc f, TValue& v) {
                {
                    std::invoke(f, v)
                } -> std::convertible_to<xll::Expected<typename std::invoke_result_t<TFunc, TValue&>::value_type>>;
            }
        auto and_then(this TSelf&& self, TFunc&& func)
        {
            using result_type = std::invoke_result_t<TFunc, TValue&>;

            if (self.has_value()) return std::invoke(std::forward<TFunc>(func), self.value());
            result_type res = self.error();
            return res;
        }

        template<typename TSelf, typename TFunc>
            requires std::invocable<TFunc, TValue&>
        auto transform(this TSelf&& self, TFunc&& func)
        {
            using result_type          = std::invoke_result_t<TFunc, TValue&>;
            using expected_result_type = xll::Expected<result_type>;

            if (self.has_value()) return expected_result_type(std::invoke(std::forward<TFunc>(func), self.value()));
            return expected_result_type(self.error());
        }

        template<typename TSelf, typename TFunc>
            requires std::invocable<TFunc, const xll::Error&> && requires(TFunc f, const xll::Error& e) {
                {
                    std::invoke(f, e)
                } -> std::convertible_to<xll::Expected<TValue>>;
            }
        auto or_else(this TSelf&& self, TFunc&& func)
        {
            using result_type = std::invoke_result_t<TFunc, const xll::Error&>;

            if (!self.has_value()) return std::invoke(std::forward<TFunc>(func), self.error());
            return result_type(self.value());
        }

        template<typename TSelf, typename TFunc>
            requires std::invocable<TFunc, const xll::Error&>
        auto transform_error(this TSelf&& self, TFunc&& func)
        {
            using result_type          = std::invoke_result_t<TFunc, const xll::Error&>;
            using expected_result_type = xll::Expected<TValue>;

            if (!self.has_value()) return expected_result_type(std::invoke(std::forward<TFunc>(func), self.error()));
            return expected_result_type(self.value());
        }
    };

}