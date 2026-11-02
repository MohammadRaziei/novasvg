# Naming table (ADR-0004), public C++ API

Methods: **applied** (2026-11-03). Types: **pending** (they collide with variables like `Element element`).
Python names and CLI are unchanged (string literals were not touched).

## Types (pending)
| Old | New |
|---|---|
| `Box` `Matrix` `Node` `TextNode` `Element` `Document` | `box` `matrix` `node` `text_node` `element` `document` |
| `SVGNode` `SVGTextNode` `SVGElement` `SVGRootElement` `SVGURIReference` | `svg_node` `svg_text_node` `svg_element` `svg_root_element` `svg_uri_reference` |

## Methods (applied)
| Old | New | Old | New |
|---|---|---|---|
| `getAttribute` | `attribute` | `isNull` | `is_null` |
| `setAttribute` | `set_attribute` | `isEmpty` | `is_empty` (STL says `empty`: open) |
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
