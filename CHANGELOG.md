## [Unreleased]

### Added

- **Python: what the C++ API and the CLI had.** `novasvg.fonts.add_font_file(path)` (every face of a .ttf/.otf/.ttc, returns how many), `novasvg.fonts.measure_text(text, font_family, font_size, bold=False, italic=False)` and the numeric-weight form `measure_text(text, font_family, font_size, 700)` (returns `TextMetrics`: `width`, `ascent`, `descent`, `line_gap`; `True` means bold, not weight 1), `novasvg.is_valid_selector(selector)`, and `Document.render_to_pdf(mode=PdfMode.VECTOR, dpi=192.0)` / `Document.save_pdf(filename, mode, dpi)` with `novasvg.PdfMode.VECTOR | RASTER`.
- **`nova::svg::is_valid_selector(selector)`**: `query_selector_all()` returns an empty list both for a selector that matches nothing and for one that is wrong; this tells them apart. `novasvg query` uses it, so a bad selector is an error (exit 1, "Invalid or unsupported selector") instead of "Found 0 element(s)" with exit 0.
- Tests for the XML prolog and entities (`test_xml_prolog.cpp`: DOCTYPE, comments, predefined entities, character references, CDATA; a DTD `<!ENTITY>` is not expanded, now a documented limit) and for the dash fix (`test_stroke_dash.cpp`). ADR-0013 now holds the measured parse shares (5% to 20% of a render for drawings) and is closed as not worth a rewrite.
- **`monospace` and `serif` have an embedded fallback**: DejaVu Sans Mono and DejaVu Serif (regular and bold each, Latin subsets, 12 to 17 KB each) are compiled in next to DejaVu Sans, so on a machine without fonts `font-family: monospace` is drawn with equal-width characters and `serif` with serifs, instead of the proportional Sans. There is no oblique Mono or Serif, so an italic request gets the upright face. The Mono and Serif subsets are from DejaVu 2.37, the Sans ones from 2.35. The full Mono and Serif fonts are not kept in the repository, only the subsets: `tools/gen_default_font.py` takes the directory that holds them as its argument.
- **`novasvg in.svg -o out.pdf` writes a real one-page vector PDF** (page size = the SVG's size) instead of a PNG with a `.pdf` name. `-w`, `-H`, `-s` and `-b` do not apply to a PDF and print a warning. Any other unknown extension is still PNG.
- **`NOVA_SANITIZE`** CMake option: builds novasvg and the libstb it fetches with AddressSanitizer and UndefinedBehaviorSanitizer (GCC and Clang). Off by default in every build type, and `pyproject.toml` pins it off for wheels. With `NOVA_BUILD_PYTHON` it warns that `import novasvg` needs `libasan` preloaded; run the C++ tests under it instead.
- **`nova::vg::add_font_file(path)`** (new public header `nova/vg/font.h`): registers every face of a font file under the family name, weight range and italic flag the font declares. It is what `measure_text` and the tests need to use a font from a file without touching internal headers.
- **`measure_text(text, family, size, int weight, bool italic = false)`**: the numeric `font-weight` form (1 to 1000, clamped). `measure_text(..., 700)` equals `measure_text(..., true)`; the `bool bold` form stays.
- **PDF output: gradients with `spreadMethod` reflect and repeat** are painted as such (they were written as pad), and a `pdf_canvas::set_info()` / `/Info` entry with the producer and, for callers of the canvas, a title. A page has no date, so the same drawing still gives the same bytes.
- **`foreign_object_font::weight`** (`nova/svg/foreign_object.h`): the CSS font-weight (1 to 1000) a foreignObject painter is asked to draw with. `bold` stays and is `weight >= 600`, for painters that only know regular and bold; novasvg sets both.
- **`@font-face` `font-weight` descriptor**: one weight, a keyword or a range (`300 800`). A static font is registered as the face of that weight (two rules of one family, 300 and 700, used to be two regular faces and the newest won); a variable font serves the declared range as far as its own axis goes. With no descriptor a variable font keeps its whole axis. New `font_face_cache::add_font_face(family, weight_min, weight_max, italic, face)`.
- **Variable fonts render at the weight asked for.** An installed variable font (Open Sans, Inter, Roboto Flex) is one face for every weight; `font_face_cache` now sets its weight axis to 700 for bold and 400 for regular (the result is made once per face and kept), so `font-weight: bold` is no longer drawn regular. This needs libstb with `font::with_weight` (variable fonts).
- **`font-weight` is a number from 1 to 1000** end to end (`css_font_weight` is an `int` now; it was `normal` or `bold`). `bolder` and `lighter` follow the parent's weight as CSS asks, an invalid value is regular as before, and a variable font is set to that weight (clamped to its axis); for static faces the nearest weight wins and, on a tie, the heavier face for a heavy request. `font_face_cache::get_font_face*` take an `int weight` (the `bool bold` forms stay). A variable font added by hand (`add_font_face`, `@font-face`) takes its weight range from its own `wght` axis.
- **`nova::svg::measure_text()` and `nova::svg::text_metrics`**: size a piece of text (width, ascent, descent, line gap) without building a document. It uses the same font-family list lookup, shaping and kerning as `<text>`, and the tests check it against the box of a real `<text>` element.

### Changed

- **PDF: all the W3C blend modes** (`overlay`, `color-dodge`, `color-burn`, `hard-light`, `soft-light`, `hue`, `saturation`, `color`, `luminosity`) are written to the page; only the six oldest were, the rest came out as `Normal`. `add` has no PDF name and stays `Normal`. See ADR-0015 for the fade gradients; a two-colour fade interpolates non-premultiplied in PDF and, it seems, premultiplied in the raster render.
- **PDF: a gradient whose stops have different alpha (a fade) is painted as one**, as a luminosity soft mask (`/SMask`) over the same gradient, in linear and radial form, with pad, reflect and repeat, on fills and strokes, under transforms, `fill-opacity` and group `opacity`. It used to be painted opaque, with a warning. Checked against the raster render with pdfium and poppler (mean difference under 1.2 per channel on a test page), and `qpdf --check` finds no errors. Every graphics state now names its soft mask (`/SMask /None` when it has none).
- **A zero-sum `stroke-dasharray` is `none`** (SVG 1.1 section 11.4), where it used to reach libstb as it was: libstb logged "the dash pattern is not usable" for every stroke, which mermaid's `stroke-dasharray: 0` turned into dozens of warnings on stderr for one diagram (35 on `40-erd-relations`, now 0). The rendering did not change.
- **`pyproject.toml` uses `[[tool.dynamic-metadata]]`** for the version (the `[tool.scikit-build.metadata]` table is deprecated and scikit-build-core warned about it on every build). That table is read from scikit-build-core 1.0.0 on (0.11.x and 0.12.x ignore it and silently build `0.0.0`), so `requires` is `>=1.0.0` now (built and tested with 1.0.0, 1.1.0 and 1.1.2); with build isolation pip installs the newest one anyway.
- **The embedded fallback fonts are byte arrays now, not base64.** `src/sources/vg/default_font.h` (98 KB of base64 text) is gone: `tools/gen_default_font.py` writes the Latin subsets as zlib-compressed `data/fonts/embedded/*.ttf.z` (125 KB in all, committed), and `cmake/EmbedFile.cmake` turns them into a header of `unsigned char` arrays in the build tree at configure time. The build needs no Python or other tool. The font bytes are identical to before. (The old generator script wrote to a path and a namespace that no longer existed; it works again.)
- **`novasvg query --json` printed invalid JSON**: the `global_bbox` object was never closed (and had a stray quote), and strings were not escaped, so an attribute value or a Windows path in `info --json` could break it too. All strings go through one `json_escape` now.
- **Layers and patterns are part of the `canvas` interface.** `canvas` gained `begin_layer` / `end_layer` (a group with opacity, blend mode and, for masks, clip masks and filters, a callback that runs on its pixels), `set_pattern` and `raster_scale`. The svg layer no longer casts to `pdf_canvas` or `raster_canvas`; a PDF page decides itself whether a group is a vector transparency group or a bitmap. Output is unchanged: the sample renders and 68 sample PDFs match the build before the change byte for byte (except the fix below).
- **foreignObject text uses numeric weights.** A run's style is `{weight, italic}` instead of two bits: `font-weight: 300` or `550` inside the HTML reaches the face (a variable font is set to it), `<b>` and `<strong>` are `bolder` than the weight they sit in (400 gives 700, 700 gives 900), and `lighter` and `bolder` follow the parent. `measure_foreign_object` takes an `int weight` instead of `bool bold`.
- Faces of equal distance in the cache: up to 500 the lighter face wins, above 500 the heavier one (the CSS font matching order); a variable font's bold flag no longer counts against it when its weight range holds the request.
- novasvg builds against the builder-style libstb API: `stb::loader(...)` for decoding images and `stb::shaping(...)` for shaping a part of a run replace `stb::load_options` and `stb::shape_options`, which libstb no longer has.
- `svgparser.hpp` split into `svg_style.h` (types and declarations) and `svg_style.cpp` (bodies); the CSS helpers are no longer copied into every translation unit that included the header.
- **Every `mix-blend-mode` keyword is supported**: `color-dodge`, `color-burn`, `hard-light`, `soft-light`, `hue`, `saturation`, `color` and `luminosity` join the nine that worked (libstb 's new `blend_mode` values; they painted as `normal` before). Keywords are matched case-insensitively; `plus-lighter` is a composite operator in CSS, not a keyword of this property, so it still paints as `normal`. Intentional golden changes: `golden-blend-modes` (its `color-dodge` and `hard-light` tiles) is re-baselined and `golden-blend-modes-2` is new; sample pixels of both agree with the W3C formulas to within one level. A unit test covers all eight keywords.
- **Gradients and textures are plain objects.** The malloc'd, hand-counted `paint_t` family (`solid_paint_t`, `gradient_paint_t` with a stops array after the struct, `texture_paint_t` with a `shared_ptr` constructed in place, `paint_create_*`, `paint_reference/destroy/get_reference_count`, `canvas_set_paint/get_paint`, `paint.{h,cpp}`) is gone. `gradient_paint` (a `std::vector` of stops, clamped and made non-decreasing when it is set) and `texture_paint` are structs, `paint_source` is a `std::variant` of the two, and a canvas state holds a `std::shared_ptr<const paint_source>` (none means the plain color), so a saved state shares the paint without a count. Sampling (`paint_gradient`, `paint_texture` in `paint_spans.cpp`) is unchanged; output is identical (golden, tolerance 0).
- **`font_face_cache` is a plain class.** The C layer under it (`font_face_cache_t`, `font_face_cache_create/reference/destroy/reference_count/reset/add/add_file/get/load_file/load_dir/load_sys`, with its hand-made reference count) is gone: the class owns the table and the mutex, has a public constructor (an empty cache; the global one loads the system fonts on first use) and the methods `add_font_face`, `add_file`, `load_file`, `load_dir`, `load_system`, `reset`, `get_font_face_local`. Selection is unchanged (bold and italic, then width, then weight, the newest face wins a tie); output is unchanged (golden, tolerance 0).
- **The font layer is stb::font.** `font_face` is a shared pointer to a `stb::font` (`get()` returns it; two copies are the same face when `get()` is equal) and `font_face_data`, its glyph cache, its mutex, its C-style out-parameter metrics and its manual reference count are gone: what depends on a pixel size (shaping, glyph outlines, `x_height`, `glyph_boxes`) lives in `sized_font`, and the outlines are cached inside libstb now (`font::outline`). Files and memory load through `stb::font::open`/`from_memory`; `add_font_face_from_data` and `font_face(data, length)` copy the bytes (no `destroy_func`/`closure`); `font_face_cache_get` returns a `font_face`. The hand-written UTF-8/16/32 `text_iterator` and `text_encoding` are gone (text is `std::u32string_view`, decoded with libstb's `utf8_decode_lossy`, encoded with `utf8_encode`); `font_face.{h,cpp}` is now `font_face_cache.cpp`. Removed the C canvas calls nobody used (`canvas_add_glyph`, `canvas_clip_text`, `canvas_font_metrics`, `canvas_glyph_metrics`, `canvas_text_extents`, `canvas_select_font_face`, `canvas_add_font_face`/`_file`, `canvas_set/get_font_face(_cache)`). Output is unchanged (golden, tolerance 0; a 1500-line text benchmark renders byte-identical and as fast as before). Malformed UTF-8 in text now becomes U+FFFD per bad byte (it decoded to arbitrary code points before).
- The Python module's nested build now gets `FETCHCONTENT_SOURCE_DIR_LIBSTB` too, so it builds against the same libstb as the rest.
- **`mix-blend-mode: overlay`** is supported (libstb's `blend_mode::overlay`; it painted as `normal` before). `golden-blend-modes` is re-baselined for it, the only golden image that changed; a unit test checks the W3C formula. `color-dodge`, `color-burn`, `hard-light`, `soft-light` and the non-separable modes are still treated as `normal`.
- **The Python module builds again**: `binding.cpp` follows the current font API (`global_font_face_cache()`, `sized_font`); its nanobind module has a domain of its own (`NB_DOMAIN novasvg`), so `import libstb, novasvg` no longer aborts on both registering `stb::matrix`; the extension's runpath points at `libstb/sdk/lib`, where libstb installs its library now.
- **Spans, base64, pixmap and parsers from libstb or shared** (libstb `span_set`, `try_base64_decode`, `premultiplied_image::fill` / `into_image`): `span_buffer_t` and the `novasvg_array_*` macros are `stb::span_set` (about 170 lines); `base64_decode` is `stb::try_base64_decode` (browsers' forgiving decode: white space ignored, padding optional but complete); a finished render leaves the canvas without a copy; `vg::blend_mode` and the CSS mix-blend-mode enum are `stb::composite_op` + `stb::blend_mode`. `utils.cpp` is gone: the SVG number and separator parser lives once in `parserutils.h` (`parse_number` leaves its input alone when it fails), and the transform list and path data parsers use it. A transform with an `em` or `ex` length is invalid (it was by accident of the exponent rule, only sometimes). Golden images identical.
- **Dead code out of `rasterize()`**: `cubics_only` (quadratics raised to cubics before libstb's rasterizer, which converts them itself); golden images identical.
- **Row compositing and font discovery come from libstb** (libstb issues 10 and 11). The Porter-Duff operators and CSS
  blend modes on rows of premultiplied pixels are `stb::composite_row` / `stb::composite_solid_row` (about 280 lines of
  `blend.{h,cpp}` and the hand-written SSE2 fill are gone; gradient and texture sampling stay). The font cache lists
  faces with `stb::font::scan` / `system_fonts` (the sfnt reader of `font_face.cpp` is gone, about 250 lines) and picks
  a face by bold, italic, width and weight, so `DejaVu Sans` no longer resolves to the Light or Condensed face; a face
  is also known under its legacy family name (`DejaVu Sans Light`). The system font folders are libstb's (user font
  folders are now searched too). Output unchanged: golden images identical at tolerance 0.
- **The library is split into `nova::svg` and `nova::vg`.** `nova::vg` is the drawing layer (canvas, `pixmap`, fonts,
  PDF output) and `nova::svg` the SVG engine on top of it; the headers live under `include/nova/`. foreignObject
  rendering is an extension (ADR-0010).
- **STL-like naming.** Types, functions, members and enumerators are `snake_case`; keywords used as names get a
  trailing underscore (`auto_`, `class_`, ...). A variable, member or parameter that has the name of a type gets a
  trailing underscore as well (`fill_rule_`, `selector_()`), and `rect::Empty` is `rect::empty_rect`.
- **`Bitmap` is `vg::pixmap`; the public image type is `stb::image`** (ADR-0003). Fonts and image codecs come from
  libstb (ADR-0005, ADR-0006).

### Fixed

- `Element` and `Document` shared their size-fitting and layer-blending code by copy; it is one helper now. No change in output.
- **Fixes of 0.11.5 and 0.11.6, brought over from the header-only line.**
  - A `<line>` that runs left or up (`x2 < x1`, `y2 < y1`) has a box of positive size. A negative one made the box empty, so a group with `opacity` that held only such a line (d3's axis ticks, `y2="-111"`; Mermaid's gantt grid) drew nothing.
  - `<foreignObject>` text wraps after a hyphen, like a browser: Mermaid's `DELIVERY-ADDRESS` is two lines at 120 px. A line breaks right after a hyphen-minus inside a word, except after a hyphen that opens the word or follows another hyphen and before a digit or another hyphen (`a-1`, `--x` stay whole). Measure and paint use the same rule.
  - `<foreignObject>`: `<code>`, `<tt>`, `<kbd>`, `<samp>` and an inline `font-family: monospace` (also after a quoted name, `'Courier New', monospace`) are drawn and measured with the `monospace` face.
  - `<foreignObject>`: a word too long for a label that may wrap overflows at its natural size from the box's left edge instead of being condensed into the box; only a `white-space: nowrap` line wider than its box is condensed.

- **PDF output kept the opacity of a gradient fill.** `fill-opacity` (and any alpha shared by all stops) on a gradient was dropped, so translucent gradients came out opaque in PDF. A gradient whose stops differ in alpha (a fade) is still painted opaque, with a warning.
- **Right-to-left and Arabic text.** Text is now laid out by libstb's `font::shape`: bidirectional order, joined
  Arabic letters, ligatures, mirrored brackets and kerning come from one place, in the canvas, the PDF canvas and
  `font::measure_text` / `glyph_boxes`. Before, Hebrew came out in reverse order and Arabic letters were not joined.
  Latin text is unchanged (the 66 other golden images are bit-identical).

### Removed

- **The C API `path_t`** (`path.cpp`, `path.h`, about 900 lines) and the bridges to it. The canvas builds and the
  rasterizer takes an `stb::outline` directly; SVG path data, arcs and the rounded-rect/ellipse shapes live in
  `outline_util.cpp` (same geometry as before: the 67 golden images are bit-identical).
- The single-header (dist) build and the vendored stb sources (ADR-0001, ADR-0005).

### Internal

- `nova::utils` (`src/sources/utils/text.h`): `is_space`, `trim`, `ltrim`, `rtrim`, `to_lower`, shared by the svg and
  vg layers instead of per-function lambdas.
- Private C struct tags no longer share a name with a class (`font_face_impl`, `font_face_cache_impl`); a clash made
  `delete face` recurse into the class destructor.
- snake_case pass 3: the last PascalCase types (`align_type`, `spread_method`, `length_context`, `simple_selector`,
  `match_type`, `gradient_stop(s)`, `length_adjust`, `marker_units`, `meet_or_slice`, `orient_type`, `dash_array`,
  `attribute_selector`, `pseudo_class_selector`), the matching camelCase variables and members (with a trailing `_`
  where the name equals a type), and `property_id` enumerators. SVG attribute names in strings stay as the SVG spells them.
- `NOVA_INLINE` is gone (it was `inline`); `include/nova/config.h` no longer defines it.
- ADR-0011 step 1: the eight C enums in `render.h` (`fill_rule_t`, `line_cap_t`, `line_join_t`, `spread_method_t`,
  `texture_type_t`, `text_encoding_t`, `path_command_t`, `operator_t`) are now `enum class`; the duplicate enum classes in
  `canvas.h` and their cast code are gone. `operator_t` became `blend_mode` (all Porter-Duff and CSS modes); `xor` is `exclusive_or` (`xor` is a C++ keyword; also `porter_duff::exclusive_or`).
- ADR-0011 step 2 (part 1): `matrix_t` and `point_t` are `stb::matrix` and `stb::point`. The 17 `matrix_*` C functions are gone:
  call sites use `stb::matrix::translation/scaling/rotation/shearing`, `.translate/.scale/.rotate/.shear`, `.then`, `.map` and
  `.inverse()`; three helpers remain in `render.h` (`map_points`, `mapped_bounds`, `parse_matrix`). `transform::inverse()`
  of a singular matrix now gives the identity instead of an uninitialised matrix. `to_matrix` (a copy) is removed.
- ADR-0011 step 3 (part 1): `font_face_t` is the class `font_face_data` (`font_face.h`); the twelve `font_face_*(face, ...)` functions
  are its member functions (`face->shape(...)`, `face->get_metrics(...)`), the loaders are `font_face_data::load_from_file/data`, and
  `render.h` lost ten declarations. Reference counting stays intrusive until the C canvas layer goes.
- Less duplication with libstb (ADR-0011): `fill_rule`, `line_cap`, `line_join` are libstb's; `stroke_style_t`, `stroke_dash_t` and
  `stroke_data_t` are `stb::stroke_style`; `span_t` is `stb::span`; the drawing state is allocated with `new` (it holds a `std::vector`).
- `color_t` (a float RGBA, plus `color_init_*`, `color_to_*` and the `NOVA_*_COLOR` macros) is gone: colours are `stb::color` everywhere,
  gradient stops included (their colour was already 8-bit through `with_alpha`; golden 0 diff). `color_parse` now fills a `stb::color`
  and the SVG code no longer round-trips colours through a packed ARGB value.
- `add_rect_path`, `add_round_rect_path`, `add_ellipse_path` are gone: `stb::outline::add_rect`, `add_rounded_rect`, `add_ellipse`
  draw the same shapes.
- Builds against libstb 6af6df8 (GPOS mark placement, `glyph_kind`, shaping of a text range): no code change was needed, golden 0 diff.
- Builds against libstb 7ede1f4 (CSS colours, Porter-Duff operators, `map_rect`, `outline::arc_to`, `tight_bounds`; `hsl()` rounds like
  browsers). Less duplication with libstb (ADR-0011): `color_parse` and the 148-name colour table, `map_points`/`mapped_bounds`,
  `arc_to`/`arc_to_radians` and `bounding_rect` are gone and `stb::color::try_parse`, `stb::matrix::map`/`map_rect`, `stb::outline::arc_to`
  and `outline::tight_bounds` are used. Three golden images (`01-venn`, `34-venn`, `42-mindmap-shapes`) changed on arc edge pixels only
  (libstb splits a semicircle into three cubics, we used two); the old images are in `refactor/golden-baseline-step32/`.
- `ref_count_t` is `std::atomic<int>` on every platform (the three per-compiler variants are gone).
- `README_PYTHON.md` used the C++ spelling `stb::color` in its Python examples (a rename slip); it is `Color` again. The generated
  `docs/naming-table-internal.md` is deleted and `docs/naming-table.md` is marked as the applied, historical record of ADR-0004.
- Text is shaped as a run (ADR-0012): kerning, ligatures and joined letters that cross a `<tspan>` or per-character position boundary
  are kept (`shaping_context`, libstb's `shape(utf8, begin, end)`); `letter-spacing` turns the optional ligatures off. Golden 0 diff.
- `surface_t` is `stb::premultiplied_image` (libstb issue 9): `surface.{h,cpp}` (the malloc'd, intrusively counted surface, its
  loaders and `surface_clear`) are gone. `pixmap`, `raster_canvas`, the canvas and texture paints share one through `std::shared_ptr`;
  images are decoded by `stb::image` and premultiplied with `premultiplied_image::from_image` (`pixmap_from_image`);
  `convert_argb_to_rgba` / `convert_rgba_to_argb` (unused) are removed; `base64_decode` moved to `vg/utils.cpp`. Golden 0 diff.
- `NOVA_LIBSTB_GIT_TAG` is `master` again. Checked against libstb e9702c0 (`premultiplied_image`, `image::premultiplied()`, GSUB/GPOS for
  left-to-right text): no code change was needed, golden 0 diff.
- `nova::utils` has `starts_with` and `iequals` (C++17 has neither on `string_view`); the angle-unit lambda in `vg/utils.cpp`, the `!important`
  loop, the font-file extension loop and the `data:` check use them.
- The last non-snake_case names: `property_id::RefX/RefY` -> `ref_x/ref_y`, and the unused accessors `refX()/refY()` -> `ref_x()/ref_y()`.
- Two tests for a damaged WOFF (a table that claims 16 MiB of data, a file cut in half): the font is dropped and the text falls back.
- ADR-0012 (proposed): shape a text run once and split the glyphs into fragments (`svg_text.cpp` does not do it yet).

## [0.11.5] - 2026-11-05

### Fixed

- **CSS attribute selectors on attributes novasvg has no property for.** `[data-k]`, `[data-k="1"]`,
  `.a[data-k="1"]`, `rect[data-k="1"]` and chains like `[data-look="neo"][data-color-id="color-0"].node` (what
  Mermaid 12.3 emits to color ER entities and other nodes) never matched, because the parser dropped every
  attribute it has no `property_id` for. Such attributes are now kept on the element (and copied by `<use>`) and
  matched by name. Not covered: attribute selectors against the HTML inside a `<foreignObject>`.

## [0.11.4] - 2026-10-30

### Added

- **CSS custom properties and `var()`.** `--name: value` is read from an element's `style=""` and from
  stylesheets (`:root{--c:red}`, `svg{...}`, `.a{...}`, with the usual cascade: inline over sheet, `!important`
  over both), custom properties **inherit** down the tree, and `var(--name)` / `var(--name, fallback)` is
  replaced in every property value, in presentation attributes as well as in `style` and sheets. A custom
  property can use `var()` itself (what a theme chain like `--_fg: var(--pele-fg, #1f1f1f)` does); the
  fallback is used when a property does not exist or cannot be resolved (a cycle); a value that cannot be
  resolved at all is left unparsed, as if it were not there. Names are case-sensitive. Values are resolved
  once the document (and every sheet applied so far, `Document::applyStyleSheet()` included) is known, so a
  sheet that comes after the element still counts, and a `<use>` copy gets the value its original computed.
  Before this, any `var()` made the property invalid -- a `fill="var(--c)"` was painted black -- which broke
  SVGs that theme themselves with CSS variables (e.g. [pele](https://pele.run)'s output).
  Not covered yet: `var()` inside the HTML of a `<foreignObject>`, and `calc()`/`env()`.

- Benchmarks: a **Human defect grade (HDG)** next to RMSE, which counts pixels rather than what a person
  sees. Every render is graded against the Chromium one on an ordinal scale where lower is better:
  `0` accurate, `1` defective, `2` broken. The grades are kept by hand in `benchmarks/ratings.json`
  (`{"hdg": 0|1|2, "note": ...}` per sample and engine; a first draft covers the 60 samples x 6 engines),
  merged by `report/gather.py` into `render_benchmark.json` (`hdg` / `hdg_note` per cell, per-engine counts
  in `summary`, the metric's description under `metrics.hdg`) and shown in the report as a check / warning /
  stop icon under each render, a legend, and an HDG column in the engine table. An unrated render shows as
  "not rated"; a grade that is not 0, 1 or 2 is reported and ignored.

### Changed

- Benchmarks: nanosvg has no version macro and no release tags, so the report now shows the commit the
  bench was built against (`git 239e102`) instead of a placeholder. CMake reads it from the fetched checkout
  and passes it to `nanosvg_native_bench` as `NANOSVG_COMMIT`.
- Benchmarks: the RMSE comparison against the Chromium ground truth (`benchmarks/python/compare.py`)
  decodes the PNGs with [libstb](https://github.com/MohammadRaziei/libstb) instead of Pillow. It is
  lighter and about 5x faster to decode, and the pixels it reads are identical, so the RMSE values do
  not change. `benchmarks/python/requirements.txt` lists `libstb>=0.7.0` in place of `Pillow`. Pillow stays an
  optional extra for `Bitmap.to_pil()` / `Bitmap.from_pil()`.

### Tests

- `test_benchmark_report.py` covers the HDG merge, the per-engine counts, the "not rated" and invalid-grade
  cases and the icons in the page.
- The benchmark tests (`test_benchmark_corpus.py`, `test_benchmark_report.py`) build their PNGs with
  libstb instead of Pillow. `libstb>=0.7.0` joins the `test` extra and `requirements-dev.txt`; Pillow is kept
  there for the Bitmap interop tests.

## [0.11.2] - 2026-10-17

### Added

- CSS: the structural pseudo-classes `:nth-child()`, `:nth-last-child()`, `:nth-of-type()` and
  `:nth-last-of-type()`, with the full `An+B` syntax (`odd`, `even`, `3`, `2n`, `2n+1`, `-n+3`,
  `2n + 1`...). They are matched against the element's siblings, counted from the first or the last one,
  all siblings or only those of the same element type.

### Fixed

- A single rule the CSS parser could not handle discarded **every following rule** of the `<style>`
  sheet. An unsupported pseudo-class with an argument, such as `:nth-of-type(2)` (which was not
  recognised until now), made `parseStyleSheet` stop at that rule, so rules that came after it, such
  as `#id .node rect{fill:#fff}`, were never applied. Shapes then fell back to an inherited value, e.g.
  the dark root `fill`. This is how SVGs written by merman (mermaid-rs) came out as solid dark boxes.
  An invalid or unsupported rule is now skipped on its own, up to its closing `}`, and parsing carries
  on with the next rule, as CSS error recovery prescribes.

### Tests

- `tests/python/test_css_nth_and_recovery.py`: `An+B` formulas, `nth-last-*`, `nth-of-type` counting
  only same-type siblings, the merman regression (rules after an `:nth-of-type()` rule still apply),
  and dropping an invalid rule without losing the rules before or after it.

## [0.11.1] - 2026-10-17

### Added

- `novasvg::InvalidColorError` (C++, derives from `std::invalid_argument`) and `novasvg.InvalidColorError`
  (Python, a `ValueError` subclass), raised for every color parse failure: empty string, missing `#`,
  wrong length, non-hex digit and unknown color name. Existing `except ValueError` /
  `catch (const std::invalid_argument&)` code keeps working.

### Fixed

- `Color("#gggggg")` raised a bare `ValueError: stoi`. The message now names the problem, e.g.
  `Invalid hex color '#gggggg': 'g' is not a hex digit (0-9, a-f, A-F)`, and a wrong length lists the
  accepted forms.
- `Color::fromHash` silently accepted a bad trailing digit: `#12345g` parsed as a valid color because
  `std::stoi` stops at the first invalid character. Every digit is now checked.

## [0.11.0] - 2026-10-17

### Added

- **Python: `Bitmap` interoperability with the array ecosystem.** NumPy, Pillow and
  [libstb](https://github.com/MohammadRaziei/libstb) are optional extras
  (`novasvg[numpy]`, `novasvg[pillow]`, `novasvg[stb]`); `import novasvg` never loads them.
  - Zero-copy export of the Bitmap's own memory: the buffer protocol (`memoryview(bitmap)`,
    a writable 3-D `(height, width, 4)` uint8 view, no NumPy needed), `bitmap.numpy()` /
    `np.asarray(bitmap)`, DLPack (`__dlpack__`, `__dlpack_device__`: `np.from_dlpack`,
    `torch.from_dlpack`, `jax.numpy.from_dlpack`, `cupy.from_dlpack`) and
    `__array_interface__`, plus `bitmap.shape`.
  - Conversions to ordinary straight-alpha RGBA, as copies: `bitmap.tobytes()`,
    `bitmap.numpy(rgba=True)`, `bitmap.to_pil()` (Pillow) and `bitmap.to_stb()` (libstb).
  - Import, always as a copy: `Bitmap(array)`, `Bitmap.from_array(x, raw=False)`,
    `Bitmap.from_dlpack(x, raw=False)`, `Bitmap.from_pil(img)`, `Bitmap.from_stb(img)`.
    Accepts uint8 `(H, W)` or `(H, W, 1..4)` (L, LA, RGB, RGBA, straight alpha) from any
    buffer, DLPack object or `__array_interface__` object. `raw=True` takes `(H, W, 4)` already
    in the Bitmap's own layout.
  - `Bitmap.copy()`, `copy.copy` and `copy.deepcopy` give an independent deep copy
    (the C++ copy constructor shares pixels). `Bitmap.__repr__`.
- CI: native `ubuntu-24.04-arm` runner for ARM64 wheels, replacing emulation.
- Tests for all of the above, including a run with NumPy, Pillow and libstb blocked.

### Changed

- `Bitmap.numpy()` now takes the keyword-only arguments `rgba`, `dtype` and `copy`. The default is
  unchanged: a zero-copy view of the raw pixels. For a null bitmap it returns an empty
  `(0, 0, 4)` array instead of `None`.
- NumPy is optional: `numpy()` and `np.asarray(bitmap)` raise a clear `ImportError` when it is
  missing, and everything else keeps working.
- `README_PYTHON.md` documents the interop and the pixel format.

### Notes

- A Bitmap always stores premultiplied ARGB32 (BGRA bytes on little-endian). Zero-copy views show
  that memory as it is, so pass a Bitmap to Pillow or libstb through `to_pil()` / `to_stb()`,
  not `PIL.Image.fromarray(bitmap)` or `libstb.Image(bitmap)`, which would read the colours in the
  wrong order.
- `convert_to_rgba()` rewrites the buffer in place; do not combine it with `numpy(rgba=True)` or
  `tobytes()`, which would convert a second time.