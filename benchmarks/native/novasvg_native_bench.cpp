// Drives novasvg through its own C++ API directly (novasvg::Document /
// novasvg::Bitmap) -- see manifest.h for the shared timing/manifest
// protocol every native_*_bench binary speaks.
//
// Before timing anything, one throwaway render pays novasvg's process-wide
// font-cache warm-up cost so that one-time cost doesn't land arbitrarily
// on whichever sample happens to render text first.

#include "manifest.h"
#include <novasvg/novasvg.h>

int main(int argc, char** argv)
{
    if(argc < 3) {
        std::cerr << "usage: novasvg_native_bench <manifest.tsv> <runs>\n";
        return 2;
    }
    const auto jobs = bench::readManifest(argv[1]);
    const int runs = std::max(1, std::stoi(argv[2]));
    std::cout << "@@VERSION\t" << novasvg::versionString() << "\n";

    {
        auto warm = novasvg::Document::loadFromData(
            "<svg xmlns='http://www.w3.org/2000/svg' width='1' height='1'>"
            "<text x='0' y='1'>x</text></svg>");
        if(warm)
            warm->renderToBitmap(1, 1, novasvg::Color::Transparent);
    }

    bench::runAll(jobs, runs, [](const bench::Job& job, bool isLast) {
        auto doc = novasvg::Document::loadFromFile(job.svg_path);
        if(!doc)
            throw std::runtime_error("failed to load/parse SVG");
        auto bitmap = doc->renderToBitmap(job.width, job.height, novasvg::Color::Transparent);
        if(bitmap.isNull())
            throw std::runtime_error("renderToBitmap returned a null bitmap");
        if(isLast && !bitmap.write(job.out_path))
            throw std::runtime_error("failed to write output PNG to " + job.out_path);
        return true;
    });

    return 0;
}
