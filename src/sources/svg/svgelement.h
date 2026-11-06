#pragma once

// The svg DOM and its renderer, one header per layer; each includes the one before it:
//   svg_property -> svg_node -> svg_elements -> svg_paint -> svg_shapes -> svg_text -> svg_layout_state
//   -> svg_impl -> svg_element_helpers
// This umbrella keeps `#include "svgelement.h"` working.
#include "svg_element_helpers.h"
#include "svg_parse.h"
