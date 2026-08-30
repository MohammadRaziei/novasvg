#ifndef NOVASVG_COLOR_H
#define NOVASVG_COLOR_H

#include <stdexcept>
#include <string>
#include <cstdint>
#include <math.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <ostream>
#include <utility>
#include <type_traits>

/**
 * @namespace novasvg
 * @brief Main namespace for the NovaSVG library
 */
namespace novasvg {
struct ColorBlend;

/**
 * @class Color
 * @brief Represents a color in RGBA format (0xRRGGBBAA).
 * 
 * This class provides a comprehensive color representation with support for
 * various color creation methods and manipulation operations.
 * 
 * @section ColorFeatures Features
 * - RGBA format storage (0xRRGGBBAA)
 * - Implicit conversion to std::string
 * - Stream output operator (<<)
 * - String literal operator (_c)
 * - Blend operator (|) for mixing colors
 * 
 * @section ColorUsage Usage Examples
 * @code
 * // Constructor from hex string
 * Color c1("#FF0000");
 * 
 * // Constructor from color name
 * Color c2("skyblue");
 * 
 * // Using literal operator
 * Color c3 = "red"_c;
 * 
 * // Blend with transparent (LaTeX style)
 * Color c4 = "gray"_c | 50;
 * 
 * // Blend with another color
 * Color c5 = "gray"_c | "black"_c;
 * 
 * // Chain blends
 * Color c6 = "gray"_c | 50 | "black"_c;
 * 
 * // Convert to integer
 * uint32_t val = c1.value();
 * 
 * // Stream output
 * std::cout << c1 << std::endl;
 * 
 * // Implicit string conversion
 * std::string s = c1;
 * @endcode
 * 
 * @note The alpha component defaults to 255 (opaque) when not specified.
 * 
 * @author NovaSVG Team
 * @version 1.0.0
 */
class Color {
private:
    uint8_t r;  ///< Red component (0-255)
    uint8_t g;  ///< Green component (0-255)
    uint8_t b;  ///< Blue component (0-255)
    uint8_t a;  ///< Alpha component (0-255), 0 = transparent, 255 = opaque

public:
    // ==================== Constructors ====================

    /**
     * @brief Construct a new Color object from RGBA components.
     * 
     * Creates a color with specified red, green, blue, and alpha components.
     * 
     * @param r Red component (0-255). Default is 0.
     * @param g Green component (0-255). Default is 0.
     * @param b Blue component (0-255). Default is 0.
     * @param a Alpha component (0-255). Default is 255 (fully opaque).
     * 
     * @note Values outside the 0-255 range are clamped automatically.
     * 
     * @par Example
     * @code
     * Color c1;              // Black, opaque (0, 0, 0, 255)
     * Color c2(255, 0, 0);   // Red, opaque
     * Color c3(0, 255, 0, 128); // Green, semi-transparent
     * @endcode
     */
    explicit Color(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 255)
        : r(r), g(g), b(b), a(a) {}

    /**
     * @brief Construct a new Color object from a string.
     * 
     * Automatically detects whether the string is a hex color code
     * or a color name and delegates to the appropriate parser.
     * 
     * @param str Hex color string (e.g., "#FF0000", "#F00") or 
     *            color name (e.g., "red", "skyblue")
     * 
     * @throws std::invalid_argument If the format is invalid or 
     *         the color name is not recognized.
     * 
     * @par Example
     * @code
     * Color c1("#FF0000");   // Red from hex
     * Color c2("#F00");      // Red from shorthand hex
     * Color c3("blue");      // Blue from name
     * Color c4("invalid");   // throws std::invalid_argument
     * @endcode
     * 
     * @see fromString()
     * @see fromHash()
     * @see fromName()
     */
    Color(const std::string& str) : Color(fromString(str)) {}

    // ==================== Interop with Other Color Types ====================

    /**
     * @brief Construct a Color from any "color-like" type.
     *
     * This constructor lets Color be built (implicitly) from any other
     * color type that exposes red()/green()/blue()/alpha() accessors
     * returning integral values -- for example novasvg::Color from
     * graphics.h. It exists so that novasvg::Color can be used
     * as a drop-in replacement for novasvg::Color throughout the engine.
     *
     * @warning The conversion is always done component-by-component
     *          (r, g, b, a), NEVER via value()/raw 32-bit integers.
     *          Different Color types are free to pack RGBA into a
     *          32-bit value using different byte orders -- this class
     *          uses 0xRRGGBBAA, while e.g. novasvg::Color uses
     *          0xAARRGGBB. Converting through the packed value would
     *          silently corrupt colors.
     *
     * @tparam OtherColor Any type providing red()/green()/blue()/alpha().
     *
     * @par Example
     * @code
     * novasvg::Color engine_color(255, 0, 0);     // engine's own Color
     * novasvg::Color c = engine_color;      // implicit conversion in
     * @endcode
     */
    template <typename OtherColor,
              typename = decltype(std::declval<const OtherColor&>().red()),
              typename = decltype(std::declval<const OtherColor&>().green()),
              typename = decltype(std::declval<const OtherColor&>().blue()),
              typename = decltype(std::declval<const OtherColor&>().alpha())>
    constexpr Color(const OtherColor& other)
        : r(static_cast<uint8_t>(other.red())),
          g(static_cast<uint8_t>(other.green())),
          b(static_cast<uint8_t>(other.blue())),
          a(static_cast<uint8_t>(other.alpha())) {}

    /**
     * @brief Convert this Color to any "color-like" type constructible
     *        from four (r, g, b, a) components.
     *
     * Symmetric counterpart to the templated constructor above. Lets this
     * Color be handed to APIs expecting a different Color type (e.g. the
     * engine's novasvg::Color) without ever touching value()/raw bit
     * layout. Kept explicit (a named method, not an implicit conversion
     * operator) since an unconstrained implicit `operator T()` would be
     * far too eager to match unrelated types and would also collide with
     * the existing `operator std::string()` below.
     *
     * @tparam OtherColor Target type, constructible as OtherColor(r, g, b, a).
     *
     * @par Example
     * @code
     * using namespace novasvg::color_literals;
     * novasvg::Color c = "red"_c;
     * novasvg::Color engine_color = c.as<novasvg::Color>();  // out
     * @endcode
     */
    template <typename OtherColor>
    constexpr OtherColor as() const {
        return OtherColor(r, g, b, a);
    }

    // ==================== Getters & Setters ====================

    /**
     * @brief Get the red component of the color.
     * @return Red component value (0-255).
     */
    uint8_t getR() const { return r; }
    
    /**
     * @brief Get the green component of the color.
     * @return Green component value (0-255).
     */
    uint8_t getG() const { return g; }
    
    /**
     * @brief Get the blue component of the color.
     * @return Blue component value (0-255).
     */
    uint8_t getB() const { return b; }
    
    /**
     * @brief Get the alpha component of the color.
     * @return Alpha component value (0-255), where 0 is fully transparent
     *         and 255 is fully opaque.
     */
    uint8_t getA() const { return a; }

    /**
     * @brief Set the red component of the color.
     * @param value New red component value (0-255).
     */
    void setR(uint8_t value) { r = value; }
    
    /**
     * @brief Set the green component of the color.
     * @param value New green component value (0-255).
     */
    void setG(uint8_t value) { g = value; }
    
    /**
     * @brief Set the blue component of the color.
     * @param value New blue component value (0-255).
     */
    void setB(uint8_t value) { b = value; }
    
    /**
     * @brief Set the alpha component of the color.
     * @param value New alpha component value (0-255).
     */
    void setA(uint8_t value) { a = value; }

    // ==================== Engine-Facing Additions ====================
    // The methods below are additions for the SVG rendering engine's
    // internal use (float 0..1 channel access, opacity checks, deriving
    // a new color with a different alpha) -- your original getters/
    // setters/static factories above are untouched.

    /// Red component as a 0..1 float.
    float getRF() const { return r / 255.f; }
    /// Green component as a 0..1 float.
    float getGF() const { return g / 255.f; }
    /// Blue component as a 0..1 float.
    float getBF() const { return b / 255.f; }
    /// Alpha component as a 0..1 float.
    float getAF() const { return a / 255.f; }

    /// True if fully opaque (alpha == 255).
    bool isOpaque() const { return a == 255; }
    /// True if not fully transparent (alpha > 0).
    bool isVisible() const { return a > 0; }

    /// This color with alpha forced to fully opaque (255).
    Color opaqueColor() const { return Color(r, g, b, 255); }

    /// This color with its alpha scaled by @p opacity (0..1).
    Color colorWithAlpha(float opacity) const {
        return Color(r, g, b, static_cast<uint8_t>(a * opacity));
    }

    // ==================== Value Conversion ====================

    /**
     * @brief Convert the color to a 32-bit unsigned integer.
     * 
     * Converts the RGBA color components to a single 32-bit integer
     * in the format 0xRRGGBBAA (little-endian: 0xAABBGGRR on some systems).
     * 
     * @return 32-bit integer representation of the color.
     * 
     * @par Example
     * @code
     * Color c(255, 0, 0, 255);  // Red
     * uint32_t val = c.value(); // 0xFF0000FF
     * @endcode
     * 
     * @note The format is 0xRRGGBBAA where:
     *       - RR = Red (bits 24-31)
     *       - GG = Green (bits 16-23)
     *       - BB = Blue (bits 8-15)
     *       - AA = Alpha (bits 0-7)
     */
    uint32_t value() const {
        return (static_cast<uint32_t>(r) << 24) |
               (static_cast<uint32_t>(g) << 16) |
               (static_cast<uint32_t>(b) << 8) |
               static_cast<uint32_t>(a);
    }

    /**
     * @brief Equality comparison operator.
     * 
     * Compares two colors for equality. Two colors are equal if all their
     * RGBA components are identical.
     * 
     * @param lhs Left-hand side color.
     * @param rhs Right-hand side color.
     * @return true if both colors have identical R, G, B, and A components.
     * 
     * @par Example
     * @code
     * Color c1(255, 0, 0, 255);
     * Color c2(255, 0, 0, 255);
     * Color c3(0, 0, 0, 255);
     * 
     * bool eq1 = (c1 == c2);  // true
     * bool eq2 = (c1 == c3);  // false
     * @endcode
     */
    friend bool operator==(const Color& lhs, const Color& rhs) {
        return lhs.r == rhs.r &&
            lhs.g == rhs.g &&
            lhs.b == rhs.b &&
            lhs.a == rhs.a;
    }

    /**
     * @brief Inequality comparison operator.
     * 
     * Compares two colors for inequality. Two colors are not equal if any
     * of their RGBA components differ.
     * 
     * @param lhs Left-hand side color.
     * @param rhs Right-hand side color.
     * @return true if any of the R, G, B, or A components differ.
     * 
     * @par Example
     * @code
     * Color c1(255, 0, 0, 255);
     * Color c2(255, 0, 0, 255);
     * Color c3(0, 0, 0, 255);
     * 
     * bool neq1 = (c1 != c2);  // false
     * bool neq2 = (c1 != c3);  // true
     * @endcode
     */
    friend bool operator!=(const Color& lhs, const Color& rhs) {
        return !(lhs == rhs);
    }

    // ==================== String Conversion ====================

    /**
     * @brief Implicit conversion to std::string.
     * 
     * Allows automatic conversion of Color to std::string in contexts
     * that require a string, such as function parameters or string
     * concatenation.
     * 
     * @return String representation of the color.
     * 
     * @par Example
     * @code
     * Color c(255, 0, 0);
     * std::string s = c;  // Uses implicit conversion
     * std::cout << "Color: " + c << std::endl;  // Concatenation works
     * @endcode
     */
    operator std::string() const { return toString(); }

    /**
     * @brief Convert the color to a human-readable string.
     * 
     * Returns a string representation of the color in the format:
     * "Color(r=RR, g=GG, b=BB, a=AA)"
     * 
     * @return String representation of the color.
     * 
     * @par Example
     * @code
     * Color c(255, 0, 0, 255);
     * std::string s = c.toString(); // "Color(r=255, g=0, b=0, a=255)"
     * @endcode
     */
    std::string toString() const {
        std::ostringstream oss;
        oss << "Color(r=" << static_cast<int>(r) 
            << ", g=" << static_cast<int>(g) 
            << ", b=" << static_cast<int>(b) 
            << ", a=" << static_cast<int>(a) << ")";
        return oss.str();
    }

    // ==================== Blend Operators ====================

    /**
     * @brief Blend this color with transparent by a percentage.
     * 
     * Similar to LaTeX's color!percentage syntax. Creates a new color
     * that is a blend of this color and transparent.
     * 
     * @param percentage Percentage of transparent to mix (0-100).
     *                   - 0 = original color (fully opaque)
     *                   - 100 = fully transparent
     * 
     * @return A ColorBlend object that can be further chained with other colors.
     * 
     * @par Example
     * @code
     * Color c = "gray"_c | 50;  // 50% gray + 50% transparent
     * @endcode
     * 
     * @note This returns a ColorBlend to allow chaining with another color.
     *       Use ColorBlend|Color to finalize the blend.
     * 
     * @see operator|(const Color&) for direct blending with another color
     */
    ColorBlend operator|(int percentage) const;

    /**
     * @brief Blend this color with another color (50% each).
     * 
     * Creates a new color that is an equal blend of this color and
     * the specified color.
     * 
     * @param other The color to blend with.
     * @return The blended color (50% this + 50% other).
     * 
     * @par Example
     * @code
     * Color c = "gray"_c | "black"_c;  // 50% gray + 50% black
     * @endcode
     */
    Color operator|(const Color& other) const {
        return blend(other, 50);
    }

    /**
     * @brief Blend this color with another color by a specific percentage.
     * 
     * Linear interpolation between this color and another color.
     * 
     * @param other The color to blend with.
     * @param percentage Percentage of the other color to include (0-100).
     *                   - 0 = this color only
     *                   - 100 = other color only
     * 
     * @return The blended color.
     * 
     * @par Example
     * @code
     * Color c = Color::red().blend(Color::blue(), 25);  // 75% red + 25% blue
     * @endcode
     */
    Color blend(const Color& other, int percentage) const {
        int pct = std::max(0, std::min(100, percentage));
        double p = pct / 100.0;
        double q = 1.0 - p;

        return Color(
            static_cast<uint8_t>(std::ceil(r * q + other.r * p)),
            static_cast<uint8_t>(std::ceil(g * q + other.g * p)),
            static_cast<uint8_t>(std::ceil(b * q + other.b * p)),
            static_cast<uint8_t>(std::ceil(a * q + other.a * p))
        );
    }

    // ==================== Static Factory Methods ====================

    /**
     * @brief Create a color from a string (auto-detects hex or name).
     * 
     * This method automatically detects whether the input is a hex color
     * code or a color name and delegates to the appropriate parser.
     * 
     * @param str Hex color string or color name.
     * 
     * @return A Color instance.
     * 
     * @throws std::invalid_argument If the format is invalid or 
     *         the color name is not recognized.
     * 
     * @par Supported Hex Formats
     * - #RRGGBB   (e.g., "#FF0000" = red)
     * - #RGB      (e.g., "#F00" = red)
     * - #RRGGBBAA (e.g., "#FF000080" = red with 50% alpha)
     * - #RGBA     (e.g., "#F008" = red with 50% alpha)
     * 
     * @par Supported Color Names
     * All standard CSS color names are supported:
     * black, white, red, green, lime, blue, yellow, cyan, magenta,
     * silver, gray, maroon, olive, purple, teal, navy, orange, pink,
     * brown, transparent
     * 
     * @par Example
     * @code
     * Color c1 = Color::fromString("#FF0000");  // Red from hex
     * Color c2 = Color::fromString("blue");    // Blue from name
     * @endcode
     * 
     * @see fromHash()
     * @see fromName()
     */
    static Color fromString(const std::string& str) {
        if (str.empty()) {
            throw std::invalid_argument("Empty string provided");
        }
        bool is_hex = std::all_of(str.begin(), str.end(), [](unsigned char c) { return std::isxdigit(c); });
        return (str[0] == '#') ? fromHash(str) : ( is_hex ? fromHash("#" + str) : fromName(str));
    }

    /**
     * @brief Create a color from RGBA components.
     * 
     * Convenience factory method for creating colors from individual
     * RGBA components.
     * 
     * @param r Red component (0-255).
     * @param g Green component (0-255).
     * @param b Blue component (0-255).
     * @param a Alpha component (0-255). Default is 255 (opaque).
     * 
     * @return A Color instance.
     * 
     * @par Example
     * @code
     * Color c = Color::rgba(255, 0, 0, 255);  // Red, opaque
     * @endcode
     */
    static Color rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        return Color(r, g, b, a);
    }

    /**
     * @brief Create a color from a hexadecimal string.
     * 
     * Parses a hex color string in various formats.
     * 
     * @param hex_code Hexadecimal string representing the color.
     * 
     * @return A Color instance.
     * 
     * @throws std::invalid_argument If the hex string format is invalid.
     * 
     * @par Supported Formats
     * | Format    | Example    | Description                          |
     * |-----------|------------|--------------------------------------|
     * | #RRGGBB   | #FF0000    | Full hex (red)                      |
     * | #RGB      | #F00       | Shorthand (red)                     |
     * | #RRGGBBAA | #FF000080  | Full hex with alpha (50% red)       |
     * | #RGBA     | #F008      | Shorthand with alpha (50% red)       |
     * 
     * @par Example
     * @code
     * Color c1 = Color::fromHash("#FF0000");   // Red
     * Color c2 = Color::fromHash("#F00");      // Red (shorthand)
     * Color c3 = Color::fromHash("#FF000080"); // 50% alpha red
     * @endcode
     */
    static Color fromHash(const std::string& hex_code) {
        if (hex_code.empty() || hex_code[0] != '#') {
            throw std::invalid_argument("Hex color string must start with '#'");
        }

        std::string hex_body = hex_code.substr(1);
        
        if (hex_body.length() != 3 && hex_body.length() != 4 &&
            hex_body.length() != 6 && hex_body.length() != 8) {
            throw std::invalid_argument("Invalid hex color format");
        }

        // Expand shorthand: #RGB -> #RRGGBB
        if (hex_body.length() == 3) {
            hex_body = std::string() + hex_body[0] + hex_body[0] 
                              + hex_body[1] + hex_body[1] 
                              + hex_body[2] + hex_body[2];
        }
        // Expand shorthand with alpha: #RGBA -> #RRGGBBAA
        else if (hex_body.length() == 4) {
            hex_body = std::string() + hex_body[0] + hex_body[0] 
                              + hex_body[1] + hex_body[1] 
                              + hex_body[2] + hex_body[2]
                              + hex_body[3] + hex_body[3];
        }

        uint8_t r = static_cast<uint8_t>(std::stoi(hex_body.substr(0, 2), nullptr, 16));
        uint8_t g = static_cast<uint8_t>(std::stoi(hex_body.substr(2, 2), nullptr, 16));
        uint8_t b = static_cast<uint8_t>(std::stoi(hex_body.substr(4, 2), nullptr, 16));
        uint8_t a = (hex_body.length() == 8) 
            ? static_cast<uint8_t>(std::stoi(hex_body.substr(6, 2), nullptr, 16))
            : 255;

        return Color(r, g, b, a);
    }

    /**
     * @brief Create a color from a standard CSS color name.
     * 
     * Looks up a color by its name and returns the corresponding color.
     * 
     * @param name The name of the color (case-insensitive).
     * 
     * @return A Color instance.
     * 
     * @throws std::invalid_argument If the color name is not recognized.
     * 
     * @par Supported Colors
     * - Basic: black, white, red, green, blue, yellow, cyan, magenta
     * - Extended: silver, gray, maroon, olive, purple, teal, navy
     * - Additional: lime, orange, pink, brown, transparent
     * 
     * @note Both "gray" and "grey" spellings are supported.
     * 
     * @par Example
     * @code
     * Color c1 = Color::fromName("red");
     * Color c2 = Color::fromName("SkyBlue");  // Case insensitive
     * Color c3 = Color::fromName("grey");     // British spelling
     * @endcode
     */
    static Color fromName(const std::string& name) {
        std::string name_lower = name;
        for (char& c : name_lower) c = static_cast<char>(std::tolower(c));

        if (name_lower == "black")       return black();
        if (name_lower == "white")       return white();
        if (name_lower == "red")         return red();
        if (name_lower == "green")       return green();
        if (name_lower == "lime")        return lime();
        if (name_lower == "blue")        return blue();
        if (name_lower == "yellow")      return yellow();
        if (name_lower == "cyan")        return cyan();
        if (name_lower == "magenta")     return magenta();
        if (name_lower == "silver")      return silver();
        if (name_lower == "gray" || name_lower == "grey") return gray();
        if (name_lower == "maroon")      return maroon();
        if (name_lower == "olive")       return olive();
        if (name_lower == "purple")      return purple();
        if (name_lower == "teal")        return teal();
        if (name_lower == "navy")        return navy();
        if (name_lower == "orange")      return orange();
        if (name_lower == "pink")        return pink();
        if (name_lower == "brown")       return brown();
        if (name_lower == "transparent") return transparent();

        throw std::invalid_argument("Unknown color name: '" + name + "'");
    }

    /**
     * @brief Create a color from a 32-bit integer value.
     * 
     * Parses a 32-bit integer in the format 0xRRGGBBAA (or 0xAABBGGRR depending
     * on system endianness) and creates a Color instance.
     * 
     * @param value 32-bit integer representing the color.
     *              Expected format: 0xRRGGBBAA
     *              - Bits 24-31: Red component
     *              - Bits 16-23: Green component
     *              - Bits 8-15:  Blue component
     *              - Bits 0-7:   Alpha component
     * 
     * @return A Color instance with the parsed RGBA components.
     * 
     * @par Example
     * @code
     * Color c1 = Color::fromValue(0xFF0000FF);  // Red, opaque
     * Color c2 = Color::fromValue(0x00000080);  // Black, 50% alpha
     * Color c3 = Color::fromValue(0xFF00FF00);  // Green, opaque
     * @endcode
     * 
     * @note The expected format is 0xRRGGBBAA where:
     *       - RR = Red (bits 24-31)
     *       - GG = Green (bits 16-23)
     *       - BB = Blue (bits 8-15)
     *       - AA = Alpha (bits 0-7)
     */
    static Color fromValue(uint32_t value) {
        uint8_t r = static_cast<uint8_t>((value >> 24) & 0xFF);
        uint8_t g = static_cast<uint8_t>((value >> 16) & 0xFF);
        uint8_t b = static_cast<uint8_t>((value >> 8) & 0xFF);
        uint8_t a = static_cast<uint8_t>(value & 0xFF);
        return Color(r, g, b, a);
    }

    // ==================== Predefined Colors ====================

    /**
     * @brief Returns the color black (0, 0, 0, 255).
     * @return Color instance representing black.
     */
    static Color black()       { return Color(0, 0, 0); }

    /**
     * @brief Returns the color white (255, 255, 255, 255).
     * @return Color instance representing white.
     */
    static Color white()       { return Color(255, 255, 255); }

    /**
     * @brief Returns the color red (255, 0, 0, 255).
     * @return Color instance representing red.
     */
    static Color red()         { return Color(255, 0, 0); }

    /**
     * @brief Returns the color green (0, 128, 0, 255).
     * @return Color instance representing green.
     */
    static Color green()       { return Color(0, 128, 0); }

    /**
     * @brief Returns the color lime (0, 255, 0, 255).
     * @return Color instance representing lime.
     */
    static Color lime()        { return Color(0, 255, 0); }

    /**
     * @brief Returns the color blue (0, 0, 255, 255).
     * @return Color instance representing blue.
     */
    static Color blue()        { return Color(0, 0, 255); }

    /**
     * @brief Returns the color yellow (255, 255, 0, 255).
     * @return Color instance representing yellow.
     */
    static Color yellow()      { return Color(255, 255, 0); }

    /**
     * @brief Returns the color cyan (0, 255, 255, 255).
     * @return Color instance representing cyan.
     */
    static Color cyan()        { return Color(0, 255, 255); }

    /**
     * @brief Returns the color magenta (255, 0, 255, 255).
     * @return Color instance representing magenta.
     */
    static Color magenta()     { return Color(255, 0, 255); }

    /**
     * @brief Returns the color silver (192, 192, 192, 255).
     * @return Color instance representing silver.
     */
    static Color silver()      { return Color(192, 192, 192); }

    /**
     * @brief Returns the color gray (128, 128, 128, 255).
     * @return Color instance representing gray.
     */
    static Color gray()        { return Color(128, 128, 128); }

    /**
     * @brief Returns the color maroon (128, 0, 0, 255).
     * @return Color instance representing maroon.
     */
    static Color maroon()      { return Color(128, 0, 0); }

    /**
     * @brief Returns the color olive (128, 128, 0, 255).
     * @return Color instance representing olive.
     */
    static Color olive()       { return Color(128, 128, 0); }

    /**
     * @brief Returns the color purple (128, 0, 128, 255).
     * @return Color instance representing purple.
     */
    static Color purple()      { return Color(128, 0, 128); }

    /**
     * @brief Returns the color teal (0, 128, 128, 255).
     * @return Color instance representing teal.
     */
    static Color teal()        { return Color(0, 128, 128); }

    /**
     * @brief Returns the color navy (0, 0, 128, 255).
     * @return Color instance representing navy.
     */
    static Color navy()        { return Color(0, 0, 128); }

    /**
     * @brief Returns the color orange (255, 165, 0, 255).
     * @return Color instance representing orange.
     */
    static Color orange()      { return Color(255, 165, 0); }

    /**
     * @brief Returns the color pink (255, 192, 203, 255).
     * @return Color instance representing pink.
     */
    static Color pink()        { return Color(255, 192, 203); }

    /**
     * @brief Returns the color brown (165, 42, 42, 255).
     * @return Color instance representing brown.
     */
    static Color brown()       { return Color(165, 42, 42); }

    /**
     * @brief Returns the color transparent (0, 0, 0, 0).
     * @return Color instance representing fully transparent.
     */
    static Color transparent() { return Color(0, 0, 0, 0); }

    // Static member-variable form, distinct from the black()/white()/
    // transparent() factory functions above (used pervasively as
    // default values through the SVG engine, e.g. `Color::Transparent`).
    static const Color Black;
    static const Color White;
    static const Color Transparent;
};

inline const Color Color::Black(0, 0, 0, 255);
inline const Color Color::White(255, 255, 255, 255);
inline const Color Color::Transparent(0, 0, 0, 0);

// ==================== ColorBlend ====================

/**
 * @class ColorBlend
 * @brief Lightweight helper class for color blending operations.
 * 
 * This class holds the intermediate state of a color blend operation
 * and allows chaining multiple blends in a fluent manner.
 * 
 * @note This class is primarily used internally by the blend operators.
 *       It supports implicit conversion to Color when the blend is finalized.
 * 
 * @par Usage
 * @code
 * // Single blend with transparent (default)
 * ColorBlend b1 = "gray"_c | 50;
 * Color c1 = b1;  // 50% gray + 50% transparent
 * 
 * // Chain with another color
 * Color c2 = "gray"_c | 50 | "black"_c;
 * @endcode
 */
struct ColorBlend {
    Color baseColor;   ///< The base color being blended
    int percentage;    ///< Percentage for the blend operation (0-100)

    /**
     * @brief Construct a ColorBlend from a base color and percentage.
     * 
     * @param base The base color to blend.
     * @param percent The percentage of the blend (0-100).
     */
    ColorBlend(const Color& base, int percent)
        : baseColor(base), percentage(percent) {}

    /**
     * @brief Finalize the blend with a specific color.
     * 
     * Blends the base color with the specified color using the
     * stored percentage.
     * 
     * @param other The color to blend with.
     * @return The final blended Color.
     * 
     * @par Example
     * @code
     * Color c = ("gray"_c | 50) | "black"_c;
     * @endcode
     */
    Color operator|(const Color& other) const {
        return baseColor.blend(other, percentage);
    }

    /**
     * @brief Chain multiple blend operations.
     * 
     * Allows chaining like: "gray"_c | 50 | "black"_c
     * 
     * @param other The ColorBlend to chain with.
     * @return A new ColorBlend representing the chained operation.
     */
    ColorBlend operator|(const ColorBlend& other) const {
        // First blend: baseColor | percentage → transparent
        Color intermediate = baseColor.blend(Color::transparent(), percentage);
        // Second blend: intermediate | other.percentage → other.baseColor
        return ColorBlend(intermediate, other.percentage);
    }

    /**
     * @brief Implicit conversion to Color.
     * 
     * Finalizes the blend with transparent (default behavior).
     * 
     * @return The final blended color.
     */
    operator Color() const {
        return baseColor.blend(Color::transparent(), percentage);
    }
};

/**
 * @brief Blend a color with transparent by a percentage.
 * 
 * Creates a ColorBlend that can be further chained with other colors.
 * 
 * @param color The base color.
 * @param percentage Percentage of transparent to mix (0-100).
 * @return ColorBlend object for further chaining.
 */
inline ColorBlend Color::operator|(int percentage) const {
    return ColorBlend(*this, percentage);
}

// ==================== Non-Member Operators ====================

/**
 * @brief Stream output operator for Color.
 * 
 * Allows using Color with std::cout and other output streams.
 * 
 * @param os Output stream (e.g., std::cout).
 * @param color Color to output.
 * @return Reference to the output stream.
 * 
 * @par Example
 * @code
 * Color c(255, 0, 0);
 * std::cout << c << std::endl;  // Prints: Color(r=255, g=0, b=0, a=255)
 * @endcode
 */
inline std::ostream& operator<<(std::ostream& os, const Color& color) {
    os << color.toString();
    return os;
}

/**
 * @brief Stream output operator for ColorBlend.
 * 
 * Allows using ColorBlend with output streams. The blend is automatically
 * converted to a Color before output.
 * 
 * @param os Output stream.
 * @param blend ColorBlend to output.
 * @return Reference to the output stream.
 */
inline std::ostream& operator<<(std::ostream& os, const ColorBlend& blend) {
    os << static_cast<Color>(blend);
    return os;
}

} // namespace novasvg

// ==================== Color Literals Namespace ====================

/**
 * @namespace novasvg::color_literals
 * @brief String literal operators for creating Color instances.
 * 
 * This namespace provides the _c suffix operator for creating colors
 * from string literals at compile-time (when possible).
 * 
 * @par Usage
 * @code
 * using namespace novasvg::color_literals;
 * 
 * Color c1 = "red"_c;           // From color name
 * Color c2 = "#FF0000"_c;      // From hex
 * Color c3 = "gray"_c | 50;    // Blend with transparent
 * Color c4 = "gray"_c | 50 | "black"_c;  // Chain blends
 * @endcode
 * 
 * @note Always include "using namespace novasvg::color_literals;" or
 *       use explicit qualification to use these literals.
 */
namespace novasvg::color_literals {

/**
 * @brief String literal operator for creating Color instances.
 * 
 * This operator allows creating colors using the _c suffix on string literals.
 * It automatically detects whether the string is a hex color code or a
 * color name.
 * 
 * @param str String literal (must include quotes).
 * @param len Length of the string literal.
 * 
 * @return Color instance parsed from the string.
 * 
 * @throws std::invalid_argument If the format is invalid or 
 *         the color name is not recognized.
 * 
 * @par Example
 * @code
 * using namespace novasvg::color_literals;
 * 
 * Color c1 = "red"_c;         // Color::red()
 * Color c2 = "#FF0000"_c;     // Color::fromHash("#FF0000")
 * Color c3 = "skyblue"_c;     // Color::fromName("skyblue")
 * Color c4 = "gray"_c | 50;   // Blend with 50% transparent
 * @endcode
 * 
 * @note The string must be a valid string literal. For runtime strings,
 *       use Color::fromString() instead.
 */
inline novasvg::Color operator""_c(const char* str, std::size_t len) {
    return novasvg::Color::fromString(std::string(str, len));
}

} // namespace novasvg::color_literals

#endif // NOVASVG_COLOR_H