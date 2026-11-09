// Implementation of the public API declared in include/nova/svg/svg.h.
#include "vg/pixmap.h"
#include "nova/svg/svg.h"

// Always included -- see the note above. Defining
// NOVA_IMPLEMENTATION is no longer required and has no effect; it is
// kept as a harmless no-op purely so existing code that still defines it
// keeps compiling unchanged.
//
// This used to be a separate "detail/novasvg_impl.h" file, but that file
// did nothing except forward this exact list of includes to the one
// place (here) that used it, so it's inlined directly instead.
//
// The render/ layer's own extern "C" wrapper is gone too: it's pure,
// header-only C++ (/ internal-linkage everywhere), nothing
// in this project links against it from actual C code, and the
// STB headers already manage their own extern "C" internally when
// compiled as C++.

#include <cstring>
#include <fstream>
#include <cmath>

#include "vg/canvas.h"
#include "vg/font.h"
#include "vg/pdf_canvas.h"
#include <stb/logging.hpp>
#include "svgelement.h"
#include "svgparser.hpp"

namespace nova::svg {
namespace {
stb::logging::logger& svg_log() { static auto& l = stb::logging::get_logger("novasvg.svg"); return l; }
stb::logging::logger& pdf_log() { static auto& l = stb::logging::get_logger("novasvg.pdf"); return l; }
}  // namespace
using namespace vg;

// ---------------------------------------------------------------
// Implementation (formerly detail/novasvg.hpp -- merged in since
// this header is fully now, there's no reason left
// to keep the public declarations and their definitions apart).
// ---------------------------------------------------------------

using namespace vg; // the drawing layer (nova::vg), the former plutovg

int version()
{
    return NOVA_VERSION;
}

std::string version_string()
{
    return NOVA_VERSION_STRING;
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
    return empty_string;
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
    return empty_string;
}

void element::set_attribute(const std::string& name, const std::string& value)
{
    if(m_node) {
        el()->set_attribute(name, value);
    }
}

void element::render(pixmap& bitmap, const stb::matrix& matrix) const
{
    if(m_node == nullptr || bitmap.is_null())
        return;
    auto canvas = raster_canvas::create(bitmap);
    svg_render_state state(nullptr, nullptr, matrix, svg_render_mode::painting, canvas);
    el(true)->render(state);
}

pixmap element::render_to_pixmap(int width, int height, const stb::color& background_color) const
{
    if(m_node == nullptr)
        return pixmap();
    auto element_bounds = el(true)->local_transform().map_rect(el()->paint_bounding_box());
    if(element_bounds.empty())
        return pixmap();
    if(width <= 0 && height <= 0) {
        width = static_cast<int>(std::ceil(element_bounds.w));
        height = static_cast<int>(std::ceil(element_bounds.h));
    } else if(width > 0 && height <= 0) {
        height = static_cast<int>(std::ceil(width * element_bounds.h / element_bounds.w));
    } else if(height > 0 && width <= 0) {
        width = static_cast<int>(std::ceil(height * element_bounds.w / element_bounds.h));
    }

    auto x_scale = width / element_bounds.w;
    auto y_scale = height / element_bounds.h;

    stb::matrix matrix{x_scale, 0, 0, y_scale, -element_bounds.x * x_scale, -element_bounds.y * y_scale};
    pixmap bitmap(width, height);
    if(background_color.value()) bitmap.clear(background_color);
    render(bitmap, matrix);
    return bitmap;
}

void element::render(stb::image& image, const stb::matrix& matrix) const
{
    if(m_node == nullptr || image.empty())
        return;
    pixmap layer(image.width(), image.height());  // transparent
    if(layer.is_null())
        return;
    render(layer, matrix);
    stb::image over = pixmap_release_to_image(std::move(layer));
    if(!over.empty())
        image.blend(over);
}

stb::image element::render_to_image(int width, int height, const stb::color& background_color) const
{
    return pixmap_release_to_image(render_to_pixmap(width, height, background_color));
}

stb::matrix element::local_matrix() const
{
    if(m_node)
        return el(true)->local_transform().matrix();
    return stb::matrix();
}

stb::matrix element::global_matrix() const
{
    if(m_node == nullptr)
        return stb::matrix();
    auto transform_ = el(true)->local_transform();
    for(auto parent = el()->parent_element(); parent; parent = parent->parent_element())
        transform_.post_multiply(parent->local_transform());
    return transform_.matrix();
}

box element::local_bounding_box() const
{
    return bounding_box().transformed(local_matrix());
}

box element::global_bounding_box() const
{
    return bounding_box().transformed(global_matrix());
}

box element::bounding_box() const
{
    if(m_node)
        return el(true)->paint_bounding_box();
    return box();
}

node_list element::children() const
{
    if(m_node == nullptr)
        return node_list();
    node_list children;
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
    if(!fs.is_open()) {
        svg_log().error("cannot open '%s'", filename.c_str());
        return nullptr;
    }
    svg_log().debug("loading '%s'", filename.c_str());
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

std::unique_ptr<document> document::load_data(const char* data, size_t length_)
{
    std::unique_ptr<document> dom(new document);
    if(!dom->parse(data, length_)) {
        svg_log().error("cannot parse the document (%zu bytes)", length_);
        return nullptr;
    }
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

box document::bounding_box() const
{
    return root_element(true)->local_transform().map_rect(root_element()->paint_bounding_box());
}

void document::update_layout()
{
    m_root_element->layout_if_needed();
}

void document::force_layout()
{
    m_root_element->force_layout();
}

void document::render(pixmap& bitmap, const stb::matrix& matrix) const
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
    // priority over render_to_pixmap()'s own `backgroundColor` argument
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
    // priority over render_to_pixmap()'s own `backgroundColor` argument
    // (this runs after that clear) since that argument means "what to
    // show through transparent content", and content here isn't actually
    // transparent -- it explicitly asked for this color.
    if(const auto& bg = root_element(true)->attribute(property_id::background_color); !bg.empty()) {
        if(const auto parsed = stb::color::try_parse(bg)) {
            const stb::color color_ = *parsed;
            // A fully transparent `background-color` (Mermaid emits `transparent`) paints nothing, so it must
            // not wipe the caller's `backgroundColor` argument -- that made every PNG come out transparent.
            // ponytail: a *semi*-transparent root color still replaces the argument instead of compositing
            // over it; upgrade path is to fill a rect through the canvas instead of clear().
            if(color_.a() > 0)
                bitmap.clear(color_);
        }
    }

    auto canvas = raster_canvas::create(bitmap);
    svg_render_state state(nullptr, nullptr, matrix, svg_render_mode::painting, canvas);
    root_element(true)->render(state);
}

pixmap document::render_to_pixmap(int width, int height, const stb::color& background_color) const
{
    auto intrinsic_width = root_element(true)->intrinsic_width();
    auto intrinsic_height = root_element()->intrinsic_height();
    if(intrinsic_width == 0.f || intrinsic_height == 0.f)
        return pixmap();
    if(width <= 0 && height <= 0) {
        width = static_cast<int>(std::ceil(intrinsic_width));
        height = static_cast<int>(std::ceil(intrinsic_height));
    } else if(width > 0 && height <= 0) {
        height = static_cast<int>(std::ceil(width * intrinsic_height / intrinsic_width));
    } else if(height > 0 && width <= 0) {
        width = static_cast<int>(std::ceil(height * intrinsic_width / intrinsic_height));
    }

    auto x_scale = width / intrinsic_width;
    auto y_scale = height / intrinsic_height;

    stb::matrix matrix{x_scale, 0, 0, y_scale, 0, 0};
    pixmap bitmap(width, height);
    if(background_color.value()) bitmap.clear(background_color);
    render(bitmap, matrix);
    return bitmap;
}

std::vector<uint8_t> document::render_to_pdf(pdf_mode mode, float dpi) const
{
    const float width = root_element(true)->intrinsic_width();
    const float height = root_element()->intrinsic_height();
    if(!(width > 0.f) || !(height > 0.f)) {
        pdf_log().error("cannot render to PDF: the document has no size (%g x %g)", width, height);
        return {};
    }
    pdf_log().debug("rendering %g x %g px to PDF, mode %s, %g dpi", width, height, mode == pdf_mode::vector ? "vector" : "raster", dpi);
    if(!(dpi > 0.f))
        dpi = 192.f;
    const float scale = dpi / 96.f;

    auto pdf = pdf_canvas::create(width, height);
    pdf->set_raster_scale(scale);
    if(mode == pdf_mode::raster) {
        const int pixel_width = static_cast<int>(std::ceil(width * scale));
        const int pixel_height = static_cast<int>(std::ceil(height * scale));
        pixmap bitmap = render_to_pixmap(pixel_width, pixel_height);
        if(bitmap.is_null())
            return {};
        pdf->draw_image(bitmap, rect(0, 0, width, height), rect(0, 0, float(pixel_width), float(pixel_height)), transform());
        return pdf->data();
    }

    // Same rule as render(pixmap&): a root `background-color` sits under everything.
    if(const auto& bg = root_element(true)->attribute(property_id::background_color); !bg.empty()) {
        if(const auto parsed = stb::color::try_parse(bg)) {
            const stb::color color_ = *parsed;
            if(color_.a() > 0) {
                stb::outline page;
                page.add_rect(0, 0, width, height);
                pdf->set_color(red_f(color_), green_f(color_), blue_f(color_), alpha_f(color_));
                pdf->fill_path(page, fill_rule::non_zero, transform());
            }
        }
    }

    svg_render_state state(nullptr, nullptr, transform(), svg_render_mode::painting, pdf);
    root_element(true)->render(state);
    return pdf->data();
}

bool document::save_pdf(const std::string& filename, pdf_mode mode, float dpi) const
{
    auto bytes = render_to_pdf(mode, dpi);
    if(bytes.empty())
        return false;
    std::ofstream out(filename, std::ios::binary);
    if(!out)
        return false;
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(out);
}

void document::render(stb::image& image, const stb::matrix& matrix) const
{
    if(image.empty())
        return;
    pixmap layer(image.width(), image.height());  // transparent
    if(layer.is_null())
        return;
    render(layer, matrix);
    stb::image over = pixmap_release_to_image(std::move(layer));
    if(!over.empty())
        image.blend(over);
}

stb::image document::render_to_image(int width, int height, const stb::color& background_color) const
{
    return pixmap_release_to_image(render_to_pixmap(width, height, background_color));
}

element document::element_from_point(float x, float y) const
{
    return root_element(true)->element_from_point(x, y);
}

element document::element_by_id(const std::string& id) const
{
    return m_root_element->element_by_id(id);
}

element document::document_element() const
{
    return m_root_element.get();
}

svg_root_element* document::root_element(bool layout_if_needed) const
{
    if(layout_if_needed)
        m_root_element->layout_if_needed();
    return m_root_element.get();
}

document::document(document&&) = default;
document& document::operator=(document&&) = default;

document::document() = default;
document::~document() = default;

} // namespace nova::svg

namespace nova::svg {
using namespace vg;

// ---- foreignObject: the document's CSS, as a renderer may ask for it --------------------------------

namespace {

// css_query over the document's sorted rules, for the markup inside one foreignObject.
class rule_css_query final : public css_query {
public:
    rule_css_query(const rule_data_list& rules, const svg_element* owner) : m_rules(rules), m_owner(owner) {}

    std::optional<css_declaration> declared(const std::vector<html_tag>& tags, int index, std::string_view property) const override
    {
        if(index < 0 || size_t(index) >= tags.size())
            return std::nullopt;
        std::vector<html_node> nodes(tags.size());
        for(size_t i = 0; i < tags.size(); ++i) {
            nodes[i].tag = tags[i].tag;
            nodes[i].id = tags[i].id;
            nodes[i].class_name = tags[i].class_names;
            nodes[i].style = tags[i].style;
            nodes[i].parent = tags[i].parent;
        }

        std::optional<css_declaration> best;
        for(const auto& rule : m_rules) {
            const std::string* wanted = nullptr;
            for(const auto& declaration : rule.declarations()) {
                if(declaration.name == property)
                    wanted = &declaration.name;
            }

            if(wanted == nullptr || !match_html_selector(rule.selector(), nodes, index, m_owner))
                continue;
            for(const auto& declaration : rule.declarations()) {
                if(declaration.name != property)
                    continue;
                if(!best || declaration.specificity >= best->specificity)
                    best = css_declaration{declaration.value, declaration.specificity};
            }
        }

        return best;
    }

private:
    const rule_data_list& m_rules;
    const svg_element* m_owner;
};

void prepare_foreign_object(svg_foreign_object_element* el, const foreign_object_renderer* renderer, const rule_data_list& rules)
{
    if(renderer == nullptr) {
        el->set_prepared(nullptr);
        return;
    }

    rule_css_query css(rules, el);
    el->set_prepared(renderer->prepare(el->raw_content(), css));
}

} // namespace

} // namespace nova::svg

// ---- definitions of public-class members that used to sit in svgelement.h / svgparser.hpp ----
namespace nova::svg {
using namespace vg;

bool document::parse(const char* data, size_t length_)
{
    std::string buffer;
    std::string style_sheet;
    svg_element* current_element = nullptr;
    int ignoring = 0;
    auto handle_text = [&](std::string_view text, bool in_cdata) {
        if(text.empty() || current_element == nullptr || ignoring > 0)
            return;
        if(current_element->id() != element_id::text && current_element->id() != element_id::tspan && current_element->id() != element_id::style) {
            return;
        }

        if(in_cdata) {
            buffer.assign(text);
        } else {
            decode_text(text, buffer);
        }

        if(current_element->id() == element_id::style) {
            remove_style_comments(buffer);
            style_sheet.append(buffer);
        } else {
            auto nd = std::make_unique<svg_text_node>(this);
            nd->set_data(buffer);
            current_element->add_child(std::move(nd));
        }
    };

    std::string_view input(data, length_);
    if(length_ >= 3) {
        auto buffer = (const uint8_t*)(data);

        const auto c1 = buffer[0];
        const auto c2 = buffer[1];
        const auto c3 = buffer[2];
        if(c1 == 0xEF && c2 == 0xBB && c3 == 0xBF) {
            input.remove_prefix(3);
        }
    }

    while(!input.empty()) {
        if(current_element) {
            auto text = input.substr(0, input.find('<'));
            handle_text(text, false);
            input.remove_prefix(text.length());
        } else {
            if(!skip_optional_spaces(input)) {
                break;
            }
        }

        if(!skip_delimiter(input, '<'))
            return false;
        if(skip_delimiter(input, '?')) {
            if(!read_identifier(input, buffer))
                return false;
            auto n = input.find("?>");
            if(n == std::string_view::npos)
                return false;
            input.remove_prefix(n + 2);
            continue;
        }

        if(skip_delimiter(input, '!')) {
            if(skip_string(input, "--")) {
                auto n = input.find("-->");
                if(n == std::string_view::npos)
                    return false;
                handle_text(input.substr(0, n), false);
                input.remove_prefix(n + 3);
                continue;
            }

            if(skip_string(input, "[CDATA[")) {
                auto n = input.find("]]>");
                if(n == std::string_view::npos)
                    return false;
                handle_text(input.substr(0, n), true);
                input.remove_prefix(n + 3);
                continue;
            }

            if(skip_string(input, "DOCTYPE")) {
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

                if(!skip_delimiter(input, '>'))
                    return false;
                continue;
            }

            return false;
        }

        if(skip_delimiter(input, '/')) {
            if(current_element == nullptr && ignoring == 0)
                return false;
            if(!read_identifier(input, buffer))
                return false;
            if(ignoring == 0) {
                auto id = elementid(buffer);
                if(id != current_element->id())
                    return false;
                current_element = current_element->parent_element();
            } else {
                --ignoring;
            }

            skip_optional_spaces(input);
            if(!skip_delimiter(input, '>'))
                return false;
            continue;
        }

        if(!read_identifier(input, buffer))
            return false;
        svg_element* el = nullptr;
        if(ignoring > 0) {
            ++ignoring;
        } else {
            auto id = elementid(buffer);
            if(id == element_id::unknown) {
                ignoring = 1;
            } else {
                if(m_root_element && current_element == nullptr)
                    return false;
                if(m_root_element == nullptr) {
                    if(id != element_id::svg)
                        return false;
                    m_root_element = std::make_unique<svg_root_element>(this);
                    el = m_root_element.get();
                } else {
                    auto child = svg_element::create(this, id);
                    el = child.get();
                    current_element->add_child(std::move(child));
                }
            }
        }

        skip_optional_spaces(input);
        while(read_identifier(input, buffer)) {
            skip_optional_spaces(input);
            if(!skip_delimiter(input, '='))
                return false;
            skip_optional_spaces(input);
            if(input.empty() || !(input.front() == '\"' || input.front() == '\''))
                return false;
            auto quote = input.front();
            input.remove_prefix(1);
            auto n = input.find(quote);
            if(n == std::string_view::npos)
                return false;
            auto id = property_id::unknown;
            if(el != nullptr)
                id = propertyid(buffer);
            if(id != property_id::unknown) {
                decode_text(input.substr(0, n), buffer);
                if(id == property_id::style) {
                    remove_style_comments(buffer);
                    parse_inline_style(buffer, el);
                } else {
                    if(id == property_id::id)
                        m_root_element->add_element_by_id(buffer, el);
                    el->set_attribute(specificity::presentation_attribute, id, buffer);
                }
            } else if(el != nullptr) {
                // Not an attribute novasvg knows: keep it for CSS attribute selectors.
                std::string name = buffer;
                decode_text(input.substr(0, n), buffer);
                el->set_custom_attribute(name, buffer);
            }

            input.remove_prefix(n + 1);
            skip_optional_spaces(input);
        }

        if(skip_delimiter(input, '>')) {
            if(el != nullptr) {
                if(el->id() == element_id::foreign_object) {
                    // foreignObject content is arbitrary HTML/XML, not svg --
                    // capture it verbatim instead of trying (and failing) to
                    // parse div/span/p/etc. as svg elements. Mirrors the
                    // CDATA/comment handling above: find the matching close
                    // tag and skip straight past it.
                    static constexpr std::string_view close_tag = "</foreignObject>";
                    auto n = input.find(close_tag);
                    if(n == std::string_view::npos)
                        return false;
                    static_cast<svg_foreign_object_element*>(el)->set_raw_content(std::string(input.substr(0, n)));
                    input.remove_prefix(n + close_tag.length());
                } else {
                    current_element = el;
                }
            }
            continue;
        }

        if(skip_delimiter(input, '/')) {
            if(!skip_delimiter(input, '>'))
                return false;
            if(ignoring > 0)
                --ignoring;
            continue;
        }

        return false;
    }

    if(m_root_element == nullptr || ignoring > 0 || !input.empty())
        return false;
    apply_style_sheet(style_sheet);
    m_root_element->build();
    return true;
}

void document::apply_style_sheet(const std::string& content)
{
    auto rules = parse_style_sheet(content);
    std::sort(rules.begin(), rules.end());
    if(!rules.empty()) {
        const rule_index index(rules);
        std::vector<uint32_t> candidates;
        m_root_element->transverse([&](svg_element* el) {
            index.candidates(el, candidates);
            for(auto i : candidates) {
                const auto& rule = rules[i];
                if(rule.match(el)) {
                    for(const auto& declaration : rule.declarations()) {
                        if(declaration.id != property_id::unknown)
                            el->set_attribute(declaration.specificity, declaration.id, declaration.value);
                        else if(is_custom_property_name(declaration.name))
                            el->set_custom_property(declaration.specificity, declaration.name, declaration.value);
                    }
                }
            }
        });
    }

    // Every custom property is known now (the elements' own `style`, and every sheet so far): the values that
    // use var() are given theirs.
    m_root_element->transverse([](svg_element* el) { el->resolve_variables(); });

    if(!content.empty())
        m_root_element->add_style_sheet(content);

    // The HTML in a foreignObject is not part of the el tree above, so it is styled here, from every
    // sheet applied so far (a stylesheet given later than the one in the file sits after it in the cascade).
    // Built only if there is a foreignObject to style; and when `content` is the only sheet so far those
    // rules are exactly the ones just parsed and sorted, so they are not parsed a second time.
    rule_data_list html_rules;
    bool have_html_rules = false;
    m_root_element->transverse([&](svg_element* el) {
        if(el->id() != element_id::foreign_object)
            return;
        if(!have_html_rules) {
            const auto& sheets = m_root_element->style_sheets();
            if(!content.empty() && sheets.size() == 1)
                html_rules = std::move(rules);
            else
                html_rules = parse_style_sheets(sheets);
            have_html_rules = true;
        }

        prepare_foreign_object(static_cast<svg_foreign_object_element*>(el), m_foreign_object_renderer.get(), html_rules);
    });
}

void document::set_foreign_object_renderer(std::shared_ptr<foreign_object_renderer> renderer)
{
    m_foreign_object_renderer = std::move(renderer);
    if(!m_root_element)
        return;
    // What a renderer prepared belongs to that renderer: ask the new one, from every sheet applied so far.
    rule_data_list rules;
    bool have_rules = false;
    m_root_element->transverse([&](svg_element* el) {
        if(el->id() != element_id::foreign_object)
            return;
        if(!have_rules) {
            rules = parse_style_sheets(m_root_element->style_sheets());
            have_rules = true;
        }

        prepare_foreign_object(static_cast<svg_foreign_object_element*>(el), m_foreign_object_renderer.get(), rules);
    });
}

element_list document::query_selector_all(const std::string& content) const
{
    auto selectors = parse_query_selectors(content);
    if(selectors.empty())
        return element_list();
    element_list elements;
    m_root_element->transverse([&](svg_element* el) {
        for(const auto& selector : selectors) {
            if(match_selector(selector, el)) {
                elements.push_back(el);
                break;
            }
        }
    });

    return elements;
}

} // namespace nova::svg
