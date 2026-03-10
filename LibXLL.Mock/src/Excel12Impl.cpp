// Strong definitions of Excel12 and Excel12v for Linux.
//
// The XLL has no definition of Excel12/Excel12v of its own (unix/xlcall_cpp.h
// only includes the declarations from xlcall.h).  The dynamic linker resolves
// them from the host executable at runtime, provided the executable is linked
// with -rdynamic.

#include "MockXL/Excel12Server.hpp"

#ifndef _WIN32

#include <cstdarg>
#include <vector>

extern "C" __attribute__((visibility("default")))
int Excel12(int xlfn, LPXLOPER12 operRes, int count, ...)
{
    std::vector<LPXLOPER12> ops(static_cast<std::size_t>(count));
    std::va_list ap;
    va_start(ap, count);
    for (int i = 0; i < count; ++i)
        ops[static_cast<std::size_t>(i)] = va_arg(ap, LPXLOPER12);
    va_end(ap);
    return MockXL::impl::Excel12Server::instance()
               .dispatch(xlfn, count, ops.data(), operRes);
}

extern "C" __attribute__((visibility("default")))
int Excel12v(int xlfn, LPXLOPER12 operRes, int count, LPXLOPER12 opers[])
{
    return MockXL::impl::Excel12Server::instance()
               .dispatch(xlfn, count, &opers[0], operRes);
}

#endif // !_WIN32

