// ADR-0003 evidence: cost of the premultiplied-ARGB32 -> straight-RGBA conversion (the one pass paid at the
// boundary with stb::image). The loop is a verbatim copy of novasvg's convert_argb_to_rgba (in place).
//
//   g++ -O3 -std=c++17 conversion.cpp -o conversion && ./conversion
//
// Two runs on 2026-10-31 (GCC 13.3, one core): 1.3 to 5.6 ns/pixel, 21 to 94 ms for 4096 x 4096
// (cheapest when most pixels are opaque or fully transparent, dearest when every pixel is translucent).
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
// exact copy of novasvg's convert_argb_to_rgba
void convert_argb_to_rgba(unsigned char* dst, const unsigned char* src, int width, int height, int stride) {
    for (int y = 0; y < height; y++) {
        const uint32_t* src_row = (const uint32_t*)(src + stride * y);
        unsigned char* dst_row = dst + stride * y;
        for (int x = 0; x < width; x++) {
            uint32_t pixel = src_row[x];
            uint32_t a = (pixel >> 24) & 0xFF;
            if (a == 0) { *dst_row++ = 0; *dst_row++ = 0; *dst_row++ = 0; *dst_row++ = 0; }
            else {
                uint32_t r = (pixel >> 16) & 0xFF, g = (pixel >> 8) & 0xFF, b = pixel & 0xFF;
                if (a != 255) { r = (r * 255) / a; g = (g * 255) / a; b = (b * 255) / a; }
                *dst_row++ = r; *dst_row++ = g; *dst_row++ = b; *dst_row++ = a;
            }
        }
    }
}
int main() {
    const int W = 4096, H = 4096;
    auto make = [&](int mode) { // mode 0: all opaque, 1: half transparent, 2: all translucent (worst: divides every pixel)
        std::vector<uint32_t> v(size_t(W) * H); uint32_t s = 12345;
        for (auto& p : v) { s = s * 1664525u + 1013904223u; uint32_t a = mode == 0 ? 255 : mode == 1 ? ((s >> 8) & 1 ? 0 : 255) : 1 + ((s >> 8) % 254);
            uint32_t c = (s >> 16) % (a + 1); p = (a << 24) | (c << 16) | (c << 8) | c; }
        return v; };
    const char* names[] = {"all opaque (typical background filled)", "half transparent / half opaque", "all translucent (worst case)"};
    for (int m = 0; m < 3; m++) {
        auto v = make(m); auto copy = v;
        double best = 1e9;
        for (int rep = 0; rep < 5; rep++) {
            copy = v; auto t0 = std::chrono::steady_clock::now();
            convert_argb_to_rgba((unsigned char*)copy.data(), (unsigned char*)copy.data(), W, H, W * 4);  // in place
            best = std::min(best, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count());
        }
        std::printf("%-42s 16.7 Mpx in place: %7.1f ms  (%.2f ns/px)\n", names[m], best, best * 1e6 / (double(W) * H));
    }
}
