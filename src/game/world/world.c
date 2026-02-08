#include "world.h"
#include "ai.h"
#include "base/line.h"
#include "base/linear_arena.h"
#include "base/list.h"
#include "base/matrix.h"
#include "base/ring_buffer.h"
#include "base/string8.h"
#include "base/triangle.h"
#include "collision/collision_event.h"
#include "collision/collider.h"
#include "components/component.h"
#include "components/particle.h"
#include "entity/entity_faction.h"
#include "entity/entity_id.h"
#include "entity/entity_state.h"
#include "equipment.h"
#include "game.h"
#include "animation.h"
#include "base/format.h"
#include "base/rectangle.h"
#include "base/rgba.h"
#include "base/random.h"
#include "base/utils.h"
#include "base/sl_list.h"
#include "health.h"
#include "item_system.h"
#include "light.h"
#include "renderer/frontend/render_target.h"
#include "status_effect.h"
#include "damage.h"
#include "debug.h"
#include "camera.h"
#include "collision/collision.h"
#include "entity/entity.h"
#include "entity/entity_system.h"
#include "item.h"
#include "magic.h"
#include "modifier.h"
#include "quad_tree.h"
#include "renderer/frontend/render_batch.h"
#include "renderer/frontend/render_key.h"
#include "stats.h"
#include "tilemap.h"
#include "asset_table.h"
#include "collision/trigger.h"
#include "line_of_sight.h"
#include "base/polygon.h"

#define WORLD_ARENA_SIZE FREE_LIST_ARENA_SIZE / 4

/* TODO:
   - Function for getting entity id from alive entity index
   - Function getting entity ptr from alive entity index
   - Clean up order of parameters in new functions
   - Move ItemSystem to game in a similar way
   - Reactivate inactivated warnings
   - Ensure that entities are destroyed when destroying world!
   - Pass in world size to world_initialize
 */

Rectangle world_get_entity_bounding_box(Entity *entity)
{
    // NOTE: this minimum size is completely arbitrary
    Vector2 size = {4, 4};

    if (es_has_component(entity, ColliderComponent)) {
        ColliderComponent *collider = es_get_component(entity, ColliderComponent);
        size.x = MAX(size.x, collider->size.x);
        size.y = MAX(size.y, collider->size.y);
    }

    if (es_has_component(entity, SpriteComponent)) {
        SpriteComponent *sprite_comp = es_get_component(entity, SpriteComponent);
	Sprite *sprite = &sprite_comp->sprite;

        size.x = MAX(size.x, sprite->size.x);
        size.y = MAX(size.y, sprite->size.y);
    }

    if (es_has_component(entity, AnimationComponent)) {
        AnimationComponent *anim_comp = es_get_component(entity, AnimationComponent);
	AnimationInstance *current_anim = &anim_comp->current_animation;
	AnimationFrame current_frame = anim_get_current_frame(current_anim);

        size.x = MAX(size.x, current_frame.sprite.size.x);
        size.y = MAX(size.y, current_frame.sprite.size.y);
    }

    Rectangle result = {entity->position, size};

    return result;
}

static Rectangle get_tile_rectangle_in_world_space(Vector2i tile_coords)
{
    Rectangle result = {
	tile_to_world_coords(tile_coords),
	{(f32)TILE_SIZE, (f32)TILE_SIZE },
    };

    return result;
}

static void world_update_entity_quad_tree_location(World *world, ssize alive_entity_index)
{
    ASSERT(alive_entity_index < world->alive_entity_count);

    EntityID id = world->alive_entity_ids[alive_entity_index];
    Entity *entity = es_get_entity(&world->entity_system, id);
    ASSERT(entity);

    QuadTreeLocation *loc = &world->alive_entity_quad_tree_locations[alive_entity_index];
    Rectangle entity_area = world_get_entity_bounding_box(entity);

    *loc = qt_set_entity_area(&world->quad_tree, id, *loc, entity_area, &world->world_arena);
}

EntityWithID world_spawn_entity(World *world, EntityFaction faction)
{
    ASSERT(faction >= 0);
    ASSERT(faction < FACTION_COUNT);
    ASSERT(world->alive_entity_count < MAX_ENTITIES);

    EntityWithID result = es_create_entity(&world->entity_system, faction);

    ssize alive_index = world->alive_entity_count++;
    world->alive_entity_ids[alive_index] = result.id;
    world->alive_entity_quad_tree_locations[alive_index] = QT_NULL_LOCATION;

    return result;
}

// Handles anything that needs to be handled before removing an entity,
// such as transferring components that need to stay alive a little longer
// over to a new entity.
static void world_remove_entity(World *world, ssize alive_entity_index)
{
    ASSERT(alive_entity_index < world->alive_entity_count);

    EntityID *id = &world->alive_entity_ids[alive_entity_index];
    Entity *entity = es_get_entity(&world->entity_system, *id);

    if (es_has_component(entity, ParticleSpawner)) {
	// If entity dies while particles are active, create a new particle spawner
	// entity with the remaining particles
	ParticleSpawner *ps = es_get_component(entity, ParticleSpawner);

	if (ring_length(&ps->particle_buffer) > 0) {
	    EntityWithID new_ps_entity = world_spawn_entity(world, FACTION_NEUTRAL);

	    new_ps_entity.entity->position = entity->position;
	    ParticleSpawner *new_ps = es_add_component(new_ps_entity.entity, ParticleSpawner);

	    // TODO: if particle spawner get a non-static buffer, a deep copy will be required
	    *new_ps = *ps;
	    new_ps->particles_left_to_spawn = 0;
	    new_ps->action_when_done = PS_WHEN_DONE_REMOVE_ENTITY;
	    new_ps->config.infinite = false;
	}
    }

    if (es_has_component(entity, LightEmitter)) {
	// If an entity dies while it has a light, the light is transferred
	// to a new entity and the light starts fading out
	LightEmitter *old_light = es_get_component(entity, LightEmitter);

	b32 should_transfer_light = !old_light->light.fading_out
	    || (old_light->light.time_elapsed < old_light->light.fade_duration);

        if (should_transfer_light) {
            Entity *new_entity = world_spawn_entity(world, FACTION_NEUTRAL).entity;
	    new_entity->position = entity->position;

	    Vector2i tile_coords = world_to_tile_coords(new_entity->position);
	    Tile *tile = tilemap_get_tile(&world->tilemap, tile_coords);

	    if (!tile || (tile->type == TILE_WALL)) {
		Rectangle tile_rect = get_tile_rectangle_in_world_space(tile_coords);
		RectanglePoint closest_point = rect_bounds_point_closest_to_point(
		    tile_rect, new_entity->position);

		// The entity will be placed on the perimeter of the tile it was spawned in,
		// which means it will be considered to be inside it, so we offset it slightly.
		new_entity->position = v2_sub(closest_point.point, v2_norm(entity->velocity));
	    }

            LightEmitter *new_light = es_add_component(new_entity, LightEmitter);
            *new_light = *old_light;

	    f32 light_fade_duration = new_light->light.fade_duration;

	    if (light_fade_duration == 0.0f) {
		light_fade_duration = LIGHT_DEFAULT_FADE_OUT_TIME;
	    }

	    new_light->light.fade_duration = light_fade_duration;

            LifetimeComponent *lt = es_add_component(new_entity, LifetimeComponent);
            lt->time_to_live = light_fade_duration;

            es_remove_component(entity, LightEmitter);

            new_light->light.fading_out = true;
        }
    }

    es_remove_entity(&world->entity_system, *id);

    QuadTreeLocation *qt_location = &world->alive_entity_quad_tree_locations[alive_entity_index];
    qt_remove_entity(&world->quad_tree, *id, *qt_location);

    ssize last_index = world->alive_entity_count - 1;
    *qt_location = world->alive_entity_quad_tree_locations[last_index];

    *id = world->alive_entity_ids[last_index];

    --world->alive_entity_count;
}

void world_kill_entity(World *world, Entity *entity)
{
    EventData death_event = event_data_death();
    send_event_to_entity(entity, death_event, world);

    es_schedule_entity_for_removal(entity);
}

Vector2i world_to_tile_coords(Vector2 world_coords)
{
    Vector2i result = {
        (s32)((f32)world_coords.x / (f32)TILE_SIZE),
        (s32)((f32)world_coords.y / (f32)TILE_SIZE)
    };

    return result;
}

Vector2 tile_to_world_coords(Vector2i tile_coords)
{
    Vector2 result = {
        (f32)tile_coords.x * TILE_SIZE,
        (f32)tile_coords.y * TILE_SIZE
    };

    return result;
}

static b32 entity_should_die(Entity *entity)
{
    if (es_has_component(entity, HealthComponent)) {
	HealthComponent *hp = es_get_component(entity, HealthComponent);

	ASSERT(hp->health.current_hitpoints >= 0);
	if (hp->health.current_hitpoints == 0) {
	    return true;
	}
    }

    if (es_has_component(entity, ParticleSpawner)) {
        ParticleSpawner *ps = es_get_component(entity, ParticleSpawner);

        if ((ps->action_when_done == PS_WHEN_DONE_REMOVE_ENTITY)
            && particle_spawner_is_finished(ps)) {
            return true;
        }
    }

    if (es_has_component(entity, LifetimeComponent)) {
        LifetimeComponent *lt = es_get_component(entity, LifetimeComponent);

        if (lt->time_to_live <= 0.0f) {
            return true;
        }
    }

    return false;
}

static void entity_update(World *world, ssize alive_entity_index, f32 dt)
{
    EntityID id = world->alive_entity_ids[alive_entity_index];
    Entity *entity = es_get_entity(&world->entity_system, id);

    if (es_has_component(entity, HealthComponent)) {
	HealthComponent *hp = es_get_component(entity, HealthComponent);
	StatValue max_hp = get_total_stat_value(entity, STAT_HEALTH, &world->item_system);
	set_max_health(&hp->health, max_hp);
    }

    if (es_has_component(entity, AIComponent)) {
	AIComponent *ai = es_get_component(entity, AIComponent);
	entity_update_ai(world, entity, ai);
    }

    if (es_has_component(entity, ParticleSpawner)) {
        ParticleSpawner *particle_spawner = es_get_component(entity, ParticleSpawner);
        update_particle_spawner(entity, particle_spawner, dt);
    }

    if (es_has_component(entity, LifetimeComponent)) {
        LifetimeComponent *lifetime = es_get_component(entity, LifetimeComponent);
        lifetime->time_to_live -= dt;
    }

    if (es_has_component(entity, StatusEffectComponent)) {
        StatusEffectComponent *status_effects = es_get_component(entity, StatusEffectComponent);
        update_status_effects(status_effects, dt);
    }

    if (es_has_component(entity, AnimationComponent)) {
        AnimationComponent *anim_component = es_get_component(entity, AnimationComponent);
	ASSERT(anim_component->current_animation.animation_id != ANIM_NULL);
	anim_update_instance(world, entity, &anim_component->current_animation, dt);
    }

    if (es_has_component(entity, LightEmitter)) {
        LightEmitter *light = es_get_component(entity, LightEmitter);

        if (light->light.fading_out) {
	    ASSERT(light->light.fade_duration > 0.0f);
            light->light.time_elapsed += dt;
        }
    }

    if (entity_should_die(entity)) {
        // NOTE: won't be removed until after this frame
	world_kill_entity(world, entity);
    }
}

static void entity_render(Entity *entity, RenderBatches rbs,
    LinearArena *scratch, struct DebugState *debug_state, World *world)
{
    if (es_has_component(entity, AnimationComponent)) {
        AnimationComponent *anim_component = es_get_component(entity, AnimationComponent);
        AnimationInstance *anim_instance = &anim_component->current_animation;
	anim_render_instance(anim_instance, entity, rbs.world_rb, scratch);
    } else if (es_has_component(entity, SpriteComponent)) {
        SpriteComponent *sprite_comp = es_get_component(entity, SpriteComponent);
	Sprite *sprite = &sprite_comp->sprite;
        // TODO: how to handle if entity has both sprite and animation component?
	SpriteModifiers sprite_mods = sprite_get_modifiers(entity->direction, sprite->rotation_behaviour);

        Rectangle sprite_rect = { entity->position, sprite->size };
        draw_colored_sprite(rbs.world_rb, scratch, sprite->texture, sprite_rect, sprite_mods,
	    sprite->color, shader_handle(ASSET_TEXTURE_SHADER), RENDER_LAYER_ENTITIES);
    }

    if (es_has_component(entity, ColliderComponent) && debug_state->render_colliders) {
        ColliderComponent *collider = es_get_component(entity, ColliderComponent);

        Rectangle collider_rect = {
            .position = entity->position,
            .size = collider->size
        };

        draw_rectangle(rbs.worldspace_ui_rb, scratch, collider_rect, (RGBA32){0, 1, 0, 0.5f},
	    shader_handle(ASSET_SHAPE_SHADER), RENDER_LAYER_ENTITIES);
    }

    if (es_has_component(entity, ParticleSpawner)) {
        ParticleSpawner *ps = es_get_component(entity, ParticleSpawner);

        render_particle_spawner(world, ps, rbs, scratch);
    }

    if (es_has_component(entity, LightEmitter)) {
        LightEmitter *light = es_get_component(entity, LightEmitter);

        // TODO: this might look better if the origin is center of entity
        render_light_source(world, rbs.lighting_rb, entity->position, light->light, 1.0f, scratch);
    }

    if (debug_state->render_entity_bounds) {
        Rectangle entity_rect = world_get_entity_bounding_box(entity);

        draw_rectangle(rbs.worldspace_ui_rb, scratch, entity_rect, (RGBA32){1, 0, 1, 0.4f},
	    shader_handle(ASSET_SHAPE_SHADER), RENDER_LAYER_ENTITIES);
    }

    if (debug_state->render_entity_velocity) {
	Vector2 dir = v2_norm(entity->velocity);

	draw_line(rbs.worldspace_ui_rb, scratch, entity->position, v2_add(entity->position, v2_mul_s(dir, 20.0f)),
	    RGBA32_GREEN, 2.0f, shader_handle(ASSET_SHAPE_SHADER), 0);
    }
}

static void swap_and_reset_collision_tables(World *world)
{
    CollisionEventTable tmp = world->previous_frame_collisions;
    world->previous_frame_collisions = world->current_frame_collisions;
    world->current_frame_collisions = tmp;

    la_reset(&world->current_frame_collisions.arena);

    // TODO: this can just be a mem_zero to reset head and tail pointers
    for (ssize i = 0; i < world->current_frame_collisions.table_size; ++i) {
        CollisionEventList *list = &world->current_frame_collisions.table[i];
        list_clear(list);
    }
}

void world_add_trigger_cooldown(World *world, EntityID a, EntityID b, ComponentBitset component,
    RetriggerBehaviour retrigger_behaviour)
{
    add_trigger_cooldown(&world->trigger_cooldowns, a, b, component,
	retrigger_behaviour, &world->world_arena);
}

static void deal_damage_to_entity(World *world, Entity *entity, HealthComponent *hp, DamageInstance damage)
{
    Damage damage_taken = calculate_damage_received(entity, damage, &world->item_system);
    StatValue dmg_sum = calculate_damage_sum(damage_taken);
    ASSERT(dmg_sum >= 0);

    StatValue new_hp = hp->health.current_hitpoints - dmg_sum;
    set_current_health(&hp->health, new_hp);

    hitsplats_create(world, entity->position, damage_taken);
}

static void try_deal_damage_to_entity(World *world, Entity *receiver, Entity *sender, DamageInstance damage)
{
    (void)sender;

    HealthComponent *hp = es_get_component(receiver, HealthComponent);

    if (hp) {
        deal_damage_to_entity(world, receiver, hp, damage);
    }
}

static Rectangle get_entity_collider_rectangle(Entity *entity, ColliderComponent *collider)
{
    Rectangle result = {
        entity->position,
        collider->size
    };

    return result;
}

// TODO: move to trigger file?
static void invoke_entity_vs_entity_collision_triggers(World *world, Entity *self, Entity *other)
{
    EntityID self_id = es_get_id_of_entity(&world->entity_system, self);
    EntityID other_id = es_get_id_of_entity(&world->entity_system, other);
    ASSERT(!entity_id_equal(self_id, other_id));

    b32 same_faction = self->faction == other->faction;

    // Hostile triggers
    if (!same_faction) {
	if (should_invoke_trigger(world, self, other, DamageFieldComponent)) {
	    DamageFieldComponent *dmg_field = es_get_component(self, DamageFieldComponent);

	    try_deal_damage_to_entity(world, other, self, dmg_field->damage);

	    world_add_trigger_cooldown(world, self_id, other_id,
		component_flag(DamageFieldComponent), dmg_field->retrigger_behaviour);
	}

	if (should_invoke_trigger(world, self, other, EffectApplierComponent)) {
	    EffectApplierComponent *ea = es_get_component(self, EffectApplierComponent);

	    world_add_trigger_cooldown(world, self_id, other_id,
		component_flag(EffectApplierComponent), ea->retrigger_behaviour);
	}
    }
}

static f32 entity_vs_entity_collision(World *world, Entity *a,
    ColliderComponent *collider_a, Entity *b, ColliderComponent *collider_b,
    f32 movement_fraction_left, f32 dt)
{
    ASSERT(a);
    ASSERT(b);

    EntityID id_a = es_get_id_of_entity(&world->entity_system, a);
    EntityID id_b = es_get_id_of_entity(&world->entity_system, b);

    Rectangle rect_a = get_entity_collider_rectangle(a, collider_a);
    Rectangle rect_b = get_entity_collider_rectangle(b, collider_b);

    CollisionInfo collision = collision_rect_vs_rect(movement_fraction_left, rect_a, rect_b,
	a->velocity, b->velocity, dt);

    b32 same_collision_group = (collider_a->collision_group == collider_b->collision_group)
	&& (collider_a->collision_group != COLLISION_GROUP_NONE);

    if ((collision.collision_status != COLLISION_STATUS_NOT_COLLIDING)
	&& !same_collision_group && !entities_intersected_this_frame(world, id_a, id_b)) {
	b32 neither_collider_on_cooldown =
	    !trigger_is_on_cooldown(&world->trigger_cooldowns, id_a, id_b,
		component_flag(ColliderComponent))
	    && !trigger_is_on_cooldown(&world->trigger_cooldowns, id_b, id_a,
		component_flag(ColliderComponent));

	if (neither_collider_on_cooldown) {
	    execute_entity_vs_entity_collision_policy(world, a, b, collision, ENTITY_PAIR_INDEX_FIRST);
	    execute_entity_vs_entity_collision_policy(world, b, a, collision, ENTITY_PAIR_INDEX_SECOND);

	    invoke_entity_vs_entity_collision_triggers(world, a, b);
	    invoke_entity_vs_entity_collision_triggers(world, b, a);

	    movement_fraction_left = collision.movement_fraction_left;
	}

	// Only send collision events on first contact
	if (!entities_intersected_previous_frame(world, id_a, id_b)) {
	    if (a->faction != b->faction) {
		EventData event_data = event_data_hostile_collision(id_b);
		send_event_to_entity(a, event_data, world);
	    }
	}

	collision_event_table_insert(&world->current_frame_collisions, id_a, id_b);
    }

    return movement_fraction_left;
}

static Rectangle get_entity_collision_area(const Entity *entity, ColliderComponent *collider, f32 dt)
{
    Vector2 next_pos = v2_add(entity->position, v2_mul_s(entity->velocity, dt));

    f32 min_x = MIN(entity->position.x, next_pos.x);
    f32 max_x = MAX(entity->position.x, next_pos.x) + collider->size.x;
    f32 min_y = MIN(entity->position.y, next_pos.y);
    f32 max_y = MAX(entity->position.y, next_pos.y) + collider->size.y;

    f32 width = max_x - min_x;
    f32 height = max_y - min_y;

    Rectangle result = {{min_x, min_y}, {width, height}};

    return result;
}

static f32 entity_vs_tilemap_collision(Entity *entity, ColliderComponent *collider,
    World *world, f32 movement_fraction_left, f32 dt)
{
    Rectangle collision_area = get_entity_collision_area(entity, collider, dt);

    f32 min_x = collision_area.position.x;
    f32 max_x = min_x + collision_area.size.x;
    f32 min_y = collision_area.position.y;
    f32 max_y = min_y + collision_area.size.y;

    s32 min_tile_x = (s32)(min_x / TILE_SIZE);
    s32 max_tile_x = (s32)(max_x / TILE_SIZE);
    s32 min_tile_y = (s32)(min_y / TILE_SIZE);
    s32 max_tile_y = (s32)(max_y / TILE_SIZE);

    min_tile_x = MAX(min_tile_x, world->tilemap.min_x);
    max_tile_x = MIN(max_tile_x, world->tilemap.max_x);
    min_tile_y = MAX(min_tile_y, world->tilemap.min_y);
    max_tile_y = MIN(max_tile_y, world->tilemap.max_y);

    Vector2i collision_coords = {0};

    for (s32 y = min_tile_y - 1; y <= max_tile_y + 1; ++y) {
        for (s32 x = min_tile_x - 1; x <= max_tile_x + 1; ++x) {
            Vector2i tile_coords = {x, y};
            Tile *tile = tilemap_get_tile(&world->tilemap, tile_coords);

            if (!tile || (tile->type == TILE_WALL)) {
                Rectangle entity_rect = get_entity_collider_rectangle(entity, collider);
                Rectangle tile_rect = get_tile_rectangle_in_world_space(tile_coords);

                CollisionInfo collision = collision_rect_vs_rect(movement_fraction_left, entity_rect,
		    tile_rect, entity->velocity, V2_ZERO, dt);

                if (collision.collision_status != COLLISION_STATUS_NOT_COLLIDING) {
		    movement_fraction_left = collision.movement_fraction_left;

		    execute_entity_vs_tilemap_collision_policy(world, entity, collision);

		    EventData event_data = event_data_tilemap_collision(collision_coords);
		    send_event_to_entity(entity, event_data, world);
                }
            }
        }
    }

    ASSERT(movement_fraction_left >= 0.0f);

    return movement_fraction_left;
}

static void handle_collision_and_movement(World *world, f32 dt, LinearArena *frame_arena)
{
    // TODO: don't access alive entity array directly
    for (EntityIndex i = 0; i < world->alive_entity_count; ++i) {
        // TODO: this seems bug prone

        EntityID id_a = world->alive_entity_ids[i];
        Entity *a = es_get_entity(&world->entity_system, id_a);
        ColliderComponent *collider_a = es_get_component(a, ColliderComponent);
        ASSERT(a);

        f32 movement_fraction_left = 1.0f;

        if (collider_a) {
            movement_fraction_left = entity_vs_tilemap_collision(a, collider_a, world,
		movement_fraction_left, dt);
	    ASSERT(movement_fraction_left >= 0.0f);

            Rectangle collision_area = get_entity_collision_area(a, collider_a, dt);
            EntityIDList entities_in_area =
		qt_get_entities_in_area(&world->quad_tree, collision_area, frame_arena);

            for (EntityIDNode *node = list_head(&entities_in_area); node; node = list_next(node)) {
                if (!entity_id_equal(node->id, id_a)) {
                    Entity *b = es_get_entity(&world->entity_system, node->id);

                    ColliderComponent *collider_b = es_get_component(b, ColliderComponent);

                    if (collider_b) {
                        movement_fraction_left = entity_vs_entity_collision(world, a, collider_a,
                            b, collider_b, movement_fraction_left, dt);
                    }
                }
            }

            ASSERT(movement_fraction_left >= 0.0f);
            ASSERT(movement_fraction_left <= 1.0f);

            Vector2 to_move_this_frame = v2_mul_s(v2_mul_s(a->velocity, movement_fraction_left), dt);
            a->position = v2_add(a->position, to_move_this_frame);

	    f32 mag = v2_mag(a->velocity);

            if (mag > 0.5f) {
                a->direction = v2_norm(a->velocity);
            } else if (mag < 0.01f) {
		//a->velocity = V2_ZERO;
	    }
        }
    }
}

Entity *world_get_player_entity(World *world)
{
    Entity *result = es_try_get_entity(&world->entity_system, world->player_entity);

    return result;
}

void world_set_player_entity(World *world, EntityID id)
{
    ASSERT(es_entity_exists(&world->entity_system, id));
    world->player_entity = id;
}

void world_update(World *world, const FrameData *frame_data, LinearArena *frame_arena)
{
    if (world->alive_entity_count < 1) {
        return;
    }

    handle_collision_and_movement(world, frame_data->dt, frame_arena);

    // TODO: should any newly spawned entities be updated this frame?
    EntityIndex entity_count = world->alive_entity_count;

    for (EntityIndex i = 0; i < entity_count; ++i) {
        entity_update(world, i, frame_data->dt);
    }

    hitsplats_update(world, frame_data);

    update_trigger_cooldowns(world, frame_data->dt);

    swap_and_reset_collision_tables(world);

    // Remove inactive entities on end of frame
    for (ssize i = 0; i < world->alive_entity_count; ++i) {
	EntityID *id = &world->alive_entity_ids[i];
	Entity *entity = es_get_entity(&world->entity_system, *id);

	if (es_entity_is_inactive(entity)) {
	    world_remove_entity(world, i);
	    --i;
	}
    }

    // Make an extra pass over all entities alive at end of frame and update their
    // quad tree locations. This ensures that when this frame is rendered, any entities
    // that were spawned during the frame have the correct quad tree location and are therefore
    // rendered if they are on screen.
    for (ssize i = 0; i < world->alive_entity_count; ++i) {
        world_update_entity_quad_tree_location(world, i);
    }
}

void world_render(World *world, RenderBatches rb_list, const struct FrameData *frame_data,
    LinearArena *frame_arena, struct DebugState *debug_state)
{
    Rectangle visible_area = camera_get_visible_area(world->camera, frame_data->window_size);
    visible_area.position = v2_sub(visible_area.position, v2(TILE_SIZE, TILE_SIZE));
    visible_area.size = v2_add(visible_area.size, v2(TILE_SIZE * 2, TILE_SIZE * 2));

    Vector2i min_tile = world_to_tile_coords(rect_bottom_left(visible_area));
    Vector2i max_tile = world_to_tile_coords(rect_top_right(visible_area));

    s32 min_x = min_tile.x;
    s32 max_x = max_tile.x;
    s32 min_y = min_tile.y;
    s32 max_y = max_tile.y;

    for (s32 y = min_y; y <= max_y; ++y) {
        for (s32 x = min_x; x <= max_x; ++x) {
            Vector2i tile_coords = {x, y};
            Tile *tile = tilemap_get_tile(&world->tilemap, tile_coords);

            if (tile) {
                TextureHandle texture = {0};

                switch (tile->type) {
                    case TILE_FLOOR: {
                        texture = texture_handle(ASSET_FLOOR_TEXTURE);
                    } break;

                    case TILE_WALL: {
                        texture = texture_handle(ASSET_WALL_TEXTURE);
                    } break;

                    INVALID_DEFAULT_CASE;
                }

                Rectangle tile_rect = {
                    .position = tile_to_world_coords(tile_coords),
                    .size = { (f32)TILE_SIZE, (f32)TILE_SIZE }
                };

                RenderLayer layer = RENDER_LAYER_FLOORS;

                if (tile->type == TILE_WALL) {
                    tile_rect.size.y += TILE_SIZE;
                    layer = RENDER_LAYER_WALLS;
                }


                if (tile->type == TILE_FLOOR) {
                    draw_sprite(rb_list.world_rb, frame_arena, texture, tile_rect, (SpriteModifiers){0},
                        shader_handle(ASSET_TEXTURE_SHADER), layer);
                } else if (tile->type == TILE_WALL) {
                    Tile *tile_above = tilemap_get_tile(&world->tilemap,
                        (Vector2i){tile_coords.x, tile_coords.y + 1});
                    b32 can_walk_behind_tile = tile_above && (tile_above->type == TILE_FLOOR);

                    Rectangle bottom_segment = {
                        tile_rect.position,
                        {tile_rect.size.x, tile_rect.size.y / 2}
                    };

                    Rectangle top_segment = {
                        v2_add(bottom_segment.position, v2(0, bottom_segment.size.y)),
                        bottom_segment.size
                    };

                    EntityIDList entities_near_tile =
			qt_get_entities_in_area(&world->quad_tree, top_segment, frame_arena);

                    b32 make_wall_transparent = false;

                    for (EntityIDNode *node = list_head(&entities_near_tile); node; node = list_next(node)) {
                        Entity *entity = es_get_entity(&world->entity_system, node->id);

                        // TODO: only do this if it's the player?
                        // TODO: get sprite rect instead of all component bounds?
                        Rectangle entity_bounds = world_get_entity_bounding_box(entity);
                        b32 entity_intersects_wall = rect_intersects(entity_bounds, top_segment);

                        b32 entity_is_behind_wall = can_walk_behind_tile
                            && entity_intersects_wall
                            && (entity->position.y > tile_rect.position.y);

			b32 entity_is_visible = es_has_component(entity, AnimationComponent)
			    || es_has_component(entity, SpriteComponent);

                        if (entity_is_behind_wall && entity_is_visible) {
                            make_wall_transparent = true;
                            break;
                        }
                    }

                    draw_clipped_sprite(rb_list.world_rb, frame_arena, texture,
                        tile_rect, bottom_segment, RGBA32_WHITE, shader_handle(ASSET_TEXTURE_SHADER),
			RENDER_LAYER_FLOORS);

                    RGBA32 top_segment_color = RGBA32_WHITE;

                    // If entity is behind wall, make top segment slightly transparent
                    if (make_wall_transparent) {
                        top_segment_color.a = 0.5f;
                    }

                    draw_clipped_sprite(rb_list.world_rb, frame_arena, texture,
                        tile_rect, top_segment, top_segment_color, shader_handle(ASSET_TEXTURE_SHADER),
			RENDER_LAYER_WALLS);

                    if (!make_wall_transparent) {
                        // Render top segment to lighting stencil buffer so that wall top sides are never lit
                        draw_rectangle(rb_list.lighting_stencil_rb, frame_arena, top_segment, RGBA32_BLACK,
                            shader_handle(ASSET_SHAPE_SHADER), RENDER_LAYER_WALLS);
                    }

                } else {
                    ASSERT(0);
                }
            }
        }
    }

    // TODO: debug camera that is detached from regular camera
    EntityIDList entities_in_area = qt_get_entities_in_area(&world->quad_tree, visible_area, frame_arena);

    for (EntityIDNode *node = list_head(&entities_in_area); node; node = list_next(node)) {
        Entity *entity = es_get_entity(&world->entity_system, node->id);

        entity_render(entity, rb_list, frame_arena, debug_state, world);
    }

    hitsplats_render(world, rb_list.worldspace_ui_rb, frame_arena);

    if (debug_state->render_edge_list) {
        Allocator frame_alloc = la_allocator(frame_arena);
        EdgePool edge_pool = tilemap_get_edge_list(&world->tilemap, frame_alloc);

        for (ssize i = 0; i < edge_pool.count; ++i) {
            EdgeLine edge = edge_pool.items[i];
            Vector2 vec = cardinal_direction_vector(edge.direction);
            Vector2 origin = line_center(edge.line);
            Vector2 end = v2_add(origin, v2_mul_s(vec, 10.0f));

            draw_line(rb_list.worldspace_ui_rb, frame_arena, origin, end, RGBA32_GREEN, 4.0f,
                shader_handle(ASSET_SHAPE_SHADER), 0);

            draw_line(rb_list.worldspace_ui_rb, frame_arena, edge.line.start, edge.line.end, RGBA32_BLUE, 4.0f,
                shader_handle(ASSET_SHAPE_SHADER), 0);
        }
    }

#if 0
    TriangleFan fan = get_visibility_polygon(
        world_get_player_entity(world)->position, &world->tilemap, frame_arena);

    rb_push_triangle_fan(rb_list.overlay_rb, frame_arena, fan, rgba32(0.2f, 0.5f, 0.9f, 0.8f),
        get_asset_table()->shape_shader, RENDER_LAYER_OVERLAY);


    /* for (ssize i = 0; i < fan.count; ++i) { */
    /*     TriangleFanElement elem = fan.items[i]; */
    /*     Triangle triangle = {fan.center, elem.a, elem.b}; */

    /*     rb_push_triangle(rb, frame_arena, triangle, rgba32(0, 1, 0, 0.2f), */
    /*         get_asset_table()->shape_shader, RENDER_LAYER_OVERLAY); */

    /* } */
#endif
}

static void drop_ground_item(World *world, Vector2 pos, ItemID id)
{
    EntityWithID entity = world_spawn_entity(world, FACTION_NEUTRAL);
    entity.entity->position = pos;

    GroundItemComponent *ground_item = es_add_component(entity.entity, GroundItemComponent);
    ground_item->item_id = id;

    SpriteComponent *sprite = es_add_component(entity.entity, SpriteComponent);
    sprite->sprite = sprite_create(texture_handle(ASSET_FIREBALL_TEXTURE), v2(16, 16), SPRITE_ROTATE_NONE);
}

void world_initialize(World *world, FreeListArena *parent_arena)
{
    world->world_arena = la_create(fl_allocator(parent_arena), WORLD_ARENA_SIZE);

    es_initialize(&world->entity_system);
    item_sys_initialize(&world->item_system);

    world->previous_frame_collisions = collision_event_table_create(&world->world_arena);
    world->current_frame_collisions = collision_event_table_create(&world->world_arena);

    ring_initialize_static(&world->active_hitsplats);

    // NOTE: testing code
    s32 world_width = 16;
    s32 world_height = 16;

    {
	for (s32 x = 0; x < world_width; ++x) {
	    tilemap_insert_tile(&world->tilemap, (Vector2i){x, 0}, TILE_WALL,
		&world->world_arena);

	    tilemap_insert_tile(&world->tilemap, (Vector2i){x, world_height - 1}, TILE_WALL,
		&world->world_arena);
	}

	for (s32 y = 1; y < world_height - 1; ++y) {
	    tilemap_insert_tile(&world->tilemap, (Vector2i){0, y}, TILE_WALL,
		&world->world_arena);

	    tilemap_insert_tile(&world->tilemap, (Vector2i){world_width - 1, y}, TILE_WALL,
		&world->world_arena);
	}

	for (s32 y = 1; y < world_height - 1; ++y) {
	    for (s32 x = 1; x < world_width - 1; ++x) {
		tilemap_insert_tile(&world->tilemap, (Vector2i){x, y}, TILE_FLOOR,
		    &world->world_arena);
	    }
	}
    }

    tilemap_get_tile(&world->tilemap, (Vector2i){world_width / 2, world_height / 2})->type = TILE_WALL;

    Rectangle tilemap_area = tilemap_get_bounding_box(&world->tilemap);

    qt_initialize(&world->quad_tree, tilemap_area);

    for (s32 i = 0; i < 2; ++i) {
#if 1
        EntityWithID entity_with_id = world_spawn_entity(world,
	    i == 0 ? FACTION_PLAYER : FACTION_ENEMY);
        Entity *entity = entity_with_id.entity;
        entity->position = v2(128, 128);

        if (i == 0) {
            world_set_player_entity(world, entity_with_id.id);
        }

        ASSERT(!es_has_component(entity, ColliderComponent));

	if (i == 1) {
	    AIComponent *ai = es_add_component(entity, AIComponent);

	    ai->current_state.kind = AI_STATE_IDLE;
	}

        ColliderComponent *collider = es_add_component(entity, ColliderComponent);
        collider->size = v2(16.0f, 16.0f);
	set_collision_policy_vs_entities(collider, COLLISION_POLICY_STOP);
	set_collision_policy_vs_tilemaps(collider, COLLISION_POLICY_STOP);

        ASSERT(es_has_component(entity, ColliderComponent));

	SpellCasterComponent *spellcaster = es_add_component(entity, SpellCasterComponent);
	for (SpellID spell = 0; spell < SPELL_COUNT; ++spell) {
	    magic_add_to_spellbook(spellcaster, spell);
	}

#if 0
        SpriteComponent *sprite_comp = es_add_component(entity, SpriteComponent);
	Sprite *sprite = &sprite_comp->sprite;

        sprite->texture = asset_list->player_idle1;
        sprite->size = v2(32, 32);
	sprite->rotation_behaviour = SPRITE_ROTATE_BASED_ON_DIR;

#else

        AnimationComponent *anim = es_add_component(entity, AnimationComponent);
        anim->state_animations[ENTITY_STATE_IDLE] = ANIM_PLAYER_IDLE;
        anim->state_animations[ENTITY_STATE_WALKING] = ANIM_PLAYER_WALKING;
        anim->state_animations[ENTITY_STATE_ATTACKING] = ANIM_PLAYER_ATTACKING;

	entity_transition_to_state(world, entity, state_idle());
#endif

        StatsComponent *stats = es_add_component(entity, StatsComponent);
	stats->stats = create_base_stats();
	set_stat_value(&stats->stats, STAT_HEALTH, 1000);

	HealthComponent *hp = es_add_component(entity, HealthComponent);
	hp->health = create_health_instance(world, entity);

        // TODO: ensure components are zeroed out
        InventoryComponent *inventory = es_add_component(entity, InventoryComponent);

        EquipmentComponent *equipment = es_add_component(entity, EquipmentComponent);
        (void)equipment;

#if 1
        LightEmitter *light = es_add_component(entity, LightEmitter);
        light->light.radius = 500.0f;
        light->light.kind = LIGHT_RAYCASTED;

        if (i == 0) {
            light->light.color = RGBA32_GREEN;
        } else {
            light->light.color = RGBA32_BLUE;
        }
#endif

        {
            ItemWithID item_with_id = item_sys_create_item(&world->item_system);
            Item *item = item_with_id.item;

            item_sys_set_item_name(item, str_lit("Sword"), &world->world_arena);
            item_set_prop(item, ITEM_PROP_EQUIPPABLE | ITEM_PROP_HAS_MODIFIERS);
            ASSERT(item_has_prop(item, ITEM_PROP_EQUIPPABLE));
            item->equipment.slot = EQUIP_SLOT_WEAPON;

            // TODO: proper test case
            Item *test_item = item_sys_get_item(&world->item_system, item_with_id.id);
            ASSERT(item_has_prop(test_item, ITEM_PROP_EQUIPPABLE));
            ASSERT(str_equal(test_item->name, str_lit("Sword")));


#if 1
            Modifier dmg_mod = create_modifier(STAT_FIRE_DAMAGE, 100, NUMERIC_MOD_FLAT_ADDITIVE);
            item_add_modifier(item, dmg_mod);

	    Modifier dmg_mod2 = create_modifier(STAT_FIRE_DAMAGE, 150, NUMERIC_MOD_ADDITIVE_PERCENTAGE);
            item_add_modifier(item, dmg_mod2);
            //item_add_modifier(item, res_mod);
#endif

#if 0
	    drop_ground_item(world, v2(400, 400), item_with_id.id);
#else
            inventory_add_item(&inventory->inventory, item_with_id.id);
	    equip_item_from_inventory(&world->item_system, &equipment->equipment,
		&inventory->inventory, item_with_id.id);
#endif
        }
#endif
    }
}

void world_destroy(World *world)
{
    la_destroy(&world->world_arena);
}
