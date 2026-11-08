#include "matrix.h"

namespace nova {
namespace vg {

static int parse_matrix_parameters(const char** begin, const char* end, float values[6], int required, int optional);

void map_points(const stb::matrix& matrix, const stb::point* src, stb::point* dst, int count)
{
    for(int i = 0; i < count; ++i) {
        dst[i] = matrix.map(src[i]);
    }
}

void mapped_bounds(const stb::matrix& matrix, const rect_t* src, rect_t* dst)
{
    stb::point p[4];
    p[0].x = src->x;
    p[0].y = src->y;
    p[1].x = src->x + src->w;
    p[1].y = src->y;
    p[2].x = src->x + src->w;
    p[2].y = src->y + src->h;
    p[3].x = src->x;
    p[3].y = src->y + src->h;
    map_points(matrix, p, p, 4);

    float l = p[0].x;
    float t = p[0].y;
    float r = p[0].x;
    float b = p[0].y;

    for(int i = 1; i < 4; i++) {
        if(p[i].x < l) l = p[i].x;
        if(p[i].x > r) r = p[i].x;
        if(p[i].y < t) t = p[i].y;
        if(p[i].y > b) b = p[i].y;
    }

    dst->x = l;
    dst->y = t;
    dst->w = r - l;
    dst->h = b - t;
}

static int parse_matrix_parameters(const char** begin, const char* end, float values[6], int required, int optional)
{
    if(!skip_ws_and_delim(begin, end, '('))
        return 0;
    int count = 0;
    int max_count = required + optional;
    bool has_trailing_comma = false;
    for(; count < max_count; ++count) {
        if(!parse_number(begin, end, values + count))
            break;
        skip_css_unit(begin, end, values + count);
        skip_ws_or_comma(begin, end, &has_trailing_comma);
    }

    if(!has_trailing_comma && (count == required || count == max_count)
        && skip_delim(begin, end, ')')) {
        return count;
    }

    return 0;
}

bool parse_matrix(stb::matrix* matrix, const char* data, int length_)
{
    float values[6];
    (*matrix) = stb::matrix{};
    if(length_ == -1)
        length_ = strlen(data);
    const char* it = data;
    const char* end = it + length_;
    bool has_trailing_comma = false;
    skip_ws(&it, end);
    while(it < end) {
        if(skip_string(&it, end, "matrix")) {
            int count = parse_matrix_parameters(&it, end, values, 6, 0);
            if(count == 0)
                return false;
            stb::matrix m = { values[0], values[1], values[2], values[3], values[4], values[5] };
            (*matrix) = m.then((*matrix));
        } else if(skip_string(&it, end, "translate")) {
            int count = parse_matrix_parameters(&it, end, values, 1, 1);
            if(count == 0)
                return false;
            if(count == 1) {
                matrix->translate(values[0], 0);
            } else {
                matrix->translate(values[0], values[1]);
            }
        } else if(skip_string(&it, end, "scale")) {
            int count = parse_matrix_parameters(&it, end, values, 1, 1);
            if(count == 0)
                return false;
            if(count == 1) {
                matrix->scale(values[0], values[0]);
            } else {
                matrix->scale(values[0], values[1]);
            }
        } else if(skip_string(&it, end, "rotate")) {
            int count = parse_matrix_parameters(&it, end, values, 1, 2);
            if(count == 0)
                return false;
            if(count == 3)
                matrix->translate(values[1], values[2]);
            matrix->rotate(NOVA_DEG2RAD(values[0]));
            if(count == 3) {
                matrix->translate(-values[1], -values[2]);
            }
        } else if(skip_string(&it, end, "skewX")) {
            int count = parse_matrix_parameters(&it, end, values, 1, 0);
            if(count == 0)
                return false;
            matrix->shear(NOVA_DEG2RAD(values[0]), 0);
        } else if(skip_string(&it, end, "skewY")) {
            int count = parse_matrix_parameters(&it, end, values, 1, 0);
            if(count == 0)
                return false;
            matrix->shear(0, NOVA_DEG2RAD(values[0]));
        } else {
            return false;
        }

        skip_ws_or_comma(&it, end, &has_trailing_comma);
    }

    return !has_trailing_comma;
}

} // namespace vg
} // namespace nova
