#ifndef QUAD_TREE_H
#define QUAD_TREE_H

#include "base/free_list_arena.h"
#include "base/linear_arena.h"
#include "base/rectangle.h"
#include "base/utils.h"
#include "entity.h"

#define QUAD_TREE_MAX_DEPTH 6
#define QT_NULL_LOCATION (QuadTreeLocation){0}

/*
  TODO:
  - Make sure that subdivided areas aren't too small
  - Prune empty branches at end of frame
  - Deallocate empty tree nodes, hold on to freed nodes
  - Move child nodes into one struct, allocate all at once
 */

typedef struct QuadTreeElement {
    EntityID entity_id;
    Rectangle area;
    struct QuadTreeElement *next;
    struct QuadTreeElement *prev;
} QuadTreeElement;

typedef struct {
    QuadTreeElement *head;
    QuadTreeElement *tail;
} QuadTreeEntityList;

typedef struct QuadTreeNode {
    Rectangle area;

    struct QuadTreeNode *top_left;
    struct QuadTreeNode *top_right;
    struct QuadTreeNode *bottom_right;
    struct QuadTreeNode *bottom_left;

    QuadTreeEntityList entities_in_node;
} QuadTreeNode;

typedef struct {
    QuadTreeNode root;
    QuadTreeEntityList entity_element_free_list;
} QuadTree;

// Store in EntitySlot
// Return when inserting entity
typedef struct {
    QuadTreeNode *node;
    QuadTreeElement *element;
} QuadTreeLocation;

typedef struct EntityIDNode {
    EntityID id;
    struct EntityIDNode *next;
} EntityIDNode;

typedef struct {
    EntityIDNode *head;
    EntityIDNode *tail;
} EntityIDList;

void qt_initialize(QuadTree *qt, Rectangle area);
QuadTreeLocation qt_move_entity(QuadTree *qt, EntityID id,
    QuadTreeLocation location, Vector2 new_position, FreeListArena *arena);
QuadTreeLocation qt_set_entity_area(QuadTree *qt, EntityID id,
    QuadTreeLocation location, Rectangle area, FreeListArena *arena);
void qt_remove_entity(QuadTree *qt, EntityID id, QuadTreeLocation location);
EntityIDList qt_get_entities_in_area(QuadTree *qt, Rectangle area, LinearArena *arena);
ssize qt_get_node_count(const QuadTree *qt);

#endif //QUAD_TREE_H
