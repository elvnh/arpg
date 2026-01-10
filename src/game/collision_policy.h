#ifndef COLLISION_POLICY_H
#define COLLISION_POLICY_H

#include "collision.h"
#include "entity_id.h"

struct World;
struct Entity;
struct EntitySystem;
struct FreeListArena;

typedef enum {
    COLLIDE_EFFECT_PASS_THROUGH, // NOTE: since this is the first enum value, it's the default behaviour
    COLLIDE_EFFECT_STOP,
    COLLIDE_EFFECT_BOUNCE,
    COLLIDE_EFFECT_DIE,
} CollideEffect;

// TODO: not needed?
typedef enum {
    OBJECT_KIND_ENTITIES = (1 << 0),
    OBJECT_KIND_TILES = (1 << 1),
} ObjectKind;

// TODO: only the effect enum is needed for regular collision policies,
typedef struct {
    CollideEffect effect;
} CollisionPolicy;


// TODO: more consistent naming
void execute_collision_policy_for_tilemap_collision(struct World *world, struct Entity *entity,
    CollisionInfo collision);
void execute_entity_vs_entity_collision_policy(struct World *world, struct Entity *entity,
    struct Entity *other, CollisionInfo collision, EntityPairIndex collision_index);

#endif //COLLISION_POLICY_H
