#ifndef COLLISION_H
#define COLLISION_H

#include "base/vector.h"
#include "base/rectangle.h"
#include "base/linear_arena.h"
#include "entity.h"
#include "game/component.h"

/*
  TODO:
  - Make collision rule able to match criteria other than entity ID, eg. all entities from
    the same faction don't collide with eachother
  - Collision rule with entity vs tilemap collision?
  - Time to live on collision rules to remove them automatically once time runs out, to be used
    for projectiles with cooldown before they can hit again etc.
 */

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

// typedef enum {
//     COLL_EXC_EXPIRE_ON_DEATH,
//     COLL_EXC_EXPIRE_ON_NON_CONTACT,
//     //COLL_EXC_EXPIRE_AFTER_DURATION
// } CollisionExceptionExpiry;

// TODO: rename
typedef struct CollisionRule {
    struct CollisionRule  *next;
    struct CollisionRule  *prev;
    EntityID owning_entity;
    EntityID collided_entity;
    s32 collision_effect_index;
} CollisionException;

typedef struct {
    CollisionException *head;
    CollisionException *tail;
} CollisionExceptionList;

typedef struct {
    CollisionExceptionList  table[512];
    CollisionExceptionList  free_node_list;
} CollisionExceptionTable;

typedef struct CollisionEvent {
    struct CollisionEvent *next;
    EntityPair entity_pair;
} CollisionEvent;

typedef struct {
    CollisionEvent *head;
    CollisionEvent *tail;
} CollisionEventList;

typedef struct {
    CollisionEventList *table;
    ssize               table_size;
    LinearArena         arena;
} CollisionEventTable;

struct World;

/* Collision algorithms */
CollisionInfo collision_rect_vs_rect(f32 movement_fraction_left, Rectangle rect_a, Rectangle rect_b,
    Vector2 velocity_a, Vector2 velocity_b, f32 dt);

/* Collision exception table */
CollisionException *collision_exception_find(CollisionExceptionTable *table, EntityID a, EntityID b, s32 effect_index);
void collision_exception_add(CollisionExceptionTable *table, EntityID a, EntityID b,
    s32 effect_index, LinearArena *arena);
void remove_expired_collision_exceptions(struct World *world);

/* Collision event table */
CollisionEventTable collision_event_table_create(LinearArena *parent_arena);
CollisionEvent *collision_event_table_find(CollisionEventTable *table, EntityID a, EntityID b);
void collision_event_table_insert(CollisionEventTable *table, EntityID a, EntityID b);

#endif //COLLISION_H
