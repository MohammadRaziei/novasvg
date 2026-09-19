// Drives lunasvg through its own C++ API (lunasvg::Document /
// lunasvg::Bitmap) directly -- no Python binding, no CLI. See manifest.h
// for the shared timing/manifest protocol every native_*_bench binary
// speaks.

#include "manifest.h"
#include <lunasvg.h>

int main(int argc, char** argv)
{
    if(argc < 3) {
        std::cerr << "usage: lunasvg_native_bench <manifest.tsv> <runs>\n";
        return 2;
    }
    const auto jobs = bench::readManifest(argv[1]);
    const int runs = std::max(1, std::stoi(argv[2]));
    std::cout << "@@VERSION\t" << LUNASVG_VERSION_STRING << "\n";

    bench::runAll(jobs, runs, [](const bench::Job& job, bool isLast) {
        auto doc = lunasvg::Document::loadFromFile(job.svg_path);
        if(!doc)
            throw std::runtime_error("failed to load/parse SVG");
        auto bitmap = doc->renderToBitmap(job.width, job.height);
        if(bitmap.isNull())
            throw std::runtime_error("renderToBitmap returned a null bitmap");
        if(isLast && !bitmap.writeToPng(job.out_path))
            throw std::runtime_error("failed to write output PNG to " + job.out_path);
        return true;
    });

    return 0;
}
