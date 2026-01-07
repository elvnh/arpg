#ifndef WORLD_H
#define WORLD_H

#include "entity_system.h"
#include "item.h"
#include "quad_tree.h"
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

    // Pointer to the entity system is stored here to avoid having to pass it around along with World
    EntitySystem *entity_system;
    ItemSystem item_manager; // TODO: move to Game

    CollisionCooldownTable collision_effect_cooldowns;

    CollisionEventTable  previous_frame_collisions;
    CollisionEventTable  current_frame_collisions;

    Hitsplat active_hitsplats[128];
    s32 hitsplat_count;

    EntityID       alive_entity_ids[MAX_ENTITIES];
    EntityIndex    alive_entity_count;
    QuadTree       quad_tree;
    QuadTreeLocation alive_entity_quad_tree_locations[MAX_ENTITIES];
} World;

void world_initialize(World *world, LinearArena *arena, EntitySystem *entity_system);

// TODO: fix parameters
void world_update(World *world, const struct FrameData *frame_data, LinearArena *frame_arena,
    struct DebugState *debug_state);
void world_render(World *world, struct RenderBatch *rb, const struct FrameData *frame_data,
    LinearArena *frame_arena, struct DebugState *debug_state);
EntityWithID world_spawn_entity(World *world, EntityFaction faction);
Rectangle world_get_entity_bounding_box(Entity *entity);

Entity *world_get_player_entity(World *world);

#endif //WORLD_H
