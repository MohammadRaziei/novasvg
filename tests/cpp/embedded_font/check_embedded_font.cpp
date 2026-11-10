// Built with NOVA_DISABLE_LOAD_SYSTEM_FONTS and no font registered: the machine has no fonts at all
// as far as novasvg can tell, which is the situation the embedded fallback font exists for.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../doctest.h"

#include <nova/svg/svg.h>
#include "woff_fixture.h"
#include <vg/canvas.h>
#include <vg/font.h>

#include <cstring>
#include <string>

#ifndef NOVA_DISABLE_LOAD_SYSTEM_FONTS
#error "this test must be built with NOVA_DISABLE_LOAD_SYSTEM_FONTS"
#endif

namespace {
size_t inked_pixels(const std::string& svg)
{
    auto dom = nova::svg::document::load_data(svg);
    REQUIRE(dom != nullptr);
    auto image = dom->render_to_image();
    size_t inked = 0;
    for(int y = 0; y < image.height(); ++y) {
        const auto* row = image.data() + size_t(y) * image.stride();
        for(int x = 0; x < image.width(); ++x)
            inked += row[x * 4 + 3] != 0;
    }
    return inked;
}

std::string text_svg(const std::string& family)
{
    return "<svg xmlns='http://www.w3.org/2000/svg' width='200' height='60'>"
           "<text x='5' y='40' font-size='32' font-family='" + family + "'>Hello</text></svg>";
}
std::string font_face_svg(const char* mime, const char* base64)
{
    return std::string("<svg xmlns='http://www.w3.org/2000/svg' width='300' height='80'><style>@font-face{font-family:'Probe';src:url(data:")
        + mime + ";base64," + base64 + ")}</style><text x='10' y='50' font-size='36' font-family='Probe'>Woff Test 123</text></svg>";
}

std::string pixels_of(const std::string& svg)
{
    auto dom = nova::svg::document::load_data(svg);
    REQUIRE(dom != nullptr);
    auto image = dom->render_to_image();
    return std::string(reinterpret_cast<const char*>(image.data()), size_t(image.height()) * image.stride());
}
} // namespace

TEST_CASE("text is drawn although the machine has no fonts")
{
    CHECK(inked_pixels(text_svg("sans-serif")) > 200);
    CHECK(inked_pixels(text_svg("Some Family, Another, sans-serif")) > 200);
    CHECK(inked_pixels(text_svg("serif")) > 200);
}

TEST_CASE("the embedded font lays text out exactly like the full DejaVu Sans")
{
    REQUIRE(nova::vg::add_font_face_from_file("Full Probe", false, false, NOVA_TEST_FONT_PATH));
    auto* cache = nova::vg::global_font_face_cache();
    auto full = cache->get_font_face("Full Probe", false, false);
    auto embedded = cache->get_font_face("sans-serif", false, false);
    REQUIRE(!full.is_null());
    REQUIRE(!embedded.is_null());
    CHECK(embedded.units_per_em() == full.units_per_em());

    // kerning pairs, ligatures, digits, punctuation, Latin-1
    const std::u32string samples[] = {
        U"Hello World", U"AV To LT Ty Yo WA", U"fi fl ffi ffl office", U"0123456789 +-*/=<>",
        U"The quick brown fox jumps over the lazy dog", U"Caf\u00e9 \u00d1and\u00fa \u00fcber \u00c5ngstr\u00f6m",
        U"\u201cquoted\u201d \u2013 dash \u2014 \u2026 \u20ac \u2122 \u2022",
    };
    int index = 0;
    for(const auto& text : samples) {
        ++index;
        for(float size : {10.0f, 16.0f, 31.0f}) {
            CAPTURE(index);
            CAPTURE(size);
            CHECK(nova::vg::sized_font(embedded, size).measure_text(text) == nova::vg::sized_font(full, size).measure_text(text));
        }
    }
}

namespace {
// The directory the regular test font lives in; the bold/oblique faces sit beside it.
std::string font_dir()
{
    std::string path = NOVA_TEST_FONT_PATH;
    return path.substr(0, path.find_last_of("/\\") + 1);
}
} // namespace

TEST_CASE("bold and italic requests get the bold / oblique embedded faces, laid out like the full fonts")
{
    struct Face { const char* file; bool bold; bool italic; };
    const Face faces[] = {
        {"DejaVuSans-Bold.ttf", true, false},
        {"DejaVuSans-Oblique.ttf", false, true},
        {"DejaVuSans-BoldOblique.ttf", true, true},
    };
    auto* cache = nova::vg::global_font_face_cache();
    const auto regular = cache->get_font_face("sans-serif", false, false);
    REQUIRE(!regular.is_null());
    const std::u32string samples[] = {U"Hello World", U"AV To LT Ty Yo WA", U"fi fl ffi ffl office", U"The quick brown fox jumps over the lazy dog"};

    for(const auto& spec : faces) {
        CAPTURE(spec.file);
        const std::string family = std::string("Full ") + spec.file;
        REQUIRE(nova::vg::add_font_face_from_file(family.c_str(), spec.bold, spec.italic, (font_dir() + spec.file).c_str()));
        const auto full = cache->get_font_face(family, spec.bold, spec.italic);
        const auto embedded = cache->get_font_face("sans-serif", spec.bold, spec.italic);
        REQUIRE(!full.is_null());
        REQUIRE(!embedded.is_null());
        CHECK(embedded.get() != regular.get()); // not just the regular face under another request
        for(const auto& text : samples)
            CHECK(nova::vg::sized_font(embedded, 16.f).measure_text(text) == nova::vg::sized_font(full, 16.f).measure_text(text));
        // and it is a different design from the regular face (bold is wider, oblique has other advances)
        CHECK(nova::vg::sized_font(embedded, 16.f).measure_text(samples[3]) != nova::vg::sized_font(regular, 16.f).measure_text(samples[3]));
    }
}

TEST_CASE("bold text is drawn heavier than regular text although the machine has no fonts")
{
    auto text = [](const char* attrs) {
        return std::string("<svg xmlns='http://www.w3.org/2000/svg' width='200' height='60'><text x='5' y='40' font-size='32' font-family='sans-serif' ") + attrs + ">Hello</text></svg>";
    };
    const auto regular = inked_pixels(text(""));
    CHECK(inked_pixels(text("font-weight='bold'")) > regular * 11 / 10);
    CHECK(inked_pixels(text("font-style='italic'")) != regular);
}

TEST_CASE("bold and italic runs inside a foreignObject use the embedded faces")
{
    auto label = [](const char* inner) {
        return std::string("<svg xmlns='http://www.w3.org/2000/svg' width='300' height='60'><foreignObject x='0' y='0' width='300' height='60'>"
                           "<div xmlns='http://www.w3.org/1999/xhtml' style='font-size:24px;font-family:sans-serif;text-align:center'>") + inner + "</div></foreignObject></svg>";
    };
    const auto plain = inked_pixels(label("Hello world"));
    CHECK(inked_pixels(label("<b>Hello world</b>")) > plain * 11 / 10);
    CHECK(inked_pixels(label("<i>Hello world</i>")) != plain);
    CHECK(inked_pixels(label("Hello <b>world</b>")) > plain);
}

TEST_CASE("an @font-face font given as WOFF 1.0 is drawn exactly like the same font as TrueType")
{
    const std::string ttf = pixels_of(font_face_svg("font/ttf", woff_fixture::ttf_base64));
    const std::string woff = pixels_of(font_face_svg("font/woff", woff_fixture::woff_base64));
    CHECK(woff == ttf);
    // and it is the embedded face, not the regular fallback (the fixture is the oblique one)
    CHECK(woff != pixels_of("<svg xmlns='http://www.w3.org/2000/svg' width='300' height='80'><text x='10' y='50' font-size='36' "
                            "font-family='sans-serif'>Woff Test 123</text></svg>"));
}

namespace {
const char base64_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64_decode(const std::string& text)
{
    std::string out;
    unsigned bits = 0;
    int count = 0;
    for(char c : text) {
        const char* at = c == '\0' ? nullptr : std::strchr(base64_alphabet, c);
        if(!at)
            continue;   // '=' and anything else
        bits = (bits << 6) | unsigned(at - base64_alphabet);
        count += 6;
        if(count >= 8) {
            count -= 8;
            out.push_back(char((bits >> count) & 0xFF));
        }
    }
    return out;
}

std::string base64_encode(const std::string& bytes)
{
    std::string out;
    for(size_t i = 0; i < bytes.size(); i += 3) {
        const unsigned b0 = (unsigned char)bytes[i];
        const unsigned b1 = i + 1 < bytes.size() ? (unsigned char)bytes[i + 1] : 0;
        const unsigned b2 = i + 2 < bytes.size() ? (unsigned char)bytes[i + 2] : 0;
        out.push_back(base64_alphabet[b0 >> 2]);
        out.push_back(base64_alphabet[((b0 & 3) << 4) | (b1 >> 4)]);
        out.push_back(i + 1 < bytes.size() ? base64_alphabet[((b1 & 15) << 2) | (b2 >> 6)] : '=');
        out.push_back(i + 2 < bytes.size() ? base64_alphabet[b2 & 63] : '=');
    }
    return out;
}
} // namespace

TEST_CASE("a WOFF whose table directory lies about a length is dropped, and the text falls back")
{
    std::string woff = base64_decode(woff_fixture::woff_base64);
    REQUIRE(woff.size() > 64);
    // WOFF 1.0: a 44-byte header, then 20-byte table entries (tag, offset, compLength, origLength, checksum).
    // Make the first table claim 16 MiB of compressed data, far past the end of the file.
    woff[44 + 8] = 0x01; woff[44 + 9] = 0x00; woff[44 + 10] = 0x00; woff[44 + 11] = 0x00;
    const std::string broken = pixels_of(font_face_svg("font/woff", base64_encode(woff).c_str()));
    // no embedded face: 'Probe' is an unknown family, so it is drawn like any other unknown family
    const std::string unknown = pixels_of("<svg xmlns='http://www.w3.org/2000/svg' width='300' height='80'><text x='10' y='50' font-size='36' "
                                          "font-family='Probe'>Woff Test 123</text></svg>");
    CHECK(broken == unknown);
}

TEST_CASE("a WOFF cut off in the middle is dropped, and the text falls back")
{
    std::string woff = base64_decode(woff_fixture::woff_base64);
    REQUIRE(woff.size() > 200);
    woff.resize(woff.size() / 2);
    const std::string cut = pixels_of(font_face_svg("font/woff", base64_encode(woff).c_str()));
    const std::string unknown = pixels_of("<svg xmlns='http://www.w3.org/2000/svg' width='300' height='80'><text x='10' y='50' font-size='36' "
                                          "font-family='Probe'>Woff Test 123</text></svg>");
    CHECK(cut == unknown);
}
