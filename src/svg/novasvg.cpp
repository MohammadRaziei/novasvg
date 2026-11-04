// Implementation of the public API declared in include/novasvg/novasvg.h.
#include "novasvg/novasvg.h"

// Always included -- see the note above. Defining
// NOVASVG_IMPLEMENTATION is no longer required and has no effect; it is
// kept as a harmless no-op purely so existing code that still defines it
// keeps compiling unchanged.
//
// This used to be a separate "detail/novasvg_impl.h" file, but that file
// did nothing except forward this exact list of includes to the one
// place (here) that used it, so it's inlined directly instead.
//
// The render/ layer's own extern "C" wrapper is gone too: it's pure,
// header-only C++ (/ internal-linkage everywhere), nothing
// in this project links against it from actual C code, and the vendored
// STB headers already manage their own extern "C" internally when
// compiled as C++.

#include <cstring>
#include <fstream>
#include <cmath>

#include "gfx/canvas.h"
#include "gfx/font.h"
#include "svgelement.h"
#include "svgparser.hpp"

namespace novasvg {
using namespace gfx;

// ---------------------------------------------------------------
// Implementation (formerly detail/novasvg.hpp -- merged in since
// this header is fully now, there's no reason left
// to keep the public declarations and their definitions apart).
// ---------------------------------------------------------------

using namespace gfx; // render/ layer (novasvg::render), the former plutovg

int version()
{
    return NOVASVG_VERSION;
}

std::string version_string()
{
    return NOVASVG_VERSION_STRING;
}


Box::Box(float x, float y, float w, float h)
    : x(x), y(y), w(w), h(h)
{
}

Box::Box(const Rect& rect)
    : x(rect.x), y(rect.y), w(rect.w), h(rect.h)
{
}

Box& Box::transform(const Matrix &matrix)
{
    *this = transformed(matrix);
    return *this;
}

Box Box::transformed(const Matrix& matrix) const
{
    return Transform(matrix).map_rect(*this);
}

Rect::Rect(const Box& box)
    : x(box.x), y(box.y), w(box.w), h(box.h)
{
}

Matrix::Matrix(float a, float b, float c, float d, float e, float f)
    : a(a), b(b), c(c), d(d), e(e), f(f)
{
}

Matrix::Matrix(const matrix_t& matrix)
    : a(matrix.a), b(matrix.b), c(matrix.c), d(matrix.d), e(matrix.e), f(matrix.f)
{
}

Matrix::Matrix(const Transform& transform)
    : Matrix(transform.matrix())
{
}

Matrix Matrix::operator*(const Matrix& matrix) const
{
    return Transform(*this) * Transform(matrix);
}

Transform::Transform(const Matrix& matrix)
    : Transform(matrix.a, matrix.b, matrix.c, matrix.d, matrix.e, matrix.f)
{
}

Matrix& Matrix::operator*=(const Matrix &matrix)
{
    return (*this = *this * matrix);
}

Matrix& Matrix::multiply(const Matrix& matrix)
{
    return (*this *= matrix);
}

Matrix& Matrix::scale(float sx, float sy)
{
    return multiply(scaled(sx, sy));
}

Matrix& Matrix::translate(float tx, float ty)
{
    return multiply(translated(tx, ty));
}

Matrix& Matrix::rotate(float angle, float cx, float cy)
{
    return multiply(rotated(angle, cx, cy));
}

Matrix& Matrix::shear(float shx, float shy)
{
    return multiply(sheared(shx, shy));
}

Matrix Matrix::inverse() const
{
    return Transform(*this).inverse();
}

Matrix& Matrix::invert()
{
    return (*this = inverse());
}

void Matrix::reset()
{
    *this = Matrix(1, 0, 0, 1, 0, 0);
}

Matrix Matrix::translated(float tx, float ty)
{
    return Transform::translated(tx, ty);
}

Matrix Matrix::scaled(float sx, float sy)
{
    return Transform::scaled(sx, sy);
}

Matrix Matrix::rotated(float angle, float cx, float cy)
{
    return Transform::rotated(angle, cx, cy);
}

Matrix Matrix::sheared(float shx, float shy)
{
    return Transform::sheared(shx, shy);
}

node::node(svg_node* nd)
    : m_node(nd)
{
}

bool node::is_text_node() const
{
    return m_node && m_node->is_text_node();
}

bool node::is_element() const
{
    return m_node && m_node->is_element();
}

text_node node::to_text_node() const
{
    if(m_node && m_node->is_text_node())
        return static_cast<svg_text_node*>(m_node);
    return text_node();
}

element node::to_element() const
{
    if(m_node && m_node->is_element())
        return static_cast<svg_element*>(m_node);
    return element();
}

element node::parent_element() const
{
    if(m_node)
        return m_node->parent_element();
    return element();
}

text_node::text_node(svg_text_node* text)
    : node(text)
{
}

const std::string& text_node::data() const
{
    if(m_node)
        return text()->data();
    return emptyString;
}

void text_node::set_data(const std::string& data)
{
    if(m_node) {
        text()->set_data(data);
    }
}

svg_text_node* text_node::text() const
{
    return static_cast<svg_text_node*>(m_node);
}

element::element(svg_element* el)
    : node(el)
{
}

bool element::has_attribute(const std::string& name) const
{
    if(m_node)
        return el()->has_attribute(name);
    return false;
}

const std::string& element::attribute(const std::string& name) const
{
    if(m_node)
        return el()->attribute(name);
    return emptyString;
}

void element::set_attribute(const std::string& name, const std::string& value)
{
    if(m_node) {
        el()->set_attribute(name, value);
    }
}

void element::render(Bitmap& bitmap, const Matrix& matrix) const
{
    if(m_node == nullptr || bitmap.is_null())
        return;
    auto canvas = raster_canvas::create(bitmap);
    svg_render_state state(nullptr, nullptr, matrix, svg_render_mode::Painting, canvas);
    el(true)->render(state);
}

Bitmap element::render_to_bitmap(int width, int height, const Color& backgroundColor) const
{
    if(m_node == nullptr)
        return Bitmap();
    auto elementBounds = el(true)->local_transform().map_rect(el()->paint_bounding_box());
    if(elementBounds.empty())
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
    if(backgroundColor.value()) bitmap.clear(backgroundColor);
    render(bitmap, matrix);
    return bitmap;
}

void element::render(stb::image& image, const Matrix& matrix) const
{
    if(m_node == nullptr || image.empty())
        return;
    Bitmap layer(image.width(), image.height());  // transparent
    if(layer.is_null())
        return;
    render(layer, matrix);
    stb::image over = bitmap_release_to_image(std::move(layer));
    if(!over.empty())
        image.blend(over);
}

stb::image element::render_to_image(int width, int height, const Color& backgroundColor) const
{
    return bitmap_release_to_image(render_to_bitmap(width, height, backgroundColor));
}

Matrix element::local_matrix() const
{
    if(m_node)
        return el(true)->local_transform();
    return Matrix();
}

Matrix element::global_matrix() const
{
    if(m_node == nullptr)
        return Matrix();
    auto transform = el(true)->local_transform();
    for(auto parent = el()->parent_element(); parent; parent = parent->parent_element())
        transform.post_multiply(parent->local_transform());
    return transform;
}

Box element::local_bounding_box() const
{
    return bounding_box().transformed(local_matrix());
}

Box element::global_bounding_box() const
{
    return bounding_box().transformed(global_matrix());
}

Box element::bounding_box() const
{
    if(m_node)
        return el(true)->paint_bounding_box();
    return Box();
}

NodeList element::children() const
{
    if(m_node == nullptr)
        return NodeList();
    NodeList children;
    for(const auto& child : el()->children())
        children.push_back(child.get());
    return children;
}

svg_element* element::el(bool layout_if_needed) const
{
    auto el = static_cast<svg_element*>(m_node);
    if(el && layout_if_needed)
        el->root_element()->layout_if_needed();
    return el;
}

std::unique_ptr<document> document::load_file(const std::string& filename)
{
    std::ifstream fs;
    fs.open(filename);
    if(!fs.is_open())
        return nullptr;
    std::string content;
    std::getline(fs, content, '\0');
    fs.close();
    return load_data(content);
}

std::unique_ptr<document> document::load_data(const std::string& string)
{
    return load_data(string.data(), string.size());
}

std::unique_ptr<document> document::load_data(const char* data)
{
    return load_data(data, std::strlen(data));
}

std::unique_ptr<document> document::load_data(const char* data, size_t length)
{
    std::unique_ptr<document> dom(new document);
    if(!dom->parse(data, length))
        return nullptr;
    return dom;
}

float document::width() const
{
    return root_element(true)->intrinsic_width();
}

float document::height() const
{
    return root_element(true)->intrinsic_height();
}

Box document::bounding_box() const
{
    return root_element(true)->local_transform().map_rect(root_element()->paint_bounding_box());
}

void document::update_layout()
{
    m_rootElement->layout_if_needed();
}

void document::force_layout()
{
    m_rootElement->force_layout();
}

void document::render(Bitmap& bitmap, const Matrix& matrix) const
{
    if(bitmap.is_null())
        return;

    // A `background-color` in the root <svg>'s own inline style isn't a
    // real svg presentation property -- it's a CSS box-model property
    // that only means something when an svg is displayed as a standalone
    // dom/replaced el (which is exactly how a browser navigated
    // straight to a .svg file treats it, painting that color behind
    // everything). Most pure-vector svg renderers ignore it entirely, but
    // since generated svgs (Mermaid in particular) rely on it for a
    // correct-looking standalone render, honor it here the same way:
    // clear the bitmap to it before any content paints, so it sits under
    // everything and behind any transparency in the content itself. Takes
    // priority over render_to_bitmap()'s own `backgroundColor` argument
    // (this runs after that clear) since that argument means "what to
    // show through transparent content", and content here isn't actually
    // transparent -- it explicitly asked for this color.
    // A `background-color` in the root <svg>'s own style isn't a real svg
    // presentation property -- it's a CSS box-model property that only
    // means something when an svg is displayed as a standalone
    // dom/replaced el (exactly how a browser navigated straight
    // to a .svg file treats it, painting that color behind everything).
    // Most pure-vector svg renderers ignore it entirely, but since
    // generated svgs (Mermaid in particular) rely on it for a
    // correct-looking standalone render, honor it here the same way:
    // clear the bitmap to it before any content paints, so it sits under
    // everything and behind any transparency in the content itself. Takes
    // priority over render_to_bitmap()'s own `backgroundColor` argument
    // (this runs after that clear) since that argument means "what to
    // show through transparent content", and content here isn't actually
    // transparent -- it explicitly asked for this color.
    if(const auto& bg = root_element(true)->attribute(PropertyID::Background_Color); !bg.empty()) {
        color_t color;
        if(color_parse(&color, bg.data(), static_cast<int>(bg.length())) > 0) {
            auto argb = color_to_argb32(&color);
            // A fully transparent `background-color` (Mermaid emits `transparent`) paints nothing, so it must
            // not wipe the caller's `backgroundColor` argument -- that made every PNG come out transparent.
            // ponytail: a *semi*-transparent root color still replaces the argument instead of compositing
            // over it; upgrade path is to fill a rect through the canvas instead of clear().
            if((argb >> 24) & 0xff)
                bitmap.clear(Color((argb >> 16) & 0xff, (argb >> 8) & 0xff, argb & 0xff, (argb >> 24) & 0xff));
        }
    }

    auto canvas = raster_canvas::create(bitmap);
    svg_render_state state(nullptr, nullptr, matrix, svg_render_mode::Painting, canvas);
    root_element(true)->render(state);
}

Bitmap document::render_to_bitmap(int width, int height, const Color& backgroundColor) const
{
    auto intrinsic_width = root_element(true)->intrinsic_width();
    auto intrinsic_height = root_element()->intrinsic_height();
    if(intrinsic_width == 0.f || intrinsic_height == 0.f)
        return Bitmap();
    if(width <= 0 && height <= 0) {
        width = static_cast<int>(std::ceil(intrinsic_width));
        height = static_cast<int>(std::ceil(intrinsic_height));
    } else if(width > 0 && height <= 0) {
        height = static_cast<int>(std::ceil(width * intrinsic_height / intrinsic_width));
    } else if(height > 0 && width <= 0) {
        width = static_cast<int>(std::ceil(height * intrinsic_width / intrinsic_height));
    }

    auto xScale = width / intrinsic_width;
    auto yScale = height / intrinsic_height;

    Matrix matrix(xScale, 0, 0, yScale, 0, 0);
    Bitmap bitmap(width, height);
    if(backgroundColor.value()) bitmap.clear(backgroundColor);
    render(bitmap, matrix);
    return bitmap;
}

void document::render(stb::image& image, const Matrix& matrix) const
{
    if(image.empty())
        return;
    Bitmap layer(image.width(), image.height());  // transparent
    if(layer.is_null())
        return;
    render(layer, matrix);
    stb::image over = bitmap_release_to_image(std::move(layer));
    if(!over.empty())
        image.blend(over);
}

stb::image document::render_to_image(int width, int height, const Color& backgroundColor) const
{
    return bitmap_release_to_image(render_to_bitmap(width, height, backgroundColor));
}

element document::element_from_point(float x, float y) const
{
    return root_element(true)->element_from_point(x, y);
}

element document::element_by_id(const std::string& id) const
{
    return m_rootElement->element_by_id(id);
}

element document::document_element() const
{
    return m_rootElement.get();
}

svg_root_element* document::root_element(bool layout_if_needed) const
{
    if(layout_if_needed)
        m_rootElement->layout_if_needed();
    return m_rootElement.get();
}

document::document(document&&) = default;
document& document::operator=(document&&) = default;

document::document() = default;
document::~document() = default;

} // namespace novasvg

// ---- definitions of public-class members that used to sit in svgelement.h / svgparser.hpp ----
namespace novasvg {
using namespace gfx;

bool document::parse(const char* data, size_t length)
{
    std::string buffer;
    std::string styleSheet;
    svg_element* currentElement = nullptr;
    int ignoring = 0;
    auto handleText = [&](std::string_view text, bool in_cdata) {
        if(text.empty() || currentElement == nullptr || ignoring > 0)
            return;
        if(currentElement->id() != ElementID::Text && currentElement->id() != ElementID::Tspan && currentElement->id() != ElementID::Style) {
            return;
        }

        if(in_cdata) {
            buffer.assign(text);
        } else {
            decodeText(text, buffer);
        }

        if(currentElement->id() == ElementID::Style) {
            removeStyleComments(buffer);
            styleSheet.append(buffer);
        } else {
            auto nd = std::make_unique<svg_text_node>(this);
            nd->set_data(buffer);
            currentElement->addChild(std::move(nd));
        }
    };

    std::string_view input(data, length);
    if(length >= 3) {
        auto buffer = (const uint8_t*)(data);

        const auto c1 = buffer[0];
        const auto c2 = buffer[1];
        const auto c3 = buffer[2];
        if(c1 == 0xEF && c2 == 0xBB && c3 == 0xBF) {
            input.remove_prefix(3);
        }
    }

    while(!input.empty()) {
        if(currentElement) {
            auto text = input.substr(0, input.find('<'));
            handleText(text, false);
            input.remove_prefix(text.length());
        } else {
            if(!skipOptionalSpaces(input)) {
                break;
            }
        }

        if(!skipDelimiter(input, '<'))
            return false;
        if(skipDelimiter(input, '?')) {
            if(!readIdentifier(input, buffer))
                return false;
            auto n = input.find("?>");
            if(n == std::string_view::npos)
                return false;
            input.remove_prefix(n + 2);
            continue;
        }

        if(skipDelimiter(input, '!')) {
            if(skipString(input, "--")) {
                auto n = input.find("-->");
                if(n == std::string_view::npos)
                    return false;
                handleText(input.substr(0, n), false);
                input.remove_prefix(n + 3);
                continue;
            }

            if(skipString(input, "[CDATA[")) {
                auto n = input.find("]]>");
                if(n == std::string_view::npos)
                    return false;
                handleText(input.substr(0, n), true);
                input.remove_prefix(n + 3);
                continue;
            }

            if(skipString(input, "DOCTYPE")) {
                while(!input.empty() && input.front() != '>') {
                    if(input.front() == '[') {
                        int depth = 1;
                        input.remove_prefix(1);
                        while(!input.empty() && depth > 0) {
                            if(input.front() == '[') ++depth;
                            else if(input.front() == ']') --depth;
                            input.remove_prefix(1);
                        }
                    } else {
                        input.remove_prefix(1);
                    }
                }

                if(!skipDelimiter(input, '>'))
                    return false;
                continue;
            }

            return false;
        }

        if(skipDelimiter(input, '/')) {
            if(currentElement == nullptr && ignoring == 0)
                return false;
            if(!readIdentifier(input, buffer))
                return false;
            if(ignoring == 0) {
                auto id = elementid(buffer);
                if(id != currentElement->id())
                    return false;
                currentElement = currentElement->parent_element();
            } else {
                --ignoring;
            }

            skipOptionalSpaces(input);
            if(!skipDelimiter(input, '>'))
                return false;
            continue;
        }

        if(!readIdentifier(input, buffer))
            return false;
        svg_element* el = nullptr;
        if(ignoring > 0) {
            ++ignoring;
        } else {
            auto id = elementid(buffer);
            if(id == ElementID::Unknown) {
                ignoring = 1;
            } else {
                if(m_rootElement && currentElement == nullptr)
                    return false;
                if(m_rootElement == nullptr) {
                    if(id != ElementID::Svg)
                        return false;
                    m_rootElement = std::make_unique<svg_root_element>(this);
                    el = m_rootElement.get();
                } else {
                    auto child = svg_element::create(this, id);
                    el = child.get();
                    currentElement->addChild(std::move(child));
                }
            }
        }

        skipOptionalSpaces(input);
        while(readIdentifier(input, buffer)) {
            skipOptionalSpaces(input);
            if(!skipDelimiter(input, '='))
                return false;
            skipOptionalSpaces(input);
            if(input.empty() || !(input.front() == '\"' || input.front() == '\''))
                return false;
            auto quote = input.front();
            input.remove_prefix(1);
            auto n = input.find(quote);
            if(n == std::string_view::npos)
                return false;
            auto id = PropertyID::Unknown;
            if(el != nullptr)
                id = propertyid(buffer);
            if(id != PropertyID::Unknown) {
                decodeText(input.substr(0, n), buffer);
                if(id == PropertyID::Style) {
                    removeStyleComments(buffer);
                    parseInlineStyle(buffer, el);
                } else {
                    if(id == PropertyID::Id)
                        m_rootElement->addElementById(buffer, el);
                    el->set_attribute(Specificity::PresentationAttribute, id, buffer);
                }
            }

            input.remove_prefix(n + 1);
            skipOptionalSpaces(input);
        }

        if(skipDelimiter(input, '>')) {
            if(el != nullptr) {
                if(el->id() == ElementID::ForeignObject) {
                    // foreignObject content is arbitrary HTML/XML, not svg --
                    // capture it verbatim instead of trying (and failing) to
                    // parse div/span/p/etc. as svg elements. Mirrors the
                    // CDATA/comment handling above: find the matching close
                    // tag and skip straight past it.
                    static constexpr std::string_view closeTag = "</foreignObject>";
                    auto n = input.find(closeTag);
                    if(n == std::string_view::npos)
                        return false;
                    static_cast<svg_foreign_object_element*>(el)->setRawContent(std::string(input.substr(0, n)));
                    input.remove_prefix(n + closeTag.length());
                } else {
                    currentElement = el;
                }
            }
            continue;
        }

        if(skipDelimiter(input, '/')) {
            if(!skipDelimiter(input, '>'))
                return false;
            if(ignoring > 0)
                --ignoring;
            continue;
        }

        return false;
    }

    if(m_rootElement == nullptr || ignoring > 0 || !input.empty())
        return false;
    apply_style_sheet(styleSheet);
    m_rootElement->build();
    return true;
}

void document::apply_style_sheet(const std::string& content)
{
    auto rules = parseStyleSheet(content);
    std::sort(rules.begin(), rules.end());
    if(!rules.empty()) {
        const RuleIndex index(rules);
        std::vector<uint32_t> candidates;
        m_rootElement->transverse([&](svg_element* el) {
            index.candidates(el, candidates);
            for(auto i : candidates) {
                const auto& rule = rules[i];
                if(rule.match(el)) {
                    for(const auto& declaration : rule.declarations()) {
                        if(declaration.id != PropertyID::Unknown)
                            el->set_attribute(declaration.specificity, declaration.id, declaration.value);
                        else if(isCustomPropertyName(declaration.name))
                            el->setCustomProperty(declaration.specificity, declaration.name, declaration.value);
                    }
                }
            }
        });
    }

    // Every custom property is known now (the elements' own `style`, and every sheet so far): the values that
    // use var() are given theirs.
    m_rootElement->transverse([](svg_element* el) { el->resolveVariables(); });

    if(!content.empty())
        m_rootElement->addStyleSheet(content);

    // The HTML in a foreignObject is not part of the el tree above, so it is styled here, from every
    // sheet applied so far (a stylesheet given later than the one in the file sits after it in the cascade).
    // Built only if there is a foreignObject to style; and when `content` is the only sheet so far those
    // rules are exactly the ones just parsed and sorted, so they are not parsed a second time.
    RuleDataList htmlRules;
    bool haveHtmlRules = false;
    m_rootElement->transverse([&](svg_element* el) {
        if(el->id() != ElementID::ForeignObject)
            return;
        if(!haveHtmlRules) {
            const auto& sheets = m_rootElement->styleSheets();
            if(!content.empty() && sheets.size() == 1)
                htmlRules = std::move(rules);
            else
                htmlRules = parseStyleSheets(sheets);
            haveHtmlRules = true;
        }

        resolveForeignObjectStyle(static_cast<svg_foreign_object_element*>(el), htmlRules);
    });
}

ElementList document::query_selector_all(const std::string& content) const
{
    auto selectors = parseQuerySelectors(content);
    if(selectors.empty())
        return ElementList();
    ElementList elements;
    m_rootElement->transverse([&](svg_element* el) {
        for(const auto& selector : selectors) {
            if(matchSelector(selector, el)) {
                elements.push_back(el);
                break;
            }
        }
    });

    return elements;
}

} // namespace novasvg
