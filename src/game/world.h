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

typedef struct World {
    LinearArena world_arena;
    Camera camera;
    Tilemap tilemap;
    EntitySystem entities;
    CollisionExceptionTable collision_rules;
    CollisionEventTable  previous_frame_collisions;
    CollisionEventTable  current_frame_collisions;
} World;

void world_initialize(World *world, const struct AssetList *assets, LinearArena *arena);
void world_update(World *world, const struct Input *input, f32 dt, const AssetList *assets, LinearArena *frame_arena);
void world_render(World *world, struct RenderBatch *rb, const struct AssetList *asset_list, FrameData frame_data,
    LinearArena *frame_arena, struct DebugState *debug_state);
void world_add_collision_exception(World *world, EntityID a, EntityID b);

#endif //WORLD_H
