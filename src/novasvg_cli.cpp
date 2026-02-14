/**
 * NovaSVG Command Line Interface
 * 
 * A comprehensive CLI tool for working with SVG files using the NovaSVG library.
 * 
 * Features:
 * - Convert SVG to PNG
 * - Query SVG information (size, bounding box, file size)
 * - Extract elements using CSS selectors
 * - Apply CSS stylesheets
 * - Manage fonts
 * - Batch processing
 */

#include <novasvg/novasvg.h>
#include "CLI11.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

// Utility function to convert bytes to human readable format
std::string human_readable_size(std::uintmax_t bytes) {
    const char* suffixes[] = {"B", "KB", "MB", "GB", "TB"};
    double size = static_cast<double>(bytes);
    int i = 0;

    while (size >= 1024 && i < 4) {
        size /= 1024;
        ++i;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << suffixes[i];
    return oss.str();
}

// Command implementations
int cmd_convert(const std::string& input, const std::string& output, 
                int width, int height, uint32_t bg_color, float scale) {
    std::unique_ptr<novasvg::Document> doc = novasvg::Document::loadFromFile(input);
    if (!doc) {
        std::cerr << "Error: Failed to load SVG file: " << input << "\n";
        return 1;
    }

    std::cout << "Converting: " << input << " -> " << output << "\n";
    std::cout << "Original size: " << doc->width() << "x" << doc->height() << "px\n";
    
    if (scale > 0.0f && scale != 1.0f) {
        width = static_cast<int>(doc->width() * scale);
        height = static_cast<int>(doc->height() * scale);
    }
    
    if (width > 0 || height > 0) {
        std::cout << "Output size: ";
        if (width > 0) std::cout << width << "x";
        if (height > 0) std::cout << height << "px\n";
    }

    auto bitmap = doc->renderToBitmap(width, height, bg_color);
    if (bitmap.isNull()) {
        std::cerr << "Error: Failed to render SVG\n";
        return 1;
    }

    if (!bitmap.writeToPng(output)) {
        std::cerr << "Error: Failed to save PNG file: " << output << "\n";
        return 1;
    }

    std::cout << "Successfully converted to " << output << "\n";
    return 0;
}

int cmd_info(const std::string& input, bool show_file_size) {
    std::unique_ptr<novasvg::Document> doc = novasvg::Document::loadFromFile(input);
    if (!doc) {
        std::cerr << "Error: Failed to load SVG file: " << input << "\n";
        return 1;
    }

    std::cout << "SVG Information:\n";
    std::cout << "  File: " << input << "\n";
    std::cout << "  Size: " << doc->width() << "x" << doc->height() << "px\n";
    
    auto bbox = doc->boundingBox();
    std::cout << "  Bounding Box: " 
              << bbox.x << "," << bbox.y << " "
              << bbox.w << "x" << bbox.h << "\n";
    
    if (show_file_size) {
        if (fs::exists(input)) {
            auto size = fs::file_size(input);
            std::cout << "  File size: " << size << " bytes\n";
            std::cout << "  Readable size: " << human_readable_size(size) << "\n";
        } else {
            std::cout << "  File size: N/A\n";
        }
    }

    return 0;
}

int cmd_query(const std::string& selector, const std::string& input) {
    std::unique_ptr<novasvg::Document> doc = novasvg::Document::loadFromFile(input);
    if (!doc) {
        std::cerr << "Error: Failed to load SVG file: " << input << "\n";
        return 1;
    }

    auto elements = doc->querySelectorAll(selector);
    std::cout << "Found " << elements.size() << " element(s) matching: " << selector << "\n\n";
    
    for (size_t i = 0; i < elements.size(); i++) {
        const auto& elem = elements[i];
        auto bbox = elem.getBoundingBox();
        auto local_bbox = elem.getLocalBoundingBox();
        auto global_bbox = elem.getGlobalBoundingBox();
        
        std::cout << "Element #" << (i + 1) << ":\n";
        std::cout << "  Bounding Box: " 
                  << bbox.x << "," << bbox.y << " "
                  << bbox.w << "x" << bbox.h << "\n";
        std::cout << "  Local BBox: " 
                  << local_bbox.x << "," << local_bbox.y << " "
                  << local_bbox.w << "x" << local_bbox.h << "\n";
        std::cout << "  Global BBox: " 
                  << global_bbox.x << "," << global_bbox.y << " "
                  << global_bbox.w << "x" << global_bbox.h << "\n";
        
        // Try to get common attributes
        if (elem.hasAttribute("id")) {
            std::cout << "  ID: " << elem.getAttribute("id") << "\n";
        }
        if (elem.hasAttribute("class")) {
            std::cout << "  Class: " << elem.getAttribute("class") << "\n";
        }
        if (elem.hasAttribute("fill")) {
            std::cout << "  Fill: " << elem.getAttribute("fill") << "\n";
        }
        if (elem.hasAttribute("stroke")) {
            std::cout << "  Stroke: " << elem.getAttribute("stroke") << "\n";
        }
        
        std::cout << "\n";
    }

    return 0;
}

int cmd_apply_css(const std::string& css_file, const std::string& input, const std::string& output) {
    // Load CSS
    std::ifstream css_stream(css_file);
    if (!css_stream) {
        std::cerr << "Error: Failed to open CSS file: " << css_file << "\n";
        return 1;
    }
    
    std::string css_content((std::istreambuf_iterator<char>(css_stream)),
                            std::istreambuf_iterator<char>());
    
    // Load SVG
    std::unique_ptr<novasvg::Document> doc = novasvg::Document::loadFromFile(input);
    if (!doc) {
        std::cerr << "Error: Failed to load SVG file: " << input << "\n";
        return 1;
    }
    
    // Apply CSS
    doc->applyStyleSheet(css_content);
    
    // Re-render to apply styles
    doc->forceLayout();
    
    // For now, we'll just render to PNG since NovaSVG doesn't have SVG export
    std::cout << "Note: CSS applied successfully. Rendering to PNG instead of SVG export.\n";
    
    auto bitmap = doc->renderToBitmap();
    if (bitmap.isNull()) {
        std::cerr << "Error: Failed to render SVG after CSS application\n";
        return 1;
    }
    
    // Change extension to .png if output is .svg
    fs::path output_path(output);
    if (output_path.extension() == ".svg") {
        output_path.replace_extension(".png");
    }
    
    if (!bitmap.writeToPng(output_path.string())) {
        std::cerr << "Error: Failed to save output file: " << output_path.string() << "\n";
        return 1;
    }
    
    std::cout << "CSS applied and rendered to: " << output_path.string() << "\n";
    return 0;
}

int cmd_batch(const std::string& input_dir, const std::string& output_dir) {
    if (!fs::exists(input_dir) || !fs::is_directory(input_dir)) {
        std::cerr << "Error: Input directory does not exist: " << input_dir << "\n";
        return 1;
    }
    
    fs::create_directories(output_dir);
    
    int success = 0;
    int failed = 0;
    
    for (const auto& entry : fs::directory_iterator(input_dir)) {
        if (!entry.is_regular_file()) continue;
        
        auto path = entry.path();
        if (path.extension() != ".svg") continue;
        
        std::string input_file = path.string();
        std::string output_file = (fs::path(output_dir) / path.stem()).string() + ".png";
        
        std::cout << "Processing: " << path.filename() << " -> " 
                  << fs::path(output_file).filename() << "\n";
        
        std::unique_ptr<novasvg::Document> doc = novasvg::Document::loadFromFile(input_file);
        if (!doc) {
            std::cerr << "  Failed to load\n";
            failed++;
            continue;
        }
        
        auto bitmap = doc->renderToBitmap();
        if (bitmap.isNull()) {
            std::cerr << "  Failed to render\n";
            failed++;
            continue;
        }
        
        if (!bitmap.writeToPng(output_file)) {
            std::cerr << "  Failed to save\n";
            failed++;
            continue;
        }
        
        success++;
    }
    
    std::cout << "\nBatch processing complete:\n";
    std::cout << "  Success: " << success << "\n";
    std::cout << "  Failed:  " << failed << "\n";
    std::cout << "  Output:  " << output_dir << "\n";
    
    return failed > 0 ? 1 : 0;
}

int main(int argc, char** argv) {
    CLI::App app{"NovaSVG CLI - SVG processing tool"};
    
    // Version information
    app.set_version_flag("-v,--version", NOVASVG_VERSION_STRING, "Show version information");
    
    // Add footer with examples
    app.footer("\nEXAMPLES:\n"
               "  novasvg convert input.svg output.png\n"
               "  novasvg convert -w 800 -H 600 input.svg output.png\n"
               "  novasvg convert -s 2.0 input.svg output.png\n"
               "  novasvg info input.svg\n"
               "  novasvg info --size input.svg\n"
               "  novasvg query \"circle\" input.svg\n"
               "  novasvg query \"rect[fill='red']\" input.svg\n"
               "  novasvg batch input_dir/ output_dir/\n");
    
    // Convert command
    auto convert_cmd = app.add_subcommand("convert", "Convert SVG to PNG");
    std::string convert_input, convert_output;
    int convert_width = -1, convert_height = -1;
    std::string convert_bg_color;
    float convert_scale = 0.0f;
    
    convert_cmd->add_option("input", convert_input, "Input SVG file")->required()->check(CLI::ExistingFile);
    convert_cmd->add_option("output", convert_output, "Output PNG file")->required();
    convert_cmd->add_option("-w,--width", convert_width, "Output width in pixels");
    convert_cmd->add_option("-H,--height", convert_height, "Output height in pixels");
    convert_cmd->add_option("-b,--bg", convert_bg_color, "Background color (hex: RRGGBBAA, default: transparent)");
    convert_cmd->add_option("-s,--scale", convert_scale, "Scale factor");
    
    convert_cmd->callback([&]() {
        uint32_t bg_color = 0x00000000; // Transparent
        if (!convert_bg_color.empty()) {
            bg_color = std::stoul(convert_bg_color, nullptr, 16);
        }
        return cmd_convert(convert_input, convert_output, convert_width, convert_height, bg_color, convert_scale);
    });
    
    // Info command
    auto info_cmd = app.add_subcommand("info", "Display SVG information");
    std::string info_input;
    bool info_show_size = false;
    
    info_cmd->add_option("input", info_input, "Input SVG file")->required()->check(CLI::ExistingFile);
    info_cmd->add_flag("-s,--size", info_show_size, "Show file size information");
    
    info_cmd->callback([&]() {
        return cmd_info(info_input, info_show_size);
    });
    
    // Query command
    auto query_cmd = app.add_subcommand("query", "Query elements using CSS selectors");
    std::string query_selector, query_input;
    
    query_cmd->add_option("selector", query_selector, "CSS selector")->required();
    query_cmd->add_option("input", query_input, "Input SVG file")->required()->check(CLI::ExistingFile);
    
    query_cmd->callback([&]() {
        return cmd_query(query_selector, query_input);
    });
    
    // Apply CSS command
    auto apply_css_cmd = app.add_subcommand("apply-css", "Apply CSS stylesheet to SVG");
    std::string css_file, css_input, css_output;
    
    apply_css_cmd->add_option("css", css_file, "CSS file")->required()->check(CLI::ExistingFile);
    apply_css_cmd->add_option("input", css_input, "Input SVG file")->required()->check(CLI::ExistingFile);
    apply_css_cmd->add_option("output", css_output, "Output file")->required();
    
    apply_css_cmd->callback([&]() {
        return cmd_apply_css(css_file, css_input, css_output);
    });
    
    // Batch command
    auto batch_cmd = app.add_subcommand("batch", "Batch process multiple files");
    std::string batch_input, batch_output = "output";
    
    batch_cmd->add_option("input", batch_input, "Input directory")->required()->check(CLI::ExistingDirectory);
    batch_cmd->add_option("output", batch_output, "Output directory");
    
    batch_cmd->callback([&]() {
        return cmd_batch(batch_input, batch_output);
    });
    
    // Parse and run
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}