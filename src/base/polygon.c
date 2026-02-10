#define _GNU_SOURCE
#include <stdlib.h>
// TODO: don't use qsort_r

#include "base/utils.h"
#include "polygon.h"
#include "base/list.h"
#include "sl_list.h"
#include "triangle.h"
#include "linear_arena.h"
#include "vector.h"

static b32 angle_is_convex(Vector2 middle, Vector2 prev, Vector2 next)
{
    b32 result = v2_half_plane_side(middle, prev, next) >= 0.0f;

    return result;
}

static ssize polygon_size(Polygon *polygon)
{
    ssize result = 0;
    for (PolygonVertex *curr = list_head(polygon); curr; curr = list_next(curr)) {
        ++result;
    }

    return result;
}

static b32 triangle_is_ear(Polygon *polygon, PolygonVertex *a, PolygonVertex *b, PolygonVertex *c)
{
    Triangle potential_triangle = {a->point, b->point, c->point};

    if (!angle_is_convex(a->point, b->point, c->point)) {
        return false;
    }

    b32 contains_other_vertex = false;

    for (PolygonVertex *other = list_head(polygon); other; other = list_next(other)) {
        if ((other != a) && (other != b) && (other != c)) {
            if (triangle_contains_point(potential_triangle, other->point)) {
                contains_other_vertex = true;
                break;
            }
        }
    }

    return !contains_other_vertex;
}

void remove_colinear_vertices(Polygon *polygon)
{
    for (PolygonVertex *curr = list_head(polygon); curr;) {
        PolygonVertex *prev = list_circular_prev(polygon, curr);
        PolygonVertex *next = list_circular_next(polygon, curr);
        PolygonVertex *next_to_visit = list_next(curr);

        f32 x = v2_half_plane_side(curr->point, prev->point, next->point);

        if (abs_f32(x) <= 0.1f) {
            list_remove(polygon, curr);
        }

        curr = next_to_visit;
    }
}

static int sort_polygon_cmp(const void *a, const void *b, void *data)
{
    Vector2 center = *(Vector2 *)data;

    Vector2 pa = *(Vector2 *)a;
    Vector2 pb = *(Vector2 *)b;

    f32 angle_a = (f32)fmodf(rad_to_deg(atan2f(pa.x - center.x, pa.y - center.y)) + 360, 360);
    f32 angle_b = (f32)fmodf(rad_to_deg(atan2f(pb.x - center.x, pb.y - center.y)) + 360, 360);

    int result = (int)(angle_a - angle_b);
    return result;
}

void sort_polygon_vertices(Polygon *polygon, Vector2 center, LinearArena *arena)
{
    ssize point_count = polygon_size(polygon);
    ASSERT(point_count > 0);
    //Vector2 center = polygon_center(*polygon);

    Vector2 *verts = la_allocate_array(arena, Vector2, point_count);

    ssize index = 0;
    for (PolygonVertex *curr = list_head(polygon); curr; curr = list_next(curr)) {
        verts[index++] = curr->point;
    }

    list_clear(polygon);

    qsort_r(verts, ssize_to_usize(point_count), sizeof(*verts), sort_polygon_cmp, &center);

    for (ssize i = 0; i < point_count; ++i) {
        PolygonVertex *vert = la_allocate_item(arena, PolygonVertex);
        vert->point = verts[i];
        list_push_back(polygon, vert);
    }
}

TriangulatedPolygon triangulate_polygon(Polygon *polygon, Vector2 center, LinearArena *arena)
{
    sort_polygon_vertices(polygon, center, arena);

    remove_colinear_vertices(polygon);

    // TODO: don't modify polygon in place
    ASSERT(polygon_size(polygon) >= 3);
    ASSERT(polygon_winding_order(*polygon) == WINDING_ORDER_CLOCKWISE);


    ASSERT(polygon_size(polygon) >= 3);

    TriangulatedPolygon result = {0};

    while (polygon_size(polygon) > 3) {
        for (PolygonVertex *curr = list_head(polygon); curr; curr = list_next(curr)) {
            PolygonVertex *prev = list_circular_prev(polygon, curr);
            PolygonVertex *next = list_circular_next(polygon, curr);

            if (triangle_is_ear(polygon, prev, curr, next)) {
                PolygonTriangle *node = la_allocate_item(arena, PolygonTriangle);
                Triangle triangle = {prev->point, curr->point, next->point};
                node->triangle = triangle;

                sl_list_push_back(&result, node);

                list_remove(polygon, curr);

                break;
            }
        }
    }

    ASSERT(polygon_size(polygon) == 3);

    Triangle final_triangle = {
        list_head(polygon)->point,
        list_head(polygon)->next->point,
        list_tail(polygon)->point
    };

    PolygonTriangle *final_node = la_allocate_item(arena, PolygonTriangle);
    final_node->triangle = final_triangle;

    sl_list_push_back(&result, final_node);

    list_clear(polygon);

    return result;
}

PolygonWindingOrder polygon_winding_order(Polygon polygon)
{
    f32 sum = 0.0f;

    ASSERT(polygon_size(&polygon) > 3);

    for (PolygonVertex *vert = list_head(&polygon); vert; vert = list_next(vert)) {
        PolygonVertex *next = list_circular_next(&polygon, vert);

        sum += (next->point.x - vert->point.x) * (next->point.y + vert->point.y);
    }

    PolygonWindingOrder result =
        (sum > 0)
        ? WINDING_ORDER_CLOCKWISE
        : WINDING_ORDER_COUNTER_CLOCKWISE;

    return result;
}

Vector2 polygon_center(Polygon polygon)
{
    ssize size = polygon_size(&polygon);

    f32 x = 0.0f;
    f32 y = 0.0f;

    for (PolygonVertex *curr = list_head(&polygon); curr; curr = list_next(curr)) {
        x += curr->point.x;
        y += curr->point.y;
    }

    x /= (f32)size;
    y /= (f32)size;

    Vector2 result = {x, y};
    return result;
}
