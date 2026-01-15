#include "world.h"
#include "base/linear_arena.h"
#include "collision_event.h"
#include "components/collider.h"
#include "components/component.h"
#include "components/particle.h"
#include "entity_faction.h"
#include "entity_id.h"
#include "entity_state.h"
#include "equipment.h"
#include "game.h"
#include "animation.h"
#include "base/format.h"
#include "base/rectangle.h"
#include "base/rgba.h"
#include "base/random.h"
#include "base/utils.h"
#include "base/sl_list.h"
#include "components/status_effect.h"
#include "damage.h"
#include "debug.h"
#include "camera.h"
#include "collision.h"
#include "entity.h"
#include "entity_system.h"
#include "item.h"
#include "magic.h"
#include "modifier.h"
#include "quad_tree.h"
#include "renderer/render_batch.h"
#include "renderer/render_key.h"
#include "tilemap.h"
#include "asset_table.h"
#include "trigger.h"

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

// TODO: move somewhere else
void entity_transition_to_state(World *world, Entity *entity, EntityState state)
{
    EntityState old_state = entity->state;

    switch (old_state.kind) {
	case ENTITY_STATE_ATTACKING: {
	    SpellID spell = old_state.as.attacking.spell_being_cast;
	    Vector2 cast_dir = old_state.as.attacking.attack_direction;
	    Vector2 cast_pos = rect_center(world_get_entity_bounding_box(entity));

	    magic_cast_spell(world, spell, entity, cast_pos, cast_dir);
	} break;

	default: {
	} break;
    }

    entity->state = state;

    if (es_has_component(entity, AnimationComponent)) {
	AnimationComponent *anim_comp = es_get_component(entity, AnimationComponent);
	AnimationID next_anim = anim_comp->state_animations[state.kind];

	// TODO: instead check if there is an animation for this state?
	ASSERT(next_anim != ANIM_NULL);

	// TODO: rename anim_transition_to_animation to start_animation or something
	anim_transition_to_animation(&anim_comp->current_animation, next_anim);
    }

    switch (state.kind) {
	case ENTITY_STATE_IDLE:
	case ENTITY_STATE_WALKING: {
	} break;

	case ENTITY_STATE_ATTACKING: {
	    entity->velocity = V2_ZERO;
	    entity->direction = state.as.attacking.attack_direction;
	} break;

        INVALID_DEFAULT_CASE;
    }
}

// First checks if current state can be cancelled and if we're not already in state
static void entity_try_transition_to_state(World *world, Entity *entity, EntityState state)
{
    if (entity->state.kind != state.kind) {
	entity_transition_to_state(world, entity, state);
    }
}

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

EntityWithID world_spawn_entity(World *world, EntityFaction faction)
{
    ASSERT(faction >= 0);
    ASSERT(faction < FACTION_COUNT);
    ASSERT(world->alive_entity_count < MAX_ENTITIES);

    EntityWithID result = es_create_entity(world->entity_system, faction, &world->world_arena);

    ssize alive_index = world->alive_entity_count++;
    world->alive_entity_ids[alive_index] = result.id;
    world->alive_entity_quad_tree_locations[alive_index] = QT_NULL_LOCATION;

    return result;
}

static void world_remove_entity(World *world, ssize alive_entity_index)
{
    ASSERT(alive_entity_index < world->alive_entity_count);

    EntityID *id = &world->alive_entity_ids[alive_entity_index];
    Entity *entity = es_get_entity(world->entity_system, *id);

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

    es_remove_entity(world->entity_system, *id);

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
    send_event(entity, death_event, world);

    es_schedule_entity_for_removal(entity);
}

static void world_update_entity_quad_tree_location(World *world, ssize alive_entity_index)
{
    ASSERT(alive_entity_index < world->alive_entity_count);

    EntityID id = world->alive_entity_ids[alive_entity_index];
    Entity *entity = es_get_entity(world->entity_system, id);
    ASSERT(entity);

    QuadTreeLocation *loc = &world->alive_entity_quad_tree_locations[alive_entity_index];
    Rectangle entity_area = world_get_entity_bounding_box(entity);

    *loc = qt_set_entity_area(&world->quad_tree, id, *loc, entity_area, &world->world_arena);
}

static Vector2i world_to_tile_coords(Vector2 world_coords)
{
    Vector2i result = {
        (s32)((f32)world_coords.x / (f32)TILE_SIZE),
        (s32)((f32)world_coords.y / (f32)TILE_SIZE)
    };

    return result;
}

static Vector2 tile_to_world_coords(Vector2i tile_coords)
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
        HealthComponent *health = es_get_component(entity, HealthComponent);

        if (health->health.hitpoints <= 0) {
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
    Entity *entity = es_get_entity(world->entity_system, id);

    if (es_has_component(entity, ParticleSpawner)) {
        ParticleSpawner *particle_spawner = es_get_component(entity, ParticleSpawner);
        component_update_particle_spawner(entity, particle_spawner, dt);
    }

    if (es_has_component(entity, LifetimeComponent)) {
        LifetimeComponent *lifetime = es_get_component(entity, LifetimeComponent);
        lifetime->time_to_live -= dt;
    }

    if (es_has_component(entity, StatusEffectComponent)) {
        StatusEffectComponent *status_effects = es_get_component(entity, StatusEffectComponent);
        status_effects_update(status_effects, dt);
    }

    if (es_has_component(entity, AnimationComponent)) {
        AnimationComponent *anim_component = es_get_component(entity, AnimationComponent);
	ASSERT(anim_component->current_animation.animation_id != ANIM_NULL);
	anim_update_instance(world, entity, &anim_component->current_animation, dt);
    }

    if (entity_should_die(entity)) {
        // NOTE: won't be removed until after this frame
	world_kill_entity(world, entity);
    }

    world_update_entity_quad_tree_location(world, alive_entity_index);
}

static void entity_render(Entity *entity, struct RenderBatch *rb,
    LinearArena *scratch, struct DebugState *debug_state, World *world, const FrameData *frame_data)
{
    if (es_has_component(entity, AnimationComponent)) {
        AnimationComponent *anim_component = es_get_component(entity, AnimationComponent);
	// TODO: move into separate function
        AnimationInstance *anim_instance = &anim_component->current_animation;
	anim_render_instance(anim_instance, entity, rb, scratch);
    } else if (es_has_component(entity, SpriteComponent)) {
        SpriteComponent *sprite_comp = es_get_component(entity, SpriteComponent);
	Sprite *sprite = &sprite_comp->sprite;
        // TODO: how to handle if entity has both sprite and animation component?
        // TODO: UI should be drawn on separate layer

	SpriteModifiers sprite_mods = sprite_get_modifiers(entity->direction, sprite->rotation_behaviour);

        Rectangle sprite_rect = { entity->position, sprite->size };
        rb_push_sprite(rb, scratch, sprite->texture, sprite_rect, sprite_mods,
	    get_asset_table()->texture_shader, RENDER_LAYER_ENTITIES);
    }

    if (es_has_component(entity, ColliderComponent) && debug_state->render_colliders) {
        ColliderComponent *collider = es_get_component(entity, ColliderComponent);

        Rectangle collider_rect = {
            .position = entity->position,
            .size = collider->size
        };

        rb_push_rect(rb, scratch, collider_rect, (RGBA32){0, 1, 0, 0.5f},
	    get_asset_table()->shape_shader, RENDER_LAYER_ENTITIES);
    }

    if (es_has_component(entity, ParticleSpawner)) {
        ParticleSpawner *ps = es_get_component(entity, ParticleSpawner);

        if (ps->config.particle_texture.id == NULL_TEXTURE.id) {
            ASSERT(ps->config.particle_color.a != 0.0f);

            rb_push_particles(rb, scratch, &ps->particle_buffer, ps->config.particle_color,
		ps->config.particle_size, get_asset_table()->shape_shader, RENDER_LAYER_PARTICLES);
        } else {
            // If color isn't set, assume it to be white
            RGBA32 particle_color = ps->config.particle_color;
            if (particle_color.a == 0.0f) {
                particle_color = RGBA32_WHITE;
            }

            rb_push_particles_textured(rb, scratch, &ps->particle_buffer,
		get_asset_table()->default_texture, particle_color, ps->config.particle_size,
		get_asset_table()->texture_shader, RENDER_LAYER_PARTICLES);
        }
    }

    if (debug_state->render_entity_bounds) {
        Rectangle entity_rect = world_get_entity_bounding_box(entity);
        rb_push_rect(rb, scratch, entity_rect, (RGBA32){1, 0, 1, 0.4f},
	    get_asset_table()->shape_shader, RENDER_LAYER_ENTITIES);
    }

    if (debug_state->render_entity_velocity) {
	Vector2 dir = v2_norm(entity->velocity);

	rb_push_line(rb, scratch, entity->position, v2_add(entity->position, v2_mul_s(dir, 20.0f)),
	    RGBA32_GREEN, 2.0f, get_asset_table()->shape_shader, RENDER_LAYER_OVERLAY);
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
    RetriggerBehaviour retrigger_behaviour, f32 duration)
{
    add_trigger_cooldown(&world->trigger_cooldowns, a, b, component,
	retrigger_behaviour, duration, &world->world_arena);
}

static void deal_damage_to_entity(World *world, Entity *entity, HealthComponent *health, DamageInstance damage)
{
    DamageValues damage_taken = {0};

    if (es_has_component(entity, StatsComponent)) {
        StatsComponent *stats = es_get_component(entity, StatsComponent);
        DamageValues resistances = calculate_resistances_after_boosts(entity, world->item_system);

        damage_taken = calculate_damage_received(resistances, damage);
    } else {
	ASSERT(0 && "Should this ever happen? An entity with no stats taking damage?");
        damage_taken = damage.types;
    }

    DamageValue dmg_sum = calculate_damage_sum(damage_taken);

    health->health.hitpoints -= dmg_sum;
    hitsplats_create(world, entity->position, damage_taken);
}

static void try_deal_damage_to_entity(World *world, Entity *receiver, Entity *sender, DamageInstance damage)
{
    (void)sender;

    HealthComponent *receiver_health = es_get_component(receiver, HealthComponent);

    if (receiver_health) {
        deal_damage_to_entity(world, receiver, receiver_health, damage);
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
    EntityID self_id = es_get_id_of_entity(world->entity_system, self);
    EntityID other_id = es_get_id_of_entity(world->entity_system, other);
    ASSERT(!entity_id_equal(self_id, other_id));

    b32 same_faction = self->faction == other->faction;

    if (should_invoke_trigger(world, self, other, DamageFieldComponent) && !same_faction) {
	DamageFieldComponent *dmg_field = es_get_component(self, DamageFieldComponent);

	try_deal_damage_to_entity(world, other, self, dmg_field->damage);

	world_add_trigger_cooldown(world, self_id, other_id,
	    component_flag(DamageFieldComponent), dmg_field->retrigger_behaviour, dmg_field->cooldown);
    }
}

static f32 entity_vs_entity_collision(World *world, Entity *a,
    ColliderComponent *collider_a, Entity *b, ColliderComponent *collider_b,
    f32 movement_fraction_left, f32 dt)
{
    ASSERT(a);
    ASSERT(b);

    EntityID id_a = es_get_id_of_entity(world->entity_system, a);
    EntityID id_b = es_get_id_of_entity(world->entity_system, b);

    Rectangle rect_a = get_entity_collider_rectangle(a, collider_a);
    Rectangle rect_b = get_entity_collider_rectangle(b, collider_b);

    CollisionInfo collision = collision_rect_vs_rect(movement_fraction_left, rect_a, rect_b,
	a->velocity, b->velocity, dt);

    if ((collision.collision_status != COLLISION_STATUS_NOT_COLLIDING)
	&& !entities_intersected_this_frame(world, id_a, id_b)) {
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
		send_event(a, event_data, world);
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
                Rectangle tile_rect = {
                    tile_to_world_coords(tile_coords),
                    {(f32)TILE_SIZE, (f32)TILE_SIZE },
                };

                CollisionInfo collision = collision_rect_vs_rect(movement_fraction_left, entity_rect,
		    tile_rect, entity->velocity, V2_ZERO, dt);

                if (collision.collision_status != COLLISION_STATUS_NOT_COLLIDING) {
		    movement_fraction_left = collision.movement_fraction_left;

		    execute_entity_vs_tilemap_collision_policy(world, entity, collision);

		    EventData event_data = event_data_tilemap_collision(collision_coords);
		    send_event(entity, event_data, world);
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
        Entity *a = es_get_entity(world->entity_system, id_a);
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
                    Entity *b = es_get_entity(world->entity_system, node->id);

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
		a->velocity = V2_ZERO;
	    }
        }
    }
}

Entity *world_get_player_entity(World *world)
{
    ASSERT(world->alive_entity_count > 0);
    EntityID player_id = world->alive_entity_ids[0];
    Entity *result = es_get_entity(world->entity_system, player_id);

    return result;
}

void update_player(World *world, const FrameData *frame_data, LinearArena *frame_arena,
    DebugState *debug_state, GameUIState *game_ui)
{
    Entity *player = world_get_player_entity(world);
    ASSERT(player);

    Vector2 camera_target = rect_center(world_get_entity_bounding_box(player));
    camera_set_target(&world->camera, camera_target);

    if (player->state.kind != ENTITY_STATE_ATTACKING) {
	f32 speed = 350.0f;

	if (input_is_key_held(&frame_data->input, KEY_LEFT_SHIFT)) {
	    speed *= 3.0f;
	}

	Vector2 acceleration = {0};

	if (input_is_key_down(&frame_data->input, KEY_W)) {
	    acceleration.y = 1.0f;
	} else if (input_is_key_down(&frame_data->input, KEY_S)) {
	    acceleration.y = -1.0f;
	}

	if (input_is_key_down(&frame_data->input, KEY_A)) {
	    acceleration.x = -1.0f;
	} else if (input_is_key_down(&frame_data->input, KEY_D)) {
	    acceleration.x = 1.0f;
	}

	acceleration = v2_norm(acceleration);

	if (!v2_eq(acceleration, V2_ZERO)) {
	    entity_try_transition_to_state(world, player, state_walking());
	} else {
	    entity_try_transition_to_state(world, player, state_idle());
	}

	f32 dt = frame_data->dt;
	Vector2 v = player->velocity;
	Vector2 p = player->position;

	acceleration = v2_mul_s(acceleration, speed);
	acceleration = v2_add(acceleration, v2_mul_s(v, -3.5f));

	Vector2 new_pos = v2_add(
	    v2_add(v2_mul_s(v2_mul_s(acceleration, dt * dt), 0.5f), v2_mul_s(v, dt)),
	    p
	);

	Vector2 new_velocity = v2_add(v2_mul_s(acceleration, dt), v);

	player->position = new_pos;
	player->velocity = new_velocity;

	if (input_is_key_held(&frame_data->input, MOUSE_LEFT)
	    && player->state.kind != ENTITY_STATE_ATTACKING) {
            Vector2 mouse_pos = frame_data->input.mouse_position;
            mouse_pos = screen_to_world_coords(world->camera, mouse_pos, frame_data->window_size);

	    Vector2 player_center = rect_center(world_get_entity_bounding_box(player));
            Vector2 mouse_dir = v2_sub(mouse_pos, player_center);
            mouse_dir = v2_norm(mouse_dir);

	    SpellCasterComponent *spellcaster = es_get_component(player, SpellCasterComponent);
	    ASSERT(spellcaster);

	    SpellID selected_spell = get_spell_at_spellbook_index(
		spellcaster, game_ui->selected_spellbook_index);


	    entity_transition_to_state(world, player, state_attacking(selected_spell, mouse_dir));
        }
    }

    if (input_is_key_pressed(&frame_data->input, MOUSE_LEFT)
	&& !entity_id_is_null(game_ui->hovered_entity)) {
	Entity *hovered_entity = es_get_entity(world->entity_system, game_ui->hovered_entity);
	GroundItemComponent *ground_item = es_get_component(hovered_entity, GroundItemComponent);

	if (ground_item) {
	    // TODO: make try_get_component be nullable, make get_component assert on failure to get
	    InventoryComponent *inv = es_get_component(player, InventoryComponent);
	    ASSERT(inv);

	    inventory_add_item(&inv->inventory, ground_item->item_id);
	    es_schedule_entity_for_removal(hovered_entity);
	}
    }

}

// TODO: move all controlling logic to game.c
void world_update(World *world, const FrameData *frame_data, LinearArena *frame_arena,
    DebugState *debug_state, GameUIState *game_ui)
{
    if (world->alive_entity_count < 1) {
        return;
    }

    update_player(world, frame_data, frame_arena, debug_state, game_ui);

    camera_zoom(&world->camera, (s32)frame_data->input.scroll_delta);
    camera_update(&world->camera, frame_data->dt);

    handle_collision_and_movement(world, frame_data->dt, frame_arena);

    // TODO: should any newly spawned entities be updated this frame?
    EntityIndex entity_count = world->alive_entity_count;

    for (EntityIndex i = 0; i < entity_count; ++i) {
        entity_update(world, i, frame_data->dt);
    }

    hitsplats_update(world, frame_data);

    update_trigger_cooldowns(world, world->entity_system, frame_data->dt);

    swap_and_reset_collision_tables(world);

    // Remove inactive entities on end of frame
    for (ssize i = 0; i < world->alive_entity_count; ++i) {
	EntityID *id = &world->alive_entity_ids[i];
	Entity *entity = es_get_entity(world->entity_system, *id);

	if (es_entity_is_inactive(entity)) {
	    world_remove_entity(world, i);
	    --i;
	}
    }
}

void world_render(World *world, RenderBatch *rb, const FrameData *frame_data,
    LinearArena *frame_arena, DebugState *debug_state)
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
                        texture = get_asset_table()->floor_texture;
                    } break;

                    case TILE_WALL: {
                        texture = get_asset_table()->wall_texture;
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
                    rb_push_sprite(rb, frame_arena, texture, tile_rect, (SpriteModifiers){0},
                        get_asset_table()->texture_shader, layer);
                } else if (tile->type == TILE_WALL) {
                    // NOTE: Walls are rendered in two segments and are made transparent if entities
		    // are behind them
                    RGBA32 tile_sprite_color = RGBA32_WHITE;

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

                    for (EntityIDNode *node = list_head(&entities_near_tile); node; node = list_next(node)) {
                        Entity *entity = es_get_entity(world->entity_system, node->id);

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
                            tile_sprite_color.a = 0.5f;
                            break;
                        }
                    }

                    rb_push_clipped_sprite(rb, frame_arena, texture,
                        tile_rect, bottom_segment, tile_sprite_color, get_asset_table()->texture_shader,
			RENDER_LAYER_FLOORS);

                    rb_push_clipped_sprite(rb, frame_arena, texture,
                        tile_rect, top_segment, tile_sprite_color, get_asset_table()->texture_shader,
			RENDER_LAYER_WALLS);
                } else {
                    ASSERT(0);
                }
            }
        }
    }

    // TODO: debug camera that is detached from regular camera
    EntityIDList entities_in_area = qt_get_entities_in_area(&world->quad_tree, visible_area, frame_arena);

    for (EntityIDNode *node = list_head(&entities_in_area); node; node = list_next(node)) {
        Entity *entity = es_get_entity(world->entity_system, node->id);

        entity_render(entity, rb, frame_arena, debug_state, world, frame_data);
    }

    hitsplats_render(world, rb, frame_arena);
}

static void drop_ground_item(World *world, Vector2 pos, ItemID id)
{
    EntityWithID entity = world_spawn_entity(world, FACTION_NEUTRAL);
    entity.entity->position = pos;

    GroundItemComponent *ground_item = es_add_component(entity.entity, GroundItemComponent);
    ground_item->item_id = id;

    SpriteComponent *sprite = es_add_component(entity.entity, SpriteComponent);
    sprite->sprite = sprite_create(get_asset_table()->fireball_texture, v2(16, 16), SPRITE_ROTATE_NONE);
}

void world_initialize(World *world, EntitySystem *entity_system, ItemSystem *item_system,
    FreeListArena *parent_arena)
{
    world->world_arena = fl_create(fl_allocator(parent_arena), WORLD_ARENA_SIZE);

    world->previous_frame_collisions = collision_event_table_create(&world->world_arena);
    world->current_frame_collisions = collision_event_table_create(&world->world_arena);

    world->entity_system = entity_system;
    world->item_system = item_system;

    // NOTE: testing code
    {
	for (s32 x = 0; x < 8; ++x) {
	    tilemap_insert_tile(&world->tilemap, (Vector2i){x, 0}, TILE_WALL,
		&world->world_arena);

	    tilemap_insert_tile(&world->tilemap, (Vector2i){x, 7}, TILE_WALL,
		&world->world_arena);
	}

	for (s32 y = 1; y < 7; ++y) {
	    tilemap_insert_tile(&world->tilemap, (Vector2i){0, y}, TILE_WALL,
		&world->world_arena);

	    tilemap_insert_tile(&world->tilemap, (Vector2i){7, y}, TILE_WALL,
		&world->world_arena);
	}

	for (s32 y = 1; y < 7; ++y) {
	    for (s32 x = 1; x < 7; ++x) {
		tilemap_insert_tile(&world->tilemap, (Vector2i){x, y}, TILE_FLOOR,
		    &world->world_arena);
	    }
	}
    }

    Rectangle tilemap_area = tilemap_get_bounding_box(&world->tilemap);

    qt_initialize(&world->quad_tree, tilemap_area);

    for (s32 i = 0; i < 1; ++i) {
#if 1
        EntityWithID entity_with_id = world_spawn_entity(world,
	    i == 0 ? FACTION_PLAYER : FACTION_ENEMY);
        Entity *entity = entity_with_id.entity;
        entity->position = v2(256, 256);

        ASSERT(!es_has_component(entity, ColliderComponent));

        ColliderComponent *collider = es_add_component(entity, ColliderComponent);
        collider->size = v2(16.0f, 16.0f);
	set_collision_policy_vs_entities(collider, COLLISION_POLICY_STOP);
	set_collision_policy_vs_tilemaps(collider, COLLISION_POLICY_STOP);

        HealthComponent *hp = es_add_component(entity, HealthComponent);
        hp->health.hitpoints = 100000;

        ASSERT(es_has_component(entity, ColliderComponent));
        ASSERT(es_has_component(entity, HealthComponent));

	SpellCasterComponent *spellcaster = es_add_component(entity, SpellCasterComponent);
	magic_add_to_spellbook(spellcaster, SPELL_ICE_SHARD);
	magic_add_to_spellbook(spellcaster, SPELL_SPARK);
	magic_add_to_spellbook(spellcaster, SPELL_FIREBALL);


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
        set_damage_value_for_type(&stats->base_resistances, DMG_TYPE_LIGHTNING, 0);

        StatusEffectComponent *effects = es_add_component(entity, StatusEffectComponent);

#if 0
        Modifier dmg_mod = {
            .kind = MODIFIER_DAMAGE,
        };

        set_damage_value_of_type(&dmg_mod.as.damage_modifier.additive_modifiers, DMG_KIND_LIGHTNING, 1000);
        status_effects_add(effects, dmg_mod, 5.0f);

        // TODO: easier way to add status effects and create modifiers
        Modifier res_mod = {
            .kind = MODIFIER_RESISTANCE,
        };

        set_damage_value_of_type(&res_mod.as.resistance_modifier, DMG_KIND_LIGHTNING, 120);
        status_effects_add(effects, res_mod, 3.0f);
#endif

        // TODO: ensure components are zeroed out
        InventoryComponent *inventory = es_add_component(entity, InventoryComponent);

        EquipmentComponent *equipment = es_add_component(entity, EquipmentComponent);
        (void)equipment;

        {
            ItemWithID item_with_id = item_sys_create_item(world->item_system);
            Item *item = item_with_id.item;


            item_sys_set_item_name(world->item_system, item, str_lit("Sword"));
            item_set_prop(item, ITEM_PROP_EQUIPPABLE | ITEM_PROP_HAS_MODIFIERS);
            item->equipment.slot = EQUIP_SLOT_WEAPON;

#if 1
            Modifier dmg_mod = create_damage_modifier(NUMERIC_MOD_FLAT_ADDITIVE, DMG_TYPE_FIRE, 100);
            item_add_modifier(item, dmg_mod);

	    Modifier dmg_mod2 = create_damage_modifier(NUMERIC_MOD_ADDITIVE_PERCENTAGE,
		DMG_TYPE_FIRE, 150);
            item_add_modifier(item, dmg_mod2);
            //item_add_modifier(item, res_mod);
#endif

#if 0
	    drop_ground_item(world, v2(400, 400), item_with_id.id);
#else
            inventory_add_item(&inventory->inventory, item_with_id.id);
#endif
	    equip_item_from_inventory(item_system, &equipment->equipment,
		&inventory->inventory, item_with_id.id);
        }
#endif
    }
}

void world_destroy(World *world)
{
    fl_destroy(&world->world_arena);
}
