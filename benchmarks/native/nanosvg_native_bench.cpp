// Drives nanosvg (single-header C parser + rasterizer, nsvgParseFromFile /
// nsvgRasterize) directly -- the lightest-weight engine in this benchmark
// by design: no filters, no CSS, no text layout, just paths and gradients.
// Included here as a baseline, not a fair fight with the other engines.
// PNG encoding uses vendored stb_image_write.h since nanosvg only
// rasterizes to a raw RGBA buffer and has no PNG writer of its own. See
// manifest.h for the shared timing/manifest protocol.

#include "manifest.h"

#define NANOSVG_IMPLEMENTATION
#include <nanosvg.h>
#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvgrast.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "thirdparty/stb_image_write.h"

#include <memory>

int main(int argc, char** argv)
{
    if(argc < 3) {
        std::cerr << "usage: nanosvg_native_bench <manifest.tsv> <runs>\n";
        return 2;
    }
    const auto jobs = bench::readManifest(argv[1]);
    const int runs = std::max(1, std::stoi(argv[2]));
    std::cout << "@@VERSION\tgit-master (no version macro upstream)\n";

    bench::runAll(jobs, runs, [](const bench::Job& job, bool isLast) {
        NSVGimage* image = nsvgParseFromFile(job.svg_path.c_str(), "px", 96.0f);
        if(!image)
            throw std::runtime_error("failed to load/parse SVG");
        if(image->width <= 0.f || image->height <= 0.f) {
            nsvgDelete(image);
            throw std::runtime_error("image has no intrinsic size (nanosvg needs width/height or viewBox)");
        }

        const float scale = job.width / image->width;
        std::vector<unsigned char> pixels(static_cast<size_t>(job.width) * job.height * 4);

        NSVGrasterizer* rast = nsvgCreateRasterizer();
        if(!rast) {
            nsvgDelete(image);
            throw std::runtime_error("nsvgCreateRasterizer failed");
        }
        nsvgRasterize(rast, image, 0, 0, scale, pixels.data(), job.width, job.height, job.width * 4);
        nsvgDeleteRasterizer(rast);
        nsvgDelete(image);

        if(isLast && !stbi_write_png(job.out_path.c_str(), job.width, job.height, 4, pixels.data(), job.width * 4))
            throw std::runtime_error("failed to write output PNG to " + job.out_path);
        return true;
    });

    return 0;
}
