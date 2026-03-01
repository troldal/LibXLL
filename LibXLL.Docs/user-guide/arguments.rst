Function Arguments
==================

Arguments are declared with ``xll::Parameter<T>`` in the function descriptor
and received as ``const T*`` pointers in the C++ implementation.  The type
``T`` determines both what Excel accepts in the cell formula and how LibXLL
marshals the value into C++.

Typed arguments
---------------

Use a concrete type when the function only makes sense with a specific kind of
input.  Excel will coerce compatible types automatically (e.g. ``TRUE``/``FALSE``
to ``1.0``/``0.0`` for a ``Number`` parameter):

.. code-block:: cpp

   auto reg =
       xll::Function("ADD.ONE")
       | xll::Result<xll::Number>()
       | xll::Procedure("AddOne")
       | xll::Parameter<xll::Number>("x", "A number");
   XLL_REGISTER(reg);

   XLL_FUNCTION xll::Number* XLLAPI AddOne(const xll::Number* x)
   {
       static xll::Number result;
       result = xll::Number(static_cast<double>(*x) + 1.0);
       return &result;
   }

Accepting any input with ``xll::Any``
--------------------------------------

Declare the parameter as ``xll::Any`` to receive the raw Excel value without
any prior coercion.  Use ``xll::cast<T>`` to inspect it:

.. code-block:: cpp

   auto reg =
       xll::Function("DESCRIBE")
       | xll::Result<xll::String>()
       | xll::Procedure("Describe")
       | xll::Parameter<xll::Any>("value", "Any Excel value");
   XLL_REGISTER(reg);

   XLL_FUNCTION xll::String* XLLAPI Describe(const xll::Any* value)
   {
       static xll::String result;

       if (xll::cast<xll::Number>(*value))       result = "Number";
       else if (xll::cast<xll::String>(*value))  result = "String";
       else if (xll::cast<xll::Bool>(*value))    result = "Bool";
       else if (xll::holds<xll::Missing>(*value)) result = "(missing)";
       else                                       result = "Other";

       return &result;
   }

Optional arguments
------------------

An argument is optional when the caller may omit it.  Declare the parameter
type as ``xll::Any`` and check for ``xll::Missing`` at runtime:

.. code-block:: cpp

   auto reg =
       xll::Function("GREET")
       | xll::Result<xll::String>()
       | xll::Procedure("Greet")
       | xll::Parameter<xll::Any>("name", "Name to greet (optional)");
   XLL_REGISTER(reg);

   XLL_FUNCTION xll::String* XLLAPI Greet(const xll::Any* name)
   {
       static xll::String result;

       if (xll::holds<xll::Missing>(*name))
           result = xll::String("Hello, stranger!");
       else if (auto s = xll::cast<xll::String>(*name))
           result = xll::String("Hello, ") + *s;
       else
           result = xll::String("Hello!");

       return &result;
   }

Validated string arguments with ``xll::StringEnum``
----------------------------------------------------

When an argument must be one of a known set of strings, use
``xll::StringEnum`` together with ``xll::cast``.  The cast returns an empty
``xll::Optional`` for any value that is not in the allowed set:

.. code-block:: cpp

   using Color = xll::StringEnum<"Red", "Green", "Blue">;

   XLL_FUNCTION xll::Expected<xll::Number>* XLLAPI ColorIndex(const xll::Any* arg)
   {
       static xll::Expected<xll::Number> result;

       const auto color = xll::cast<Color>(*arg);
       if (!color) {
           result = xll::Error::Value;
           return &result;
       }

       result = xll::Number(static_cast<double>(color->index()));
       return &result;
   }

See :doc:`types` for a full description of all available types.

