#include <nanobind/nanobind.h>
#include <nanobind/stl/optional.h>
#include <optional>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/ndarray.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <stdexcept>
#include <string>
#include <vector>

// Include the main library header
#include "novasvg/novasvg.h"
#include "gfx/canvas.h" // Transform, Path, ...
#include "gfx/font.h"
#include "svg/svgelement.h" // private: foreignObject text measurement
#include "bitmap_interop.hpp"
#include <filesystem>

namespace nb = nanobind;
using namespace nb::literals;


namespace {

// Python's Bitmap stays the premultiplied-ARGB32 render target. Encoding goes through the public pixel type:
// bitmap_to_rgba_image() makes a straight-alpha stb::image copy (the Bitmap itself is left alone), and libstb's
// encoders write it. One path for every format, so the bytes and the file variants cannot drift apart.
enum class image_format { png, bmp, tga, jpg };

const char* format_name(image_format format)
{
    switch(format) {
    case image_format::png: return "PNG";
    case image_format::bmp: return "BMP";
    case image_format::tga: return "TGA";
    case image_format::jpg: return "JPEG";
    }
    return "?";
}

// The old writers clamped the JPEG quality instead of rejecting it; keep that.
int clamp_quality(int quality)
{
    return quality < 1 ? 1 : (quality > 100 ? 100 : quality);
}

// A null bitmap is the caller's mistake (ValueError); an encoder that fails or produces nothing is a
// RuntimeError, never a silent empty `bytes` (an empty result would look like a valid, empty image).
nb::bytes encode_to_bytes(const nova::vg::Bitmap& bitmap, image_format format, int quality = 80)
{
    if(bitmap.is_null())
        throw std::invalid_argument(std::string("cannot encode a null Bitmap as ") + format_name(format));
    const stb::image image = nova::vg::bitmap_to_rgba_image(bitmap);
    std::vector<std::uint8_t> bytes;
    try {
        switch(format) {
        case image_format::png: bytes = image.to_png(); break;
        case image_format::bmp: bytes = image.to_bmp(); break;
        case image_format::tga: bytes = image.to_tga(); break;
        case image_format::jpg: bytes = image.to_jpg(clamp_quality(quality)); break;
        }
    } catch(const std::exception&) {
        bytes.clear();
    }
    if(bytes.empty())
        throw std::runtime_error(std::string("failed to encode Bitmap as ") + format_name(format));
    return nb::bytes(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

// File variants report success as a bool, like they always did.
bool write_to_file(const nova::vg::Bitmap& bitmap, const std::string& filename, image_format format, int quality = 80)
{
    if(bitmap.is_null())
        return false;
    try {
        const stb::image image = nova::vg::bitmap_to_rgba_image(bitmap);
        switch(format) {
        case image_format::png: image.write_png(filename); break;
        case image_format::bmp: image.write_bmp(filename); break;
        case image_format::tga: image.write_tga(filename); break;
        case image_format::jpg: image.write_jpg(filename, clamp_quality(quality)); break;
        }
        return true;
    } catch(const std::exception&) {
        return false;
    }
}

// png/bmp/tga/jpg/jpeg, optionally with a leading '.', any case; nullopt if unknown.
bool parse_format(std::string text, image_format& out);

std::string lowercase(std::string text)
{
    for(auto& c : text)
        c = char(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

bool parse_format(std::string text, image_format& out)
{
    text = lowercase(std::move(text));
    if(!text.empty() && text[0] == '.')
        text.erase(0, 1);
    if(text == "png") out = image_format::png;
    else if(text == "bmp") out = image_format::bmp;
    else if(text == "tga") out = image_format::tga;
    else if(text == "jpg" || text == "jpeg") out = image_format::jpg;
    else return false;
    return true;
}

} // namespace

NB_MODULE(novasvg_py, m) {
    m.doc() = "Python bindings for NovaSVG using nanobind";

    // Color parse errors: novasvg.InvalidColorError, a ValueError subclass.
    nb::exception<nova::vg::InvalidColorError>(m, "InvalidColorError", PyExc_ValueError);

    // --- Bind Static Utility Functions ---
    m.def("version_string", &nova::svg::version_string, "Get the library version as a string (e.g., '0.1.0').");
    m.def("add_font_face_from_file", &nova::vg::addFontFaceFromFile, 
          "family"_a, "bold"_a, "italic"_a, "filename"_a,
          "Add a font face from a file to the cache.");
    
    // Binding for adding a font from memory. The C++ API takes a raw
    // (destroy_func, closure) pair so it can avoid copying the caller's
    // buffer -- but exposing that pair as Python-visible parameters is
    // both wrong and (on MSVC specifically) doesn't even compile: wrong
    // because with destroy_func=nullptr novasvg never copies the data,
    // it just keeps the original pointer alive only as long as the
    // caller does -- here, only as long as Python's nb::bytes object
    // isn't garbage-collected, which nothing guarantees once the font is
    // cached indefinitely in FontFaceCache; a raw C function pointer
    // isn't something a Python caller could usefully pass in anyway.
    // Fixed by copying the buffer once here and owning that copy with a
    // capture-less lambda (implicitly convertible to the destroy_func_t
    // function-pointer type novasvg wants, portably across compilers --
    // unlike a raw function pointer arriving through nanobind's
    // Python-argument machinery, which is what didn't compile on MSVC).
    m.def("add_font_face_from_data",
          [](const char* family, bool bold, bool italic, nb::bytes data) -> bool {
               auto* copy = std::malloc(data.size());
               if(copy == nullptr)
                   return false;
               std::memcpy(copy, data.data(), data.size());
               return nova::vg::addFontFaceFromData(family, bold, italic, copy, data.size(),
                                                     [](void* p) { std::free(p); }, copy);
          },
          "family"_a, "bold"_a, "italic"_a, "data"_a,
          "Add a font face from a memory buffer (bytes) to the cache.");

    // --- Bind FontFace / Font (text measurement) ---
    // Exposes novasvg's own font stack (built on libstb's stb::font,
    // src/gfx/font_face.cpp) to Python, so callers that need
    // to measure text -- e.g. mermaidx computing label/box sizes before it
    // ever builds the svg -- can ask the exact same engine that will later
    // paint the glyphs, instead of hand-rolling a second TTF parser that
    // has to be kept in sync with it by hand.
    nb::class_<nova::vg::FontFace>(m, "FontFace")
        .def(nb::init<>(), "Construct a null font face.")
        .def(nb::init<const char*>(), "filename"_a, "Load a font face directly from a font file (no cache involved).")
        .def("__init__", [](nova::vg::FontFace* self, nb::bytes data) {
            auto* copy = std::malloc(data.size());
            if(copy == nullptr) {
                new (self) nova::vg::FontFace();
                return;
            }
            std::memcpy(copy, data.data(), data.size());
            new (self) nova::vg::FontFace(copy, data.size(), [](void* p) { std::free(p); }, copy);
        }, "data"_a, "Load a font face directly from a memory buffer (bytes), no cache involved.")
        .def("is_null", &nova::vg::FontFace::is_null, "Check if the font face is null/invalid.")
        .def("__bool__", [](const nova::vg::FontFace& face) { return !face.is_null(); })
        .def_prop_ro("units_per_em", &nova::vg::FontFace::unitsPerEm,
             "Raw font design-unit space this face's own tables are defined in "
             "(e.g. head.unitsPerEm). Pair with advance_width_units()/ascent_units()/"
             "descent_units() to build a complete, size-independent advance table -- "
             "e.g. to hand text-measurement data to a JS engine that can't call back "
             "into Python per string (mermaidx's v8_engine does exactly this).")
        .def_prop_ro("ascent_units", &nova::vg::FontFace::ascentUnits, "Ascent in raw font design units (unscaled).")
        .def_prop_ro("descent_units", &nova::vg::FontFace::descentUnits, "Descent in raw font design units (unscaled).")
        .def("advance_width_units", [](const nova::vg::FontFace& face, uint32_t codepoint) {
            return face.advanceWidthUnits(static_cast<char32_t>(codepoint));
        }, "codepoint"_a, "Advance width of a single Unicode codepoint, in raw font design units (unscaled).")
        .def("kern_advance_units", [](const nova::vg::FontFace& face, uint32_t first, uint32_t second) {
            return face.kernAdvanceUnits(static_cast<char32_t>(first), static_cast<char32_t>(second));
        }, "first"_a, "second"_a,
           "Kerning adjustment between two consecutive codepoints, in raw font design units "
           "(unscaled) -- see FontFace::kernAdvanceUnits() in font.h for why this is a per-pair "
           "query rather than a bulk table like codepoints().")
        .def_prop_ro("notdef_advance_width_units", &nova::vg::FontFace::notdefAdvanceWidthUnits,
             "The advance width used for any codepoint outside this font's cmap "
             "(glyph id 0, the \".notdef\" glyph) -- what advance_width_units() itself "
             "already falls back to for such a codepoint.")
        .def("codepoints", [](const nova::vg::FontFace& face) {
            auto cps = face.codepoints();
            std::vector<uint32_t> out;
            out.reserve(cps.size());
            for(auto cp : cps)
                out.push_back(static_cast<uint32_t>(cp));
            return out;
        }, "Every Unicode codepoint this face's cmap maps to a glyph (see FontFace::codepoints() in font.h "
           "for exactly which cmap formats are covered -- virtually every real-world font).");

    m.def("get_font_face", [](const std::string& family, bool bold, bool italic) {
        return nova::vg::fontFaceCache()->getFontFace(family, bold, italic);
    }, "family"_a, "bold"_a = false, "italic"_a = false,
       "Look up a font face previously registered via add_font_face_from_file/_data "
       "(falling back to novasvg's built-in generic-family table, e.g. \"sans-serif\").");

    m.def("get_font_face_for_family_stack", [](const std::string& family_stack, bool bold, bool italic) {
        return nova::vg::fontFaceCache()->getFontFaceForFamilyStack(family_stack, bold, italic);
    }, "family_stack"_a, "bold"_a = false, "italic"_a = false,
       "Resolve a CSS-style comma-separated font-family stack (e.g. "
       "'\"trebuchet ms\", verdana, arial, sans-serif') name by name: a face registered or "
       "installed under that exact name, then a metric-compatible substitute from novasvg's "
       "built-in table (case-insensitive, e.g. 'arial' -> Liberation Sans), then generic "
       "keywords. Names nothing matches are skipped; returns a null face if none resolve.");

    nb::class_<nova::vg::Font>(m, "Font")
        .def(nb::init<>(), "Construct a null font.")
        .def(nb::init<const nova::vg::FontFace&, float>(), "face"_a, "size"_a,
             "Construct a font from a face at a given pixel size.")
        .def_prop_ro("ascent", &nova::vg::Font::ascent, "Ascent in pixels, at this font's size.")
        .def_prop_ro("descent", &nova::vg::Font::descent, "Descent in pixels (typically negative), at this font's size.")
        .def_prop_ro("height", &nova::vg::Font::height, "ascent - descent.")
        .def_prop_ro("line_gap", &nova::vg::Font::lineGap, "Recommended extra spacing between lines, in pixels.")
        .def_prop_ro("x_height", &nova::vg::Font::xHeight, "Height of a lowercase 'x' glyph, in pixels.")
        .def_prop_ro("size", &nova::vg::Font::size, "The pixel size this font was constructed with.")
        .def_prop_ro("face", &nova::vg::Font::face, nb::rv_policy::reference_internal, "The underlying FontFace.")
        .def("is_null", &nova::vg::Font::is_null, "Check if the font is null/invalid.")
        .def("__bool__", [](const nova::vg::Font& font) { return !font.is_null(); })
        .def("measure_text", [](const nova::vg::Font& font, const std::string& text) {
            // Python str arrives as UTF-8 (nanobind's std::string conversion);
            // Font::measureText wants UTF-32, same as novasvg's own svg text
            // layout path (see svg_text_fragments_builder), so route through the
            // identical utf8ToU32() conversion for byte-for-byte consistency
            // with what novasvg will actually paint.
            auto u32text = nova::vg::utf8ToU32(text);
            return font.measureText(u32text);
        }, "text"_a, "Measure the advance width of a UTF-8 string, in pixels, at this font's size.");

    // nb::class_<Font> chain above ends with measure_text; attach glyph_boxes to it.
    m.attr("Font").attr("glyph_boxes") = nb::cpp_function(
        [](const nova::vg::Font& font, const std::string& text) {
            nb::list out;
            for(const auto& box : font.glyphBoxes(nova::vg::utf8ToU32(text)))
                out.append(nb::make_tuple(box.pen, box.inkLeft, box.inkRight, box.inkTop, box.inkBottom));
            return out;
        }, nb::is_method(), "text"_a,
        "Per-glyph (pen_x, ink_left, ink_right, ink_top, ink_bottom) in pixels (top/bottom relative to the baseline, y down) for every glyph that has an outline "
        "(spaces are skipped), using the same ligature + kerning walk as measure_text().");

    m.def("measure_foreign_object", [](const std::string& html, const nova::vg::Font& font, std::optional<float> max_width,
                                       std::optional<std::string> family, bool bold, bool italic) {
        nova::svg::ForeignObjectMetrics metrics;
        if(!family) {
            metrics = nova::svg::measureForeignObjectContent(html, font, max_width.value_or(-1.f));
        } else {
            // The same faces ForeignObjectSimple::render() paints with (svg_foreign_object_element::fontsFor()):
            // `font` is the base style, every other bold/italic variant comes from the family.
            const unsigned char base = (unsigned char)((bold ? nova::svg::ForeignObjectFonts::Bold : 0)
                                                       | (italic ? nova::svg::ForeignObjectFonts::Italic : 0));
            nova::svg::ForeignObjectFonts fonts;
            for(unsigned style = 0; style < 4; ++style) {
                if(style == base) {
                    fonts.face[style] = font;
                    continue;
                }
                auto variant = nova::svg::makeFont(*family, font.size(), (style & nova::svg::ForeignObjectFonts::Bold) != 0,
                                                 (style & nova::svg::ForeignObjectFonts::Italic) != 0);
                fonts.face[style] = variant.is_null() ? font : variant;
            }
            metrics = nova::svg::measureForeignObjectContent(html, fonts, max_width.value_or(-1.f), base);
        }
        nb::dict result;
        result["height"] = metrics.height;
        result["width"] = metrics.width;
        result["line_count"] = metrics.lineCount;
        result["line_height"] = metrics.lineHeight;
        return result;
    }, "html"_a, "font"_a, "max_width"_a = nb::none(), "family"_a = nb::none(), "bold"_a = false, "italic"_a = false,
       "Height/width (and line count/line-height) novasvg will use when painting <foreignObject> "
       "content, computed WITHOUT rendering -- the exact same logic ForeignObjectSimple::render() "
       "itself uses (see measureForeignObjectContent() in svgelement.h), so a caller building the "
       "SVG's own layout (e.g. mermaidx's headless JS DOM shim sizing a node box for an HTML label) "
       "can ask novasvg what size it will need instead of guessing separately. "
       "Text soft-wraps only if the HTML's own CSS `white-space` allows it (not `nowrap`/`pre`), "
       "at `max_width` (px) if given, else at the px `width`/`max-width` the HTML itself declares "
       "-- the same rule render() applies at the foreignObject's own width, so layout and paint "
       "agree. Bold and italic runs (<b>, <strong>, <i>, <em>, inline font-weight / font-style) are "
       "measured with the faces they are painted with when `family` is given: `font` is then the face of "
       "the base style (`bold` / `italic` say which it is) and the other variants are looked up in `family` "
       "exactly as SVG <text> looks them up. Without `family` every run is measured with `font` alone, as "
       "if it had no bold or italic in it. "
       "Returns {'height': float, 'width': float, 'line_count': int, 'line_height': float}; "
       "'width' is the widest resulting line.");

    // --- Bind Color Class ---
    nb::class_<nova::vg::Color>(m, "Color")
        .def(nb::init<uint8_t, uint8_t, uint8_t, uint8_t>(),
             "r"_a = 0, "g"_a = 0, "b"_a = 0, "a"_a = 255,
             "Construct a color from red, green, blue, alpha components (0-255 each).")
        .def(nb::init<const std::string&>(), "value"_a,
             "Construct a color from a CSS color name or hexadecimal string.")
        .def_static("from_value", &nova::vg::Color::fromValue, "value"_a,
                    "Construct a color from a packed 0xRRGGBBAA integer.")
        .def_static("from_hash", &nova::vg::Color::fromHash, "value"_a)
        .def_static("from_name", &nova::vg::Color::fromName, "value"_a)
        .def("value", &nova::vg::Color::value,
             "Get the color packed as a 0xRRGGBBAA integer.")
        .def("to_int", &nova::vg::Color::value)
        .def_static("rgba", [](uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
            return nova::vg::Color(r, g, b, a);
        }, "r"_a, "g"_a, "b"_a, "a"_a = 255)
        .def_static("black", [] { return nova::vg::Color::fromName("black"); })
        .def_static("white", [] { return nova::vg::Color::fromName("white"); })
        .def_static("red", [] { return nova::vg::Color::fromName("red"); })
        .def_static("green", [] { return nova::vg::Color::fromName("green"); })
        .def_static("blue", [] { return nova::vg::Color::fromName("blue"); })
        .def_static("yellow", [] { return nova::vg::Color::fromName("yellow"); })
        .def_static("cyan", [] { return nova::vg::Color::fromName("cyan"); })
        .def_static("magenta", [] { return nova::vg::Color::fromName("magenta"); })
        .def_static("orange", [] { return nova::vg::Color::fromName("orange"); })
        .def_static("transparent", [] { return nova::vg::Color(0, 0, 0, 0); })
        .def_prop_rw("r", &nova::vg::Color::getR, &nova::vg::Color::setR)
        .def_prop_rw("g", &nova::vg::Color::getG, &nova::vg::Color::setG)
        .def_prop_rw("b", &nova::vg::Color::getB, &nova::vg::Color::setB)
        .def_prop_rw("a", &nova::vg::Color::getA, &nova::vg::Color::setA)
        .def("__str__", [](const nova::vg::Color& color) {
            return static_cast<std::string>(color);
        })
        .def("__eq__", [](const nova::vg::Color& lhs, const nova::vg::Color& rhs) {
            return lhs == rhs;
        });

    // --- Bind Matrix Class ---
    nb::class_<nova::vg::Matrix>(m, "Matrix")
        .def(nb::init<>(), "Construct an identity matrix.")
        .def(nb::init<float, float, float, float, float, float>(), 
             "a"_a, "b"_a, "c"_a, "d"_a, "e"_a, "f"_a,
             "Construct a matrix with specific components.")
        .def_rw("a", &nova::vg::Matrix::a, "Horizontal scaling factor.")
        .def_rw("b", &nova::vg::Matrix::b, "Vertical shearing factor.")
        .def_rw("c", &nova::vg::Matrix::c, "Horizontal shearing factor.")
        .def_rw("d", &nova::vg::Matrix::d, "Vertical scaling factor.")
        .def_rw("e", &nova::vg::Matrix::e, "Horizontal translation offset.")
        .def_rw("f", &nova::vg::Matrix::f, "Vertical translation offset.")
        .def("multiply", &nova::vg::Matrix::operator*, nb::is_operator(), "Multiply this matrix by another.")
        .def("translate", &nova::vg::Matrix::translate, "tx"_a, "ty"_a, "Translate the matrix.")
        .def("scale", &nova::vg::Matrix::scale, "sx"_a, "sy"_a, "Scale the matrix.")
        .def("rotate", &nova::vg::Matrix::rotate, "angle"_a, "cx"_a = 0.f, "cy"_a = 0.f, "Rotate the matrix around a point.")
        .def("invert", &nova::vg::Matrix::invert, "Invert this matrix in place.")
        .def("inverse", &nova::vg::Matrix::inverse, "Return a new inverted matrix.")
        .def("reset", &nova::vg::Matrix::reset, "Reset the matrix to identity.")
        .def_static("translated", &nova::vg::Matrix::translated, "tx"_a, "ty"_a, "Create a translation matrix.")
        .def_static("scaled", &nova::vg::Matrix::scaled, "sx"_a, "sy"_a, "Create a scaling matrix.")
        .def_static("rotated", &nova::vg::Matrix::rotated, "angle"_a, "cx"_a = 0.f, "cy"_a = 0.f, "Create a rotation matrix.");

    // --- Bind Box Class ---
    nb::class_<nova::vg::Box>(m, "Box")
        .def(nb::init<>(), "Construct a box with zero dimensions.")
        .def(nb::init<float, float, float, float>(), "x"_a, "y"_a, "w"_a, "h"_a, "Construct a box with position and size.")
        .def_rw("x", &nova::vg::Box::x, "X-coordinate of the box.")
        .def_rw("y", &nova::vg::Box::y, "Y-coordinate of the box.")
        .def_rw("w", &nova::vg::Box::w, "Width of the box.")
        .def_rw("h", &nova::vg::Box::h, "Height of the box.")
        .def("transform", &nova::vg::Box::transform, "matrix"_a, "Transform the box by a matrix in place.")
        .def("transformed", &nova::vg::Box::transformed, "matrix"_a, "Return a new transformed box.");

    // --- Bind Bitmap Class ---
    // The buffer protocol, DLPack, NumPy, __array_interface__ and array import all live in
    // bitmap_interop.hpp; bind_bitmap_interop() adds them below, after the chain.
    nb::class_<nova::vg::Bitmap> bitmap_cls(m, "Bitmap", nb::type_slots(novasvg_py_interop::bitmap_slots));
    bitmap_cls
        .def(nb::init<>(), "Construct a null bitmap.")
        .def(nb::init<int, int>(), "width"_a, "height"_a, "Construct a bitmap with specified dimensions.")
        .def_prop_ro("width", &nova::vg::Bitmap::width, "Get the width of the bitmap.")
        .def_prop_ro("height", &nova::vg::Bitmap::height, "Get the height of the bitmap.")
        .def_prop_ro("stride", &nova::vg::Bitmap::stride, "Get the stride (bytes per row) of the bitmap.")
        .def("is_null", &nova::vg::Bitmap::is_null, "Check if the bitmap is null.")
        .def("clear", &nova::vg::Bitmap::clear, "value"_a, "Clear the bitmap with a Color.")
        .def("clear", [](nova::vg::Bitmap& bitmap, uint32_t value) {
            bitmap.clear(nova::vg::Color::fromValue(value));
        }, "value"_a, "Clear the bitmap with a packed 0xRRGGBBAA value.")
        .def("convert_to_rgba", &nova::vg::Bitmap::convertToRGBA, "Convert pixel data in place from ARGB32 Premultiplied to RGBA Plain. Only needed to hand the raw "
             "buffer to something else (e.g. numpy consumers wanting straight RGBA); NOT before "
             "write_to_*/to_*/write, which convert internally -- doing both converts twice.")
        // The encoders below all take the Bitmap as it comes out of
        // render_to_bitmap() (ARGB32 premultiplied) and do their own
        // premultiplied->straight-RGBA conversion internally, into a
        // scratch copy. Do NOT call convert_to_rgba() first: that converts
        // the Bitmap in place, and encoding it afterwards converts it a
        // *second* time -- swapping red/blue channels and corrupting
        // semi-transparent pixels.
        .def("write_to_png", [](const nova::vg::Bitmap &bmp, const std::string &filename) {
            return write_to_file(bmp, filename, image_format::png);
        }, "filename"_a, "Write the bitmap to a PNG file. Returns True on success.")
        .def("write_to_bmp", [](const nova::vg::Bitmap &bmp, const std::string &filename) {
            return write_to_file(bmp, filename, image_format::bmp);
        }, "filename"_a, "Write the bitmap to a BMP file. Returns True on success.")
        .def("write_to_tga", [](const nova::vg::Bitmap &bmp, const std::string &filename) {
            return write_to_file(bmp, filename, image_format::tga);
        }, "filename"_a, "Write the bitmap to a TGA file. Returns True on success.")
        .def("write_to_jpg", [](const nova::vg::Bitmap &bmp, const std::string &filename, int quality) {
            return write_to_file(bmp, filename, image_format::jpg, quality);
        }, "filename"_a, "quality"_a = 80,
           "Write the bitmap to a JPEG file (quality 1-100). JPEG has no alpha channel: "
           "render onto an opaque background first. Returns True on success.")
        .def("write", [](const nova::vg::Bitmap &bmp, const std::string &filename, int jpg_quality) {
            // The extension picks the encoder; anything unrecognised is PNG, as it always was.
            std::string ext = std::filesystem::path(filename).extension().string();
            image_format format = image_format::png;
            if(!parse_format(ext, format))
                format = image_format::png;
            return write_to_file(bmp, filename, format, jpg_quality);
        }, "filename"_a, "jpg_quality"_a = 80,
           "Write the bitmap to a file, picking PNG/BMP/TGA/JPEG from the extension "
           "(.png/.bmp/.tga/.jpg/.jpeg; PNG if unrecognized). Returns True on success.")
        .def("to_png", [](const nova::vg::Bitmap &bmp) { return encode_to_bytes(bmp, image_format::png); },
             "Encode the bitmap as PNG and return the file contents as bytes (no temp file).")
        .def("to_bmp", [](const nova::vg::Bitmap &bmp) { return encode_to_bytes(bmp, image_format::bmp); },
             "Encode the bitmap as BMP and return the file contents as bytes.")
        .def("to_tga", [](const nova::vg::Bitmap &bmp) { return encode_to_bytes(bmp, image_format::tga); },
             "Encode the bitmap as TGA and return the file contents as bytes.")
        .def("to_jpg", [](const nova::vg::Bitmap &bmp, int quality) {
            return encode_to_bytes(bmp, image_format::jpg, quality);
        }, "quality"_a = 80,
           "Encode the bitmap as JPEG (quality 1-100) and return the file contents as bytes. "
           "JPEG has no alpha channel: render onto an opaque background first.")
        .def("to_bytes", [](const nova::vg::Bitmap &bmp, const std::string &format, int quality) {
            image_format parsed;
            if(!parse_format(format, parsed))
                throw std::invalid_argument("unknown image format '" + format + "' (expected png, bmp, tga, jpg or jpeg)");
            return encode_to_bytes(bmp, parsed, quality);
        }, "format"_a = "png", "quality"_a = 80,
           "Encode the bitmap in the given format ('png', 'bmp', 'tga', 'jpg'/'jpeg'; a leading '.' "
           "and any letter case are accepted) and return the file contents as bytes. `quality` "
           "(1-100) is used only for JPEG.");

    novasvg_py_interop::bind_bitmap_interop(bitmap_cls);

    // --- Bind node Class ---
    nb::class_<nova::svg::node>(m, "Node")
        .def("is_text_node", &nova::svg::node::is_text_node, "Check if the node is a text node.")
        .def("is_element", &nova::svg::node::is_element, "Check if the node is an element node.")
        .def("to_text_node", &nova::svg::node::to_text_node, "Convert the node to a TextNode.")
        .def("to_element", &nova::svg::node::to_element, "Convert the node to an Element.")
        .def("parent_element", &nova::svg::node::parent_element, "Get the parent element.")
        .def("is_null", &nova::svg::node::is_null, "Check if the node is null.")
        .def("__bool__", &nova::svg::node::operator bool, "Check if the node is valid.")
        .def("__eq__", &nova::svg::node::operator==, "Check equality.")
        .def("__ne__", &nova::svg::node::operator!=, "Check inequality.");

    // --- Bind text_node Class ---
    nb::class_<nova::svg::text_node, nova::svg::node>(m, "TextNode")
        .def(nb::init<>(), "Construct a null text node.")
        .def("data", &nova::svg::text_node::data, nb::rv_policy::reference, "Get the text content.")
        .def("set_data", &nova::svg::text_node::set_data, "data"_a, "Set the text content.");

    // --- Bind element Class ---
    nb::class_<nova::svg::element, nova::svg::node>(m, "Element")
        .def(nb::init<>(), "Construct a null element.")
        .def("has_attribute", &nova::svg::element::has_attribute, "name"_a, "Check if an attribute exists.")
        .def("get_attribute", &nova::svg::element::attribute, "name"_a, nb::rv_policy::reference, "Get an attribute value.")
        .def("set_attribute", &nova::svg::element::set_attribute, "name"_a, "value"_a, "Set an attribute value.")
        .def("render", static_cast<void (nova::svg::element::*)(nova::vg::Bitmap&, const nova::vg::Matrix&) const>(&nova::svg::element::render), "bitmap"_a, "matrix"_a = nova::vg::Matrix(), "Render the element onto a bitmap.")
        .def("render_to_bitmap", &nova::svg::element::render_to_bitmap, 
             "width"_a = -1, "height"_a = -1, "backgroundColor"_a = nova::vg::Color(),
             "Render the element to a new Bitmap.")
        .def("render_to_bitmap", [](const nova::svg::element& el, int width, int height,
                                    uint32_t background_color) {
            return el.render_to_bitmap(width, height, nova::vg::Color::fromValue(background_color));
        }, "width"_a = -1, "height"_a = -1, "backgroundColor"_a = 0u,
           "Render the element using a packed 0xRRGGBBAA background color.")
        .def("get_local_matrix", &nova::svg::element::local_matrix, "Get the local transformation matrix.")
        .def("get_global_matrix", &nova::svg::element::global_matrix, "Get the global transformation matrix.")
        .def("get_local_bounding_box", &nova::svg::element::local_bounding_box, "Get the local bounding box.")
        .def("get_global_bounding_box", &nova::svg::element::global_bounding_box, "Get the global bounding box.")
        .def("get_bounding_box", &nova::svg::element::bounding_box, "Get the bounding box without transformations.")
        .def("children", &nova::svg::element::children, "Get child nodes.");

    // --- Bind document Class ---
    nb::class_<nova::svg::document>(m, "Document")
        .def_static("load_from_file", &nova::svg::document::load_file, "filename"_a, "Load an SVG document from a file.")
        .def_static("load_from_data", 
            nb::overload_cast<const std::string&>(&nova::svg::document::load_data), 
            "string"_a, "Load an SVG document from a string.")
        .def_static("load_from_data", 
            [](const nb::bytes& data) {
                return nova::svg::document::load_data(reinterpret_cast<const char*>(data.data()), data.size());
            }, 
            "data"_a, "Load an SVG document from a bytes object.")
        .def("apply_stylesheet", &nova::svg::document::apply_style_sheet, "content"_a, "Apply a CSS stylesheet to the document.")
        .def("query_selector_all", &nova::svg::document::query_selector_all, "content"_a, "Select elements matching a CSS selector.")
        .def_prop_ro("width", &nova::svg::document::width, "Get the intrinsic width of the document.")
        .def_prop_ro("height", &nova::svg::document::height, "Get the intrinsic height of the document.")
        .def("bounding_box", &nova::svg::document::bounding_box, "Get the bounding box of the document content.")
        .def("update_layout", &nova::svg::document::update_layout, "Update the layout of the document if needed.")
        .def("force_layout", &nova::svg::document::force_layout, "Force an immediate layout update.")
        .def("render", static_cast<void (nova::svg::document::*)(nova::vg::Bitmap&, const nova::vg::Matrix&) const>(&nova::svg::document::render), "bitmap"_a, "matrix"_a = nova::vg::Matrix(), "Render the document onto a bitmap.")
        .def("render_to_bitmap", &nova::svg::document::render_to_bitmap, 
             "width"_a = -1, "height"_a = -1, "backgroundColor"_a = nova::vg::Color(),
             "Render the document to a new Bitmap.")
        .def("render_to_bitmap", [](const nova::svg::document& dom, int width, int height,
                                    uint32_t background_color) {
            return dom.render_to_bitmap(width, height, nova::vg::Color::fromValue(background_color));
        }, "width"_a = -1, "height"_a = -1, "backgroundColor"_a = 0u,
           "Render the document using a packed 0xRRGGBBAA background color.")
        .def("element_from_point", &nova::svg::document::element_from_point, "x"_a, "y"_a, "Get the topmost element at a specific point.")
        .def("get_element_by_id", &nova::svg::document::element_by_id, "id"_a, "Get an element by its ID.")
        .def("document_element", &nova::svg::document::document_element, "Get the root element of the document.");
}
