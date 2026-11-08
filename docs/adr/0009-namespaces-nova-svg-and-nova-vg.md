# ADR-0009: namespaces `nova::svg` and `nova::vg`; the drawing layer is a library of its own

**Status:** Accepted. Partly implemented (2026-11-08): namespaces `nova::svg` / `nova::vg`, the `NOVA_` macro prefix, the directories, the layering check and `snake_case` are done; the installable `nova::vg` CMake package (`find_package`) is not. Supersedes the namespace decision of ADR-0004 (the `snake_case` rules there stay).
**Date:** 2026-11-05
**Deciders:** Mohammad Raziei (maintainer)

## Context

Today the C-style drawing API is `novasvg::gfx`, the classes (`canvas`, `raster_canvas`, `pdf_canvas`, `Path`, `Transform`,
`Color`, `Font`) are in `novasvg`, the DOM is in `novasvg`, and `color_literals` is `novasvg::color_literals`. The drawing layer
(`canvas` with a pixel backend and a PDF backend, fonts, paths, colours) has nothing to do with SVG and is wanted by other
users, for example plotting. The open question "what is the public gfx library called" was never answered.

## Decision

1. The top namespace is **`nova`** (ADR-0004 had `novasvg`, with `nova` only as an optional alias).
2. **`nova::svg`**: everything about SVG: `document`, `element`, `node`, the SVG renderer, `render_to_pdf`.
3. **`nova::vg`**: the vector-graphics layer: `canvas`, `raster_canvas`, `pdf_canvas`, `path`, `matrix`, `color`, `font`, the C API
   now in `novasvg::gfx`. It does not know SVG. `nova::svg` depends on `nova::vg`, never the other way round.
4. `vg` is built as its **own CMake target** (`nova::vg`) in this repository, so the direction of the dependency is enforced by the
   build. Moving it to its own repository later is mechanical. The project, the pip package and the CLI keep the name `novasvg`.
5. `vg` stays on top of libstb (`stb::outline`, `stb::raster`, `stb::font`, `stb::zlib`, `stb::image`); libstb's core does not get a canvas.
6. Directories follow: `include/nova/svg/`, `include/nova/vg/`, `src/svg/`, `src/vg/`.
7. Order: after step 5b (types become `stb::`) and the split of `svgelement`. The namespace move, the directory move and the
   `snake_case` rename (ADR-0004) are **one scripted step**, so nothing is renamed twice.

## Options considered

- `stb::canvas` inside libstb: keeps one library, but turns libstb from a stateless toolbox into a graphics library and freezes an
  interface that is still incomplete (layers are not virtual yet).
- `gfx`: the current name; says nothing and does not pair with `svg`.
- `draw`: clashes with `stb::draw`.
- `vg`: pairs with `svg`, matches NanoVG / ThorVG / plutovg / OpenVG.

## Consequences

- About 300 `novasvg::` occurrences, all includes and the Doxygen paths change; Python and CLI names do not.
- `Color`, `Matrix`, `Rect`, `Transform` move from `novasvg` to `vg` (and then become `stb::` types in step 5b).
- A plotting library can link `nova::vg` alone. Its axis/tick/legend layer is separate and does not exist yet.

## Action items

- [ ] Write the old-to-new table (extends `docs/naming-table.md`) and run the scripted move.
- [ ] CMake target `nova::vg`; make `vg` sources unable to include `svg` headers.
- [ ] Promote layers (`begin_layer`, `end_layer`, pattern, mask) to virtuals of `vg::canvas` before publishing it as public API.
