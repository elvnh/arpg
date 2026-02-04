#ifndef COLLISION_POLICY_H
#define COLLISION_POLICY_H

#include "collision.h"
#include "entity_id.h"

struct World;
struct Entity;
struct EntitySystem;
struct FreeListArena;

typedef enum {
    COLLISION_POLICY_PASS_THROUGH, // NOTE: since this is the first enum value, it's the default behaviour
    COLLISION_POLICY_STOP,
    COLLISION_POLICY_BOUNCE,
    COLLISION_POLICY_DIE,
} CollisionPolicy;

void execute_entity_vs_tilemap_collision_policy(struct World *world, struct Entity *entity,
    CollisionInfo collision);
void execute_entity_vs_entity_collision_policy(struct World *world, struct Entity *entity,
    struct Entity *other, CollisionInfo collision, EntityPairIndex collision_index);

#endif //COLLISION_POLICY_H
