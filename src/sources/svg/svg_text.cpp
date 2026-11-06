#include "vg/pixmap.h"
#include "vg/pdf_canvas.h"
#include "svgelement.h"

namespace nova::svg {
using namespace vg;

AlignmentBaseline resolveDominantBaseline(const svg_text_positioning_element* el)
{
    switch(el->dominant_baseline()) {
    case DominantBaseline::Auto:
        if(el->isVerticalWritingMode())
            return AlignmentBaseline::Central;
        return AlignmentBaseline::Alphabetic;
    case DominantBaseline::UseScript:
    case DominantBaseline::NoChange:
    case DominantBaseline::ResetSize:
        return AlignmentBaseline::Auto;
    case DominantBaseline::Ideographic:
        return AlignmentBaseline::Ideographic;
    case DominantBaseline::Alphabetic:
        return AlignmentBaseline::Alphabetic;
    case DominantBaseline::Hanging:
        return AlignmentBaseline::Hanging;
    case DominantBaseline::Mathematical:
        return AlignmentBaseline::Mathematical;
    case DominantBaseline::Central:
        return AlignmentBaseline::Central;
    case DominantBaseline::Middle:
        return AlignmentBaseline::Middle;
    case DominantBaseline::TextAfterEdge:
        return AlignmentBaseline::TextAfterEdge;
    case DominantBaseline::TextBeforeEdge:
        return AlignmentBaseline::TextBeforeEdge;
    default:
        assert(false);
    }

    return AlignmentBaseline::Auto;
}

float calculateBaselineOffset(const svg_text_positioning_element* el)
{
    auto offset = el->baseline_offset();
    auto parent = el->parent_element();
    while(parent->isTextPositioningElement()) {
        offset += toSVGTextPositioningElement(parent)->baseline_offset();
        parent = parent->parent_element();
    }

    auto baseline = el->alignment_baseline();
    if(baseline == AlignmentBaseline::Auto || baseline == AlignmentBaseline::Baseline) {
        baseline = resolveDominantBaseline(el);
    }

    const auto& font = el->font();
    switch(baseline) {
    case AlignmentBaseline::BeforeEdge:
    case AlignmentBaseline::TextBeforeEdge:
        offset -= font.ascent();
        break;
    case AlignmentBaseline::Middle:
        offset -= font.xHeight() / 2.f;
        break;
    case AlignmentBaseline::Central:
        offset -= (font.ascent() + font.descent()) / 2.f;
        break;
    case AlignmentBaseline::AfterEdge:
    case AlignmentBaseline::TextAfterEdge:
    case AlignmentBaseline::Ideographic:
        offset -= font.descent();
        break;
    case AlignmentBaseline::Hanging:
        offset -= font.ascent() * 8.f / 10.f;
        break;
    case AlignmentBaseline::Mathematical:
        offset -= font.ascent() / 2.f;
        break;
    default:
        break;
    }

    return offset;
}

bool needsTextAnchorAdjustment(const svg_text_positioning_element* el)
{
    auto direction = el->direction();
    switch(el->text_anchor()) {
    case TextAnchor::Start:
        return direction == Direction::Rtl;
    case TextAnchor::Middle:
        return true;
    case TextAnchor::End:
        return direction == Direction::Ltr;
    default:
        assert(false);
    }

    return false;
}

float calculateTextAnchorOffset(const svg_text_positioning_element* el, float width)
{
    auto direction = el->direction();
    switch(el->text_anchor()) {
    case TextAnchor::Start:
        if(direction == Direction::Ltr)
            return 0.f;
        return -width;
    case TextAnchor::Middle:
        return -width / 2.f;
    case TextAnchor::End:
        if(direction == Direction::Ltr)
            return -width;
        return 0.f;
    default:
        assert(false);
    }

    return 0.f;
}

float calculateTextChunkLength(svg_text_fragment_iterator begin, svg_text_fragment_iterator end, bool isVerticalText)
{
    float chunkLength = 0;
    const svg_text_fragment* lastFragment = nullptr;
    for(auto it = begin; it != end; ++it) {
        const svg_text_fragment& fragment = *it;
        chunkLength += isVerticalText ? fragment.height : fragment.width;
        if(!lastFragment) {
            lastFragment = &fragment;
            continue;
        }

        if(isVerticalText) {
            chunkLength += fragment.y - (lastFragment->y + lastFragment->height);
        } else {
            chunkLength += fragment.x - (lastFragment->x + lastFragment->width);
        }

        lastFragment = &fragment;
    }

    return chunkLength;
}

void handleTextChunk(svg_text_fragment_iterator begin, svg_text_fragment_iterator end)
{
    const svg_text_fragment& firstFragment = *begin;
    const auto isVerticalText = firstFragment.el->isVerticalWritingMode();
    if(firstFragment.el->has_attribute(PropertyID::TextLength)) {
        LengthContext lengthContext(firstFragment.el);
        auto textLength = lengthContext.valueForLength(firstFragment.el->textLength());
        auto chunkLength = calculateTextChunkLength(begin, end, isVerticalText);
        if(textLength > 0.f && chunkLength > 0.f) {
            size_t numCharacters = 0;
            for(auto it = begin; it != end; ++it) {
                const svg_text_fragment& fragment = *it;
                numCharacters += fragment.length;
            }

            if(firstFragment.el->lengthAdjust() == LengthAdjust::SpacingAndGlyphs) {
                auto textLengthScale = textLength / chunkLength;
                auto lengthAdjustTransform = Transform::translated(firstFragment.x, firstFragment.y);
                if(isVerticalText) {
                    lengthAdjustTransform.scale(1.f, textLengthScale);
                } else {
                    lengthAdjustTransform.scale(textLengthScale, 1.f);
                }

                lengthAdjustTransform.translate(-firstFragment.x, -firstFragment.y);
                for(auto it = begin; it != end; ++it) {
                    svg_text_fragment& fragment = *it;
                    fragment.lengthAdjustTransform = lengthAdjustTransform;
                }
            } else if(numCharacters > 1) {
                assert(firstFragment.el->lengthAdjust() == LengthAdjust::Spacing);
                size_t characterOffset = 0;
                auto textLengthShift = (textLength - chunkLength) / (numCharacters - 1);
                for(auto it = begin; it != end; ++it) {
                    svg_text_fragment& fragment = *it;
                    if(isVerticalText) {
                        fragment.y += textLengthShift * characterOffset;
                    } else {
                        fragment.x += textLengthShift * characterOffset;
                    }

                    characterOffset += fragment.length;
                }
            }
        }
    }

    if(needsTextAnchorAdjustment(firstFragment.el)) {
        auto chunkLength = calculateTextChunkLength(begin, end, isVerticalText);
        auto chunkOffset = calculateTextAnchorOffset(firstFragment.el, chunkLength);
        for(auto it = begin; it != end; ++it) {
            svg_text_fragment& fragment = *it;
            if(isVerticalText) {
                fragment.y += chunkOffset;
            } else {
                fragment.x += chunkOffset;
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

void svg_text_fragments_builder::build(const svg_text_element* textElement)
{
    handleElement(textElement);
    for(const auto& position : m_textPositions) {
        fillCharacterPositions(position);
    }

    std::u32string_view wholeText(m_text);
    for(const auto& textPosition : m_textPositions) {
        if(!textPosition.nd->is_text_node())
            continue;
        auto el = toSVGTextPositioningElement(textPosition.nd->parent_element());
        const auto isVerticalText = el->isVerticalWritingMode();
        const auto isUprightText = el->isUprightTextOrientation();
        const auto& font = el->font();

        svg_text_fragment fragment(el);
        auto recordTextFragment = [&](auto startOffset, auto endOffset) {
            auto text = wholeText.substr(startOffset, endOffset - startOffset);
            fragment.offset = startOffset;
            fragment.length = text.length();
            fragment.width = font.measureText(text);
            fragment.height = font.height() + font.lineGap();
            if(isVerticalText) {
                m_y += isUprightText ? fragment.height : fragment.width;
            } else {
                m_x += fragment.width;
            }

            m_fragments.push_back(fragment);
        };

        auto needsTextLengthSpacing = el->lengthAdjust() == LengthAdjust::Spacing && el->has_attribute(PropertyID::TextLength);
        auto baselineOffset = calculateBaselineOffset(el);
        auto startOffset = textPosition.startOffset;
        auto textOffset = textPosition.startOffset;
        auto didStartTextFragment = false;
        auto applySpacingToNextCharacter = false;
        auto lastCharacter = 0u;
        auto lastAngle = 0.f;
        while(textOffset < textPosition.endOffset) {
            svg_character_position characterPosition;
            if(auto it = m_characterPositions.find(m_characterOffset); it != m_characterPositions.end()) {
                characterPosition = it->second;
            }

            auto currentCharacter = wholeText.at(textOffset);
            auto angle = characterPosition.rotate.value_or(0);
            auto dx = characterPosition.dx.value_or(0);
            auto dy = characterPosition.dy.value_or(0);

            auto shouldStartNewFragment = needsTextLengthSpacing || isVerticalText || applySpacingToNextCharacter
                || characterPosition.x || characterPosition.y || dx || dy || angle || angle != lastAngle;
            if(shouldStartNewFragment && didStartTextFragment) {
                recordTextFragment(startOffset, textOffset);
                applySpacingToNextCharacter = false;
                startOffset = textOffset;
            }

            auto startsNewTextChunk = (characterPosition.x || characterPosition.y) && textOffset == textPosition.startOffset;
            if(startsNewTextChunk || shouldStartNewFragment || !didStartTextFragment) {
                m_x = dx + characterPosition.x.value_or(m_x);
                m_y = dy + characterPosition.y.value_or(m_y);
                fragment.x = isVerticalText ? m_x + baselineOffset : m_x;
                fragment.y = isVerticalText ? m_y : m_y - baselineOffset;
                fragment.angle = angle;
                if(isVerticalText) {
                    if(isUprightText) {
                        fragment.y += font.height();
                    } else {
                        fragment.angle += 90.f;
                    }
                }

                fragment.startsNewTextChunk = startsNewTextChunk;
                didStartTextFragment = true;
            }

            auto spacing = el->letter_spacing();
            if(currentCharacter && lastCharacter && el->word_spacing()) {
                if(currentCharacter == ' ' && lastCharacter != ' ') {
                    spacing += el->word_spacing();
                }
            }

            if(spacing) {
                applySpacingToNextCharacter = true;
                if(isVerticalText) {
                    m_y += spacing;
                } else {
                    m_x += spacing;
                }
            }

            lastAngle = angle;
            lastCharacter = currentCharacter;
            ++textOffset;
            ++m_characterOffset;
        }

        recordTextFragment(startOffset, textOffset);
    }

    if(m_fragments.empty())
        return;
    auto it = m_fragments.begin();
    auto begin = m_fragments.begin();
    auto end = m_fragments.end();
    for(++it; it != end; ++it) {
        const svg_text_fragment& fragment = *it;
        if(!fragment.startsNewTextChunk)
            continue;
        handleTextChunk(begin, it);
        begin = it;
    }

    handleTextChunk(begin, it);
}

void svg_text_fragments_builder::handleText(const svg_text_node* nd)
{
    const auto& text = nd->data();
    if(text.empty())
        return;
    auto el = toSVGTextPositioningElement(nd->parent_element());
    const auto startOffset = m_text.length();
    uint32_t lastCharacter = ' ';
    if(!m_text.empty()) {
        lastCharacter = m_text.back();
    }

    text_iterator_t it;
    text_iterator_init(&it, text.data(), text.length(), NOVA_TEXT_ENCODING_UTF8);
    while(text_iterator_has_next(&it)) {
        auto currentCharacter = text_iterator_next(&it);
        if(currentCharacter == '\t' || currentCharacter == '\n' || currentCharacter == '\r')
            currentCharacter = ' ';
        if(currentCharacter == ' ' && lastCharacter == ' ' && el->white_space() == WhiteSpace::Default)
            continue;
        m_text.push_back(currentCharacter);
        lastCharacter = currentCharacter;
    }

    if(startOffset < m_text.length()) {
        m_textPositions.emplace_back(nd, startOffset, m_text.length());
    }
}

void svg_text_fragments_builder::handleElement(const svg_text_positioning_element* el)
{
    if(el->isDisplayNone())
        return;
    const auto itemIndex = m_textPositions.size();
    m_textPositions.emplace_back(el, m_text.length(), m_text.length());
    for(const auto& child : el->children()) {
        if(child->is_text_node()) {
            handleText(toSVGTextNode(child.get()));
        } else if(child->isTextPositioningElement()) {
            handleElement(toSVGTextPositioningElement(child.get()));
        }
    }

    auto& position = m_textPositions[itemIndex];
    assert(position.nd == el);
    position.endOffset = m_text.length();
}

void svg_text_fragments_builder::fillCharacterPositions(const svg_text_position& position)
{
    if(!position.nd->isTextPositioningElement())
        return;
    auto el = toSVGTextPositioningElement(position.nd);
    const auto& xList = el->x();
    const auto& yList = el->y();
    const auto& dxList = el->dx();
    const auto& dyList = el->dy();
    const auto& rotateList = el->rotate();

    auto xListSize = xList.size();
    auto yListSize = yList.size();
    auto dxListSize = dxList.size();
    auto dyListSize = dyList.size();
    auto rotateListSize = rotateList.size();
    if(!xListSize && !yListSize && !dxListSize && !dyListSize && !rotateListSize) {
        return;
    }

    LengthContext lengthContext(el);
    std::optional<float> lastRotation;
    for(auto offset = position.startOffset; offset < position.endOffset; ++offset) {
        auto index = offset - position.startOffset;
        if(index >= xListSize && index >= yListSize && index >= dxListSize && index >= dyListSize && index >= rotateListSize)
            break;
        auto& characterPosition = m_characterPositions[offset];
        if(index < xListSize)
            characterPosition.x = lengthContext.valueForLength(xList[index], LengthDirection::Horizontal);
        if(index < yListSize)
            characterPosition.y = lengthContext.valueForLength(yList[index], LengthDirection::Vertical);
        if(index < dxListSize)
            characterPosition.dx = lengthContext.valueForLength(dxList[index], LengthDirection::Horizontal);
        if(index < dyListSize)
            characterPosition.dy = lengthContext.valueForLength(dyList[index], LengthDirection::Vertical);
        if(index < rotateListSize) {
            characterPosition.rotate = rotateList[index];
            lastRotation = characterPosition.rotate;
        }
    }

    if(lastRotation == std::nullopt)
        return;
    auto offset = position.startOffset + rotateList.size();
    while(offset < position.endOffset) {
        m_characterPositions[offset++].rotate = lastRotation;
    }
}

svg_text_positioning_element::svg_text_positioning_element(document* dom, ElementID id)
    : svg_graphics_element(dom, id)
    , m_x(PropertyID::X, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_y(PropertyID::Y, LengthDirection::Vertical, LengthNegativeMode::Allow)
    , m_dx(PropertyID::Dx, LengthDirection::Horizontal, LengthNegativeMode::Allow)
    , m_dy(PropertyID::Dy, LengthDirection::Vertical, LengthNegativeMode::Allow)
    , m_rotate(PropertyID::Rotate)
    , m_textLength(PropertyID::TextLength, LengthDirection::Horizontal, LengthNegativeMode::Forbid)
    , m_lengthAdjust(PropertyID::LengthAdjust, LengthAdjust::Spacing)
{
    addProperty(m_x);
    addProperty(m_y);
    addProperty(m_dx);
    addProperty(m_dy);
    addProperty(m_rotate);
    addProperty(m_textLength);
    addProperty(m_lengthAdjust);
}

void svg_text_positioning_element::layoutElement(const svg_layout_state& state)
{
    m_font = state.font();
    m_fill = getPaintServer(state.fill(), state.fill_opacity());
    m_stroke = getPaintServer(state.stroke(), state.stroke_opacity());
    svg_graphics_element::layoutElement(state);

    LengthContext lengthContext(this);
    m_stroke_width = lengthContext.valueForLength(state.stroke_width(), LengthDirection::Diagonal);
    m_letter_spacing = lengthContext.valueForLength(state.letter_spacing(), LengthDirection::Diagonal);
    m_word_spacing = lengthContext.valueForLength(state.word_spacing(), LengthDirection::Diagonal);

    m_baseline_offset = convertBaselineOffset(state.baseline_shift());
    m_alignment_baseline = state.alignment_baseline();
    m_dominant_baseline = state.dominant_baseline();
    m_text_anchor = state.text_anchor();
    m_white_space = state.white_space();
    m_writing_mode = state.writing_mode();
    m_text_orientation = state.text_orientation();
    m_direction = state.direction();
}

float svg_text_positioning_element::convertBaselineOffset(const BaselineShift& baselineShift) const
{
    if(baselineShift.type() == BaselineShift::Type::Baseline)
        return 0.f;
    if(baselineShift.type() == BaselineShift::Type::Sub)
        return -m_font.height() / 2.f;
    if(baselineShift.type() == BaselineShift::Type::Super) {
        return m_font.height() / 2.f;
    }

    const auto& length = baselineShift.length();
    if(length.units() == LengthUnits::Percent)
        return length.value() * m_font.size() / 100.f;
    if(length.units() == LengthUnits::Ex)
        return length.value() * m_font.size() / 2.f;
    if(length.units() == LengthUnits::Em)
        return length.value() * m_font.size();
    return length.value();
}

svg_t_span_element::svg_t_span_element(document* dom)
    : svg_text_positioning_element(dom, ElementID::Tspan)
{
}

svg_text_element::svg_text_element(document* dom)
    : svg_text_positioning_element(dom, ElementID::Text)
{
}

void svg_text_element::layout(svg_layout_state& state)
{
    svg_text_positioning_element::layout(state);
    svg_text_fragments_builder(m_text, m_fragments).build(this);
}

void svg_text_element::render(svg_render_state& state) const
{
    if(m_fragments.empty() || isVisibilityHidden() || isDisplayNone())
        return;
    svg_blend_info blendInfo(this);
    svg_render_state newState(this, state, local_transform());
    newState.beginGroup(blendInfo);
    if(newState.mode() == svg_render_mode::Clipping) {
        newState->set_color(stb::color::white());
    }

    std::u32string_view wholeText(m_text);
    for(const auto& fragment : m_fragments) {
        if(fragment.el->isVisibilityHidden())
            continue;
        auto transform = newState.currentTransform() * Transform::rotated(fragment.angle, fragment.x, fragment.y) * fragment.lengthAdjustTransform;
        auto text = wholeText.substr(fragment.offset, fragment.length);
        auto origin = Point(fragment.x, fragment.y);

        const auto& font = fragment.el->font();
        if(newState.mode() == svg_render_mode::Clipping) {
            newState->fill_text(text, font, origin, transform);
        } else {
            const auto& fill = fragment.el->fill();
            const auto& stroke = fragment.el->stroke();
            auto stroke_width = fragment.el->stroke_width();
            if(fill.applyPaint(newState))
                newState->fill_text(text, font, origin, transform);
            if(stroke.applyPaint(newState)) {
                newState->stroke_text(text, stroke_width, font, origin, transform);
            }
        }
    }

    newState.endGroup(blendInfo);
}

Rect svg_text_element::bounding_box(bool includeStroke) const
{
    auto bounding_box = Rect::Invalid;
    for(const auto& fragment : m_fragments) {
        const auto& font = fragment.el->font();
        const auto& stroke = fragment.el->stroke();
        auto fragmentTranform = Transform::rotated(fragment.angle, fragment.x, fragment.y) * fragment.lengthAdjustTransform;
        auto fragmentRect = Rect(fragment.x, fragment.y - font.ascent(), fragment.width, fragment.height);
        if(includeStroke && stroke.isRenderable())
            fragmentRect.inflate(fragment.el->stroke_width() / 2.f);
        bounding_box.unite(fragmentTranform.map_rect(fragmentRect));
    }

    if(!bounding_box.isValid())
        bounding_box = Rect::Empty;
    return bounding_box;
}

} // namespace nova::svg
