Introduction
============

LibXLL is a modern C++23 header-only library for building Excel XLL add-ins.
It wraps the raw Excel SDK in a type-safe, expressive API that eliminates the
boilerplate of working directly with ``XLOPER12`` structures.

Why LibXLL?
-----------

The Excel C SDK is powerful but low-level.  Writing add-ins directly against it
requires manually managing ``XLOPER12`` unions, calling ``xlfRegister`` with
positional string arguments, and writing ``xlAutoOpen``/``xlAutoClose`` stubs
by hand.  Mistakes are easy and the compiler cannot help.

LibXLL addresses this by providing:

- **Strongly-typed value wrappers** — ``xll::Number``, ``xll::String``,
  ``xll::Bool``, ``xll::Error``, ``xll::Array``, and more, each mapping
  directly to an Excel type with zero overhead.
- **A fluent registration API** — describe worksheet functions in readable
  C++ rather than positional string arrays.
- **Lifecycle hooks** — ``xll::OnOpen``, ``xll::OnClose``, and related
  helpers manage the add-in lifecycle without boilerplate.
- **Safe casting** — ``xll::cast<T>`` converts an ``xll::Any`` (a raw
  Excel value) to a typed wrapper, returning ``xll::Optional<T>`` so
  invalid input is handled gracefully at the call site.

Design philosophy
-----------------

LibXLL is designed to be thin.  It does not abstract away Excel — it makes
working with Excel's own type system safer and more convenient.  The generated
``.xll`` binary calls the same SDK functions that a hand-written add-in would;
LibXLL just arranges for those calls to be made correctly.

The library requires a C++23 compiler and is tested with MSVC, Clang-cl,
GCC, and Clang on Linux.

