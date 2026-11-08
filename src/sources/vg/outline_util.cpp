// What the engine needs on top of stb::outline: SVG arcs, SVG path data, shapes with the exact geometry the
// rasterizer was tuned against, the point box. See canvas.h.

#include "canvas.h"
#include "utils.h"

namespace nova::vg {

static void get_current_point(const stb::outline& o, float* x, float* y)
{
    const stb::point p = o.current_point().value_or(stb::point{0.f, 0.f});
    if(x) *x = p.x;
    if(y) *y = p.y;
}

void arc_to_radians(stb::outline& path_, float rx, float ry, float angle, bool large_arc_flag, bool sweep_flag, float x, float y)
{
    float current_x, current_y;
    get_current_point(path_, &current_x, &current_y);
    if(rx == 0.f || ry == 0.f || (current_x == x && current_y == y)) {
        path_.line_to(x, y);
        return;
    }

    if(rx < 0.f) rx = -rx;
    if(ry < 0.f) ry = -ry;

    float dx = (current_x - x) * 0.5f;
    float dy = (current_y - y) * 0.5f;

    stb::matrix matrix;
    matrix = stb::matrix::rotation(-angle);
    { const stb::point p_ = matrix.map(dx, dy); dx = p_.x; dy = p_.y; }

    float rxrx = rx * rx;
    float ryry = ry * ry;
    float dxdx = dx * dx;
    float dydy = dy * dy;
    float radius = dxdx / rxrx + dydy / ryry;
    if(radius > 1.f) {
        rx *= sqrtf(radius);
        ry *= sqrtf(radius);
    }

    matrix = stb::matrix::scaling(1.f / rx, 1.f / ry);
    matrix.rotate(-angle);

    float x1, y1;
    float x2, y2;
    { const stb::point p_ = matrix.map(current_x, current_y); x1 = p_.x; y1 = p_.y; }
    { const stb::point p_ = matrix.map(x, y); x2 = p_.x; y2 = p_.y; }

    float dx1 = x2 - x1;
    float dy1 = y2 - y1;
    float d = dx1 * dx1 + dy1 * dy1;
    float scale_sq = 1.f / d - 0.25f;
    if(scale_sq < 0.f) scale_sq = 0.f;
    float scale = sqrtf(scale_sq);
    if(sweep_flag == large_arc_flag)
        scale = -scale;
    dx1 *= scale;
    dy1 *= scale;

    float cx1 = 0.5f * (x1 + x2) - dy1;
    float cy1 = 0.5f * (y1 + y2) + dx1;

    float th1 = atan2f(y1 - cy1, x1 - cx1);
    float th2 = atan2f(y2 - cy1, x2 - cx1);
    float th_arc = th2 - th1;
    if(th_arc < 0.f && sweep_flag)
        th_arc += NOVA_TWO_PI;
    else if(th_arc > 0.f && !sweep_flag)
        th_arc -= NOVA_TWO_PI;
    matrix = stb::matrix::rotation(angle);
    matrix.scale(rx, ry);
    int segments = (int)(ceilf(fabsf(th_arc / (NOVA_HALF_PI + 0.001f))));
    for(int i = 0; i < segments; i++) {
        float th_start = th1 + i * th_arc / segments;
        float th_end = th1 + (i + 1) * th_arc / segments;
        float t = (8.f / 6.f) * tanf(0.25f * (th_end - th_start));

        float x3 = cosf(th_end) + cx1;
        float y3 = sinf(th_end) + cy1;

        float cp2x = x3 + t * sinf(th_end);
        float cp2y = y3 - t * cosf(th_end);

        float cp1x = cosf(th_start) - t * sinf(th_start);
        float cp1y = sinf(th_start) + t * cosf(th_start);

        cp1x += cx1;
        cp1y += cy1;

        { const stb::point p_ = matrix.map(cp1x, cp1y); cp1x = p_.x; cp1y = p_.y; }
        { const stb::point p_ = matrix.map(cp2x, cp2y); cp2x = p_.x; cp2y = p_.y; }
        { const stb::point p_ = matrix.map(x3, y3); x3 = p_.x; y3 = p_.y; }

        path_.cubic_to(cp1x, cp1y, cp2x, cp2y, x3, y3);
    }
}


static bool parse_arc_flag(const char** begin, const char* end, bool* flag)
{
    if(skip_delim(begin, end, '0'))
        *flag = 0;
    else if(skip_delim(begin, end, '1'))
        *flag = 1;
    else
        return false;
    skip_ws_or_comma(begin, end, NULL);
    return true;
}

static bool parse_path_coordinates(const char** begin, const char* end, float values[6], int offset, int count)
{
    for(int i = 0; i < count; i++) {
        if(!parse_number(begin, end, values + offset + i))
            return false;
        skip_ws_or_comma(begin, end, NULL);
    }

    return true;
}

static bool parse_svg_path(stb::outline& path_, const char* data, int length_)
{
    if(length_ == -1)
        length_ = strlen(data);
    const char* it = data;
    const char* end = it + length_;

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
    skip_ws(&it, end);
    while(it < end) {
        if(NOVA_IS_ALPHA(*it)) {
            command = *it++;
            skip_ws(&it, end);
        }

        if(!last_command && !(command == 'M' || command == 'm'))
            return false;
        if(command == 'M' || command == 'm') {
            if(!parse_path_coordinates(&it, end, values, 0, 2))
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
            if(!parse_path_coordinates(&it, end, values, 0, 2))
                return false;
            if(command == 'l') {
                values[0] += current_x;
                values[1] += current_y;
            }

            path_.line_to(values[0], values[1]);
            current_x = values[0];
            current_y = values[1];
        } else if(command == 'H' || command == 'h') {
            if(!parse_path_coordinates(&it, end, values, 0, 1))
                return false;
            if(command == 'h') {
                values[0] += current_x;
            }

            path_.line_to(values[0], current_y);
            current_x = values[0];
        } else if(command == 'V' || command == 'v') {
            if(!parse_path_coordinates(&it, end, values, 1, 1))
                return false;
            if(command == 'v') {
                values[1] += current_y;
            }

            path_.line_to(current_x, values[1]);
            current_y = values[1];
        } else if(command == 'Q' || command == 'q') {
            if(!parse_path_coordinates(&it, end, values, 0, 4))
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
            if(!parse_path_coordinates(&it, end, values, 0, 6))
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

            if(!parse_path_coordinates(&it, end, values, 2, 2))
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

            if(!parse_path_coordinates(&it, end, values, 2, 4))
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
            if(!parse_path_coordinates(&it, end, values, 0, 3)
                || !parse_arc_flag(&it, end, &flags[0])
                || !parse_arc_flag(&it, end, &flags[1])
                || !parse_path_coordinates(&it, end, values, 3, 2)) {
                return false;
            }

            if(command == 'a') {
                values[3] += current_x;
                values[4] += current_y;
            }

            arc_to_radians(path_, values[0], values[1], NOVA_DEG2RAD(values[2]), flags[0], flags[1], values[3], values[4]);
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



void add_rect_path(stb::outline& path_, float x, float y, float w, float h)
{
    path_.move_to(x, y);
    path_.line_to(x + w, y);
    path_.line_to(x + w, y + h);
    path_.line_to(x, y + h);
    path_.line_to(x, y);
    path_.close();
}

void add_round_rect_path(stb::outline& path_, float x, float y, float w, float h, float rx, float ry)
{
    rx = STB_MIN(rx, w * 0.5f);
    ry = STB_MIN(ry, h * 0.5f);
    if(rx == 0.f && ry == 0.f) {
        add_rect_path(path_, x, y, w, h);
        return;
    }

    const float right = x + w, bottom = y + h;
    const float cpx = rx * NOVA_KAPPA, cpy = ry * NOVA_KAPPA;
    path_.move_to(x, y + ry);
    path_.cubic_to(x, y + ry - cpy, x + rx - cpx, y, x + rx, y);
    path_.line_to(right - rx, y);
    path_.cubic_to(right - rx + cpx, y, right, y + ry - cpy, right, y + ry);
    path_.line_to(right, bottom - ry);
    path_.cubic_to(right, bottom - ry + cpy, right - rx + cpx, bottom, right - rx, bottom);
    path_.line_to(x + rx, bottom);
    path_.cubic_to(x + rx - cpx, bottom, x, bottom - ry + cpy, x, bottom - ry);
    path_.line_to(x, y + ry);
    path_.close();
}

void add_ellipse_path(stb::outline& path_, float cx, float cy, float rx, float ry)
{
    const float left = cx - rx, top = cy - ry, right = cx + rx, bottom = cy + ry;
    const float cpx = rx * NOVA_KAPPA, cpy = ry * NOVA_KAPPA;
    path_.move_to(cx, top);
    path_.cubic_to(cx + cpx, top, right, cy - cpy, right, cy);
    path_.cubic_to(right, cy + cpy, cx + cpx, bottom, cx, bottom);
    path_.cubic_to(cx - cpx, bottom, left, cy + cpy, left, cy);
    path_.cubic_to(left, cy - cpy, cx - cpx, top, cx, top);
    path_.close();
}

void arc_to(stb::outline& outline, float rx, float ry, float x_axis_rotation_deg, bool large_arc, bool sweep, float x, float y)
{
    arc_to_radians(outline, rx, ry, NOVA_DEG2RAD(x_axis_rotation_deg), large_arc, sweep, x, y);
}

bool parse_path(stb::outline& outline, const char* data, size_t length_)
{
    outline.clear();
    return parse_svg_path(outline, data, (int)length_);
}

rect bounding_rect(const stb::outline& outline)
{
    // The control-point box of the outline with its quadratics raised to cubics (what the C API's path_extents measured).
    bool first = true;
    float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    auto add = [&](float x, float y) {
        if(first) { x0 = x1 = x; y0 = y1 = y; first = false; return; }
        x0 = STB_MIN(x0, x); y0 = STB_MIN(y0, y); x1 = STB_MAX(x1, x); y1 = STB_MAX(y1, y);
    };
    stb::point cur{0.f, 0.f};
    for(const stb::outline_command& c : outline) {
        switch(c.op) {
        case stb::outline_op::move_to:
        case stb::outline_op::line_to:
            add(c.p[0].x, c.p[0].y); cur = c.p[0]; break;
        case stb::outline_op::quad_to:
            add(2.f / 3.f * c.p[0].x + 1.f / 3.f * cur.x, 2.f / 3.f * c.p[0].y + 1.f / 3.f * cur.y);
            add(2.f / 3.f * c.p[0].x + 1.f / 3.f * c.p[1].x, 2.f / 3.f * c.p[0].y + 1.f / 3.f * c.p[1].y);
            add(c.p[1].x, c.p[1].y); cur = c.p[1]; break;
        case stb::outline_op::cubic_to:
            add(c.p[0].x, c.p[0].y); add(c.p[1].x, c.p[1].y); add(c.p[2].x, c.p[2].y); cur = c.p[2]; break;
        case stb::outline_op::close:
            break;
        }
    }
    if(first)
        return rect::empty_rect;
    return rect(x0, y0, x1 - x0, y1 - y0);
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
