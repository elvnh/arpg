#ifndef WORLD_H
#define WORLD_H

#include "entity_system.h"
#include "tilemap.h"
#include "camera.h"
#include "collision.h"
#include "input.h"

struct Input;
struct DebugState;
struct RenderBatch;
struct AssetList;

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
    CollisionExceptionTable collision_rules;
    CollisionTable  previous_frame_collisions;
    CollisionTable  current_frame_collisions;
} World;

void world_initialize(World *world, const struct AssetList *assets, LinearArena *arena);
void world_update(World *world, const struct Input *input, f32 dt, const AssetList *assets, LinearArena *frame_arena);
void world_render(World *world, struct RenderBatch *rb, const struct AssetList *asset_list, FrameData frame_data,
    LinearArena *frame_arena, struct DebugState *debug_state);

#endif //WORLD_H
