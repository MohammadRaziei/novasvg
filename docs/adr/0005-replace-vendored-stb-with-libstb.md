# ADR-0005: Replace the vendored stb_image, image writer and inflater with libstb

**Status:** Accepted and implemented (step 2a)
**Date:** 2026-11-01
**Deciders:** Mohammad Raziei (maintainer)

## Context

novasvg carried private copies of code that libstb now provides (ADR-0002):

- stb_image 2.30, about 7,700 lines inlined in `detail/render/surface.h`;
- `BitmapCodec`, a port of stb_image_write (PNG, BMP, TGA, JPEG), about 1,400 lines in `detail/bitmap.h`;
- the zlib inflater (`stbi_zlib_decode_malloc`) used for compressed embedded fonts.

Despite the size, novasvg's own use of that code was small: `stbi_load`, `stbi_load_from_memory`,
`stbi_image_free`, `stbi_zlib_decode_malloc`, and the four writers behind `Bitmap::writeTo*`.

## Decision

Delete the vendored code and call libstb:

| Was | Now |
|---|---|
| `stbi_load`, `stbi_load_from_memory` | `stb::image::open` / `decode` with `load_options{format = rgba}` |
| `stbi_zlib_decode_malloc` | `stb::zlib_decompress` |
| `BitmapCodec` writers | `stb::image::write_png/bmp/tga/jpg` and `to_png/bmp/tga/jpg` |

Details that keep behaviour unchanged where it matters:

- `load_options::orient` stays at its default (off), so EXIF orientation is **not** applied, as with stb_image.
- libstb reports failure by exception. At the edges these become what callers already saw: a null surface for
  decode, an empty `FontFace` for a bad compressed font, `false` from `Bitmap::writeTo*`.
- `Bitmap` still stores premultiplied ARGB32 (ADR-0003). Encoders get straight RGBA from one conversion into a
  buffer that the `stb::image` owns (`wrap` with `keep_alive`); the conversion runs row by row, so a bitmap whose
  stride is wider than `width * 4` is handled.
- JPEG quality is still clamped to 1..100, which libstb itself would reject.
- The write callback is now called with the whole encoded buffer (split only to fit its `int` size), not in many
  small pieces as stb_image_write did.

## Options Considered

| Option | Assessment |
|---|---|
| Keep the vendored copies | Zero risk now; keeps about 9,150 lines, duplicate code to maintain, and no path to libstb's SIMD and fixes |
| **Replace with libstb (chosen)** | Removes the duplication; adds a runtime dependency (ADR-0002) |
| Wrap the vendored code behind the new API first, replace later | Extra step that is deleted again immediately |

## Trade-off Analysis

The surface area used was four functions plus the writers, so the replacement is small and the old code has no
remaining caller. The dependency it adds was already decided in ADR-0002. Doing it before moving code into
`src/` (ADR-0001) also removes about a fifth of the header text (9,152 of roughly 43,600 lines) first.

## Consequences

Measured when the step was done (baseline `85ebfc7` against the working tree):

- **Code removed:** 9,152 vendored lines (7,726 in `surface.h`, 1,426 for `BitmapCodec`); the change is 182
  insertions and 9,246 deletions over 7 files. Headers went from about 43,600 to 34,469 lines.
- **Behaviour held:** `test_novasvg_cpp` 64 cases / 377 assertions and `test_novasvg_embedded_font` 5 / 71 pass,
  as at baseline; the 62 golden renders are pixel-identical (tolerance 0); no `stbi_*` symbols remain in the
  binaries; `ldd novasvg` now lists libstb.
- **Binaries:** the CLI went from 1.4 to 1.3 MB and the main test binary from 2.4 to 1.8 MB.
- **Bug fixed (reproduced):** the old writer overran a heap buffer for bitmaps with a stride wider than
  `width * 4`. AddressSanitizer flags it on baseline `85ebfc7`; the new code is clean under ASan and UBSan.
- **New tests:** `tests/cpp/test_image_io.cpp` (6 cases, 66 assertions) covers PNG and TGA losslessly with
  straight alpha, BMP and JPEG size and colour, file against callback output, the padded stride, failure paths
  and JPEG quality clamping.
- **Differences to know about:**
  - The bytes of encoded PNG/BMP/TGA/JPEG differ from before (different encoder); the pixels do not. The
    golden diff decodes PNGs, so it is insensitive to this by design.
  - Softimage PIC, which stb_image could read, is gone: libstb's sources contain no PIC decoder.
  - libstb refuses to decode more than 512 MiB by default (`load_options::max_bytes`, which can be raised).
- **Coverage honesty:** the golden set exercises PNG output and decoding of the raster images embedded in three
  of the SVGs; BMP, TGA and JPEG output rely on the new unit tests only, which check size and colour, not
  byte-for-byte output.
- **Still to do:** README, docs and licence notices still name stb_image; they change with step 7.

## Action Items

1. [x] Replace decode, inflate and the four writers; delete the vendored code.
2. [x] Link the test targets to libstb.
3. [x] Add `test_image_io.cpp`; reproduce the stride overflow on baseline with ASan.
4. [ ] Extend the golden set with SVGs that embed JPEG and GIF images.
5. [ ] Update README, docs and licence notices (step 7).
