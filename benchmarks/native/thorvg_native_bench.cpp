// Drives thorvg through its own C++ API (thorvg::SwCanvas / thorvg::Picture)
// directly -- no Python binding, no CLI. thorvg has no CMake build of its
// own (see ../cmake/FetchThorvg.cmake, which drives meson+ninja for it), but
// once built it links like any other native dependency here. PNG encoding
// uses vendored stb_image_write.h: thorvg's software canvas fills a raw
// pixel buffer directly, it doesn't write files itself in this minimal
// (no-savers) build. See manifest.h for the shared timing/manifest
// protocol.

#include "manifest.h"
#include <thorvg.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "thirdparty/stb_image_write.h"

int main(int argc, char** argv)
{
    if(argc < 3) {
        std::cerr << "usage: thorvg_native_bench <manifest.tsv> <runs>\n";
        return 2;
    }
    const auto jobs = bench::readManifest(argv[1]);
    const int runs = std::max(1, std::stoi(argv[2]));

    if(tvg::Initializer::init(0) != tvg::Result::Success) {
        std::cerr << "thorvg::Initializer::init failed\n";
        return 1;
    }
    std::cout << "@@VERSION\t" << TVG_VERSION_MAJOR << "." << TVG_VERSION_MINOR << "." << TVG_VERSION_MICRO << "\n";

    bench::runAll(jobs, runs, [](const bench::Job& job, bool isLast) {
        // ABGR8888S: alpha,blue,green,red as a 32-bit value, un-premultiplied
        // -- in little-endian memory that's byte order R,G,B,A, which is
        // exactly what stbi_write_png expects for comp=4, no swizzling needed.
        std::vector<uint32_t> pixels(static_cast<size_t>(job.width) * job.height, 0);

        auto* canvas = tvg::SwCanvas::gen();
        if(!canvas)
            throw std::runtime_error("SwCanvas::gen() failed");
        if(canvas->target(pixels.data(), job.width, job.width, job.height, tvg::ColorSpace::ABGR8888S) != tvg::Result::Success) {
            delete canvas;
            throw std::runtime_error("SwCanvas::target failed");
        }

        auto* picture = tvg::Picture::gen();
        if(!picture || picture->load(job.svg_path.c_str()) != tvg::Result::Success) {
            delete canvas;
            throw std::runtime_error("failed to load/parse SVG");
        }
        picture->size(static_cast<float>(job.width), static_cast<float>(job.height));

        if(canvas->add(picture) != tvg::Result::Success) {
            delete canvas;
            throw std::runtime_error("Canvas::add failed");
        }
        canvas->draw(true);
        canvas->sync();

        bool ok = true;
        if(isLast && !stbi_write_png(job.out_path.c_str(), job.width, job.height, 4, pixels.data(), job.width * 4))
            ok = false;

        delete canvas; // also destroys the picture it now owns
        if(!ok)
            throw std::runtime_error("failed to write output PNG to " + job.out_path);
        return true;
    });

    tvg::Initializer::term();
    return 0;
}
