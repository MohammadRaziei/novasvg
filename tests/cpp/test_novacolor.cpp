#define DOCTEST_CONFIG_SUPER_FAST_ASSERTS
#include "doctest.h"
#include <sstream>
#include "nova/vg/color.h"

using namespace nova::vg;
using namespace stb::color_literals;

TEST_CASE("Color - Default Constructor") {
    stb::color c;
    CHECK(c.r() == 0);
    CHECK(c.g() == 0);
    CHECK(c.b() == 0);
    CHECK(c.a() == 255);
}

TEST_CASE("Color - RGBA Constructor") {
    stb::color c(255, 0, 0, 255);
    CHECK(c.r() == 255);
    CHECK(c.g() == 0);
    CHECK(c.b() == 0);
    CHECK(c.a() == 255);

    stb::color c2(0, 255, 0, 128);
    CHECK(c2.r() == 0);
    CHECK(c2.g() == 255);
    CHECK(c2.b() == 0);
    CHECK(c2.a() == 128);
}

TEST_CASE("Color - Setters") {
    stb::color c;
    c.set_r(100);
    c.set_g(150);
    c.set_b(200);
    c.set_a(50);

    CHECK(c.r() == 100);
    CHECK(c.g() == 150);
    CHECK(c.b() == 200);
    CHECK(c.a() == 50);
}

TEST_CASE("Color - value()") {
    stb::color c(255, 0, 0, 255);
    CHECK(c.value() == 0xFF0000FF);

    stb::color c2(0, 255, 0, 128);
    CHECK(c2.value() == 0x00FF0080);

    stb::color c3(0, 0, 0, 0);
    CHECK(c3.value() == 0x00000000);

    stb::color c4(255, 255, 255, 255);
    CHECK(c4.value() == 0xFFFFFFFF);
}

TEST_CASE("Color - toString()") {
    stb::color c(255, 0, 0, 255);
    CHECK(c.to_string() == "color(r=255, g=0, b=0, a=255)");
}

TEST_CASE("Color - Implicit String Conversion") {
    stb::color c(100, 150, 200, 50);
    std::string s = c;
    CHECK(s == "color(r=100, g=150, b=200, a=50)");
}

TEST_CASE("Color - from_hash()") {
    SUBCASE("Full hex #RRGGBB") {
        stb::color c = stb::color::from_hash("#FF0000");
        CHECK(c.r() == 255);
        CHECK(c.g() == 0);
        CHECK(c.b() == 0);
        CHECK(c.a() == 255);
    }

    SUBCASE("Shorthand hex #RGB") {
        stb::color c = stb::color::from_hash("#F00");
        CHECK(c.r() == 255);
        CHECK(c.g() == 0);
        CHECK(c.b() == 0);
        CHECK(c.a() == 255);
    }

    SUBCASE("Full hex with alpha #RRGGBBAA") {
        stb::color c = stb::color::from_hash("#FF000080");
        CHECK(c.r() == 255);
        CHECK(c.g() == 0);
        CHECK(c.b() == 0);
        CHECK(c.a() == 128);
    }

    SUBCASE("Shorthand hex with alpha #RGBA") {
        stb::color c = stb::color::from_hash("#F008");
        CHECK(c.r() == 255);
        CHECK(c.g() == 0);
        CHECK(c.b() == 0);
        CHECK(c.a() == 136);  // 0x08 * 0x11 = 136
    }

    SUBCASE("Invalid hex throws") {
        CHECK_THROWS_AS(stb::color::from_hash("FF0000"), std::invalid_argument);
        CHECK_THROWS_AS(stb::color::from_hash("#GG0000"), std::invalid_argument);
        CHECK_THROWS_AS(stb::color::from_hash("#FF000"), std::invalid_argument);
        CHECK_THROWS_AS(stb::color::from_hash("#12345g"), std::invalid_argument);  // used to parse as valid
        CHECK_THROWS_AS(stb::color::from_hash("#12345g"), stb::invalid_color_error);
        CHECK_THROWS_WITH_AS(stb::color::from_hash("#gggggg"),
                             "Invalid hex color '#gggggg': 'g' is not a hex digit (0-9, a-f, A-F)",
                             std::invalid_argument);
    }
}

TEST_CASE("Color - from_name()") {
    CHECK(stb::color::from_name("red") == stb::color::red());
    CHECK(stb::color::from_name("blue") == stb::color::blue());
    CHECK(stb::color::from_name("green") == stb::color::green());
    CHECK(stb::color::from_name("RED") == stb::color::red());      // Case insensitive
    CHECK(stb::color::from_name("Blue") == stb::color::blue());   // Case insensitive
    CHECK(stb::color::from_name("grey") == stb::color::gray());    // British spelling

    CHECK_THROWS_AS(stb::color::from_name("invalid_color"), std::invalid_argument);
    CHECK_THROWS_AS(stb::color::from_name(""), std::invalid_argument);
}

TEST_CASE("Color - from_string()") {
    CHECK(stb::color::from_string("#FF0000") == stb::color::red());
    CHECK(stb::color::from_string("blue") == stb::color::blue());
    CHECK(stb::color::from_string("#F00") == stb::color::red());

    CHECK_THROWS_AS(stb::color::from_string(""), std::invalid_argument);
}

TEST_CASE("Color - from_value()") {
    stb::color c = stb::color::from_value(0xFF0000FF);
    CHECK(c.r() == 255);
    CHECK(c.g() == 0);
    CHECK(c.b() == 0);
    CHECK(c.a() == 255);

    stb::color c2 = stb::color::from_value(0x00000000);
    CHECK(c2.r() == 0);
    CHECK(c2.g() == 0);
    CHECK(c2.b() == 0);
    CHECK(c2.a() == 0);

    stb::color c3 = stb::color::from_value(0xAABBCCDD);
    CHECK(c3.r() == 0xAA);
    CHECK(c3.g() == 0xBB);
    CHECK(c3.b() == 0xCC);
    CHECK(c3.a() == 0xDD);
}

TEST_CASE("Color - Predefined Colors") {
    CHECK(stb::color::black() == stb::color(0, 0, 0));
    CHECK(stb::color::white() == stb::color(255, 255, 255));
    CHECK(stb::color::red() == stb::color(255, 0, 0));
    CHECK(stb::color::green() == stb::color(0, 128, 0));
    CHECK(stb::color::lime() == stb::color(0, 255, 0));
    CHECK(stb::color::blue() == stb::color(0, 0, 255));
    CHECK(stb::color::yellow() == stb::color(255, 255, 0));
    CHECK(stb::color::cyan() == stb::color(0, 255, 255));
    CHECK(stb::color::magenta() == stb::color(255, 0, 255));
    CHECK(stb::color::silver() == stb::color(192, 192, 192));
    CHECK(stb::color::gray() == stb::color(128, 128, 128));
    CHECK(stb::color::maroon() == stb::color(128, 0, 0));
    CHECK(stb::color::olive() == stb::color(128, 128, 0));
    CHECK(stb::color::purple() == stb::color(128, 0, 128));
    CHECK(stb::color::teal() == stb::color(0, 128, 128));
    CHECK(stb::color::navy() == stb::color(0, 0, 128));
    CHECK(stb::color::orange() == stb::color(255, 165, 0));
    CHECK(stb::color::pink() == stb::color(255, 192, 203));
    CHECK(stb::color::brown() == stb::color(165, 42, 42));
    CHECK(stb::color::transparent() == stb::color(0, 0, 0, 0));
}

TEST_CASE("Color - Blend with Transparent (| int)") {
    stb::color gray = stb::color::gray();  // (128, 128, 128, 255)
    stb::color transparent = stb::color::transparent();  // (0, 0, 0, 0)

    SUBCASE("0% - Original color") {
        stb::color result = gray | 0;
        CHECK(result.r() == 128);
        CHECK(result.g() == 128);
        CHECK(result.b() == 128);
        CHECK(result.a() == 255);
    }

    SUBCASE("50% - Half transparent") {
        stb::color result = gray | 50;
        CHECK(result.r() == 64);   // 128 * 0.5
        CHECK(result.g() == 64);
        CHECK(result.b() == 64);
        CHECK(result.a() == 128);  // 255 * 0.5
    }

    SUBCASE("100% - Fully transparent") {
        stb::color result = gray | 100;
        CHECK(result.r() == 0);
        CHECK(result.g() == 0);
        CHECK(result.b() == 0);
        CHECK(result.a() == 0);
    }

    SUBCASE("Clamping - negative becomes 0") {
        stb::color result = gray | -50;
        CHECK(result == gray);
    }

    SUBCASE("Clamping - over 100 becomes 100") {
        stb::color result = gray | 150;
        CHECK(result == transparent);
    }
}

TEST_CASE("Color - Blend with Another Color (| Color)") {
    stb::color red = stb::color::red();    // (255, 0, 0)
    stb::color blue = stb::color::blue();   // (0, 0, 255)

    stb::color result = red | blue;  // 50% each

    CHECK(result.r() == 128);  // 255 * 0.5 + 0 * 0.5
    CHECK(result.g() == 0);
    CHECK(result.b() == 128);  // 0 * 0.5 + 255 * 0.5
    CHECK(result.a() == 255);
}

TEST_CASE("Color - blend() method") {
    stb::color red = stb::color::red();
    stb::color blue = stb::color::blue();

    SUBCASE("0% - All red") {
        stb::color result = red.blend(blue, 0);
        CHECK(result == red);
    }

    SUBCASE("25% - Mostly red") {
        stb::color result = red.blend(blue, 25);
        CHECK(result.r() == 192);  // 255 * 0.75 + 0 * 0.25
        CHECK(result.b() == 64);   // 0 * 0.75 + 255 * 0.25
    }

    SUBCASE("100% - All blue") {
        stb::color result = red.blend(blue, 100);
        CHECK(result == blue);
    }
}

TEST_CASE("ColorBlend - Chaining") {
    SUBCASE("gray | 50 | black") {
        // 50% gray + 50% black
        stb::color result = "gray"_c | 50 | "black"_c;
        
        // gray = (128, 128, 128), black = (0, 0, 0)
        CHECK(result.r() == 64);
        CHECK(result.g() == 64);
        CHECK(result.b() == 64);
    }

    SUBCASE("red | 50 | blue") {
        // red = (255, 0, 0), blue = (0, 0, 255)
        stb::color result = "red"_c | 50 | "blue"_c;
        
        CHECK(result.r() == 128);
        CHECK(result.g() == 0);
        CHECK(result.b() == 128);
    }

    SUBCASE("red | 30 | blue") {
        // red = (255, 0, 0), blue = (0, 0, 255)
        stb::color result = "red"_c | 30 | "blue"_c;
        
        CHECK(result.r() == 179);
        CHECK(result.g() == 0);
        CHECK(result.b() == 77);
    }
}

TEST_CASE("Color - String Constructor") {
    stb::color c1("#FF0000");
    CHECK(c1 == stb::color::red());

    stb::color c2("blue");
    CHECK(c2 == stb::color::blue());

    stb::color c3("#F00");
    CHECK(c3 == stb::color::red());

    CHECK_THROWS_AS(stb::color("invalid"), std::invalid_argument);
}

TEST_CASE("Color - Literal Operator") {
    CHECK("red"_c == stb::color::red());
    CHECK("blue"_c == stb::color::blue());
    CHECK("#FF0000"_c == stb::color::red());
    CHECK("#F00"_c == stb::color::red());
    CHECK("Gray"_c == stb::color::gray());  // Case insensitive
}

TEST_CASE("Color - Stream Output") {
    std::ostringstream oss;
    oss << stb::color::red();
    CHECK(oss.str() == "color(r=255, g=0, b=0, a=255)");

    oss.str("");
    oss << stb::color_blend(stb::color::blue(), 50);
    CHECK(oss.str() == "color(r=0, g=0, b=128, a=128)");
}

TEST_CASE("Color - rgba() Factory") {
    stb::color c = stb::color::rgba(100, 150, 200, 50);
    CHECK(c.r() == 100);
    CHECK(c.g() == 150);
    CHECK(c.b() == 200);
    CHECK(c.a() == 50);

    stb::color c2 = stb::color::rgba(255, 0, 0);
    CHECK(c2.a() == 255);  // Default alpha
}

TEST_CASE("Color - Roundtrip") {
    SUBCASE("value() -> from_value()") {
        stb::color original(100, 150, 200, 50);
        uint32_t val = original.value();
        stb::color restored = stb::color::from_value(val);
        
        CHECK(restored.r() == original.r());
        CHECK(restored.g() == original.g());
        CHECK(restored.b() == original.b());
        CHECK(restored.a() == original.a());
    }

    SUBCASE("toString() -> constructor") {
        stb::color original(100, 150, 200, 50);
        std::string s = original.to_string();
        CHECK(s.find("100") != std::string::npos);
        CHECK(s.find("150") != std::string::npos);
        CHECK(s.find("200") != std::string::npos);
        CHECK(s.find("50") != std::string::npos);
    }
}