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
  - Handle player input in game instead
  - Add a free list arena to World then allocate a subarena from it for each
    entity. On entity death, destroy the arena and all memory tied to that entity is
    released
 */

struct FrameData;
struct DebugState;
struct RenderBatch;
struct GameUIState;

// TODO: keep a list of items that exist in this world to avoid having to iterate all items when destroying world
// TODO: move world arena to game, since that memory should be reused for new worlds?

typedef struct World {
    // All allocations specific to the world instance should go here, and when destroying
    // a world, it should be destroyed so that the memory can be reused by other world instances
    LinearArena world_arena;

    Camera camera;
    Tilemap tilemap;

    // Pointer to the entity system and item system are stored here to avoid having to
    // pass them around along with World.
    // TODO: should they just be global instead?
    EntitySystem *entity_system;
    ItemSystem *item_system;

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

void world_initialize(World *world, EntitySystem *entity_system, ItemSystem *item_system,
    FreeListArena *parent_arena);
void world_destroy(World *world);

// TODO: fix parameters
void world_update(World *world, const struct FrameData *frame_data, LinearArena *frame_arena,
    struct DebugState *debug_state, struct GameUIState *game_ui);
void world_render(World *world, struct RenderBatch *rb, const struct FrameData *frame_data,
    LinearArena *frame_arena, struct DebugState *debug_state);
EntityWithID world_spawn_entity(World *world, EntityFaction faction);
Rectangle world_get_entity_bounding_box(Entity *entity);

Entity *world_get_player_entity(World *world);

#endif //WORLD_H
