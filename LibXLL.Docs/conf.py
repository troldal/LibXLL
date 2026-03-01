# Configuration file for the Sphinx documentation builder.
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import os

# -- Project information -----------------------------------------------------
project = "LibXLL"
copyright = "XLThermo"
author = "XLThermo"

# -- General configuration ---------------------------------------------------
extensions = [
    "breathe",
    "sphinx_rtd_theme",
]

# -- Breathe configuration ---------------------------------------------------
# The XML path can be overridden at build time with:
#   sphinx-build -Dbreathe_projects.LibXLL=<path> ...
# When running manually from the LibXLL.Docs/ directory the default below is used.
breathe_projects = {"LibXLL": os.path.join(os.path.dirname(__file__), "output", "xml")}
breathe_default_project = "LibXLL"
breathe_default_members = ("members", "undoc-members")

# -- Options for HTML output -------------------------------------------------
html_theme = "sphinx_rtd_theme"

