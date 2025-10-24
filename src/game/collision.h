#ifndef COLLISION_H
#define COLLISION_H

#include "base/vector.h"
#include "base/rectangle.h"
#include "base/linear_arena.h"
#include "entity.h"
#include "game/component.h"

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

typedef struct CollisionEffectCooldown {
    struct CollisionEffectCooldown *next;
    struct CollisionEffectCooldown *prev;
    EntityID owning_entity;
    EntityID collided_entity;
    s32 collision_effect_index;
} CollisionEffectCooldown;

typedef struct {
    CollisionEffectCooldown *head;
    CollisionEffectCooldown *tail;
} CollisionCooldownList;

typedef struct {
    CollisionCooldownList  table[512];
    CollisionCooldownList  free_node_list;
} CollisionCooldownTable;

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

/* Collision cooldown table */
CollisionEffectCooldown *collision_cooldown_find(CollisionCooldownTable *table, EntityID a, EntityID b, s32 effect_index);
void collision_cooldown_add(CollisionCooldownTable *table, EntityID a, EntityID b,
    s32 effect_index, LinearArena *arena);
void remove_expired_collision_cooldowns(struct World *world);

/* Collision event table */
CollisionEventTable collision_event_table_create(LinearArena *parent_arena);
CollisionEvent *collision_event_table_find(CollisionEventTable *table, EntityID a, EntityID b);
void collision_event_table_insert(CollisionEventTable *table, EntityID a, EntityID b);

#endif //COLLISION_H
