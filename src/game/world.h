#ifndef WORLD_H
#define WORLD_H

#include "animation.h"
#include "entity_system.h"
#include "item.h"
#include "tilemap.h"
#include "camera.h"
#include "collision.h"
#include "input.h"
#include "item_manager.h"
#include "hitsplat.h"

/*
  TODO:
  - Should a World be a single level or something more persistent?
 */

struct Input;
struct DebugState;
struct RenderBatch;
struct AssetList;

typedef struct World {
    LinearArena world_arena;
    Camera camera;
    Tilemap tilemap;
    EntitySystem entity_system;
    ItemManager item_manager;

    CollisionCooldownTable collision_effect_cooldowns;

    CollisionEventTable  previous_frame_collisions;
    CollisionEventTable  current_frame_collisions;

    Hitsplat active_hitsplats[128];
    s32 hitsplat_count;
} World;

void world_initialize(World *world, const struct AssetList *asset_list, LinearArena *arena);
void world_update(World *world, const FrameData *frame_data, const AssetList *assets,
    LinearArena *frame_arena, AnimationTable *animations);
void world_render(World *world, struct RenderBatch *rb, const struct AssetList *asset_list,
    const FrameData *frame_data, LinearArena *frame_arena, struct DebugState *debug_state,
    AnimationTable *animations);
Entity *world_get_player_entity(World *world);

#endif //WORLD_H
