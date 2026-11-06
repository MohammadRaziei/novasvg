// Font faces and the font-face cache, built on stb::font (libstb). Replaces the inlined
// stb_truetype that used to live in include/novasvg/gfx/font_impl.h (ADR-0006).
//
// libstb reads outlines, metrics, kerning (kern/GPOS) and GSUB shaping. It has no API for the `name`
// table or `head.macStyle`, so the small system-font scanner below reads those two tables itself.
#include "font_face.h"
#include "internal.h"
#include <stb/logging.hpp>

#include <stb/font.hpp>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#ifndef NOVA_DISABLE_FONT_FACE_CACHE_LOAD
#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#ifdef __linux__
#include <linux/limits.h>
#else
#include <limits.h>
#endif
#include <sys/mman.h>
#include <sys/stat.h>
#endif
#endif

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

static int text_iterator_length(const void* data, text_encoding_t encoding)
{
    int length = 0;
    switch(encoding) {
    case NOVA_TEXT_ENCODING_LATIN1:
    case NOVA_TEXT_ENCODING_UTF8: {
        const uint8_t* text = static_cast<const uint8_t*>(data);
        while(*text++)
            length++;
        break;
    } case NOVA_TEXT_ENCODING_UTF16: {
        const uint16_t* text = static_cast<const uint16_t*>(data);
        while(*text++)
            length++;
        break;
    } case NOVA_TEXT_ENCODING_UTF32: {
        const uint32_t* text = static_cast<const uint32_t*>(data);
        while(*text++)
            length++;
        break;
    } default:
        assert(false);
    }

    return length;
}

void text_iterator_init(text_iterator_t* it, const void* text, int length, text_encoding_t encoding)
{
    if(length == -1)
        length = text_iterator_length(text, encoding);
    it->text = text;
    it->length = length;
    it->encoding = encoding;
    it->index = 0;
}

bool text_iterator_has_next(const text_iterator_t* it)
{
    return it->index < it->length;
}

codepoint_t text_iterator_next(text_iterator_t* it)
{
    codepoint_t codepoint = 0;
    switch(it->encoding) {
    case NOVA_TEXT_ENCODING_LATIN1: {
        const uint8_t* text = static_cast<const uint8_t*>(it->text);
        codepoint = text[it->index++];
        break;
    } case NOVA_TEXT_ENCODING_UTF8: {
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
        while(trailing_offset > 0 && it->index < it->length - 1) {
            codepoint += text[it->index++];
            codepoint <<= 6;
            trailing_offset--;
        }

        codepoint += text[it->index++];
        codepoint -= offset_value;
        break;
    } case NOVA_TEXT_ENCODING_UTF16: {
        const uint16_t* text = static_cast<const uint16_t*>(it->text);
        codepoint = text[it->index++];
        if(((codepoint) & 0xfffffc00) == 0xd800) {
            if(it->index < it->length && (((codepoint) & 0xfffffc00) == 0xdc00)) {
                uint16_t trail = text[it->index++];
                codepoint = (codepoint << 10) + trail - ((0xD800u << 10) - 0x10000u + 0xDC00u);
            }
        }

        break;
    } case NOVA_TEXT_ENCODING_UTF32: {
        const uint32_t* text = static_cast<const uint32_t*>(it->text);
        codepoint = text[it->index++];
        break;
    } default:
        assert(false);
    }

    return codepoint;
}

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

namespace {

// What is cached per code point: its glyph index, horizontal metrics, ink box and outline, all in
// font units. Entries are never removed while the face lives, so pointers stay valid.
struct glyph_entry {
    uint32_t index = 0;
    float advance_width = 0;
    float left_side_bearing = 0;
    float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    std::vector<stb::outline_command> outline;
};

} // namespace

struct font_face {
    ref_count_t ref_count;
    stb::font ft;
    stb::font_design_metrics design;
    mutex_t mutex;
    std::unordered_map<codepoint_t, std::unique_ptr<glyph_entry>> cache;
    destroy_func_t destroy_func = nullptr;
    void* closure = nullptr;
    // Ligatures the font's GSUB `liga`/`clig` applies to f-sequences (see
    // font_face_apply_ligature()); built lazily, once, under `mutex`.
    int ligatures_ready = 0;
    int ligature_count = 0;
    struct {
        codepoint_t seq[3];
        int len;
        codepoint_t ligature;
    } ligatures[5];

    explicit font_face(stb::font f) : ft(std::move(f)), design(ft.design_metrics()) {}
};

static uint32_t glyph_index_of(const font_face_t* face, codepoint_t codepoint)
{
    // stb::font::glyph_index throws above U+10FFFF; such a code point is just "not in the font".
    return codepoint > 0x10FFFFu ? 0u : face->ft.glyph_index(static_cast<char32_t>(codepoint));
}

static glyph_entry* font_face_get_glyph(font_face_t* face, codepoint_t codepoint)
{
    novasvg_mutex_lock(&face->mutex);
    auto& slot = face->cache[codepoint];
    if(!slot) {
        auto glyph = std::make_unique<glyph_entry>();
        glyph->index = glyph_index_of(face, codepoint);
        stb::glyph_metrics info = face->ft.glyph_info(glyph->index);
        glyph->advance_width = info.advance;
        glyph->left_side_bearing = info.lsb;
        glyph->x1 = info.x_min;
        glyph->y1 = info.y_min;
        glyph->x2 = info.x_max;
        glyph->y2 = info.y_max;
        face->ft.outline(glyph->index, glyph->outline);
        slot = std::move(glyph);
    }
    glyph_entry* result = slot.get();
    novasvg_mutex_unlock(&face->mutex);
    return result;
}

font_face_t* font_face_load_from_file(const char* filename, int ttcindex)
{
    FILE* fp = fopen(filename, "rb");
    if(fp == NULL) {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    if(length == -1L) {
        fclose(fp);
        return NULL;
    }

    void* data = static_cast<void*>(malloc(length));
    if(data == NULL) {
        fclose(fp);
        return NULL;
    }

    fseek(fp, 0, SEEK_SET);
    size_t nread = fread(data, 1, length, fp);
    fclose(fp);

    if(nread != static_cast<size_t>(length)) {
        free(data);
        return NULL;
    }

    return font_face_load_from_data(data, length, ttcindex, free, data);
}

font_face_t* font_face_load_from_data(const void* data, unsigned int length, int ttcindex, destroy_func_t destroy_func, void* closure)
{
    font_face_t* face = NULL;
    try {
        const auto* bytes = static_cast<const std::uint8_t*>(data);
        face = new font_face(stb::font::from_memory(std::vector<std::uint8_t>(bytes, bytes + length), ttcindex));
    } catch(const std::exception& e) {
        font_log().warning("cannot load a font from memory (%u bytes, index %d): %s", length, ttcindex, e.what());
        if(destroy_func)
            destroy_func(closure);
        return NULL;
    } catch(...) {
        font_log().warning("cannot load a font from memory (%u bytes, index %d)", length, ttcindex);
        if(destroy_func)
            destroy_func(closure);
        return NULL;
    }

    novasvg_init_reference(face);
    novasvg_mutex_init(&face->mutex);
    face->destroy_func = destroy_func;
    face->closure = closure;
    return face;
}

font_face_t* font_face_reference(font_face_t* face)
{
    novasvg_increment_reference(face);
    return face;
}

void font_face_destroy(font_face_t* face)
{
    if(novasvg_destroy_reference(face)) {
        novasvg_mutex_destroy(&face->mutex);
        if(face->destroy_func)
            face->destroy_func(face->closure);
        delete face;
    }
}

int font_face_get_reference_count(const font_face_t* face)
{
    return novasvg_get_reference_count(face);
}

static float font_face_get_scale(const font_face_t* face, float size)
{
    return size / static_cast<float>(face->design.units_per_em);
}

float font_face_get_units_per_em(const font_face_t* face)
{
    return static_cast<float>(face->design.units_per_em);
}

// Every code point the face draws with a real glyph (glyph 0, .notdef, does not count). libstb has no
// cmap walker, so every Unicode scalar value is asked; has_glyph is one table lookup.
void font_face_enumerate_codepoints(const font_face_t* face, void (*callback)(uint32_t codepoint, void* closure), void* closure)
{
    for(uint32_t cp = 0; cp <= 0x10FFFFu; ++cp) {
        if(cp >= 0xD800u && cp <= 0xDFFFu)
            continue;
        if(face->ft.has_glyph(static_cast<char32_t>(cp)))
            callback(cp, closure);
    }
}

void font_face_get_metrics(const font_face_t* face, float size, float* ascent, float* descent, float* line_gap, rect_t* extents)
{
    float scale = font_face_get_scale(face, size);
    if(ascent) *ascent = face->design.ascent * scale;
    if(descent) *descent = face->design.descent * scale;
    if(line_gap) *line_gap = face->design.line_gap * scale;
    if(extents) {
        extents->x = face->design.x_min * scale;
        extents->y = face->design.y_max * -scale;
        extents->w = (face->design.x_max - face->design.x_min) * scale;
        extents->h = (face->design.y_min - face->design.y_max) * -scale;
    }
}

void font_face_get_glyph_metrics(font_face_t* face, float size, codepoint_t codepoint, float* advance_width, float* left_side_bearing, rect_t* extents)
{
    float scale = font_face_get_scale(face, size);
    glyph_entry* glyph = font_face_get_glyph(face, codepoint);
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
float font_face_get_kern_advance(font_face_t* face, float size, codepoint_t ch1, codepoint_t ch2)
{
    float scale = font_face_get_scale(face, size);
    uint32_t a = font_face_get_glyph(face, ch1)->index;
    uint32_t b = font_face_get_glyph(face, ch2)->index;
    return face->ft.glyph_kerning(a, b) * scale;
}

static void glyph_traverse_func(void* closure, path_command_t command, const point_t* points, int npoints)
{
    path_t* path = (path_t*)(closure);
    switch(command) {
    case NOVA_PATH_COMMAND_MOVE_TO:
        path_move_to(path, points[0].x, points[0].y);
        break;
    case NOVA_PATH_COMMAND_LINE_TO:
        path_line_to(path, points[0].x, points[0].y);
        break;
    case NOVA_PATH_COMMAND_CUBIC_TO:
        path_cubic_to(path, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y);
        break;
    case NOVA_PATH_COMMAND_CLOSE:
        assert(false);
    }
}

float font_face_get_glyph_path(font_face_t* face, float size, float x, float y, codepoint_t codepoint, path_t* path)
{
    return font_face_traverse_glyph_path(face, size, x, y, codepoint, glyph_traverse_func, path);
}

float font_face_traverse_glyph_path(font_face_t* face, float size, float x, float y, codepoint_t codepoint, path_traverse_func_t traverse_func, void* closure)
{
    float scale = font_face_get_scale(face, size);
    matrix_t matrix;
    matrix_init_translate(&matrix, x, y);
    matrix_scale(&matrix, scale, -scale);

    point_t points[3];
    point_t current_point = {0, 0};
    glyph_entry* glyph = font_face_get_glyph(face, codepoint);
    for(const stb::outline_command& cmd : glyph->outline) {
        switch(cmd.op) {
        case stb::outline_op::move_to:
            points[0].x = cmd.p[0].x;
            points[0].y = cmd.p[0].y;
            current_point = points[0];
            matrix_map_points(&matrix, points, points, 1);
            traverse_func(closure, NOVA_PATH_COMMAND_MOVE_TO, points, 1);
            break;
        case stb::outline_op::line_to:
            points[0].x = cmd.p[0].x;
            points[0].y = cmd.p[0].y;
            current_point = points[0];
            matrix_map_points(&matrix, points, points, 1);
            traverse_func(closure, NOVA_PATH_COMMAND_LINE_TO, points, 1);
            break;
        case stb::outline_op::quad_to: // TrueType: raised to a cubic, as the renderer only knows cubics
            points[0].x = 2.f / 3.f * cmd.p[0].x + 1.f / 3.f * current_point.x;
            points[0].y = 2.f / 3.f * cmd.p[0].y + 1.f / 3.f * current_point.y;
            points[1].x = 2.f / 3.f * cmd.p[0].x + 1.f / 3.f * cmd.p[1].x;
            points[1].y = 2.f / 3.f * cmd.p[0].y + 1.f / 3.f * cmd.p[1].y;
            points[2].x = cmd.p[1].x;
            points[2].y = cmd.p[1].y;
            current_point = points[2];
            matrix_map_points(&matrix, points, points, 3);
            traverse_func(closure, NOVA_PATH_COMMAND_CUBIC_TO, points, 3);
            break;
        case stb::outline_op::cubic_to: // CFF
            points[0].x = cmd.p[0].x;
            points[0].y = cmd.p[0].y;
            points[1].x = cmd.p[1].x;
            points[1].y = cmd.p[1].y;
            points[2].x = cmd.p[2].x;
            points[2].y = cmd.p[2].y;
            current_point = points[2];
            matrix_map_points(&matrix, points, points, 3);
            traverse_func(closure, NOVA_PATH_COMMAND_CUBIC_TO, points, 3);
            break;
        case stb::outline_op::close:
            // Contours were never closed explicitly here (stb_truetype gave none): the fill closes
            // them anyway, and this keeps stroked text identical.
            break;
        }
    }

    return glyph->advance_width * scale;
}

// Builds (once) the list of f-ligatures this font's own GSUB applies. A sequence is only taken when the
// glyph the shaper substitutes is the very glyph the Unicode presentation-form code point (U+FB00..FB04)
// maps to, so both measuring and painting can simply swap in that code point: a font without a
// `liga`/`clig` for the sequence (or whose ligature glyph has no such code point) is left as it was.
static void font_face_init_ligatures(font_face_t* face)
{
    static const struct { codepoint_t seq[3]; int len; codepoint_t ligature; } candidates[5] = {
        {{'f', 'f', 'i'}, 3, 0xFB03}, {{'f', 'f', 'l'}, 3, 0xFB04}, {{'f', 'f', 0}, 2, 0xFB00},
        {{'f', 'i', 0}, 2, 0xFB01}, {{'f', 'l', 0}, 2, 0xFB02},
    };
    novasvg_mutex_lock(&face->mutex);
    if(!face->ligatures_ready) {
        face->ligature_count = 0;
        for(int c = 0; c < 5; ++c) {
            bool ok = true;
            std::string text;
            for(int i = 0; i < candidates[c].len; ++i) {
                if(glyph_index_of(face, candidates[c].seq[i]) == 0)
                    ok = false;
                text.push_back(static_cast<char>(candidates[c].seq[i]));
            }
            uint32_t presentation = glyph_index_of(face, candidates[c].ligature);
            if(!ok || presentation == 0)
                continue;
            try {
                std::vector<uint32_t> shaped = face->ft.shape_glyphs(text);
                if(shaped.size() != 1 || shaped[0] != presentation)
                    continue;
            } catch(...) {
                continue;
            }
            auto& e = face->ligatures[face->ligature_count++];
            for(int i = 0; i < 3; ++i)
                e.seq[i] = candidates[c].seq[i];
            e.len = candidates[c].len;
            e.ligature = candidates[c].ligature;
        }
        face->ligatures_ready = 1;
    }
    novasvg_mutex_unlock(&face->mutex);
}

codepoint_t font_face_apply_ligature(font_face_t* face, text_iterator_t* it, codepoint_t codepoint)
{
    if(codepoint != 'f')
        return codepoint;
    if(!face->ligatures_ready)
        font_face_init_ligatures(face);
    for(int k = 0; k < face->ligature_count; ++k) {
        const auto& e = face->ligatures[k];
        text_iterator_t peek = *it;
        int matched = 1;
        for(int i = 1; i < e.len; ++i) {
            if(!text_iterator_has_next(&peek) || text_iterator_next(&peek) != e.seq[i]) {
                matched = 0;
                break;
            }
        }
        if(matched) {
            *it = peek;
            return e.ligature;
        }
    }
    return codepoint;
}

float font_face_text_extents(font_face_t* face, float size, const void* text, int length, text_encoding_t encoding, rect_t* extents)
{
    text_iterator_t it;
    text_iterator_init(&it, text, length, encoding);
    rect_t* text_extents = NULL;
    float total_advance_width = 0.f;
    codepoint_t previous = 0;
    bool has_previous = false;
    while(text_iterator_has_next(&it)) {
        codepoint_t codepoint = text_iterator_next(&it);
        codepoint = font_face_apply_ligature(face, &it, codepoint);
        if(has_previous)
            total_advance_width += font_face_get_kern_advance(face, size, previous, codepoint);
        previous = codepoint;
        has_previous = true;

        float advance_width;
        if(extents == NULL) {
            font_face_get_glyph_metrics(face, size, codepoint, &advance_width, NULL, NULL);
            total_advance_width += advance_width;
            continue;
        }

        rect_t glyph_extents;
        font_face_get_glyph_metrics(face, size, codepoint, &advance_width, NULL, &glyph_extents);

        glyph_extents.x += total_advance_width;
        total_advance_width += advance_width;
        if(text_extents == NULL) {
            text_extents = extents;
            *text_extents = glyph_extents;
            continue;
        }

        float x1 = STB_MIN(text_extents->x, glyph_extents.x);
        float y1 = STB_MIN(text_extents->y, glyph_extents.y);
        float x2 = STB_MAX(text_extents->x + text_extents->w, glyph_extents.x + glyph_extents.w);
        float y2 = STB_MAX(text_extents->y + text_extents->h, glyph_extents.y + glyph_extents.h);

        text_extents->x = x1;
        text_extents->y = y1;
        text_extents->w = x2 - x1;
        text_extents->h = y2 - y1;
    }

    if(extents && !text_extents) {
        extents->x = 0;
        extents->y = 0;
        extents->w = 0;
        extents->h = 0;
    }

    return total_advance_width;
}

typedef struct font_face_entry {
    font_face_t* face;
    char* family;
    char* filename;
    int ttcindex;
    bool bold;
    bool italic;
    struct font_face_entry* next;
} font_face_entry_t;

struct font_face_cache {
    ref_count_t ref_count;
    mutex_t mutex;
    font_face_entry_t** entries;
    int size;
    int capacity;
    bool is_sorted;
};

font_face_cache_t* font_face_cache_create(void)
{
    font_face_cache_t* cache = static_cast<font_face_cache_t*>(malloc(sizeof(font_face_cache_t)));
    novasvg_init_reference(cache);
    novasvg_mutex_init(&cache->mutex);
    cache->entries = NULL;
    cache->size = 0;
    cache->capacity = 0;
    cache->is_sorted = false;
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
        novasvg_mutex_destroy(&cache->mutex);
        free(cache);
    }
}

int font_face_cache_reference_count(const font_face_cache_t* cache)
{
    return novasvg_get_reference_count(cache);
}

void font_face_cache_reset(font_face_cache_t* cache)
{
    novasvg_mutex_lock(&cache->mutex);

    for(int i = 0; i < cache->size; ++i) {
        font_face_entry_t* entry = cache->entries[i];
        do {
            font_face_entry_t* next = entry->next;
            font_face_destroy(entry->face);
            free(entry);
            entry = next;
        } while(entry);
    }

    free(cache->entries);
    cache->entries = NULL;
    cache->size = 0;
    cache->capacity = 0;
    cache->is_sorted = false;

    novasvg_mutex_unlock(&cache->mutex);
}

static void font_face_cache_add_entry(font_face_cache_t* cache, font_face_entry_t* entry)
{
    novasvg_mutex_lock(&cache->mutex);

    for(int i = 0; i < cache->size; ++i) {
        if(strcmp(entry->family, cache->entries[i]->family) == 0) {
            entry->next = cache->entries[i];
            cache->entries[i] = entry;
            goto unlock;
        }
    }

    if(cache->size >= cache->capacity) {
        cache->capacity = cache->capacity == 0 ? 8 : cache->capacity << 2;
        cache->entries = static_cast<decltype(cache->entries)>(realloc(cache->entries, cache->capacity * sizeof(font_face_entry_t*)));
    }

    entry->next = NULL;
    cache->entries[cache->size++] = entry;
    cache->is_sorted = false;

unlock:
    novasvg_mutex_unlock(&cache->mutex);
}

void font_face_cache_add(font_face_cache_t* cache, const char* family, bool bold, bool italic, font_face_t* face)
{
    if(family == NULL) family = "";
    size_t family_length = strlen(family) + 1;

    font_face_entry_t* entry = static_cast<font_face_entry_t*>(malloc(family_length + sizeof(font_face_entry_t)));
    entry->face = font_face_reference(face);
    entry->family = (char*)(entry + 1);
    memcpy(entry->family, family, family_length);

    entry->filename = NULL;
    entry->ttcindex = 0;
    entry->bold = bold;
    entry->italic = italic;

    font_face_cache_add_entry(cache, entry);
}

bool font_face_cache_add_file(font_face_cache_t* cache, const char* family, bool bold, bool italic, const char* filename, int ttcindex)
{
    font_face_t* face = font_face_load_from_file(filename, ttcindex);
    if(face == NULL) {
        font_log().warning("cannot add '%s' to the font cache (family '%s')", filename, family);
        return false;
    }
    font_face_cache_add(cache, family, bold, italic, face);
    font_face_destroy(face);
    return true;
}

static font_face_entry_t* font_face_entry_select(font_face_entry_t* a, font_face_entry_t* b, bool bold, bool italic)
{
    int a_score = (bold == a->bold) + (italic == a->italic);
    int b_score = (bold == b->bold) + (italic == b->italic);
    return a_score > b_score ? a : b;
}

static int font_face_entry_compare(const void* a, const void* b)
{
    const font_face_entry_t* a_entry = *(const font_face_entry_t**)a;
    const font_face_entry_t* b_entry = *(const font_face_entry_t**)b;
    return strcmp(a_entry->family, b_entry->family);
}

font_face_t* font_face_cache_get(font_face_cache_t* cache, const char* family, bool bold, bool italic)
{
    novasvg_mutex_lock(&cache->mutex);

    if(!cache->is_sorted && cache->size > 0) {
        qsort(cache->entries, cache->size, sizeof(cache->entries[0]), font_face_entry_compare);
        cache->is_sorted = true;
    }

    font_face_entry_t entry_key;
    entry_key.family = (char*)(family);

    font_face_entry_t* entry_key_ptr = &entry_key;
    font_face_entry_t** entry_result = static_cast<font_face_entry_t**>(bsearch(
        &entry_key_ptr,
        cache->entries,
        cache->size,
        sizeof(cache->entries[0]),
        font_face_entry_compare
    ));

    font_face_t* face = NULL;
    if(entry_result) {
        font_face_entry_t* selected = *entry_result;
        font_face_entry_t* entry = selected->next;
        while(entry) {
            selected = font_face_entry_select(entry, selected, bold, italic);
            entry = entry->next;
        }

        if(selected->filename && selected->face == NULL)
            selected->face = font_face_load_from_file(selected->filename, selected->ttcindex);
        face = selected->face;
    }

    novasvg_mutex_unlock(&cache->mutex);
    return face;
}


#ifndef NOVA_DISABLE_FONT_FACE_CACHE_LOAD

// ---- just enough of the sfnt container to list the faces of a font file and read their family name and style
typedef uint8_t tt_uint8;
typedef uint16_t tt_uint16;
typedef int32_t tt_int32;
typedef uint32_t tt_uint32;

static tt_uint16 tt_ushort(const tt_uint8* p) { return static_cast<tt_uint16>(p[0] * 256 + p[1]); }
static tt_uint32 tt_ulong(const tt_uint8* p) { return (static_cast<tt_uint32>(p[0]) << 24) + (p[1] << 16) + (p[2] << 8) + p[3]; }
static bool tt_tag4(const tt_uint8* p, char c0, char c1, char c2, char c3) { return p[0] == c0 && p[1] == c1 && p[2] == c2 && p[3] == c3; }

static bool tt_is_font(const tt_uint8* font)
{
    return tt_tag4(font, '1', 0, 0, 0) || tt_tag4(font, 0, 1, 0, 0) || tt_tag4(font, 'O', 'T', 'T', 'O')
        || tt_tag4(font, 't', 'r', 'u', 'e') || tt_tag4(font, 't', 'y', 'p', '1');
}

static int tt_number_of_fonts(const tt_uint8* data)
{
    if(tt_is_font(data))
        return 1;
    if(tt_tag4(data, 't', 't', 'c', 'f') && (tt_ulong(data + 4) == 0x00010000 || tt_ulong(data + 4) == 0x00020000))
        return tt_ulong(data + 8);
    return 0;
}

static int tt_offset_for_index(const tt_uint8* data, int index)
{
    if(tt_is_font(data))
        return index == 0 ? 0 : -1;
    if(tt_tag4(data, 't', 't', 'c', 'f') && (tt_ulong(data + 4) == 0x00010000 || tt_ulong(data + 4) == 0x00020000)) {
        if(index >= tt_ulong(data + 8))
            return -1;
        return tt_ulong(data + 12 + index * 4);
    }
    return -1;
}

static tt_uint32 tt_find_table(const tt_uint8* data, tt_uint32 font_start, const char* tag)
{
    tt_int32 num_tables = tt_ushort(data + font_start + 4);
    tt_uint32 table_dir = font_start + 12;
    for(tt_int32 i = 0; i < num_tables; ++i) {
        tt_uint32 loc = table_dir + 16 * i;
        if(tt_tag4(data + loc, tag[0], tag[1], tag[2], tag[3]))
            return tt_ulong(data + loc + 8);
    }
    return 0;
}

#ifdef _WIN32

static void* mmap(const char* filename, long* length)
{
    HANDLE file = create_file_a(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(file == INVALID_HANDLE_VALUE)
        return NULL;
    DWORD size = get_file_size(file, NULL);
    if(size == INVALID_FILE_SIZE) {
        close_handle(file);
        return NULL;
    }

    HANDLE mapping = create_file_mapping_a(file, NULL, PAGE_READONLY, 0, 0, NULL);
    if(mapping == NULL) {
        close_handle(file);
        return NULL;
    }

    void* data = map_view_of_file(mapping, FILE_MAP_READ, 0, 0, 0);
    close_handle(mapping);
    close_handle(file);

    if(data == NULL)
        return NULL;
    *length = size;
    return data;
}

static void unmap(void* data, long length)
{
    unmap_view_of_file(data);
}

#else

static void* mmap(const char* filename, long* length)
{
    int fd = open(filename, O_RDONLY);
    if(fd < 0)
        return NULL;
    struct stat st;
    if(fstat(fd, &st) < 0) {
        close(fd);
        return NULL;
    }

    if(st.st_size == 0) {
        close(fd);
        return NULL;
    }

    void* data = ::mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    if(data == MAP_FAILED)
        return NULL;
    *length = st.st_size;
    return data;
}

static void unmap(void* data, long length)
{
    munmap(data, length);
}

#endif // _WIN32

int font_face_cache_load_file(font_face_cache_t* cache, const char* filename)
{
    long length;
    tt_uint8* data = static_cast<tt_uint8*>(mmap(filename, &length));
    if(data == NULL) {
        return 0;
    }

    int num_faces = 0;

    int num_fonts = tt_number_of_fonts(data);
    for(int index = 0; index < num_fonts; ++index) {
        int offset = tt_offset_for_index(data, index);
        if(offset == -1 || !tt_is_font(data + offset)) {
            continue;
        }

        tt_uint32 nm = tt_find_table(data, offset, "name");
        tt_uint16 nm_count = tt_ushort(data + nm + 2);

        const tt_uint8* unicode_family_name = NULL;
        const tt_uint8* roman_family_name = NULL;

        size_t family_length = 0;
        for(tt_int32 i = 0; i < nm_count; ++i) {
            tt_uint32 loc = nm + 6 + 12 * i;
            tt_uint16 nm_id = tt_ushort(data + loc + 6);
            if(nm_id != 1) {
                continue;
            }

            tt_uint16 platform = tt_ushort(data + loc + 0);
            tt_uint16 encoding = tt_ushort(data + loc + 2);

            const tt_uint8* family_name = data + nm + tt_ushort(data + nm + 4) + tt_ushort(data + loc + 10);
            if(platform == 1 && encoding == 0) {
                family_length = tt_ushort(data + loc + 8);
                roman_family_name = family_name;
                continue;
            }

            if(platform == 0 || (platform == 3 && encoding == 1) || (platform == 3 && encoding == 10)) {
                family_length = tt_ushort(data + loc + 8);
                unicode_family_name = family_name;
                break;
            }
        }

        if(unicode_family_name == NULL && roman_family_name == NULL)
            continue;
        size_t filename_length = strlen(filename) + 1;
        size_t max_family_length = (unicode_family_name ? 3 * (family_length / 2) : family_length * 3) + 1;

        font_face_entry_t* entry = static_cast<font_face_entry_t*>(malloc(max_family_length + filename_length + sizeof(font_face_entry_t)));
        entry->family = (char*)(entry + 1);
        entry->filename = entry->family + max_family_length;
        memcpy(entry->filename, filename, filename_length);

        size_t family_index = 0;
        if(unicode_family_name) {
            const tt_uint8* family_name = unicode_family_name;
            while(family_length) {
                tt_uint16 ch = family_name[0] * 256 + family_name[1];
                if(ch < 0x80) {
                    entry->family[family_index++] = ch;
                } else if(ch < 0x800) {
                    entry->family[family_index++] = (0xc0 + (ch >> 6));
                    entry->family[family_index++] = (0x80 + (ch & 0x3f));
                } else if(ch >= 0xd800 && ch < 0xdc00) {
                    tt_uint16 ch2 = family_name[2] * 256 + family_name[3];
                    tt_uint32 c = ((ch - 0xd800) << 10) + (ch2 - 0xdc00) + 0x10000;

                    entry->family[family_index++] = (0xf0 + (c >> 18));
                    entry->family[family_index++] = (0x80 + ((c >> 12) & 0x3f));
                    entry->family[family_index++] = (0x80 + ((c >> 6) & 0x3f));
                    entry->family[family_index++] = (0x80 + ((c) & 0x3f));

                    family_name += 2;
                    family_length -= 2;
                } else {
                    entry->family[family_index++] = (0xe0 + (ch >> 12));
                    entry->family[family_index++] = (0x80 + ((ch >> 6) & 0x3f));
                    entry->family[family_index++] = (0x80 + ((ch) & 0x3f));
                }

                family_name += 2;
                family_length -= 2;
            }

            entry->family[family_index] = '\0';
        } else {
            static const tt_uint16 MAC_ROMAN_TABLE[256] = {
                0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
                0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F,
                0x0010, 0x2318, 0x21E7, 0x2325, 0x2303, 0x0015, 0x0016, 0x0017,
                0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F,
                0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
                0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
                0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
                0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
                0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
                0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
                0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
                0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
                0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
                0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
                0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
                0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F,
                0x00C4, 0x00C5, 0x00C7, 0x00C9, 0x00D1, 0x00D6, 0x00DC, 0x00E1,
                0x00E0, 0x00E2, 0x00E4, 0x00E3, 0x00E5, 0x00E7, 0x00E9, 0x00E8,
                0x00EA, 0x00EB, 0x00ED, 0x00EC, 0x00EE, 0x00EF, 0x00F1, 0x00F3,
                0x00F2, 0x00F4, 0x00F6, 0x00F5, 0x00FA, 0x00F9, 0x00FB, 0x00FC,
                0x2020, 0x00B0, 0x00A2, 0x00A3, 0x00A7, 0x2022, 0x00B6, 0x00DF,
                0x00AE, 0x00A9, 0x2122, 0x00B4, 0x00A8, 0x2260, 0x00C6, 0x00D8,
                0x221E, 0x00B1, 0x2264, 0x2265, 0x00A5, 0x00B5, 0x2202, 0x2211,
                0x220F, 0x03C0, 0x222B, 0x00AA, 0x00BA, 0x03A9, 0x00E6, 0x00F8,
                0x00BF, 0x00A1, 0x00AC, 0x221A, 0x0192, 0x2248, 0x2206, 0x00AB,
                0x00BB, 0x2026, 0x00A0, 0x00C0, 0x00C3, 0x00D5, 0x0152, 0x0153,
                0x2013, 0x2014, 0x201C, 0x201D, 0x2018, 0x2019, 0x00F7, 0x25CA,
                0x00FF, 0x0178, 0x2044, 0x20AC, 0x2039, 0x203A, 0xFB01, 0xFB02,
                0x2021, 0x00B7, 0x201A, 0x201E, 0x2030, 0x00C2, 0x00CA, 0x00C1,
                0x00CB, 0x00C8, 0x00CD, 0x00CE, 0x00CF, 0x00CC, 0x00D3, 0x00D4,
                0xF8FF, 0x00D2, 0x00DA, 0x00DB, 0x00D9, 0x0131, 0x02C6, 0x02DC,
                0x00AF, 0x02D8, 0x02D9, 0x02DA, 0x00B8, 0x02DD, 0x02DB, 0x02C7,
            };

            const tt_uint8* family_name = roman_family_name;
            while(family_length) {
                tt_uint16 ch = MAC_ROMAN_TABLE[family_name[0]];
                if(ch < 0x80) {
                    entry->family[family_index++] = ch;
                } else if(ch < 0x800) {
                    entry->family[family_index++] = (0xc0 + (ch >> 6));
                    entry->family[family_index++] = (0x80 + (ch & 0x3f));
                } else {
                    entry->family[family_index++] = (0xe0 + (ch >> 12));
                    entry->family[family_index++] = (0x80 + ((ch >> 6) & 0x3f));
                    entry->family[family_index++] = (0x80 + ((ch) & 0x3f));
                }

                family_name += 1;
                family_length -= 1;
            }

            entry->family[family_index] = '\0';
        }

        entry->face = NULL;
        entry->bold = false;
        entry->italic = false;
        entry->ttcindex = index;

        tt_uint32 hd = tt_find_table(data, offset, "head");
        tt_uint16 style = tt_ushort(data + hd + 44);
        if(style & 0x1)
            entry->bold = true;
        if(style & 0x2) {
            entry->italic = true;
        }

        font_face_cache_add_entry(cache, entry);
        num_faces++;
    }

    unmap(data, length);
    return num_faces;
}

static bool font_face_supports_file(const char* filename)
{
    const char* extension = strrchr(filename, '.');
    if(extension) {
        char ext[5];
        size_t length = strlen(extension);
        if(length == 4) {
            for(size_t i = 0; i < length; ++i)
                ext[i] = tolower(extension[i]);
            ext[length] = '\0';
            return strcmp(ext, ".ttf") == 0
                || strcmp(ext, ".otf") == 0
                || strcmp(ext, ".ttc") == 0
                || strcmp(ext, ".otc") == 0;
        }
    }

    return false;
}

#ifdef _WIN32

int font_face_cache_load_dir(font_face_cache_t* cache, const char* dirname)
{
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*", dirname);

    WIN32_FIND_DATAA find_data;
    HANDLE handle = find_first_file_a(search_path, &find_data);
    if(handle == INVALID_HANDLE_VALUE) {
        return 0;
    }

    int num_faces = 0;

    do {
        const char* name = find_data.c_file_name;
        if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
            continue;
        char path[MAX_PATH * 2];
        snprintf(path, sizeof(path), "%s\\%s", dirname, name);

        if(find_data.dw_file_attributes & FILE_ATTRIBUTE_DIRECTORY) {
            num_faces += font_face_cache_load_dir(cache, path);
        } else if(font_face_supports_file(path)) {
            num_faces += font_face_cache_load_file(cache, path);
        }
    } while(find_next_file_a(handle, &find_data));

    find_close(handle);
    return num_faces;
}

#else

int font_face_cache_load_dir(font_face_cache_t* cache, const char* dirname)
{
    DIR* dir = opendir(dirname);
    if(dir == NULL) {
        return 0;
    }

    int num_faces = 0;

    struct dirent* entry;
    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dirname, entry->d_name);

        struct stat st;
        if(stat(path, &st) == -1)
            continue;
        if(S_ISDIR(st.st_mode)) {
            num_faces += font_face_cache_load_dir(cache, path);
        } else if(S_ISREG(st.st_mode) && font_face_supports_file(path)) {
            num_faces += font_face_cache_load_file(cache, path);
        }
    }

    closedir(dir);
    return num_faces;
}

#endif // _WIN32

int font_face_cache_load_sys(font_face_cache_t* cache)
{
    int num_faces = 0;
#if defined(_WIN32)
    num_faces += font_face_cache_load_dir(cache, "C:\\Windows\\Fonts");
#elif defined(__APPLE__)
    num_faces += font_face_cache_load_dir(cache, "/Library/Fonts");
    num_faces += font_face_cache_load_dir(cache, "/System/Library/Fonts");
#elif defined(__linux__)
    num_faces += font_face_cache_load_dir(cache, "/usr/share/fonts");
    num_faces += font_face_cache_load_dir(cache, "/usr/local/share/fonts");
#endif
    return num_faces;
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
