#ifndef GAME_WORLD_H
#define GAME_WORLD_H

#include "entity.h"
#include "tilemap.h"
#include "camera.h"

// TODO: more arbitrary collision responses
typedef struct CollisionRule {
    struct CollisionRule *next;
    struct CollisionRule *prev;
    EntityID   a;
    EntityID   b;
    b32        should_collide;
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

#endif //GAME_WORLD_H
