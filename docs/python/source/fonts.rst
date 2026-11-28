Fonts
=====

Font loading and text measurement live in the :mod:`novasvg.fonts` submodule. It is
backed by the same font stack that paints text and ``<foreignObject>`` content, so a
measurement can never disagree with what is rendered.

.. code-block:: python

   import novasvg.fonts as fonts

   fonts.add_font_face_from_file("DejaVu Sans", False, False, "DejaVuSans.ttf")
   face = fonts.get_font_face("DejaVu Sans", bold=False, italic=False)
   font = fonts.Font(face, size=16.0)
   font.measure_text("Hello, world!")   # width in pixels

A font file is registered with :func:`novasvg.fonts.add_font_file` (every face of a
``.ttf``, ``.otf`` or ``.ttc``, under the names it declares), and a run of text is
measured the way ``<text>`` is with :func:`novasvg.fonts.measure_text`:

.. code-block:: python

   fonts.add_font_file("DejaVuSans.ttf")
   m = fonts.measure_text("Hello, world!", "DejaVu Sans", 16.0)   # a CSS font-family list
   m.width, m.ascent, m.descent, m.line_gap
   fonts.measure_text("Hello", "DejaVu Sans", 16.0, True)          # bold
   fonts.measure_text("Hello", "DejaVu Sans", 16.0, 700)           # a numeric weight

Reference
---------

.. autoclass:: novasvg.fonts.FontFace
   :members:

.. autoclass:: novasvg.fonts.Font
   :members:

.. autoclass:: novasvg.fonts.TextMetrics
   :members:

.. autofunction:: novasvg.fonts.add_font_file
.. autofunction:: novasvg.fonts.measure_text
.. autofunction:: novasvg.fonts.add_font_face_from_file
.. autofunction:: novasvg.fonts.add_font_face_from_data
.. autofunction:: novasvg.fonts.get_font_face
.. autofunction:: novasvg.fonts.get_font_face_for_family_stack
.. autofunction:: novasvg.fonts.measure_foreign_object
