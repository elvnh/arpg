#ifndef COLLISION_H
#define COLLISION_H

#include "base/vector.h"
#include "base/rectangle.h"
#include "base/linear_arena.h"
#include "entity.h"

/*
  TODO:
  - Arbitrary collision responses, eg. bounce, pass through, block, etc.
  - Make collision rule able to match criteria other than entity ID, eg. all entities from
    the same faction don't collide with eachother
  - Collision rule with entity vs tilemap collision?
  - Keep track of previous and current frame intersections for registering projectile damage etc.
  - Time to live on collision rules to remove them automatically once time runs out, to be used
    for projectiles with cooldown before they can hit again for etc.
 */

typedef struct CollisionRule {
    struct CollisionRule  *next;
    struct CollisionRule  *prev;
    EntityPair		   entity_pair;
    b32			   should_collide;
} CollisionException;

typedef struct {
    CollisionException *head;
    CollisionException *tail;
} CollisionExceptionList;

typedef struct {
    CollisionExceptionList  table[512];
    CollisionExceptionList  free_node_list;
} CollisionExceptionTable;

typedef enum {
    COLL_NOT_COLLIDING,
    COLL_ARE_INTERSECTING,
    COLL_WILL_COLLIDE_THIS_FRAME,
} CollisionState; // TODO: better name

typedef struct {
    CollisionState collision_state;
    Vector2  new_position_a;
    Vector2  new_position_b;
    Vector2  new_velocity_a;
    Vector2  new_velocity_b;
    f32      movement_fraction_left;
    Vector2  collision_normal; // TODO: separate normals for A and B?
} CollisionInfo;

CollisionInfo collision_rect_vs_rect(f32 movement_fraction_left, Rectangle rect_a, Rectangle rect_b,
    Vector2 velocity_a, Vector2 velocity_b, f32 dt);
CollisionException *collision_exception_find(CollisionExceptionTable *table, EntityID a, EntityID b);
void collision_exception_add(CollisionExceptionTable *table, EntityID a, EntityID b, b32 should_collide,
    LinearArena *arena);
void remove_collision_exceptions_with_entity(CollisionExceptionTable *table, EntityID a);

#endif //COLLISION_H
