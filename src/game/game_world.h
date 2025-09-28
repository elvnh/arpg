#ifndef GAME_WORLD_H
#define GAME_WORLD_H

#include "entity_system.h"
#include "tilemap.h"
#include "camera.h"
#include "collision.h"

typedef struct IntersectionNode {
    struct IntersectionNode *next;
    EntityPair entity_pair;
} IntersectionNode;

typedef struct {
    IntersectionNode *head;
    IntersectionNode *tail;
} IntersectionList;

typedef struct {
    IntersectionList *table;
    ssize             table_size;
    LinearArena       arena;
} IntersectionTable;

typedef struct {
    LinearArena world_arena;
    Camera camera;
    Tilemap tilemap;
    EntitySystem entities;
    CollisionRuleTable collision_rules;
    IntersectionTable  previous_frame_intersections;
    IntersectionTable  current_frame_intersections;
} GameWorld;

#endif //GAME_WORLD_H
