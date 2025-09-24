#include "quad_tree.h"
#include "base/list.h"
#include "base/utils.h"
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

    // TODO: is this assert needed?
    //ASSERT(rect_contains_rect(qt->area, area));

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
        // TODO: free list to reuse nodes
        QuadTreeElement *element = la_allocate_item(arena, QuadTreeElement);
        element->entity_id = id;
        element->area = area;

        // TODO: function that returns pushed element
        list_push_back(&qt->entities_in_node, element);

        result.node = qt;
        result.element = element;
    }

    return result;
}

QuadTreeLocation qt_move_entity(QuadTree *qt, struct EntityStorage *es, EntityID id,
    QuadTreeLocation location, Vector2 new_position, LinearArena *arena)
{
    ASSERT(!qt_location_is_null(location));
    Rectangle new_area = {new_position, location.element->area.size};

    QuadTreeLocation result = qt_set_entity_area(qt, es, id, location, new_area, arena);

    return result;
}

QuadTreeLocation qt_set_entity_area(QuadTree *qt, struct EntityStorage *es, EntityID id,
    QuadTreeLocation location, Rectangle area, LinearArena *arena)
{
    if (!qt_location_is_null(location)) {
        qt_remove_entity(es, id, location);
    }

    QuadTreeLocation result = qt_insert(qt, id, area, 0, arena);

    return result;
}

void qt_remove_entity(struct EntityStorage *es, EntityID id, QuadTreeLocation location)
{
    ASSERT(!qt_location_is_null(location));
    ASSERT(location.element->entity_id.slot_index == id.slot_index);

    list_remove(&location.node->entities_in_node, location.element);
}
