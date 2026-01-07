#include "world.h"
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
#include "tilemap.h"
#include "asset_table.h"

#define WORLD_ARENA_SIZE MB(64)

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
	AnimationInstance *current_anim = anim_get_current_animation(entity, anim_comp);
	AnimationFrame current_frame = anim_get_current_frame(current_anim);

        size.x = MAX(size.x, current_frame.sprite.size.x);
        size.y = MAX(size.y, current_frame.sprite.size.y);
    }


    Rectangle result = {entity->position, size};

    return result;
}

EntityWithID world_spawn_entity(World *world, EntityFaction faction)
{
    ASSERT(world->alive_entity_count < MAX_ENTITIES);

    EntityWithID result = es_create_entity(world->entity_system, faction);

    ssize alive_index = world->alive_entity_count++;
    world->alive_entity_ids[alive_index] = result.id;
    world->alive_entity_quad_tree_locations[alive_index] = QT_NULL_LOCATION;

    return result;
}

static void world_remove_entity(World *world, ssize alive_entity_index)
{
    ASSERT(alive_entity_index < world->alive_entity_count);

    EntityID *id = &world->alive_entity_ids[alive_entity_index];
    es_remove_entity(world->entity_system, *id);

    QuadTreeLocation *qt_location = &world->alive_entity_quad_tree_locations[alive_entity_index];
    qt_remove_entity(&world->quad_tree, *id, *qt_location);

    ssize last_index = world->alive_entity_count - 1;
    *qt_location = world->alive_entity_quad_tree_locations[last_index];

    *id = world->alive_entity_ids[last_index];

    --world->alive_entity_count;
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
        component_update_particle_spawner(entity, particle_spawner, entity->position, dt);
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
	anim_update_instance(&anim_component->state_animations[entity->state], dt);
    }

    if (entity_should_die(entity)) {
        // NOTE: won't be removed until after this frame
        es_schedule_entity_for_removal(entity);

        if (es_has_component(entity, OnDeathComponent)) {
            OnDeathComponent *on_death = es_get_component(entity, OnDeathComponent);

            switch (on_death->kind) {
                case DEATH_EFFECT_SPAWN_PARTICLES: {
                    EntityWithID entity_with_id
			= world_spawn_entity(world, FACTION_NEUTRAL);
                    Entity *spawner = entity_with_id.entity;
                    spawner->position = entity->position;

                    ParticleSpawner *ps = es_add_component(spawner, ParticleSpawner);
                    particle_spawner_initialize(ps, on_death->as.spawn_particles.config);
                    ps->action_when_done = PS_WHEN_DONE_REMOVE_ENTITY;
                } break;
            }
        }
    }

    world_update_entity_quad_tree_location(world, alive_entity_index);
}

static void entity_render(Entity *entity, struct RenderBatch *rb,
    LinearArena *scratch, struct DebugState *debug_state, World *world, const FrameData *frame_data)
{
    if (es_has_component(entity, AnimationComponent)) {
        AnimationComponent *anim_component = es_get_component(entity, AnimationComponent);
	// TODO: move into separate function
        AnimationInstance *anim_instance = anim_get_current_animation(entity, anim_component);
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

void world_add_collision_cooldown(World *world, EntityID a, EntityID b, s32 effect_index)
{
    collision_cooldown_add(&world->collision_effect_cooldowns, a, b, effect_index, &world->world_arena);
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
    hitsplat_create(world, entity->position, damage_taken);
}

static void try_deal_damage_to_entity(World *world, Entity *receiver, Entity *sender, DamageInstance damage)
{
    (void)sender;

    HealthComponent *receiver_health = es_get_component(receiver, HealthComponent);

    if (receiver_health) {
        deal_damage_to_entity(world, receiver, receiver_health, damage);
    }
}

// TODO: too many parameters
static b32 should_execute_collision_effect(World *world, Entity *entity, Entity *other,
    OnCollisionEffect effect, ObjectKind colliding_with_obj_kind, s32 effect_index)
{
    EntityID entity_id = es_get_id_of_entity(world->entity_system, entity);
    b32 result = (effect.affects_object_kinds & colliding_with_obj_kind) != 0;

    if (result && other) {
        EntityID other_id = es_get_id_of_entity(world->entity_system, other);
        b32 in_same_faction = entity->faction == other->faction;

        b32 not_on_cooldown = !collision_cooldown_find(&world->collision_effect_cooldowns,
	    entity_id, other_id, effect_index);

        result = not_on_cooldown && (!in_same_faction || (effect.affects_same_faction_entities));
    }

    return result;
}

// TODO: make this take entitypair as parameter
static void execute_collision_effects(World *world, Entity *entity, Entity *other, ColliderComponent *collider,
    CollisionInfo collision, EntityPairIndex collision_index, ObjectKind colliding_with_obj_kind)
{
    // TODO: clean up this function
    ASSERT(entity);

    b32 should_block = !other;

    // TODO: if no blocking effect is present, pass through
    // If other entity has a pass through effect, we shouldn't be blocked by that entity
    if (other) {
        ColliderComponent *other_collider = es_get_component(other, ColliderComponent);
        ASSERT(other_collider);

        for (s32 i = 0; i < other_collider->on_collide_effects.count; ++i) {
            OnCollisionEffect effect = other_collider->on_collide_effects.effects[i];

            b32 should_execute = should_execute_collision_effect(world, other, entity, effect,
		colliding_with_obj_kind, i);

            if (should_execute && (effect.kind == ON_COLLIDE_STOP)) {
                should_block = true;
            }
        }
    }

    for (s32 i = 0; i < collider->on_collide_effects.count; ++i) {
        OnCollisionEffect effect = collider->on_collide_effects.effects[i];

        b32 should_execute = should_execute_collision_effect(world, entity, other, effect,
	    colliding_with_obj_kind, i);

        if (!should_execute) {
            continue;
        }

        switch (effect.kind) {
            case ON_COLLIDE_STOP: {
                if (should_block) {
                    if (collision_index == ENTITY_PAIR_INDEX_FIRST) {
                        entity->position = collision.new_position_a;
                        entity->velocity = collision.new_velocity_a;
                    } else {
                        entity->position = collision.new_position_b;
                        entity->velocity = collision.new_velocity_b;
                    }
                }
            } break;

            case ON_COLLIDE_BOUNCE: {
                if (should_block) {
                    if (collision_index == ENTITY_PAIR_INDEX_FIRST) {
                        entity->position = collision.new_position_a;
                        entity->velocity = v2_reflect(entity->velocity, collision.collision_normal);
                    } else {
                        entity->position = collision.new_position_b;
                        entity->velocity = v2_reflect(entity->velocity, v2_neg(collision.collision_normal));
                    }
                }
            } break;

            case ON_COLLIDE_DEAL_DAMAGE: {
                ASSERT(other);
                ASSERT(effect.affects_object_kinds == OBJECT_KIND_ENTITIES);

                try_deal_damage_to_entity(world, other, entity, effect.as.deal_damage.damage);
            } break;

            case ON_COLLIDE_DIE: {
                es_schedule_entity_for_removal(entity);
            } break;

            case ON_COLLIDE_PASS_THROUGH: {} break;

            INVALID_DEFAULT_CASE;
        }

        if (other && (effect.retrigger_behaviour != COLL_RETRIGGER_WHENEVER)) {
            EntityID entity_id = es_get_id_of_entity(world->entity_system, entity);
            EntityID other_id = es_get_id_of_entity(world->entity_system, other);

            s32 effect_index = i;
            world_add_collision_cooldown(world, entity_id, other_id, effect_index);
        }
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

static f32 entity_vs_entity_collision(World *world, Entity *a,
    ColliderComponent *collider_a, Entity *b, ColliderComponent *collider_b,
    f32 movement_fraction_left, f32 dt)
{
    EntityID id_a = es_get_id_of_entity(world->entity_system, a);
    EntityID id_b = es_get_id_of_entity(world->entity_system, b);

    Rectangle rect_a = get_entity_collider_rectangle(a, collider_a);
    Rectangle rect_b = get_entity_collider_rectangle(b, collider_b);

    CollisionInfo collision = collision_rect_vs_rect(movement_fraction_left, rect_a, rect_b,
        a->velocity, b->velocity, dt);

    if ((collision.collision_state != COLL_NOT_COLLIDING)
	&& !entities_intersected_this_frame(world, id_a, id_b)) {
        execute_collision_effects(world, a, b, collider_a, collision,
	    ENTITY_PAIR_INDEX_FIRST, OBJECT_KIND_ENTITIES);

        execute_collision_effects(world, b, a, collider_b, collision,
	    ENTITY_PAIR_INDEX_SECOND, OBJECT_KIND_ENTITIES);

        movement_fraction_left = collision.movement_fraction_left;

        collision_event_table_insert(&world->current_frame_collisions, id_a, id_b);
    }

    return movement_fraction_left;
}


static f32 entity_vs_tilemap_collision(Entity *entity, ColliderComponent *collider,
    World *world, f32 movement_fraction_left, f32 dt)
{
    // TODO: use get_entity_collision_area here
    Vector2 old_entity_pos = entity->position;
    Vector2 new_entity_pos = v2_add(entity->position, v2_mul_s(entity->velocity, dt));
    f32 entity_width = collider->size.x;
    f32 entity_height = collider->size.y;

    f32 min_x = MIN(old_entity_pos.x, new_entity_pos.x);
    f32 max_x = MAX(old_entity_pos.x, new_entity_pos.x) + entity_width;
    f32 min_y = MIN(old_entity_pos.y, new_entity_pos.y);
    f32 max_y = MAX(old_entity_pos.y, new_entity_pos.y) + entity_height;

    s32 min_tile_x = (s32)(min_x / TILE_SIZE);
    s32 max_tile_x = (s32)(max_x / TILE_SIZE);
    s32 min_tile_y = (s32)(min_y / TILE_SIZE);
    s32 max_tile_y = (s32)(max_y / TILE_SIZE);

    // TODO: fix this, should only check tiles that exist
    /* min_tile_x = MAX(min_tile_x, world->tilemap.min_x); */
    /* max_tile_x = MIN(max_tile_x, world->tilemap.max_x); */
    /* min_tile_y = MAX(min_tile_y, world->tilemap.min_y); */
    /* max_tile_y = MIN(max_tile_y, world->tilemap.max_y); */

    CollisionInfo closest_collision = { .movement_fraction_left = INFINITY };

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

                // TODO: only handle the closest collision
                CollisionInfo collision = collision_rect_vs_rect(movement_fraction_left, entity_rect,
		    tile_rect, entity->velocity, V2_ZERO, dt);

                if ((collision.collision_state != COLL_NOT_COLLIDING)
                    && (collision.movement_fraction_left < closest_collision.movement_fraction_left)) {
                    closest_collision = collision;

                }
            }
        }
    }

    if (closest_collision.collision_state != COLL_NOT_COLLIDING) {
        movement_fraction_left = closest_collision.movement_fraction_left;

        execute_collision_effects(world, entity, 0, collider, closest_collision,
	    ENTITY_PAIR_INDEX_FIRST, OBJECT_KIND_TILES);
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

            if (v2_mag(a->velocity) > 0.5f) {
                a->direction = v2_norm(a->velocity);
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

void world_update(World *world, const FrameData *frame_data, LinearArena *frame_arena,
    DebugState *debug_state)
{
    if (world->alive_entity_count < 1) {
        return;
    }

    Entity *player = world_get_player_entity(world);
    ASSERT(player);

    {
        Vector2 target = player->position;

        if (es_has_component(player, ColliderComponent)) {
            ColliderComponent *collider = es_get_component(player, ColliderComponent);
            target = v2_add(target, v2_div_s(collider->size, 2));
        }

        if (input_is_key_released(&frame_data->input, MOUSE_LEFT)) {
            Vector2 mouse_pos = frame_data->input.mouse_position;
            mouse_pos = screen_to_world_coords(world->camera, mouse_pos, frame_data->window_size);

	    Vector2 player_center = rect_center(world_get_entity_bounding_box(player));
            Vector2 mouse_dir = v2_sub(mouse_pos, player_center);
            mouse_dir = v2_norm(mouse_dir);

            magic_cast_spell(world, SPELL_FIREBALL, player, player_center, mouse_dir);
        }

        camera_set_target(&world->camera, target);
    }

    camera_zoom(&world->camera, (s32)frame_data->input.scroll_delta);
    camera_update(&world->camera, frame_data->dt);

    f32 speed = 350.0f;

    if (input_is_key_pressed(&frame_data->input, MOUSE_LEFT)
	&& !entity_id_is_null(debug_state->hovered_entity)) {
	Entity *hovered_entity = es_get_entity(world->entity_system, debug_state->hovered_entity);
	GroundItemComponent *ground_item = es_get_component(hovered_entity, GroundItemComponent);

	if (ground_item) {
	    // TODO: make try_get_component be nullable, make get_component assert on failure to get
	    InventoryComponent *inv = es_get_component(player, InventoryComponent);
	    ASSERT(inv);

	    inventory_add_item(&inv->inventory, ground_item->item_id);
	    es_schedule_entity_for_removal(hovered_entity);
	}
    }

    if (input_is_key_held(&frame_data->input, KEY_LEFT_SHIFT)) {
        speed *= 3.0f;
    }

    {
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
            player->state = ENTITY_STATE_WALKING;
        } else {
            player->state = ENTITY_STATE_IDLE;
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
    }

    handle_collision_and_movement(world, frame_data->dt, frame_arena);

    // TODO: should any newly spawned entities be updated this frame?
    EntityIndex entity_count = world->alive_entity_count;

    for (EntityIndex i = 0; i < entity_count; ++i) {
        entity_update(world, i, frame_data->dt);
    }

    hitsplats_update(world, frame_data);

    // TODO: should this be done at beginning of each frame so inactive entities are rendered?
    remove_expired_collision_cooldowns(world, world->entity_system);

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
                    // NOTE: Walls are rendered in two segments and are made transparent if entities are behind them
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

                        if (entity_is_behind_wall) {
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

void world_initialize(World *world, LinearArena *arena, EntitySystem *entity_system, ItemSystem *item_system)
{
    world->world_arena = la_create(la_allocator(arena), WORLD_ARENA_SIZE);

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
    item_sys_initialize(world->item_system, la_allocator(&world->world_arena));

    for (s32 i = 0; i < 2; ++i) {
#if 1
        EntityWithID entity_with_id = world_spawn_entity(world, i + 1);
        Entity *entity = entity_with_id.entity;
        entity->position = v2(256, 256);

        ASSERT(!es_has_component(entity, ColliderComponent));

        ColliderComponent *collider = es_add_component(entity, ColliderComponent);
        collider->size = v2(16.0f, 16.0f);
        add_blocking_collide_effect(collider, OBJECT_KIND_ENTITIES | OBJECT_KIND_TILES, true);

        HealthComponent *hp = es_add_component(entity, HealthComponent);
        hp->health.hitpoints = 100000;

        ASSERT(es_has_component(entity, ColliderComponent));
        ASSERT(es_has_component(entity, HealthComponent));

#if 0
        ParticleSpawner *spawner = es_add_component(entity, ParticleSpawner);
        spawner->action_when_done = PS_WHEN_DONE_REMOVE_COMPONENT;
        ParticleSpawnerConfig config = {
            .particle_color = (RGBA32){0.2f, 0.9f, 0.1f, 0.4f},
            .particle_size = 4.0f,
            .particles_per_second = 250.0f,
            .total_particles_to_spawn = 1000000.0f, // TODO: infinite particles
            .particle_speed = 100.0f,
            .particle_lifetime = 1.0f
        };

        particle_spawner_initialize(spawner, config);
#endif

#if 0
        SpriteComponent *sprite_comp = es_add_component(entity, SpriteComponent);
	Sprite *sprite = &sprite_comp->sprite;

        sprite->texture = asset_list->player_idle1;
        sprite->size = v2(32, 32);
	sprite->rotation_behaviour = SPRITE_ROTATE_BASED_ON_DIR;

#else

        AnimationComponent *anim = es_add_component(entity, AnimationComponent);
        anim->state_animations[ENTITY_STATE_IDLE].animation_id = ANIM_PLAYER_IDLE;
        anim->state_animations[ENTITY_STATE_WALKING].animation_id = ANIM_PLAYER_WALKING;
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
            Modifier dmg_mod = create_damage_modifier(NUMERIC_MOD_FLAT_ADDITIVE, DMG_TYPE_FIRE, 1000);
            item_add_modifier(item, dmg_mod);
            //item_add_modifier(item, res_mod);
#endif

#if 1
	    drop_ground_item(world, v2(400, 400), item_with_id.id);
#else
            inventory_add_item(&inventory->inventory, item_with_id.id);
#endif

            //try_equip_item_in_slot(&world->item_manager, &equipment->equipment, item_with_id.id, EQUIP_SLOT_HEAD);
        }
#endif
    }
}
