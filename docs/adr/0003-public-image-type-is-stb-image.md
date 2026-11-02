# ADR-0003: The public pixel type is `stb::image`; the premultiplied render surface is internal

**Status:** Accepted. Implemented in step 2b on the C++ side (`renderToImage`, `render(stb::image&)`); the Python `Bitmap` class still sits over the internal type (open, see the checklist)
**Date:** 2026-11-01
**Deciders:** Mohammad Raziei (maintainer). The maintainer asked for no duplicated pixel structures and for the
choice to be made on performance, not effort; the analysis below was done on that basis.

## Context

Today novasvg has its own public pixel class, `Bitmap`. It stores **premultiplied ARGB32** and does three jobs:
it is the surface the rasterizer draws into, the texture source for `<image>` and pattern fills
(`canvas_set_texture` reads `bitmap.surface()` directly), and the image users get back from `render()`.

libstb's `stb::image` stores **straight-alpha RGBA** (8-bit, tightly packed, move-only) and has
`image::wrap(...)` to adopt an existing buffer without copying. With libstb as a dependency (ADR-0002),
`Bitmap` is a second image type that duplicates `stb::image` in the public API.

The two formats are the same picture written two ways. Semi-transparent red at 50 %:
straight `(255, 0, 0, a=128)`, premultiplied `(128, 0, 0, a=128)`. Premultiplied is what rasterizers use
because one formula (`out = src + dst * (1 - a_src)`) serves all four channels, and because averaging pixels is
only correct in it: averaging opaque red with a transparent pixel whose stored colour is junk gives
`(128, 128, 0, 128)` in straight form (a green fringe) but the correct `(128, 0, 0, 128)` premultiplied.
Averaging happens in bilinear sampling, blur, resize and gradients.

So any design has exactly one question: where does the conversion between the two representations happen?

## Decision

- `render()` and the other public entry points return **`stb::image`**. `Bitmap` leaves the public API and
  becomes an internal (`detail`) type; novasvg has **no pixel type of its own** in its public interface.
- The rasterizer keeps drawing into a **private premultiplied surface**. At the boundary the surface is
  converted **in place** to straight RGBA and handed over with `stb::image::wrap` (the surface's memory is the
  image's `keep_alive` owner), so the hand-off is one pass and no copy.
- Decoding goes the other way: libstb decodes to straight RGBA and one pass premultiplies it into a surface.
- Rendering *onto an existing* `stb::image` renders into a transparent layer and composites it with
  `stb::image::blend`, instead of drawing straight into straight-alpha pixels.

## Options Considered

### Option A: Keep `Bitmap` public, convert to `stb::image` on request

| Dimension | Assessment |
|---|---|
| Duplicate types | Two public image types forever |
| Performance | Same as C, plus a copy if conversion allocates |
| Churn | Lowest |

**Pros:** no breaking change at this boundary. **Cons:** the exact duplication the maintainer wants gone.

### Option B: Draw directly into `stb::image` (straight alpha everywhere)

| Dimension | Assessment |
|---|---|
| Duplicate types | None |
| Performance | See measurements: composite is on par only with hand-written SIMD |
| Churn | Highest: composite, gradients, sampling, filters all rewritten; output pixels change |

**Pros:** removes the final conversion pass. **Cons:** see below.

### Option C: `stb::image` public, premultiplied surface private (chosen)

| Dimension | Assessment |
|---|---|
| Duplicate types | None in the public API |
| Performance | One in-place pass at the boundary, no copy |
| Churn | Moderate: the public API and its users change; the engine does not |

## Trade-off Analysis

Measured on 4096 x 4096 (16.7 megapixels), GCC 13.3, one core, AVX2 machine, two runs; sources in
[`bench/`](bench/).

**The conversion pass (premultiplied to straight, in place, scalar):** 1.3 to 5.6 ns/pixel, i.e. 21 to 94 ms.
A full 4096 x 4096 render plus PNG encode of two of the larger SVGs in `data/` took 1.39 s and 1.63 s, so the
pass is roughly 1 to 7 percent of that. It is cheapest when most pixels are opaque or fully transparent and
dearest when all are translucent, because the unpremultiply divides per channel.

**Source-over compositing, translucent over translucent, in ns/pixel (each figure includes about 0.33 for one
64 MB memcpy that both sides pay):**

| Kernel | ns/pixel |
|---|---|
| novasvg, premultiplied, buffer over buffer (scalar C) | 2.35 to 2.37 |
| novasvg, premultiplied, solid-colour fill (scalar C) | 1.14 to 1.20 |
| libstb `image::blend`, straight, AVX2 | 2.39 to 2.59 |
| libstb `image::blend`, straight, SSE2 | 3.37 to 3.52 |
| libstb `image::blend`, straight, scalar | 7.41 to 8.00 |

Reading the numbers: with hand-written AVX2, straight alpha is within noise of novasvg's premultiplied
buffer-over-buffer kernel, so SIMD does close most of the gap. It does not make straight faster. Why B still
loses:

1. **No gain.** At best it saves the 1 to 7 percent conversion pass and composite is at parity or worse; the
   solid-fill case (the common one) was only measured on the premultiplied side, 1.14 to 1.20 ns/pixel, a
   single multiply-add per channel.
2. **Sampling, blur and resize must be premultiplied anyway**, so a straight engine would convert back and
   forth inside those operations, adding conversions rather than removing one.
3. **Without SIMD, straight is up to about 3.5 times slower** (scalar 7.4 to 8.0 against 2.4); premultiplied is
   fast with no SIMD at all, which matters on targets where libstb's dispatch is not available.
4. **The whole engine is premultiplied** (the rasterizer is a plutovg port). Rewriting it for B changes output
   pixels, which breaks the golden gate (ADR-0007), for no measured benefit.

Option A is rejected because it keeps the duplicate type. Option C keeps the fast representation where it
matters and puts one in-place pass on the boundary.

*Superseded evidence.* An early benchmark in this analysis reported straight alpha as 2 to 4 times slower. That
compared an auto-vectorised premultiplied loop with a straight-alpha loop the compiler did not vectorise; it
was an unfair test, was withdrawn, and the numbers above replace it. The decision survives the correction, but
its justification is now points 1 to 4, not raw composite speed.

## Consequences

- **Easier:** one image type across novasvg and libstb; no copy on hand-off; `render()` results go straight to
  `stb::image` encoders, `blend`, `resize`.
- **Harder:** the public API and every user of `Bitmap` change (CLI, tests, examples, the Python numpy
  interop). Rendering onto an existing image changes from "draw on it" to "render a layer and blend", which
  must be checked for blend modes that read the backdrop (`mix-blend-mode`).
- **Cost to consumers that want premultiplied** (for example GPU texture upload, or a UI toolkit's
  premultiplied image): they pay an unpremultiply they will undo. An optional premultiplied output can be added
  later if such a consumer exists.
- **Found on the way (reproduced):** the baseline's writer converted a `Bitmap` whose stride exceeds
  `width * 4` into a scratch buffer using the bitmap's stride as the destination row pitch. With a 5 x 3 bitmap
  over caller memory (stride 32), AddressSanitizer reports a `heap-buffer-overflow` write in
  `BitmapRgbaScratch` on baseline `85ebfc7`: a 60-byte buffer written 4 bytes past its end. The replacement
  converts row by row and runs clean under ASan and UBSan on the same test (`tests/cpp/test_image_io.cpp`,
  ADR-0005). Step 2b must keep row-by-row conversion.
- **Follow-up (does not change this decision):** a libstb issue proposes SIMD kernels for premultiplied to
  straight conversion and back, bit-exact with the scalar reference. When it lands, the two call sites (final
  hand-off and `<image>` decode) swap to it. It shrinks the 1 to 7 percent further and so strengthens the
  decision.
- **Revisit if:** a measurement of the engine's own hot paths shows straight-alpha composite clearly faster, or
  a feature appears that needs straight alpha inside the engine.

## Action Items

1. [ ] `render()` returns `stb::image` through in-place conversion and `image::wrap` with `keep_alive`.
2. [ ] Render-onto-existing-image through a transparent layer and `image::blend`; test `mix-blend-mode`.
3. [ ] Move `Bitmap` to `detail`, drop its `writeTo*` (the `stb::image` encoders take over).
4. [ ] Update the CLI, tests, examples and Python bindings (numpy interop).
5. [ ] Keep the stride-safe, row-by-row conversion (guarded by `test_image_io.cpp`).
6. [ ] Swap to libstb's conversion kernels when the issue is resolved.
7. [ ] Reproduce the numbers with `bench/` if the engine or libstb's blend code changes materially.
