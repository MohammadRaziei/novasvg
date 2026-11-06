# Naming table (ADR-0004), public C++ API

Methods and the DOM types: **applied** (2026-11-03). `Box`, `Matrix`, `Rect`, `Transform`: **deferred to step 5** (they are replaced by `stb::` types, and `gfx::matrix` already exists). Other internal types (non-`SVG*`) are renamed with their module.
Python names and CLI are unchanged (string literals were not touched).

## Types (applied)
| Old | New |
|---|---|
| `Node` `TextNode` `Element` `Document` | `node` `text_node` `element` `document` |
| `SVGNode` `SVGTextNode` `SVGElement` `SVGRootElement` `SVGURIReference` | `svg_node` `svg_text_node` `svg_element` `svg_root_element` `svg_uri_reference` |

## Methods (applied)
| Old | New | Old | New |
|---|---|---|---|
| `getAttribute` | `attribute` | `isNull` | `is_null` |
| `setAttribute` | `set_attribute` | `isEmpty` | `empty` (STL / libstb style; overrides ADR-0004's `is_` rule for this case) |
| `hasAttribute` | `has_attribute` | `isElement` / `isTextNode` | `is_element` / `is_text_node` |
| `getElementById` | `element_by_id` | `toElement` / `toTextNode` | `to_element` / `to_text_node` |
| `querySelectorAll` | `query_selector_all` | `loadFromFile` / `loadFromData` | `load_file` / `load_data` |
| `elementFromPoint` | `element_from_point` | `renderToImage` / `renderToBitmap` | `render_to_image` / `render_to_bitmap` |
| `getBoundingBox` / `boundingBox` | `bounding_box` | `getGlobalBoundingBox` | `global_bounding_box` |
| `getLocalBoundingBox` | `local_bounding_box` | `paintBoundingBox` | `paint_bounding_box` |
| `getGlobalMatrix` / `getLocalMatrix` | `global_matrix` / `local_matrix` | `localTransform` | `local_transform` |
| `documentElement` / `rootElement` / `parentElement` | `document_element` / `root_element` / `parent_element` | `mapRect` / `postMultiply` | `map_rect` / `post_multiply` |
| `intrinsicWidth` / `intrinsicHeight` | `intrinsic_width` / `intrinsic_height` | `applyStyleSheet` | `apply_style_sheet` |
| `forceLayout` / `layoutIfNeeded` / `updateLayout` | `force_layout` / `layout_if_needed` / `update_layout` | `setData` / `versionString` | `set_data` / `version_string` |

## Variables renamed to free the type names
`element` -> `el`, `node` -> `nd`, `document` -> `dom`. All `SVG*` identifiers became `svg_*` (`SVGPathElement` -> `svg_path_element`).

## Internal identifiers
Second pass (2026-11-06) applied to the engine and the svg layer: see [naming-table-internal.md](naming-table-internal.md) for what is left.
