MockXL
======

MockXL is a thin test harness that loads an XLL add-in as a shared library and
drives it through the same lifecycle that Excel itself uses — without requiring
Excel to be installed.  It is the recommended way to write unit and integration
tests for LibXLL-based add-ins.

The public API consists of a single class: ``MockXL::Session``.  All other
types in the ``MockXL`` namespace are internal implementation details.

.. code-block:: cpp

   #include <MockXL/Session.hpp>


Overview
--------

A ``MockXL::Session`` object owns one loaded XLL for its lifetime.  Creating
it calls ``xlAutoOpen``; destroying it calls ``xlAutoClose``.  Between
construction and destruction, any function registered by the add-in can be
called through ``Session::call``.

.. code-block:: cpp

   MockXL::Session session{ "path/to/addin.xll" };

   xll::Number a{ 3.0 }, b{ 4.0 };
   xll::Any result = session.call<"ADD.NUMBERS">(a, b);


``MockXL::Session``
-------------------

.. cpp:class:: MockXL::Session

   Scoped RAII owner of one loaded XLL add-in.

   Only one ``Session`` may exist at a time.  Attempting to construct a second
   instance while the first is alive throws ``std::runtime_error``.

   ``Session`` is non-copyable and non-movable.

Construction and destruction
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. cpp:function:: explicit Session(const std::filesystem::path& xllPath)

   Loads the XLL at *xllPath* and drives the full add-in initialisation
   sequence:

   1. Enforces the single-instance invariant.
   2. Loads the shared library (``RTLD_NOW | RTLD_GLOBAL`` on Linux;
      equivalent flags on Windows).
   3. Resolves ``xlAutoFree12`` so DLL-allocated return values can be
      released after each call.
   4. Configures the internal ``Excel12Server`` singleton with the free
      callback, the XLL name, and a symbol resolver for ``xlfRegister``.
   5. Injects the ``Excel12Server::dispatch`` callback into the XLL via
      ``SetExcel12EntryPt``, routing every ``Excel12`` / ``Excel12v`` call
      made from inside the XLL to MockXL.
   6. Calls the XLL's exported ``xlAutoOpen``.

   :param xllPath: Path to the ``.xll`` / ``.dll`` / ``.so`` file.
   :throws std\:\:runtime_error: If a ``Session`` already exists, if the
      library cannot be loaded, or if ``xlAutoOpen`` is not exported.

   .. note::

      **Windows** — ``SetDllDirectoryA`` is called with the XLL's parent
      directory before loading, so MinGW runtime dependencies
      (``libstdc++``, ``libgcc``, etc.) are found automatically.

      **Linux** — the XLL is loaded with ``RTLD_GLOBAL`` so that the
      hidden ``pexcel12`` variable inside the XLL is reachable by the
      ``SetExcel12EntryPt`` call in step 5.

.. cpp:function:: ~Session()

   Calls ``xlAutoClose`` and unloads the shared library.  If ``xlAutoClose``
   is not exported, a message is printed to ``std::cerr`` and destruction
   continues.  The instance counter is decremented so a new ``Session`` may
   be created afterwards.

Calling registered functions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. cpp:function:: template<fixstr::fixed_string Name, typename... Args> \
                  xll::Any call(Args&&... args) const

   Calls the registered XLL function whose Excel-visible name equals *Name*.

   The name is resolved at compile time; no runtime string lookup is
   performed.  Each argument is deep-copied into an ``xll::Any`` and
   forwarded to the function.

   :tparam Name: Compile-time Excel function name, e.g. ``"ADD.NUMBERS"``.
   :tparam Args: Argument types; each must be derived from ``XLOPER12``
                 (e.g. ``xll::Number``, ``xll::String``, ``xll::Bool``).
   :returns: An owning ``xll::Any`` containing the return value, or
             ``xll::Nil`` if the name is not registered.

   .. code-block:: cpp

      xll::Number a{ 3.0 }, b{ 4.0 };
      xll::Any result = session.call<"ADD.NUMBERS">(a, b);

.. cpp:function:: template<typename... Args> \
                  xll::Any call(std::string_view excelName, Args&&... args) const

   Runtime overload.  Identical to the compile-time overload except that the
   Excel name is supplied as a ``std::string_view`` at runtime.  Use this
   when the function name is not known at compile time.

   :param excelName: Excel-visible name under which the function was
                     registered (case-sensitive).
   :tparam Args: Argument types; each must be derived from ``XLOPER12``.
   :returns: An owning ``xll::Any`` containing the return value, or
             ``xll::Nil`` if the name is not registered.

   .. code-block:: cpp

      xll::Number a{ 3.0 }, b{ 4.0 };
      xll::Any result = session.call("ADD.NUMBERS", a, b);

Custom Excel function handlers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

MockXL has built-in handling for a small set of Excel API calls
(``xlFree``, ``xlGetName``, ``xlfRegister``, ``xlCoerce``).  All other
codes produce a silent default response — ``xlretSuccess`` with
``xll::Nil`` — unless a custom handler is registered.

.. cpp:function:: void register_handler(int xlfn, HandlerFn handler)

   Registers *handler* for the Excel function/command code *xlfn*.  If a
   handler is already registered for that code it is silently replaced.

   Typical use is to intercept ``xlcAlert`` or similar side-effecting calls
   during tests so they do not produce real dialogs:

   .. code-block:: cpp

      session.register_handler(xlcAlert, [](auto& args, auto& result) {
          result = xll::Bool(true);   // swallow the dialog, return success
          return xlretSuccess;
      });

.. cpp:function:: void unregister_handler(int xlfn)

   Removes the custom handler for *xlfn*.  After this call, MockXL reverts
   to the silent-default behaviour for that code.

Accessors
~~~~~~~~~

.. cpp:function:: const std::filesystem::path& path() const noexcept

   Returns the absolute path to the loaded XLL.


Required XLL function signature
---------------------------------

MockXL dispatches every registered function through a uniform
function-pointer type whose parameters and return type are all ``xll::Any*``:

.. code-block:: cpp

   xll::Any* fn(const xll::Any*, const xll::Any*, ...);

XLL-exported functions **must** therefore be declared with ``xll::Any*`` as
both the return type and every parameter type:

.. code-block:: cpp

   XLL_FUNCTION xll::Any* XLLAPI MyFunction(const xll::Any* a, const xll::Any* b);

Using a more specific ``XLOPER12``-derived type (e.g. ``xll::Number*``,
``xll::String const*``) in the signature instead of ``xll::Any*`` is
**binary-compatible** on all supported platforms — every xll type inherits
from ``XLOPER12`` and adds no data members — but constitutes undefined
behaviour under the C++ type system, because the function is called through a
pointer to a different (though layout-compatible) type.  Undefined Behaviour
Sanitizer (``-fsanitize=function``) will emit a warning for every such call.

To keep UBSan clean, always use ``xll::Any*`` in the exported signature and
cast to the specific type inside the function body:

.. code-block:: cpp

   XLL_FUNCTION xll::Any* XLLAPI AddNumbers(const xll::Any* a, const xll::Any* b)
   {
       auto na = xll::cast<xll::Number>(*a);
       auto nb = xll::cast<xll::Number>(*b);
       if (!na || !nb)
           return xll::AutoFree()(xll::Any{ xll::ErrValue });
       return xll::AutoFree()(xll::Any{ *na + *nb });
   }

