Installation
============

Prerequisites
-------------

- A C++23-capable compiler (MSVC 2022, Clang 16+, or GCC 13+)
- CMake 3.28 or later
- Excel 2016 or later (32-bit or 64-bit)

Adding LibXLL to your project
------------------------------

LibXLL is header-only and distributed via CMake.  The recommended way to
consume it is through `CPM.cmake <https://github.com/cpm-cmake/CPM.cmake>`_:

.. code-block:: cmake

   CPMAddPackage(
       NAME LibXLL
       GITHUB_REPOSITORY troldal/LibXLL
       GIT_TAG main
   )

Then link your add-in target against it:

.. code-block:: cmake

   add_library(MyAddIn SHARED MyAddIn.cpp)
   target_link_libraries(MyAddIn PRIVATE LibXLL)

Configuring as an XLL
---------------------

An XLL is a regular Windows DLL with a ``.xll`` extension.  Tell CMake to
produce the right output:

.. code-block:: cmake

   set_target_properties(MyAddIn PROPERTIES
       SUFFIX ".xll"
   )

That is all the CMake configuration required.  The rest is C++.

