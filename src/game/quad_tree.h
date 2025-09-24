#ifndef QUAD_TREE_H
#define QUAD_TREE_H

#include "base/linear_arena.h"
#include "base/rectangle.h"
#include "base/utils.h"
#include "entity_id.h"

#define QUAD_TREE_MAX_DEPTH 6
#define QT_NULL_LOCATION (QuadTreeLocation){0}

/*
  TODO:
  - Make sure that subdivided areas aren't too small
  - Make entityID arrays be dynamic
  - Deallocate empty nodes, hold on to freed nodes
  - How to update entity locations on removing entity?
    - Bidirectional pointers? Element has pointer back to quad tree
      location?
    - Entity ID already acts as a pointer, pass in EntityStorage when removing/inserting
      an entity, will look up any entities that move array index and correct them
    - Or use a linked list instead?
  - Move children into one struct, allocate all at once
 */

struct EntityStorage;

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
    // TODO: free list for nodes and elements
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

// TODO: the EntityStorage* parameters are only temporary

void qt_initialize(QuadTree *qt, Rectangle area);
QuadTreeLocation qt_move_entity(QuadTree *qt, struct EntityStorage *es, EntityID id,
    QuadTreeLocation location, Vector2 new_position, LinearArena *arena);
QuadTreeLocation qt_set_entity_area(QuadTree *qt, struct EntityStorage *es, EntityID id,
    QuadTreeLocation location, Rectangle area, LinearArena *arena);
void qt_remove_entity(struct EntityStorage *es, EntityID id, QuadTreeLocation location);
EntityIDList qt_get_entities_in_area(QuadTree *qt, Rectangle area, LinearArena *arena);

#endif //QUAD_TREE_H
