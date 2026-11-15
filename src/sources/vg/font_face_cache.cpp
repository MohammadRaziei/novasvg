// The font-face cache: which face answers a request for a family, bold and italic. libstb tells what a font file is
// called (family, weight, width, bold, italic: font::scan and font::system_fonts) and reads the font; this file only
// keeps the table and picks.
#include "font.h"
#include "internal.h"
#include "../utils/text.h"
#include <stb/logging.hpp>
#include <stb/util.hpp>

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


// One face of a family: either loaded already or only known by its file, loaded on first use. A face listed under two
// family names (the typographic one and the legacy one) has two entries and one slot, so it is loaded once.
struct font_face_slot {
    font_face face;
    std::string filename;   // empty for a face that was added directly
    int ttcindex = 0;
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

void font_face_cache_add(font_face_cache_t* cache, const char* family, bool bold, bool italic, const font_face& face)
{
    font_face_entry entry;
    entry.slot = std::make_shared<font_face_slot>();
    entry.slot->face = face;
    entry.bold = bold;
    entry.italic = italic;
    entry.weight = bold ? 700 : 400;
    font_face_cache_add_entry(cache, family ? family : "", std::move(entry));
}

bool font_face_cache_add_file(font_face_cache_t* cache, const char* family, bool bold, bool italic, const char* filename, int ttcindex)
{
    font_face face = font_face::from_file(filename, ttcindex);
    if(face.is_null()) {
        font_log().warning("cannot add '%s' to the font cache (family '%s')", filename, family);
        return false;
    }
    font_face_cache_add(cache, family, bold, italic, face);
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

font_face font_face_cache_get(font_face_cache_t* cache, const char* family, bool bold, bool italic)
{
    std::lock_guard<std::mutex> lock(cache->mutex);

    auto found = cache->families.find(family);
    if(found == cache->families.end())
        return font_face();

    // The newest face wins a tie, so walk from the newest to the oldest and replace only on a better score.
    std::vector<font_face_entry>& faces = found->second;
    font_face_entry* selected = &faces.back();
    for(std::size_t i = faces.size() - 1; i-- > 0;) {
        if(font_face_entry_better(faces[i], *selected, bold, italic))
            selected = &faces[i];
    }

    font_face_slot& slot = *selected->slot;
    if(!slot.filename.empty() && slot.face.is_null())
        slot.face = font_face::from_file(slot.filename.c_str(), slot.ttcindex);
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
