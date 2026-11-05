// What the engine needs on top of stb::outline: SVG arcs, SVG path data, the point box, and the bridge to the
// C API's path_t (the rasterizer's input). See canvas.h.

#include "canvas.h"

namespace nova::vg {

stb::outline from_path_t(const path_t* path)
{
    stb::outline outline;
    path_iterator_t it;
    path_iterator_init(&it, path);

    point_t points[3];
    while(path_iterator_has_next(&it)) {
        switch(path_iterator_next(&it, points)) {
        case NOVA_PATH_COMMAND_MOVE_TO:
            outline.move_to(points[0].x, points[0].y);
            break;
        case NOVA_PATH_COMMAND_LINE_TO:
            outline.line_to(points[0].x, points[0].y);
            break;
        case NOVA_PATH_COMMAND_CUBIC_TO:
            outline.cubic_to(points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y);
            break;
        case NOVA_PATH_COMMAND_CLOSE:
            outline.close();
            break;
        }
    }

    return outline;
}

path_t* to_path_t(const stb::outline& outline)
{
    path_t* path = path_create();
    for(const stb::outline_command& c : outline) {
        switch(c.op) {
        case stb::outline_op::move_to:
            path_move_to(path, c.p[0].x, c.p[0].y);
            break;
        case stb::outline_op::line_to:
            path_line_to(path, c.p[0].x, c.p[0].y);
            break;
        case stb::outline_op::quad_to:
            path_quad_to(path, c.p[0].x, c.p[0].y, c.p[1].x, c.p[1].y);
            break;
        case stb::outline_op::cubic_to:
            path_cubic_to(path, c.p[0].x, c.p[0].y, c.p[1].x, c.p[1].y, c.p[2].x, c.p[2].y);
            break;
        case stb::outline_op::close:
            path_close(path);
            break;
        }
    }

    return path;
}

void arc_to(stb::outline& outline, float rx, float ry, float x_axis_rotation_deg, bool large_arc, bool sweep, float x, float y)
{
    // The arc is converted by the C API from the current point; the cubics it makes are appended to `outline`.
    const stb::point from = outline.current_point().value_or(stb::point{0.f, 0.f});
    path_t* tmp = path_create();
    path_move_to(tmp, from.x, from.y);
    path_arc_to(tmp, rx, ry, NOVA_DEG2RAD(x_axis_rotation_deg), large_arc, sweep, x, y);

    const stb::outline tail = from_path_t(tmp);
    path_destroy(tmp);

    bool first = true;
    for(const stb::outline_command& c : tail) {
        if(first) {
            first = false;
            if(c.op == stb::outline_op::move_to)
                continue;
        }

        switch(c.op) {
        case stb::outline_op::move_to:
            outline.move_to(c.p[0]);
            break;
        case stb::outline_op::line_to:
            outline.line_to(c.p[0]);
            break;
        case stb::outline_op::quad_to:
            outline.quad_to(c.p[0], c.p[1]);
            break;
        case stb::outline_op::cubic_to:
            outline.cubic_to(c.p[0], c.p[1], c.p[2]);
            break;
        case stb::outline_op::close:
            outline.close();
            break;
        }
    }
}

bool parse_path(stb::outline& outline, const char* data, size_t length)
{
    path_t* tmp = path_create();
    const bool ok = path_parse(tmp, data, length);
    outline = from_path_t(tmp);
    path_destroy(tmp);
    return ok;
}

Rect bounding_rect(const stb::outline& outline)
{
    if(outline.empty())
        return Rect::Empty;
    path_t* tmp = to_path_t(outline);
    rect_t extents;
    path_extents(tmp, &extents, false);
    path_destroy(tmp);
    return extents;
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
            s.points[0] = Point(c.p[0].x, c.p[0].y);
            start = current = c.p[0];
            break;
        case stb::outline_op::line_to:
            s.command = segment_kind::line_to;
            s.points[0] = Point(c.p[0].x, c.p[0].y);
            current = c.p[0];
            break;
        case stb::outline_op::quad_to: {
            // Exactly what path_quad_to does: the quadratic raised to a cubic.
            s.command = segment_kind::cubic_to;
            const float x1 = 2.f / 3.f * c.p[0].x + 1.f / 3.f * current.x;
            const float y1 = 2.f / 3.f * c.p[0].y + 1.f / 3.f * current.y;
            const float x2 = 2.f / 3.f * c.p[0].x + 1.f / 3.f * c.p[1].x;
            const float y2 = 2.f / 3.f * c.p[0].y + 1.f / 3.f * c.p[1].y;
            s.points[0] = Point(x1, y1);
            s.points[1] = Point(x2, y2);
            s.points[2] = Point(c.p[1].x, c.p[1].y);
            current = c.p[1];
            break;
        }
        case stb::outline_op::cubic_to:
            s.command = segment_kind::cubic_to;
            s.points[0] = Point(c.p[0].x, c.p[0].y);
            s.points[1] = Point(c.p[1].x, c.p[1].y);
            s.points[2] = Point(c.p[2].x, c.p[2].y);
            current = c.p[2];
            break;
        case stb::outline_op::close:
            s.command = segment_kind::close;
            s.points[0] = Point(start.x, start.y);
            current = start;
            break;
        }

        m_segments.push_back(s);
    }
}

segment_kind outline_walker::current_segment(std::array<Point, 3>& points) const
{
    const segment& s = m_segments[m_index];
    points[0] = s.points[0];
    points[1] = s.points[1];
    points[2] = s.points[2];
    return s.command;
}

} // namespace nova::vg
