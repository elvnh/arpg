#ifndef QUAD_TREE_H
#define QUAD_TREE_H

#include "base/linear_arena.h"
#include "base/rectangle.h"
#include "base/utils.h"
#include "game/entity.h"

#define QUAD_TREE_MAX_DEPTH 4

/*
  TODO:
  - Make sure that subdivided areas aren't too small
  - Make entityID arrays be dynamic
  - How to update entity locations on removing entity?
    - Bidirectional pointers? Element has pointer back to quad tree
      location?
    - Entity ID already acts as a pointer, pass in EntityStorage when removing/inserting
      an entity, will look up any entities that move array index and correct them
    - Or use a linked list instead?
  - Move children into one struct, allocate all at once
 */

typedef struct QuadTree {
    Rectangle area;

    struct QuadTree *top_left;
    struct QuadTree *top_right;
    struct QuadTree *bottom_right;
    struct QuadTree *bottom_left;

    EntityID entities[MAX_ENTITIES];
    ssize  entity_count;
} QuadTree;

// Store in EntitySlot
// Return when inserting entity
typedef struct {
    QuadTree *node;
    ssize array_index;
} QuadTreeLocation;

static inline void qt_initialize(QuadTree *qt, Rectangle area)
{
    qt->area = area;
}

static inline void qt_subdivide(QuadTree *qt, LinearArena *arena)
{
    ASSERT(!qt->top_left);
    ASSERT(!qt->top_right);
    ASSERT(!qt->bottom_right);
    ASSERT(!qt->bottom_left);

    qt->top_left = la_allocate_item(arena, QuadTree);
    qt->top_right = la_allocate_item(arena, QuadTree);
    qt->bottom_right = la_allocate_item(arena, QuadTree);
    qt->bottom_left = la_allocate_item(arena, QuadTree);

    RectangleQuadrants quads = rect_quadrants(qt->area);

    qt_initialize(qt->top_left, quads.top_left);
    qt_initialize(qt->top_right, quads.top_right);
    qt_initialize(qt->bottom_right, quads.bottom_right);
    qt_initialize(qt->bottom_left, quads.bottom_left);
}

static inline void qt_insert(QuadTree *qt, Rectangle area, ssize depth, LinearArena *arena)
{
    ASSERT(rect_contains_rect(qt->area, area));

    b32 no_children = !qt->top_left;
    RectangleQuadrants quads = rect_quadrants(qt->area);
    b32 found = false;

    if (depth < QUAD_TREE_MAX_DEPTH - 1) {
	if (rect_contains_rect(quads.top_left, area)) {
	    if (no_children) {
		qt_subdivide(qt, arena);
	    }

	    qt_insert(qt->top_left, area, depth + 1, arena);
	} else if (rect_contains_rect(quads.top_right, area)) {
	    if (no_children) {
		qt_subdivide(qt, arena);
	    }

	    qt_insert(qt->top_right, area, depth + 1, arena);
	} else if (rect_contains_rect(quads.bottom_right, area)) {
	    if (no_children) {
		qt_subdivide(qt, arena);
	    }

	    qt_insert(qt->bottom_right, area, depth + 1, arena);
	} else if (rect_contains_rect(quads.bottom_left, area)) {
	    if (no_children) {
		qt_subdivide(qt, arena);
	    }

	    qt_insert(qt->bottom_left, area, depth + 1, arena);
	} else {
	    found = true;
	}
    } else {
	found = true;
    }

    if (found) {
	// insert here
    }
}

#endif //QUAD_TREE_H
