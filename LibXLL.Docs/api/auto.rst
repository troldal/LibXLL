Auto Functions
==============

Excel lifecycle hooks (``xlAutoOpen``, ``xlAutoClose``, etc.) and add-in manager support.

Lifecycle tags
--------------

The following tag types are used as template arguments to select which
lifecycle event a handler targets:

.. doxygenstruct:: xll::Open
   :project: LibXLL

.. doxygenstruct:: xll::Close
   :project: LibXLL

.. doxygenstruct:: xll::Add
   :project: LibXLL

.. doxygenstruct:: xll::Remove
   :project: LibXLL

.. doxygenstruct:: xll::Free
   :project: LibXLL

AddInManagerInfo
----------------

.. doxygenclass:: xll::AddInManagerInfo
   :project: LibXLL
   :members:
   :undoc-members:
