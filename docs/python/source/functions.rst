Functions
=========

Convenience functions for the common "render this SVG to a PNG file" case, and for
reading the library version.

.. code-block:: python

   import novasvg

   novasvg.svg2png("input.svg", "output.png")
   print(novasvg.__version__)

Reference
---------

.. autofunction:: novasvg.svg2png
.. autofunction:: novasvg.version_string
