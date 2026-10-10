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

Reference
---------

.. autoclass:: novasvg.fonts.FontFace
   :members:

.. autoclass:: novasvg.fonts.Font
   :members:

.. autofunction:: novasvg.fonts.add_font_face_from_file
.. autofunction:: novasvg.fonts.add_font_face_from_data
.. autofunction:: novasvg.fonts.get_font_face
.. autofunction:: novasvg.fonts.get_font_face_for_family_stack
.. autofunction:: novasvg.fonts.measure_foreign_object
