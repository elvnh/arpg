#ifndef GAME_WORLD_H
#define GAME_WORLD_H

#include "entity_system.h"
#include "tilemap.h"
#include "camera.h"
#include "collision.h"

typedef struct CollisionEvent {
    struct CollisionEvent *next;
    EntityPair entity_pair;
} CollisionEvent;

typedef struct {
    CollisionEvent *head;
    CollisionEvent *tail;
} CollisionList;

typedef struct {
    CollisionList    *table;
    ssize             table_size;
    LinearArena       arena;
} CollisionTable;

typedef struct {
    LinearArena world_arena;
    Camera camera;
    Tilemap tilemap;
    EntitySystem entities;
    CollisionRuleTable collision_rules;
    CollisionTable  previous_frame_collisions;
    CollisionTable  current_frame_collisions;
} GameWorld;

#endif //GAME_WORLD_H
