#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "list.h"
#include "vertex.h"

typedef struct {
    Vector2 a;
    Vector2 b;
    Vector2 c;
} Triangle;

typedef struct {
    Vertex a;
    Vertex b;
    Vertex c;
} TriangleVertices;

typedef struct {
    Vector2 a;
    Vector2 b;
} TriangleFanElement;

typedef struct {
    Vector2 center;

    TriangleFanElement *items;
    ssize count;
} TriangleFan;

// https://stackoverflow.com/a/2049593
static inline b32 triangle_contains_point(Triangle triangle, Vector2 point)
{
    f32 s1 = v2_half_plane_side(point, triangle.a, triangle.b);
    f32 s2 = v2_half_plane_side(point, triangle.b, triangle.c);
    f32 s3 = v2_half_plane_side(point, triangle.c, triangle.a);

    b32 has_negative = (s1 < 0) || (s2 < 0) || (s3 < 0);
    b32 has_positive = (s1 > 0) || (s2 > 0) || (s3 > 0);

    b32 result = !(has_negative && has_positive);

    return result;
}

static inline TriangleVertices triangle_get_vertices(Triangle triangle, RGBA32 color)
{
    TriangleVertices result = {0};

    result.a = (Vertex){triangle.a, V2_ZERO, color};
    result.b = (Vertex){triangle.b, V2_ZERO, color};
    result.c = (Vertex){triangle.c, V2_ZERO, color};

    return result;
}

#endif //TRIANGLE_H
