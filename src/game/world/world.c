#include "world.h"
#include "base/rgba.h"
#include "base/utils.h"
#include "collision/collision.h"
#include "collision/collision_event.h"
#include "collision/collider.h"
#include "components/chain.h"
#include "components/component.h"
#include "components/particle_spawner.h"
#include "entity/entity_system.h"
#include "game.h"
#include "renderer/frontend/render_batch.h"
#include "asset_table.h"
#include "world/chunk.h"

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

Rectangle world_get_entity_bounding_box(Entity *entity, PhysicsComponent *physics)
{
    ASSERT(physics);

    Vector2 size = {1, 1};

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

    Rectangle result = {physics->position, size};

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

    PhysicsComponent *physics = es_get_component(entity, PhysicsComponent);
    QuadTreeLocation *loc = &world->alive_entity_quad_tree_locations[alive_entity_index];

    if (physics) {
        Rectangle entity_area = world_get_entity_bounding_box(entity, physics);

        *loc = qt_set_entity_area(&world->quad_tree, id, *loc, entity_area, &world->world_arena);
    } else if (!qt_location_is_null(*loc)) {
        // Entity doesn't have PhysicsComponent but still exists in quad tree, remove it
        *loc = qt_remove_entity(&world->quad_tree, id, *loc);
    }
}

EntityWithID world_spawn_non_spatial_entity(World *world, EntityFaction faction)
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
EntityWithID world_spawn_entity(World *world, Vector2 position, EntityFaction faction)
{
    EntityWithID result = world_spawn_non_spatial_entity(world, faction);
    PhysicsComponent *physics = es_add_component(result.entity, PhysicsComponent);
    physics->position = position;

    return result;
}

// Handles anything that needs to be handled before removing an entity,
// such as transferring components that need to stay alive a little longer
// over to a new entity.
static void handle_entity_removal_side_effects(World *world, EntityID id, LinearArena *frame_arena)
{
    Entity *dying_entity = es_get_entity(&world->entity_system, id);
    PhysicsComponent *dying_entity_physics = es_get_component(dying_entity, PhysicsComponent);

    // If the entity is non-spatial, there is (currently) nothing for us to do here
    if (dying_entity_physics) {
        Rectangle dying_entity_bounds = world_get_entity_bounding_box(dying_entity, dying_entity_physics);

        if (es_has_component(dying_entity, LightEmitter)) {
            // If an entity dies while it has a light, the light is transferred
            // to a new entity and the light starts fading out
            LightEmitter *old_light = es_get_component(dying_entity, LightEmitter);

            b32 should_transfer_light = !old_light->light.fading_out
                || (old_light->light.time_elapsed < old_light->light.fade_duration);

            if (should_transfer_light) {
                Vector2 new_entity_pos = get_light_origin_position(dying_entity_bounds);
                Vector2i tile_coords = world_to_tile_coords(new_entity_pos);
                Tile *tile = tilemap_get_tile(&world->tilemap, tile_coords);

                if (!tile || (tile->type == TILE_WALL)) {
                    // If the light origin lands inside a wall, displace it to closest floor tile
                    Rectangle tile_rect = get_tile_rectangle_in_world_space(tile_coords);
                    RectanglePoint closest_point = rect_bounds_point_closest_to_point(
                        tile_rect, new_entity_pos);

                    // The entity will be placed on the perimeter of the tile it was spawned in,
                    // which means it will be considered to be inside it, so we offset it slightly
                    new_entity_pos = v2_sub(closest_point.point, v2_norm(dying_entity_physics->velocity));
                }

                Entity *new_entity = world_spawn_entity(world, new_entity_pos, FACTION_NEUTRAL).entity;
                LightEmitter *new_light = es_add_component(new_entity, LightEmitter);
                *new_light = *old_light;

                f32 light_fade_duration = new_light->light.fade_duration;

                if (light_fade_duration == 0.0f) {
                    light_fade_duration = LIGHT_DEFAULT_FADE_OUT_TIME;
                }

                new_light->light.fade_duration = light_fade_duration;

                LifetimeComponent *lt = es_add_component(new_entity, LifetimeComponent);
                lt->time_to_live = light_fade_duration;

                new_light->light.fading_out = true;
            }
        }
    }

    if (es_has_component(dying_entity, ChainComponent)) {
	// If an entity is part of a chain, we kill all next and previous entities in the chain too
	// NOTE: The next and previous entities will only be marked for removal and therefore there
	// is no guarantee that they will be removed this frame, so don't depend on that
	Entity *curr = 0;
	ChainComponent *chain = es_get_component(dying_entity, ChainComponent);

	// Remove entities before us in chain
	curr = get_previous_entity_in_chain(&world->entity_system, chain);
	while (curr) {
	    ChainComponent *curr_link = es_get_component(curr, ChainComponent);
	    Entity *prev = get_previous_entity_in_chain(&world->entity_system, curr_link);

	    remove_link_from_chain(&world->entity_system, curr_link);
	    world_kill_entity(world, curr, frame_arena);

	    curr = prev;
	}

        // Remove entities after us in chain
	curr = get_next_entity_in_chain(&world->entity_system, chain);
	while (curr) {
	    ChainComponent *curr_link = es_get_component(curr, ChainComponent);
	    Entity *next = get_next_entity_in_chain(&world->entity_system, curr_link);

	    remove_link_from_chain(&world->entity_system, curr_link);
	    world_kill_entity(world, curr, frame_arena);

	    curr = next;
	}
    }
}

static void world_remove_entity(World *world, ssize alive_entity_index, LinearArena *frame_arena)
{
    ASSERT(alive_entity_index < world->alive_entity_count);

    EntityID *id = &world->alive_entity_ids[alive_entity_index];

    handle_entity_removal_side_effects(world, *id, frame_arena);

    es_remove_entity(&world->entity_system, *id);

    QuadTreeLocation *qt_location = &world->alive_entity_quad_tree_locations[alive_entity_index];

    if (!qt_location_is_null(*qt_location)) {
        qt_remove_entity(&world->quad_tree, *id, *qt_location);
    }

    ssize last_index = world->alive_entity_count - 1;
    *qt_location = world->alive_entity_quad_tree_locations[last_index];

    *id = world->alive_entity_ids[last_index];

    --world->alive_entity_count;
}

void world_kill_entity(World *world, Entity *entity, LinearArena *frame_arena)
{
    EventData death_event = event_data_death();
    send_event_to_entity(entity, death_event, world, frame_arena);

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

static void deal_damage_to_entity(World *world, Entity *entity, HealthComponent *hp, DamageInstance damage)
{
    Damage damage_taken = calculate_damage_received(&world->entity_system, entity, damage);
    StatValue dmg_sum = calculate_damage_sum(damage_taken);
    ASSERT(dmg_sum >= 0);

    StatValue new_hp = hp->health.current_hitpoints - dmg_sum;
    set_current_health(&hp->health, new_hp);

    PhysicsComponent *physics = es_get_component(entity, PhysicsComponent);
    ASSERT(physics && "I guess a non-spatial entity could take damage?");

    if (physics) {
        hitsplats_create(world, physics->position, damage_taken);
    }
}

static void try_deal_damage_to_entity(World *world, Entity *receiver, Entity *sender, DamageInstance damage)
{
    (void)sender;

    HealthComponent *hp = es_get_component(receiver, HealthComponent);

    if (hp) {
        deal_damage_to_entity(world, receiver, hp, damage);
    }
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

        if (has_flag(ps->config.flags, PS_FLAG_WHEN_DONE_REMOVE_ENTITY)
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

static void entity_update(World *world, ssize alive_entity_index, f32 dt, LinearArena *frame_arena)
{
    EntityID id = world->alive_entity_ids[alive_entity_index];
    Entity *entity = es_get_entity(&world->entity_system, id);

    if (es_has_components(entity, component_id(ArcingComponent) | component_id(PhysicsComponent))) {
        ArcingComponent *arcing = es_get_component(entity, ArcingComponent);
        PhysicsComponent *physics = es_get_component(entity, PhysicsComponent);
        Vector2 entity_center = rect_center(world_get_entity_bounding_box(entity, physics));

        Entity *target = es_try_get_entity(&world->entity_system, arcing->target_entity);
        Vector2 target_pos = {0};

        b32 found = false;

        if (target) {
            PhysicsComponent *target_physics = es_get_component(target, PhysicsComponent);

            // TODO: make it possible to call get_component on null entity
            if (target_physics) {
                target_pos = rect_center(world_get_entity_bounding_box(target, target_physics));
                arcing->last_known_target_position = target_pos;

                found = true;
            }
        }

        if (!found) {
            target_pos = arcing->last_known_target_position;
        }

        Vector2 dir = v2_norm(v2_sub(target_pos, entity_center));
        Vector2 next_velocity = v2_mul_s(dir, arcing->travel_speed);
        Vector2 next_center_position = v2_add(entity_center, v2_mul_s(next_velocity, dt));
        f32 dist_to_target = v2_dist_sq(entity_center, target_pos);
        f32 dist_to_next_pos = v2_dist_sq(entity_center, next_center_position);

        if (dist_to_target < 100.0f) {
            physics->velocity = V2_ZERO;

            if (target) {
                try_deal_damage_to_entity(world, target, entity, arcing->damage_on_target_reached);
            }

            es_remove_component(entity, ArcingComponent);
        } else if (dist_to_target > dist_to_next_pos) {
            physics->velocity = next_velocity;
        } else {
            f32 x = dist_to_target / dist_to_next_pos;
            physics->velocity = v2_mul_s(next_velocity, x);
        }
    }

    if (es_has_component(entity, HealthComponent)) {
	HealthComponent *hp = es_get_component(entity, HealthComponent);
	StatValue max_hp = get_total_stat_value(&world->entity_system, entity, STAT_HEALTH);
	set_max_health(&hp->health, max_hp);
    }

    if (es_has_component(entity, AIComponent)) {
	AIComponent *ai = es_get_component(entity, AIComponent);
	entity_update_ai(world, entity, ai);
    }

    if (es_has_components(entity, component_id(ParticleSpawner) | component_id(PhysicsComponent))) {
        ParticleSpawner *ps = es_get_component(entity, ParticleSpawner);
        PhysicsComponent *physics = es_get_component(entity, PhysicsComponent);

        update_particle_spawner(world, entity, ps, physics, dt);
    }

    if (es_has_component(entity, LifetimeComponent)) {
        LifetimeComponent *lifetime = es_get_component(entity, LifetimeComponent);
        lifetime->time_to_live -= dt;
    }

    if (es_has_component(entity, StatusEffectComponent)) {
        StatusEffectComponent *status_effects = es_get_component(entity, StatusEffectComponent);
        update_status_effects(status_effects, dt);
    }

    if (es_has_components(entity, component_id(AnimationComponent) | component_id(PhysicsComponent))) {
        AnimationComponent *anim_component = es_get_component(entity, AnimationComponent);
        PhysicsComponent *physics = es_get_component(entity, PhysicsComponent);
	ASSERT(anim_component->current_animation.animation_id != ANIM_NULL);

	anim_update_instance(world, entity, physics, &anim_component->current_animation, dt);
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
	world_kill_entity(world, entity, frame_arena);
    }
}

static void entity_render(Entity *entity, RenderBatches rbs,
    LinearArena *scratch, struct DebugState *debug_state, World *world)
{
    PhysicsComponent *physics = es_get_component(entity, PhysicsComponent);

    if (!physics) {
        // Can't render a non-spatial entity
        return;
    }

    if (es_has_component(entity, AnimationComponent)) {
        AnimationComponent *anim_component = es_get_component(entity, AnimationComponent);
        AnimationInstance *anim_instance = &anim_component->current_animation;

	anim_render_instance(anim_instance, physics, rbs.world_rb, scratch);
    } else if (es_has_component(entity, SpriteComponent)) {
        SpriteComponent *sprite_comp = es_get_component(entity, SpriteComponent);
	Sprite *sprite = &sprite_comp->sprite;
        // TODO: how to handle if entity has both sprite and animation component?
	SpriteModifiers sprite_mods = sprite_get_modifiers(physics->direction, sprite->rotation_behaviour);

        Rectangle sprite_rect = { physics->position, sprite->size };
        draw_colored_sprite(rbs.world_rb, scratch, sprite->texture, sprite_rect, sprite_mods,
	    sprite->color, shader_handle(TEXTURE_SHADER), RENDER_LAYER_ENTITIES);
    }

    if (es_has_component(entity, ChainComponent) && debug_state->render_chain_links) {
        // TODO: make it possible to customize how chains are rendered
        ChainComponent *chain = es_get_component(entity, ChainComponent);
        Entity *next_link = get_next_entity_in_chain(&world->entity_system, chain);

        if (next_link) {
            PhysicsComponent *next_link_physics = es_get_component(next_link, PhysicsComponent);

            if (next_link_physics) {
                draw_line(rbs.worldspace_ui_rb, scratch, physics->position, next_link_physics->position,
                    RGBA32_WHITE, 4.0f, shader_handle(SHAPE_SHADER), 0);
            }
        }
    }

    if (es_has_component(entity, ColliderComponent) && debug_state->render_colliders) {
        ColliderComponent *collider = es_get_component(entity, ColliderComponent);

        Rectangle collider_rect = {
            .position = physics->position,
            .size = collider->size
        };

        draw_rectangle(rbs.worldspace_ui_rb, scratch, collider_rect, (RGBA32){0, 1, 0, 0.5f},
	    shader_handle(SHAPE_SHADER), RENDER_LAYER_ENTITIES);
    }

    if (es_has_component(entity, LightEmitter)) {
        LightEmitter *light = es_get_component(entity, LightEmitter);
        Rectangle entity_bounds = world_get_entity_bounding_box(entity, physics);

        Vector2 light_origin = get_light_origin_position(entity_bounds);

        // TODO: this might look better if the origin is center of entity
        render_light_source(world, rbs.lighting_rb, light_origin, light->light, 1.0f, scratch);
    }

    if (debug_state->render_entity_bounds) {
        Rectangle entity_rect = world_get_entity_bounding_box(entity, physics);
        f32 minimum_bounds_rect_size = 4.0f;

        entity_rect.size.x = MAX(entity_rect.size.x, minimum_bounds_rect_size);
        entity_rect.size.y = MAX(entity_rect.size.y, minimum_bounds_rect_size);

        draw_rectangle(rbs.worldspace_ui_rb, scratch, entity_rect, (RGBA32){1, 0, 1, 0.4f},
	    shader_handle(SHAPE_SHADER), RENDER_LAYER_ENTITIES);
    }

    if (debug_state->render_entity_velocity) {
	Vector2 dir = v2_norm(physics->velocity);

	draw_line(rbs.worldspace_ui_rb, scratch, physics->position,
	    v2_add(physics->position, v2_mul_s(dir, 20.0f)),
            RGBA32_GREEN, 2.0f, shader_handle(SHAPE_SHADER), 0);
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


static Rectangle get_entity_collider_rectangle(ColliderComponent *collider, PhysicsComponent *physics)
{
    Rectangle result = {
        physics->position,
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
		component_id(DamageFieldComponent), dmg_field->retrigger_behaviour);
	}

	if (should_invoke_trigger(world, self, other, EffectApplierComponent)) {
	    EffectApplierComponent *ea = es_get_component(self, EffectApplierComponent);

	    world_add_trigger_cooldown(world, self_id, other_id,
		component_id(EffectApplierComponent), ea->retrigger_behaviour);
	}
    }
}

// TODO: reduce number of parameters
static f32 entity_vs_entity_collision(World *world,
    Entity *a, ColliderComponent *collider_a, PhysicsComponent *physics_a,
    Entity *b, ColliderComponent *collider_b, PhysicsComponent *physics_b,
    f32 movement_fraction_left, f32 dt, LinearArena *frame_arena)
{
    ASSERT(a);
    ASSERT(b);

    EntityID id_a = es_get_id_of_entity(&world->entity_system, a);
    EntityID id_b = es_get_id_of_entity(&world->entity_system, b);

    Rectangle rect_a = get_entity_collider_rectangle(collider_a, physics_a);
    Rectangle rect_b = get_entity_collider_rectangle(collider_b, physics_b);

    CollisionInfo collision = collision_rect_vs_rect(movement_fraction_left, rect_a, rect_b,
	physics_a->velocity, physics_b->velocity, dt);

    b32 same_collision_group = (collider_a->collision_group == collider_b->collision_group)
	&& (collider_a->collision_group != COLLISION_GROUP_NONE);

    if ((collision.collision_status != COLLISION_STATUS_NOT_COLLIDING)
	&& !same_collision_group && !entities_intersected_this_frame(world, id_a, id_b)) {
	b32 neither_collider_on_cooldown =
	    !trigger_is_on_cooldown(&world->trigger_cooldowns, id_a, id_b,
		component_id(ColliderComponent))
	    && !trigger_is_on_cooldown(&world->trigger_cooldowns, id_b, id_a,
		component_id(ColliderComponent));

	if (neither_collider_on_cooldown) {
	    execute_entity_vs_entity_collision_policy(world, a, physics_a, collider_a,
                b, collider_b, collision, ENTITY_PAIR_INDEX_FIRST, frame_arena);
	    execute_entity_vs_entity_collision_policy(world, b, physics_b, collider_b,
                a, collider_a, collision, ENTITY_PAIR_INDEX_SECOND, frame_arena);

	    invoke_entity_vs_entity_collision_triggers(world, a, b);
	    invoke_entity_vs_entity_collision_triggers(world, b, a);

	    movement_fraction_left = collision.movement_fraction_left;
	}

	// Only send collision events on first contact
	if (!entities_intersected_previous_frame(world, id_a, id_b)) {
	    if (a->faction != b->faction) {
		EventData event_data = event_data_hostile_collision(id_b);
		send_event_to_entity(a, event_data, world, frame_arena);
	    }
	}

	collision_event_table_insert(&world->current_frame_collisions, id_a, id_b);
    }

    return movement_fraction_left;
}

static Rectangle get_entity_collision_area(ColliderComponent *collider, PhysicsComponent *physics, f32 dt)
{
    Vector2 next_pos = v2_add(physics->position, v2_mul_s(physics->velocity, dt));

    f32 min_x = MIN(physics->position.x, next_pos.x);
    f32 max_x = MAX(physics->position.x, next_pos.x) + collider->size.x;
    f32 min_y = MIN(physics->position.y, next_pos.y);
    f32 max_y = MAX(physics->position.y, next_pos.y) + collider->size.y;

    f32 width = max_x - min_x;
    f32 height = max_y - min_y;

    Rectangle result = {{min_x, min_y}, {width, height}};

    return result;
}

static f32 entity_vs_tilemap_collision(Entity *entity, ColliderComponent *collider,
    PhysicsComponent *physics, World *world, f32 movement_fraction_left, f32 dt, LinearArena *frame_arena)
{
    Rectangle collision_area = get_entity_collision_area(collider, physics, dt);

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
                Rectangle entity_rect = get_entity_collider_rectangle(collider, physics);
                Rectangle tile_rect = get_tile_rectangle_in_world_space(tile_coords);

                CollisionInfo collision = collision_rect_vs_rect(movement_fraction_left, entity_rect,
		    tile_rect, physics->velocity, V2_ZERO, dt);

                if (collision.collision_status != COLLISION_STATUS_NOT_COLLIDING) {
		    movement_fraction_left = collision.movement_fraction_left;

		    execute_entity_vs_tilemap_collision_policy(world, entity, physics, collider,
                        collision, frame_arena);

		    EventData event_data = event_data_tilemap_collision(collision_coords);
		    send_event_to_entity(entity, event_data, world, frame_arena);
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
        ASSERT(a);

        ColliderComponent *collider_a = es_get_component(a, ColliderComponent);
        PhysicsComponent *physics_a = es_get_component(a, PhysicsComponent);

        f32 movement_fraction_left = 1.0f;

        if (collider_a && physics_a) {
            movement_fraction_left = entity_vs_tilemap_collision(a, collider_a, physics_a, world,
		movement_fraction_left, dt, frame_arena);
	    ASSERT(movement_fraction_left >= 0.0f);

            Rectangle collision_area = get_entity_collision_area(collider_a, physics_a, dt);
            EntityIDList entities_in_area =
		qt_get_entities_in_area(&world->quad_tree, collision_area, frame_arena);

            for (EntityIDNode *node = list_head(&entities_in_area); node; node = list_next(node)) {
                if (!entity_id_equal(node->id, id_a)) {
                    Entity *b = es_get_entity(&world->entity_system, node->id);

                    ColliderComponent *collider_b = es_get_component(b, ColliderComponent);
                    PhysicsComponent *physics_b = es_get_component(b, PhysicsComponent);

                    if (collider_b && physics_b) {
                        movement_fraction_left = entity_vs_entity_collision(world, a, collider_a,
                            physics_a, b, collider_b, physics_b, movement_fraction_left,
                            dt, frame_arena);
                    }
                }
            }

            ASSERT(movement_fraction_left >= 0.0f);
            ASSERT(movement_fraction_left <= 1.0f);

            Vector2 to_move_this_frame = v2_mul_s(v2_mul_s(physics_a->velocity, movement_fraction_left), dt);
            physics_a->position = v2_add(physics_a->position, to_move_this_frame);

	    f32 mag = v2_mag(physics_a->velocity);

            if (mag > 0.5f) {
                physics_a->direction = v2_norm(physics_a->velocity);
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

static Rectangle get_area_visible_to_player(World *world, const FrameData *frame_data)
{
    Rectangle result = camera_get_visible_area(world->camera, frame_data->window_size);

    return result;
}

static Rectangle get_area_to_update_and_render(World *world, const FrameData *frame_data)
{
    f32 margin = (f32)TILE_SIZE * 8.0f;

    Rectangle result = get_area_visible_to_player(world, frame_data);
    result.position = v2_sub(result.position, v2(margin, margin));
    result.size = v2_add(result.size, v2(margin * 2, margin * 2));

    return result;
}

void world_update(World *world, const FrameData *frame_data, LinearArena *frame_arena)
{
    if (world->alive_entity_count < 1) {
        return;
    }

    // TODO: only update in certain area around player
    handle_collision_and_movement(world, frame_data->dt, frame_arena);

    // TODO: should any newly spawned entities be updated this frame?
    EntityIndex entity_count = world->alive_entity_count;

    for (EntityIndex i = 0; i < entity_count; ++i) {
        entity_update(world, i, frame_data->dt, frame_arena);
    }

    hitsplats_update(world, frame_data);

    ChunkPtrArray visible_chunks = get_chunks_in_area(&world->map_chunks,
        get_area_to_update_and_render(world, frame_data), frame_arena);

    for (ssize i = 0; i < visible_chunks.count; ++i) {
        Chunk *chunk = visible_chunks.chunks[i];

        update_particle_buffers(&chunk->particle_buffers, frame_data->dt);
    }

    update_trigger_cooldowns(world, frame_data->dt);

    swap_and_reset_collision_tables(world);

    // Remove inactive entities on end of frame
    for (ssize i = 0; i < world->alive_entity_count; ++i) {
	EntityID *id = &world->alive_entity_ids[i];
	Entity *entity = es_get_entity(&world->entity_system, *id);

	if (es_entity_is_inactive(entity)) {
	    world_remove_entity(world, i, frame_arena);
	    --i;
	}
    }

    /* Make an extra pass over all entities alive at end of frame and update their
       quad tree locations. This ensures that when this frame is rendered, any entities
       that were spawned during the frame have the correct quad tree location and are therefore
       rendered if they are on screen.
    */
    for (ssize i = 0; i < world->alive_entity_count; ++i) {
        world_update_entity_quad_tree_location(world, i);
    }
}

static void render_tilemap(World *world, RenderBatches rb_list, const FrameData *frame_data,
    LinearArena *frame_arena)
{
    Rectangle tilemap_render_area = get_area_visible_to_player(world, frame_data);
    tilemap_render_area.position = v2_sub(tilemap_render_area.position, v2(TILE_SIZE, TILE_SIZE));
    tilemap_render_area.size = v2_add(tilemap_render_area.size, v2(TILE_SIZE * 2, TILE_SIZE * 2));

    Vector2i min_tile = world_to_tile_coords(rect_bottom_left(tilemap_render_area));
    Vector2i max_tile = world_to_tile_coords(rect_top_right(tilemap_render_area));

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
                        texture = texture_handle(FLOOR_TEXTURE);
                    } break;

                    case TILE_WALL: {
                        texture = texture_handle(WALL_TEXTURE);
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
                        shader_handle(TEXTURE_SHADER), layer);
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
                        PhysicsComponent *physics = es_get_component(entity, PhysicsComponent);

                        // Entity could technically exist (I think) in quad tree despite not having
                        // a physics component if it was removed mid-frame and we haven't updated all
                        // quad tree locations yet, so we probably just ignore it
                        ASSERT(physics);
                        if (physics) {
                            Rectangle entity_bounds = world_get_entity_bounding_box(entity, physics);
                            b32 entity_intersects_wall = rect_intersects(entity_bounds, top_segment);

                            b32 entity_is_behind_wall = can_walk_behind_tile
                                && entity_intersects_wall
                                && (physics->position.y > tile_rect.position.y);

                            b32 entity_is_visible = es_has_component(entity, AnimationComponent)
                                || es_has_component(entity, SpriteComponent);

                            if (entity_is_behind_wall && entity_is_visible) {
                                make_wall_transparent = true;
                                break;
                            }
                        }
                        // TODO: only do this if it's the player?
                        // TODO: get sprite rect instead of all component bounds?
                    }

                    draw_clipped_sprite(rb_list.world_rb, frame_arena, texture,
                        tile_rect, bottom_segment, RGBA32_WHITE, shader_handle(TEXTURE_SHADER),
			RENDER_LAYER_FLOORS);

                    RGBA32 top_segment_color = RGBA32_WHITE;

                    // If entity is behind wall, make top segment slightly transparent
                    if (make_wall_transparent) {
                        top_segment_color.a = 0.5f;
                    }

                    draw_clipped_sprite(rb_list.world_rb, frame_arena, texture,
                        tile_rect, top_segment, top_segment_color, shader_handle(TEXTURE_SHADER),
			RENDER_LAYER_WALLS);

                    if (!make_wall_transparent) {
                        // Render top segment to lighting stencil buffer so that wall top sides are never lit,
                        // unless the wall is transparent
                        draw_rectangle(rb_list.lighting_stencil_rb, frame_arena, top_segment, RGBA32_BLACK,
                            shader_handle(SHAPE_SHADER), RENDER_LAYER_WALLS);
                    }

                } else {
                    ASSERT(0);
                }
            }
        }
    }

}

void world_render(World *world, RenderBatches rb_list, const FrameData *frame_data,
    LinearArena *frame_arena, struct DebugState *debug_state)
{
    Rectangle render_area = get_area_to_update_and_render(world, frame_data);

    // TODO: move this and similar things to game.c
    if (debug_state->render_camera_bounds) {
        Rectangle visible_area = get_area_visible_to_player(world, frame_data);

        draw_outlined_rectangle(rb_list.worldspace_ui_rb, frame_arena, visible_area,
            RGBA32_GREEN, 4.0f / (1.0f + world->camera.zoom), shader_handle(SHAPE_SHADER), 0);

        draw_outlined_rectangle(rb_list.worldspace_ui_rb, frame_arena, render_area,
            RGBA32_RED, 4.0f / (1.0f + world->camera.zoom), shader_handle(SHAPE_SHADER), 0);
    }

    render_tilemap(world, rb_list, frame_data, frame_arena);

    ChunkPtrArray visible_chunks = get_chunks_in_area(&world->map_chunks, render_area, frame_arena);
    for (ssize i = 0; i < visible_chunks.count; ++i) {
        Chunk *chunk = visible_chunks.chunks[i];

        render_particle_buffers(&chunk->particle_buffers, rb_list, frame_arena);
    }

    EntityIDList entities_in_area = qt_get_entities_in_area(&world->quad_tree, render_area, frame_arena);

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
                shader_handle(SHAPE_SHADER), 0);

            draw_line(rb_list.worldspace_ui_rb, frame_arena, edge.line.start, edge.line.end,
		RGBA32_BLUE, 4.0f,
                shader_handle(SHAPE_SHADER), 0);
        }
    }
}

void world_initialize(World *world, FreeListArena *parent_arena)
{
    world->world_arena = la_create(fl_allocator(parent_arena), WORLD_ARENA_SIZE);

    es_initialize(&world->entity_system);

    world->previous_frame_collisions = collision_event_table_create(&world->world_arena);
    world->current_frame_collisions = collision_event_table_create(&world->world_arena);

    ring_initialize_static(&world->active_hitsplats);

    tilemap_initialize(&world->tilemap);

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

    //tilemap_get_tile(&world->tilemap, (Vector2i){world_width / 2, world_height / 2})->type = TILE_WALL;

    Rectangle tilemap_area = tilemap_get_bounding_box(&world->tilemap);

    qt_initialize(&world->quad_tree, tilemap_area);

    for (s32 i = 0; i < 3; ++i) {
#if 1
        EntityWithID entity_with_id = world_spawn_entity(world, v2(128 * (f32)(i + 1), 128 * (f32)(i + 1)),
	    i == 0 ? FACTION_PLAYER : FACTION_ENEMY);
        Entity *entity = entity_with_id.entity;

        PhysicsComponent *physics = es_get_component(entity, PhysicsComponent);

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

	entity_transition_to_state(world, entity, physics, state_idle());
#endif

        StatsComponent *stats = es_add_component(entity, StatsComponent);
	stats->stats = create_base_stats();
	set_stat_value(&stats->stats, STAT_HEALTH, 10000000);

	HealthComponent *hp = es_add_component(entity, HealthComponent);
	hp->health = create_health_instance(world, entity);

#if 1
        LightEmitter *light = es_add_component(entity, LightEmitter);
        light->light.radius = 500.0f;
        light->light.kind = LIGHT_RAYCASTED;

        if (true || i == 0) {
            light->light.color = RGBA32_GREEN;
        } else {
            light->light.color = RGBA32_BLUE;
        }
#endif

#if 0
        if (i == 0) {
            ParticleSpawnerNew *ps = es_get_or_add_component(entity, ParticleSpawnerNew);
            ps->config = (ParticleSpawnerConfig) {
                .kind = PS_SPAWN_DISTRIBUTED,
                .particle_color = rgba32(1, 0, 0.1f, 0.05f),
                .particle_size = 4.0f,
                .particle_lifetime = 2.0f,
                .particle_speed = 30.0f,
                .infinite = true,
                .particles_per_second = 100,
                .emits_light = true,
                .light_source = (LightSource){LIGHT_REGULAR, 2.5f, RGBA32_RED, false, 0, 0}
            };
        } else {
            ParticleSpawner *ps = es_get_or_add_component(entity, ParticleSpawner);
            ParticleSpawnerConfig config = {
                .kind = PS_SPAWN_DISTRIBUTED,
                .particle_color = rgba32(1, 0, 0.1f, 0.05f),
                .particle_size = 4.0f,
                .particle_lifetime = 2.0f,
                .particle_speed = 30.0f,
                .infinite = true,
                .particles_per_second = 100,
                .emits_light = true,
                .light_source = (LightSource){LIGHT_REGULAR, 2.5f, RGBA32_RED, false, 0, 0}
            };

            particle_spawner_initialize(ps, config);
        }
#endif

        {
            /*Inventory *inv = */es_add_component(entity, Inventory);
            if (i != 0) {
                /*Inventory *inv_storable = */es_add_component(entity, InventoryStorable);
                Equippable *equippable = es_add_component(entity, Equippable);
                equippable->equippable_in_slot = EQUIP_SLOT_WEAPON;

		ItemModifiers *mods = es_add_component(entity, ItemModifiers);
		add_item_modifier(mods, create_modifier(STAT_FIRE_DAMAGE, 1000, NUMERIC_MOD_FLAT_ADDITIVE));

		NameComponent *name = es_add_component(entity, NameComponent);
		*name = name_component(str_lit("Item name"));
            }

            /*Equipment *eq =*/ es_add_component(entity, Equipment);
        }
#endif
    }

    // NOTE: be sure to only create these AFTER tilemap is finished
    world->map_chunks = create_chunks_for_tilemap(&world->tilemap, &world->world_arena);
}

void world_destroy(World *world)
{
    la_destroy(&world->world_arena);
}
