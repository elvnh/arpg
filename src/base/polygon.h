#ifndef POLYGON_H
#define POLYGON_H

#include "vector.h"
#include "triangle.h"

struct LinearArena;

typedef enum {
    WINDING_ORDER_CLOCKWISE,
    WINDING_ORDER_COUNTER_CLOCKWISE,
} PolygonWindingOrder;

typedef struct PolygonVertex {
    Vector2 point;
    struct PolygonVertex *next;
    struct PolygonVertex *prev;
} PolygonVertex;

typedef struct {
    // TODO: use array instead
    PolygonVertex *head;
    PolygonVertex *tail;
} Polygon;

typedef struct PolygonTriangle {
    Triangle triangle;
    struct PolygonTriangle *next;
} PolygonTriangle;

typedef struct {
    // TODO: use array instead
    PolygonTriangle *head;
    PolygonTriangle *tail;
} TriangulatedPolygon;

TriangulatedPolygon triangulate_polygon(Polygon *polygon, Vector2 center, struct LinearArena *arena);
PolygonWindingOrder polygon_winding_order(Polygon polygon);
Vector2 polygon_center(Polygon polygon);

void remove_colinear_vertices(Polygon *polygon);
void sort_polygon_vertices(Polygon *polygon, Vector2 center, struct LinearArena *arena);

#endif //POLYGON_H
