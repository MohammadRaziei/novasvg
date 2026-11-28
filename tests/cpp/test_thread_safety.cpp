#include "doctest.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <nova/svg/svg.h>

// The guarantee (docs/adr/0017): documents that do not share an object may be loaded and rendered on different threads
// at the same time (the process-wide font cache is the one thing they share, and it is locked); and one document may be
// rendered, to an image or to a PDF, by several threads at once (its layout and bounding boxes are filled in lazily, on
// const calls too, under a lock). A call that changes a document needs the document to itself.
using nova::svg::document;

namespace {

// Text, a gradient, a mask, a clip mask and a foreignObject: the paths that touch the shared font cache and the
// parts of the renderer with the most state.
const char* sample = R"(
<svg xmlns='http://www.w3.org/2000/svg' width='160' height='100' viewBox='0 0 160 100'>
  <defs>
    <linearGradient id='g'><stop offset='0' stop-color='#fff'/><stop offset='1' stop-color='#06c'/></linearGradient>
    <mask id='m'><circle cx='40' cy='50' r='30' fill='#fff'/></mask>
    <clipPath id='c'><circle cx='110' cy='50' r='20'/><circle cx='130' cy='50' r='20'/></clipPath>
  </defs>
  <rect width='160' height='100' fill='url(#g)' mask='url(#m)'/>
  <rect width='160' height='100' fill='#c30' clip-path='url(#c)' opacity='.8'/>
  <text x='8' y='92' font-size='14' font-family='sans-serif'>thread safety</text>
  <foreignObject x='70' y='4' width='80' height='40' font-size='11'>
    <div xmlns='http://www.w3.org/1999/xhtml'>DELIVERY-ADDRESS <code>code</code></div>
  </foreignObject>
</svg>)";

struct output {
    std::vector<uint8_t> pdf;
    std::vector<uint8_t> pixels;
    bool operator==(const output& other) const { return pdf == other.pdf && pixels == other.pixels; }
};

output render_one()
{
    output out;
    auto doc = document::load_data(sample);
    if(doc == nullptr)
        return out;
    out.pdf = doc->render_to_pdf(document::pdf_mode::vector, 192.f);
    const stb::image image = doc->render_to_image(160, 100);
    if(!image.empty())
        out.pixels.assign(image.data(), image.data() + size_t(image.stride()) * size_t(image.height()));
    return out;
}

}

TEST_CASE("threads: separate documents load and render at the same time, each giving the single-thread result")
{
    // The threads start first, on a cold process (no font cache, no embedded font loaded yet), so that a race in the
    // first use of a shared cache is in the test too; the single-thread result is taken after they are done.
    constexpr int thread_count = 8;
    constexpr int rounds = 4;
    std::vector<std::vector<output>> results(thread_count);
    std::vector<std::thread> threads;
    for(int t = 0; t < thread_count; ++t)
        threads.emplace_back([&results, t] {
            for(int r = 0; r < rounds; ++r)
                results[size_t(t)].push_back(render_one());
        });
    for(auto& thread : threads)
        thread.join();

    const output reference = render_one();
    REQUIRE(!reference.pdf.empty());
    REQUIRE(!reference.pixels.empty());
    int different = 0;
    for(const auto& per_thread : results)
        for(const auto& out : per_thread)
            different += !(out == reference);
    CHECK(different == 0);
}

namespace fs = std::filesystem;

// Every sample of the repository (data/), one document at a time, rendered to an image and to a PDF by
// three threads at once. The same document, so what the threads share is what a document fills in lazily: its layout and
// the memo of every paint bounding box. Odd documents have their layout done first, even ones leave it to the threads.
TEST_CASE("threads: one document rendered from several threads, every sample, layout done or not")
{
    const fs::path root = fs::path(__FILE__).parent_path().parent_path().parent_path();
    std::vector<fs::path> files;
    REQUIRE(fs::exists(root / "data"));
    for(const auto& e : fs::recursive_directory_iterator(root / "data"))
        if(e.path().extension() == ".svg")
            files.push_back(e.path());
    REQUIRE(!files.empty());
    int index = 0;
    for(const auto& f : files) {
        std::ifstream in(f, std::ios::binary);
        std::stringstream ss;
        ss << in.rdbuf();
        auto doc = document::load_data(ss.str());
        if(doc == nullptr)
            continue;
        if(++index % 2 == 1)
            (void)doc->width();
        std::vector<std::thread> threads;
        for(int t = 0; t < 3; ++t)
            threads.emplace_back([&] {
                (void)doc->render_to_image(120, 90);
                (void)doc->render_to_pdf(document::pdf_mode::vector, 96.f);
            });
        for(auto& th : threads)
            th.join();
    }
}
