#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/ndarray.h>
#include <cstdint>

// Include the main library header
#include "novasvg/novasvg.h"

namespace nb = nanobind;
using namespace nb::literals;


NB_MODULE(novasvg_py, m) {
    m.doc() = "Python bindings for NovaSVG using nanobind";

    // --- Bind Static Utility Functions ---
    m.def("version_string", &novasvg::versionString, "Get the library version as a string (e.g., '0.1.0').");
    m.def("add_font_face_from_file", &novasvg::addFontFaceFromFile, 
          "family"_a, "bold"_a, "italic"_a, "filename"_a,
          "Add a font face from a file to the cache.");
    
    // Binding for adding font from memory. 
    // The destroy_func and closure are advanced usage; usually defaults to nullptr for simple bindings.
    m.def("add_font_face_from_data", 
          [](const char* family, bool bold, bool italic, nb::bytes data, novasvg_destroy_func_t destroy_func, void* closure) -> bool {
               return novasvg::addFontFaceFromData(family, bold, italic, data.data(), data.size(), destroy_func, closure);
          }, 
          "family"_a, "bold"_a, "italic"_a, "data"_a, "destroy_func"_a = nullptr, "closure"_a = nullptr,
          "Add a font face from a memory buffer.");

    // --- Bind Color Class ---
    nb::class_<novasvg::Color>(m, "Color")
        .def(nb::init<uint8_t, uint8_t, uint8_t, uint8_t>(),
             "r"_a = 0, "g"_a = 0, "b"_a = 0, "a"_a = 255,
             "Construct a color from red, green, blue, alpha components (0-255 each).")
        .def(nb::init<const std::string&>(), "value"_a,
             "Construct a color from a CSS color name or hexadecimal string.")
        .def_static("from_value", &novasvg::Color::fromValue, "value"_a,
                    "Construct a color from a packed 0xRRGGBBAA integer.")
        .def_static("from_hash", &novasvg::Color::fromHash, "value"_a)
        .def_static("from_name", &novasvg::Color::fromName, "value"_a)
        .def("value", &novasvg::Color::value,
             "Get the color packed as a 0xRRGGBBAA integer.")
        .def("to_int", &novasvg::Color::value)
        .def_static("rgba", [](uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
            return novasvg::Color(r, g, b, a);
        }, "r"_a, "g"_a, "b"_a, "a"_a = 255)
        .def_static("black", [] { return novasvg::Color::fromName("black"); })
        .def_static("white", [] { return novasvg::Color::fromName("white"); })
        .def_static("red", [] { return novasvg::Color::fromName("red"); })
        .def_static("green", [] { return novasvg::Color::fromName("green"); })
        .def_static("blue", [] { return novasvg::Color::fromName("blue"); })
        .def_static("yellow", [] { return novasvg::Color::fromName("yellow"); })
        .def_static("cyan", [] { return novasvg::Color::fromName("cyan"); })
        .def_static("magenta", [] { return novasvg::Color::fromName("magenta"); })
        .def_static("orange", [] { return novasvg::Color::fromName("orange"); })
        .def_static("transparent", [] { return novasvg::Color(0, 0, 0, 0); })
        .def_prop_rw("r", &novasvg::Color::getR, &novasvg::Color::setR)
        .def_prop_rw("g", &novasvg::Color::getG, &novasvg::Color::setG)
        .def_prop_rw("b", &novasvg::Color::getB, &novasvg::Color::setB)
        .def_prop_rw("a", &novasvg::Color::getA, &novasvg::Color::setA)
        .def("__str__", [](const novasvg::Color& color) {
            return static_cast<std::string>(color);
        })
        .def("__eq__", [](const novasvg::Color& lhs, const novasvg::Color& rhs) {
            return lhs == rhs;
        });

    // --- Bind Matrix Class ---
    nb::class_<novasvg::Matrix>(m, "Matrix")
        .def(nb::init<>(), "Construct an identity matrix.")
        .def(nb::init<float, float, float, float, float, float>(), 
             "a"_a, "b"_a, "c"_a, "d"_a, "e"_a, "f"_a,
             "Construct a matrix with specific components.")
        .def_rw("a", &novasvg::Matrix::a, "Horizontal scaling factor.")
        .def_rw("b", &novasvg::Matrix::b, "Vertical shearing factor.")
        .def_rw("c", &novasvg::Matrix::c, "Horizontal shearing factor.")
        .def_rw("d", &novasvg::Matrix::d, "Vertical scaling factor.")
        .def_rw("e", &novasvg::Matrix::e, "Horizontal translation offset.")
        .def_rw("f", &novasvg::Matrix::f, "Vertical translation offset.")
        .def("multiply", &novasvg::Matrix::operator*, nb::is_operator(), "Multiply this matrix by another.")
        .def("translate", &novasvg::Matrix::translate, "tx"_a, "ty"_a, "Translate the matrix.")
        .def("scale", &novasvg::Matrix::scale, "sx"_a, "sy"_a, "Scale the matrix.")
        .def("rotate", &novasvg::Matrix::rotate, "angle"_a, "cx"_a = 0.f, "cy"_a = 0.f, "Rotate the matrix around a point.")
        .def("invert", &novasvg::Matrix::invert, "Invert this matrix in place.")
        .def("inverse", &novasvg::Matrix::inverse, "Return a new inverted matrix.")
        .def("reset", &novasvg::Matrix::reset, "Reset the matrix to identity.")
        .def_static("translated", &novasvg::Matrix::translated, "tx"_a, "ty"_a, "Create a translation matrix.")
        .def_static("scaled", &novasvg::Matrix::scaled, "sx"_a, "sy"_a, "Create a scaling matrix.")
        .def_static("rotated", &novasvg::Matrix::rotated, "angle"_a, "cx"_a = 0.f, "cy"_a = 0.f, "Create a rotation matrix.");

    // --- Bind Box Class ---
    nb::class_<novasvg::Box>(m, "Box")
        .def(nb::init<>(), "Construct a box with zero dimensions.")
        .def(nb::init<float, float, float, float>(), "x"_a, "y"_a, "w"_a, "h"_a, "Construct a box with position and size.")
        .def_rw("x", &novasvg::Box::x, "X-coordinate of the box.")
        .def_rw("y", &novasvg::Box::y, "Y-coordinate of the box.")
        .def_rw("w", &novasvg::Box::w, "Width of the box.")
        .def_rw("h", &novasvg::Box::h, "Height of the box.")
        .def("transform", &novasvg::Box::transform, "matrix"_a, "Transform the box by a matrix in place.")
        .def("transformed", &novasvg::Box::transformed, "matrix"_a, "Return a new transformed box.");

    // --- Bind Bitmap Class ---
    nb::class_<novasvg::Bitmap>(m, "Bitmap")
        .def(nb::init<>(), "Construct a null bitmap.")
        .def(nb::init<int, int>(), "width"_a, "height"_a, "Construct a bitmap with specified dimensions.")
        // Expose data as a NumPy array (read-only reference to internal data)
        .def("numpy", [](const novasvg::Bitmap &bmp) {
            if (bmp.isNull()) return nb::ndarray<nb::numpy, uint8_t>();
            // Shape: {height, width, 4} for ARGB32
            return nb::ndarray<nb::numpy, uint8_t>(bmp.data(), {(uint64_t) bmp.height(), (uint64_t) bmp.width(), 4});
        }, nb::rv_policy::reference_internal, "Get pixel data as a NumPy array.")
        .def_prop_ro("width", &novasvg::Bitmap::width, "Get the width of the bitmap.")
        .def_prop_ro("height", &novasvg::Bitmap::height, "Get the height of the bitmap.")
        .def_prop_ro("stride", &novasvg::Bitmap::stride, "Get the stride (bytes per row) of the bitmap.")
        .def("is_null", &novasvg::Bitmap::isNull, "Check if the bitmap is null.")
        .def("clear", &novasvg::Bitmap::clear, "value"_a, "Clear the bitmap with a Color.")
        .def("clear", [](novasvg::Bitmap& bitmap, uint32_t value) {
            bitmap.clear(novasvg::Color::fromValue(value));
        }, "value"_a, "Clear the bitmap with a packed 0xRRGGBBAA value.")
        .def("convert_to_rgba", &novasvg::Bitmap::convertToRGBA, "Convert pixel data from ARGB32 Premultiplied to RGBA Plain.")
        .def("write_to_png", [](const novasvg::Bitmap &bmp, const std::string &filename) {
            return bmp.writeToPng(filename);
        }, "filename"_a, "Write the bitmap to a PNG file.");

    // --- Bind Node Class ---
    nb::class_<novasvg::Node>(m, "Node")
        .def("is_text_node", &novasvg::Node::isTextNode, "Check if the node is a text node.")
        .def("is_element", &novasvg::Node::isElement, "Check if the node is an element node.")
        .def("to_text_node", &novasvg::Node::toTextNode, "Convert the node to a TextNode.")
        .def("to_element", &novasvg::Node::toElement, "Convert the node to an Element.")
        .def("parent_element", &novasvg::Node::parentElement, "Get the parent element.")
        .def("is_null", &novasvg::Node::isNull, "Check if the node is null.")
        .def("__bool__", &novasvg::Node::operator bool, "Check if the node is valid.")
        .def("__eq__", &novasvg::Node::operator==, "Check equality.")
        .def("__ne__", &novasvg::Node::operator!=, "Check inequality.");

    // --- Bind TextNode Class ---
    nb::class_<novasvg::TextNode, novasvg::Node>(m, "TextNode")
        .def(nb::init<>(), "Construct a null text node.")
        .def("data", &novasvg::TextNode::data, nb::rv_policy::reference, "Get the text content.")
        .def("set_data", &novasvg::TextNode::setData, "data"_a, "Set the text content.");

    // --- Bind Element Class ---
    nb::class_<novasvg::Element, novasvg::Node>(m, "Element")
        .def(nb::init<>(), "Construct a null element.")
        .def("has_attribute", &novasvg::Element::hasAttribute, "name"_a, "Check if an attribute exists.")
        .def("get_attribute", &novasvg::Element::getAttribute, "name"_a, nb::rv_policy::reference, "Get an attribute value.")
        .def("set_attribute", &novasvg::Element::setAttribute, "name"_a, "value"_a, "Set an attribute value.")
        .def("render", &novasvg::Element::render, "bitmap"_a, "matrix"_a = novasvg::Matrix(), "Render the element onto a bitmap.")
        .def("render_to_bitmap", &novasvg::Element::renderToBitmap, 
             "width"_a = -1, "height"_a = -1, "backgroundColor"_a = novasvg::Color(),
             "Render the element to a new Bitmap.")
        .def("render_to_bitmap", [](const novasvg::Element& element, int width, int height,
                                    uint32_t background_color) {
            return element.renderToBitmap(width, height, novasvg::Color::fromValue(background_color));
        }, "width"_a = -1, "height"_a = -1, "backgroundColor"_a = 0u,
           "Render the element using a packed 0xRRGGBBAA background color.")
        .def("get_local_matrix", &novasvg::Element::getLocalMatrix, "Get the local transformation matrix.")
        .def("get_global_matrix", &novasvg::Element::getGlobalMatrix, "Get the global transformation matrix.")
        .def("get_local_bounding_box", &novasvg::Element::getLocalBoundingBox, "Get the local bounding box.")
        .def("get_global_bounding_box", &novasvg::Element::getGlobalBoundingBox, "Get the global bounding box.")
        .def("get_bounding_box", &novasvg::Element::getBoundingBox, "Get the bounding box without transformations.")
        .def("children", &novasvg::Element::children, "Get child nodes.");

    // --- Bind Document Class ---
    nb::class_<novasvg::Document>(m, "Document")
        .def_static("load_from_file", &novasvg::Document::loadFromFile, "filename"_a, "Load an SVG document from a file.")
        .def_static("load_from_data", 
            nb::overload_cast<const std::string&>(&novasvg::Document::loadFromData), 
            "string"_a, "Load an SVG document from a string.")
        .def_static("load_from_data", 
            [](const nb::bytes& data) {
                return novasvg::Document::loadFromData(reinterpret_cast<const char*>(data.data()), data.size());
            }, 
            "data"_a, "Load an SVG document from a bytes object.")
        .def("apply_stylesheet", &novasvg::Document::applyStyleSheet, "content"_a, "Apply a CSS stylesheet to the document.")
        .def("query_selector_all", &novasvg::Document::querySelectorAll, "content"_a, "Select elements matching a CSS selector.")
        .def_prop_ro("width", &novasvg::Document::width, "Get the intrinsic width of the document.")
        .def_prop_ro("height", &novasvg::Document::height, "Get the intrinsic height of the document.")
        .def("bounding_box", &novasvg::Document::boundingBox, "Get the bounding box of the document content.")
        .def("update_layout", &novasvg::Document::updateLayout, "Update the layout of the document if needed.")
        .def("force_layout", &novasvg::Document::forceLayout, "Force an immediate layout update.")
        .def("render", &novasvg::Document::render, "bitmap"_a, "matrix"_a = novasvg::Matrix(), "Render the document onto a bitmap.")
        .def("render_to_bitmap", &novasvg::Document::renderToBitmap, 
             "width"_a = -1, "height"_a = -1, "backgroundColor"_a = novasvg::Color(),
             "Render the document to a new Bitmap.")
        .def("render_to_bitmap", [](const novasvg::Document& document, int width, int height,
                                    uint32_t background_color) {
            return document.renderToBitmap(width, height, novasvg::Color::fromValue(background_color));
        }, "width"_a = -1, "height"_a = -1, "backgroundColor"_a = 0u,
           "Render the document using a packed 0xRRGGBBAA background color.")
        .def("element_from_point", &novasvg::Document::elementFromPoint, "x"_a, "y"_a, "Get the topmost element at a specific point.")
        .def("get_element_by_id", &novasvg::Document::getElementById, "id"_a, "Get an element by its ID.")
        .def("document_element", &novasvg::Document::documentElement, "Get the root element of the document.");
}
