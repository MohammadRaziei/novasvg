#define DOCTEST_CONFIG_SUPER_FAST_ASSERTS
#include "doctest.h"
#include "novasvg/detail/novacolor.h"

using namespace novasvg;
using namespace novasvg::color_literals;

TEST_CASE("Color - Default Constructor") {
    Color c;
    CHECK(c.getR() == 0);
    CHECK(c.getG() == 0);
    CHECK(c.getB() == 0);
    CHECK(c.getA() == 255);
}

TEST_CASE("Color - RGBA Constructor") {
    Color c(255, 0, 0, 255);
    CHECK(c.getR() == 255);
    CHECK(c.getG() == 0);
    CHECK(c.getB() == 0);
    CHECK(c.getA() == 255);

    Color c2(0, 255, 0, 128);
    CHECK(c2.getR() == 0);
    CHECK(c2.getG() == 255);
    CHECK(c2.getB() == 0);
    CHECK(c2.getA() == 128);
}

TEST_CASE("Color - Setters") {
    Color c;
    c.setR(100);
    c.setG(150);
    c.setB(200);
    c.setA(50);

    CHECK(c.getR() == 100);
    CHECK(c.getG() == 150);
    CHECK(c.getB() == 200);
    CHECK(c.getA() == 50);
}

TEST_CASE("Color - value()") {
    Color c(255, 0, 0, 255);
    CHECK(c.value() == 0xFF0000FF);

    Color c2(0, 255, 0, 128);
    CHECK(c2.value() == 0x00FF0080);

    Color c3(0, 0, 0, 0);
    CHECK(c3.value() == 0x00000000);

    Color c4(255, 255, 255, 255);
    CHECK(c4.value() == 0xFFFFFFFF);
}

TEST_CASE("Color - toString()") {
    Color c(255, 0, 0, 255);
    CHECK(c.toString() == "Color(r=255, g=0, b=0, a=255)");
}

TEST_CASE("Color - Implicit String Conversion") {
    Color c(100, 150, 200, 50);
    std::string s = c;
    CHECK(s == "Color(r=100, g=150, b=200, a=50)");
}

TEST_CASE("Color - fromHash()") {
    SUBCASE("Full hex #RRGGBB") {
        Color c = Color::fromHash("#FF0000");
        CHECK(c.getR() == 255);
        CHECK(c.getG() == 0);
        CHECK(c.getB() == 0);
        CHECK(c.getA() == 255);
    }

    SUBCASE("Shorthand hex #RGB") {
        Color c = Color::fromHash("#F00");
        CHECK(c.getR() == 255);
        CHECK(c.getG() == 0);
        CHECK(c.getB() == 0);
        CHECK(c.getA() == 255);
    }

    SUBCASE("Full hex with alpha #RRGGBBAA") {
        Color c = Color::fromHash("#FF000080");
        CHECK(c.getR() == 255);
        CHECK(c.getG() == 0);
        CHECK(c.getB() == 0);
        CHECK(c.getA() == 128);
    }

    SUBCASE("Shorthand hex with alpha #RGBA") {
        Color c = Color::fromHash("#F008");
        CHECK(c.getR() == 255);
        CHECK(c.getG() == 0);
        CHECK(c.getB() == 0);
        CHECK(c.getA() == 136);  // 0x08 * 0x11 = 136
    }

    SUBCASE("Invalid hex throws") {
        CHECK_THROWS_AS(Color::fromHash("FF0000"), std::invalid_argument);
        CHECK_THROWS_AS(Color::fromHash("#GG0000"), std::invalid_argument);
        CHECK_THROWS_AS(Color::fromHash("#FF000"), std::invalid_argument);
    }
}

TEST_CASE("Color - fromName()") {
    CHECK(Color::fromName("red") == Color::red());
    CHECK(Color::fromName("blue") == Color::blue());
    CHECK(Color::fromName("green") == Color::green());
    CHECK(Color::fromName("RED") == Color::red());      // Case insensitive
    CHECK(Color::fromName("Blue") == Color::blue());   // Case insensitive
    CHECK(Color::fromName("grey") == Color::gray());    // British spelling

    CHECK_THROWS_AS(Color::fromName("invalid_color"), std::invalid_argument);
    CHECK_THROWS_AS(Color::fromName(""), std::invalid_argument);
}

TEST_CASE("Color - fromString()") {
    CHECK(Color::fromString("#FF0000") == Color::red());
    CHECK(Color::fromString("blue") == Color::blue());
    CHECK(Color::fromString("#F00") == Color::red());

    CHECK_THROWS_AS(Color::fromString(""), std::invalid_argument);
}

TEST_CASE("Color - fromValue()") {
    Color c = Color::fromValue(0xFF0000FF);
    CHECK(c.getR() == 255);
    CHECK(c.getG() == 0);
    CHECK(c.getB() == 0);
    CHECK(c.getA() == 255);

    Color c2 = Color::fromValue(0x00000000);
    CHECK(c2.getR() == 0);
    CHECK(c2.getG() == 0);
    CHECK(c2.getB() == 0);
    CHECK(c2.getA() == 0);

    Color c3 = Color::fromValue(0xAABBCCDD);
    CHECK(c3.getR() == 0xAA);
    CHECK(c3.getG() == 0xBB);
    CHECK(c3.getB() == 0xCC);
    CHECK(c3.getA() == 0xDD);
}

TEST_CASE("Color - Predefined Colors") {
    CHECK(Color::black() == Color(0, 0, 0));
    CHECK(Color::white() == Color(255, 255, 255));
    CHECK(Color::red() == Color(255, 0, 0));
    CHECK(Color::green() == Color(0, 128, 0));
    CHECK(Color::lime() == Color(0, 255, 0));
    CHECK(Color::blue() == Color(0, 0, 255));
    CHECK(Color::yellow() == Color(255, 255, 0));
    CHECK(Color::cyan() == Color(0, 255, 255));
    CHECK(Color::magenta() == Color(255, 0, 255));
    CHECK(Color::silver() == Color(192, 192, 192));
    CHECK(Color::gray() == Color(128, 128, 128));
    CHECK(Color::maroon() == Color(128, 0, 0));
    CHECK(Color::olive() == Color(128, 128, 0));
    CHECK(Color::purple() == Color(128, 0, 128));
    CHECK(Color::teal() == Color(0, 128, 128));
    CHECK(Color::navy() == Color(0, 0, 128));
    CHECK(Color::orange() == Color(255, 165, 0));
    CHECK(Color::pink() == Color(255, 192, 203));
    CHECK(Color::brown() == Color(165, 42, 42));
    CHECK(Color::transparent() == Color(0, 0, 0, 0));
}

TEST_CASE("Color - Blend with Transparent (| int)") {
    Color gray = Color::gray();  // (128, 128, 128, 255)
    Color transparent = Color::transparent();  // (0, 0, 0, 0)

    SUBCASE("0% - Original color") {
        Color result = gray | 0;
        CHECK(result.getR() == 128);
        CHECK(result.getG() == 128);
        CHECK(result.getB() == 128);
        CHECK(result.getA() == 255);
    }

    SUBCASE("50% - Half transparent") {
        Color result = gray | 50;
        CHECK(result.getR() == 64);   // 128 * 0.5
        CHECK(result.getG() == 64);
        CHECK(result.getB() == 64);
        CHECK(result.getA() == 128);  // 255 * 0.5
    }

    SUBCASE("100% - Fully transparent") {
        Color result = gray | 100;
        CHECK(result.getR() == 0);
        CHECK(result.getG() == 0);
        CHECK(result.getB() == 0);
        CHECK(result.getA() == 0);
    }

    SUBCASE("Clamping - negative becomes 0") {
        Color result = gray | -50;
        CHECK(result == gray);
    }

    SUBCASE("Clamping - over 100 becomes 100") {
        Color result = gray | 150;
        CHECK(result == transparent);
    }
}

TEST_CASE("Color - Blend with Another Color (| Color)") {
    Color red = Color::red();    // (255, 0, 0)
    Color blue = Color::blue();   // (0, 0, 255)

    Color result = red | blue;  // 50% each

    CHECK(result.getR() == 128);  // 255 * 0.5 + 0 * 0.5
    CHECK(result.getG() == 0);
    CHECK(result.getB() == 128);  // 0 * 0.5 + 255 * 0.5
    CHECK(result.getA() == 255);
}

TEST_CASE("Color - blend() method") {
    Color red = Color::red();
    Color blue = Color::blue();

    SUBCASE("0% - All red") {
        Color result = red.blend(blue, 0);
        CHECK(result == red);
    }

    SUBCASE("25% - Mostly red") {
        Color result = red.blend(blue, 25);
        CHECK(result.getR() == 192);  // 255 * 0.75 + 0 * 0.25
        CHECK(result.getB() == 64);   // 0 * 0.75 + 255 * 0.25
    }

    SUBCASE("100% - All blue") {
        Color result = red.blend(blue, 100);
        CHECK(result == blue);
    }
}

TEST_CASE("ColorBlend - Chaining") {
    SUBCASE("gray | 50 | black") {
        // 50% gray + 50% black
        Color result = "gray"_c | 50 | "black"_c;
        
        // gray = (128, 128, 128), black = (0, 0, 0)
        CHECK(result.getR() == 64);
        CHECK(result.getG() == 64);
        CHECK(result.getB() == 64);
    }

    SUBCASE("red | 50 | blue") {
        // red = (255, 0, 0), blue = (0, 0, 255)
        Color result = "red"_c | 50 | "blue"_c;
        
        CHECK(result.getR() == 128);
        CHECK(result.getG() == 0);
        CHECK(result.getB() == 128);
    }

    SUBCASE("red | 30 | blue") {
        // red = (255, 0, 0), blue = (0, 0, 255)
        Color result = "red"_c | 30 | "blue"_c;
        
        CHECK(result.getR() == 179);
        CHECK(result.getG() == 0);
        CHECK(result.getB() == 77);
    }
}

TEST_CASE("Color - String Constructor") {
    Color c1("#FF0000");
    CHECK(c1 == Color::red());

    Color c2("blue");
    CHECK(c2 == Color::blue());

    Color c3("#F00");
    CHECK(c3 == Color::red());

    CHECK_THROWS_AS(Color("invalid"), std::invalid_argument);
}

TEST_CASE("Color - Literal Operator") {
    CHECK("red"_c == Color::red());
    CHECK("blue"_c == Color::blue());
    CHECK("#FF0000"_c == Color::red());
    CHECK("#F00"_c == Color::red());
    CHECK("Gray"_c == Color::gray());  // Case insensitive
}

TEST_CASE("Color - Stream Output") {
    std::ostringstream oss;
    oss << Color::red();
    CHECK(oss.str() == "Color(r=255, g=0, b=0, a=255)");

    oss.str("");
    oss << ColorBlend(Color::blue(), 50);
    CHECK(oss.str() == "Color(r=0, g=0, b=128, a=128)");
}

TEST_CASE("Color - rgba() Factory") {
    Color c = Color::rgba(100, 150, 200, 50);
    CHECK(c.getR() == 100);
    CHECK(c.getG() == 150);
    CHECK(c.getB() == 200);
    CHECK(c.getA() == 50);

    Color c2 = Color::rgba(255, 0, 0);
    CHECK(c2.getA() == 255);  // Default alpha
}

TEST_CASE("Color - Roundtrip") {
    SUBCASE("value() -> fromValue()") {
        Color original(100, 150, 200, 50);
        uint32_t val = original.value();
        Color restored = Color::fromValue(val);
        
        CHECK(restored.getR() == original.getR());
        CHECK(restored.getG() == original.getG());
        CHECK(restored.getB() == original.getB());
        CHECK(restored.getA() == original.getA());
    }

    SUBCASE("toString() -> constructor") {
        Color original(100, 150, 200, 50);
        std::string s = original.toString();
        CHECK(s.find("100") != std::string::npos);
        CHECK(s.find("150") != std::string::npos);
        CHECK(s.find("200") != std::string::npos);
        CHECK(s.find("50") != std::string::npos);
    }
}