#ifndef LINE_H
#define LINE_H

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

#endif //LINE_H
