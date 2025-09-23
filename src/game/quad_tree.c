#include "quad_tree.h"
#include "entity.h"

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

void qt_initialize(QuadTree *qt, Rectangle area)
{
    qt->area = area;
}

/* QuadTreeLocation qt_move_entity(QuadTree *qt, EntityID id, QuadTreeLocation location, Vector2 new_position, LinearArena *arena) */
/* { */

/* } */

static inline b32 qt_location_is_null(QuadTreeLocation location)
{
    b32 result = location.node == 0;

    return result;
}

static QuadTreeLocation qt_insert(QuadTree *qt, EntityID id, Rectangle area, ssize depth, LinearArena *arena)
{
    // TODO: clean up this function
    ASSERT(area.size.x > 0);
    ASSERT(area.size.y > 0);
    ASSERT(rect_contains_rect(qt->area, area));

    b32 no_children = !qt->top_left;
    RectangleQuadrants quads = rect_quadrants(qt->area);

    QuadTreeLocation result = {0};

    if (depth < QUAD_TREE_MAX_DEPTH - 1) {
	if (rect_contains_rect(quads.top_left, area)) {
	    if (no_children) {
		qt_subdivide(qt, arena);
	    }

	    result = qt_insert(qt->top_left, id, area, depth + 1, arena);
	} else if (rect_contains_rect(quads.top_right, area)) {
	    if (no_children) {
		qt_subdivide(qt, arena);
	    }

	    result = qt_insert(qt->top_right, id, area, depth + 1, arena);
	} else if (rect_contains_rect(quads.bottom_right, area)) {
	    if (no_children) {
		qt_subdivide(qt, arena);
	    }

	    result = qt_insert(qt->bottom_right, id, area, depth + 1, arena);
	} else if (rect_contains_rect(quads.bottom_left, area)) {
	    if (no_children) {
		qt_subdivide(qt, arena);
	    }

	    result = qt_insert(qt->bottom_left, id, area, depth + 1, arena);
	}
    }

    if (qt_location_is_null(result)) {
        ssize index = qt->entity_count++;
        qt->entities[index] = id;

        result.node = qt;
        result.array_index = index;
    }

    return result;
}

QuadTreeLocation qt_set_entity_area(QuadTree *qt, struct EntityStorage *es, EntityID id,
    QuadTreeLocation location, Rectangle area, LinearArena *arena)
{
    if (!qt_location_is_null(location)) {
        qt_remove_entity(qt, es, id, location);
    }

    QuadTreeLocation result = qt_insert(qt, id, area, 0, arena);

    return result;
}

void qt_remove_entity(QuadTree *qt, struct EntityStorage *es, EntityID id, QuadTreeLocation location)
{
    ASSERT(!qt_location_is_null(location));

    ssize last_index = location.node->entity_count;
    EntityID *to_remove = &location.node->entities[location.array_index];
    // TODO: ID comparison function
    ASSERT(to_remove->slot_index == id.slot_index);
    ASSERT(to_remove->generation == id.generation);

    EntityID *last = &location.node->entities[last_index];
    *to_remove = *last;

    // TODO: don't do it this way
    // Update swapped entity's array index
    es->entity_slots[last->slot_index].quad_tree_location.array_index = location.array_index;

    --qt->entity_count;
}
