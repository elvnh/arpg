#ifndef WORLD_H
#define WORLD_H

#include "base/free_list_arena.h"
#include "camera.h"
#include "collision/collision.h"
#include "collision/collision_event.h"
#include "components/component_id.h"
#include "entity/entity_system.h"
#include "hitsplat.h"
#include "platform/input_event.h"
#include "quad_tree.h"
#include "renderer/frontend/render_target.h"
#include "tilemap.h"
#include "world/chunk.h"

/*
  TODO:
  - Copying over entities and items when switching world
 */

#define LEVEL_COUNT 3

struct DebugState;
struct RenderBatch;
struct RenderBatchList;

typedef struct World {
    // All allocations specific to the world instance should go here, and when destroying
    // a world, it should be destroyed so that the memory can be reused by other world instances
    // TODO: split up arena into a linear arena and free list arena
    LinearArena world_arena;

    Tilemap tilemap;

    TriggerCooldownTable trigger_cooldowns;

    CollisionEventTable previous_frame_collisions;
    CollisionEventTable current_frame_collisions;

    HitsplatBuffer active_hitsplats;

    EntityID player_entity;

    Chunks map_chunks;

    EntitySystem entity_system;
    EntityID active_entity_ids[MAX_ENTITIES];
    EntityIndex active_entity_count;
    QuadTreeLocation active_entity_quad_tree_locations[MAX_ENTITIES];
    QuadTree quad_tree;
} World;

typedef struct {
    World data[LEVEL_COUNT];
    s32 current_world_index;
} WorldArray;

void world_initialize(World *world, FreeListArena *parent_arena);
void world_destroy(World *world);
void world_make_inactive(World *world, LinearArena *frame_arena);
void world_update(World *world, f32 dt, Camera camera, Vector2i viewport_size,
    LinearArena *frame_arena);
void world_render(World *world, RenderBatches rb_list, Camera camera, Vector2i viewport_size,
    LinearArena *frame_arena, struct DebugState *debug_state);
EntityWithID world_spawn_entity(World *world, Vector2 position, EntityFaction faction,
    EntityKind kind);
EntityWithID world_spawn_non_spatial_entity(World *world, EntityFaction faction,
    EntityKind kind);
void world_remove_entity_by_id(World *world, EntityID id, LinearArena *frame_arena);
void world_make_entity_non_spatial(World *world, Entity *entity);
void world_drop_item_from_position(Vector2 position, Entity *item_entity);
Rectangle world_get_entity_bounding_box(Entity *entity, PhysicsComponent *physics);
void world_kill_entity(World *world, Entity *entity, LinearArena *frame_arena);
void world_add_trigger_cooldown(World *world, EntityID a, EntityID b, ComponentID component,
    RetriggerBehaviour retrigger_behaviour);
void world_set_player_entity(World *world, EntityID id);
Entity *world_get_player_entity(World *world);
Vector2i world_to_tile_coords(Vector2 world_coords);
Vector2 tile_to_world_coords(Vector2i tile_coords);

#endif //WORLD_H
