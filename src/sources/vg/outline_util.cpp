// What the engine needs on top of stb::outline: SVG path data and a walker over the commands. See canvas.h.

#include "canvas.h"
#include "parserutils.h"
#include "utils.h"

#include <cstring>
#include <string_view>

namespace nova::vg {

static bool parse_arc_flag(std::string_view& input, bool* flag)
{
    if(skip_delimiter(input, '0'))
        *flag = 0;
    else if(skip_delimiter(input, '1'))
        *flag = 1;
    else
        return false;
    skip_optional_spaces_or_comma(input);
    return true;
}

static bool parse_path_coordinates(std::string_view& input, float values[6], int offset, int count)
{
    for(int i = 0; i < count; i++) {
        if(!parse_number(input, values[offset + i]))
            return false;
        skip_optional_spaces_or_comma(input);
    }

    return true;
}

static bool parse_svg_path(stb::outline& path_, const char* data, int length_)
{
    std::string_view input(data, length_ == -1 ? strlen(data) : size_t(length_));

    float values[6];
    bool flags[2];

    float start_x = 0;
    float start_y = 0;
    float current_x = 0;
    float current_y = 0;
    float last_control_x = 0;
    float last_control_y = 0;

    char command = 0;
    char last_command = 0;
    skip_optional_spaces(input);
    while(!input.empty()) {
        if(IS_ALPHA(input.front())) {
            command = input.front();
            input.remove_prefix(1);
            skip_optional_spaces(input);
        }

        if(!last_command && !(command == 'M' || command == 'm'))
            return false;
        if(command == 'M' || command == 'm') {
            if(!parse_path_coordinates(input, values, 0, 2))
                return false;
            if(command == 'm') {
                values[0] += current_x;
                values[1] += current_y;
            }

            path_.move_to(values[0], values[1]);
            current_x = start_x = values[0];
            current_y = start_y = values[1];
            command = command == 'm' ? 'l' : 'L';
        } else if(command == 'L' || command == 'l') {
            if(!parse_path_coordinates(input, values, 0, 2))
                return false;
            if(command == 'l') {
                values[0] += current_x;
                values[1] += current_y;
            }

            path_.line_to(values[0], values[1]);
            current_x = values[0];
            current_y = values[1];
        } else if(command == 'H' || command == 'h') {
            if(!parse_path_coordinates(input, values, 0, 1))
                return false;
            if(command == 'h') {
                values[0] += current_x;
            }

            path_.line_to(values[0], current_y);
            current_x = values[0];
        } else if(command == 'V' || command == 'v') {
            if(!parse_path_coordinates(input, values, 1, 1))
                return false;
            if(command == 'v') {
                values[1] += current_y;
            }

            path_.line_to(current_x, values[1]);
            current_y = values[1];
        } else if(command == 'Q' || command == 'q') {
            if(!parse_path_coordinates(input, values, 0, 4))
                return false;
            if(command == 'q') {
                values[0] += current_x;
                values[1] += current_y;
                values[2] += current_x;
                values[3] += current_y;
            }

            path_.quad_to(values[0], values[1], values[2], values[3]);
            last_control_x = values[0];
            last_control_y = values[1];
            current_x = values[2];
            current_y = values[3];
        } else if(command == 'C' || command == 'c') {
            if(!parse_path_coordinates(input, values, 0, 6))
                return false;
            if(command == 'c') {
                values[0] += current_x;
                values[1] += current_y;
                values[2] += current_x;
                values[3] += current_y;
                values[4] += current_x;
                values[5] += current_y;
            }

            path_.cubic_to(values[0], values[1], values[2], values[3], values[4], values[5]);
            last_control_x = values[2];
            last_control_y = values[3];
            current_x = values[4];
            current_y = values[5];
        } else if(command == 'T' || command == 't') {
            if(last_command != 'Q' && last_command != 'q' && last_command != 'T' && last_command != 't') {
                values[0] = current_x;
                values[1] = current_y;
            } else {
                values[0] = 2 * current_x - last_control_x;
                values[1] = 2 * current_y - last_control_y;
            }

            if(!parse_path_coordinates(input, values, 2, 2))
                return false;
            if(command == 't') {
                values[2] += current_x;
                values[3] += current_y;
            }

            path_.quad_to(values[0], values[1], values[2], values[3]);
            last_control_x = values[0];
            last_control_y = values[1];
            current_x = values[2];
            current_y = values[3];
        } else if(command == 'S' || command == 's') {
            if(last_command != 'C' && last_command != 'c' && last_command != 'S' && last_command != 's') {
                values[0] = current_x;
                values[1] = current_y;
            } else {
                values[0] = 2 * current_x - last_control_x;
                values[1] = 2 * current_y - last_control_y;
            }

            if(!parse_path_coordinates(input, values, 2, 4))
                return false;
            if(command == 's') {
                values[2] += current_x;
                values[3] += current_y;
                values[4] += current_x;
                values[5] += current_y;
            }

            path_.cubic_to(values[0], values[1], values[2], values[3], values[4], values[5]);
            last_control_x = values[2];
            last_control_y = values[3];
            current_x = values[4];
            current_y = values[5];
        } else if(command == 'A' || command == 'a') {
            if(!parse_path_coordinates(input, values, 0, 3)
                || !parse_arc_flag(input, &flags[0])
                || !parse_arc_flag(input, &flags[1])
                || !parse_path_coordinates(input, values, 3, 2)) {
                return false;
            }

            if(command == 'a') {
                values[3] += current_x;
                values[4] += current_y;
            }

            path_.arc_to(values[0], values[1], values[2], flags[0], flags[1], values[3], values[4]);
            current_x = values[3];
            current_y = values[4];
        } else if(command == 'Z' || command == 'z') {
            if(last_command == 'Z' || last_command == 'z')
                return false;
            path_.close();
            current_x = start_x;
            current_y = start_y;
        } else {
            return false;
        }

        last_command = command;
    }

    return true;
}



bool parse_path(stb::outline& outline, const char* data, size_t length_)
{
    outline.clear();
    return parse_svg_path(outline, data, (int)length_);
}

outline_walker::outline_walker(const stb::outline& outline)
{
    stb::point start{0.f, 0.f};
    stb::point current{0.f, 0.f};
    m_segments.reserve(outline.size());
    for(const stb::outline_command& c : outline) {
        segment s{};
        switch(c.op) {
        case stb::outline_op::move_to:
            s.command = segment_kind::move_to;
            s.points[0] = point(c.p[0].x, c.p[0].y);
            start = current = c.p[0];
            break;
        case stb::outline_op::line_to:
            s.command = segment_kind::line_to;
            s.points[0] = point(c.p[0].x, c.p[0].y);
            current = c.p[0];
            break;
        case stb::outline_op::quad_to: {
            // Exactly what path_quad_to does: the quadratic raised to a cubic.
            s.command = segment_kind::cubic_to;
            const float x1 = 2.f / 3.f * c.p[0].x + 1.f / 3.f * current.x;
            const float y1 = 2.f / 3.f * c.p[0].y + 1.f / 3.f * current.y;
            const float x2 = 2.f / 3.f * c.p[0].x + 1.f / 3.f * c.p[1].x;
            const float y2 = 2.f / 3.f * c.p[0].y + 1.f / 3.f * c.p[1].y;
            s.points[0] = point(x1, y1);
            s.points[1] = point(x2, y2);
            s.points[2] = point(c.p[1].x, c.p[1].y);
            current = c.p[1];
            break;
        }
        case stb::outline_op::cubic_to:
            s.command = segment_kind::cubic_to;
            s.points[0] = point(c.p[0].x, c.p[0].y);
            s.points[1] = point(c.p[1].x, c.p[1].y);
            s.points[2] = point(c.p[2].x, c.p[2].y);
            current = c.p[2];
            break;
        case stb::outline_op::close:
            s.command = segment_kind::close;
            s.points[0] = point(start.x, start.y);
            current = start;
            break;
        }

        m_segments.push_back(s);
    }
}

segment_kind outline_walker::current_segment(std::array<point, 3>& points) const
{
    const segment& s = m_segments[m_index];
    points[0] = s.points[0];
    points[1] = s.points[1];
    points[2] = s.points[2];
    return s.command;
}

} // namespace nova::vg
