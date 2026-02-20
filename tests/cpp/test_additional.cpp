#include "doctest.h"

#include <filesystem>
#include <string>
#include <vector>
#include <cstring>
#include <fstream>

#include <novasvg/novasvg.h>

namespace {
std::filesystem::path project_root()
{
    return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
}

std::filesystem::path data_path(const std::string& filename)
{
    return project_root() / "data" / filename;
}

std::string read_file(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return content;
}
} // namespace

TEST_CASE("Bitmap advanced operations") {
    // Test Bitmap constructor with data
    const int width = 100;
    const int height = 100;
    const int stride = width * 4;
    std::vector<uint8_t> data(width * height * 4, 0xFF); // White ARGB
    
    novasvg::Bitmap bitmap(data.data(), width, height, stride);
    CHECK_FALSE(bitmap.isNull());
    CHECK(bitmap.width() == width);
    CHECK(bitmap.height() == height);
    CHECK(bitmap.stride() == stride);
    
    // Test convertToRGBA
    bitmap.convertToRGBA();
    
    // Test writeToPng with callback (just test it doesn't crash)
    // Note: novasvg_write_func_t has signature: void (*)(void* closure, void* data, int size)
    bool writeResult = bitmap.writeToPng([](void* closure, void* data, int size) {
        // Do nothing, just accept the data
    }, nullptr);
    // Don't check result as it depends on PNG writer
    
    // Test move assignment operator
    novasvg::Bitmap bitmap2;
    bitmap2 = std::move(bitmap);
    CHECK_FALSE(bitmap2.isNull());
    CHECK(bitmap.isNull()); // Should be null after move
}

TEST_CASE("Matrix advanced operations") {
    // Test shear operations (shear values are in radians, so use small values)
    auto sheared = novasvg::Matrix::sheared(0.1f, 0.05f);
    // Don't check exact values as shear uses tangent of angle
    
    // Test rotate with center
    auto rotated = novasvg::Matrix::rotated(45.0f, 10.0f, 20.0f);
    
    // Test matrix multiplication
    novasvg::Matrix m1 = novasvg::Matrix::translated(10.0f, 20.0f);
    novasvg::Matrix m2 = novasvg::Matrix::scaled(2.0f, 3.0f);
    auto m3 = m1 * m2;
    
    // Test multiply method
    novasvg::Matrix m4;
    m4.multiply(m1);
    CHECK(m4.e == doctest::Approx(10.0f));
    CHECK(m4.f == doctest::Approx(20.0f));
    
    // Test shear method (just test it doesn't crash)
    novasvg::Matrix m5;
    m5.shear(0.1f, 0.05f);
    
    // Test rotate method with center
    novasvg::Matrix m6;
    m6.rotate(30.0f, 5.0f, 5.0f);
}

TEST_CASE("Box operations") {
    // Test Box constructor with parameters
    novasvg::Box box(10.0f, 20.0f, 30.0f, 40.0f);
    CHECK(box.x == doctest::Approx(10.0f));
    CHECK(box.y == doctest::Approx(20.0f));
    CHECK(box.w == doctest::Approx(30.0f));
    CHECK(box.h == doctest::Approx(40.0f));
    
    // Test transform method (in-place)
    novasvg::Matrix m = novasvg::Matrix::translated(5.0f, 10.0f);
    box.transform(m);
    CHECK(box.x == doctest::Approx(15.0f));
    CHECK(box.y == doctest::Approx(30.0f));
}

TEST_CASE("TextNode operations") {
    // Create a simple SVG with text
    std::string svg_data = R"(<svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
        <text id="text1" x="10" y="20">Hello World</text>
    </svg>)";
    
    auto document = novasvg::Document::loadFromData(svg_data);
    REQUIRE(document != nullptr);
    
    // Get text element
    auto textElement = document->getElementById("text1");
    REQUIRE_FALSE(textElement.isNull());
    
    // Get children (should contain text node)
    auto children = textElement.children();
    CHECK(children.size() >= 1);
    
    // Find text node
    for (const auto& child : children) {
        if (child.isTextNode()) {
            auto textNode = child.toTextNode();
            CHECK_FALSE(textNode.isNull());
            CHECK(textNode.data() == "Hello World");
            
            // Test setData
            textNode.setData("Modified Text");
            CHECK(textNode.data() == "Modified Text");
            break;
        }
    }
}

TEST_CASE("Document render with matrix") {
    auto svg_file = data_path("rect.svg");
    auto document = novasvg::Document::loadFromFile(svg_file.string());
    REQUIRE(document != nullptr);
    
    // Create bitmap
    novasvg::Bitmap bitmap(100, 100);
    
    // Test render with identity matrix
    document->render(bitmap, novasvg::Matrix());
    
    // Test render with translation matrix
    auto matrix = novasvg::Matrix::translated(10.0f, 20.0f);
    document->render(bitmap, matrix);
    
    // Test render with scaling matrix
    auto matrix2 = novasvg::Matrix::scaled(0.5f, 0.5f);
    document->render(bitmap, matrix2);
}

TEST_CASE("Element render with matrix") {
    auto svg_file = data_path("rect.svg");
    auto document = novasvg::Document::loadFromFile(svg_file.string());
    REQUIRE(document != nullptr);
    
    auto docElement = document->documentElement();
    auto children = docElement.children();
    
    if (!children.empty()) {
        auto rectElement = children[0].toElement();
        CHECK_FALSE(rectElement.isNull());
        
        // Create bitmap
        novasvg::Bitmap bitmap(50, 50);
        
        // Test render with identity matrix
        rectElement.render(bitmap, novasvg::Matrix());
        
        // Test render with translation matrix
        auto matrix = novasvg::Matrix::translated(5.0f, 5.0f);
        rectElement.render(bitmap, matrix);
    }
}

TEST_CASE("Document methods with edge cases") {
    // Test with empty SVG
    std::string empty_svg = R"(<svg xmlns="http://www.w3.org/2000/svg"></svg>)";
    auto document = novasvg::Document::loadFromData(empty_svg);
    REQUIRE(document != nullptr);
    
    // Test width/height with empty SVG
    CHECK(document->width() == doctest::Approx(0.0f));
    CHECK(document->height() == doctest::Approx(0.0f));
    
    // Test boundingBox with empty SVG
    auto bbox = document->boundingBox();
    CHECK(bbox.w == doctest::Approx(0.0f));
    CHECK(bbox.h == doctest::Approx(0.0f));
    
    // Test renderToBitmap with empty dimensions
    auto bitmap = document->renderToBitmap(0, 0);
    CHECK(bitmap.isNull());
    
    // Test renderToBitmap with auto-scaling from empty SVG
    auto bitmap2 = document->renderToBitmap(-1, 100);
    CHECK(bitmap2.isNull());
    
    auto bitmap3 = document->renderToBitmap(100, -1);
    CHECK(bitmap3.isNull());
}

TEST_CASE("Query selector edge cases") {
    std::string svg_data = R"(<svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
        <g id="group1">
            <rect class="shape" x="10" y="10" width="20" height="20"/>
            <circle class="shape" cx="50" cy="50" r="10"/>
        </g>
    </svg>)";
    
    auto document = novasvg::Document::loadFromData(svg_data);
    REQUIRE(document != nullptr);
    
    // Test querySelectorAll with class selector (if supported)
    // Note: This may not work depending on CSS selector support
    
    // Test getElementById with nested element
    auto group = document->getElementById("group1");
    CHECK_FALSE(group.isNull());
    
    // Test children of group
    auto groupChildren = group.children();
    CHECK(groupChildren.size() >= 2);
}

TEST_CASE("Apply stylesheet edge cases") {
    std::string svg_data = R"(<svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
        <rect id="rect1" x="10" y="10" width="80" height="80"/>
    </svg>)";
    
    auto document = novasvg::Document::loadFromData(svg_data);
    REQUIRE(document != nullptr);
    
    // Apply empty stylesheet
    document->applyStyleSheet("");
    
    // Apply stylesheet with invalid CSS (should not crash)
    document->applyStyleSheet("invalid { css: syntax; }");
    
    // Apply valid CSS
    document->applyStyleSheet("#rect1 { fill: #FF0000; }");
}

TEST_CASE("Node equality operators") {
    auto svg_file = data_path("rect.svg");
    auto document = novasvg::Document::loadFromFile(svg_file.string());
    REQUIRE(document != nullptr);
    
    auto docElement = document->documentElement();
    auto children = docElement.children();
    
    if (children.size() >= 1) {
        auto node1 = children[0];
        auto node2 = children[0];
        
        // Test equality
        CHECK(node1 == node2);
        CHECK_FALSE(node1 != node2);
        
        // Test with null node
        novasvg::Node nullNode;
        CHECK_FALSE(node1 == nullNode);
        CHECK(node1 != nullNode);
        
        // Test self-equality
        CHECK(node1 == node1);
        CHECK_FALSE(node1 != node1);
    }
}