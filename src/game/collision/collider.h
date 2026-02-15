#ifndef COLLIDER_H
#define COLLIDER_H

#include "base/vector.h"
#include "entity/entity_faction.h"
#include "collision/collision_policy.h"
#include "collision/trigger.h"

// Entities that are in the same collision group won't collide with eachother
typedef enum {
    COLLISION_GROUP_NONE,
    COLLISION_GROUP_PROJECTILES,
} CollisionGroup;

typedef struct ColliderComponent {
    // TODO: offset from pos
    Vector2 size;
    CollisionPolicy per_faction_collision_policies[FACTION_COUNT];
    CollisionPolicy tilemap_collision_policy;
    CollisionGroup collision_group;
} ColliderComponent;

static inline void set_collision_policy_vs_tilemaps(ColliderComponent *collider, CollisionPolicy policy)
{
    collider->tilemap_collision_policy = policy;
}

static inline void set_collision_policy_vs_faction(ColliderComponent *collider,
    CollisionPolicy policy, EntityFaction faction)
{
    ASSERT(faction >= 0);
    ASSERT(faction < FACTION_COUNT);

    collider->per_faction_collision_policies[faction] = policy;
}

static inline void set_collision_policy_vs_entities(ColliderComponent *collider, CollisionPolicy policy)
{
    for (EntityFaction faction = 0; faction < FACTION_COUNT; ++faction) {
	set_collision_policy_vs_faction(collider, policy, faction);
    }
}

static inline void set_collision_policy_vs_hostile_faction(ColliderComponent *collider,
    CollisionPolicy policy, EntityFaction our_faction)
{
    ASSERT(our_faction != FACTION_NEUTRAL);
    ASSERT(our_faction != FACTION_COUNT);

    GetHostileFactionResult result = get_hostile_faction(our_faction);

    ASSERT(result.ok);
    set_collision_policy_vs_faction(collider, policy, result.hostile_faction);
}

#endif //COLLIDER_H
