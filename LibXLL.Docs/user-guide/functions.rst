Registering Functions
=====================

Every worksheet function must be described to Excel before it can be called.
LibXLL uses a fluent builder API — ``xll::Function`` — to construct that
description, and ``XLL_REGISTER`` to register it with the global registry.

The registration pattern
------------------------

A registration consists of two parts that must share the same procedure name:

1. A global descriptor built with ``xll::Function``.
2. The C++ function implementation, declared with ``XLL_FUNCTION``.

.. code-block:: cpp

   auto addReg =
       xll::Function("MY.ADD")
       | xll::Result<xll::Number>()
       | xll::Procedure("MyAdd")
       | xll::Parameter<xll::Number>("x", "First number")
       | xll::Parameter<xll::Number>("y", "Second number")
       | xll::Category("My Functions")
       | xll::Description("Returns x + y");
   XLL_REGISTER(addReg);

   XLL_FUNCTION xll::Number* XLLAPI MyAdd(const xll::Number* x,
                                           const xll::Number* y)
   {
       static xll::Number result;
       result = xll::Number(static_cast<double>(*x) + static_cast<double>(*y));
       return &result;
   }

Builder methods
---------------

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Method
     - Purpose
   * - ``xll::Function("NAME")``
     - Excel name shown in the formula bar
   * - ``xll::Result<T>()``
     - Return type of the function
   * - ``xll::Procedure("name")``
     - Name of the exported C++ function
   * - ``xll::Parameter<T>("name", "help")``
     - Adds one argument; repeat for each parameter
   * - ``xll::Category("name")``
     - Groups the function in the Insert Function dialog
   * - ``xll::Description("text")``
     - Tooltip in the Insert Function dialog
   * - ``xll::Hidden()``
     - Hides the function from the Insert Function dialog
   * - ``xll::ThreadSafe()``
     - Marks the function as thread-safe for multi-threaded recalc

Return types
------------

Return a pointer to a ``static`` local for all non-trivial types:

.. code-block:: cpp

   XLL_FUNCTION xll::String* XLLAPI Greet(const xll::String* name)
   {
       static xll::String result;
       result = xll::String("Hello, ") + *name;
       return &result;
   }

Use ``xll::Expected<T>`` for functions that may return an Excel error:

.. code-block:: cpp

   XLL_FUNCTION xll::Expected<xll::Number>* XLLAPI MySqrt(const xll::Number* x)
   {
       static xll::Expected<xll::Number> result;
       const double v = static_cast<double>(*x);
       result = (v < 0.0) ? xll::Expected<xll::Number>(xll::Error::Value)
                           : xll::Expected<xll::Number>(std::sqrt(v));
       return &result;
   }

Why ``XLL_REGISTER``?
---------------------

``XLL_REGISTER(var)`` expands to ``extern const xll::AddIn var##_registered(var)``.
The ``extern const`` storage class forces the linker to retain the object even
though nothing in the program explicitly references it.  Without this, some
linkers silently discard global objects and the function would not be registered.

See :doc:`../api/registration` for the full API reference.

