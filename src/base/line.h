#ifndef LINE_H
#define LINE_H

#include "vector.h"
#include "utils.h"

typedef struct {
    Vector2 start;
    Vector2 end;
} Line;

typedef struct {
    bool are_intersecting;
    Vector2 intersection_point;
} LineIntersection;

// Taken from: https://blog.hamaluik.ca/posts/swept-aabb-collision-using-minkowski-difference/
static inline LineIntersection line_intersection(Line a, Line b, f32 epsilon)
{
    LineIntersection result = {0};

    Vector2 r = v2_sub(a.end, a.start);
    Vector2 s = v2_sub(b.end, b.start);

    f32 numerator = v2_cross(v2_sub(b.start, a.start), r);
    f32 denominator = v2_cross(r, s);

    if ((numerator == 0.0f) && (denominator == 0.0f)) {
        // Lines are colinear so they might still overlap
        // TODO: check if they overlap
    } else if ((denominator != 0.0f) && (abs_f32(denominator) > epsilon)) {
        // NOTE: this should guard against infinity being
        f32 t = v2_cross(v2_sub(b.start, a.start), s) / denominator;
        f32 u = numerator / denominator;

        if (f32_in_range(u, 0.0f, 1.0f, epsilon) && f32_in_range(t, 0.0f, 1.0f, epsilon)) {
            Vector2 intersection = {
                a.start.x + r.x * t,
                b.start.y + s.y * u,
            };

            result.are_intersecting = true;
            result.intersection_point = intersection;
        }
    }

    return result;
}

// Returns how far along A the intersection point between A and B lies
static inline f32 line_intersection_fraction(Line a, Line b, f32 epsilon)
{
    LineIntersection intersection = line_intersection(a, b, epsilon);

    if (intersection.are_intersecting) {
        f32 ray_length = v2_dist_sq(a.start, a.end);
        f32 intersect_point_dist = v2_dist_sq(intersection.intersection_point, a.start);
        f32 fraction = intersect_point_dist / ray_length;

        return fraction;
    }

    return INFINITY;
}

// https://ncase.me/sight-and-light/
// https://github.com/OneLoneCoder/Javidx9/blob/master/PixelGameEngine/SmallerProjects/OneLoneCoder_PGE_ShadowCasting2D.cpp
static inline LineIntersection ray_vs_line_intersection(Vector2 ray_origin, Vector2 ray_dir, Line line)
{
    LineIntersection result = {0};

    Vector2 line_dir = v2_sub(line.end, line.start);

    // If the directions are parallel, there can be no intersection
    if (v2_cross(line_dir, ray_dir) != 0.0f) {
        ray_dir = v2_norm(ray_dir);

        f32 line_sx = line.start.x;
        f32 line_sy = line.start.y;
        f32 line_dx = line_dir.x;
        f32 line_dy = line_dir.y;

        f32 ray_ox = ray_origin.x;
        f32 ray_oy = ray_origin.y;
        f32 ray_dx = ray_dir.x;
        f32 ray_dy = ray_dir.y;

        f32 t2 = (ray_dx * (line_sy - ray_oy) + ray_dy * (ray_ox - line_sx))
            / (line_dx * ray_dy - line_dy * ray_dx);
        f32 t1 = (line_sx + line_dx * t2 - ray_ox) / ray_dx;


        b32 intersecting = (t1 > 0.0f) && (t2 > 0.0f) && (t2 < 1.0f);

        if (intersecting) {
            result.are_intersecting = true;
            result.intersection_point = v2_add(ray_origin, v2_mul_s(ray_dir, t1));
        }
    }

    return result;
}

static inline Vector2 line_center(Line line)
{
    Vector2 line_dir = v2_norm(v2_sub(line.end, line.start));
    f32 length = v2_dist(line.start, line.end);

    Vector2 result = v2_add(line.start, v2_mul_s(line_dir, length / 2.0f));

    return result;
}

#endif //LINE_H
