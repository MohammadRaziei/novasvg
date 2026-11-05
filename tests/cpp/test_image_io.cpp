#include "doctest.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <nova/svg/svg.h>
#include <stb/image.hpp>

// Bitmap is internal and stores premultiplied ARGB32; the public pixel type is stb::image (straight RGBA).
// bitmap_to_rgba_image / bitmap_release_to_image are the boundary, and libstb's encoders write the result, so
// these tests encode with stb::image, decode what was written and compare straight pixels.

namespace {

// Premultiplied 0xAARRGGBB words, the way the engine stores them.
constexpr std::uint32_t kOpaqueRed = 0xFFFF0000u;
constexpr std::uint32_t kHalfBlue = 0x80000040u;  // a = 128, premultiplied b = 64: straight b = 64 * 255 / 128 = 127
constexpr std::uint32_t kTransparent = 0x00000000u;

stb::image decode(const std::vector<std::uint8_t>& bytes)
{
    stb::load_options options;
    options.format = stb::pixel_format::rgba;
    return stb::image::decode(bytes.data(), bytes.size(), options);
}

struct rgba {
    int r, g, b, a;
};

rgba pixel_at(const stb::image& image, int x, int y)
{
    const std::uint8_t* p = image.data() + (static_cast<std::size_t>(y) * image.width() + x) * 4;
    return {p[0], p[1], p[2], p[3]};
}

// 8 x 4: pixel 0 opaque red, pixel 1 half-transparent blue, pixel 2 fully transparent, the rest opaque red.
nova::vg::Bitmap make_pattern()
{
    nova::vg::Bitmap bitmap(8, 4);
    auto* words = reinterpret_cast<std::uint32_t*>(bitmap.data());
    for(int i = 0; i < 8 * 4; ++i)
        words[i] = kOpaqueRed;
    words[1] = kHalfBlue;
    words[2] = kTransparent;
    return bitmap;
}

nova::vg::Bitmap make_solid_red(int size)
{
    nova::vg::Bitmap bitmap(size, size);
    auto* words = reinterpret_cast<std::uint32_t*>(bitmap.data());
    for(int i = 0; i < size * size; ++i)
        words[i] = kOpaqueRed;
    return bitmap;
}

} // namespace

TEST_CASE("image io: PNG and TGA are lossless and carry straight alpha")
{
    const stb::image source = nova::vg::bitmap_release_to_image(make_pattern());
    REQUIRE(!source.empty());
    for(const bool png : {true, false}) {
        CAPTURE(png ? "png" : "tga");
        const std::vector<std::uint8_t> bytes = png ? source.to_png() : source.to_tga();
        REQUIRE(!bytes.empty());

        const stb::image image = decode(bytes);
        REQUIRE(image.width() == 8);
        REQUIRE(image.height() == 4);

        const rgba red = pixel_at(image, 0, 0);
        CHECK((red.r == 255 && red.g == 0 && red.b == 0 && red.a == 255));
        const rgba blue = pixel_at(image, 1, 0);
        CHECK((blue.r == 0 && blue.g == 0 && blue.b == 127 && blue.a == 128));
        const rgba clear = pixel_at(image, 2, 0);
        CHECK((clear.r == 0 && clear.g == 0 && clear.b == 0 && clear.a == 0));
        const rgba last = pixel_at(image, 7, 3);
        CHECK((last.r == 255 && last.g == 0 && last.b == 0 && last.a == 255));
    }
}

TEST_CASE("image io: BMP and JPEG decode back to the right size and colour")
{
    const stb::image source = nova::vg::bitmap_release_to_image(make_solid_red(32));

    const stb::image from_bmp = decode(source.to_bmp());
    REQUIRE(from_bmp.width() == 32);
    REQUIRE(from_bmp.height() == 32);
    const rgba b = pixel_at(from_bmp, 16, 16);
    CHECK((b.r == 255 && b.g == 0 && b.b == 0));

    const stb::image from_jpg = decode(source.to_jpg(90));
    REQUIRE(from_jpg.width() == 32);
    REQUIRE(from_jpg.height() == 32);
    const rgba j = pixel_at(from_jpg, 16, 16);  // lossy: close to red, not necessarily exact
    CHECK(j.r > 235);
    CHECK(j.g < 20);
    CHECK(j.b < 20);
}

TEST_CASE("image io: writing to a file gives the same pixels as encoding to bytes")
{
    const stb::image source = nova::vg::bitmap_release_to_image(make_pattern());
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "novasvg_image_io_test.png";

    const std::vector<std::uint8_t> bytes = source.to_png();
    source.write(path);  // dispatch by extension reaches the PNG encoder

    std::ifstream in(path, std::ios::binary);
    const std::vector<std::uint8_t> from_file((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::filesystem::remove(path);

    const stb::image a = decode(bytes);
    const stb::image b = decode(from_file);
    REQUIRE(a.width() == b.width());
    REQUIRE(a.height() == b.height());
    CHECK(std::memcmp(a.data(), b.data(), static_cast<std::size_t>(a.width()) * a.height() * 4) == 0);
}

TEST_CASE("image io: a bitmap over caller memory with a padded stride converts correctly, both ways")
{
    // Rows are 32 bytes apart but hold only 5 pixels (20 bytes); the padding is filled with junk. Converting
    // with the wrong row pitch would read or write outside the buffer or smear the picture.
    constexpr int width = 5, height = 3, stride = 32;
    std::vector<std::uint8_t> memory(static_cast<std::size_t>(stride) * height, 0xAB);
    for(int y = 0; y < height; ++y) {
        auto* row = reinterpret_cast<std::uint32_t*>(memory.data() + static_cast<std::size_t>(y) * stride);
        for(int x = 0; x < width; ++x)
            row[x] = 0xFF000000u | (static_cast<std::uint32_t>(x * 40) << 16) | (static_cast<std::uint32_t>(y * 80) << 8) | 0x33u;
    }
    nova::vg::Bitmap bitmap(memory.data(), width, height, stride);
    REQUIRE(!bitmap.is_null());

    auto check = [&](const stb::image& image) {
        REQUIRE(image.width() == width);
        REQUIRE(image.height() == height);
        for(int y = 0; y < height; ++y) {
            for(int x = 0; x < width; ++x) {
                CAPTURE(x);
                CAPTURE(y);
                const rgba p = pixel_at(image, x, y);
                CHECK((p.r == x * 40 && p.g == y * 80 && p.b == 0x33 && p.a == 255));
            }
        }
    };
    check(nova::vg::bitmap_to_rgba_image(bitmap));                       // copy, leaves the bitmap alone
    check(decode(nova::vg::bitmap_to_rgba_image(bitmap).to_png()));
    check(nova::vg::bitmap_release_to_image(std::move(bitmap)));         // padded rows cannot be wrapped: copies
}

TEST_CASE("image io: a null bitmap gives an empty image, and encoding an empty image throws")
{
    CHECK(nova::vg::bitmap_to_rgba_image(nova::vg::Bitmap()).empty());
    CHECK(nova::vg::bitmap_release_to_image(nova::vg::Bitmap()).empty());
    const stb::image empty;
    CHECK_THROWS(empty.to_png());
    const stb::image image = nova::vg::bitmap_release_to_image(make_solid_red(4));
    CHECK_THROWS(image.write_png("/this/directory/does/not/exist/out.png"));
}

TEST_CASE("image io: the hand-off to stb::image does not copy a tightly packed bitmap")
{
    nova::vg::Bitmap bitmap = make_solid_red(16);
    const std::uint8_t* before = bitmap.data();
    const stb::image image = nova::vg::bitmap_release_to_image(std::move(bitmap));
    REQUIRE(!image.empty());
    CHECK(image.data() == before);  // same memory: converted in place and wrapped
    const rgba p = pixel_at(image, 3, 3);
    CHECK((p.r == 255 && p.g == 0 && p.b == 0 && p.a == 255));
}
