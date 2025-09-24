#ifndef GAME_WORLD_H
#define GAME_WORLD_H

#include "entity.h"
#include "tilemap.h"
#include "camera.h"

typedef struct CollisionRule {
    EntityID   a;
    EntityID   b;
    b32        should_collide;
    struct CollisionRule *next_in_hash;
} CollisionRule;

typedef struct {
    CollisionRule *collision_rules[512];
} CollisionRuleTable;

typedef struct {
    Camera camera;
    Tilemap tilemap;
    EntityStorage entities;
    CollisionRuleTable collision_rules;
} GameWorld;

#endif //GAME_WORLD_H
