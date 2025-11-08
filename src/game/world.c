#include "world.h"
#include "animation.h"
#include "base/format.h"
#include "base/matrix.h"
#include "base/rectangle.h"
#include "base/rgba.h"
#include "base/random.h"
#include "base/utils.h"
#include "base/sl_list.h"
#include "base/vector.h"
#include "components/component.h"
#include "components/status_effect.h"
#include "debug.h"
#include "camera.h"
#include "collision.h"
#include "damage.h"
#include "entity.h"
#include "entity_system.h"
#include "equipment.h"
#include "item.h"
#include "magic.h"
#include "input.h"
#include "modifier.h"
#include "quad_tree.h"
#include "renderer/render_batch.h"
#include "renderer/render_key.h"
#include "tilemap.h"

#define WORLD_ARENA_SIZE MB(64)

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

static void animation_instance_update(AnimationInstance *anim_instance, AnimationTable *animations, f32 dt)
{
    Animation *anim = anim_get_by_id(animations, anim_instance->animation_id);
    ASSERT(anim_instance->current_frame < anim->frame_count);

    AnimationFrame curr_frame = anim->frames[anim_instance->current_frame];

    anim_instance->current_frame_elapsed_time += dt;

    if (anim_instance->current_frame_elapsed_time >= curr_frame.duration) {
        anim_instance->current_frame_elapsed_time = 0.0f;

        anim_instance->current_frame += 1;

        if ((anim_instance->current_frame == anim->frame_count) && anim->is_repeating) {
            anim_instance->current_frame = 0;
        }

        // Non-repeating animations should stay on the last frame
        anim_instance->current_frame = MIN(anim_instance->current_frame, anim->frame_count - 1);
    }
}

static void component_update_animation(Entity *entity, AnimationComponent *anim_component,
    AnimationTable *animations, f32 dt)
{
    // TODO: this function is unnecessary
    animation_instance_update(&anim_component->state_animations[entity->state], animations, dt);
}

static void entity_update(World *world, Entity *entity, f32 dt, AnimationTable *animations)
{
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
        component_update_animation(entity, anim_component, animations, dt);
    }

    if (entity_should_die(entity)) {
        // NOTE: won't be removed until after this frame
        es_schedule_entity_for_removal(entity);

        if (es_has_component(entity, OnDeathComponent)) {
            OnDeathComponent *on_death = es_get_component(entity, OnDeathComponent);

            switch (on_death->kind) {
                case DEATH_EFFECT_SPAWN_PARTICLES: {
                    EntityWithID entity_with_id = es_spawn_entity(&world->entity_system, FACTION_NEUTRAL);
                    Entity *spawner = entity_with_id.entity;
                    spawner->position = entity->position;

                    ParticleSpawner *ps = es_add_component(spawner, ParticleSpawner);
                    particle_spawner_initialize(ps, on_death->as.spawn_particles.config);
                    ps->action_when_done = PS_WHEN_DONE_REMOVE_ENTITY;
                } break;
            }
        }
    }

    es_update_entity_quad_tree_location(&world->entity_system, entity, &world->world_arena);
}

static void entity_render(Entity *entity, struct RenderBatch *rb, const AssetList *assets,
    LinearArena *scratch, struct DebugState *debug_state, World *world, const FrameData *frame_data,
    AnimationTable *animations)
{
    if (es_has_component(entity, AnimationComponent)) {
        AnimationComponent *anim_component = es_get_component(entity, AnimationComponent);
        AnimationInstance *anim_instance = &anim_component->state_animations[entity->state];
        Animation *anim = anim_get_by_id(animations, anim_instance->animation_id);
        AnimationFrame current_frame = anim->frames[anim_instance->current_frame];

        f32 rotation = 0.0f;
        f32 dir_angle = (f32)atan2(entity->direction.y, entity->direction.x);
        RectangleFlip flip = RECT_FLIP_NONE;

        if (anim_component->rotation_behaviour == SPRITE_ROTATE_BASED_ON_DIR) {
            rotation = dir_angle;
        } else if (anim_component->rotation_behaviour == SPRITE_MIRROR_HORIZONTALLY_BASED_ON_DIR) {
            b32 should_flip = (dir_angle > PI_2) || (dir_angle < -PI_2);

            if (should_flip) {
                flip = RECT_FLIP_HORIZONTALLY;
            }
        }

        Rectangle sprite_rect = { entity->position, anim_component->sprite_size };
        rb_push_sprite(rb, scratch, current_frame.texture, sprite_rect, rotation, flip,
            RGBA32_WHITE, assets->texture_shader, RENDER_LAYER_ENTITIES);
    } else if (es_has_component(entity, SpriteComponent)) {
        SpriteComponent *sprite = es_get_component(entity, SpriteComponent);
        // TODO: how to handle if entity has both sprite and animation component?
        // TODO: UI should be drawn on separate layer

        Rectangle sprite_rect = { entity->position, sprite->size };
        rb_push_sprite(rb, scratch, sprite->texture, sprite_rect, 0, RECT_FLIP_NONE, RGBA32_WHITE, assets->texture_shader,
            RENDER_LAYER_ENTITIES);
    }

    if (es_has_component(entity, ColliderComponent) && debug_state->render_colliders) {
        ColliderComponent *collider = es_get_component(entity, ColliderComponent);

        Rectangle collider_rect = {
            .position = entity->position,
            .size = collider->size
        };

        rb_push_rect(rb, scratch, collider_rect, (RGBA32){0, 1, 0, 0.5f}, assets->shape_shader, RENDER_LAYER_ENTITIES);
    }

    if (es_has_component(entity, ParticleSpawner)) {
        ParticleSpawner *ps = es_get_component(entity, ParticleSpawner);

        if (ps->config.particle_texture.id == NULL_TEXTURE.id) {
            ASSERT(ps->config.particle_color.a != 0.0f);

            rb_push_particles(rb, scratch, &ps->particle_buffer,
                ps->config.particle_color, ps->config.particle_size, assets->shape_shader, RENDER_LAYER_PARTICLES);
        } else {
            // If color isn't set, assume it to be white
            RGBA32 particle_color = ps->config.particle_color;
            if (particle_color.a == 0.0f) {
                particle_color = RGBA32_WHITE;
            }

            rb_push_particles_textured(rb, scratch, &ps->particle_buffer,
                assets->default_texture, particle_color, ps->config.particle_size, assets->texture_shader,
                RENDER_LAYER_PARTICLES);
        }
    }

    if (debug_state->render_entity_bounds) {
        Rectangle entity_rect = es_get_entity_bounding_box(entity);
        rb_push_rect(rb, scratch, entity_rect, (RGBA32){1, 0, 1, 0.4f}, assets->shape_shader, RENDER_LAYER_ENTITIES);
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

static void create_hitsplat(World *world, Vector2 position, DamageInstance damage)
{
    ASSERT(world->hitsplat_count < ARRAY_COUNT(world->active_hitsplats));

    // TODO: randomize size and position
    // TODO: use function for updating position instead
    Vector2 velocity = rng_direction(PI * 2);
    velocity = v2_mul_s(velocity, 20.0f);

    Hitsplat hitsplat = {
        .damage = damage,
        .position = position,
        .velocity = velocity,
        .lifetime = 2.0f,
    };

    s32 index = world->hitsplat_count++;
    world->active_hitsplats[index] = hitsplat;
}

static void deal_damage_to_entity(World *world, Entity *entity, HealthComponent *health, DamageInstance damage)
{
    DamageInstance damage_taken = {0};

    if (es_has_component(entity, StatsComponent)) {
        StatsComponent *stats = es_get_component(entity, StatsComponent);
        DamageTypes resistances = calculate_resistances_after_boosts(stats->base_resistances, entity, &world->item_manager);

        damage_taken = calculate_damage_received(resistances, damage);
    } else {
        damage_taken = damage;
    }

    DamageValue dmg_sum = calculate_damage_sum(damage_taken.types);

    health->health.hitpoints -= dmg_sum;
    create_hitsplat(world, entity->position, damage_taken);
}

static void try_deal_damage_to_entity(World *world, Entity *receiver, Entity *sender, DamageInstance damage)
{
    (void)sender;

    HealthComponent *receiver_health = es_get_component(receiver, HealthComponent);

    if (receiver_health) {
        deal_damage_to_entity(world, receiver, receiver_health, damage);
    }
}

static b32 should_execute_collision_effect(World *world, Entity *entity, Entity *other, OnCollisionEffect effect,
    ObjectKind colliding_with_obj_kind, s32 effect_index)
{
    EntityID entity_id = es_get_id_of_entity(&world->entity_system, entity);
    b32 result = (effect.affects_object_kinds & colliding_with_obj_kind) != 0;

    if (result && other) {
        EntityID other_id = es_get_id_of_entity(&world->entity_system, other);
        b32 in_same_faction = entity->faction == other->faction;

        b32 not_on_cooldown = !collision_cooldown_find(&world->collision_effect_cooldowns, entity_id, other_id, effect_index);

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
            EntityID entity_id = es_get_id_of_entity(&world->entity_system, entity);
            EntityID other_id = es_get_id_of_entity(&world->entity_system, other);

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
    EntityID id_a = es_get_id_of_entity(&world->entity_system, a);
    EntityID id_b = es_get_id_of_entity(&world->entity_system, b);

    Rectangle rect_a = get_entity_collider_rectangle(a, collider_a);
    Rectangle rect_b = get_entity_collider_rectangle(b, collider_b);

    CollisionInfo collision = collision_rect_vs_rect(movement_fraction_left, rect_a, rect_b,
        a->velocity, b->velocity, dt);

    if ((collision.collision_state != COLL_NOT_COLLIDING) && !entities_intersected_this_frame(world, id_a, id_b)) {
        execute_collision_effects(world, a, b, collider_a, collision, ENTITY_PAIR_INDEX_FIRST, OBJECT_KIND_ENTITIES);
        execute_collision_effects(world, b, a, collider_b, collision, ENTITY_PAIR_INDEX_SECOND, OBJECT_KIND_ENTITIES);

        movement_fraction_left = collision.movement_fraction_left;

        collision_event_table_insert(&world->current_frame_collisions, id_a, id_b);
    }

    return movement_fraction_left;
}


static f32 entity_vs_tilemap_collision(Entity *entity, ColliderComponent *collider,
    World *world, f32 movement_fraction_left, f32 dt)
{
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

        execute_collision_effects(world, entity, 0, collider, closest_collision, ENTITY_PAIR_INDEX_FIRST, OBJECT_KIND_TILES);
    }

    return movement_fraction_left;
}

static Rectangle get_entity_collision_area(const Entity *entity, ColliderComponent *collider)
{
    Vector2 next_pos = v2_add(entity->position, entity->velocity);

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
    for (EntityIndex i = 0; i < world->entity_system.alive_entity_count; ++i) {
        // TODO: this seems bug prone

        EntityID id_a = world->entity_system.alive_entity_ids[i];
        Entity *a = es_get_entity(&world->entity_system, id_a);
        ColliderComponent *collider_a = es_get_component(a, ColliderComponent);
        ASSERT(a);

        f32 movement_fraction_left = 1.0f;

        if (collider_a) {
            movement_fraction_left = entity_vs_tilemap_collision(a, collider_a, world, movement_fraction_left, dt);

            Rectangle collision_area = get_entity_collision_area(a, collider_a);
            EntityIDList entities_in_area = es_get_entities_in_area(&world->entity_system, collision_area, frame_arena);

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

            if (v2_mag(a->velocity) > 0.5f) {
                a->direction = v2_norm(a->velocity);
            }
        }
    }
}

void world_update(World *world, const FrameData *frame_data, const AssetList *assets, LinearArena *frame_arena,
    AnimationTable *animations)
{
    if (world->entity_system.alive_entity_count < 1) {
        return;
    }

    EntityID player_id = world->entity_system.alive_entity_ids[0];
    Entity *player = es_get_entity(&world->entity_system, player_id);
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

            Vector2 mouse_dir = v2_sub(mouse_pos, player->position);
            mouse_dir = v2_norm(mouse_dir);

            magic_cast_spell(world, SPELL_SPARK, player, player->position, mouse_dir);
        }

        camera_set_target(&world->camera, target);
    }

    camera_zoom(&world->camera, (s32)frame_data->input.scroll_delta);
    camera_update(&world->camera, frame_data->dt);

    f32 speed = 350.0f;

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

    if (input_is_key_pressed(&frame_data->input, KEY_G)) {
        InventoryComponent *inv = es_get_component(player, InventoryComponent);
        EquipmentComponent *equipment = es_get_component(player, EquipmentComponent);

        if (inv && equipment) {
            ItemID item_id = inventory_get_item_at_index(&inv->inventory, 0);

            EquipResult equip_result = try_equip_item_in_slot(&world->item_manager, &equipment->equipment,
                item_id, EQUIP_SLOT_HEAD);
            ASSERT(equip_result.success);

            if (equip_result.success) {
                inventory_remove_item_at_index(&inv->inventory, 0);
            }
        }
    }

    handle_collision_and_movement(world, frame_data->dt, frame_arena);

    // TODO: should any newly spawned entities be updated this frame?
    EntityIndex entity_count = world->entity_system.alive_entity_count;
    for (EntityIndex i = 0; i < entity_count; ++i) {
        EntityID entity_id = world->entity_system.alive_entity_ids[i];
        Entity *entity = es_get_entity(&world->entity_system, entity_id);
        ASSERT(entity);

        entity_update(world, entity, frame_data->dt, animations);
    }

    for (s32 i = 0; i < world->hitsplat_count; ++i) {
        Hitsplat *hitsplat = &world->active_hitsplats[i];

        if (hitsplat->timer >= hitsplat->lifetime) {
            world->active_hitsplats[i] = world->active_hitsplats[world->hitsplat_count - 1];
            world->hitsplat_count -= 1;

            if (world->hitsplat_count == 0) {
                break;
            }
        }

        hitsplat->position = v2_add(hitsplat->position, v2_mul_s(hitsplat->velocity, frame_data->dt));
        hitsplat->timer += frame_data->dt;
    }

    // TODO: should this be done at beginning of each frame so inactive entities are rendered?
    // TODO: it would be better to pass list of entities to remove since we just retrieved inactive entities

    remove_expired_collision_cooldowns(world);

    swap_and_reset_collision_tables(world);

    es_remove_inactive_entities(&world->entity_system, frame_arena);
}

// TODO: this shouldn't need AssetList parameter?
void world_render(World *world, RenderBatch *rb, const AssetList *assets, const FrameData *frame_data,
    LinearArena *frame_arena, DebugState *debug_state, AnimationTable *animations)
{
    Rectangle visible_area = camera_get_visible_area(world->camera, frame_data->window_size);
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
                        texture = assets->floor_texture;
                    } break;

                    case TILE_WALL: {
                        texture = assets->wall_texture;
                    } break;

                    INVALID_DEFAULT_CASE;
                }

                Rectangle tile_rect = {
                    .position = tile_to_world_coords(tile_coords),
                    .size = { (f32)TILE_SIZE, (f32)TILE_SIZE }
                };

                if (tile->type == TILE_WALL) {
                    tile_rect.size.y += TILE_SIZE;
                }

                RenderLayer layer = (tile->type == TILE_WALL)
                    ? RENDER_LAYER_WALLS
                    : RENDER_LAYER_FLOORS;

                RGBA32 tile_sprite_color = RGBA32_WHITE;

                if (tile->type == TILE_WALL) {
                    EntityIDList entities_near_tile = es_get_entities_in_area(&world->entity_system, tile_rect, frame_arena);

                    for (EntityIDNode *node = list_head(&entities_near_tile); node; node = list_next(node)) {
                        Entity *entity = es_get_entity(&world->entity_system, node->id);

                        // TODO: only do this if it's the player?
                        // TODO: only get sprite rect instead of all component bounds?

                        Rectangle entity_bounds = es_get_entity_bounding_box(entity);
                        Tile *tile_above = tilemap_get_tile(&world->tilemap, (Vector2i){tile_coords.x, tile_coords.y + 1});

                        b32 can_walk_behind_tile = tile_above && (tile_above->type == TILE_FLOOR);
                        b32 entity_is_behind_wall =
                            can_walk_behind_tile
                            && rect_intersects(entity_bounds, tile_rect)
                            && (entity->position.y > tile_rect.position.y);

                        if (entity_is_behind_wall) {
                            tile_sprite_color.a = 0.5f;
                        }
                    }
                }

                rb_push_sprite(rb, frame_arena, texture, tile_rect, 0, 0, tile_sprite_color,
                    assets->texture_shader, layer);
            }
        }
    }

    // TODO: debug camera that is detached from regular camera
    EntityIDList entities_in_area = qt_get_entities_in_area(&world->entity_system.quad_tree, visible_area, frame_arena);

    for (EntityIDNode *node = list_head(&entities_in_area); node; node = list_next(node)) {
        Entity *entity = es_get_entity(&world->entity_system, node->id);

        entity_render(entity, rb, assets, frame_arena, debug_state, world, frame_data, animations);
    }

    for (s32 i = 0; i < world->hitsplat_count; ++i) {
        Hitsplat *hitsplat = &world->active_hitsplats[i];

	for (DamageKind type = 0; type < DMG_KIND_COUNT; ++type) {
	    DamageValue value_of_type = get_damage_value_of_type(hitsplat->damage.types, type);
	    ASSERT(value_of_type >= 0);

	    if (value_of_type > 0) {
		String damage_str = ssize_to_string(value_of_type, la_allocator(frame_arena));

		f32 alpha = 1.0f - hitsplat->timer / hitsplat->lifetime;
		RGBA32 color = {0};

		switch (type) {
		    case DMG_KIND_FIRE: {
			color = RGBA32_RED;
		    } break;

		    case DMG_KIND_LIGHTNING: {
			color = RGBA32_YELLOW;
		    } break;

		    INVALID_DEFAULT_CASE;
		}

		color.a = alpha;

		rb_push_text(
		    rb, frame_arena, damage_str, hitsplat->position, color, 32,
		    assets->texture_shader, assets->default_font, 5);
	    }
	}
    }
}

void world_initialize(World *world, const struct AssetList *asset_list, LinearArena *arena)
{
    world->world_arena = la_create(la_allocator(arena), WORLD_ARENA_SIZE);

    world->previous_frame_collisions = collision_event_table_create(&world->world_arena);
    world->current_frame_collisions = collision_event_table_create(&world->world_arena);


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

    Rectangle tilemap_area = tilemap_get_bounding_box(&world->tilemap);
    es_initialize(&world->entity_system, tilemap_area);

    item_mgr_initialize(&world->item_manager);

    for (s32 i = 0; i < 1; ++i) {
        EntityWithID entity_with_id = es_spawn_entity(&world->entity_system, i + 1);
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
        SpriteComponent *sprite = es_add_component(entity, SpriteComponent);
        sprite->texture = asset_list->player_idle1;
        sprite->size = v2(32, 32);

        AnimationComponent *anim = es_add_component(entity, AnimationComponent);
        anim->state_animations[ENTITY_STATE_IDLE].animation_id = ANIM_PLAYER_IDLE;
        anim->state_animations[ENTITY_STATE_WALKING].animation_id = ANIM_PLAYER_WALKING;
        anim->rotation_behaviour = SPRITE_MIRROR_HORIZONTALLY_BASED_ON_DIR;
        anim->sprite_size = v2(32, 64);

        StatsComponent *stats = es_add_component(entity, StatsComponent);
        set_damage_value_of_type(&stats->base_resistances, DMG_KIND_LIGHTNING, 0);

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
            ItemWithID item_with_id = item_mgr_create_item(&world->item_manager);
            Item *item = item_with_id.item;
            item_set_prop(item, ITEM_PROP_EQUIPMENT | ITEM_PROP_HAS_MODIFIERS);
            item->equipment.slot = EQUIP_SLOT_HEAD;

            Modifier dmg_mod = {
                .kind = MODIFIER_DAMAGE,
            };

            set_damage_value_of_type(&dmg_mod.as.damage_modifier.additive_modifiers, DMG_KIND_LIGHTNING, 1000);

            Modifier res_mod = {
                .kind = MODIFIER_RESISTANCE,
            };

            set_damage_value_of_type(&res_mod.as.resistance_modifier, DMG_KIND_LIGHTNING, 0);

            item_add_modifier(item, dmg_mod);
            //item_add_modifier(item, res_mod);

            inventory_add_item(&inventory->inventory, item_with_id.id);

            //try_equip_item_in_slot(&world->item_manager, &equipment->equipment, item_with_id.id, EQUIP_SLOT_HEAD);
        }
    }
}
