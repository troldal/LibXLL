Types
=====

LibXLL provides a strongly-typed wrapper for every value type that Excel can
pass to or receive from an XLL function.  Each wrapper maps directly to an
``XLOPER12`` variant with no overhead.

Overview
--------

.. list-table::
   :header-rows: 1
   :widths: 20 20 60

   * - C++ type
     - Excel type
     - Notes
   * - ``xll::Number``
     - ``xltypeNum``
     - 64-bit IEEE 754 double
   * - ``xll::String``
     - ``xltypeStr``
     - Length-prefixed wide string
   * - ``xll::Bool``
     - ``xltypeBool``
     - ``TRUE`` / ``FALSE``
   * - ``xll::Int``
     - ``xltypeInt``
     - 32-bit signed integer
   * - ``xll::Error``
     - ``xltypeErr``
     - ``#NULL!``, ``#DIV/0!``, ``#VALUE!``, etc.
   * - ``xll::Array``
     - ``xltypeMulti``
     - 2-D rectangular array of mixed types
   * - ``xll::Missing``
     - ``xltypeMissing``
     - Argument was omitted by the caller
   * - ``xll::Nil``
     - ``xltypeNil``
     - Empty / unset value
   * - ``xll::Any``
     - *(any)*
     - Type-erased container; use to accept arbitrary input

Scalar types
------------

``xll::Number``, ``xll::String``, ``xll::Bool``, and ``xll::Int`` all share
the same interface.  They are implicitly constructible from and convertible to
their underlying C++ type:

.. code-block:: cpp

   xll::Number n(3.14);
   double d = static_cast<double>(n);   // 3.14

   xll::String s("Hello");
   std::string str(s);                  // "Hello"

``xll::Error``
--------------

Error values correspond to Excel error codes.  The named constants are
members of the ``xll::Error`` class:

.. code-block:: cpp

   XLL_FUNCTION xll::Error* XLLAPI DivideNumbers(const xll::Number* a,
                                                   const xll::Number* b)
   {
       static xll::Error err = xll::Error::Div0;
       return &err;   // returns #DIV/0!
   }

``xll::Expected<T>``
---------------------

For functions that can either return a value or an Excel error, use
``xll::Expected<T>``.  It behaves like ``std::expected<T, xll::Error>``:

.. code-block:: cpp

   XLL_FUNCTION xll::Expected<xll::Number>* XLLAPI SafeDivide(
       const xll::Number* a, const xll::Number* b)
   {
       static xll::Expected<xll::Number> result;

       if (static_cast<double>(*b) == 0.0)
           result = xll::Error::Div0;
       else
           result = xll::Number(static_cast<double>(*a) / static_cast<double>(*b));

       return &result;
   }

``xll::Any``
------------

``xll::Any`` holds any Excel value.  It is the right choice when a function
needs to accept or inspect heterogeneous input.  Use ``xll::cast<T>`` to
attempt a conversion to a specific type; the result is an ``xll::Optional<T>``:

.. code-block:: cpp

   XLL_FUNCTION xll::String* XLLAPI TypeName(const xll::Any* value)
   {
       static xll::String result;

       if (xll::cast<xll::Number>(*value))  result = "Number";
       else if (xll::cast<xll::String>(*value))  result = "String";
       else if (xll::cast<xll::Bool>(*value))    result = "Bool";
       else                                       result = "Other";

       return &result;
   }

``xll::StringEnum``
-------------------

``xll::StringEnum`` constrains a string argument to a fixed set of allowed
values, defined at compile time as template arguments:

.. code-block:: cpp

   using Direction = xll::StringEnum<"North", "South", "East", "West">;

When casting an ``xll::Any`` to a ``StringEnum``, the cast returns an empty
``xll::Optional`` if the string is not in the allowed set.  This means
validation and parsing happen in one step:

.. code-block:: cpp

   XLL_FUNCTION xll::Expected<xll::String>* XLLAPI Opposite(const xll::Any* arg)
   {
       static xll::Expected<xll::String> result;

       const auto dir = xll::cast<Direction>(*arg);
       if (!dir) {
           result = xll::Error::Value;
           return &result;
       }

       dir->visit(xll::overload{
           [](Direction::value<"North">) { result = xll::String("South"); },
           [](Direction::value<"South">) { result = xll::String("North"); },
           [](Direction::value<"East">)  { result = xll::String("West");  },
           [](Direction::value<"West">)  { result = xll::String("East");  },
       });

       return &result;
   }

See :doc:`../api/types` for the full API reference.

