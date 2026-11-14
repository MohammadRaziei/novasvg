// Font faces and the font-face cache, built on stb::font (libstb). Replaces the inlined
// stb_truetype that used to live in include/novasvg/gfx/font_impl.h (ADR-0006).
//
// libstb reads outlines, metrics, kerning (kern/GPOS), GSUB shaping, and what a font file is called (family, weight,
// width, bold, italic: font::scan and font::system_fonts); this file only keeps the cache that picks a face for a request.
#include "font_face.h"
#include "internal.h"
#include "../utils/text.h"
#include <stb/logging.hpp>

#include <stb/font.hpp>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <cstring>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && defined(HAVE_THREADS_H) && !defined(__STDC_NO_THREADS__)
#include <threads.h>
#endif

namespace nova {
namespace vg {
namespace {
stb::logging::logger& font_log() { static auto& l = stb::logging::get_logger("novasvg.font"); return l; }
}  // namespace

static int text_iterator_length(const void* data, text_encoding encoding)
{
    int length_ = 0;
    switch(encoding) {
    case text_encoding::latin1:
    case text_encoding::utf8: {
        const uint8_t* text = static_cast<const uint8_t*>(data);
        while(*text++)
            length_++;
        break;
    } case text_encoding::utf16: {
        const uint16_t* text = static_cast<const uint16_t*>(data);
        while(*text++)
            length_++;
        break;
    } case text_encoding::utf32: {
        const uint32_t* text = static_cast<const uint32_t*>(data);
        while(*text++)
            length_++;
        break;
    } default:
        assert(false);
    }

    return length_;
}

void text_iterator_init(text_iterator_t* it, const void* text, int length_, text_encoding encoding)
{
    if(length_ == -1)
        length_ = text_iterator_length(text, encoding);
    it->text = text;
    it->length_ = length_;
    it->encoding = encoding;
    it->index = 0;
}

bool text_iterator_has_next(const text_iterator_t* it)
{
    return it->index < it->length_;
}

codepoint_t text_iterator_next(text_iterator_t* it)
{
    codepoint_t codepoint = 0;
    switch(it->encoding) {
    case text_encoding::latin1: {
        const uint8_t* text = static_cast<const uint8_t*>(it->text);
        codepoint = text[it->index++];
        break;
    } case text_encoding::utf8: {
        static const uint8_t trailing[256] = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
        };

        static const uint32_t offsets[6] = {
            0x00000000, 0x00003080, 0x000E2080, 0x03C82080, 0xFA082080, 0x82082080
        };

        const uint8_t* text = static_cast<const uint8_t*>(it->text);
        uint8_t trailing_offset = trailing[text[it->index]];
        uint32_t offset_value = offsets[trailing_offset];
        while(trailing_offset > 0 && it->index < it->length_ - 1) {
            codepoint += text[it->index++];
            codepoint <<= 6;
            trailing_offset--;
        }

        codepoint += text[it->index++];
        codepoint -= offset_value;
        break;
    } case text_encoding::utf16: {
        const uint16_t* text = static_cast<const uint16_t*>(it->text);
        codepoint = text[it->index++];
        if(((codepoint) & 0xfffffc00) == 0xd800) {
            if(it->index < it->length_ && (((codepoint) & 0xfffffc00) == 0xdc00)) {
                uint16_t trail = text[it->index++];
                codepoint = (codepoint << 10) + trail - ((0xD800u << 10) - 0x10000u + 0xDC00u);
            }
        }

        break;
    } case text_encoding::utf32: {
        const uint32_t* text = static_cast<const uint32_t*>(it->text);
        codepoint = text[it->index++];
        break;
    } default:
        assert(false);
    }

    return codepoint;
}

font_face_data::font_face_data(stb::font f)
    : ft(std::move(f)), design(ft.design_metrics())
{
    novasvg_init_reference(this);
}

font_face_data::~font_face_data()
{
    if(destroy_func)
        destroy_func(closure);
}

uint32_t font_face_data::glyph_index_of(codepoint_t codepoint) const
{
    // stb::font::glyph_index throws above U+10FFFF; such a code point is just "not in the font".
    return codepoint > 0x10FFFFu ? 0u : ft.glyph_index(static_cast<char32_t>(codepoint));
}

glyph_entry* font_face_data::glyph_by_index(uint32_t index) const
{
    std::lock_guard<std::mutex> lock(mutex);
    auto& slot = cache[index];
    if(!slot) {
        auto glyph = std::make_unique<glyph_entry>();
        glyph->index = index;
        stb::glyph_metrics info = ft.glyph_info(glyph->index);
        glyph->advance_width = info.advance;
        glyph->left_side_bearing = info.lsb;
        glyph->x1 = info.x_min;
        glyph->y1 = info.y_min;
        glyph->x2 = info.x_max;
        glyph->y2 = info.y_max;
        ft.outline(glyph->index, glyph->outline);
        slot = std::move(glyph);
    }
    return slot.get();
}

glyph_entry* font_face_data::glyph(codepoint_t codepoint) const
{
    return glyph_by_index(glyph_index_of(codepoint));
}

font_face_data* font_face_data::load_from_file(const char* filename, int ttcindex)
{
    FILE* fp = fopen(filename, "rb");
    if(fp == NULL) {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long length_ = ftell(fp);
    if(length_ == -1L) {
        fclose(fp);
        return NULL;
    }

    void* data = static_cast<void*>(malloc(length_));
    if(data == NULL) {
        fclose(fp);
        return NULL;
    }

    fseek(fp, 0, SEEK_SET);
    size_t nread = fread(data, 1, length_, fp);
    fclose(fp);

    if(nread != static_cast<size_t>(length_)) {
        free(data);
        return NULL;
    }

    return load_from_data(data, length_, ttcindex, free, data);
}

font_face_data* font_face_data::load_from_data(const void* data, unsigned int length_, int ttcindex, destroy_func_t destroy_func, void* closure)
{
    font_face_data* face = NULL;
    try {
        const auto* bytes = static_cast<const std::uint8_t*>(data);
        face = new font_face_data(stb::font::from_memory(std::vector<std::uint8_t>(bytes, bytes + length_), ttcindex));
    } catch(const std::exception& e) {
        font_log().warning("cannot load a font from memory (%u bytes, index %d): %s", length_, ttcindex, e.what());
        if(destroy_func)
            destroy_func(closure);
        return NULL;
    } catch(...) {
        font_log().warning("cannot load a font from memory (%u bytes, index %d)", length_, ttcindex);
        if(destroy_func)
            destroy_func(closure);
        return NULL;
    }

    face->destroy_func = destroy_func;
    face->closure = closure;
    return face;
}

font_face_data* font_face_data::reference(font_face_data* face)
{
    novasvg_increment_reference(face);
    return face;
}

void font_face_data::release(font_face_data* face)
{
    if(novasvg_destroy_reference(face))
        delete face;
}

int font_face_data::reference_count_of(const font_face_data* face)
{
    return novasvg_get_reference_count(face);
}

float font_face_data::size_scale(float size_) const
{
    return size_ / static_cast<float>(design.units_per_em);
}

float font_face_data::units_per_em() const
{
    return static_cast<float>(design.units_per_em);
}

// Every code point the face draws with a real glyph (glyph 0, .notdef, does not count). libstb has no
// cmap walker, so every Unicode scalar value is asked; has_glyph is one table lookup.
void font_face_data::enumerate_codepoints(void (*callback)(uint32_t codepoint, void* closure), void* closure) const
{
    for(uint32_t cp = 0; cp <= 0x10FFFFu; ++cp) {
        if(cp >= 0xD800u && cp <= 0xDFFFu)
            continue;
        if(ft.has_glyph(static_cast<char32_t>(cp)))
            callback(cp, closure);
    }
}

void font_face_data::get_metrics(float size_, float* ascent, float* descent, float* line_gap, rect_t* extents) const
{
    float scale = size_scale(size_);
    if(ascent) *ascent = design.ascent * scale;
    if(descent) *descent = design.descent * scale;
    if(line_gap) *line_gap = design.line_gap * scale;
    if(extents) {
        extents->x = design.x_min * scale;
        extents->y = design.y_max * -scale;
        extents->w = (design.x_max - design.x_min) * scale;
        extents->h = (design.y_min - design.y_max) * -scale;
    }
}

void font_face_data::get_glyph_metrics(float size_, codepoint_t codepoint, float* advance_width, float* left_side_bearing, rect_t* extents) const
{
    get_glyph_index_metrics(size_, glyph_index_of(codepoint), advance_width, left_side_bearing, extents);
}

void font_face_data::get_glyph_index_metrics(float size_, std::uint32_t glyph_index, float* advance_width, float* left_side_bearing, rect_t* extents) const
{
    float scale = size_scale(size_);
    glyph_entry* glyph = glyph_by_index(glyph_index);
    if(advance_width) *advance_width = glyph->advance_width * scale;
    if(left_side_bearing) *left_side_bearing = glyph->left_side_bearing * scale;
    if(extents) {
        extents->x = glyph->x1 * scale;
        extents->y = glyph->y2 * -scale;
        extents->w = (glyph->x2 - glyph->x1) * scale;
        extents->h = (glyph->y1 - glyph->y2) * -scale;
    }
}

// Pairwise kerning between two consecutive code points, from the font's own kern/GPOS data.
float font_face_data::get_kern_advance(float size_, codepoint_t ch1, codepoint_t ch2) const
{
    float scale = size_scale(size_);
    uint32_t a = glyph(ch1)->index;
    uint32_t b = glyph(ch2)->index;
    return ft.glyph_kerning(a, b) * scale;
}

static void glyph_traverse_func(void* closure, path_command command, const stb::point* points, int npoints)
{
    outline* path_ = static_cast<outline*>(closure);
    switch(command) {
    case path_command::move_to:
        path_->move_to(points[0].x, points[0].y);
        break;
    case path_command::line_to:
        path_->line_to(points[0].x, points[0].y);
        break;
    case path_command::cubic_to:
        path_->cubic_to(points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y);
        break;
    case path_command::close:
        assert(false);
    }
}

float font_face_data::get_glyph_path(float size_, float x, float y, codepoint_t codepoint, outline& path_) const
{
    return traverse_glyph_path(size_, x, y, codepoint, glyph_traverse_func, &path_);
}

float font_face_data::traverse_glyph_path(float size_, float x, float y, codepoint_t codepoint, path_traverse_func_t traverse_func, void* closure) const
{
    return traverse_glyph_index_path(size_, x, y, glyph_index_of(codepoint), traverse_func, closure);
}

float font_face_data::get_glyph_index_path(float size_, float x, float y, std::uint32_t glyph, outline& path_) const
{
    return traverse_glyph_index_path(size_, x, y, glyph, glyph_traverse_func, &path_);
}

float font_face_data::traverse_glyph_index_path(float size_, float x, float y, std::uint32_t glyph_index, path_traverse_func_t traverse_func, void* closure) const
{
    float scale = size_scale(size_);
    stb::matrix matrix;
    matrix = stb::matrix::translation(x, y);
    matrix.scale(scale, -scale);

    stb::point points[3];
    stb::point current_point = {0, 0};
    glyph_entry* glyph = glyph_by_index(glyph_index);
    for(const stb::outline_command& cmd : glyph->outline) {
        switch(cmd.op) {
        case outline_op::move_to:
            points[0].x = cmd.p[0].x;
            points[0].y = cmd.p[0].y;
            current_point = points[0];
            matrix.map(points, points, 1);
            traverse_func(closure, path_command::move_to, points, 1);
            break;
        case outline_op::line_to:
            points[0].x = cmd.p[0].x;
            points[0].y = cmd.p[0].y;
            current_point = points[0];
            matrix.map(points, points, 1);
            traverse_func(closure, path_command::line_to, points, 1);
            break;
        case outline_op::quad_to: // TrueType: raised to a cubic, as the renderer only knows cubics
            points[0].x = 2.f / 3.f * cmd.p[0].x + 1.f / 3.f * current_point.x;
            points[0].y = 2.f / 3.f * cmd.p[0].y + 1.f / 3.f * current_point.y;
            points[1].x = 2.f / 3.f * cmd.p[0].x + 1.f / 3.f * cmd.p[1].x;
            points[1].y = 2.f / 3.f * cmd.p[0].y + 1.f / 3.f * cmd.p[1].y;
            points[2].x = cmd.p[1].x;
            points[2].y = cmd.p[1].y;
            current_point = points[2];
            matrix.map(points, points, 3);
            traverse_func(closure, path_command::cubic_to, points, 3);
            break;
        case outline_op::cubic_to: // CFF
            points[0].x = cmd.p[0].x;
            points[0].y = cmd.p[0].y;
            points[1].x = cmd.p[1].x;
            points[1].y = cmd.p[1].y;
            points[2].x = cmd.p[2].x;
            points[2].y = cmd.p[2].y;
            current_point = points[2];
            matrix.map(points, points, 3);
            traverse_func(closure, path_command::cubic_to, points, 3);
            break;
        case outline_op::close:
            // Contours were never closed explicitly here (stb_truetype gave none): the fill closes
            // them anyway, and this keeps stroked text identical.
            break;
        }
    }

    return glyph->advance_width * scale;
}

static std::string text_to_utf8(const void* text, int length_, text_encoding encoding)
{
    std::string utf8;
    text_iterator_t it;
    text_iterator_init(&it, text, length_, encoding);
    while(text_iterator_has_next(&it)) {
        const codepoint_t c = text_iterator_next(&it);
        if(c < 0x80) {
            utf8.push_back(static_cast<char>(c));
        } else if(c < 0x800) {
            utf8.push_back(static_cast<char>(0xC0 | (c >> 6)));
            utf8.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        } else if(c < 0x10000) {
            utf8.push_back(static_cast<char>(0xE0 | (c >> 12)));
            utf8.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
            utf8.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        } else {
            utf8.push_back(static_cast<char>(0xF0 | ((c >> 18) & 0x07)));
            utf8.push_back(static_cast<char>(0x80 | ((c >> 12) & 0x3F)));
            utf8.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
            utf8.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        }
    }
    return utf8;
}

float font_face_data::shape(float size_, const void* text, int length_, text_encoding encoding, std::vector<shaped_glyph_t>* glyphs, const shaping_context& context) const
{
    if(glyphs)
        glyphs->clear();
    const float scale = size_scale(size_);
    std::vector<stb::shaped_glyph> shaped;
    try {
        stb::shape_options options;
        const auto* part = static_cast<const char32_t*>(text);
        if(!context.run.empty() && encoding == text_encoding::utf32 && length_ >= 0
           && part >= context.run.data() && part + length_ <= context.run.data() + context.run.size()) {
            // A part of a longer run: libstb shapes [begin, end) with the context of the whole run, and its byte offsets
            // are those of the run's UTF-8.
            options.ligatures = context.ligatures;
            const size_t begin = size_t(part - context.run.data());
            const std::string utf8 = text_to_utf8(context.run.data(), int(context.run.size()), text_encoding::utf32);
            const size_t begin_byte = text_to_utf8(context.run.data(), int(begin), text_encoding::utf32).size();
            const size_t end_byte = text_to_utf8(context.run.data(), int(begin) + length_, text_encoding::utf32).size();
            shaped = ft.shape(utf8, begin_byte, end_byte, options);
        } else {
            shaped = ft.shape(text_to_utf8(text, length_, encoding));
        }
    } catch(...) {
        return 0.f;
    }
    float pen = 0.f;
    for(const stb::shaped_glyph& g : shaped) {
        if(glyphs)
            glyphs->push_back({g.glyph, pen + g.x_offset * scale, -g.y_offset * scale});
        pen += g.x_advance * scale;
    }
    return pen;
}

float font_face_data::text_extents(float size_, const void* text, int length_, text_encoding encoding, rect_t* extents, const shaping_context& context) const
{
    std::vector<shaped_glyph_t> glyphs;
    const float advance = shape(size_, text, length_, encoding, extents ? &glyphs : nullptr, context);
    if(!extents)
        return advance;

    const float scale = size_scale(size_);
    bool first = true;
    float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    for(const shaped_glyph_t& g : glyphs) {
        const glyph_entry* entry = glyph_by_index(g.glyph);
        const float gx1 = g.x + entry->x1 * scale, gx2 = g.x + entry->x2 * scale;
        const float gy1 = g.y - entry->y2 * scale, gy2 = g.y - entry->y1 * scale;
        if(first) {
            x1 = gx1; x2 = gx2; y1 = gy1; y2 = gy2; first = false;
            continue;
        }
        x1 = STB_MIN(x1, gx1); y1 = STB_MIN(y1, gy1); x2 = STB_MAX(x2, gx2); y2 = STB_MAX(y2, gy2);
    }
    extents->x = first ? 0.f : x1;
    extents->y = first ? 0.f : y1;
    extents->w = first ? 0.f : x2 - x1;
    extents->h = first ? 0.f : y2 - y1;
    return advance;
}

// One face of a family: either loaded already (`face` holds one reference) or only known by its file, loaded on first use.
// The face itself, loaded on first use. A face listed under two family names (the typographic one and the legacy one)
// has two entries and one slot, so it is loaded once and its glyph cache is shared.
struct font_face_slot {
    font_face_data* face = nullptr;
    std::string filename;   // empty for a face that was added directly
    int ttcindex = 0;
    ~font_face_slot() { font_face_data::release(face); }
};

struct font_face_entry {
    std::shared_ptr<font_face_slot> slot;
    bool bold = false;
    bool italic = false;
    int weight = 400;       // OS/2 usWeightClass
    int width = 5;          // OS/2 usWidthClass, 5 is normal
};

struct font_face_cache_impl {
    ref_count_t ref_count;
    std::mutex mutex;
    std::map<std::string, std::vector<font_face_entry>> families;   // the faces of a family, oldest first
};

font_face_cache_t* font_face_cache_create(void)
{
    font_face_cache_t* cache = new font_face_cache_impl;
    novasvg_init_reference(cache);
    return cache;
}

font_face_cache_t* font_face_cache_reference(font_face_cache_t* cache)
{
    novasvg_increment_reference(cache);
    return cache;
}

void font_face_cache_destroy(font_face_cache_t* cache)
{
    if(novasvg_destroy_reference(cache)) {
        font_face_cache_reset(cache);
        delete cache;
    }
}

int font_face_cache_reference_count(const font_face_cache_t* cache)
{
    return novasvg_get_reference_count(cache);
}

void font_face_cache_reset(font_face_cache_t* cache)
{
    std::lock_guard<std::mutex> lock(cache->mutex);
    cache->families.clear();
}

static void font_face_cache_add_entry(font_face_cache_t* cache, const std::string& family, font_face_entry entry)
{
    std::lock_guard<std::mutex> lock(cache->mutex);
    cache->families[family].push_back(std::move(entry));
}

void font_face_cache_add(font_face_cache_t* cache, const char* family, bool bold, bool italic, font_face_data* face)
{
    font_face_entry entry;
    entry.slot = std::make_shared<font_face_slot>();
    entry.slot->face = font_face_data::reference(face);
    entry.bold = bold;
    entry.italic = italic;
    entry.weight = bold ? 700 : 400;
    font_face_cache_add_entry(cache, family ? family : "", std::move(entry));
}

bool font_face_cache_add_file(font_face_cache_t* cache, const char* family, bool bold, bool italic, const char* filename, int ttcindex)
{
    font_face_data* face = font_face_data::load_from_file(filename, ttcindex);
    if(face == NULL) {
        font_log().warning("cannot add '%s' to the font cache (family '%s')", filename, family);
        return false;
    }
    font_face_cache_add(cache, family, bold, italic, face);
    font_face_data::release(face);
    return true;
}

// The better of two faces of a family for a request: the one that agrees on bold and italic, then the one nearest the
// normal width, then the one whose weight is nearest to regular (400) or bold (700). The first argument wins only if it is better.
static bool font_face_entry_better(const font_face_entry& a, const font_face_entry& b, bool bold, bool italic)
{
    const int a_score = (bold == a.bold) + (italic == a.italic);
    const int b_score = (bold == b.bold) + (italic == b.italic);
    if(a_score != b_score)
        return a_score > b_score;
    if(std::abs(a.width - 5) != std::abs(b.width - 5))
        return std::abs(a.width - 5) < std::abs(b.width - 5);
    const int target = bold ? 700 : 400;
    return std::abs(a.weight - target) < std::abs(b.weight - target);
}

font_face_data* font_face_cache_get(font_face_cache_t* cache, const char* family, bool bold, bool italic)
{
    std::lock_guard<std::mutex> lock(cache->mutex);

    auto found = cache->families.find(family);
    if(found == cache->families.end())
        return NULL;

    // The newest face wins a tie, so walk from the newest to the oldest and replace only on a better score.
    std::vector<font_face_entry>& faces = found->second;
    font_face_entry* selected = &faces.back();
    for(std::size_t i = faces.size() - 1; i-- > 0;) {
        if(font_face_entry_better(faces[i], *selected, bold, italic))
            selected = &faces[i];
    }

    font_face_slot& slot = *selected->slot;
    if(!slot.filename.empty() && slot.face == NULL)
        slot.face = font_face_data::load_from_file(slot.filename.c_str(), slot.ttcindex);
    return slot.face;
}

#ifndef NOVA_DISABLE_FONT_FACE_CACHE_LOAD

// A face found by libstb goes in under its family (name id 16, "DejaVu Sans") and, when that differs, under the name a
// program that knows only regular and bold uses (id 1, "DejaVu Sans Light"): font-family can say either.
static int font_face_cache_add_found(font_face_cache_t* cache, const std::filesystem::path& path, const stb::font_face_info& info)
{
    font_face_entry entry;
    entry.slot = std::make_shared<font_face_slot>();
    entry.slot->filename = path.string();
    entry.slot->ttcindex = info.index;
    entry.bold = info.bold;
    entry.italic = info.italic;
    entry.weight = info.weight;
    entry.width = info.width;

    int added = 0;
    if(!info.family_name.empty()) {
        font_face_cache_add_entry(cache, info.family_name, entry);
        added = 1;
    }
    if(!info.legacy_family_name.empty() && info.legacy_family_name != info.family_name) {
        font_face_cache_add_entry(cache, info.legacy_family_name, entry);
        added = 1;
    }
    return added;
}

int font_face_cache_load_file(font_face_cache_t* cache, const char* filename)
{
    try {
        const std::filesystem::path path(filename);
        int num_faces = 0;
        for(const stb::font_face_info& info : stb::font::scan(path))
            num_faces += font_face_cache_add_found(cache, path, info);
        return num_faces;
    } catch(const std::exception&) {
        return 0; // not a font, or not readable
    }
}

static int font_face_cache_load_found(font_face_cache_t* cache, const std::vector<stb::system_font_info>& found)
{
    int num_faces = 0;
    for(const stb::system_font_info& info : found)
        num_faces += font_face_cache_add_found(cache, info.path, info);
    return num_faces;
}

int font_face_cache_load_dir(font_face_cache_t* cache, const char* dirname)
{
    return font_face_cache_load_found(cache, stb::font::system_fonts({std::filesystem::path(dirname)}));
}

int font_face_cache_load_sys(font_face_cache_t* cache)
{
    return font_face_cache_load_found(cache, stb::font::system_fonts());
}

#else

int font_face_cache_load_file(font_face_cache_t* cache, const char* filename)
{
    return -1;
}

int font_face_cache_load_dir(font_face_cache_t* cache, const char* dirname)
{
    return -1;
}

int font_face_cache_load_sys(font_face_cache_t* cache)
{
    return -1;
}

#endif // NOVA_DISABLE_FONT_FACE_CACHE_LOAD

} // namespace vg
} // namespace nova
