#include "quad_tree.h"
#include "base/linear_arena.h"
#include "base/list.h"
#include "base/rectangle.h"
#include "base/sl_list.h"
#include "base/utils.h"

static void qt_initialize_node(QuadTreeNode *node, Rectangle area)
{
    node->area = area;
}

static inline void qt_subdivide(QuadTreeNode *node, LinearArena *arena)
{
    ASSERT(!node->top_left);
    ASSERT(!node->top_right);
    ASSERT(!node->bottom_right);
    ASSERT(!node->bottom_left);

    node->top_left     = la_allocate_item(arena, QuadTreeNode);
    node->top_right    = la_allocate_item(arena, QuadTreeNode);
    node->bottom_right = la_allocate_item(arena, QuadTreeNode);
    node->bottom_left  = la_allocate_item(arena, QuadTreeNode);

    RectangleQuadrants quadrants = rect_quadrants(node->area);

    qt_initialize_node(node->top_left, quadrants.top_left);
    qt_initialize_node(node->top_right, quadrants.top_right);
    qt_initialize_node(node->bottom_right, quadrants.bottom_right);
    qt_initialize_node(node->bottom_left, quadrants.bottom_left);
}

void qt_initialize(QuadTree *qt, Rectangle area)
{
    qt_initialize_node(&qt->root, area);
}

static QuadTreeLocation qt_insert(QuadTree *qt, QuadTreeNode *node, EntityID id, Rectangle area, ssize depth,
    LinearArena *arena)
{
    // TODO: clean up this function
    ASSERT(area.size.x > 0);
    ASSERT(area.size.y > 0);

    ASSERT(rect_intersects(node->area, area));

    b32 no_children = !node->top_left;
    RectangleQuadrants quadrants = rect_quadrants(node->area);

    QuadTreeLocation result = {0};

    if (depth < QUAD_TREE_MAX_DEPTH - 1) {
	if (rect_contains_rect(quadrants.top_left, area)) {
	    if (no_children) {
		qt_subdivide(node, arena);
	    }

	    result = qt_insert(qt, node->top_left, id, area, depth + 1, arena);
	} else if (rect_contains_rect(quadrants.top_right, area)) {
	    if (no_children) {
		qt_subdivide(node, arena);
	    }

	    result = qt_insert(qt, node->top_right, id, area, depth + 1, arena);
	} else if (rect_contains_rect(quadrants.bottom_right, area)) {
	    if (no_children) {
		qt_subdivide(node, arena);
	    }

	    result = qt_insert(qt, node->bottom_right, id, area, depth + 1, arena);
	} else if (rect_contains_rect(quadrants.bottom_left, area)) {
	    if (no_children) {
		qt_subdivide(node, arena);
	    }

	    result = qt_insert(qt, node->bottom_left, id, area, depth + 1, arena);
	}
    }

    if (qt_location_is_null(result)) {
        QuadTreeElement *element = list_head(&qt->entity_element_free_list);

        if (!element) {
            element = la_allocate_item(arena, QuadTreeElement);
        } else {
            list_pop_head(&qt->entity_element_free_list);
        }

        element->entity_id = id;
        element->area = area;

        list_push_back(&node->entities_in_node, element);

        result.node = node;
        result.element = element;
    }

    return result;
}

QuadTreeLocation qt_move_entity(QuadTree *qt, EntityID id,
    QuadTreeLocation location, Vector2 new_position, LinearArena *arena)
{
    ASSERT(!qt_location_is_null(location));
    Rectangle new_area = {new_position, location.element->area.size};

    QuadTreeLocation result = qt_set_entity_area(qt, id, location, new_area, arena);

    return result;
}

QuadTreeLocation qt_set_entity_area(QuadTree *qt, EntityID id,
    QuadTreeLocation location, Rectangle area, LinearArena *arena)
{
    if (!qt_location_is_null(location)) {
        qt_remove_entity(qt, id, location);
    }

    QuadTreeLocation result = qt_insert(qt, &qt->root, id, area, 0, arena);

    return result;
}

QuadTreeLocation qt_remove_entity(QuadTree *qt, EntityID id, QuadTreeLocation location)
{
    ASSERT(!qt_location_is_null(location));
    ASSERT(location.element->entity_id.index == id.index);

    list_remove(&location.node->entities_in_node, location.element);
    list_push_back(&qt->entity_element_free_list, location.element);

    return QT_NULL_LOCATION;
}

static void qt_get_entities_in_area_recursive(QuadTreeNode *node, Rectangle area, EntityIDList *list,
    LinearArena *arena)
{
    if (node && rect_intersects(node->area, area)) {
        for (QuadTreeElement *elem = list_head(&node->entities_in_node); elem; elem = list_next(elem)) {
            if (rect_intersects(elem->area, area)) {
                EntityIDNode *id_node = la_allocate_item(arena, EntityIDNode);
                id_node->id = elem->entity_id;

                sl_list_push_back(list, id_node);
            }
        }

        qt_get_entities_in_area_recursive(node->top_left, area, list, arena);
        qt_get_entities_in_area_recursive(node->top_right, area, list, arena);
        qt_get_entities_in_area_recursive(node->bottom_right, area, list, arena);
        qt_get_entities_in_area_recursive(node->bottom_left, area, list, arena);
    }
}

EntityIDList qt_get_entities_in_area(QuadTree *qt, Rectangle area, LinearArena *arena)
{
    EntityIDList result = {0};
    qt_get_entities_in_area_recursive(&qt->root, area, &result, arena);

    return result;
}

static ssize qt_get_node_count_recursive(const QuadTree *qt, const QuadTreeNode *node)
{
    ssize result = 0;

    if (node) {
        result = 1;

        result += qt_get_node_count_recursive(qt, node->top_left);
        result += qt_get_node_count_recursive(qt, node->top_right);
        result += qt_get_node_count_recursive(qt, node->bottom_right);
        result += qt_get_node_count_recursive(qt, node->bottom_left);
    }

    return result;
}

ssize qt_get_node_count(const QuadTree *qt)
{
    ssize result = qt_get_node_count_recursive(qt, &qt->root);

    return result;
}
