# ADR-0004: stl-like `snake_case` naming; namespace `novasvg`

**Status:** Accepted. Implementation is step 6 and has not started; the alias `nova` is still open.
**Date:** 2026-11-01
**Deciders:** Mohammad Raziei (maintainer)

## Context

The public API of 0.11.4 is Qt/DOM-flavoured: `PascalCase` types and `camelCase` methods with `get`/`set`/`is`/`to`
prefixes (for example `loadFromFile`, `getElementById`, `isNull`). The internal `detail/render/` layer, a plutovg
port, is already `snake_case`, and libstb, which novasvg now depends on and shares types with, is entirely
`snake_case` in the style of the standard library. The result is three conventions in one codebase.

The refactor already breaks the API (ADR-0001, ADR-0003), so renaming now costs no extra compatibility.

The namespace was also undecided between `novasvg` and `nova`. Today `novasvg::` appears about 300 times in
`include/`, `src/`, `tests/` and `examples/`, and `nova::` appears nowhere.

## Decision

1. Adopt the **same naming rules as libstb**, so the two libraries read alike:

   | Thing | Rule | Example |
   |---|---|---|
   | Types (class, struct, enum) | `snake_case` | `document`, `text_node`, `blend_mode` |
   | Functions and methods | `snake_case` | `load_file`, `has_attribute` |
   | Getters | no `get_` prefix | `width()`, `bounding_box()` |
   | Setters | `set_` prefix | `set_attribute(...)` |
   | Booleans | `is_` / `has_` | `is_null()`, `has_attribute()` |
   | Factories and conversions | `from_*` / `to_*` | `matrix::from_rotation(...)`, `to_string()` |
   | Private data members | trailing `_` | `node_` |
   | Enums | `enum class`, `snake_case` values | `text_align::center` |
   | Exceptions | `*_error`, derived from `std::` types | `invalid_color_error` |
   | Macros | `NOVASVG_*` | `NOVASVG_API` |
   | Namespaces | lowercase | `novasvg`, `novasvg::detail` |

   The examples show direction only. The exact old-to-new name table is written and approved in step 6.
2. The namespace is **`novasvg`**. An optional alias `namespace nova = novasvg;` may be offered for brevity.
3. The Python bindings and the CLI keep their own conventions; they are a separate surface and may stay
   Pythonic.

## Options Considered

### Option A: Keep the current naming

| Dimension | Assessment |
|---|---|
| Churn | None |
| Consistency | Three styles in one codebase, and unlike libstb |

### Option B: stl-like `snake_case`, matching libstb (chosen)

| Dimension | Assessment |
|---|---|
| Churn | High but mechanical (scripted rename), one module per change |
| Consistency | One style across novasvg and libstb |

### Option C: Keep camelCase for the public API, `snake_case` internally

| Dimension | Assessment |
|---|---|
| Churn | Low |
| Consistency | Two styles, with a visible seam at the public boundary |

### Namespace: `novasvg` or `nova`

| Option | Assessment |
|---|---|
| `novasvg` | Already used everywhere; matches the project, the target names and `stb::` (libstb's own full name) |
| `nova` | Shorter, but generic enough to collide with other projects, and unused today |

## Trade-off Analysis

A rename is cheap only while the API is already breaking, and that window is now. Matching libstb removes the
mixed styles and means types that cross the two libraries (`stb::image`, `stb::color`) read naturally next to
novasvg's own. For the namespace, `novasvg` is the status quo and the safe choice; a short alias gives the
brevity of `nova` without the collision risk.

## Consequences

- **Easier:** one convention to learn; no mental switch when moving between novasvg and libstb.
- **Harder:** every consumer's code changes; documentation, examples, tests and the CLI must be updated; the
  rename diff is large.
- **Needs discipline:** the rules must be written down and applied uniformly, or the refactor ends with a fourth
  style.
- **Revisit:** whether to ship the `nova` alias.

## Action Items

1. [ ] Write the old-to-new name table for the public API and get it approved (step 6).
2. [ ] Rename by script, one module per change, with tests and the golden diff green each time (ADR-0007).
3. [ ] Update the CLI, tests, examples and docs; keep the Python names a separate decision.
4. [ ] Decide whether to ship the `nova` alias.
