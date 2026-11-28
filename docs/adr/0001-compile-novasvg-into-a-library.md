# ADR-0001: Compile novasvg into a library instead of shipping it header-only

**Status:** Accepted. Step 4 is in progress: the `render/` layer is compiled into `novasvg_core` (see `REFACTOR_CHECKLIST.md`)
**Date:** 2026-11-01
**Deciders:** Mohammad Raziei (maintainer)

## Context

novasvg 0.11.4 is a header-only C++17 library: two `INTERFACE` CMake targets (`novasvg::noflag`,
`novasvg::novasvg`) and a generated single-header "dist" form. At the baseline commit (`85ebfc7`) the headers
were about 43,600 lines. After the vendored stb code was removed (ADR-0005) they are 34,469 lines, and one
file, `svgelement.h`, still holds roughly 90 types in about 7,600 lines.

Forces at play:

- Every translation unit that includes `novasvg.h` parses and compiles all of it. Nothing is shared between
  consumers, and the library's own tests recompile everything per test file.
- The same concept is defined in several layers (public API, `detail/canvas.h`, `gfx/`), partly
  because a header-only library has no way to hide the internal layers from the public include path.
- novasvg is going to depend on libstb (ADR-0002), which is a compiled shared library. Consumers must link
  something regardless, so "one `#include` and nothing else" no longer holds.
- The maintainer's direction is that novasvg stops being header-only and takes a breaking release to do it.

## Decision

novasvg becomes a **compiled library with thin public headers**. Implementation moves out of the headers into
`src/`, one module at a time. The public headers keep only what a consumer must see (`document`, `element`,
`node`, `box`, `matrix` and their dependencies); `detail/` is no longer part of the public include path.

## Options Considered

### Option A: Stay header-only (and link libstb anyway)

| Dimension | Assessment |
|---|---|
| Consumer build time | Unchanged: every TU parses ~34k lines |
| Hiding internals | Not possible; `detail/` stays visible |
| Distribution | Still one include directory, but a link step for libstb is added |
| Effort | None |

**Pros:** no restructuring; dist single-header keeps working.
**Cons:** keeps the duplicated layers and the build cost; the headline benefit (no linking) is already gone.

### Option B: Compiled library, thin public headers (chosen)

| Dimension | Assessment |
|---|---|
| Consumer build time | Large reduction: consumers parse only the thin headers |
| Hiding internals | Yes: implementation and `detail/` live in `src/` |
| Distribution | Library plus headers; needs `find_package` / a link line |
| Effort | High; the largest step in the refactor |

**Pros:** faster builds, a real ABI boundary, room to delete the duplicated types, one place for visibility.
**Cons:** breaking for every consumer; wheels must ship the library; the dist single header goes away or changes meaning.

### Option C: Hybrid, a single header with a `NOVASVG_IMPLEMENTATION` macro (stb style)

| Dimension | Assessment |
|---|---|
| Consumer build time | Good in all but one TU |
| Hiding internals | Partly |
| Distribution | Keeps a single-file form |
| Effort | Medium, plus a permanent second mode to maintain |

**Pros:** keeps the one-file story.
**Cons:** two ways to build, so double the test matrix, and it still needs libstb linked.

## Trade-off Analysis

Option A's only real advantage was avoiding a link step, and ADR-0002 removes that advantage. Option C keeps a
second configuration alive for a benefit (one file) that the libstb dependency also undermines. Option B costs
the most to do but is the only one that fixes the build cost and the layering, so it is chosen.

## Consequences

- **Easier:** shorter consumer builds; a defined public/internal boundary; deleting the duplicated types
  (ADR-0003 and step 5); one place to set symbol visibility.
- **Harder:** every consumer must link and, with a shared libstb, ship two libraries; Python wheels must bundle
  libstb (step 7); the dist single-header generator and `NOVASVG_TEST_INCLUDE_DIR` (which lets tests compile
  against it) need to be retired or redefined.
- **Revisit:** whether novasvg itself is built static, shared, or both; the README's "header-only" claim must be
  rewritten when this lands.

## Action Items

1. [ ] Define the public/internal boundary: thin headers for `document`, `element`, `node`, `box`, `matrix`.
2. [ ] Move `detail/` into `src/` one module per change, tests and golden diff green after each (ADR-0007).
3. [ ] Split `svgelement.h` by area: DOM, properties, filters, gradients, text, foreignObject.
4. [ ] Make the CSS/HTML engine (`Rule`, `SimpleSelector`, `RuleIndex`, `HtmlNode`, `StyleNode`) its own module.
5. [ ] Add `NOVASVG_API` and symbol visibility; decide what `novasvg::noflag` means afterwards.
6. [ ] Measure build time before and after, and record it in the checklist.
7. [ ] Decide the future of the dist single header and `NOVASVG_TEST_INCLUDE_DIR`.
