#ifndef GAME_WORLD_H
#define GAME_WORLD_H

#include "entity.h"
#include "tilemap.h"
#include "camera.h"

typedef struct {
    EntityID   entity_a;
    EntityID   entity_b;
} EntityPair;

// TODO: more arbitrary collision responses
typedef struct CollisionRule {
    struct CollisionRule *next;
    struct CollisionRule *prev;
    EntityPair		  entity_pair;
    b32			  should_collide;
} CollisionRule;

typedef struct {
    CollisionRule *head;
    CollisionRule *tail;
} CollisionRuleList;

typedef struct {
    CollisionRuleList  table[512];
    CollisionRuleList  free_node_list;
} CollisionRuleTable;

typedef struct {
    LinearArena world_arena;
    Camera camera;
    Tilemap tilemap;
    EntityStorage entities;
    CollisionRuleTable collision_rules;
} GameWorld;

static inline EntityPair collision_pair_create(EntityID a, EntityID b)
{
    if (entity_id_less_than(b, a)) {
        EntityID tmp = a;
        a = b;
        b = tmp;
    }

    EntityPair result = {
	.entity_a = a,
	.entity_b = b
    };

    return result;
}

#endif //GAME_WORLD_H
