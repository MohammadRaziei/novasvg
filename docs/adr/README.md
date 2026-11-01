# Architecture Decision Records

Decisions for the 0.12 refactor (compiled library, libstb, `stb::image`, stl-like naming). One file per
decision; each records the context, the options that were weighed, what was chosen and why, and what it costs.

| ADR | Decision | Status |
|---|---|---|
| [0001](0001-compile-novasvg-into-a-library.md) | Compile novasvg into a library instead of shipping it header-only | Accepted (step 4 pending) |
| [0002](0002-depend-on-libstb-via-fetchcontent-dynamic.md) | Depend on libstb: fetched with FetchContent, pinned, linked dynamically | Accepted, implemented (step 1) |
| [0003](0003-public-image-type-is-stb-image.md) | The public pixel type is `stb::image`; the premultiplied render surface is internal | Accepted (step 2b pending) |
| [0004](0004-stl-like-naming-and-namespace.md) | stl-like `snake_case` naming; namespace `novasvg` | Accepted (step 6 pending) |
| [0005](0005-replace-vendored-stb-with-libstb.md) | Replace the vendored stb_image, image writer and inflater with libstb | Accepted, implemented (step 2a) |
| [0006](0006-fonts-on-libstb.md) | Build fonts on libstb; add a glyph-outline API to libstb | Proposed (step 3) |
| [0007](0007-refactor-in-verified-steps.md) | Refactor in verified steps, gated by tests and a pixel-level golden diff | Accepted |

The step numbers refer to [`REFACTOR_CHECKLIST.md`](../../REFACTOR_CHECKLIST.md) at the repository root.

## Adding a record

Copy the shape of an existing one: `# ADR-NNNN: Title`, then **Status**, **Date**, **Deciders**, and the
sections Context, Decision, Options Considered, Trade-off Analysis, Consequences, Action Items. Number records
in order and never reuse a number. A decision that is reversed is not edited away: set its status to
`Superseded by ADR-NNNN` and write the new record.

`bench/` holds the small programs that produced the numbers quoted in ADR-0003, so the claims can be re-run.
