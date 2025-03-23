//
// Created by kenne on 23/03/2025.
//

#pragma once

#include "Base.hpp"

namespace xll
{

    class Error : public impl::Base<Error, xltypeErr>
    {
        using BASE = impl::Base<Error, xltypeErr>;
    public:
        using BASE::BASE;
    };

    static const Error ErrNull  = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = 0; return err; }());
    static const Error ErrDiv0  = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = 7; return err; }());
    static const Error ErrValue = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = 15; return err; }());
    static const Error ErrRef   = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = 23; return err; }());
    static const Error ErrName  = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = 29; return err; }());
    static const Error ErrNum   = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = 36; return err; }());
    static const Error ErrNA    = Error([] { auto err = XLOPER12(); err.xltype = xltypeErr; err.val.err = 42; return err; }());

}    // namespace xll