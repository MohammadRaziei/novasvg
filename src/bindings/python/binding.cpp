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

// Include the main library header
#include "novasvg/novasvg.h"
#include "bitmap_interop.hpp"

namespace nb = nanobind;
using namespace nb::literals;


namespace {

// Collects an encoder's streamed output (novasvg_write_func_t chunks) into
// one std::string. Bitmap::writeTo{Png,Bmp,Tga,Jpg}()'s callback overloads
// are what make an in-memory encode possible without a temp file.
void collect_chunk(void* closure, void* data, int size)
{
    static_cast<std::string*>(closure)->append(static_cast<const char*>(data), size_t(size));
}

// Runs `encode(callback, closure)` -- one of the Bitmap::writeTo*()
// stream overloads, bound with its format-specific extra arguments -- and
// returns the result as `bytes`. A null bitmap is the caller's mistake
// (ValueError); anything else that leaves the encoder reporting failure
// or producing nothing is a RuntimeError, never a silent empty `bytes`
// (an encoder's own bool return is easy to drop on the floor, and an
// empty result would otherwise look like a valid, empty image).
template<typename Encode>
nb::bytes encode_to_bytes(const nova::svg::Bitmap& bitmap, const char* format, Encode&& encode)
{
    if(bitmap.isNull())
        throw std::invalid_argument(std::string("cannot encode a null Bitmap as ") + format);
    std::string buffer;
    if(!encode(&collect_chunk, &buffer) || buffer.empty())
        throw std::runtime_error(std::string("failed to encode Bitmap as ") + format);
    return nb::bytes(buffer.data(), buffer.size());
}

std::string lowercase(std::string text)
{
    for(auto& c : text)
        c = char(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

} // namespace

NB_MODULE(novasvg_py, m) {
    m.doc() = "Python bindings for NovaSVG using nanobind";

    // Color parse errors: novasvg.InvalidColorError, a ValueError subclass.
    nb::exception<nova::svg::InvalidColorError>(m, "InvalidColorError", PyExc_ValueError);

    // --- Bind Static Utility Functions ---
    m.def("version_string", &nova::svg::versionString, "Get the library version as a string (e.g., '0.1.0').");
    m.def("add_font_face_from_file", &nova::svg::addFontFaceFromFile, 
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
               return nova::svg::addFontFaceFromData(family, bold, italic, copy, data.size(),
                                                     [](void* p) { std::free(p); }, copy);
          },
          "family"_a, "bold"_a, "italic"_a, "data"_a,
          "Add a font face from a memory buffer (bytes) to the cache.");

    // --- Bind FontFace / Font (text measurement) ---
    // Exposes novasvg's own font stack (a vendored stb_truetype under
    // include/novasvg/detail/render/font.h) to Python, so callers that need
    // to measure text -- e.g. mermaidx computing label/box sizes before it
    // ever builds the SVG -- can ask the exact same engine that will later
    // paint the glyphs, instead of hand-rolling a second TTF parser that
    // has to be kept in sync with it by hand.
    nb::class_<nova::svg::FontFace>(m, "FontFace")
        .def(nb::init<>(), "Construct a null font face.")
        .def(nb::init<const char*>(), "filename"_a, "Load a font face directly from a font file (no cache involved).")
        .def("__init__", [](nova::svg::FontFace* self, nb::bytes data) {
            auto* copy = std::malloc(data.size());
            if(copy == nullptr) {
                new (self) nova::svg::FontFace();
                return;
            }
            std::memcpy(copy, data.data(), data.size());
            new (self) nova::svg::FontFace(copy, data.size(), [](void* p) { std::free(p); }, copy);
        }, "data"_a, "Load a font face directly from a memory buffer (bytes), no cache involved.")
        .def("is_null", &nova::svg::FontFace::isNull, "Check if the font face is null/invalid.")
        .def("__bool__", [](const nova::svg::FontFace& face) { return !face.isNull(); })
        .def_prop_ro("units_per_em", &nova::svg::FontFace::unitsPerEm,
             "Raw font design-unit space this face's own tables are defined in "
             "(e.g. head.unitsPerEm). Pair with advance_width_units()/ascent_units()/"
             "descent_units() to build a complete, size-independent advance table -- "
             "e.g. to hand text-measurement data to a JS engine that can't call back "
             "into Python per string (mermaidx's v8_engine does exactly this).")
        .def_prop_ro("ascent_units", &nova::svg::FontFace::ascentUnits, "Ascent in raw font design units (unscaled).")
        .def_prop_ro("descent_units", &nova::svg::FontFace::descentUnits, "Descent in raw font design units (unscaled).")
        .def("advance_width_units", [](const nova::svg::FontFace& face, uint32_t codepoint) {
            return face.advanceWidthUnits(static_cast<char32_t>(codepoint));
        }, "codepoint"_a, "Advance width of a single Unicode codepoint, in raw font design units (unscaled).")
        .def("kern_advance_units", [](const nova::svg::FontFace& face, uint32_t first, uint32_t second) {
            return face.kernAdvanceUnits(static_cast<char32_t>(first), static_cast<char32_t>(second));
        }, "first"_a, "second"_a,
           "Kerning adjustment between two consecutive codepoints, in raw font design units "
           "(unscaled) -- see FontFace::kernAdvanceUnits() in font.h for why this is a per-pair "
           "query rather than a bulk table like codepoints().")
        .def_prop_ro("notdef_advance_width_units", &nova::svg::FontFace::notdefAdvanceWidthUnits,
             "The advance width used for any codepoint outside this font's cmap "
             "(glyph id 0, the \".notdef\" glyph) -- what advance_width_units() itself "
             "already falls back to for such a codepoint.")
        .def("codepoints", [](const nova::svg::FontFace& face) {
            auto cps = face.codepoints();
            std::vector<uint32_t> out;
            out.reserve(cps.size());
            for(auto cp : cps)
                out.push_back(static_cast<uint32_t>(cp));
            return out;
        }, "Every Unicode codepoint this face's cmap maps to a glyph (see FontFace::codepoints() in font.h "
           "for exactly which cmap formats are covered -- virtually every real-world font).");

    m.def("get_font_face", [](const std::string& family, bool bold, bool italic) {
        return nova::svg::fontFaceCache()->getFontFace(family, bold, italic);
    }, "family"_a, "bold"_a = false, "italic"_a = false,
       "Look up a font face previously registered via add_font_face_from_file/_data "
       "(falling back to novasvg's built-in generic-family table, e.g. \"sans-serif\").");

    m.def("get_font_face_for_family_stack", [](const std::string& family_stack, bool bold, bool italic) {
        return nova::svg::fontFaceCache()->getFontFaceForFamilyStack(family_stack, bold, italic);
    }, "family_stack"_a, "bold"_a = false, "italic"_a = false,
       "Resolve a CSS-style comma-separated font-family stack (e.g. "
       "'\"trebuchet ms\", verdana, arial, sans-serif') name by name: a face registered or "
       "installed under that exact name, then a metric-compatible substitute from novasvg's "
       "built-in table (case-insensitive, e.g. 'arial' -> Liberation Sans), then generic "
       "keywords. Names nothing matches are skipped; returns a null face if none resolve.");

    nb::class_<nova::svg::Font>(m, "Font")
        .def(nb::init<>(), "Construct a null font.")
        .def(nb::init<const nova::svg::FontFace&, float>(), "face"_a, "size"_a,
             "Construct a font from a face at a given pixel size.")
        .def_prop_ro("ascent", &nova::svg::Font::ascent, "Ascent in pixels, at this font's size.")
        .def_prop_ro("descent", &nova::svg::Font::descent, "Descent in pixels (typically negative), at this font's size.")
        .def_prop_ro("height", &nova::svg::Font::height, "ascent - descent.")
        .def_prop_ro("line_gap", &nova::svg::Font::lineGap, "Recommended extra spacing between lines, in pixels.")
        .def_prop_ro("x_height", &nova::svg::Font::xHeight, "Height of a lowercase 'x' glyph, in pixels.")
        .def_prop_ro("size", &nova::svg::Font::size, "The pixel size this font was constructed with.")
        .def_prop_ro("face", &nova::svg::Font::face, nb::rv_policy::reference_internal, "The underlying FontFace.")
        .def("is_null", &nova::svg::Font::isNull, "Check if the font is null/invalid.")
        .def("__bool__", [](const nova::svg::Font& font) { return !font.isNull(); })
        .def("measure_text", [](const nova::svg::Font& font, const std::string& text) {
            // Python str arrives as UTF-8 (nanobind's std::string conversion);
            // Font::measureText wants UTF-32, same as novasvg's own SVG text
            // layout path (see SVGTextFragmentsBuilder), so route through the
            // identical utf8ToU32() conversion for byte-for-byte consistency
            // with what novasvg will actually paint.
            auto u32text = nova::svg::utf8ToU32(text);
            return font.measureText(u32text);
        }, "text"_a, "Measure the advance width of a UTF-8 string, in pixels, at this font's size.");

    // nb::class_<Font> chain above ends with measure_text; attach glyph_boxes to it.
    m.attr("Font").attr("glyph_boxes") = nb::cpp_function(
        [](const nova::svg::Font& font, const std::string& text) {
            nb::list out;
            for(const auto& box : font.glyphBoxes(nova::svg::utf8ToU32(text)))
                out.append(nb::make_tuple(box.pen, box.inkLeft, box.inkRight, box.inkTop, box.inkBottom));
            return out;
        }, nb::is_method(), "text"_a,
        "Per-glyph (pen_x, ink_left, ink_right, ink_top, ink_bottom) in pixels (top/bottom relative to the baseline, y down) for every glyph that has an outline "
        "(spaces are skipped), using the same ligature + kerning walk as measure_text().");

    m.def("measure_foreign_object", [](const std::string& html, const nova::svg::Font& font, std::optional<float> max_width,
                                       std::optional<std::string> family, bool bold, bool italic) {
        nova::svg::ForeignObjectMetrics metrics;
        if(!family) {
            metrics = nova::svg::measureForeignObjectContent(html, font, max_width.value_or(-1.f));
        } else {
            // The same faces ForeignObjectSimple::render() paints with (SVGForeignObjectElement::fontsFor()):
            // `font` is the base style, every other bold/italic variant comes from the family.
            const unsigned char base = (unsigned char)((bold ? nova::svg::ForeignObjectFonts::Bold : 0)
                                                       | (italic ? nova::svg::ForeignObjectFonts::Italic : 0));
            nova::svg::ForeignObjectFonts fonts;
            for(unsigned style = 0; style < 8; ++style) {
                if(style == base) {
                    fonts.face[style] = font;
                    continue;
                }
                auto variant = nova::svg::makeFont((style & nova::svg::ForeignObjectFonts::Mono) ? std::string("monospace") : *family,
                                                 font.size(), (style & nova::svg::ForeignObjectFonts::Bold) != 0,
                                                 (style & nova::svg::ForeignObjectFonts::Italic) != 0);
                fonts.face[style] = variant.isNull() ? font : variant;
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
    nb::class_<nova::svg::Color>(m, "Color")
        .def(nb::init<uint8_t, uint8_t, uint8_t, uint8_t>(),
             "r"_a = 0, "g"_a = 0, "b"_a = 0, "a"_a = 255,
             "Construct a color from red, green, blue, alpha components (0-255 each).")
        .def(nb::init<const std::string&>(), "value"_a,
             "Construct a color from a CSS color name or hexadecimal string.")
        .def_static("from_value", &nova::svg::Color::fromValue, "value"_a,
                    "Construct a color from a packed 0xRRGGBBAA integer.")
        .def_static("from_hash", &nova::svg::Color::fromHash, "value"_a)
        .def_static("from_name", &nova::svg::Color::fromName, "value"_a)
        .def("value", &nova::svg::Color::value,
             "Get the color packed as a 0xRRGGBBAA integer.")
        .def("to_int", &nova::svg::Color::value)
        .def_static("rgba", [](uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
            return nova::svg::Color(r, g, b, a);
        }, "r"_a, "g"_a, "b"_a, "a"_a = 255)
        .def_static("black", [] { return nova::svg::Color::fromName("black"); })
        .def_static("white", [] { return nova::svg::Color::fromName("white"); })
        .def_static("red", [] { return nova::svg::Color::fromName("red"); })
        .def_static("green", [] { return nova::svg::Color::fromName("green"); })
        .def_static("blue", [] { return nova::svg::Color::fromName("blue"); })
        .def_static("yellow", [] { return nova::svg::Color::fromName("yellow"); })
        .def_static("cyan", [] { return nova::svg::Color::fromName("cyan"); })
        .def_static("magenta", [] { return nova::svg::Color::fromName("magenta"); })
        .def_static("orange", [] { return nova::svg::Color::fromName("orange"); })
        .def_static("transparent", [] { return nova::svg::Color(0, 0, 0, 0); })
        .def_prop_rw("r", &nova::svg::Color::getR, &nova::svg::Color::setR)
        .def_prop_rw("g", &nova::svg::Color::getG, &nova::svg::Color::setG)
        .def_prop_rw("b", &nova::svg::Color::getB, &nova::svg::Color::setB)
        .def_prop_rw("a", &nova::svg::Color::getA, &nova::svg::Color::setA)
        .def("__str__", [](const nova::svg::Color& color) {
            return static_cast<std::string>(color);
        })
        .def("__eq__", [](const nova::svg::Color& lhs, const nova::svg::Color& rhs) {
            return lhs == rhs;
        });

    // --- Bind Matrix Class ---
    nb::class_<nova::svg::Matrix>(m, "Matrix")
        .def(nb::init<>(), "Construct an identity matrix.")
        .def(nb::init<float, float, float, float, float, float>(), 
             "a"_a, "b"_a, "c"_a, "d"_a, "e"_a, "f"_a,
             "Construct a matrix with specific components.")
        .def_rw("a", &nova::svg::Matrix::a, "Horizontal scaling factor.")
        .def_rw("b", &nova::svg::Matrix::b, "Vertical shearing factor.")
        .def_rw("c", &nova::svg::Matrix::c, "Horizontal shearing factor.")
        .def_rw("d", &nova::svg::Matrix::d, "Vertical scaling factor.")
        .def_rw("e", &nova::svg::Matrix::e, "Horizontal translation offset.")
        .def_rw("f", &nova::svg::Matrix::f, "Vertical translation offset.")
        .def("multiply", &nova::svg::Matrix::operator*, nb::is_operator(), "Multiply this matrix by another.")
        .def("translate", &nova::svg::Matrix::translate, "tx"_a, "ty"_a, "Translate the matrix.")
        .def("scale", &nova::svg::Matrix::scale, "sx"_a, "sy"_a, "Scale the matrix.")
        .def("rotate", &nova::svg::Matrix::rotate, "angle"_a, "cx"_a = 0.f, "cy"_a = 0.f, "Rotate the matrix around a point.")
        .def("invert", &nova::svg::Matrix::invert, "Invert this matrix in place.")
        .def("inverse", &nova::svg::Matrix::inverse, "Return a new inverted matrix.")
        .def("reset", &nova::svg::Matrix::reset, "Reset the matrix to identity.")
        .def_static("translated", &nova::svg::Matrix::translated, "tx"_a, "ty"_a, "Create a translation matrix.")
        .def_static("scaled", &nova::svg::Matrix::scaled, "sx"_a, "sy"_a, "Create a scaling matrix.")
        .def_static("rotated", &nova::svg::Matrix::rotated, "angle"_a, "cx"_a = 0.f, "cy"_a = 0.f, "Create a rotation matrix.");

    // --- Bind Box Class ---
    nb::class_<nova::svg::Box>(m, "Box")
        .def(nb::init<>(), "Construct a box with zero dimensions.")
        .def(nb::init<float, float, float, float>(), "x"_a, "y"_a, "w"_a, "h"_a, "Construct a box with position and size.")
        .def_rw("x", &nova::svg::Box::x, "X-coordinate of the box.")
        .def_rw("y", &nova::svg::Box::y, "Y-coordinate of the box.")
        .def_rw("w", &nova::svg::Box::w, "Width of the box.")
        .def_rw("h", &nova::svg::Box::h, "Height of the box.")
        .def("transform", &nova::svg::Box::transform, "matrix"_a, "Transform the box by a matrix in place.")
        .def("transformed", &nova::svg::Box::transformed, "matrix"_a, "Return a new transformed box.");

    // --- Bind Bitmap Class ---
    // The buffer protocol, DLPack, NumPy, __array_interface__ and array import all live in
    // bitmap_interop.hpp; bind_bitmap_interop() adds them below, after the chain.
    nb::class_<nova::svg::Bitmap> bitmap_cls(m, "Bitmap", nb::type_slots(novasvg_py_interop::bitmap_slots));
    bitmap_cls
        .def(nb::init<>(), "Construct a null bitmap.")
        .def(nb::init<int, int>(), "width"_a, "height"_a, "Construct a bitmap with specified dimensions.")
        .def_prop_ro("width", &nova::svg::Bitmap::width, "Get the width of the bitmap.")
        .def_prop_ro("height", &nova::svg::Bitmap::height, "Get the height of the bitmap.")
        .def_prop_ro("stride", &nova::svg::Bitmap::stride, "Get the stride (bytes per row) of the bitmap.")
        .def("is_null", &nova::svg::Bitmap::isNull, "Check if the bitmap is null.")
        .def("clear", &nova::svg::Bitmap::clear, "value"_a, "Clear the bitmap with a Color.")
        .def("clear", [](nova::svg::Bitmap& bitmap, uint32_t value) {
            bitmap.clear(nova::svg::Color::fromValue(value));
        }, "value"_a, "Clear the bitmap with a packed 0xRRGGBBAA value.")
        .def("convert_to_rgba", &nova::svg::Bitmap::convertToRGBA, "Convert pixel data in place from ARGB32 Premultiplied to RGBA Plain. Only needed to hand the raw "
             "buffer to something else (e.g. numpy consumers wanting straight RGBA); NOT before "
             "write_to_*/to_*/write, which convert internally -- doing both converts twice.")
        // The encoders below all take the Bitmap as it comes out of
        // render_to_bitmap() (ARGB32 premultiplied) and do their own
        // premultiplied->straight-RGBA conversion internally, into a
        // scratch copy. Do NOT call convert_to_rgba() first: that converts
        // the Bitmap in place, and encoding it afterwards converts it a
        // *second* time -- swapping red/blue channels and corrupting
        // semi-transparent pixels.
        .def("write_to_png", [](const nova::svg::Bitmap &bmp, const std::string &filename) {
            return bmp.writeToPng(filename);
        }, "filename"_a, "Write the bitmap to a PNG file. Returns True on success.")
        .def("write_to_bmp", [](const nova::svg::Bitmap &bmp, const std::string &filename) {
            return bmp.writeToBmp(filename);
        }, "filename"_a, "Write the bitmap to a BMP file. Returns True on success.")
        .def("write_to_tga", [](const nova::svg::Bitmap &bmp, const std::string &filename) {
            return bmp.writeToTga(filename);
        }, "filename"_a, "Write the bitmap to a TGA file. Returns True on success.")
        .def("write_to_jpg", [](const nova::svg::Bitmap &bmp, const std::string &filename, int quality) {
            return bmp.writeToJpg(filename, quality);
        }, "filename"_a, "quality"_a = 80,
           "Write the bitmap to a JPEG file (quality 1-100). JPEG has no alpha channel: "
           "render onto an opaque background first. Returns True on success.")
        .def("write", [](const nova::svg::Bitmap &bmp, const std::string &filename, int jpg_quality) {
            return bmp.write(filename, jpg_quality);
        }, "filename"_a, "jpg_quality"_a = 80,
           "Write the bitmap to a file, picking PNG/BMP/TGA/JPEG from the extension "
           "(.png/.bmp/.tga/.jpg/.jpeg; PNG if unrecognized). Returns True on success.")
        .def("to_png", [](const nova::svg::Bitmap &bmp) {
            return encode_to_bytes(bmp, "PNG", [&](novasvg_write_func_t cb, void* closure) {
                return bmp.writeToPng(cb, closure);
            });
        }, "Encode the bitmap as PNG and return the file contents as bytes (no temp file).")
        .def("to_bmp", [](const nova::svg::Bitmap &bmp) {
            return encode_to_bytes(bmp, "BMP", [&](novasvg_write_func_t cb, void* closure) {
                return bmp.writeToBmp(cb, closure);
            });
        }, "Encode the bitmap as BMP and return the file contents as bytes.")
        .def("to_tga", [](const nova::svg::Bitmap &bmp) {
            return encode_to_bytes(bmp, "TGA", [&](novasvg_write_func_t cb, void* closure) {
                return bmp.writeToTga(cb, closure);
            });
        }, "Encode the bitmap as TGA and return the file contents as bytes.")
        .def("to_jpg", [](const nova::svg::Bitmap &bmp, int quality) {
            return encode_to_bytes(bmp, "JPEG", [&](novasvg_write_func_t cb, void* closure) {
                return bmp.writeToJpg(cb, closure, quality);
            });
        }, "quality"_a = 80,
           "Encode the bitmap as JPEG (quality 1-100) and return the file contents as bytes. "
           "JPEG has no alpha channel: render onto an opaque background first.")
        .def("to_bytes", [](const nova::svg::Bitmap &bmp, const std::string &format, int quality) {
            auto fmt = lowercase(format);
            if(!fmt.empty() && fmt[0] == '.')
                fmt.erase(0, 1);
            if(fmt == "png")
                return encode_to_bytes(bmp, "PNG", [&](novasvg_write_func_t cb, void* c) { return bmp.writeToPng(cb, c); });
            if(fmt == "bmp")
                return encode_to_bytes(bmp, "BMP", [&](novasvg_write_func_t cb, void* c) { return bmp.writeToBmp(cb, c); });
            if(fmt == "tga")
                return encode_to_bytes(bmp, "TGA", [&](novasvg_write_func_t cb, void* c) { return bmp.writeToTga(cb, c); });
            if(fmt == "jpg" || fmt == "jpeg")
                return encode_to_bytes(bmp, "JPEG", [&](novasvg_write_func_t cb, void* c) { return bmp.writeToJpg(cb, c, quality); });
            throw std::invalid_argument("unknown image format '" + format + "' (expected png, bmp, tga, jpg or jpeg)");
        }, "format"_a = "png", "quality"_a = 80,
           "Encode the bitmap in the given format ('png', 'bmp', 'tga', 'jpg'/'jpeg'; a leading '.' "
           "and any letter case are accepted) and return the file contents as bytes. `quality` "
           "(1-100) is used only for JPEG.");

    novasvg_py_interop::bind_bitmap_interop(bitmap_cls);

    // --- Bind Node Class ---
    nb::class_<nova::svg::Node>(m, "Node")
        .def("is_text_node", &nova::svg::Node::isTextNode, "Check if the node is a text node.")
        .def("is_element", &nova::svg::Node::isElement, "Check if the node is an element node.")
        .def("to_text_node", &nova::svg::Node::toTextNode, "Convert the node to a TextNode.")
        .def("to_element", &nova::svg::Node::toElement, "Convert the node to an Element.")
        .def("parent_element", &nova::svg::Node::parentElement, "Get the parent element.")
        .def("is_null", &nova::svg::Node::isNull, "Check if the node is null.")
        .def("__bool__", &nova::svg::Node::operator bool, "Check if the node is valid.")
        .def("__eq__", &nova::svg::Node::operator==, "Check equality.")
        .def("__ne__", &nova::svg::Node::operator!=, "Check inequality.");

    // --- Bind TextNode Class ---
    nb::class_<nova::svg::TextNode, nova::svg::Node>(m, "TextNode")
        .def(nb::init<>(), "Construct a null text node.")
        .def("data", &nova::svg::TextNode::data, nb::rv_policy::reference, "Get the text content.")
        .def("set_data", &nova::svg::TextNode::setData, "data"_a, "Set the text content.");

    // --- Bind Element Class ---
    nb::class_<nova::svg::Element, nova::svg::Node>(m, "Element")
        .def(nb::init<>(), "Construct a null element.")
        .def("has_attribute", &nova::svg::Element::hasAttribute, "name"_a, "Check if an attribute exists.")
        .def("get_attribute", &nova::svg::Element::getAttribute, "name"_a, nb::rv_policy::reference, "Get an attribute value.")
        .def("set_attribute", &nova::svg::Element::setAttribute, "name"_a, "value"_a, "Set an attribute value.")
        .def("render", &nova::svg::Element::render, "bitmap"_a, "matrix"_a = nova::svg::Matrix(), "Render the element onto a bitmap.")
        .def("render_to_bitmap", &nova::svg::Element::renderToBitmap, 
             "width"_a = -1, "height"_a = -1, "backgroundColor"_a = nova::svg::Color(),
             "Render the element to a new Bitmap.")
        .def("render_to_bitmap", [](const nova::svg::Element& element, int width, int height,
                                    uint32_t background_color) {
            return element.renderToBitmap(width, height, nova::svg::Color::fromValue(background_color));
        }, "width"_a = -1, "height"_a = -1, "backgroundColor"_a = 0u,
           "Render the element using a packed 0xRRGGBBAA background color.")
        .def("get_local_matrix", &nova::svg::Element::getLocalMatrix, "Get the local transformation matrix.")
        .def("get_global_matrix", &nova::svg::Element::getGlobalMatrix, "Get the global transformation matrix.")
        .def("get_local_bounding_box", &nova::svg::Element::getLocalBoundingBox, "Get the local bounding box.")
        .def("get_global_bounding_box", &nova::svg::Element::getGlobalBoundingBox, "Get the global bounding box.")
        .def("get_bounding_box", &nova::svg::Element::getBoundingBox, "Get the bounding box without transformations.")
        .def("children", &nova::svg::Element::children, "Get child nodes.");

    // --- Bind Document Class ---
    nb::class_<nova::svg::Document>(m, "Document")
        .def_static("load_from_file", &nova::svg::Document::loadFromFile, "filename"_a, "Load an SVG document from a file.")
        .def_static("load_from_data", 
            nb::overload_cast<const std::string&>(&nova::svg::Document::loadFromData), 
            "string"_a, "Load an SVG document from a string.")
        .def_static("load_from_data", 
            [](const nb::bytes& data) {
                return nova::svg::Document::loadFromData(reinterpret_cast<const char*>(data.data()), data.size());
            }, 
            "data"_a, "Load an SVG document from a bytes object.")
        .def("apply_stylesheet", &nova::svg::Document::applyStyleSheet, "content"_a, "Apply a CSS stylesheet to the document.")
        .def("query_selector_all", &nova::svg::Document::querySelectorAll, "content"_a, "Select elements matching a CSS selector.")
        .def_prop_ro("width", &nova::svg::Document::width, "Get the intrinsic width of the document.")
        .def_prop_ro("height", &nova::svg::Document::height, "Get the intrinsic height of the document.")
        .def("bounding_box", &nova::svg::Document::boundingBox, "Get the bounding box of the document content.")
        .def("update_layout", &nova::svg::Document::updateLayout, "Update the layout of the document if needed.")
        .def("force_layout", &nova::svg::Document::forceLayout, "Force an immediate layout update.")
        .def("render", &nova::svg::Document::render, "bitmap"_a, "matrix"_a = nova::svg::Matrix(), "Render the document onto a bitmap.")
        .def("render_to_bitmap", &nova::svg::Document::renderToBitmap, 
             "width"_a = -1, "height"_a = -1, "backgroundColor"_a = nova::svg::Color(),
             "Render the document to a new Bitmap.")
        .def("render_to_bitmap", [](const nova::svg::Document& document, int width, int height,
                                    uint32_t background_color) {
            return document.renderToBitmap(width, height, nova::svg::Color::fromValue(background_color));
        }, "width"_a = -1, "height"_a = -1, "backgroundColor"_a = 0u,
           "Render the document using a packed 0xRRGGBBAA background color.")
        .def("element_from_point", &nova::svg::Document::elementFromPoint, "x"_a, "y"_a, "Get the topmost element at a specific point.")
        .def("get_element_by_id", &nova::svg::Document::getElementById, "id"_a, "Get an element by its ID.")
        .def("document_element", &nova::svg::Document::documentElement, "Get the root element of the document.");
}
