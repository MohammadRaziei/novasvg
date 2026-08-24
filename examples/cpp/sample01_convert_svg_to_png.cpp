#include <filesystem>
#include <iostream>
#include <string>
#include <algorithm>

#include <novasvg/novasvg.h>

namespace {
    std::filesystem::path project_root() {
        return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
    }

    // Case-insensitive extension check
    bool has_svg_extension(const std::filesystem::path& path) {
        auto ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == ".svg";
    }
}

int main() {
    const auto input_dir = project_root() / "data";
    const auto output_dir = std::filesystem::current_path() / "output";

    // Create output directory if it doesn't exist
    std::filesystem::create_directories(output_dir);

    if (!std::filesystem::exists(input_dir) || !std::filesystem::is_directory(input_dir)) {
        std::cerr << "Error: Input directory does not exist: " << input_dir << "\n";
        return 1;
    }

    std::cout << "Scanning for SVG files in: " << input_dir << "\n\n";

    int success_count = 0;
    int failure_count = 0;

    // Iterate over all files in the input directory
    for (const auto& entry : std::filesystem::directory_iterator(input_dir)) {
        if (!entry.is_regular_file() || !has_svg_extension(entry.path())) {
            continue;  // Skip non-SVG files and directories
        }

        const auto& svg_path = entry.path();
        auto png_filename = svg_path.stem().string() + ".png";
        const auto png_path = output_dir / png_filename;

        std::cout << "Processing: " << svg_path.filename() << " ... ";

        // Load SVG document
        auto document = novasvg::Document::loadFromFile(svg_path.string());
        if (!document) {
            std::cerr << "FAILED (load)\n";
            failure_count++;
            continue;
        }

        // Render to bitmap
        auto bitmap = document->renderToBitmap();
        if (bitmap.isNull()) {
            std::cerr << "FAILED (render)\n";
            failure_count++;
            continue;
        }

        // Write PNG
        if (!bitmap.writeToPng(png_path.string())) {
            std::cerr << "FAILED (write)\n";
            failure_count++;
            continue;
        }

        std::cout << "OK → " << png_path.filename() << "\n";
        success_count++;
    }

    // Summary
    std::cout << "\n=== Conversion Summary ===\n";
    std::cout << "Success: " << success_count << "\n";
    std::cout << "Failed:  " << failure_count << "\n";
    std::cout << "Output directory: " << output_dir << "\n";

    return (failure_count == 0) ? 0 : 1;
}