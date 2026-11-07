#include "path.h"

namespace nova {
namespace vg {

static path_element_t* path_add_command(path_t* path_, path_command_t command, int npoints);
static void split_bezier(const bezier_t* b, bezier_t* first, bezier_t* second);
static void dash_traverse_func(void* closure, path_command_t command, const point_t* points, int npoints);
static void clone_traverse_func(void* closure, path_command_t command, const point_t* points, int npoints);
static void extents_traverse_func(void* closure, path_command_t command, const point_t* points, int npoints);
static bool parse_arc_flag(const char** begin, const char* end, bool* flag);
static bool parse_path_coordinates(const char** begin, const char* end, float values[6], int offset, int count);

void path_iterator_init(path_iterator_t* it, const path_t* path_)
{
    it->elements = path_->elements.data;
    it->size_ = path_->elements.size_;
    it->index = 0;
}

bool path_iterator_has_next(const path_iterator_t* it)
{
    return it->index < it->size_;
}

path_command_t path_iterator_next(path_iterator_t* it, point_t points[3])
{
    const path_element_t* elements = it->elements + it->index;
    switch(elements[0].header.command) {
    case NOVA_PATH_COMMAND_MOVE_TO:
    case NOVA_PATH_COMMAND_LINE_TO:
    case NOVA_PATH_COMMAND_CLOSE:
        points[0] = elements[1].point_;
        break;
    case NOVA_PATH_COMMAND_CUBIC_TO:
        points[0] = elements[1].point_;
        points[1] = elements[2].point_;
        points[2] = elements[3].point_;
        break;
    }

    it->index += elements[0].header.length_;
    return elements[0].header.command;
}

path_t* path_create(void)
{
    path_t* path_ = static_cast<path_t*>(malloc(sizeof(path_t)));
    novasvg_init_reference(path_);
    path_->num_points = 0;
    path_->num_contours = 0;
    path_->num_curves = 0;
    path_->start_point = NOVA_EMPTY_POINT;
    novasvg_array_init(path_->elements);
    return path_;
}

path_t* path_reference(path_t* path_)
{
    novasvg_increment_reference(path_);
    return path_;
}

void path_destroy(path_t* path_)
{
    if(novasvg_destroy_reference(path_)) {
        novasvg_array_destroy(path_->elements);
        free(path_);
    }
}

int path_get_reference_count(const path_t* path_)
{
    return novasvg_get_reference_count(path_);
}

int path_get_elements(const path_t* path_, const path_element_t** elements)
{
    if(elements)
        *elements = path_->elements.data;
    return path_->elements.size_;
}

static path_element_t* path_add_command(path_t* path_, path_command_t command, int npoints)
{
    const int length_ = npoints + 1;
    novasvg_array_ensure(path_->elements, length_);
    path_element_t* elements = path_->elements.data + path_->elements.size_;
    elements->header.command = command;
    elements->header.length_ = length_;
    path_->elements.size_ += length_;
    path_->num_points += npoints;
    return elements + 1;
}

void path_move_to(path_t* path_, float x, float y)
{
    path_element_t* elements = path_add_command(path_, NOVA_PATH_COMMAND_MOVE_TO, 1);
    elements[0].point_ = NOVA_MAKE_POINT(x, y);
    path_->start_point = NOVA_MAKE_POINT(x, y);
    path_->num_contours += 1;
}

void path_line_to(path_t* path_, float x, float y)
{
    if(path_->elements.size_ == 0)
        path_move_to(path_, 0, 0);
    path_element_t* elements = path_add_command(path_, NOVA_PATH_COMMAND_LINE_TO, 1);
    elements[0].point_ = NOVA_MAKE_POINT(x, y);
}

void path_quad_to(path_t* path_, float x1, float y1, float x2, float y2)
{
    float current_x, current_y;
    path_get_current_point(path_, &current_x, &current_y);
    float cp1x = 2.f / 3.f * x1 + 1.f / 3.f * current_x;
    float cp1y = 2.f / 3.f * y1 + 1.f / 3.f * current_y;
    float cp2x = 2.f / 3.f * x1 + 1.f / 3.f * x2;
    float cp2y = 2.f / 3.f * y1 + 1.f / 3.f * y2;
    path_cubic_to(path_, cp1x, cp1y, cp2x, cp2y, x2, y2);
}

void path_cubic_to(path_t* path_, float x1, float y1, float x2, float y2, float x3, float y3)
{
    if(path_->elements.size_ == 0)
        path_move_to(path_, 0, 0);
    path_element_t* elements = path_add_command(path_, NOVA_PATH_COMMAND_CUBIC_TO, 3);
    elements[0].point_ = NOVA_MAKE_POINT(x1, y1);
    elements[1].point_ = NOVA_MAKE_POINT(x2, y2);
    elements[2].point_ = NOVA_MAKE_POINT(x3, y3);
    path_->num_curves += 1;
}

void path_arc_to(path_t* path_, float rx, float ry, float angle, bool large_arc_flag, bool sweep_flag, float x, float y)
{
    float current_x, current_y;
    path_get_current_point(path_, &current_x, &current_y);
    if(rx == 0.f || ry == 0.f || (current_x == x && current_y == y)) {
        path_line_to(path_, x, y);
        return;
    }

    if(rx < 0.f) rx = -rx;
    if(ry < 0.f) ry = -ry;

    float dx = (current_x - x) * 0.5f;
    float dy = (current_y - y) * 0.5f;

    matrix_t matrix;
    matrix_init_rotate(&matrix, -angle);
    matrix_map(&matrix, dx, dy, &dx, &dy);

    float rxrx = rx * rx;
    float ryry = ry * ry;
    float dxdx = dx * dx;
    float dydy = dy * dy;
    float radius = dxdx / rxrx + dydy / ryry;
    if(radius > 1.f) {
        rx *= sqrtf(radius);
        ry *= sqrtf(radius);
    }

    matrix_init_scale(&matrix, 1.f / rx, 1.f / ry);
    matrix_rotate(&matrix, -angle);

    float x1, y1;
    float x2, y2;
    matrix_map(&matrix, current_x, current_y, &x1, &y1);
    matrix_map(&matrix, x, y, &x2, &y2);

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
    matrix_init_rotate(&matrix, angle);
    matrix_scale(&matrix, rx, ry);
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

        matrix_map(&matrix, cp1x, cp1y, &cp1x, &cp1y);
        matrix_map(&matrix, cp2x, cp2y, &cp2x, &cp2y);
        matrix_map(&matrix, x3, y3, &x3, &y3);

        path_cubic_to(path_, cp1x, cp1y, cp2x, cp2y, x3, y3);
    }
}

void path_close(path_t* path_)
{
    if(path_->elements.size_ == 0)
        return;
    path_element_t* elements = path_add_command(path_, NOVA_PATH_COMMAND_CLOSE, 1);
    elements[0].point_ = path_->start_point;
}

void path_get_current_point(const path_t* path_, float* x, float* y)
{
    float xx = 0.f;
    float yy = 0.f;
    if(path_->num_points > 0) {
        xx = path_->elements.data[path_->elements.size_ - 1].point_.x;
        yy = path_->elements.data[path_->elements.size_ - 1].point_.y;
    }

    if(x) *x = xx;
    if(y) *y = yy;
}

void path_reserve(path_t* path_, int count)
{
    novasvg_array_ensure(path_->elements, count);
}

void path_reset(path_t* path_)
{
    novasvg_array_clear(path_->elements);
    path_->start_point = NOVA_EMPTY_POINT;
    path_->num_points = 0;
    path_->num_contours = 0;
    path_->num_curves = 0;
}

void path_add_rect(path_t* path_, float x, float y, float w, float h)
{
    path_reserve(path_, 6 * 2);
    path_move_to(path_, x, y);
    path_line_to(path_, x + w, y);
    path_line_to(path_, x + w, y + h);
    path_line_to(path_, x, y + h);
    path_line_to(path_, x, y);
    path_close(path_);
}

void path_add_round_rect(path_t* path_, float x, float y, float w, float h, float rx, float ry)
{
    rx = STB_MIN(rx, w * 0.5f);
    ry = STB_MIN(ry, h * 0.5f);
    if(rx == 0.f && ry == 0.f) {
        path_add_rect(path_, x, y, w, h);
        return;
    }

    float right = x + w;
    float bottom = y + h;

    float cpx = rx * NOVA_KAPPA;
    float cpy = ry * NOVA_KAPPA;

    path_reserve(path_, 6 * 2 + 4 * 4);
    path_move_to(path_, x, y+ry);
    path_cubic_to(path_, x, y+ry-cpy, x+rx-cpx, y, x+rx, y);
    path_line_to(path_, right-rx, y);
    path_cubic_to(path_, right-rx+cpx, y, right, y+ry-cpy, right, y+ry);
    path_line_to(path_, right, bottom-ry);
    path_cubic_to(path_, right, bottom-ry+cpy, right-rx+cpx, bottom, right-rx, bottom);
    path_line_to(path_, x+rx, bottom);
    path_cubic_to(path_, x+rx-cpx, bottom, x, bottom-ry+cpy, x, bottom-ry);
    path_line_to(path_, x, y+ry);
    path_close(path_);
}

void path_add_ellipse(path_t* path_, float cx, float cy, float rx, float ry)
{
    float left = cx - rx;
    float top = cy - ry;
    float right = cx + rx;
    float bottom = cy + ry;

    float cpx = rx * NOVA_KAPPA;
    float cpy = ry * NOVA_KAPPA;

    path_reserve(path_, 2 * 2 + 4 * 4);
    path_move_to(path_, cx, top);
    path_cubic_to(path_, cx+cpx, top, right, cy-cpy, right, cy);
    path_cubic_to(path_, right, cy+cpy, cx+cpx, bottom, cx, bottom);
    path_cubic_to(path_, cx-cpx, bottom, left, cy+cpy, left, cy);
    path_cubic_to(path_, left, cy-cpy, cx-cpx, top, cx, top);
    path_close(path_);
}

void path_add_circle(path_t* path_, float cx, float cy, float r)
{
    path_add_ellipse(path_, cx, cy, r, r);
}

void path_add_arc(path_t* path_, float cx, float cy, float r, float a0, float a1, bool ccw)
{
    float da = a1 - a0;
    if(fabsf(da) > NOVA_TWO_PI) {
        da = NOVA_TWO_PI;
    } else if(da != 0.f && ccw != (da < 0.f)) {
        da += NOVA_TWO_PI * (ccw ? -1 : 1);
    }

    int seg_n = (int)(ceilf(fabsf(da) / NOVA_HALF_PI));
    if(seg_n == 0)
        return;
    float a = a0;
    float ax = cx + cosf(a) * r;
    float ay = cy + sinf(a) * r;

    float seg_a = da / seg_n;
    float d = (seg_a / NOVA_HALF_PI) * NOVA_KAPPA * r;
    float dx = -sinf(a) * d;
    float dy = cosf(a) * d;

    path_reserve(path_, 2 + 4 * seg_n);
    if(path_->elements.size_ == 0) {
        path_move_to(path_, ax, ay);
    } else {
        path_line_to(path_, ax, ay);
    }

    for(int i = 0; i < seg_n; i++) {
        float cp1x = ax + dx;
        float cp1y = ay + dy;

        a += seg_a;
        ax = cx + cosf(a) * r;
        ay = cy + sinf(a) * r;

        dx = -sinf(a) * d;
        dy = cosf(a) * d;

        float cp2x = ax - dx;
        float cp2y = ay - dy;

        path_cubic_to(path_, cp1x, cp1y, cp2x, cp2y, ax, ay);
    }
}

void path_transform(path_t* path_, const matrix_t* matrix)
{
    path_element_t* elements = path_->elements.data;
    for(int i = 0; i < path_->elements.size_; i += elements[i].header.length_) {
        switch(elements[i].header.command) {
        case NOVA_PATH_COMMAND_MOVE_TO:
        case NOVA_PATH_COMMAND_LINE_TO:
        case NOVA_PATH_COMMAND_CLOSE:
            matrix_map_point(matrix, &elements[i + 1].point_, &elements[i + 1].point_);
            break;
        case NOVA_PATH_COMMAND_CUBIC_TO:
            matrix_map_point(matrix, &elements[i + 1].point_, &elements[i + 1].point_);
            matrix_map_point(matrix, &elements[i + 2].point_, &elements[i + 2].point_);
            matrix_map_point(matrix, &elements[i + 3].point_, &elements[i + 3].point_);
            break;
        }
    }
}

void path_add_path(path_t* path_, const path_t* source, const matrix_t* matrix)
{
    if(matrix == NULL) {
        novasvg_array_append(path_->elements, source->elements);
        path_->start_point = source->start_point;
        path_->num_points += source->num_points;
        path_->num_contours += source->num_contours;
        path_->num_curves += source->num_curves;
        return;
    }

    path_iterator_t it;
    path_iterator_init(&it, source);

    point_t points[3];
    novasvg_array_ensure(path_->elements, source->elements.size_);
    while(path_iterator_has_next(&it)) {
        switch(path_iterator_next(&it, points)) {
        case NOVA_PATH_COMMAND_MOVE_TO:
            matrix_map_points(matrix, points, points, 1);
            path_move_to(path_, points[0].x, points[0].y);
            break;
        case NOVA_PATH_COMMAND_LINE_TO:
            matrix_map_points(matrix, points, points, 1);
            path_line_to(path_, points[0].x, points[0].y);
            break;
        case NOVA_PATH_COMMAND_CUBIC_TO:
            matrix_map_points(matrix, points, points, 3);
            path_cubic_to(path_, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y);
            break;
        case NOVA_PATH_COMMAND_CLOSE:
            path_close(path_);
            break;
        }
    }
}

void path_traverse(const path_t* path_, path_traverse_func_t traverse_func, void* closure)
{
    path_iterator_t it;
    path_iterator_init(&it, path_);

    point_t points[3];
    while(path_iterator_has_next(&it)) {
        switch(path_iterator_next(&it, points)) {
        case NOVA_PATH_COMMAND_MOVE_TO:
            traverse_func(closure, NOVA_PATH_COMMAND_MOVE_TO, points, 1);
            break;
        case NOVA_PATH_COMMAND_LINE_TO:
            traverse_func(closure, NOVA_PATH_COMMAND_LINE_TO, points, 1);
            break;
        case NOVA_PATH_COMMAND_CUBIC_TO:
            traverse_func(closure, NOVA_PATH_COMMAND_CUBIC_TO, points, 3);
            break;
        case NOVA_PATH_COMMAND_CLOSE:
            traverse_func(closure, NOVA_PATH_COMMAND_CLOSE, points, 1);
            break;
        }
    }
}

static void split_bezier(const bezier_t* b, bezier_t* first, bezier_t* second)
{
    float c = (b->x2 + b->x3) * 0.5f;
    first->x2 = (b->x1 + b->x2) * 0.5f;
    second->x3 = (b->x3 + b->x4) * 0.5f;
    first->x1 = b->x1;
    second->x4 = b->x4;
    first->x3 = (first->x2 + c) * 0.5f;
    second->x2 = (second->x3 + c) * 0.5f;
    first->x4 = second->x1 = (first->x3 + second->x2) * 0.5f;

    c = (b->y2 + b->y3) * 0.5f;
    first->y2 = (b->y1 + b->y2) * 0.5f;
    second->y3 = (b->y3 + b->y4) * 0.5f;
    first->y1 = b->y1;
    second->y4 = b->y4;
    first->y3 = (first->y2 + c) * 0.5f;
    second->y2 = (second->y3 + c) * 0.5f;
    first->y4 = second->y1 = (first->y3 + second->y2) * 0.5f;
}

void path_traverse_flatten(const path_t* path_, path_traverse_func_t traverse_func, void* closure)
{
    if(path_->num_curves == 0) {
        path_traverse(path_, traverse_func, closure);
        return;
    }

    const float threshold = 0.25f;

    path_iterator_t it;
    path_iterator_init(&it, path_);

    bezier_t beziers[32];
    point_t points[3];
    point_t current_point = {0, 0};
    while(path_iterator_has_next(&it)) {
        path_command_t command = path_iterator_next(&it, points);
        switch(command) {
        case NOVA_PATH_COMMAND_MOVE_TO:
        case NOVA_PATH_COMMAND_LINE_TO:
        case NOVA_PATH_COMMAND_CLOSE:
            traverse_func(closure, command, points, 1);
            current_point = points[0];
            break;
        case NOVA_PATH_COMMAND_CUBIC_TO:
            beziers[0].x1 = current_point.x;
            beziers[0].y1 = current_point.y;
            beziers[0].x2 = points[0].x;
            beziers[0].y2 = points[0].y;
            beziers[0].x3 = points[1].x;
            beziers[0].y3 = points[1].y;
            beziers[0].x4 = points[2].x;
            beziers[0].y4 = points[2].y;
            bezier_t* b = beziers;
            while(b >= beziers) {
                float y4y1 = b->y4 - b->y1;
                float x4x1 = b->x4 - b->x1;
                float l = fabsf(x4x1) + fabsf(y4y1);
                float d;
                if(l > 1.f) {
                    d = fabsf((x4x1)*(b->y1 - b->y2) - (y4y1)*(b->x1 - b->x2)) + fabsf((x4x1)*(b->y1 - b->y3) - (y4y1)*(b->x1 - b->x3));
                } else {
                    d = fabsf(b->x1 - b->x2) + fabsf(b->y1 - b->y2) + fabsf(b->x1 - b->x3) + fabsf(b->y1 - b->y3);
                    l = 1.f;
                }

                if(d < threshold*l || b == beziers + 31) {
                    point_t p = { b->x4, b->y4 };
                    traverse_func(closure, NOVA_PATH_COMMAND_LINE_TO, &p, 1);
                    --b;
                } else {
                    split_bezier(b, b + 1, b);
                    ++b;
                }
            }

            current_point = points[2];
            break;
        }
    }
}

static void dash_traverse_func(void* closure, path_command_t command, const point_t* points, int npoints)
{
    dasher_t* dasher = (dasher_t*)(closure);
    if(command == NOVA_PATH_COMMAND_MOVE_TO) {
        if(dasher->start_toggle)
            dasher->traverse_func(dasher->closure, NOVA_PATH_COMMAND_MOVE_TO, points, npoints);
        dasher->current_point = points[0];
        dasher->phase = dasher->start_phase;
        dasher->index = dasher->start_index;
        dasher->toggle = dasher->start_toggle;
        return;
    }

    assert(command == NOVA_PATH_COMMAND_LINE_TO || command == NOVA_PATH_COMMAND_CLOSE);
    point_t p0 = dasher->current_point;
    point_t p1 = points[0];
    float dx = p1.x - p0.x;
    float dy = p1.y - p0.y;
    float dist0 = sqrtf(dx*dx + dy*dy);
    float dist1 = 0.f;
    while(dist0 - dist1 > dasher->dashes[dasher->index % dasher->ndashes] - dasher->phase) {
        dist1 += dasher->dashes[dasher->index % dasher->ndashes] - dasher->phase;
        float a = dist1 / dist0;
        point_t p = { p0.x + a * dx, p0.y + a * dy };
        if(dasher->toggle) {
            dasher->traverse_func(dasher->closure, NOVA_PATH_COMMAND_LINE_TO, &p, 1);
        } else {
            dasher->traverse_func(dasher->closure, NOVA_PATH_COMMAND_MOVE_TO, &p, 1);
        }

        dasher->phase = 0.f;
        dasher->toggle = !dasher->toggle;
        dasher->index++;
    }

    if(dasher->toggle) {
        dasher->traverse_func(dasher->closure, NOVA_PATH_COMMAND_LINE_TO, &p1, 1);
    }

    dasher->phase += dist0 - dist1;
    dasher->current_point = p1;
}

void path_traverse_dashed(const path_t* path_, float offset, const float* dashes, int ndashes, path_traverse_func_t traverse_func, void* closure)
{
    float dash_sum = 0.f;
    for(int i = 0; i < ndashes; ++i)
        dash_sum += dashes[i];
    if(ndashes % 2 == 1)
        dash_sum *= 2.f;
    if(dash_sum <= 0.f) {
        path_traverse(path_, traverse_func, closure);
        return;
    }

    dasher_t dasher;
    dasher.dashes = dashes;
    dasher.ndashes = ndashes;
    dasher.start_phase = fmodf(offset, dash_sum);
    if(dasher.start_phase < 0.f)
        dasher.start_phase += dash_sum;
    dasher.start_index = 0;
    dasher.start_toggle = true;
    while(dasher.start_phase > 0.f && dasher.start_phase >= dasher.dashes[dasher.start_index % dasher.ndashes]) {
        dasher.start_phase -= dashes[dasher.start_index % dasher.ndashes];
        dasher.start_toggle = !dasher.start_toggle;
        dasher.start_index++;
    }

    dasher.phase = dasher.start_phase;
    dasher.index = dasher.start_index;
    dasher.toggle = dasher.start_toggle;
    dasher.current_point = NOVA_EMPTY_POINT;
    dasher.traverse_func = traverse_func;
    dasher.closure = closure;
    path_traverse_flatten(path_, dash_traverse_func, &dasher);
}

path_t* path_clone(const path_t* path_)
{
    path_t* clone = path_create();
    novasvg_array_append(clone->elements, path_->elements);
    clone->start_point = path_->start_point;
    clone->num_points = path_->num_points;
    clone->num_contours = path_->num_contours;
    clone->num_curves = path_->num_curves;
    return clone;
}

static void clone_traverse_func(void* closure, path_command_t command, const point_t* points, int npoints)
{
    path_t* path_ = (path_t*)(closure);
    switch(command) {
    case NOVA_PATH_COMMAND_MOVE_TO:
        path_move_to(path_, points[0].x, points[0].y);
        break;
    case NOVA_PATH_COMMAND_LINE_TO:
        path_line_to(path_, points[0].x, points[0].y);
        break;
    case NOVA_PATH_COMMAND_CUBIC_TO:
        path_cubic_to(path_, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y);
        break;
    case NOVA_PATH_COMMAND_CLOSE:
        path_close(path_);
        break;
    }
}

path_t* path_clone_flatten(const path_t* path_)
{
    path_t* clone = path_create();
    path_reserve(clone, path_->elements.size_ + path_->num_curves * 32);
    path_traverse_flatten(path_, clone_traverse_func, clone);
    return clone;
}

path_t* path_clone_dashed(const path_t* path_, float offset, const float* dashes, int ndashes)
{
    path_t* clone = path_create();
    path_reserve(clone, path_->elements.size_ + path_->num_curves * 32);
    path_traverse_dashed(path_, offset, dashes, ndashes, clone_traverse_func, clone);
    return clone;
}

static void extents_traverse_func(void* closure, path_command_t command, const point_t* points, int npoints)
{
    extents_calculator_t* calculator = (extents_calculator_t*)(closure);
    if(calculator->is_first_point) {
        assert(command == NOVA_PATH_COMMAND_MOVE_TO);
        calculator->is_first_point = false;
        calculator->current_point = points[0];
        calculator->x1 = points[0].x;
        calculator->y1 = points[0].y;
        calculator->x2 = points[0].x;
        calculator->y2 = points[0].y;
        calculator->length_ = 0;
        return;
    }

    for(int i = 0; i < npoints; ++i) {
        calculator->x1 = STB_MIN(calculator->x1, points[i].x);
        calculator->y1 = STB_MIN(calculator->y1, points[i].y);
        calculator->x2 = STB_MAX(calculator->x2, points[i].x);
        calculator->y2 = STB_MAX(calculator->y2, points[i].y);
        if(command != NOVA_PATH_COMMAND_MOVE_TO)
            calculator->length_ += hypotf(points[i].x - calculator->current_point.x, points[i].y - calculator->current_point.y);
        calculator->current_point = points[i];
    }
}

float path_extents(const path_t* path_, rect_t* extents, bool tight)
{
    extents_calculator_t calculator = {{0, 0}, true, 0, 0, 0, 0, 0};
    if(tight) {
        path_traverse_flatten(path_, extents_traverse_func, &calculator);
    } else {
        path_traverse(path_, extents_traverse_func, &calculator);
    }

    if(extents) {
        extents->x = calculator.x1;
        extents->y = calculator.y1;
        extents->w = calculator.x2 - calculator.x1;
        extents->h = calculator.y2 - calculator.y1;
    }

    return calculator.length_;
}

float path_length(const path_t* path_)
{
    return path_extents(path_, NULL, true);
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

bool path_parse(path_t* path_, const char* data, int length_)
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

            path_move_to(path_, values[0], values[1]);
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

            path_line_to(path_, values[0], values[1]);
            current_x = values[0];
            current_y = values[1];
        } else if(command == 'H' || command == 'h') {
            if(!parse_path_coordinates(&it, end, values, 0, 1))
                return false;
            if(command == 'h') {
                values[0] += current_x;
            }

            path_line_to(path_, values[0], current_y);
            current_x = values[0];
        } else if(command == 'V' || command == 'v') {
            if(!parse_path_coordinates(&it, end, values, 1, 1))
                return false;
            if(command == 'v') {
                values[1] += current_y;
            }

            path_line_to(path_, current_x, values[1]);
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

            path_quad_to(path_, values[0], values[1], values[2], values[3]);
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

            path_cubic_to(path_, values[0], values[1], values[2], values[3], values[4], values[5]);
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

            path_quad_to(path_, values[0], values[1], values[2], values[3]);
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

            path_cubic_to(path_, values[0], values[1], values[2], values[3], values[4], values[5]);
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

            path_arc_to(path_, values[0], values[1], NOVA_DEG2RAD(values[2]), flags[0], flags[1], values[3], values[4]);
            current_x = values[3];
            current_y = values[4];
        } else if(command == 'Z' || command == 'z') {
            if(last_command == 'Z' || last_command == 'z')
                return false;
            path_close(path_);
            current_x = start_x;
            current_y = start_y;
        } else {
            return false;
        }

        last_command = command;
    }

    return true;
}

} // namespace vg
} // namespace nova
