// ADR-0003 evidence: source-over on 4096 x 4096 random translucent pixels, novasvg's real premultiplied
// kernels against libstb's image::blend (straight alpha, hand-written SIMD, per backend).
// Every figure includes one 64 MB memcpy (~0.33 ns/pixel) that both sides pay.
//
//   NOVASVG=<novasvg checkout>  BUILD=<novasvg build dir, after one configure>
//   g++ -O3 -std=c++20 -I$NOVASVG/include -I$BUILD/_deps/libstb-src/include -I$BUILD/_deps/libstb-build \
//       composite.cpp -o composite -L$BUILD/_deps/libstb-build -lstb -Wl,-rpath,$BUILD/_deps/libstb-build
//   ./composite
//
// Two runs on 2026-10-31 (GCC 13.3, one core, AVX2 machine), ns/pixel: novasvg buffer-over-buffer 2.35-2.37,
// solid fill 1.14-1.20; libstb avx2 2.39-2.59, sse2 3.37-3.52, scalar 7.41-8.00.
#include <chrono>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <vector>
#include "novasvg/novasvg.h"
#include <stb/image.hpp>
using namespace novasvg::render;
static double ns_per_px(auto&& fn, size_t n, int reps = 5) {
    double best = 1e18;
    for (int r = 0; r < reps; r++) { auto t = std::chrono::steady_clock::now(); fn();
        best = std::min(best, std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - t).count()); }
    return best / n;
}
int main() {
    const int W = 4096, H = 4096; const size_t N = size_t(W) * H;
    uint32_t r = 7; auto rnd = [&] { r = r * 1664525u + 1013904223u; return r >> 8; };
    // random translucent source and destination, both alphas 1..254
    std::vector<uint8_t> sS(N * 4), dS(N * 4);                 // straight RGBA bytes
    std::vector<uint32_t> sP(N), dP0(N), dP(N);               // premultiplied ARGB32 words
    for (size_t i = 0; i < N; i++) {
        uint32_t sa = 1 + rnd() % 254, da = 1 + rnd() % 254, c[6];
        for (auto& x : c) x = rnd() % 256;
        sS[i*4] = c[0]; sS[i*4+1] = c[1]; sS[i*4+2] = c[2]; sS[i*4+3] = sa;
        dS[i*4] = c[3]; dS[i*4+1] = c[4]; dS[i*4+2] = c[5]; dS[i*4+3] = da;
        auto pm = [](uint32_t v, uint32_t a) { return (v * a + 127) / 255; };
        sP[i] = (sa << 24) | (pm(c[0], sa) << 16) | (pm(c[1], sa) << 8) | pm(c[2], sa);
        dP0[i] = (da << 24) | (pm(c[3], da) << 16) | (pm(c[4], da) << 8) | pm(c[5], da);
    }
    std::puts("source-over, 4096x4096 (16.7 Mpx), random translucent source over translucent destination:\n");
    std::puts("novasvg's real kernels (premultiplied ARGB32):");
    std::printf("  composition_source_over       (pixel buffer over buffer)   %5.2f ns/px\n",
        ns_per_px([&] { dP = dP0; composition_source_over(dP.data(), int(N), sP.data(), 255); }, N));
    std::printf("  composition_solid_source_over (solid colour fill: the common case)  %5.2f ns/px\n",
        ns_per_px([&] { dP = dP0; composition_solid_source_over(dP.data(), int(N), 0x80402010u, 255); }, N));
    // subtract the cost of the vector copy 'dP = dP0' that both loops include
    std::printf("  (memcpy baseline included above)                              %5.2f ns/px\n",
        ns_per_px([&] { dP = dP0; }, N));
    std::puts("\nlibstb image::blend (straight alpha, hand-written SIMD):");
    for (const char* be : {"avx2", "sse2", "scalar"}) {
        if (!stb::set_simd(be)) { std::printf("  %-7s unavailable\n", be); continue; }
        stb::image over = stb::image::wrap(W, H, stb::pixel_format::rgba, sS.data());
        std::vector<uint8_t> work(N * 4);
        double t = ns_per_px([&] { std::memcpy(work.data(), dS.data(), work.size());
            stb::image base = stb::image::wrap(W, H, stb::pixel_format::rgba, work.data());
            base.blend(over); }, N);
        std::printf("  %-7s %5.2f ns/px   (includes the same memcpy)\n", be, t);
    }
    stb::set_simd("auto");
}
