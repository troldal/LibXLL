Hello, World!
=============

This tutorial creates the smallest possible LibXLL add-in: a single worksheet
function ``HELLO.WORLD`` that takes no arguments and returns the string
``"Hello, World!"``.

The complete source
-------------------

.. code-block:: cpp

   #include <Auto.hpp>
   #include <Functions.hpp>
   #include <Register.hpp>
   #include <Types.hpp>

   // 1. Describe the function to Excel.
   auto helloWorldReg =
       xll::Function("HELLO.WORLD")
       | xll::Result<xll::String>()
       | xll::Procedure("HelloWorld")
       | xll::Description("Returns the string \"Hello, World!\"");
   XLL_REGISTER(helloWorldReg);

   // 2. Implement the function.
   XLL_FUNCTION xll::String* XLLAPI HelloWorld()
   {
       static xll::String result("Hello, World!");
       return &result;
   }

Step-by-step walkthrough
-------------------------

**Step 1 — describe the function**

``xll::Function`` begins a fluent description of the worksheet function that
Excel will register:

.. code-block:: cpp

   auto helloWorldReg =
       xll::Function("HELLO.WORLD")      // Name shown in Excel
       | xll::Result<xll::String>()      // Return type
       | xll::Procedure("HelloWorld")    // Name of the C++ function
       | xll::Description("...");        // Tooltip shown in Excel

The ``|`` operator pipes each builder step into the next.  The result is
stored in a global variable.

**Step 2 — register with Excel**

``XLL_REGISTER`` is a macro that creates an ``xll::AddIn`` object, which
registers the function descriptor with the global registry.  When Excel calls
``xlAutoOpen`` (handled automatically by LibXLL), every registered descriptor
is passed to Excel via ``xlfRegister``.

.. code-block:: cpp

   XLL_REGISTER(helloWorldReg);

**Step 3 — implement the function**

The C++ function that Excel will call must:

- Be declared with ``XLL_FUNCTION`` (which expands to ``extern "C" __declspec(dllexport)``).
- Use ``XLLAPI`` for the calling convention (``__stdcall`` on 32-bit, nothing on 64-bit).
- Return a *pointer* to a static result for non-trivial types.

.. code-block:: cpp

   XLL_FUNCTION xll::String* XLLAPI HelloWorld()
   {
       static xll::String result("Hello, World!");
       return &result;
   }

.. note::
   The result must be ``static`` (or heap-allocated and freed via
   ``xlAutoFree12``) because Excel reads the returned pointer *after* the
   function returns.  A local variable would be destroyed before Excel reads it.

Loading the add-in
------------------

Build the project to produce ``MyAddIn.xll``, then in Excel go to
**File → Options → Add-ins → Go** and browse to the ``.xll`` file.
Once loaded, ``=HELLO.WORLD()`` should return ``Hello, World!``.

