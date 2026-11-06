// Colours are stb::color (libstb, <stb/color.hpp>): RGBA, 8 bits per channel, hex / CSS-name parsing, the `_c` literal in
// stb::color_literals, stb::invalid_color_error. Only what the drawing code needs on top of that lives here, as free
// functions, because stb::color is not ours to extend.
#pragma once

#include <stb/color.hpp>

#include <cstdint>

namespace nova::vg {

inline float red_f(const stb::color& c) { return c.r() / 255.f; }
inline float green_f(const stb::color& c) { return c.g() / 255.f; }
inline float blue_f(const stb::color& c) { return c.b() / 255.f; }
inline float alpha_f(const stb::color& c) { return c.a() / 255.f; }

inline bool is_opaque(const stb::color& c) { return c.a() == 255; }
inline bool is_visible(const stb::color& c) { return c.a() > 0; }

// The same colour with alpha 255.
inline stb::color opaque(const stb::color& c) { return stb::color(c.r(), c.g(), c.b(), 255); }

// The same colour with its alpha multiplied by `opacity` (0..1), truncated like the former Color::colorWithAlpha().
inline stb::color with_alpha(const stb::color& c, float opacity)
{
    return stb::color(c.r(), c.g(), c.b(), static_cast<uint8_t>(c.a() * opacity));
}

}  // namespace nova::vg
