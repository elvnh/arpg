#ifndef WORLD_H
#define WORLD_H

#include "entity_system.h"
#include "item.h"
#include "tilemap.h"
#include "camera.h"
#include "collision.h"
#include "item_system.h"
#include "hitsplat.h"

/*
  TODO:
  - Should a World be a single level or something more persistent?
 */

struct FrameData;
struct DebugState;
struct RenderBatch;

typedef struct World {
    LinearArena world_arena;
    Camera camera;
    Tilemap tilemap;
    EntitySystem entity_system;
    ItemSystem item_manager;

    CollisionCooldownTable collision_effect_cooldowns;

    CollisionEventTable  previous_frame_collisions;
    CollisionEventTable  current_frame_collisions;

    Hitsplat active_hitsplats[128];
    s32 hitsplat_count;
} World;

void world_initialize(World *world, LinearArena *arena);
void world_update(World *world, const struct FrameData *frame_data, LinearArena *frame_arena,
    struct DebugState *debug_state);
void world_render(World *world, struct RenderBatch *rb, const struct FrameData *frame_data,
    LinearArena *frame_arena, struct DebugState *debug_state);
Entity *world_get_player_entity(World *world);

#endif //WORLD_H
