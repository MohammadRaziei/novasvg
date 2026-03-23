#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/ndarray.h>
#include <cstdint>

// Include the main library header
#define NOVASVG_IMPLEMENTATION
#include "novasvg/novasvg.h"

namespace nb = nanobind;
using namespace nb::literals;

namespace novasvg {

// Helper struct to bind static utility functions
struct StaticUtils {
    static int version() { return novasvg::version(); }
    static std::string_view versionString() { return novasvg::versionString(); }
    static bool addFontFaceFromFile(const char* family, bool bold, bool italic, const char* filename) {
        return novasvg::addFontFaceFromFile(family, bold, italic, filename);
    }
    // Note: Binding addFontFaceFromData with callbacks requires complex handling.
    // This is a simplified binding assuming direct data usage or custom capsule handling.
    static bool addFontFaceFromData(const char* family, bool bold, bool italic, nb::bytes data, novasvg_destroy_func_t destroy_func, void* closure) {
        return novasvg::addFontFaceFromData(family, bold, italic, data.data(), data.size(), destroy_func, closure);
    }
};

} // namespace novasvg

NB_MODULE(novasvg_py, m) {
    m.doc() = "Python bindings for NovaSVG using nanobind";

    // --- Bind Static Utility Functions ---
    m.def("version_string", &novasvg::StaticUtils::versionString, "Get the library version as a string (e.g., '0.1.0').");
    m.def("add_font_face_from_file", &novasvg::StaticUtils::addFontFaceFromFile, 
          "family"_a, "bold"_a, "italic"_a, "filename"_a,
          "Add a font face from a file to the cache.");
    
    // Binding for adding font from memory. 
    // The destroy_func and closure are advanced usage; usually defaults to nullptr for simple bindings.
    m.def("add_font_face_from_data", &novasvg::StaticUtils::addFontFaceFromData, 
          "family"_a, "bold"_a, "italic"_a, "data"_a, "destroy_func"_a = nullptr, "closure"_a = nullptr,
          "Add a font face from a memory buffer.");

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
        .def("to_numpy", [](const novasvg::Bitmap &bmp) {
            if (bmp.isNull()) return nb::ndarray<nb::numpy, uint8_t>();
            // Shape: {height, width, 4} for ARGB32
            return nb::ndarray<nb::numpy, uint8_t>(bmp.data(), {bmp.height(), bmp.width(), 4});
        }, nb::rv_policy::reference_internal, "Get pixel data as a NumPy array.")
        .def_prop_ro("width", &novasvg::Bitmap::width, "Get the width of the bitmap.")
        .def_prop_ro("height", &novasvg::Bitmap::height, "Get the height of the bitmap.")
        .def_prop_ro("stride", &novasvg::Bitmap::stride, "Get the stride (bytes per row) of the bitmap.")
        .def("is_null", &novasvg::Bitmap::isNull, "Check if the bitmap is null.")
        .def("clear", &novasvg::Bitmap::clear, "value"_a, "Clear the bitmap with a specific color (0xRRGGBBAA).")
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
             "width"_a = -1, "height"_a = -1, "backgroundColor"_a = 0x00000000,
             "Render the element to a new Bitmap.")
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
            nb::overload_cast<const char*>(&novasvg::Document::loadFromData), 
            "data"_a, "Load an SVG document from a null-terminated char array.")
        .def_static("load_from_data", 
            nb::overload_cast<const char*, size_t>(&novasvg::Document::loadFromData), 
            "data"_a, "length"_a, "Load an SVG document from a char array with length.")
        .def("apply_stylesheet", &novasvg::Document::applyStyleSheet, "content"_a, "Apply a CSS stylesheet to the document.")
        .def("query_selector_all", &novasvg::Document::querySelectorAll, "content"_a, "Select elements matching a CSS selector.")
        .def("width", &novasvg::Document::width, "Get the intrinsic width of the document.")
        .def("height", &novasvg::Document::height, "Get the intrinsic height of the document.")
        .def("bounding_box", &novasvg::Document::boundingBox, "Get the bounding box of the document content.")
        .def("update_layout", &novasvg::Document::updateLayout, "Update the layout of the document if needed.")
        .def("force_layout", &novasvg::Document::forceLayout, "Force an immediate layout update.")
        .def("render", &novasvg::Document::render, "bitmap"_a, "matrix"_a = novasvg::Matrix(), "Render the document onto a bitmap.")
        .def("render_to_bitmap", &novasvg::Document::renderToBitmap, 
             "width"_a = -1, "height"_a = -1, "backgroundColor"_a = 0x00000000,
             "Render the document to a new Bitmap.")
        .def("element_from_point", &novasvg::Document::elementFromPoint, "x"_a, "y"_a, "Get the topmost element at a specific point.")
        .def("get_element_by_id", &novasvg::Document::getElementById, "id"_a, "Get an element by its ID.")
        .def("document_element", &novasvg::Document::documentElement, "Get the root element of the document.");
}