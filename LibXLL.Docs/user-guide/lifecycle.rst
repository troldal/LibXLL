Add-in Lifecycle
================

Excel calls a set of well-known exported functions at specific points in the
add-in lifetime.  LibXLL handles the low-level exports automatically and
exposes hooks via ``xll::OnOpen``, ``xll::OnClose``, and related helpers.

Lifecycle events
----------------

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Hook
     - When it fires
   * - ``xll::OnOpen``
     - ``xlAutoOpen`` — Excel has loaded the XLL and is ready to register functions
   * - ``xll::OnClose``
     - ``xlAutoClose`` — Excel is unloading the XLL
   * - ``xll::OnAdd``
     - ``xlAutoAdd`` — user has added the add-in via the Add-ins dialog
   * - ``xll::OnRemove``
     - ``xlAutoRemove`` — user has removed the add-in via the Add-ins dialog

Registering a hook
------------------

Each hook is registered the same way as a function — build a descriptor and
call ``XLL_REGISTER``:

.. code-block:: cpp

   auto onOpen =
       xll::OnOpen()
       | xll::Before([] { std::cerr << "xlAutoOpen\n"; });
   XLL_REGISTER(onOpen);

   auto onClose =
       xll::OnClose()
       | xll::Before([] { std::cerr << "xlAutoClose\n"; });
   XLL_REGISTER(onClose);

The ``xll::Before`` pipe runs the lambda *before* LibXLL performs its own
work (registering functions with Excel).  Use ``xll::After`` to run code
after LibXLL's own processing.

Error handling in ``xlAutoOpen``
---------------------------------

If function registration fails, it is often useful to show a dialog rather
than silently continue.  Use ``xll::OnError`` for this:

.. code-block:: cpp

   auto onOpen =
       xll::OnOpen()
       | xll::Before([] { /* initialise resources */ })
       | xll::OnError([](const std::string& msg) {
             xll::alert(xll::String(msg));
         });
   XLL_REGISTER(onOpen);

Add-in manager info
-------------------

The Add-in Manager dialog in Excel calls ``xlAddInManagerInfo12`` to get the
display name of the add-in.  Register this with ``xll::AddInManagerInfo``:

.. code-block:: cpp

   xll::AddInManagerInfo dllName([] { return xll::String("My Add-in"); });

This must be a global variable; no ``XLL_REGISTER`` call is needed.

See :doc:`../api/auto` for the full API reference.

