#pragma once

// switch conditional processing and the geometry element cast.

#include "utils/text.h"
#include "svg_css_utils.h"

namespace nova::svg {
using namespace vg; // the drawing layer (nova::vg)

inline bool switch_child_passes(const svg_element* child)
{
    if(child->has_attribute(property_id::required_extensions)) {
        std::string_view value = utils::trim(child->attribute(property_id::required_extensions));
        bool supported = false;
        size_t pos = 0;
        while(pos < value.size() && !supported) {
            while(pos < value.size() && utils::is_space(value[pos]))
                ++pos;
            auto end = pos;
            while(end < value.size() && !utils::is_space(value[end]))
                ++end;
            supported = value.substr(pos, end - pos) == "http://www.w3.org/1999/xhtml";
            pos = end;
        }
        if(!supported)
            return false;
    }
    if(child->has_attribute(property_id::system_language)) {
        std::string_view value = child->attribute(property_id::system_language);
        bool matches = false;
        size_t pos = 0;
        while(pos <= value.size() && !matches) {
            auto comma = value.find(',', pos);
            auto tag = utils::trim(value.substr(pos, comma == std::string_view::npos ? std::string_view::npos : comma - pos));
            std::string lower(tag);
            for(auto& c : lower)
                c = utils::to_lower(c);
            matches = lower == "en" || lower.rfind("en-", 0) == 0;
            if(comma == std::string_view::npos)
                break;
            pos = comma + 1;
        }
        if(!matches)
            return false;
    }
    return true;
}

inline const svg_geometry_element* to_svg_geometry_element(const svg_node* nd)
{
    if(nd && nd->is_geometry_element())
        return static_cast<const svg_geometry_element*>(nd);
    return nullptr;
}

// ---- svgfilterelement (impl) ----

} // namespace nova::svg
