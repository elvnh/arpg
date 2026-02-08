#ifndef COLLISION_POLICY_H
#define COLLISION_POLICY_H

#include "collision.h"
#include "entity/entity_id.h"

struct World;
struct Entity;
struct EntitySystem;
struct FreeListArena;
struct PhysicsComponent;
struct ColliderComponent;

typedef enum {
    COLLISION_POLICY_PASS_THROUGH, // NOTE: since this is the first enum value, it's the default behaviour
    COLLISION_POLICY_STOP,
    COLLISION_POLICY_BOUNCE,
    COLLISION_POLICY_DIE,
} CollisionPolicy;

void execute_entity_vs_tilemap_collision_policy(struct World *world, struct Entity *entity, struct PhysicsComponent *physics,
    struct ColliderComponent *collider, CollisionInfo collision);

// TODO: reduce number of parameters
void execute_entity_vs_entity_collision_policy(struct World *world,
    struct Entity *entity, struct PhysicsComponent *entity_physics, struct ColliderComponent *entity_collider,
    struct Entity *other,  struct ColliderComponent *other_collider,
    CollisionInfo collision, EntityPairIndex collision_index);



#endif //COLLISION_POLICY_H
