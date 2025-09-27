#ifndef GAME_WORLD_H
#define GAME_WORLD_H

#include "entity_system.h"
#include "tilemap.h"
#include "camera.h"
#include "collision.h"

typedef struct {
    LinearArena world_arena;
    Camera camera;
    Tilemap tilemap;
    EntitySystem entities;
    CollisionRuleTable collision_rules;
} GameWorld;


#endif //GAME_WORLD_H
