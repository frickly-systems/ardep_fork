# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = "ARDEP"
copyright = "2024, Frickly Systems GmbH"  # pylint: disable=redefined-builtin
author = "Frickly Systems GmbH"

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    "sphinx_copybutton",
    "sphinx_tabs.tabs",
]

templates_path = ["_templates"]
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store", "_board/README.rst"]

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = "alabaster"
html_static_path = ["_static"]
html_css_files = [
    "css/custom.css",
]
html_theme_options = {
    "page_width": "1600px",
    "sidebar_width": "300px",
}

breathe_projects = {project: "_build/xml/"}
breathe_default_project = project
