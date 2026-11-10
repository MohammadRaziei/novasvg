#include "vg/pixmap.h"
#include "vg/pdf_canvas.h"
#include "svgelement.h"

namespace nova::svg {
using namespace vg;

css_alignment_baseline resolve_dominant_baseline(const svg_text_positioning_element* el)
{
    switch(el->dominant_baseline()) {
    case css_dominant_baseline::auto_:
        if(el->is_vertical_writing_mode())
            return css_alignment_baseline::central;
        return css_alignment_baseline::alphabetic;
    case css_dominant_baseline::use_script:
    case css_dominant_baseline::no_change:
    case css_dominant_baseline::reset_size:
        return css_alignment_baseline::auto_;
    case css_dominant_baseline::ideographic:
        return css_alignment_baseline::ideographic;
    case css_dominant_baseline::alphabetic:
        return css_alignment_baseline::alphabetic;
    case css_dominant_baseline::hanging:
        return css_alignment_baseline::hanging;
    case css_dominant_baseline::mathematical:
        return css_alignment_baseline::mathematical;
    case css_dominant_baseline::central:
        return css_alignment_baseline::central;
    case css_dominant_baseline::middle:
        return css_alignment_baseline::middle;
    case css_dominant_baseline::text_after_edge:
        return css_alignment_baseline::text_after_edge;
    case css_dominant_baseline::text_before_edge:
        return css_alignment_baseline::text_before_edge;
    default:
        assert(false);
    }

    return css_alignment_baseline::auto_;
}

float calculate_baseline_offset(const svg_text_positioning_element* el)
{
    auto offset = el->baseline_offset();
    auto parent = el->parent_element();
    while(parent->is_text_positioning_element()) {
        offset += to_svg_text_positioning_element(parent)->baseline_offset();
        parent = parent->parent_element();
    }

    auto baseline = el->alignment_baseline();
    if(baseline == css_alignment_baseline::auto_ || baseline == css_alignment_baseline::baseline) {
        baseline = resolve_dominant_baseline(el);
    }

    const auto& font_ = el->font();
    switch(baseline) {
    case css_alignment_baseline::before_edge:
    case css_alignment_baseline::text_before_edge:
        offset -= font_.ascent();
        break;
    case css_alignment_baseline::middle:
        offset -= font_.x_height() / 2.f;
        break;
    case css_alignment_baseline::central:
        offset -= (font_.ascent() + font_.descent()) / 2.f;
        break;
    case css_alignment_baseline::after_edge:
    case css_alignment_baseline::text_after_edge:
    case css_alignment_baseline::ideographic:
        offset -= font_.descent();
        break;
    case css_alignment_baseline::hanging:
        offset -= font_.ascent() * 8.f / 10.f;
        break;
    case css_alignment_baseline::mathematical:
        offset -= font_.ascent() / 2.f;
        break;
    default:
        break;
    }

    return offset;
}

bool needs_text_anchor_adjustment(const svg_text_positioning_element* el)
{
    auto direction_ = el->direction();
    switch(el->text_anchor()) {
    case svg_text_anchor::start:
        return direction_ == css_direction::rtl;
    case svg_text_anchor::middle:
        return true;
    case svg_text_anchor::end:
        return direction_ == css_direction::ltr;
    default:
        assert(false);
    }

    return false;
}

float calculate_text_anchor_offset(const svg_text_positioning_element* el, float width)
{
    auto direction_ = el->direction();
    switch(el->text_anchor()) {
    case svg_text_anchor::start:
        if(direction_ == css_direction::ltr)
            return 0.f;
        return -width;
    case svg_text_anchor::middle:
        return -width / 2.f;
    case svg_text_anchor::end:
        if(direction_ == css_direction::ltr)
            return -width;
        return 0.f;
    default:
        assert(false);
    }

    return 0.f;
}

float calculate_text_chunk_length(svg_text_fragment_iterator begin, svg_text_fragment_iterator end, bool is_vertical_text)
{
    float chunk_length = 0;
    const svg_text_fragment* last_fragment = nullptr;
    for(auto it = begin; it != end; ++it) {
        const svg_text_fragment& fragment = *it;
        chunk_length += is_vertical_text ? fragment.height : fragment.width;
        if(!last_fragment) {
            last_fragment = &fragment;
            continue;
        }

        if(is_vertical_text) {
            chunk_length += fragment.y - (last_fragment->y + last_fragment->height);
        } else {
            chunk_length += fragment.x - (last_fragment->x + last_fragment->width);
        }

        last_fragment = &fragment;
    }

    return chunk_length;
}

void handle_text_chunk(svg_text_fragment_iterator begin, svg_text_fragment_iterator end)
{
    const svg_text_fragment& first_fragment = *begin;
    const auto is_vertical_text = first_fragment.el->is_vertical_writing_mode();
    if(first_fragment.el->has_attribute(property_id::text_length)) {
        length_context length_context_(first_fragment.el);
        auto text_length = length_context_.value_for_length(first_fragment.el->text_length());
        auto chunk_length = calculate_text_chunk_length(begin, end, is_vertical_text);
        if(text_length > 0.f && chunk_length > 0.f) {
            size_t num_characters = 0;
            for(auto it = begin; it != end; ++it) {
                const svg_text_fragment& fragment = *it;
                num_characters += fragment.length_;
            }

            if(first_fragment.el->length_adjust() == svg_length_adjust::spacing_and_glyphs) {
                auto text_length_scale = text_length / chunk_length;
                auto length_adjust_transform = transform::translated(first_fragment.x, first_fragment.y);
                if(is_vertical_text) {
                    length_adjust_transform.scale(1.f, text_length_scale);
                } else {
                    length_adjust_transform.scale(text_length_scale, 1.f);
                }

                length_adjust_transform.translate(-first_fragment.x, -first_fragment.y);
                for(auto it = begin; it != end; ++it) {
                    svg_text_fragment& fragment = *it;
                    fragment.length_adjust_transform = length_adjust_transform;
                }
            } else if(num_characters > 1) {
                assert(first_fragment.el->length_adjust() == svg_length_adjust::spacing);
                size_t character_offset = 0;
                auto text_length_shift = (text_length - chunk_length) / (num_characters - 1);
                for(auto it = begin; it != end; ++it) {
                    svg_text_fragment& fragment = *it;
                    if(is_vertical_text) {
                        fragment.y += text_length_shift * character_offset;
                    } else {
                        fragment.x += text_length_shift * character_offset;
                    }

                    character_offset += fragment.length_;
                }
            }
        }
    }

    if(needs_text_anchor_adjustment(first_fragment.el)) {
        auto chunk_length = calculate_text_chunk_length(begin, end, is_vertical_text);
        auto chunk_offset = calculate_text_anchor_offset(first_fragment.el, chunk_length);
        for(auto it = begin; it != end; ++it) {
            svg_text_fragment& fragment = *it;
            if(is_vertical_text) {
                fragment.y += chunk_offset;
            } else {
                fragment.x += chunk_offset;
            }
        }
    }
}

svg_text_fragments_builder::svg_text_fragments_builder(std::u32string& text, svg_text_fragment_list& fragments)
    : m_text(text), m_fragments(fragments)
{
    m_text.clear();
    m_fragments.clear();
}

// Where a run starts (ADR-0012): the first character, a change of font, and every character with an absolute x or y
// (a new text chunk). A run is shaped as a whole, whatever <tspan>s or per-character positions cut it into fragments.
void svg_text_fragments_builder::find_run_starts()
{
    m_run_starts.clear();
    const font_face_data* last_face = nullptr;
    float last_size = 0.f;
    size_t character = 0;
    for(const auto& text_position : m_text_positions) {
        if(!text_position.nd->is_text_node())
            continue;
        const auto& font = to_svg_text_positioning_element(text_position.nd->parent_element())->font();
        const auto font_changed = font.face().get() != last_face || font.size() != last_size;
        last_face = font.face().get();
        last_size = font.size();
        for(auto offset = text_position.start_offset; offset < text_position.end_offset; ++offset, ++character) {
            auto it = m_character_positions.find(character);
            const auto is_absolute = it != m_character_positions.end() && (it->second.x || it->second.y);
            if(((offset == text_position.start_offset && font_changed) || is_absolute)
               && (m_run_starts.empty() || m_run_starts.back() != offset))
                m_run_starts.push_back(offset);
        }
    }
}

void svg_text_fragments_builder::build(const svg_text_element* text_element)
{
    handle_element(text_element);
    for(const auto& position : m_text_positions) {
        fill_character_positions(position);
    }
    find_run_starts();

    std::u32string_view whole_text(m_text);
    for(const auto& text_position : m_text_positions) {
        if(!text_position.nd->is_text_node())
            continue;
        auto el = to_svg_text_positioning_element(text_position.nd->parent_element());
        const auto is_vertical_text = el->is_vertical_writing_mode();
        const auto is_upright_text = el->is_upright_text_orientation();
        const auto& font = el->font();

        svg_text_fragment fragment(el);
        auto record_text_fragment = [&](auto start_offset, auto end_offset) {
            auto text = whole_text.substr(start_offset, end_offset - start_offset);
            fragment.offset = start_offset;
            fragment.length_ = text.length();
            fragment.run_offset = 0;
            fragment.run_length_ = 0;
            if(!is_vertical_text) {
                auto next = std::upper_bound(m_run_starts.begin(), m_run_starts.end(), start_offset);
                assert(next != m_run_starts.begin());
                fragment.run_offset = *(next - 1);
                fragment.run_length_ = (next == m_run_starts.end() ? whole_text.length() : *next) - fragment.run_offset;
            }
            // Letter spacing is added between letters, so a ligature must not join them.
            fragment.ligatures = el->letter_spacing() == 0;
            fragment.width = font.measure_text(text, fragment.context(whole_text));
            fragment.height = font.height() + font.line_gap();
            if(is_vertical_text) {
                m_y += is_upright_text ? fragment.height : fragment.width;
            } else {
                m_x += fragment.width;
            }

            m_fragments.push_back(fragment);
        };

        auto needs_text_length_spacing = el->length_adjust() == svg_length_adjust::spacing && el->has_attribute(property_id::text_length);
        auto baseline_offset = calculate_baseline_offset(el);
        auto start_offset = text_position.start_offset;
        auto text_offset = text_position.start_offset;
        auto did_start_text_fragment = false;
        auto apply_spacing_to_next_character = false;
        auto last_character = 0u;
        auto last_angle = 0.f;
        while(text_offset < text_position.end_offset) {
            svg_character_position character_position;
            if(auto it = m_character_positions.find(m_character_offset); it != m_character_positions.end()) {
                character_position = it->second;
            }

            auto current_character = whole_text.at(text_offset);
            auto angle = character_position.rotate.value_or(0);
            auto dx = character_position.dx.value_or(0);
            auto dy = character_position.dy.value_or(0);

            auto should_start_new_fragment = needs_text_length_spacing || is_vertical_text || apply_spacing_to_next_character
                || character_position.x || character_position.y || dx || dy || angle || angle != last_angle;
            if(should_start_new_fragment && did_start_text_fragment) {
                record_text_fragment(start_offset, text_offset);
                apply_spacing_to_next_character = false;
                start_offset = text_offset;
            }

            auto starts_new_text_chunk = (character_position.x || character_position.y) && text_offset == text_position.start_offset;
            if(starts_new_text_chunk || should_start_new_fragment || !did_start_text_fragment) {
                m_x = dx + character_position.x.value_or(m_x);
                m_y = dy + character_position.y.value_or(m_y);
                fragment.x = is_vertical_text ? m_x + baseline_offset : m_x;
                fragment.y = is_vertical_text ? m_y : m_y - baseline_offset;
                fragment.angle = angle;
                if(is_vertical_text) {
                    if(is_upright_text) {
                        fragment.y += font.height();
                    } else {
                        fragment.angle += 90.f;
                    }
                }

                fragment.starts_new_text_chunk = starts_new_text_chunk;
                did_start_text_fragment = true;
            }

            auto spacing = el->letter_spacing();
            if(current_character && last_character && el->word_spacing()) {
                if(current_character == ' ' && last_character != ' ') {
                    spacing += el->word_spacing();
                }
            }

            if(spacing) {
                apply_spacing_to_next_character = true;
                if(is_vertical_text) {
                    m_y += spacing;
                } else {
                    m_x += spacing;
                }
            }

            last_angle = angle;
            last_character = current_character;
            ++text_offset;
            ++m_character_offset;
        }

        record_text_fragment(start_offset, text_offset);
    }

    if(m_fragments.empty())
        return;
    auto it = m_fragments.begin();
    auto begin = m_fragments.begin();
    auto end = m_fragments.end();
    for(++it; it != end; ++it) {
        const svg_text_fragment& fragment = *it;
        if(!fragment.starts_new_text_chunk)
            continue;
        handle_text_chunk(begin, it);
        begin = it;
    }

    handle_text_chunk(begin, it);
}

void svg_text_fragments_builder::handle_text(const svg_text_node* nd)
{
    const auto& text = nd->data();
    if(text.empty())
        return;
    auto el = to_svg_text_positioning_element(nd->parent_element());
    const auto start_offset = m_text.length();
    uint32_t last_character = ' ';
    if(!m_text.empty()) {
        last_character = m_text.back();
    }

    text_iterator_t it;
    text_iterator_init(&it, text.data(), text.length(), text_encoding::utf8);
    while(text_iterator_has_next(&it)) {
        auto current_character = text_iterator_next(&it);
        if(current_character == '\t' || current_character == '\n' || current_character == '\r')
            current_character = ' ';
        if(current_character == ' ' && last_character == ' ' && el->white_space() == css_white_space::default_)
            continue;
        m_text.push_back(current_character);
        last_character = current_character;
    }

    if(start_offset < m_text.length()) {
        m_text_positions.emplace_back(nd, start_offset, m_text.length());
    }
}

void svg_text_fragments_builder::handle_element(const svg_text_positioning_element* el)
{
    if(el->is_display_none())
        return;
    const auto item_index = m_text_positions.size();
    m_text_positions.emplace_back(el, m_text.length(), m_text.length());
    for(const auto& child : el->children()) {
        if(child->is_text_node()) {
            handle_text(to_svg_text_node(child.get()));
        } else if(child->is_text_positioning_element()) {
            handle_element(to_svg_text_positioning_element(child.get()));
        }
    }

    auto& position = m_text_positions[item_index];
    assert(position.nd == el);
    position.end_offset = m_text.length();
}

void svg_text_fragments_builder::fill_character_positions(const svg_text_position& position)
{
    if(!position.nd->is_text_positioning_element())
        return;
    auto el = to_svg_text_positioning_element(position.nd);
    const auto& x_list = el->x();
    const auto& y_list = el->y();
    const auto& dx_list = el->dx();
    const auto& dy_list = el->dy();
    const auto& rotate_list = el->rotate();

    auto x_list_size = x_list.size();
    auto y_list_size = y_list.size();
    auto dx_list_size = dx_list.size();
    auto dy_list_size = dy_list.size();
    auto rotate_list_size = rotate_list.size();
    if(!x_list_size && !y_list_size && !dx_list_size && !dy_list_size && !rotate_list_size) {
        return;
    }

    length_context length_context_(el);
    std::optional<float> last_rotation;
    for(auto offset = position.start_offset; offset < position.end_offset; ++offset) {
        auto index = offset - position.start_offset;
        if(index >= x_list_size && index >= y_list_size && index >= dx_list_size && index >= dy_list_size && index >= rotate_list_size)
            break;
        auto& character_position = m_character_positions[offset];
        if(index < x_list_size)
            character_position.x = length_context_.value_for_length(x_list[index], length_direction::horizontal);
        if(index < y_list_size)
            character_position.y = length_context_.value_for_length(y_list[index], length_direction::vertical);
        if(index < dx_list_size)
            character_position.dx = length_context_.value_for_length(dx_list[index], length_direction::horizontal);
        if(index < dy_list_size)
            character_position.dy = length_context_.value_for_length(dy_list[index], length_direction::vertical);
        if(index < rotate_list_size) {
            character_position.rotate = rotate_list[index];
            last_rotation = character_position.rotate;
        }
    }

    if(last_rotation == std::nullopt)
        return;
    auto offset = position.start_offset + rotate_list.size();
    while(offset < position.end_offset) {
        m_character_positions[offset++].rotate = last_rotation;
    }
}

svg_text_positioning_element::svg_text_positioning_element(document* dom, element_id id)
    : svg_graphics_element(dom, id)
    , m_x(property_id::X, length_direction::horizontal, length_negative_mode::allow)
    , m_y(property_id::Y, length_direction::vertical, length_negative_mode::allow)
    , m_dx(property_id::dx, length_direction::horizontal, length_negative_mode::allow)
    , m_dy(property_id::dy, length_direction::vertical, length_negative_mode::allow)
    , m_rotate(property_id::rotate)
    , m_text_length(property_id::text_length, length_direction::horizontal, length_negative_mode::forbid)
    , m_length_adjust(property_id::length_adjust, svg_length_adjust::spacing)
{
    add_property(m_x);
    add_property(m_y);
    add_property(m_dx);
    add_property(m_dy);
    add_property(m_rotate);
    add_property(m_text_length);
    add_property(m_length_adjust);
}

void svg_text_positioning_element::layout_element(const svg_layout_state& state)
{
    m_font = state.font();
    m_fill = get_paint_server(state.fill(), state.fill_opacity());
    m_stroke = get_paint_server(state.stroke(), state.stroke_opacity());
    svg_graphics_element::layout_element(state);

    length_context length_context_(this);
    m_stroke_width = length_context_.value_for_length(state.stroke_width(), length_direction::diagonal);
    m_letter_spacing = length_context_.value_for_length(state.letter_spacing(), length_direction::diagonal);
    m_word_spacing = length_context_.value_for_length(state.word_spacing(), length_direction::diagonal);

    m_baseline_offset = convert_baseline_offset(state.baseline_shift());
    m_alignment_baseline = state.alignment_baseline();
    m_dominant_baseline = state.dominant_baseline();
    m_text_anchor = state.text_anchor();
    m_white_space = state.white_space();
    m_writing_mode = state.writing_mode();
    m_text_orientation = state.text_orientation();
    m_direction = state.direction();
}

float svg_text_positioning_element::convert_baseline_offset(const css_baseline_shift& baseline_shift_) const
{
    if(baseline_shift_.type() == css_baseline_shift::shift_type::baseline)
        return 0.f;
    if(baseline_shift_.type() == css_baseline_shift::shift_type::sub)
        return -m_font.height() / 2.f;
    if(baseline_shift_.type() == css_baseline_shift::shift_type::super) {
        return m_font.height() / 2.f;
    }

    const auto& length_ = baseline_shift_.length();
    if(length_.units() == length_units::percent)
        return length_.value() * m_font.size() / 100.f;
    if(length_.units() == length_units::ex)
        return length_.value() * m_font.size() / 2.f;
    if(length_.units() == length_units::em)
        return length_.value() * m_font.size();
    return length_.value();
}

svg_t_span_element::svg_t_span_element(document* dom)
    : svg_text_positioning_element(dom, element_id::tspan)
{
}

svg_text_element::svg_text_element(document* dom)
    : svg_text_positioning_element(dom, element_id::text)
{
}

void svg_text_element::layout(svg_layout_state& state)
{
    svg_text_positioning_element::layout(state);
    svg_text_fragments_builder(m_text, m_fragments).build(this);
}

void svg_text_element::render(svg_render_state& state) const
{
    if(m_fragments.empty() || is_visibility_hidden() || is_display_none())
        return;
    svg_blend_info blend_info(this);
    svg_render_state new_state(this, state, local_transform());
    new_state.begin_group(blend_info);
    if(new_state.mode() == svg_render_mode::clipping) {
        new_state->set_color(stb::color::white());
    }

    std::u32string_view whole_text(m_text);
    for(const auto& fragment : m_fragments) {
        if(fragment.el->is_visibility_hidden())
            continue;
        auto transform = new_state.current_transform() * transform::rotated(fragment.angle, fragment.x, fragment.y) * fragment.length_adjust_transform;
        auto text = whole_text.substr(fragment.offset, fragment.length_);
        auto origin = point(fragment.x, fragment.y);

        const auto& font = fragment.el->font();
        const auto context = fragment.context(whole_text);
        if(new_state.mode() == svg_render_mode::clipping) {
            new_state->fill_text(text, font, origin, transform, context);
        } else {
            const auto& fill = fragment.el->fill();
            const auto& stroke = fragment.el->stroke();
            auto stroke_width = fragment.el->stroke_width();
            if(fill.apply_paint(new_state))
                new_state->fill_text(text, font, origin, transform, context);
            if(stroke.apply_paint(new_state)) {
                new_state->stroke_text(text, stroke_width, font, origin, transform, context);
            }
        }
    }

    new_state.end_group(blend_info);
}

rect svg_text_element::bounding_box(bool include_stroke) const
{
    auto bounding_box = rect::invalid;
    for(const auto& fragment : m_fragments) {
        const auto& font = fragment.el->font();
        const auto& stroke = fragment.el->stroke();
        auto fragment_tranform = transform::rotated(fragment.angle, fragment.x, fragment.y) * fragment.length_adjust_transform;
        auto fragment_rect = rect(fragment.x, fragment.y - font.ascent(), fragment.width, fragment.height);
        if(include_stroke && stroke.is_renderable())
            fragment_rect.inflate(fragment.el->stroke_width() / 2.f);
        bounding_box.unite(fragment_tranform.map_rect(fragment_rect));
    }

    if(!bounding_box.is_valid())
        bounding_box = rect::empty_rect;
    return bounding_box;
}

} // namespace nova::svg
