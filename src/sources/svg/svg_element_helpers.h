#pragma once

// switch conditional processing and the geometry element cast.

#include "svg_foreign_object_text.h"

namespace nova::svg {
using namespace vg; // the drawing layer (nova::vg)

inline bool switchChildPasses(const svg_element* child)
{
    auto trim = [](std::string_view v) {
        while(!v.empty() && std::isspace(static_cast<unsigned char>(v.front())))
            v.remove_prefix(1);
        while(!v.empty() && std::isspace(static_cast<unsigned char>(v.back())))
            v.remove_suffix(1);
        return v;
    };
    if(child->has_attribute(PropertyID::RequiredExtensions)) {
        std::string_view value = trim(child->attribute(PropertyID::RequiredExtensions));
        bool supported = false;
        size_t pos = 0;
        while(pos < value.size() && !supported) {
            while(pos < value.size() && std::isspace(static_cast<unsigned char>(value[pos])))
                ++pos;
            auto end = pos;
            while(end < value.size() && !std::isspace(static_cast<unsigned char>(value[end])))
                ++end;
            supported = value.substr(pos, end - pos) == "http://www.w3.org/1999/xhtml";
            pos = end;
        }
        if(!supported)
            return false;
    }
    if(child->has_attribute(PropertyID::SystemLanguage)) {
        std::string_view value = child->attribute(PropertyID::SystemLanguage);
        bool matches = false;
        size_t pos = 0;
        while(pos <= value.size() && !matches) {
            auto comma = value.find(',', pos);
            auto tag = trim(value.substr(pos, comma == std::string_view::npos ? std::string_view::npos : comma - pos));
            std::string lower(tag);
            for(auto& c : lower)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
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

inline const svg_geometry_element* toSVGGeometryElement(const svg_node* nd)
{
    if(nd && nd->isGeometryElement())
        return static_cast<const svg_geometry_element*>(nd);
    return nullptr;
}

// ---- svgfilterelement (impl) ----

} // namespace nova::svg
