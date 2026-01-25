#ifndef VECTOR2_H
#define VECTOR2_H

#include "utils.h"
#include "maths.h"

#define V2_ZERO ((Vector2) {0})

typedef struct {
    f32 x;
    f32 y;
} Vector2;

typedef struct {
    s32 x;
    s32 y;
} Vector2i;

typedef struct {
    f32 x;
    f32 y;
    f32 z;
    f32 w;
} Vector4;

static inline Vector2 v2(f32 x, f32 y)
{
    Vector2 result = {x, y};
    return result;
}

static inline Vector2 v2_add(Vector2 lhs, Vector2 rhs)
{
    Vector2 result = { lhs.x + rhs.x, lhs.y + rhs.y };

    return result;
}

static inline Vector2 v2_sub(Vector2 lhs, Vector2 rhs)
{
    Vector2 result = { lhs.x - rhs.x, lhs.y - rhs.y };

    return result;
}

static inline Vector2 v2_mul_s(Vector2 lhs, f32 scalar)
{
    Vector2 result = { lhs.x * scalar, lhs.y * scalar};

    return result;
}

static inline Vector2 v2_div_s(Vector2 lhs, f32 scalar)
{
    Vector2 result = {lhs.x / scalar, lhs.y / scalar};

    return result;
}

static inline Vector2 v2_neg(Vector2 v)
{
    Vector2 result = { -v.x, -v.y };

    return result;
}

static inline f32 v2_mag(Vector2 v)
{
    f32 result = sqrt_f32(v.x * v.x + v.y * v.y);

    return result;
}

static inline f32 v2_dist_sq(Vector2 a, Vector2 b)
{
    f32 result = ((b.x - a.x) * (b.x - a.x)) + ((b.y - a.y) * (b.y - a.y));

    return result;
}

static inline f32 v2_dist(Vector2 a, Vector2 b)
{
    f32 sq = v2_dist_sq(a, b);
    f32 result = sqrt_f32(sq);

    return result;
}

static inline f32 v2_cross(Vector2 a, Vector2 b)
{
    f32 result = a.x * b.y - a.y * b.x;

    return result;
}

static inline f32 v2_dot(Vector2 a, Vector2 b)
{
    f32 result = a.x * b.x + a.y * b.y;

    return result;
}

static inline Vector2 v2_norm(Vector2 v)
{
    f32 mag = v2_mag(v);

    if (mag == 0.0f) {
        return V2_ZERO;
    }

    Vector2 result = v2_div_s(v, mag);
    return result;
}

static inline b32 v2_eq(Vector2 a, Vector2 b)
{
    b32 result = (a.x == b.x) && (a.y == b.y);

    return result;
}

static inline b32 v2_is_zero(Vector2 v)
{
    b32 result = v2_eq(v, V2_ZERO);

    return result;
}

static inline b32 v2i_eq(Vector2i a, Vector2i b)
{
    b32 result = (a.x == b.x) && (a.y == b.y);

    return result;
}

static inline Vector2 v2_interpolate(Vector2 a, Vector2 b, f32 t)
{
    f32 new_x = interpolate_sin(a.x, b.x, t);
    f32 new_y = interpolate_sin(a.y, b.y, t);

    Vector2 result = v2(new_x, new_y);
    return result;
}

static inline Vector2 v2_reflect(Vector2 v, Vector2 normal)
{
    ASSERT(v2_mag(normal) == 1.0f);
    Vector2 result = v2_sub(v, v2_mul_s(normal, 2.0f * v2_dot(v, normal)));

    return result;
}

typedef enum {
    AXIS_HORIZONTAL,
    AXIS_VERTICAL,
    AXIS_COUNT,
} Axis;

static inline f32 *v2_index(Vector2 *v, Axis index)
{
    ASSERT((index == AXIS_HORIZONTAL) || (index == AXIS_VERTICAL));

    f32 *result = 0;
    if (index == AXIS_HORIZONTAL) {
        result = &v->x;
    } else {
        result = &v->y;
    }

    return result;
}

static inline Vector2 v2i_to_v2(Vector2i from)
{
    Vector2 result = {(f32)from.x, (f32)from.y};
    return result;
}


static inline Vector2 v2_rotate_around_point(Vector2 v, f32 rotation_in_radians, Vector2 origin)
{
    f32 s = sin_f32(rotation_in_radians);
    f32 c = cos_f32(rotation_in_radians);

    Vector2 result = {
        (v.x - origin.x) * c - (v.y - origin.y) * s + origin.x,
        (v.x - origin.x) * s + (v.y - origin.y) * c + origin.y
    };

    return result;
}

static inline f32 v2_half_plane_side(Vector2 point, Vector2 a, Vector2 b)
{
    Vector2 pb = v2_sub(a, b);
    Vector2 pc = v2_sub(point, b);

    f32 result = v2_cross(pb, pc);

    return result;
}

#endif //VECTOR2_H
