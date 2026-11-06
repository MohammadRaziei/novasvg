// Definitions declared in svgparser.hpp.
#include "svgelement.h"
#include "svgparser.hpp"

namespace nova::svg {
using namespace vg;

bool rule_data::match(const svg_element* el) const
{
    return match_selector(m_selector, el);
}

} // namespace nova::svg
