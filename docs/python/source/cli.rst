Command-line interface
======================

Installing the Python package also installs the ``novasvg`` command, so SVG files
can be converted without writing any Python.

.. code-block:: bash

   novasvg input.svg                              # convert, writes input.png
   novasvg input.svg -o output.png -w 800 -H 600  # choose output file and size
   novasvg info image.svg --json                  # document information
   novasvg query "circle" input.svg               # query elements with a CSS selector
   novasvg batch ./svgs ./images                  # convert a whole directory

The ``novasvg-cli`` entry point runs the pre-built native binary directly.
See the `full CLI reference <../../cli/index.html>`_ for every option.
