#ifndef VECTOR2_H
#define VECTOR2_H

#include "maths.h"

#define V2_ZERO ((Vector2) {0})

typedef struct {
    f32 x;
    f32 y;
} Vector2;

typedef struct {
    f32 x;
    f32 y;
    f32 z;
    f32 w;
} Vector4;

static inline Vector2 v2_add(Vector2 lhs, Vector2 rhs)
{
    Vector2 result = {
        .x = lhs.x + rhs.x,
        .y = lhs.y + rhs.y
    };

    return result;
}

static inline Vector2 v2_sub(Vector2 lhs, Vector2 rhs)
{
    Vector2 result = {
        .x = lhs.x - rhs.x,
        .y = lhs.y - rhs.y
    };

    return result;
}

static inline Vector2 v2_div(Vector2 lhs, f32 scalar)
{
    Vector2 result = {
        .x = lhs.x / scalar,
        .y = lhs.y / scalar
    };

    return result;
}


static inline Vector2 v2_neg(Vector2 v)
{
    Vector2 result = {
        .x = -v.x,
        .y = -v.y
    };

    return result;
}

static inline f32 v2_mag(Vector2 v)
{
    f32 result = (f32)sqrt(v.x * v.x + v.y * v.y);

    return result;
}

static inline f32 v2_dist(Vector2 a, Vector2 b)
{
    f32 sq = ((b.x - a.x) * (b.x - a.x)) + ((b.y - a.y) * (b.y - a.y));
    f32 result = (f32)sqrt(sq);

    return result;
}

#endif //VECTOR2_H
