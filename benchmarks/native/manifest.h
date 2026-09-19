// Shared by every native_*_bench.cpp: reads the same tab-separated job
// manifest (name, svg path, output PNG path, width, height) that
// python/native_*.py writes, so each engine's driver only has to
// implement the actual load+render+save part.
#pragma once

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace bench {

struct Job {
    std::string name;
    std::string svg_path;
    std::string out_path;
    int width = 0;
    int height = 0;
};

inline std::vector<Job> readManifest(const std::string& path)
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

// Shared driver: for each job, calls render(job, isLastRun) `runs` times,
// timing every call, and prints "<name>\tOK\t<median_seconds>" or
// "<name>\tFAIL\t<message>" -- the same protocol every native_*_bench
// binary speaks, so python/native_*.py has one parser for all of them.
// `render` returns true/false and is expected to write job.out_path only
// when isLastRun is true (no point re-encoding a PNG on every timed rep).
// On failure it may throw std::runtime_error with a message, or just
// return false (reported as a generic failure).
template <typename RenderFn>
inline void runAll(const std::vector<Job>& jobs, int runs, RenderFn render)
{
    for(const auto& job : jobs) {
        std::vector<double> times;
        times.reserve(runs);
        bool ok = true;
        std::string error;

        for(int i = 0; i < runs && ok; ++i) {
            const bool isLast = (i == runs - 1);
            const auto t0 = std::chrono::steady_clock::now();
            try {
                ok = render(job, isLast);
                if(!ok)
                    error = "render() returned false";
            } catch(const std::exception& e) {
                ok = false;
                error = e.what();
            }
            const auto t1 = std::chrono::steady_clock::now();
            if(ok)
                times.push_back(std::chrono::duration<double>(t1 - t0).count());
        }

        if(ok) {
            std::sort(times.begin(), times.end());
            std::cout << job.name << "\tOK\t" << times[times.size() / 2] << "\n";
        } else {
            std::cout << job.name << "\tFAIL\t" << error << "\n";
        }
    }
}

} // namespace bench
