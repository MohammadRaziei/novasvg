// novasvg -> libstb link check.
//
// novasvg links libstb as a SHARED library. This program is built against novasvg::noflag only
// (no direct reference to libstb in its CMake), so it passes only if
//   1. the libstb include path and link line travel with the novasvg target, and
//   2. the library really is a separate shared object, not compiled into this executable.
// It also runs a minimal round trip through the three libstb areas novasvg is going to use:
// colour, zlib and image encode/decode.

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include <stb/color.hpp>
#include <stb/image.hpp>
#include <stb/zlib.hpp>

#if defined(__unix__) || defined(__APPLE__)
#include <dlfcn.h>
#define NOVA_CAN_DLADDR 1
#endif

static int failures = 0;
#define CHECK(cond)                                                                        \
    do {                                                                                   \
        if (!(cond)) {                                                                     \
            std::fprintf(stderr, "FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond);         \
            ++failures;                                                                    \
        }                                                                                  \
    } while (0)

#ifdef NOVA_CAN_DLADDR
static void local_marker() {}

// The object file that contains `address`, as the dynamic loader sees it.
static const char* object_of(const void* address) {
    Dl_info info;
    return dladdr(address, &info) && info.dli_fname ? info.dli_fname : "";
}
#endif

int main() {
    using namespace stb;

    // colour: stb::color descends from stb::color, so the two must agree.
    CHECK(color("red").value() == 0xFF0000FFu);
    CHECK(color("#00ff0080").a() == 0x80);

    // zlib round trip.
    std::vector<std::uint8_t> raw(4096);
    for (std::size_t i = 0; i < raw.size(); ++i) raw[i] = static_cast<std::uint8_t>((i * 7) ^ (i >> 3));
    const std::vector<std::uint8_t> packed = zlib_compress(raw);
    CHECK(zlib_decompress(packed) == raw);

    // image round trip through PNG: 2x2 RGBA.
    const std::vector<std::uint8_t> px = {255, 0,   0,   255, 0,   255, 0,   128,
                                          0,   0,   255, 0,   10,  20,  30,  255};
    image img(2, 2, pixel_format::rgba, px);
    const std::vector<std::uint8_t> png = img.to_png();
    const image back = image::decode(png.data(), png.size());
    CHECK(back.width() == 2 && back.height() == 2 && back.channels() == 4);
    CHECK(std::memcmp(back.data(), px.data(), px.size()) == 0);

#ifdef NOVA_CAN_DLADDR
    // libstb must live in its own shared object, not in this executable.
    using compress_fn = std::vector<std::uint8_t> (*)(const void*, std::size_t, int);
    const char* lib = object_of(reinterpret_cast<const void*>(static_cast<compress_fn>(&zlib_compress)));
    const char* exe = object_of(reinterpret_cast<const void*>(&local_marker));
    std::printf("libstb loaded from: %s\n", lib);
    CHECK(lib[0] != '\0');
    CHECK(std::strcmp(lib, exe) != 0);  // same file would mean it was linked statically
#endif

    if (failures == 0) std::printf("novasvg -> libstb link check: OK\n");
    return failures == 0 ? 0 : 1;
}
