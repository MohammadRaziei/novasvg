// Drives resvg through its real C API (resvg.h, built from source by
// ../cmake/FetchResvg.cmake via `cargo build --release`) directly -- not
// the resvg-py Python binding, no CLI. PNG encoding uses vendored
// stb_image_write.h: resvg_render fills a raw premultiplied-RGBA8888
// buffer directly, so we un-premultiply before handing it to stb (PNG
// wants straight alpha). See manifest.h for the shared timing/manifest
// protocol.

#include "manifest.h"
#include "resvg.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "thirdparty/stb_image_write.h"

namespace {

void unpremultiply(unsigned char* pixels, size_t count)
{
    for(size_t i = 0; i < count; ++i) {
        unsigned char* p = pixels + i * 4;
        const unsigned char a = p[3];
        if(a == 0 || a == 255)
            continue;
        for(int c = 0; c < 3; ++c) {
            int v = (p[c] * 255 + a / 2) / a;
            p[c] = static_cast<unsigned char>(v > 255 ? 255 : v);
        }
    }
}

} // namespace

int main(int argc, char** argv)
{
    if(argc < 3) {
        std::cerr << "usage: resvg_native_bench <manifest.tsv> <runs>\n";
        return 2;
    }
    const auto jobs = bench::readManifest(argv[1]);
    const int runs = std::max(1, std::stoi(argv[2]));
    std::cout << "@@VERSION\t" << RESVG_VERSION << "\n";

    resvg_options* opt = resvg_options_create();

    bench::runAll(jobs, runs, [opt](const bench::Job& job, bool isLast) {
        resvg_render_tree* tree = nullptr;
        if(resvg_parse_tree_from_file(job.svg_path.c_str(), opt, &tree) != RESVG_OK)
            throw std::runtime_error("failed to load/parse SVG");

        const resvg_size intrinsic = resvg_get_image_size(tree);
        resvg_transform tf = resvg_transform_identity();
        tf.a = job.width / intrinsic.width;
        tf.d = job.height / intrinsic.height;

        std::vector<unsigned char> pixels(static_cast<size_t>(job.width) * job.height * 4, 0);
        resvg_render(tree, tf, static_cast<uint32_t>(job.width), static_cast<uint32_t>(job.height),
                     reinterpret_cast<char*>(pixels.data()));
        resvg_tree_destroy(tree);

        if(isLast) {
            unpremultiply(pixels.data(), static_cast<size_t>(job.width) * job.height);
            if(!stbi_write_png(job.out_path.c_str(), job.width, job.height, 4, pixels.data(), job.width * 4))
                throw std::runtime_error("failed to write output PNG to " + job.out_path);
        }
        return true;
    });

    resvg_options_destroy(opt);
    return 0;
}
