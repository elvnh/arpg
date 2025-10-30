#ifndef WORLD_H
#define WORLD_H

#include "entity_system.h"
#include "item.h"
#include "tilemap.h"
#include "camera.h"
#include "collision.h"
#include "input.h"

struct Input;
struct DebugState;
struct RenderBatch;
struct AssetList;

// TODO: make size and velocity depend on damage number
// TODO: make color depend on elemental types present
typedef struct {
    DamageInstance damage;
    Vector2 position;
    Vector2 velocity;
    f32 timer;
    f32 lifetime;
} Hitsplat;

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
void world_update(World *world, FrameData frame_data, const AssetList *assets, LinearArena *frame_arena);
void world_render(World *world, struct RenderBatch *rb, const struct AssetList *asset_list,
    FrameData frame_data, LinearArena *frame_arena, struct DebugState *debug_state);

#endif //WORLD_H
