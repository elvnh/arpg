#ifndef WORLD_H
#define WORLD_H

#include "base/free_list_arena.h"
#include "components/component_id.h"
#include "entity/entity_system.h"
#include "item.h"
#include "quad_tree.h"
#include "renderer/frontend/render_target.h"
#include "tilemap.h"
#include "camera.h"
#include "collision/collision.h"
#include "item_system.h"
#include "hitsplat.h"
#include "collision/collision_event.h"

/*
  TODO:
  - Copying over entities and items when switching world
  - Keep a list of items that exist in this world to avoid having to iterate
    all items when destroying world
 */

struct FrameData;
struct DebugState;
struct RenderBatch;
struct RenderBatchList;

typedef struct World {
    // All allocations specific to the world instance should go here, and when destroying
    // a world, it should be destroyed so that the memory can be reused by other world instances
    // TODO: split up arena into a linear arena and free list arena
    LinearArena world_arena;

    Camera camera;
    Tilemap tilemap;

    TriggerCooldownTable trigger_cooldowns;

    CollisionEventTable  previous_frame_collisions;
    CollisionEventTable  current_frame_collisions;

    HitsplatBuffer active_hitsplats;

    ItemSystem       item_system;

    EntitySystem     entity_system;
    EntityID         alive_entity_ids[MAX_ENTITIES];
    EntityIndex      alive_entity_count;
    QuadTreeLocation alive_entity_quad_tree_locations[MAX_ENTITIES];
    QuadTree         quad_tree;
} World;

void world_initialize(World *world, FreeListArena *parent_arena);
void world_destroy(World *world);
void world_update(World *world, const struct FrameData *frame_data, LinearArena *frame_arena);
void world_render(World *world, RenderBatches rb_list, const struct FrameData *frame_data,
                  LinearArena *frame_arena, struct DebugState *debug_state);
EntityWithID world_spawn_entity(World *world, EntityFaction faction);
Rectangle world_get_entity_bounding_box(Entity *entity);
void world_kill_entity(World *world, Entity *entity);
void world_add_trigger_cooldown(World *world, EntityID a, EntityID b, ComponentID component,
    RetriggerBehaviour retrigger_behaviour);

Entity *world_get_player_entity(World *world);
Vector2i world_to_tile_coords(Vector2 world_coords);
Vector2 tile_to_world_coords(Vector2i tile_coords);

#endif //WORLD_H
