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

Reference
---------

.. autoclass:: novasvg.Document
   :members:

.. autoclass:: novasvg.Element
   :members:

.. autoclass:: novasvg.Node
   :members:

.. autoclass:: novasvg.TextNode
   :members:
