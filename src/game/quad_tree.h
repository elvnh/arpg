#ifndef QUAD_TREE_H
#define QUAD_TREE_H

#include "base/linear_arena.h"
#include "base/rectangle.h"
#include "base/utils.h"
#include "entity_id.h"

#define QUAD_TREE_MAX_DEPTH 6

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

#define QT_NULL_LOCATION (QuadTreeLocation){0}

// TODO: this is only temporary, get rid of it
#define MAX_ENTITIES_PER_NODE 32

struct EntityStorage;

typedef struct {
    EntityID entity_id;
    Rectangle area;
} QuadTreeElement;

typedef struct QuadTree {
    Rectangle area;

    struct QuadTree *top_left;
    struct QuadTree *top_right;
    struct QuadTree *bottom_right;
    struct QuadTree *bottom_left;

    QuadTreeElement entities[MAX_ENTITIES_PER_NODE];
    ssize entity_count;
} QuadTree;

// Store in EntitySlot
// Return when inserting entity
typedef struct {
    QuadTree *node;
    ssize array_index;
} QuadTreeLocation;

// TODO: the EntityStorage* parameters are only temporary

void qt_initialize(QuadTree *qt, Rectangle area);
// if location is null, just insert, otherwise, move entity
// return new location
QuadTreeLocation qt_move_entity(QuadTree *qt, struct EntityStorage *es, EntityID id,
    QuadTreeLocation location, Vector2 new_position, LinearArena *arena);
QuadTreeLocation qt_set_entity_area(QuadTree *qt, struct EntityStorage *es, EntityID id,
    QuadTreeLocation location, Rectangle area, LinearArena *arena);
void qt_remove_entity(QuadTree *qt, struct EntityStorage *es, EntityID id,
    QuadTreeLocation location);

#endif //QUAD_TREE_H
