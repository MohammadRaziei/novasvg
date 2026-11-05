// Definitions declared in svgparser.hpp.
#include "svgelement.h"
#include "svgparser.hpp"

namespace nova::svg {
using namespace vg;

bool RuleData::match(const svg_element* el) const
{
    return matchSelector(m_selector, el);
}

} // namespace nova::svg
