# Vendored third-party code

Everything in this folder is embedded, unmodified algorithmic code from
other open-source projects. It backs `novasvg`'s own rendering layer
(`include/novasvg/detail/render/*.h`) but is not `novasvg`'s own code,
so it is kept separate on purpose and its internal naming (`stbi_`,
`stbtt_`, `FT_`, `PVG_FT_`, ...) is left as-is rather than rebranded.

| File(s) | Project | License | Notes |
|---|---|---|---|
| `stb_image.h` | [stb_image](https://github.com/nothings/stb) by Sean Barrett | public domain / MIT | Image decoding (PNG, JPEG, ...). Compiled with `STB_IMAGE_STATIC`, so it's already safe to include from multiple translation units. |
| `stb_truetype.h` | [stb_truetype](https://github.com/nothings/stb) by Sean Barrett | public domain / MIT | TrueType font parsing/rasterization. Compiled with `STBTT_STATIC`. |
| `ft_raster.h`, `ft_stroker.h`, `ft_math.h`, `ft_types.h` | [FreeType](https://freetype.org) rasterizer/stroker (`ftimage.h`, `ftstroke.h`, `fttrigon.h`, `fttypes.h`), adapted for this project as `PVG_FT_*` | FreeType License (FTL) | Path rasterization and stroking, fixed-point math. Wrapped in an anonymous C++ namespace (2-line addition per file, nothing else changed) so they're safe to include from multiple translation units without a separate "define the implementation in exactly one .cpp" step. |

None of the identifiers, algorithms, or structure inside these files
were changed -- only file names (dropped the old `plutovg-` filename
prefix) and `#include` paths were updated to match this project's
layout, plus the namespace wrapping noted above for the FreeType files.

`stb_image_write.h` ([stb_image_write](https://github.com/nothings/stb)
by Sean Barrett, public domain / MIT) is *not* in this folder: it's
inlined directly in `include/novasvg/novasvg.h`, next to `Bitmap` --
the only thing in novasvg that writes images -- instead of living here
as a separate vendored file. Unmodified otherwise, same as the rest of
this table.
