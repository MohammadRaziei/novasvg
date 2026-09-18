// Drives novasvg through its own C++ API directly (novasvg::Document /
// novasvg::Bitmap) -- not the novasvg_cli binary, not a subprocess per
// render. Every sample in the corpus is timed inside this one process, the
// same way python/engines.py times resvg/lunasvg/cairosvg/thorvg in one
// long-lived Python process: no process-spawn or library-init cost gets
// counted once per sample, so novasvg's numbers are finally apples-to-apples
// with the other 4.
//
// Reads a tab-separated manifest (one job per line: name, svg path, output
// PNG path, width, height) and a run count, both as argv. For each job,
// times `runs` full load+render passes and reports the median, writing the
// PNG from the last pass. Output is one line per job to stdout:
//   <name>\tOK\t<median_seconds>
//   <name>\tFAIL\t<error message>
// plus a leading "@@VERSION\t<novasvg::versionString()>" line -- kept as
// plain tab-separated text rather than JSON so this stays a small,
// dependency-free translation unit; python/run_native_novasvg.py is the
// only thing that parses it.

#include <novasvg/novasvg.h>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct Job {
    std::string name;
    std::string svg_path;
    std::string out_path;
    int width = 0;
    int height = 0;
};

std::vector<Job> readManifest(const std::string& path)
{
    std::vector<Job> jobs;
    std::ifstream in(path);
    std::string line;
    while(std::getline(in, line)) {
        if(line.empty())
            continue;
        std::istringstream ss(line);
        Job job;
        std::string w_str, h_str;
        std::getline(ss, job.name, '\t');
        std::getline(ss, job.svg_path, '\t');
        std::getline(ss, job.out_path, '\t');
        std::getline(ss, w_str, '\t');
        std::getline(ss, h_str, '\t');
        job.width = std::stoi(w_str);
        job.height = std::stoi(h_str);
        jobs.push_back(std::move(job));
    }
    return jobs;
}

} // namespace

int main(int argc, char** argv)
{
    if(argc < 3) {
        std::cerr << "usage: novasvg_native_bench <manifest.tsv> <runs>\n";
        return 2;
    }

    const auto jobs = readManifest(argv[1]);
    const int runs = std::max(1, std::stoi(argv[2]));

    std::cout << "@@VERSION\t" << novasvg::versionString() << "\n";

    for(const auto& job : jobs) {
        std::vector<double> times;
        times.reserve(runs);
        bool ok = true;
        std::string error;

        for(int i = 0; i < runs && ok; ++i) {
            const auto t0 = std::chrono::steady_clock::now();
            auto doc = novasvg::Document::loadFromFile(job.svg_path);
            if(!doc) {
                ok = false;
                error = "failed to load/parse SVG";
                break;
            }
            auto bitmap = doc->renderToBitmap(job.width, job.height, novasvg::Color::Transparent);
            if(bitmap.isNull()) {
                ok = false;
                error = "renderToBitmap returned a null bitmap";
                break;
            }
            const auto t1 = std::chrono::steady_clock::now();
            times.push_back(std::chrono::duration<double>(t1 - t0).count());

            if(i == runs - 1 && !bitmap.write(job.out_path)) {
                ok = false;
                error = "failed to write output PNG to " + job.out_path;
            }
        }

        if(ok) {
            std::sort(times.begin(), times.end());
            const double median = times[times.size() / 2];
            std::cout << job.name << "\tOK\t" << median << "\n";
        } else {
            std::cout << job.name << "\tFAIL\t" << error << "\n";
        }
    }

    return 0;
}
