#pragma once

// Private to src/sources/vg: a font face (one font of a file or of a memory block), on top of stb::font (libstb).
// Reference counted by hand for now (the C canvas layer shares faces through raw pointers); it becomes a plain
// value or a shared_ptr when that layer goes (ADR-0011, step 3).

#include "render.h"

#include <stb/font.hpp>

#include <stdint.h>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#endif

#include "internal.h"  // ref_count_t and the novasvg_*_reference macros

namespace nova {
namespace vg {

#if defined(_WIN32)

typedef CRITICAL_SECTION mutex_t;

#define novasvg_mutex_init(mutex) initialize_critical_section(mutex)
#define novasvg_mutex_lock(mutex) enter_critical_section(mutex)
#define novasvg_mutex_unlock(mutex) leave_critical_section(mutex)
#define novasvg_mutex_destroy(mutex) delete_critical_section(mutex)

#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && defined(HAVE_THREADS_H) && !defined(__STDC_NO_THREADS__)

typedef mtx_t mutex_t;

#define novasvg_mutex_init(mutex) mtx_init(mutex, mtx_plain | mtx_recursive)
#define novasvg_mutex_lock(mutex) mtx_lock(mutex)
#define novasvg_mutex_unlock(mutex) mtx_unlock(mutex)
#define novasvg_mutex_destroy(mutex) mtx_destroy(mutex)

#else

typedef int mutex_t;

#define novasvg_mutex_init(mutex) ((void)(mutex))
#define novasvg_mutex_lock(mutex) ((void)(mutex))
#define novasvg_mutex_unlock(mutex) ((void)(mutex))
#define novasvg_mutex_destroy(mutex) ((void)(mutex))

#endif

// A text shaped by libstb (font::shape): the glyphs to draw, in visual order, with the pen already placed (kerning,
// ligatures, joined and mirrored letters, marks), in pixels at the font size, y down.
struct shaped_glyph_t {
    std::uint32_t glyph; // glyph index of the font
    float x;             // where the glyph is drawn, from the start of the text
    float y;             // offset from the baseline (mark placement)
};

// What is cached per code point: its glyph index, horizontal metrics, ink box and outline, all in
// font units. Entries are never removed while the face lives, so pointers stay valid.
struct glyph_entry {
    uint32_t index = 0;
    float advance_width = 0;
    float left_side_bearing = 0;
    float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    std::vector<stb::outline_command> outline;
};

class font_face_data {
public:
    // A null pointer when the file or the data is not a font. `destroy_func(closure)` runs when the last
    // reference goes (also when loading fails).
    static font_face_data* load_from_file(const char* filename, int ttcindex);
    static font_face_data* load_from_data(const void* data, unsigned int length, int ttcindex, destroy_func_t destroy_func, void* closure);

    // Null-safe: reference() adds one and returns `face`; release() drops one and deletes at zero.
    static font_face_data* reference(font_face_data* face);
    static void release(font_face_data* face);
    static int reference_count_of(const font_face_data* face);

    ~font_face_data();
    font_face_data(const font_face_data&) = delete;
    font_face_data& operator=(const font_face_data&) = delete;

    // Design units per em (the font's own grid), e.g. 1000 or 2048.
    float units_per_em() const;

    // Calls `callback` for every Unicode code point the face has a glyph for, in ascending order.
    void enumerate_codepoints(void (*callback)(uint32_t codepoint, void* closure), void* closure) const;

    void get_metrics(float size_, float* ascent, float* descent, float* line_gap, rect_t* extents) const;
    void get_glyph_metrics(float size_, codepoint_t codepoint, float* advance_width, float* left_side_bearing, rect_t* extents) const;
    void get_glyph_index_metrics(float size_, std::uint32_t glyph_index, float* advance_width, float* left_side_bearing, rect_t* extents) const;

    // Kerning between two consecutive code points, in pixels at `size_` (kern table or GPOS).
    float get_kern_advance(float size_, codepoint_t ch1, codepoint_t ch2) const;

    // The outline of a glyph at (x, y); returns its own advance.
    float get_glyph_path(float size_, float x, float y, codepoint_t codepoint, stb::outline& path_) const;
    float traverse_glyph_path(float size_, float x, float y, codepoint_t codepoint, path_traverse_func_t traverse_func, void* closure) const;
    // The same by glyph index (what shape() returns).
    float get_glyph_index_path(float size_, float x, float y, std::uint32_t glyph, stb::outline& path_) const;
    float traverse_glyph_index_path(float size_, float x, float y, std::uint32_t glyph_index, path_traverse_func_t traverse_func, void* closure) const;

    // Shapes `text`; fills `glyphs` (when not null) and returns the advance of the whole text. UTF-8, UTF-16 or UTF-32.
    float shape(float size_, const void* text, int length_, text_encoding encoding, std::vector<shaped_glyph_t>* glyphs) const;

    // The advance of `text`, and (when not null) its bounding box.
    float text_extents(float size_, const void* text, int length_, text_encoding encoding, rect_t* extents) const;

private:
    explicit font_face_data(stb::font f);

    uint32_t glyph_index_of(codepoint_t codepoint) const;
    glyph_entry* glyph_by_index(uint32_t index) const;
    glyph_entry* glyph(codepoint_t codepoint) const;
    float size_scale(float size_) const;

    mutable ref_count_t ref_count;
    stb::font ft;
    stb::font_design_metrics design;
    mutable mutex_t mutex;
    mutable std::unordered_map<codepoint_t, std::unique_ptr<glyph_entry>> cache;
    destroy_func_t destroy_func = nullptr;
    void* closure = nullptr;
};

} // namespace vg
} // namespace nova
