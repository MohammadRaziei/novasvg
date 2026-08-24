#include <filesystem>
#include <iostream>

#include <novasvg/novasvg.h>

namespace {
    std::filesystem::path project_root() {
        return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
    }
}

int main()
{
    const auto input = project_root() / "data" / "circle.svg";

    auto document = novasvg::Document::loadFromFile(input.string());
    if(!document)
    {
        std::cerr << "Failed to load SVG: " << input << "\n";
        return 1;
    }

    std::cout << "SVG size: " << document->width() << "x" << document->height() << "\n";
    return 0;
}
