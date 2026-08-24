#include "svgelement.h"
#include "svgrenderstate.h"

#include <cstring>
#include <fstream>
#include <cmath>


namespace novasvg {

NOVASVG_INLINE int version()
{
    return NOVASVG_VERSION;
}

NOVASVG_INLINE std::string versionString()
{
    return NOVASVG_VERSION_STRING;
}

NOVASVG_INLINE bool addFontFaceFromFile(const char* family, bool bold, bool italic, const char* filename)
{
    return fontFaceCache()->addFontFace(family, bold, italic, FontFace(filename));
}

NOVASVG_INLINE bool addFontFaceFromData(const char* family, bool bold, bool italic, const void* data, size_t length, novasvg_destroy_func_t destroy_func, void* closure)
{
    return fontFaceCache()->addFontFace(family, bold, italic, FontFace(data, length, destroy_func, closure));
}

NOVASVG_INLINE Bitmap::Bitmap(int width, int height)
    : m_surface(novasvg_surface_create(width, height))
{
}

NOVASVG_INLINE Bitmap::Bitmap(uint8_t* data, int width, int height, int stride)
    : m_surface(novasvg_surface_create_for_data(data, width, height, stride))
{
}

NOVASVG_INLINE Bitmap::Bitmap(const Bitmap& bitmap)
    : m_surface(novasvg_surface_reference(bitmap.surface()))
{
}

NOVASVG_INLINE Bitmap::Bitmap(Bitmap&& bitmap)
    : m_surface(bitmap.release())
{
}

NOVASVG_INLINE Bitmap::~Bitmap()
{
    novasvg_surface_destroy(m_surface);
}

NOVASVG_INLINE Bitmap& Bitmap::operator=(const Bitmap& bitmap)
{
    Bitmap(bitmap).swap(*this);
    return *this;
}

NOVASVG_INLINE void Bitmap::swap(Bitmap& bitmap)
{
    std::swap(m_surface, bitmap.m_surface);
}

NOVASVG_INLINE uint8_t* Bitmap::data() const
{
    if(m_surface)
        return novasvg_surface_get_data(m_surface);
    return nullptr;
}

NOVASVG_INLINE int Bitmap::width() const
{
    if(m_surface)
        return novasvg_surface_get_width(m_surface);
    return 0;
}

NOVASVG_INLINE int Bitmap::height() const
{
    if(m_surface)
        return novasvg_surface_get_height(m_surface);
    return 0;
}

NOVASVG_INLINE int Bitmap::stride() const
{
    if(m_surface)
        return novasvg_surface_get_stride(m_surface);
    return 0;
}

NOVASVG_INLINE void Bitmap::clear(uint32_t value)
{
    if(m_surface == nullptr)
        return;
    novasvg_color_t color;
    novasvg_color_init_rgba32(&color, value);
    novasvg_surface_clear(m_surface, &color);
}

NOVASVG_INLINE void Bitmap::convertToRGBA()
{
    if(m_surface == nullptr)
        return;
    auto data = novasvg_surface_get_data(m_surface);
    auto width = novasvg_surface_get_width(m_surface);
    auto height = novasvg_surface_get_height(m_surface);
    auto stride = novasvg_surface_get_stride(m_surface);
    novasvg_convert_argb_to_rgba(data, data, width, height, stride);
}

NOVASVG_INLINE Bitmap& Bitmap::operator=(Bitmap&& bitmap)
{
    Bitmap(std::move(bitmap)).swap(*this);
    return *this;
}

NOVASVG_INLINE bool Bitmap::writeToPng(const std::string& filename) const
{
    if(m_surface)
        return novasvg_surface_write_to_png(m_surface, filename.data());
    return false;
}

NOVASVG_INLINE bool Bitmap::writeToPng(novasvg_write_func_t callback, void* closure) const
{
    if(m_surface)
        return novasvg_surface_write_to_png_stream(m_surface, callback, closure);
    return false;
}

NOVASVG_INLINE novasvg_surface_t* Bitmap::release()
{
    return std::exchange(m_surface, nullptr);
}

NOVASVG_INLINE Box::Box(float x, float y, float w, float h)
    : x(x), y(y), w(w), h(h)
{
}

NOVASVG_INLINE Box::Box(const Rect& rect)
    : x(rect.x), y(rect.y), w(rect.w), h(rect.h)
{
}

NOVASVG_INLINE Box& Box::transform(const Matrix &matrix)
{
    *this = transformed(matrix);
    return *this;
}

NOVASVG_INLINE Box Box::transformed(const Matrix& matrix) const
{
    return Transform(matrix).mapRect(*this);
}

NOVASVG_INLINE Matrix::Matrix(float a, float b, float c, float d, float e, float f)
    : a(a), b(b), c(c), d(d), e(e), f(f)
{
}

NOVASVG_INLINE Matrix::Matrix(const novasvg_matrix_t& matrix)
    : a(matrix.a), b(matrix.b), c(matrix.c), d(matrix.d), e(matrix.e), f(matrix.f)
{
}

NOVASVG_INLINE Matrix::Matrix(const Transform& transform)
    : Matrix(transform.matrix())
{
}

NOVASVG_INLINE Matrix Matrix::operator*(const Matrix& matrix) const
{
    return Transform(*this) * Transform(matrix);
}

NOVASVG_INLINE Matrix& Matrix::operator*=(const Matrix &matrix)
{
    return (*this = *this * matrix);
}

NOVASVG_INLINE Matrix& Matrix::multiply(const Matrix& matrix)
{
    return (*this *= matrix);
}

NOVASVG_INLINE Matrix& Matrix::scale(float sx, float sy)
{
    return multiply(scaled(sx, sy));
}

NOVASVG_INLINE Matrix& Matrix::translate(float tx, float ty)
{
    return multiply(translated(tx, ty));
}

NOVASVG_INLINE Matrix& Matrix::rotate(float angle, float cx, float cy)
{
    return multiply(rotated(angle, cx, cy));
}

NOVASVG_INLINE Matrix& Matrix::shear(float shx, float shy)
{
    return multiply(sheared(shx, shy));
}

NOVASVG_INLINE Matrix Matrix::inverse() const
{
    return Transform(*this).inverse();
}

NOVASVG_INLINE Matrix& Matrix::invert()
{
    return (*this = inverse());
}

NOVASVG_INLINE void Matrix::reset()
{
    *this = Matrix(1, 0, 0, 1, 0, 0);
}

NOVASVG_INLINE Matrix Matrix::translated(float tx, float ty)
{
    return Transform::translated(tx, ty);
}

NOVASVG_INLINE Matrix Matrix::scaled(float sx, float sy)
{
    return Transform::scaled(sx, sy);
}

NOVASVG_INLINE Matrix Matrix::rotated(float angle, float cx, float cy)
{
    return Transform::rotated(angle, cx, cy);
}

NOVASVG_INLINE Matrix Matrix::sheared(float shx, float shy)
{
    return Transform::sheared(shx, shy);
}

NOVASVG_INLINE Node::Node(SVGNode* node)
    : m_node(node)
{
}

NOVASVG_INLINE bool Node::isTextNode() const
{
    return m_node && m_node->isTextNode();
}

NOVASVG_INLINE bool Node::isElement() const
{
    return m_node && m_node->isElement();
}

NOVASVG_INLINE TextNode Node::toTextNode() const
{
    if(m_node && m_node->isTextNode())
        return static_cast<SVGTextNode*>(m_node);
    return TextNode();
}

NOVASVG_INLINE Element Node::toElement() const
{
    if(m_node && m_node->isElement())
        return static_cast<SVGElement*>(m_node);
    return Element();
}

NOVASVG_INLINE Element Node::parentElement() const
{
    if(m_node)
        return m_node->parentElement();
    return Element();
}

NOVASVG_INLINE TextNode::TextNode(SVGTextNode* text)
    : Node(text)
{
}

NOVASVG_INLINE const std::string& TextNode::data() const
{
    if(m_node)
        return text()->data();
    return emptyString;
}

NOVASVG_INLINE void TextNode::setData(const std::string& data)
{
    if(m_node) {
        text()->setData(data);
    }
}

NOVASVG_INLINE SVGTextNode* TextNode::text() const
{
    return static_cast<SVGTextNode*>(m_node);
}

NOVASVG_INLINE Element::Element(SVGElement* element)
    : Node(element)
{
}

NOVASVG_INLINE bool Element::hasAttribute(const std::string& name) const
{
    if(m_node)
        return element()->hasAttribute(name);
    return false;
}

NOVASVG_INLINE const std::string& Element::getAttribute(const std::string& name) const
{
    if(m_node)
        return element()->getAttribute(name);
    return emptyString;
}

NOVASVG_INLINE void Element::setAttribute(const std::string& name, const std::string& value)
{
    if(m_node) {
        element()->setAttribute(name, value);
    }
}

NOVASVG_INLINE void Element::render(Bitmap& bitmap, const Matrix& matrix) const
{
    if(m_node == nullptr || bitmap.isNull())
        return;
    auto canvas = Canvas::create(bitmap);
    SVGRenderState state(nullptr, nullptr, matrix, SVGRenderMode::Painting, canvas);
    element(true)->render(state);
}

NOVASVG_INLINE Bitmap Element::renderToBitmap(int width, int height, uint32_t backgroundColor) const
{
    if(m_node == nullptr)
        return Bitmap();
    auto elementBounds = element(true)->localTransform().mapRect(element()->paintBoundingBox());
    if(elementBounds.isEmpty())
        return Bitmap();
    if(width <= 0 && height <= 0) {
        width = static_cast<int>(std::ceil(elementBounds.w));
        height = static_cast<int>(std::ceil(elementBounds.h));
    } else if(width > 0 && height <= 0) {
        height = static_cast<int>(std::ceil(width * elementBounds.h / elementBounds.w));
    } else if(height > 0 && width <= 0) {
        width = static_cast<int>(std::ceil(height * elementBounds.w / elementBounds.h));
    }

    auto xScale = width / elementBounds.w;
    auto yScale = height / elementBounds.h;

    Matrix matrix(xScale, 0, 0, yScale, -elementBounds.x * xScale, -elementBounds.y * yScale);
    Bitmap bitmap(width, height);
    if(backgroundColor) bitmap.clear(backgroundColor);
    render(bitmap, matrix);
    return bitmap;
}

NOVASVG_INLINE Matrix Element::getLocalMatrix() const
{
    if(m_node)
        return element(true)->localTransform();
    return Matrix();
}

NOVASVG_INLINE Matrix Element::getGlobalMatrix() const
{
    if(m_node == nullptr)
        return Matrix();
    auto transform = element(true)->localTransform();
    for(auto parent = element()->parentElement(); parent; parent = parent->parentElement())
        transform.postMultiply(parent->localTransform());
    return transform;
}

NOVASVG_INLINE Box Element::getLocalBoundingBox() const
{
    return getBoundingBox().transformed(getLocalMatrix());
}

NOVASVG_INLINE Box Element::getGlobalBoundingBox() const
{
    return getBoundingBox().transformed(getGlobalMatrix());
}

NOVASVG_INLINE Box Element::getBoundingBox() const
{
    if(m_node)
        return element(true)->paintBoundingBox();
    return Box();
}

NOVASVG_INLINE NodeList Element::children() const
{
    if(m_node == nullptr)
        return NodeList();
    NodeList children;
    for(const auto& child : element()->children())
        children.push_back(child.get());
    return children;
}

NOVASVG_INLINE SVGElement* Element::element(bool layoutIfNeeded) const
{
    auto element = static_cast<SVGElement*>(m_node);
    if(element && layoutIfNeeded)
        element->rootElement()->layoutIfNeeded();
    return element;
}

NOVASVG_INLINE std::unique_ptr<Document> Document::loadFromFile(const std::string& filename)
{
    std::ifstream fs;
    fs.open(filename);
    if(!fs.is_open())
        return nullptr;
    std::string content;
    std::getline(fs, content, '\0');
    fs.close();
    return loadFromData(content);
}

NOVASVG_INLINE std::unique_ptr<Document> Document::loadFromData(const std::string& string)
{
    return loadFromData(string.data(), string.size());
}

NOVASVG_INLINE std::unique_ptr<Document> Document::loadFromData(const char* data)
{
    return loadFromData(data, std::strlen(data));
}

NOVASVG_INLINE std::unique_ptr<Document> Document::loadFromData(const char* data, size_t length)
{
    std::unique_ptr<Document> document(new Document);
    if(!document->parse(data, length))
        return nullptr;
    return document;
}

NOVASVG_INLINE float Document::width() const
{
    return rootElement(true)->intrinsicWidth();
}

NOVASVG_INLINE float Document::height() const
{
    return rootElement(true)->intrinsicHeight();
}

NOVASVG_INLINE Box Document::boundingBox() const
{
    return rootElement(true)->localTransform().mapRect(rootElement()->paintBoundingBox());
}

NOVASVG_INLINE void Document::updateLayout()
{
    m_rootElement->layoutIfNeeded();
}

NOVASVG_INLINE void Document::forceLayout()
{
    m_rootElement->forceLayout();
}

NOVASVG_INLINE void Document::render(Bitmap& bitmap, const Matrix& matrix) const
{
    if(bitmap.isNull())
        return;
    auto canvas = Canvas::create(bitmap);
    SVGRenderState state(nullptr, nullptr, matrix, SVGRenderMode::Painting, canvas);
    rootElement(true)->render(state);
}

NOVASVG_INLINE Bitmap Document::renderToBitmap(int width, int height, uint32_t backgroundColor) const
{
    auto intrinsicWidth = rootElement(true)->intrinsicWidth();
    auto intrinsicHeight = rootElement()->intrinsicHeight();
    if(intrinsicWidth == 0.f || intrinsicHeight == 0.f)
        return Bitmap();
    if(width <= 0 && height <= 0) {
        width = static_cast<int>(std::ceil(intrinsicWidth));
        height = static_cast<int>(std::ceil(intrinsicHeight));
    } else if(width > 0 && height <= 0) {
        height = static_cast<int>(std::ceil(width * intrinsicHeight / intrinsicWidth));
    } else if(height > 0 && width <= 0) {
        width = static_cast<int>(std::ceil(height * intrinsicWidth / intrinsicHeight));
    }

    auto xScale = width / intrinsicWidth;
    auto yScale = height / intrinsicHeight;

    Matrix matrix(xScale, 0, 0, yScale, 0, 0);
    Bitmap bitmap(width, height);
    if(backgroundColor) bitmap.clear(backgroundColor);
    render(bitmap, matrix);
    return bitmap;
}

NOVASVG_INLINE Element Document::elementFromPoint(float x, float y) const
{
    return rootElement(true)->elementFromPoint(x, y);
}

NOVASVG_INLINE Element Document::getElementById(const std::string& id) const
{
    return m_rootElement->getElementById(id);
}

NOVASVG_INLINE Element Document::documentElement() const
{
    return m_rootElement.get();
}

NOVASVG_INLINE SVGRootElement* Document::rootElement(bool layoutIfNeeded) const
{
    if(layoutIfNeeded)
        m_rootElement->layoutIfNeeded();
    return m_rootElement.get();
}

NOVASVG_INLINE Document::Document(Document&&) = default;
NOVASVG_INLINE Document& Document::operator=(Document&&) = default;

NOVASVG_INLINE Document::Document() = default;
NOVASVG_INLINE Document::~Document() = default;

} // namespace novasvg
