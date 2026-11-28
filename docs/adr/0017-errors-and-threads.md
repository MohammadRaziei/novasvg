# ADR-0017: How the library reports errors, and what may be used from several threads

**Status:** Accepted and implemented (2026-11-26).
**Date:** 2026-11-26
**Deciders:** Mohammad Raziei (maintainer)

This writes down what the code already did (found by reading it and by running it under ThreadSanitizer), and closes the
one place where it did not hold.

## Errors

**Decision.** The core and the public C++ headers throw no exception of their own. A failure is a value:

| call | on failure |
| --- | --- |
| `document::load_file`, `load_data` | `nullptr` |
| `render_to_pixmap` | a null `pixmap` (no size, a side of 32768 px or more, or no memory for the pixels) |
| `render_to_image` | an empty `stb::image` |
| `render_to_pdf` | an empty vector (the document has no size) |
| `save_pdf` | `false` |
| `query_selector_all` | an empty list; `is_valid_selector()` tells a selector that matches nothing from one that is wrong |
| `add_font_file` | `0` faces added |

Inside, libstb and the allocation of a pixel buffer can throw; the core catches `std::exception` at those few
boundaries (image decoding in `svg_image.cpp`, font loading in `font.cpp`, `font_face_cache.cpp` and
`embedded_font.cpp`, the `pixmap` constructors) and turns it into one of the values above. The command line has a
top-level handler.

The Python module is the one place that raises, because that is what Python code expects: `InvalidColorError` (a
`ValueError`) for a colour that does not parse, `ValueError` for an unknown image format or a null `Bitmap`, `RuntimeError`
when encoding fails or the simple foreignObject renderer is not built in.

**What is not promised.** An ordinary allocation (a DOM node, a string, a vector) that fails throws `std::bad_alloc`, and
nothing catches it on the way out of the public API. The public headers carry no `noexcept`.

## Threads

**Decision.**
1. Documents that share no object may be loaded and rendered on different threads at the same time. The one thing they
   share is the process-wide font face cache, and it is locked (`font_face_cache`'s mutex).
2. One document may be rendered, to an image or to a PDF, by several threads at once. A document fills in its layout and
   the memo of every paint bounding box lazily, on the first call that needs them, `const` calls included; both are now
   done under one recursive lock on the root element (`svg_root_element::state_mutex()`; recursive because they call each
   other).
3. A call that changes a document (adding a style sheet, setting an attribute) needs the document to itself.
4. Not examined, so not promised: using the `element` and `node` handles of one document from several threads, and the
   const queries other than rendering.

**Evidence (ThreadSanitizer, tests/cpp/test_thread_safety.cpp).**
- Eight threads, four rounds each, every one loading and rendering its own document (text, a gradient, a mask, a clip
  mask, a foreignObject, as an image and as a PDF), started on a cold process so that a race in the first use of a cache
  would show: no report, and every result equal to the single-thread one.
- One document at a time, every SVG in `data/`, rendered by three threads at once to an image
  and to a PDF, half of the documents with their layout done first and half not: before the lock, 12 reports at two
  places (the `paint_bounding_box()` memo and the read of the same field); with it, none. Nothing else in the renderer
  was reported across those samples.

**Python.** The binding never releases the GIL, so Python threads take turns and none of this can race there; a thread
that renders blocks the others. With point 2 in place a render could release it later without a data race in the
renderer.

## Consequences
- A lock per call to `layout_if_needed()` and `paint_bounding_box()`: an uncontended recursive mutex, not measurable next
  to a render.
- The tests keep the contract: the two thread tests run in the normal suite (as plain tests, and as the TSAN run when the
  build is made with `-fsanitize=thread`).
