#ifndef RECTANGLE_H
#define RECTANGLE_H

#include "base/vector.h"
#include "base/vertex.h"

typedef struct {
    Vector2 position;
    Vector2 size;
} Rectangle;

typedef struct {
    Vertex top_left;
    Vertex top_right;
    Vertex bottom_right;
    Vertex bottom_left;
} RectangleVertices;

static inline Vector2 rect_top_left(Rectangle rect)
{
    Vector2 result = rect.position;

    return result;
}

static inline Vector2 rect_top_right(Rectangle rect)
{
    Vector2 result = v2_add(rect.position, (Vector2){rect.size.x, 0});

    return result;
}

static inline Vector2 rect_bottom_right(Rectangle rect)
{
    Vector2 result = v2_add(rect.position, (Vector2){rect.size.x, rect.size.y});

    return result;
}

static inline Vector2 rect_bottom_left(Rectangle rect)
{
    Vector2 result = v2_add(rect.position, (Vector2){0, rect.size.y});

    return result;
}

static inline RectangleVertices rect_get_vertices(Rectangle rect, RGBA32 color)
{
    Vertex tl = {
        .position = rect_top_left(rect),
        .color = color,
        .uv = {0, 1}
    };

    Vertex tr = {
        .position = rect_top_right(rect),
        .color = color,
        .uv = {1, 1}
    };

    Vertex br = {
        .position = rect_bottom_right(rect),
        .color = color,
        .uv = {1, 0}
    };

    Vertex bl = {
        .position = rect_bottom_left(rect),
        .color = color,
        .uv = {0, 0}
    };

    RectangleVertices result = {
        .top_left = tl,
        .top_right = tr,
        .bottom_right = br,
        .bottom_left = bl
    };

    return result;
}

static inline Vector2 rect_bounds_point_closest_to_point(Rectangle rect, Vector2 point)
{
    f32 min_x = rect.position.x;
    f32 max_x = min_x + rect.size.x;
    f32 min_y = rect.position.y;
    f32 max_y = min_y + rect.size.y;

    f32 min_dist = abs_f32(point.x - min_x);
    Vector2 bounds_point = { min_x, point.y };

    if (abs_f32(max_x - point.x) < min_dist) {
        min_dist = abs_f32(max_x - point.x);
        bounds_point = (Vector2){ max_x, point.y };
    }

    if (abs_f32(max_y - point.y) < min_dist) {
        min_dist = abs_f32(max_y - point.y);
        bounds_point = (Vector2){ point.x, max_y };
    }

    if (abs_f32(min_y - point.y) < min_dist) {
        min_dist = abs_f32(min_y - point.y);
        bounds_point = (Vector2){point.x, min_y};
    }

    return bounds_point;
}

#endif //RECTANGLE_H
