#ifndef RECTANGLE_H
#define RECTANGLE_H

#include "base/vector.h"
#include "base/vertex.h"
#include "base/line.h"

typedef struct {
    Vector2 position;
    Vector2 size;
} Rectangle;

typedef enum {
    RECT_SIDE_TOP,
    RECT_SIDE_RIGHT,
    RECT_SIDE_BOTTOM,
    RECT_SIDE_LEFT,
} RectangleSide;

typedef struct {
    b32 is_intersecting;
    f32 time_of_impact;
    RectangleSide side_of_collision;
} RectRayIntersection;

typedef struct {
    Vertex top_left;
    Vertex top_right;
    Vertex bottom_right;
    Vertex bottom_left;
} RectangleVertices;

typedef struct {
    Rectangle top_left;
    Rectangle top_right;
    Rectangle bottom_right;
    Rectangle bottom_left;
} RectangleQuadrants;

static inline Vector2 rect_top_left(Rectangle rect)
{
    Vector2 result = v2_add(rect.position, v2(0.0f, rect.size.y));

    return result;
}

static inline Vector2 rect_top_right(Rectangle rect)
{
    Vector2 result = v2_add(rect.position, rect.size);

    return result;
}

static inline Vector2 rect_bottom_right(Rectangle rect)
{
    Vector2 result = v2_add(rect.position, v2(rect.size.x, 0.0f));

    return result;
}

static inline Vector2 rect_bottom_left(Rectangle rect)
{
    Vector2 result = rect.position;

    return result;
}

static inline RectangleVertices rect_get_vertices(Rectangle rect, RGBA32 color)
{
    Vertex tl = {
        .position = rect_top_left(rect),
        .color = color,
        .uv = {0, 0}
    };

    Vertex tr = {
        .position = rect_top_right(rect),
        .color = color,
        .uv = {1, 0}
    };

    Vertex br = {
        .position = rect_bottom_right(rect),
        .color = color,
        .uv = {1, 1}
    };

    Vertex bl = {
        .position = rect_bottom_left(rect),
        .color = color,
        .uv = {0, 1}
    };

    RectangleVertices result = {
        .top_left = tl,
        .top_right = tr,
        .bottom_right = br,
        .bottom_left = bl
    };

    return result;
}

typedef struct {
    Vector2 point;
    RectangleSide side;
} RectanglePoint;

static inline RectanglePoint rect_bounds_point_closest_to_point(Rectangle rect, Vector2 point)
{
    f32 min_x = rect.position.x;
    f32 max_x = min_x + rect.size.x;
    f32 min_y = rect.position.y;
    f32 max_y = min_y + rect.size.y;

    RectangleSide side;
    Vector2 bounds_point = { min_x, point.y };

    f32 min_dist  = INFINITY;
    f32 x = 0;

    x = abs_f32(point.x - min_x);
    if (x < min_dist) {
        min_dist = x;
        bounds_point = (Vector2){min_x, point.y};
        side = RECT_SIDE_LEFT;
    }

    x = abs_f32(max_x - point.x);
    if (x < min_dist) {
        min_dist = x;
        bounds_point = (Vector2){ max_x, point.y };
        side = RECT_SIDE_RIGHT;
    }

    x = abs_f32(max_y - point.y);
    if (x < min_dist) {
        min_dist = x;
        bounds_point = (Vector2){ point.x, max_y };
        side = RECT_SIDE_TOP;
    }

    x = abs_f32(min_y - point.y);
    if (x < min_dist) {
        min_dist = x;
        bounds_point = (Vector2){point.x, min_y};
        side = RECT_SIDE_BOTTOM;
    }

    RectanglePoint result = {
        .point = bounds_point,
        .side = side
    };

    return result;
}

static inline b32 rect_contains_point(Rectangle rect, Vector2 p)
{
    b32 result = (rect.position.x <= p.x) && (rect.position.x + rect.size.x >= p.x)
        && (rect.position.y <= p.y) && (rect.position.y + rect.size.y >= p.y);

    return result;
}

static inline Rectangle rect_move_to(Rectangle rect, Vector2 v)
{
    rect.position = v;

    return rect;
}

static inline b32 rect_intersects(Rectangle a, Rectangle b)
{
    b32 result =
            (a.position.x < (b.position.x + b.size.x))
        && ((a.position.x + a.size.x) > b.position.x)
        && (a.position.y < (b.position.y + b.size.y))
        && ((a.position.y + a.size.y) > b.position.y);

    return result;
}

static inline Rectangle rect_minkowski_diff(Rectangle a, Rectangle b)
{
    Vector2 size = v2_add(a.size, b.size);
    Vector2 left = v2_sub(a.position, rect_top_right(b));
    Vector2 pos = { left.x, left.y };

    Rectangle result = { pos, size };

    return result;
}

static inline RectRayIntersection rect_shortest_ray_intersection(Rectangle rect, Line ray_line,
    f32 epsilon)
{
    Line left = {
        rect_bottom_left(rect),
        rect_top_left(rect)
    };

    Line top = {
        rect_top_left(rect),
        rect_top_right(rect)
    };

    Line right = {
        rect_top_right(rect),
        rect_bottom_right(rect)
    };

    Line bottom = {
        rect_bottom_right(rect),
        rect_bottom_left(rect)
    };

    RectangleSide side_of_collision = 0;
    f32 min_fraction = INFINITY;
    f32 x = min_fraction;

    x = line_intersection_fraction(ray_line, left, epsilon);

    if (x < min_fraction) {
        min_fraction = x;
        side_of_collision = RECT_SIDE_LEFT;
    }

    x = MIN(min_fraction, line_intersection_fraction(ray_line, top, epsilon));

    if (x < min_fraction) {
        min_fraction = x;
        side_of_collision = RECT_SIDE_TOP;
    }

    x = MIN(min_fraction, line_intersection_fraction(ray_line, right, epsilon));

    if (x < min_fraction) {
        min_fraction = x;
        side_of_collision = RECT_SIDE_RIGHT;
    }

    x = MIN(min_fraction, line_intersection_fraction(ray_line, bottom, epsilon));

    if (x < min_fraction) {
        min_fraction = x;
        side_of_collision = RECT_SIDE_BOTTOM;
    }

    RectRayIntersection result = {0};

    if (min_fraction != INFINITY) {
        result.is_intersecting = true;
        result.side_of_collision = side_of_collision;
        result.time_of_impact = min_fraction;
    }

    return result;
}

static inline b32 rect_contains_rect(Rectangle a, Rectangle b)
{
    f32 a_left = a.position.x;
    f32 b_left = b.position.x;
    f32 a_right = a.position.x + a.size.x;
    f32 b_right = b.position.x + b.size.x;

    f32 a_bottom = a.position.y;
    f32 b_bottom = b.position.y;
    f32 a_top = a.position.y + a.size.y;
    f32 b_top = b.position.y + b.size.y;

    b32 result = a_left <= b_left
	&& a_right >= b_right
	&& a_bottom <= b_bottom
	&& a_top >= b_top;

    return result;
}

static inline RectangleQuadrants rect_quadrants(Rectangle rect)
{
    f32 half_width = rect.size.x / 2.0f;
    f32 half_height = rect.size.y / 2.0f;
    Vector2 size = v2(half_width, half_height);

    Rectangle top_left = {
	.position = v2(rect.position.x, rect.position.y + half_height),
	.size = size
    };

    Rectangle top_right = {
	.position = v2(rect.position.x + half_width, rect.position.y + half_height),
	.size = size
    };

    Rectangle bottom_right = {
	.position = v2(rect.position.x + half_width, rect.position.y),
	.size = size
    };

    Rectangle bottom_left = {
	.position = v2(rect.position.x, rect.position.y),
	.size = size
    };

    RectangleQuadrants result = {
	.top_left = top_left,
	.top_right = top_right,
	.bottom_right = bottom_right,
	.bottom_left = bottom_left
    };

    return result;
}

#endif //RECTANGLE_H
