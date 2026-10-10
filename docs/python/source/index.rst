NovaSVG Python API
==================

**NovaSVG** is a lightweight, header-only C++17 library for parsing, manipulating
and rasterizing SVG files. The Python bindings are built with nanobind and expose
the same document, element and bitmap model, plus NumPy access to the rendered
pixels.

.. code-block:: python

   import novasvg

   doc = novasvg.Document.load_from_data(
       '<svg width="200" height="200" xmlns="http://www.w3.org/2000/svg">'
       '<circle cx="100" cy="100" r="60" fill="green"/></svg>'
   )

   bitmap = doc.render_to_bitmap()
   bitmap.write_to_png("output.png")   # save as PNG
   pixels = bitmap.numpy()             # NumPy array, shape (H, W, 4)

Installation
------------

.. code-block:: bash

   pip install novasvg

.. toctree::
   :maxdepth: 1

   Overview <self>
   document
   graphics
   fonts
   functions
   cli
