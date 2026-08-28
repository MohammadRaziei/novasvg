/**
 * NovaSVG Command Line Interface
 * 
 * A comprehensive CLI tool for working with SVG files using the NovaSVG library.
 * 
 * Features:
 * - Convert SVG to PNG
 * - Query SVG information (size, bounding box, file size)
 * - Extract elements using CSS selectors
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

// ============================================================
// Utilities
// ============================================================

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

// ============================================================
// Command Implementations
// ============================================================

int cmd_convert(const std::string& input, 
                const std::string& output, 
                int width, 
                int height, 
                const std::string& bg_color_str,
                float scale,
                const std::string& style_content,
                const std::string& css_file) {
    
    // Load SVG
    std::unique_ptr<novasvg::Document> doc = novasvg::Document::loadFromFile(input);
    if (!doc) {
        std::cerr << "Error: Failed to load SVG file: " << input << "\n";
        return 1;
    }

    // Handle CSS
    std::string final_css;
    if (!css_file.empty()) {
        std::ifstream f(css_file);
        if (!f) {
            std::cerr << "Error: Failed to open CSS file: " << css_file << "\n";
            return 1;
        }
        final_css.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    }
    if (!style_content.empty()) {
        if (!final_css.empty()) final_css += "\n";
        final_css += style_content;
    }

    if (!final_css.empty()) {
        doc->applyStyleSheet(final_css);
        doc->forceLayout();
    }

    // Determine Output Path
    fs::path output_path = output;
    if (output.empty()) {
        output_path = fs::path(input);
        output_path.replace_extension(".png");
    }

    // Handle Background Color
    uint32_t bg_color = 0x00000000; // Default transparent
    if (!bg_color_str.empty()) {
        std::string color_str = bg_color_str;
        // Remove leading '#' if present
        if (color_str[0] == '#') {
            color_str = color_str.substr(1);
        }
        try {
            bg_color = std::stoul(color_str, nullptr, 16);
        } catch (...) {
            std::cerr << "Error: Invalid background color format. Use RRGGBB or RRGGBBAA.\n";
            return 1;
        }
    }

    std::cout << "Converting: " << input << " -> " << output_path.string() << "\n";
    std::cout << "Original size: " << doc->width() << "x" << doc->height() << "px\n";
    
    // Calculate Dimensions
    if (scale > 0.0f && scale != 1.0f) {
        width = static_cast<int>(doc->width() * scale);
        height = static_cast<int>(doc->height() * scale);
    }
    
    if (width > 0 || height > 0) {
        std::cout << "Output size: ";
        if (width > 0) std::cout << width;
        if (width > 0 && height > 0) std::cout << "x";
        if (height > 0) std::cout << height;
        std::cout << "px\n";
    }
    
    // Render
    auto bitmap = doc->renderToBitmap(width, height, novasvg::Color::fromValue(bg_color));
    if (bitmap.isNull()) {
        std::cerr << "Error: Failed to render SVG\n";
        return 1;
    }
    
    if (!bitmap.writeToPng(output_path.string())) {
        std::cerr << "Error: Failed to save PNG file: " << output_path.string() << "\n";
        return 1;
    }
    
    std::cout << "Successfully converted to " << output_path.string() << "\n";
    return 0;
}

// Helper function to count all elements recursively
size_t count_all_elements(const novasvg::Element& element) {
    size_t count = 1; 
    auto children = element.children();
    for (const auto& child : children) {
        if (child.isElement()) {
            count += count_all_elements(child.toElement());
        }
    }
    return count;
}

int cmd_info(const std::string& input, bool json_output = false) {
    std::unique_ptr<novasvg::Document> doc = novasvg::Document::loadFromFile(input);
    if (!doc) {
        std::cerr << "Error: Failed to load SVG file: " << input << "\n";
        return 1;
    }
    if (json_output) {
        auto bbox = doc->boundingBox();
        size_t total_elements = 0;
        auto root = doc->documentElement();
        if (root) total_elements = count_all_elements(root);
        
        std::uintmax_t file_size = 0;
        std::string readable_size = "N/A";
        if (fs::exists(input)) {
            file_size = fs::file_size(input);
            readable_size = human_readable_size(file_size);
        }
        
        std::string escaped_file = input;
        size_t pos = 0;
        while ((pos = escaped_file.find('"', pos)) != std::string::npos) {
            escaped_file.replace(pos, 1, "\\\"");
            pos += 2;
        }
        
        std::cout << "{\"file\":\"" << escaped_file << "\",\"width\":" << doc->width() 
                  << ",\"height\":" << doc->height() << ",\"bounding_box\":{\"x\":" 
                  << bbox.x << ",\"y\":" << bbox.y << ",\"width\":" << bbox.w 
                  << ",\"height\":" << bbox.h << "},\"total_elements\":" << total_elements 
                  << ",\"file_size\":" << file_size << ",\"readable_size\":\"" 
                  << readable_size << "\"}\n";
        return 0;
    }
    
    std::cout << "SVG Information:\n";
    std::cout << "  File: " << input << "\n";
    std::cout << "  Size: " << doc->width() << "x" << doc->height() << "px\n";
    auto bbox = doc->boundingBox();
    std::cout << "  Bounding Box: " 
              << bbox.x << "," << bbox.y << " "
              << bbox.w << "x" << bbox.h << "\n";
    
    size_t total_elements = 0;
    auto root = doc->documentElement();
    if (root) {
        total_elements = count_all_elements(root);
        std::cout << "  Total elements: " << total_elements << "\n";
    }
    
    if (fs::exists(input)) {
        auto size = fs::file_size(input);
        std::cout << "  File size: " << size << " bytes\n";
        std::cout << "  Readable size: " << human_readable_size(size) << "\n";
    } else {
        std::cout << "  File size: N/A\n";
    }
    return 0;
}

int cmd_query(const std::string& selector, const std::string& input, bool json_output = false) {
    std::unique_ptr<novasvg::Document> doc = novasvg::Document::loadFromFile(input);
    if (!doc) {
        std::cerr << "Error: Failed to load SVG file: " << input << "\n";
        return 1;
    }
    auto elements = doc->querySelectorAll(selector);
    
    if (json_output) {
        std::cout << "{\"selector\":\"" << selector << "\",\"count\":" << elements.size() << ",\"elements\":[";
        for (size_t i = 0; i < elements.size(); i++) {
            const auto& elem = elements[i];
            auto bbox = elem.getBoundingBox();
            auto local_bbox = elem.getLocalBoundingBox();
            auto global_bbox = elem.getGlobalBoundingBox();
            
            if (i > 0) std::cout << ",";
            std::cout << "{\"index\":" << (i + 1) << ",\"bounding_box\":{\"x\":" 
                      << bbox.x << ",\"y\":" << bbox.y << ",\"width\":" << bbox.w 
                      << ",\"height\":" << bbox.h << "},\"local_bbox\":{\"x\":" 
                      << local_bbox.x << ",\"y\":" << local_bbox.y << ",\"width\":" 
                      << local_bbox.w << ",\"height\":" << local_bbox.h 
                      << "},\"global_bbox\":{\"x\":" << global_bbox.x << ",\"y\":" 
                      << global_bbox.y << ",\"width\":" << global_bbox.w << ",\"height\":" 
                      << global_bbox.h << "\"";
            
            std::cout << ",\"attributes\":{";
            bool first_attr = true;
            if (elem.hasAttribute("id")) {
                std::cout << "\"id\":\"" << elem.getAttribute("id") << "\"";
                first_attr = false;
            }
            if (elem.hasAttribute("class")) {
                if (!first_attr) std::cout << ",";
                std::cout << "\"class\":\"" << elem.getAttribute("class") << "\"";
                first_attr = false;
            }
            if (elem.hasAttribute("fill")) {
                if (!first_attr) std::cout << ",";
                std::cout << "\"fill\":\"" << elem.getAttribute("fill") << "\"";
                first_attr = false;
            }
            if (elem.hasAttribute("stroke")) {
                if (!first_attr) std::cout << ",";
                std::cout << "\"stroke\":\"" << elem.getAttribute("stroke") << "\"";
                first_attr = false;
            }
            std::cout << "}}";
        }
        std::cout << "]}\n";
        return 0;
    }
    
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
        
        if (elem.hasAttribute("id")) std::cout << "  ID: " << elem.getAttribute("id") << "\n";
        if (elem.hasAttribute("class")) std::cout << "  Class: " << elem.getAttribute("class") << "\n";
        if (elem.hasAttribute("fill")) std::cout << "  Fill: " << elem.getAttribute("fill") << "\n";
        if (elem.hasAttribute("stroke")) std::cout << "  Stroke: " << elem.getAttribute("stroke") << "\n";
        std::cout << "\n";
    }
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
        if (!doc) { std::cerr << "  Failed to load\n"; failed++; continue; }
        
        auto bitmap = doc->renderToBitmap();
        if (bitmap.isNull()) { std::cerr << "  Failed to render\n"; failed++; continue; }
        
        if (!bitmap.writeToPng(output_file)) { std::cerr << "  Failed to save\n"; failed++; continue; }
        
        success++;
    }
    
    std::cout << "\nBatch processing complete:\n";
    std::cout << "  Success: " << success << "\n";
    std::cout << "  Failed:  " << failed << "\n";
    std::cout << "  Output:  " << output_dir << "\n";
    
    return failed > 0 ? 1 : 0;
}

// ============================================================
// Main
// ============================================================

int main(int argc, char** argv) {
    const std::string version_str = novasvg::versionString();

    CLI::App app{"novasvg-cli Command Line Interface v" + version_str + " - High-performance SVG rendering toolkit."};   
    app.name("novasvg-cli");
    app.set_version_flag("-v,--version", version_str, "Show version information");

    // --------------------------------------------------------
    // Convert Command
    // --------------------------------------------------------
    auto convert_cmd = app.add_subcommand("convert", "Convert SVG files to PNG with optional styling.");
    std::string convert_input, convert_output, convert_bg_color, convert_style, convert_css_file;
    int convert_width = -1, convert_height = -1;
    float convert_scale = 0.0f;

    convert_cmd->add_option("input", convert_input, "Input SVG file")->required()->check(CLI::ExistingFile);
    convert_cmd->add_option("-o,--output", convert_output, "Output PNG file (default: input name with .png extension)");
    convert_cmd->add_option("-w,--width", convert_width, "Output width in pixels");
    convert_cmd->add_option("-H,--height", convert_height, "Output height in pixels");
    convert_cmd->add_option("-s,--scale", convert_scale, "Scale factor (e.g., 2.0)");
    convert_cmd->add_option("-b,--background-color", convert_bg_color, "Background color (hex: RRGGBB or RRGGBBAA, default: transparent)");
    convert_cmd->add_option("--style", convert_style, "Apply CSS styles directly (string)");
    convert_cmd->add_option("--css-file", convert_css_file, "Apply CSS styles from file")->check(CLI::ExistingFile);

    // Add specific help for convert command
    convert_cmd->footer(
        "Examples:\n"
        "  novasvg-cli convert input.svg\n"
        "  novasvg-cli convert input.svg -o out.png\n"
        "  novasvg-cli convert input.svg -w 800 -H 600\n"
        "  novasvg-cli convert input.svg --css-file style.css\n"
        "  novasvg-cli convert input.svg --css \"rect { fill: red; }\"\n"
    );

    convert_cmd->callback([&]() {
        return cmd_convert(convert_input, convert_output, convert_width, convert_height, 
                           convert_bg_color, convert_scale, convert_style, convert_css_file);
    });

    // --------------------------------------------------------
    // Info Command
    // --------------------------------------------------------
    auto info_cmd = app.add_subcommand("info", "Display detailed SVG file information.");
    std::string info_input;
    bool info_json = false;

    info_cmd->add_option("input", info_input, "Input SVG file")->required()->check(CLI::ExistingFile);
    info_cmd->add_flag("--json", info_json, "Output in JSON format");
    
    info_cmd->footer(
        "Examples:\n"
        "  novasvg-cli info image.svg\n"
        "  novasvg-cli info image.svg --json\n"
    );

    info_cmd->callback([&]() {
        return cmd_info(info_input, info_json);
    });

    // --------------------------------------------------------
    // Query Command
    // --------------------------------------------------------
    auto query_cmd = app.add_subcommand("query", "Query SVG elements using CSS selectors.");
    std::string query_selector, query_input;
    bool query_json = false;

    query_cmd->add_option("selector", query_selector, "CSS selector (e.g., 'circle', '#myId')")->required();
    query_cmd->add_option("input", query_input, "Input SVG file")->required()->check(CLI::ExistingFile);
    query_cmd->add_flag("--json", query_json, "Output in JSON format");

    query_cmd->footer(
        "Examples:\n"
        "  novasvg-cli query \"circle\" input.svg\n"
        "  novasvg-cli query \"rect[fill='red']\" input.svg\n"
    );

    query_cmd->callback([&]() {
        return cmd_query(query_selector, query_input, query_json);
    });

    // --------------------------------------------------------
    // Batch Command
    // --------------------------------------------------------
    auto batch_cmd = app.add_subcommand("batch", "Batch convert all SVGs in a directory.");
    std::string batch_input, batch_output = "output";

    batch_cmd->add_option("input", batch_input, "Input directory")->required()->check(CLI::ExistingDirectory);
    batch_cmd->add_option("output", batch_output, "Output directory (default: ./output)");

    batch_cmd->footer(
        "Examples:\n"
        "  novasvg-cli batch ./svgs ./images\n"
    );

    batch_cmd->callback([&]() {
        return cmd_batch(batch_input, batch_output);
    });

    // --------------------------------------------------------
    // Parse
    // --------------------------------------------------------
    try {
        if (argc < 2) {
            std::cout << app.help() << std::endl;
            return 0;
        }
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}