#include "matrix.h"
#include "parserutils.h"

#include <cstring>
#include <string_view>

namespace nova {
namespace vg {

// A number followed by a CSS unit: angles in rad, grad and turn are turned into degrees, any other unit (deg, px, pt, %)
// is dropped and the value used as it is.
// False for em and ex (and any unit that starts with them): a length relative to the font cannot be resolved in a
// transform, which is then invalid.
static bool skip_css_unit(std::string_view& input, float* number)
{
    size_t length = 0;
    while(length < input.size() && (IS_ALPHA(input[length]) || input[length] == '%'))
        ++length;
    if(length == 0)
        return true;

    const std::string_view unit = input.substr(0, length);
    if(unit.substr(0, 2) == "em" || unit.substr(0, 2) == "ex")
        return false;
    if(unit == "rad")
        *number *= 180.f / 3.14159265f;
    else if(unit == "grad")
        *number *= 0.9f;
    else if(unit == "turn")
        *number *= 360.f;
    input.remove_prefix(length);
    return true;
}

static int parse_matrix_parameters(std::string_view& input, float values[6], int required, int optional)
{
    std::string_view rest = input;
    skip_optional_spaces(rest);
    if(!skip_delimiter_and_optional_spaces(rest, '(') || rest.empty())
        return 0;
    input = rest;

    int count = 0;
    int max_count = required + optional;
    bool has_trailing_comma = false;
    for(; count < max_count; ++count) {
        if(!parse_number(input, values[count]))
            break;
        if(!skip_css_unit(input, values + count))
            break;
        skip_optional_spaces_or_comma(input, &has_trailing_comma);
    }

    if(!has_trailing_comma && (count == required || count == max_count) && skip_delimiter(input, ')'))
        return count;

    return 0;
}

bool parse_matrix(stb::matrix* matrix, const char* data, int length_)
{
    float values[6];
    (*matrix) = stb::matrix{};
    std::string_view input(data, length_ == -1 ? strlen(data) : size_t(length_));
    bool has_trailing_comma = false;
    skip_optional_spaces(input);
    while(!input.empty()) {
        if(skip_string(input, "matrix")) {
            int count = parse_matrix_parameters(input, values, 6, 0);
            if(count == 0)
                return false;
            stb::matrix m = { values[0], values[1], values[2], values[3], values[4], values[5] };
            (*matrix) = m.then((*matrix));
        } else if(skip_string(input, "translate")) {
            int count = parse_matrix_parameters(input, values, 1, 1);
            if(count == 0)
                return false;
            if(count == 1) {
                matrix->translate(values[0], 0);
            } else {
                matrix->translate(values[0], values[1]);
            }
        } else if(skip_string(input, "scale")) {
            int count = parse_matrix_parameters(input, values, 1, 1);
            if(count == 0)
                return false;
            if(count == 1) {
                matrix->scale(values[0], values[0]);
            } else {
                matrix->scale(values[0], values[1]);
            }
        } else if(skip_string(input, "rotate")) {
            int count = parse_matrix_parameters(input, values, 1, 2);
            if(count == 0)
                return false;
            if(count == 3)
                matrix->translate(values[1], values[2]);
            matrix->rotate(deg_to_rad(values[0]));
            if(count == 3) {
                matrix->translate(-values[1], -values[2]);
            }
        } else if(skip_string(input, "skewX")) {
            int count = parse_matrix_parameters(input, values, 1, 0);
            if(count == 0)
                return false;
            matrix->shear(deg_to_rad(values[0]), 0);
        } else if(skip_string(input, "skewY")) {
            int count = parse_matrix_parameters(input, values, 1, 0);
            if(count == 0)
                return false;
            matrix->shear(0, deg_to_rad(values[0]));
        } else {
            return false;
        }

        skip_optional_spaces_or_comma(input, &has_trailing_comma);
    }

    return !has_trailing_comma;
}

} // namespace vg
} // namespace nova
