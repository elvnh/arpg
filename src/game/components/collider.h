#ifndef COLLIDER_H
#define COLLIDER_H

#include "base/vector.h"
#include "entity_faction.h"
#include "collision_policy.h"
#include "trigger.h"

typedef struct {
    // TODO: offset from pos
    Vector2 size;
    CollisionPolicy per_faction_collision_policies[FACTION_COUNT];
    CollisionPolicy tilemap_collision_policy;
} ColliderComponent;

static inline void set_collision_policy_vs_tilemaps(ColliderComponent *collider, CollideEffect effect)
{
    collider->tilemap_collision_policy.effect = effect;
}

static inline void set_collision_policy_vs_faction(ColliderComponent *collider,
    CollideEffect effect, EntityFaction faction)
{
    ASSERT(faction >= 0);
    ASSERT(faction < FACTION_COUNT);

    collider->per_faction_collision_policies[faction].effect = effect;
}

static inline void set_collision_policy_vs_entities(ColliderComponent *collider, CollideEffect effect)
{
    for (EntityFaction faction = 0; faction < FACTION_COUNT; ++faction) {
	set_collision_policy_vs_faction(collider, effect, faction);
    }
}

static inline void set_collision_policy_vs_hostile_faction(ColliderComponent *collider,
    CollideEffect effect, EntityFaction our_faction)
{
    ASSERT(our_faction != FACTION_NEUTRAL);
    ASSERT(our_faction != FACTION_COUNT);

    EntityFaction hostile_faction = 0;

    if (our_faction == FACTION_PLAYER) {
	hostile_faction = FACTION_ENEMY;
    } else {
	hostile_faction = FACTION_PLAYER;
    }

    set_collision_policy_vs_faction(collider, effect, hostile_faction);
}

#endif //COLLIDER_H
