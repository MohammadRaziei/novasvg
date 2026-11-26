Documents and Elements
======================

A :class:`novasvg.Document` is loaded from a file or from a string, can be queried
with CSS selectors, styled with a stylesheet and rendered. Its tree is made of
:class:`novasvg.Element` and :class:`novasvg.TextNode` objects, both of which derive
from :class:`novasvg.Node`.

.. code-block:: python

   import novasvg

   doc = novasvg.Document.load_from_file("artwork.svg")
   for rect in doc.query_selector_all("rect"):
       print(rect)

PDF
---

``render_to_pdf()`` returns a one-page vector PDF as ``bytes`` and ``save_pdf()`` writes
it to a file. Shapes and text stay vector at any zoom, ``<mask>`` and clip paths are PDF
soft masks, the text can be selected, searched and copied, and the document's ``<title>``
(:attr:`novasvg.Document.title`) is the PDF's title. ``PdfMode.RASTER`` makes the page one
bitmap at a given dpi instead.

.. code-block:: python

   doc = novasvg.Document.load_from_file("artwork.svg")
   data = doc.render_to_pdf()                       # PdfMode.VECTOR, 192 dpi for what must be a bitmap
   doc.save_pdf("artwork.pdf", novasvg.PdfMode.VECTOR)

A PDF luminosity mask weighs red, green and blue as 0.30 / 0.59 / 0.11, an SVG luminance
mask as 0.2125 / 0.7154 / 0.0721: grey and gradient masks are identical, a saturated colour
in a mask is not (use ``PdfMode.RASTER`` if that matters).

Reference
---------

.. autoclass:: novasvg.Document
   :members:

.. autoclass:: novasvg.PdfMode
   :members:

.. autoclass:: novasvg.Element
   :members:

.. autoclass:: novasvg.Node
   :members:

.. autoclass:: novasvg.TextNode
   :members:
