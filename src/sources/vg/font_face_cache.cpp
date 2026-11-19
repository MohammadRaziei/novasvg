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


void font_face_cache::add_entry(const std::string& family, entry e)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_families[family].push_back(std::move(e));
}

bool font_face_cache::add_font_face(const std::string& family, bool bold, bool italic, const font_face& face)
{
    if(face.is_null())
        return false;
    entry e;
    e.slot_ = std::make_shared<slot>();
    e.slot_->face = face;
    e.bold = bold;
    e.italic = italic;
    e.weight = e.weight_min = e.weight_max = bold ? 700 : 400;
    add_entry(family, std::move(e));
    return true;
}

bool font_face_cache::add_file(const std::string& family, bool bold, bool italic, const char* filename, int ttcindex)
{
    font_face face = font_face::from_file(filename, ttcindex);
    if(face.is_null()) {
        font_log().warning("cannot add '%s' to the font cache (family '%s')", filename, family.c_str());
        return false;
    }
    return add_font_face(family, bold, italic, face);
}

void font_face_cache::reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_families.clear();
}

namespace {

// The better of two faces of a family for a request, first argument wins only if it is better.
template <class Entry>
bool entry_better(const Entry& a, const Entry& b, bool bold, bool italic)
{
    const int a_score = (bold == a.bold) + (italic == a.italic);
    const int b_score = (bold == b.bold) + (italic == b.italic);
    if(a_score != b_score)
        return a_score > b_score;
    if(std::abs(a.width - 5) != std::abs(b.width - 5))
        return std::abs(a.width - 5) < std::abs(b.width - 5);
    // a variable font reaches every weight of its axis: the distance is to the range, not to the default instance
    const int target = bold ? 700 : 400;
    const auto distance = [target](const Entry& e) {
        return target < e.weight_min ? e.weight_min - target : target > e.weight_max ? target - e.weight_max : 0;
    };
    const int da = distance(a), db = distance(b);
    if(da != db)
        return da < db;
    return std::abs(a.weight - target) < std::abs(b.weight - target);
}

}  // namespace

font_face font_face_cache::get_font_face_local(const std::string& family, bool bold, bool italic) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto found = m_families.find(family);
    if(found == m_families.end())
        return font_face();

    // The newest face wins a tie, so walk from the newest to the oldest and replace only on a better score.
    const std::vector<entry>& faces = found->second;
    const entry* selected = &faces.back();
    for(std::size_t i = faces.size() - 1; i-- > 0;) {
        if(entry_better(faces[i], *selected, bold, italic))
            selected = &faces[i];
    }

    slot& s = *selected->slot_;
    if(!s.filename.empty() && s.face.is_null())
        s.face = font_face::from_file(s.filename.c_str(), s.ttcindex);

    // A variable font is one face for every weight it has: set the weight the request means (bold is 700, regular 400).
    const int target = bold ? 700 : 400;
    if(!s.face.is_null() && selected->weight_min < selected->weight_max && target >= selected->weight_min &&
       target <= selected->weight_max && target != s.face.get()->weight()) {
        auto [at, made] = s.weighted.try_emplace(target);
        if(made)
            at->second = s.face.with_weight(target);
        return at->second;
    }
    return s.face;
}

#ifndef NOVA_DISABLE_FONT_FACE_CACHE_LOAD

// A face found by libstb goes in under its family (name id 16, "DejaVu Sans") and, when that differs, under the name a
// program that knows only regular and bold uses (id 1, "DejaVu Sans Light"): font-family can say either.
int font_face_cache::add_found(const std::string& path, int index, bool bold, bool italic, int weight, int width,
                               const std::string& family_name, const std::string& legacy_family_name, int weight_min, int weight_max)
{
    entry e;
    e.slot_ = std::make_shared<slot>();
    e.slot_->filename = path;
    e.slot_->ttcindex = index;
    e.bold = bold;
    e.italic = italic;
    e.weight = weight;
    e.weight_min = weight_min;
    e.weight_max = weight_max;
    e.width = width;

    int added = 0;
    if(!family_name.empty()) {
        add_entry(family_name, e);
        added = 1;
    }
    if(!legacy_family_name.empty() && legacy_family_name != family_name) {
        add_entry(legacy_family_name, e);
        added = 1;
    }
    return added;
}

int font_face_cache::load_file(const char* filename)
{
    try {
        const std::filesystem::path path(filename);
        int num_faces = 0;
        for(const stb::font_face_info& info : stb::font::scan(path))
            num_faces += add_found(path.string(), info.index, info.bold, info.italic, info.weight, info.width, info.family_name, info.legacy_family_name, info.weight_min, info.weight_max);
        return num_faces;
    } catch(const std::exception&) {
        return 0; // not a font, or not readable
    }
}

int font_face_cache::load_dir(const char* dirname)
{
    int num_faces = 0;
    for(const stb::system_font_info& info : stb::font::system_fonts({std::filesystem::path(dirname)}))
        num_faces += add_found(info.path.string(), info.index, info.bold, info.italic, info.weight, info.width, info.family_name, info.legacy_family_name, info.weight_min, info.weight_max);
    return num_faces;
}

int font_face_cache::load_system()
{
    int num_faces = 0;
    for(const stb::system_font_info& info : stb::font::system_fonts())
        num_faces += add_found(info.path.string(), info.index, info.bold, info.italic, info.weight, info.width, info.family_name, info.legacy_family_name, info.weight_min, info.weight_max);
    return num_faces;
}

#else

int font_face_cache::add_found(const std::string&, int, bool, bool, int, int, const std::string&, const std::string&, int, int) { return 0; }
int font_face_cache::load_file(const char*) { return -1; }
int font_face_cache::load_dir(const char*) { return -1; }
int font_face_cache::load_system() { return -1; }

#endif // NOVA_DISABLE_FONT_FACE_CACHE_LOAD

} // namespace vg
} // namespace nova
