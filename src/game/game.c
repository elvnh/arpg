#include "game.h"
#include "asset.h"
#include "base/allocator.h"
#include "base/format.h"
#include "base/free_list_arena.h"
#include "base/linear_arena.h"
#include "base/list.h"
#include "base/matrix.h"
#include "base/rectangle.h"
#include "base/ring_buffer.h"
#include "base/sl_list.h"
#include "base/rgba.h"
#include "base/maths.h"
#include "base/string8.h"
#include "base/utils.h"
#include "base/vector.h"
#include "base/line.h"
#include "game/camera.h"
#include "game/component.h"
#include "game/damage.h"
#include "game/entity.h"
#include "game/entity_system.h"
#include "game/game_world.h"
#include "game/health.h"
#include "game/quad_tree.h"
#include "game/renderer/render_key.h"
#include "game/tilemap.h"
#include "game/ui/ui_builder.h"
#include "game/ui/ui_core.h"
#include "game/ui/widget.h"
#include "input.h"
#include "collision.h"
#include "renderer/render_batch.h"

#define WORLD_ARENA_SIZE MB(64)
#define INTERSECTION_TABLE_ARENA_SIZE (128 * SIZEOF(CollisionEvent))
#define INTERSECTION_TABLE_SIZE 512

// TODO: move these collision table functions elsewhere
static CollisionTable collision_table_create(LinearArena *parent_arena)
{
    CollisionTable result = {0};
    result.table = la_allocate_array(parent_arena, CollisionList, INTERSECTION_TABLE_SIZE);
    result.table_size = INTERSECTION_TABLE_SIZE;
    result.arena = la_create(la_allocator(parent_arena), INTERSECTION_TABLE_ARENA_SIZE);

    return result;
}

static CollisionEvent *collision_table_find(CollisionTable *table, EntityID a, EntityID b)
{
    EntityPair searched_pair = entity_pair_create(a, b);
    u64 hash = entity_pair_hash(searched_pair);
    ssize index = mod_index(hash, table->table_size);

    CollisionList *list = &table->table[index];

    CollisionEvent *node = 0;
    for (node = sl_list_head(list); node; node = sl_list_next(node)) {
        if (entity_pair_equal(searched_pair, node->entity_pair)) {
            break;
        }
    }

    return node;
}

static void collision_table_insert(CollisionTable *table, EntityID a, EntityID b)
{
    if (!collision_table_find(table, a, b)) {
        EntityPair entity_pair = entity_pair_create(a, b);
        u64 hash = entity_pair_hash(entity_pair);
        ssize index = mod_index(hash, table->table_size);

        CollisionEvent *new_node = la_allocate_item(&table->arena, CollisionEvent);
        new_node->entity_pair = entity_pair;

        CollisionList *list = &table->table[index];
        sl_list_push_back(list, new_node);
    } else {
        ASSERT(0);
    }
}

static void swap_and_reset_collision_tables(GameWorld *world)
{
    CollisionTable tmp = world->previous_frame_collisions;
    world->previous_frame_collisions = world->current_frame_collisions;
    world->current_frame_collisions = tmp;

    la_reset(&world->current_frame_collisions.arena);

    // TODO: this can just be a mem_zero to reset head and tail pointers
    for (ssize i = 0; i < world->current_frame_collisions.table_size; ++i) {
        CollisionList *list = &world->current_frame_collisions.table[i];
        sl_list_clear(list);
    }
}

static b32 particle_spawner_is_finished(ParticleSpawner *ps)
{
    b32 result = (ring_length(&ps->particle_buffer) == 0)
        && (ps->particles_left_to_spawn <= 0);

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


// TODO: don't update particle spawners when out of sight of player since they don't affect gameplay
static void component_update_particle_spawner(Entity *entity, ParticleSpawner *ps, Vector2 position, f32 dt)
{
    s32 particles_to_spawn_this_frame = 0;

    switch (ps->config.kind) {
        case PS_SPAWN_DISTRIBUTED: {
            ps->particle_timer += ps->config.particles_per_second * dt;

            particles_to_spawn_this_frame = MIN((s32)ps->particle_timer, ps->particles_left_to_spawn);
            ps->particle_timer -= (f32)particles_to_spawn_this_frame;
            ps->particles_left_to_spawn -= particles_to_spawn_this_frame;
        } break;

        case PS_SPAWN_ALL_AT_ONCE: {
            particles_to_spawn_this_frame = ps->particles_left_to_spawn;
            ps->particles_left_to_spawn = 0;
        } break;
    }

    for (s32 i = 0; i < particles_to_spawn_this_frame; ++i) {
        f32 x = cos_f32(((f32)rand() / (f32)RAND_MAX) * PI * 2.0f);
        f32 y = sin_f32(((f32)rand() / (f32)RAND_MAX) * PI * 2.0f);

        f32 min_speed = ps->config.particle_speed * 0.5f;
        f32 max_speed = ps->config.particle_speed * 1.5f;
        f32 speed = min_speed + ((f32)rand() / (f32)RAND_MAX) * (max_speed - min_speed);

        // TODO: color variance
        Particle new_particle = {
            .timer = 0.0f,
            .lifetime = ps->config.particle_lifetime,
            .position = position,
            .velocity = v2_mul_s(v2(x, y), speed)
        };

        ring_push_overwrite(&ps->particle_buffer, &new_particle);
    }

    for (ssize i = 0; i < ring_length(&ps->particle_buffer); ++i) {
        Particle *p = ring_at(&ps->particle_buffer, i);

        if (p->timer > p->lifetime) {
            ring_swap_remove(&ps->particle_buffer, i);
        }

        p->timer += dt;
        p->position = v2_add(p->position, v2_mul_s(p->velocity, dt));
        p->velocity = v2_add(p->velocity, v2(0, -1.0f));
        p->velocity = v2_add(p->velocity, v2_mul_s(p->velocity, -0.009f));
    }


    if ((ps->action_when_done == PS_WHEN_DONE_REMOVE_COMPONENT) && particle_spawner_is_finished(ps)) {
        es_remove_component(entity, ParticleSpawner);
    }
}

static void entity_update(GameWorld *world, Entity *entity, f32 dt)
{
    if (es_has_component(entity, ParticleSpawner)) {
        ParticleSpawner *particle_spawner = es_get_component(entity, ParticleSpawner);

        component_update_particle_spawner(entity, particle_spawner, entity->position, dt);
    }

    if (es_has_component(entity, LifetimeComponent)) {
        LifetimeComponent *lifetime = es_get_component(entity, LifetimeComponent);
        lifetime->time_to_live -= dt;
    }

    if (entity_should_die(entity)) {
        // NOTE: won't be removed until after this frame
        es_schedule_entity_for_removal(entity);

        if (es_has_component(entity, OnDeathComponent)) {
            OnDeathComponent *on_death = es_get_component(entity, OnDeathComponent);

            switch (on_death->kind) {
                case DEATH_EFFECT_SPAWN_PARTICLES: {
                    EntityID id = es_create_entity(&world->entities);
                    Entity *spawner = es_get_entity(&world->entities, id);
                    spawner->position = entity->position;

                    ParticleSpawner *ps = es_add_component(spawner, ParticleSpawner);
                    particle_spawner_initialize(ps, on_death->as.spawn_particles.config);
                    ps->action_when_done = PS_WHEN_DONE_REMOVE_ENTITY;
                } break;
            }
        }
    }

    es_update_entity_quad_tree_location(&world->entities, entity, &world->world_arena);
}

static void entity_render(Entity *entity, RenderBatch *rb, const AssetList *assets, LinearArena *scratch,
    DebugState *debug_state)
{
    if (es_has_component(entity, SpriteComponent)) {
        SpriteComponent *sprite = es_get_component(entity, SpriteComponent);

        Rectangle sprite_rect = { entity->position, sprite->size };
        rb_push_sprite(rb, scratch, sprite->texture, sprite_rect, RGBA32_WHITE, assets->texture_shader, RENDER_LAYER_ENTITIES);
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

static Vector2 tile_to_world_coords(Vector2i tile_coords)
{
    Vector2 result = {
        (f32)tile_coords.x * TILE_SIZE,
        (f32)tile_coords.y * TILE_SIZE
    };

    return result;
}

static Rectangle get_entity_collider_rectangle(Entity *entity, ColliderComponent *collider)
{
    Rectangle result = {
        entity->position,
        collider->size
    };

    return result;
}

static b32 entities_intersected_this_frame(GameWorld *world, EntityID a, EntityID b)
{
    b32 result = collision_table_find(&world->current_frame_collisions, a, b) != 0;

    return result;
}


static b32 entities_intersected_previous_frame(GameWorld *world, EntityID a, EntityID b)
{
    b32 result = collision_table_find(&world->previous_frame_collisions, a, b) != 0;

    return result;
}

static void deal_damage_to_entity(Entity *entity, HealthComponent *health, Damage damage)
{
    (void)entity;
    s64 damage_taken = 0;

    if (es_has_component(entity, StatsComponent)) {
        StatsComponent *stats = es_get_component(entity, StatsComponent);
        damage_taken = calculate_damage_received(stats->resistances, damage);
    } else {
        damage_taken = damage_sum(damage);
    }

    health->health.hitpoints -= damage_taken;
}

static void try_deal_damage_field_damage(Entity *a, Entity *b)
{
    HealthComponent *receiver_health = es_get_component(a, HealthComponent);
    DamageFieldComponent *sender_damage_field = es_get_component(b, DamageFieldComponent);

    if (receiver_health && sender_damage_field) {
        deal_damage_to_entity(a, receiver_health, sender_damage_field->damage);
    }
}

static void execute_collision_effects(Entity *entity, ColliderComponent *collider,
    CollisionInfo collision, s32 index, ObjectKind colliding_with_obj_kind)
{
    ASSERT((index == 0) || (index == 1));

    b32 effect_executed = false;

    for (s32 i = 0; i < collider->on_collide_effects.count; ++i) {
        OnCollisionEffect effect = collider->on_collide_effects.effects[i];

        if (effect.affects_object_kinds & colliding_with_obj_kind) {
            effect_executed = true;

            switch (effect.kind) {
                case ON_COLLIDE_BOUNCE: {
                    if (index == 0) {
                        entity->position = collision.new_position_a;
                        entity->velocity = v2_reflect(entity->velocity, collision.collision_normal);
                    } else {
                        entity->position = collision.new_position_b;
                        entity->velocity = v2_reflect(entity->velocity, v2_neg(collision.collision_normal));
                    }
                } break;

                case ON_COLLIDE_PASS_THROUGH: {} break;
            }
        }
    }

    if (!effect_executed) {
        if (index == 0) {
            entity->position = collision.new_position_a;
            entity->velocity = collision.new_velocity_a;
        } else {
            entity->position = collision.new_position_b;
            entity->velocity = collision.new_velocity_b;
        }
    }
}

static f32 entity_vs_entity_collision(GameWorld *world, Entity *a,
    ColliderComponent *collider_a, Entity *b, ColliderComponent *collider_b,
    f32 movement_fraction_left, f32 dt)
{
    EntityID id_a = es_get_id_of_entity(&world->entities, a);
    EntityID id_b = es_get_id_of_entity(&world->entities, b);

    CollisionRule *rule_for_entities = collision_rule_find(&world->collision_rules, id_a, id_b);
    b32 should_collide = !rule_for_entities || rule_for_entities->should_collide;

    if (should_collide) {
        Rectangle rect_a = get_entity_collider_rectangle(a, collider_a);
        Rectangle rect_b = get_entity_collider_rectangle(b, collider_b);

        CollisionInfo collision = collision_rect_vs_rect(movement_fraction_left, rect_a, rect_b,
            a->velocity, b->velocity, dt);

        if ((collision.collision_state != COLL_NOT_COLLIDING) && !entities_intersected_this_frame(world, id_a, id_b)) {
            // TODO: only handle closest collision
            movement_fraction_left = collision.movement_fraction_left;

            execute_collision_effects(a, collider_a, collision, 0, OBJECT_KIND_ENTITIES);
            execute_collision_effects(b, collider_b, collision, 1, OBJECT_KIND_ENTITIES);

            // TODO: it should be possible to collide multiple frames in a row
            if (!entities_intersected_previous_frame(world, id_a, id_b)) {
                try_deal_damage_field_damage(a, b);
                try_deal_damage_field_damage(b, a);
            }

            collision_table_insert(&world->current_frame_collisions, id_a, id_b);
        }
    }

    return movement_fraction_left;
}

static f32 entity_vs_tilemap_collision(Entity *entity, ColliderComponent *collider,
    GameWorld *world, f32 movement_fraction_left, f32 dt)
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

        execute_collision_effects(entity, collider, closest_collision, 0, OBJECT_KIND_TILES);
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

static void handle_collision_and_movement(GameWorld *world, f32 dt, LinearArena *frame_arena)
{
    // TODO: don't access alive entity array directly
    for (EntityIndex i = 0; i < world->entities.alive_entity_count; ++i) {
        // TODO: this seems bug prone

        EntityID id_a = world->entities.alive_entity_ids[i];
        Entity *a = es_get_entity(&world->entities, id_a);
        ColliderComponent *collider_a = es_get_component(a, ColliderComponent);
        ASSERT(a);

        f32 movement_fraction_left = 1.0f;

        if (collider_a) {
            movement_fraction_left = entity_vs_tilemap_collision(a, collider_a, world, movement_fraction_left, dt);

            Rectangle collision_area = get_entity_collision_area(a, collider_a);
            EntityIDList entities_in_area = es_get_entities_in_area(&world->entities, collision_area, frame_arena);

            for (EntityIDNode *node = sl_list_head(&entities_in_area); node; node = sl_list_next(node)) {
                if (!entity_id_equal(node->id, id_a)) {
                    Entity *b = es_get_entity(&world->entities, node->id);

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
        }
    }
}

static void spawn_projectile(GameWorld *world, Vector2 pos, EntityID spawner_id, const AssetList *assets)
{
    // TODO: return id and entity pointer when creating
    EntityID id = es_create_entity(&world->entities);
    Entity *entity = es_get_entity(&world->entities, id);

    entity->position = pos;
    entity->velocity = v2(250.f, 0.0f);

    ColliderComponent *collider = es_add_component(entity, ColliderComponent);
    collider->size = v2(16.0f, 16.0f);
    collider->non_blocking = true;

    add_collide_effect(collider, (OnCollisionEffect){
            .kind = ON_COLLIDE_BOUNCE,
            .affects_object_kinds = OBJECT_KIND_ENTITIES// | OBJECT_KIND_TILES
        });

    DamageFieldComponent *damage_field = es_add_component(entity, DamageFieldComponent);
    damage_field->damage.types.values[DMG_KIND_FIRE] = 4;

    SpriteComponent *sprite = es_add_component(entity, SpriteComponent);
    sprite->size = v2(16.0f, 16.0f);
    sprite->texture = assets->default_texture;

    LifetimeComponent *lifetime = es_add_component(entity, LifetimeComponent);
    lifetime->time_to_live = 2.0f;

    OnDeathComponent *on_death = es_add_component(entity, OnDeathComponent);
    on_death->kind = DEATH_EFFECT_SPAWN_PARTICLES;
    on_death->as.spawn_particles.config = (ParticleSpawnerConfig) {
        .kind = PS_SPAWN_ALL_AT_ONCE,
        .particle_color = (RGBA32){1.0f, 0.2f, 0.1f, 0.7f},
        .particle_size = 4.0f,
        .particle_lifetime = 1.0f,
        .total_particles_to_spawn = 50,
        .particle_speed = 100.0f,
    };
    // TODO: configure particles

    collision_rule_add(&world->collision_rules, spawner_id, id, false, &world->world_arena);
}

static void world_update(GameWorld *world, const Input *input, f32 dt, const AssetList *assets, LinearArena *frame_arena)
{
    if (world->entities.alive_entity_count < 1) {
        return;
    }

    EntityID player_id = world->entities.alive_entity_ids[0];
    Entity *player = es_get_entity(&world->entities, player_id);
    ASSERT(player);

    {
        Vector2 target = player->position;

        if (es_has_component(player, ColliderComponent)) {
            ColliderComponent *collider = es_get_component(player, ColliderComponent);
            target = v2_add(target, v2_div_s(collider->size, 2));
        }

        if (input_is_key_pressed(input, KEY_G)) {
            spawn_projectile(world, player->position, player_id, assets);
        }

        camera_set_target(&world->camera, target);
    }

    camera_zoom(&world->camera, (s32)input->scroll_delta);
    camera_update(&world->camera, dt);

    f32 speed = 350.0f;

    if (input_is_key_held(input, KEY_LEFT_SHIFT)) {
        speed *= 3.0f;
    }

    {
	Vector2 acceleration = {0};

	if (input_is_key_down(input, KEY_W)) {
	    acceleration.y = 1.0f;
	} else if (input_is_key_down(input, KEY_S)) {
	    acceleration.y = -1.0f;
	}

	if (input_is_key_down(input, KEY_A)) {
	    acceleration.x = -1.0f;
	} else if (input_is_key_down(input, KEY_D)) {
	    acceleration.x = 1.0f;
	}

        Vector2 v = player->velocity;
        Vector2 p = player->position;

        acceleration = v2_norm(acceleration);
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

    handle_collision_and_movement(world, dt, frame_arena);

    // TODO: should any newly spawned entities be updated this frame?
    EntityIndex entity_count = world->entities.alive_entity_count;
    for (EntityIndex i = 0; i < entity_count; ++i) {
        EntityID entity_id = world->entities.alive_entity_ids[i];
        Entity *entity = es_get_entity(&world->entities, entity_id);
        ASSERT(entity);

        entity_update(world, entity, dt);
    }

    // TODO: don't use linked list for this
    EntityIDList inactive_entities = es_get_inactive_entities(&world->entities, frame_arena);

    // NOTE: wait until after all updates have ran before removing collision rules
    for (EntityIDNode *node = list_head(&inactive_entities); node; node = list_next(node)) {
        collision_rule_remove_rules_with_entity(&world->collision_rules, node->id);
    }

    // TODO: should this be done at beginning of each frame so inactive entities are rendered?
    // TODO: it would be better to pass list of entities to remove since we just retrieved inactive entities
    es_remove_inactive_entities(&world->entities, frame_arena);

    swap_and_reset_collision_tables(world);
}

static Vector2i world_to_tile_coords(Vector2 world_coords)
{
    Vector2i result = {
        (s32)((f32)world_coords.x / (f32)TILE_SIZE),
        (s32)((f32)world_coords.y / (f32)TILE_SIZE)
    };

    return result;
}

// TODO: this shouldn't need AssetList parameter?
static void world_render(GameWorld *world, RenderBatch *rb, const AssetList *assets, FrameData frame_data,
    LinearArena *frame_arena, DebugState *debug_state)
{
    Rectangle visible_area = camera_get_visible_area(world->camera, frame_data.window_size);
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
                RGBA32 color;

                switch (tile->type) {
                    case TILE_FLOOR: {
                        color = (RGBA32){0.1f, 0.5f, 0.9f, 1.0f};
                    } break;

                    case TILE_WALL: {
                        color = (RGBA32){1.0f, 0.2f, 0.1f, 1.0f};
                    } break;

                        INVALID_DEFAULT_CASE;
                }

                Rectangle tile_rect = {
                    .position = tile_to_world_coords(tile_coords),
                    .size = { (f32)TILE_SIZE, (f32)TILE_SIZE }
                };

                rb_push_rect(rb, frame_arena, tile_rect, color, assets->shape_shader, RENDER_LAYER_TILEMAP);
            }
        }
    }

    // TODO: debug camera that is detached from regular camera
    EntityIDList entities_in_area = qt_get_entities_in_area(&world->entities.quad_tree, visible_area, frame_arena);

    for (EntityIDNode *node = sl_list_head(&entities_in_area); node; node = sl_list_next(node)) {
        Entity *entity = es_get_entity(&world->entities, node->id);

        entity_render(entity, rb, assets, frame_arena, debug_state);
    }
}

static void game_update(GameState *game_state, const Input *input, f32 dt, LinearArena *frame_arena)
{
    (void)frame_arena;

    if (input_is_key_pressed(input, KEY_ESCAPE)) {
        DEBUG_BREAK;
    }

    world_update(&game_state->world, input, dt, &game_state->asset_list, frame_arena);

}

static void render_tree(QuadTreeNode *tree, RenderBatch *rb, LinearArena *arena,
    const AssetList *assets, ssize depth)
{
    if (tree) {
	render_tree(tree->top_left, rb, arena, assets, depth + 1);
	render_tree(tree->top_right, rb, arena, assets, depth + 1);
	render_tree(tree->bottom_right, rb, arena, assets, depth + 1);
	render_tree(tree->bottom_left, rb, arena, assets, depth + 1);

	static const RGBA32 colors[] = {
	    {1.0f, 0.0f, 0.0f, 0.5f},
	    {0.0f, 1.0f, 0.0f, 0.5f},
	    {0.0f, 0.0f, 1.0f, 0.5f},
	    {1.0f, 0.0f, 1.0f, 0.5f},
	    {0.2f, 0.3f, 0.8f, 0.5f},
	    {1.0f, 0.1f, 0.5f, 0.5f},
	    {0.5f, 0.5f, 0.5f, 0.5f},
	    {0.5f, 1.0f, 0.5f, 0.5f},
	};

	ASSERT(depth < ARRAY_COUNT(colors));

	RGBA32 color = colors[depth];
	rb_push_outlined_rect(rb, arena, tree->area, color, 4.0f, assets->shape_shader, 2);
    }
}

static void game_render(GameState *game_state, RenderBatchList *rbs, FrameData frame_data, LinearArena *frame_arena)
{
    Matrix4 proj = camera_get_matrix(game_state->world.camera, frame_data.window_size);
    RenderBatch *rb = rb_list_push_new(rbs, proj, Y_IS_UP, frame_arena);

    world_render(&game_state->world, rb, &game_state->asset_list, frame_data, frame_arena,
	&game_state->debug_state);

    if (game_state->debug_state.quad_tree_overlay) {
        render_tree(&game_state->world.entities.quad_tree.root, rb, frame_arena, &game_state->asset_list, 0);
    }

    if (game_state->debug_state.render_origin) {
        rb_push_rect(rb, frame_arena, (Rectangle){{0, 0}, {8, 8}}, RGBA32_RED,
	    game_state->asset_list.shape_shader, 3);
    }

    rb_sort_entries(rb);
}

static String dbg_arena_usage_string(String name, ssize usage, Allocator allocator)
{
    String result = str_concat(
        name,
        str_concat(
            f32_to_string((f32)usage / 1024.0f, 2, allocator),
            str_lit(" KBs"),
            allocator),
        allocator
    );

    return result;
}

static void debug_ui(UIState *ui, GameState *game_state, GameMemory *game_memory, FrameData frame_data)
{
    ui_begin_container(ui, str_lit("root"), V2_ZERO, UI_SIZE_KIND_SUM_OF_CHILDREN, 8.0f);

    ssize temp_arena_memory_usage = la_get_memory_usage(&game_memory->temporary_memory);
    ssize perm_arena_memory_usage = la_get_memory_usage(&game_memory->permanent_memory);
    ssize world_arena_memory_usage = la_get_memory_usage(&game_state->world.world_arena);
    // TODO: asset memory usage

    Allocator temp_alloc = la_allocator(&game_memory->temporary_memory);

    String frame_time_str = str_concat(
        str_lit("Frame time: "),
        f32_to_string(frame_data.dt, 5, temp_alloc),
        temp_alloc
    );

    String fps_str = str_concat(
        str_lit("FPS: "),
        f32_to_string(game_state->debug_state.average_fps, 2, temp_alloc),
        temp_alloc
    );

    String temp_arena_str = dbg_arena_usage_string(str_lit("Frame arena: "), temp_arena_memory_usage, temp_alloc);
    String perm_arena_str = dbg_arena_usage_string(str_lit("Permanent arena: "), perm_arena_memory_usage, temp_alloc);
    String world_arena_str = dbg_arena_usage_string(str_lit("World arena: "), world_arena_memory_usage, temp_alloc);

    ssize qt_nodes = qt_get_node_count(&game_state->world.entities.quad_tree);
    String node_string = str_concat(str_lit("Quad tree nodes: "), ssize_to_string(qt_nodes, temp_alloc), temp_alloc);

    String entity_string = str_concat(
        str_lit("Alive entity count: "),
        ssize_to_string(game_state->world.entities.alive_entity_count, temp_alloc),
        temp_alloc
    );


    ui_text(ui, frame_time_str);
    ui_text(ui, fps_str);

    ui_spacing(ui, 8);

    ui_text(ui, temp_arena_str);
    ui_text(ui, perm_arena_str);
    ui_text(ui, world_arena_str);
    ui_text(ui, node_string);
    ui_text(ui, entity_string);

    ui_spacing(ui, 8);

    ui_checkbox(ui, str_lit("Render quad tree"),     &game_state->debug_state.quad_tree_overlay);
    ui_checkbox(ui, str_lit("Render colliders"),     &game_state->debug_state.render_colliders);
    ui_checkbox(ui, str_lit("Render origin"),        &game_state->debug_state.render_origin);
    ui_checkbox(ui, str_lit("Render entity bounds"), &game_state->debug_state.render_entity_bounds);

    ui_spacing(ui, 8);

    {
        String camera_pos_str = str_concat(
            str_lit("Camera position: "),
            v2_to_string(game_state->world.camera.position, temp_alloc),
            temp_alloc
        );

        ui_text(ui, camera_pos_str);
        ui_text(ui, str_concat(str_lit("Zoom: "), f32_to_string(game_state->world.camera.zoom, 2, temp_alloc), temp_alloc));
    }

    ui_spacing(ui, 8);

    // TODO: display more stats about hovered entity
    if (!entity_id_equal(game_state->debug_state.hovered_entity, NULL_ENTITY_ID)) {
        const Entity *entity = es_get_entity(&game_state->world.entities, game_state->debug_state.hovered_entity);

        String entity_str = str_concat(
            str_lit("Hovered entity: "),
            ssize_to_string(game_state->debug_state.hovered_entity.slot_id, temp_alloc),
            temp_alloc
        );

        String entity_pos_str = str_concat(
            str_lit("Position: "), v2_to_string(entity->position, temp_alloc), temp_alloc
        );

        ui_text(ui, entity_str);
        ui_text(ui, entity_pos_str);
    }

    ui_pop_container(ui);
}

static void game_update_and_render_ui(UIState *ui, GameState *game_state, GameMemory *game_memory, FrameData frame_data)
{
    debug_ui(ui, game_state, game_memory, frame_data);
}

void debug_update(GameState *game_state, FrameData frame_data, LinearArena *frame_arena)
{
    f32 curr_fps = 1.0f / frame_data.dt;
    f32 avg_fps = game_state->debug_state.average_fps;

    // NOTE: lower alpha value means more smoothing
    game_state->debug_state.average_fps = exponential_moving_avg(avg_fps, curr_fps, 0.9f);

    Vector2 hovered_coords = screen_to_world_coords(
        game_state->world.camera,
        frame_data.input->mouse_position,
        frame_data.window_size
    );

    Rectangle hovered_rect = {hovered_coords, {1, 1}};
    EntityIDList hovered_entities = es_get_entities_in_area(&game_state->world.entities, hovered_rect, frame_arena);

    if (!sl_list_is_empty(&hovered_entities)) {
        game_state->debug_state.hovered_entity = sl_list_head(&hovered_entities)->id;
    } else {
        game_state->debug_state.hovered_entity = NULL_ENTITY_ID;
    }
}

void game_update_and_render(GameState *game_state, PlatformCode platform_code, RenderBatchList *rbs,
    FrameData frame_data, GameMemory *game_memory)
{
    debug_update(game_state, frame_data, &game_memory->temporary_memory);

    game_update(game_state, frame_data.input, frame_data.dt, &game_memory->temporary_memory);
    game_render(game_state, rbs, frame_data, &game_memory->temporary_memory);

    if (input_is_key_pressed(frame_data.input, KEY_T)) {
        game_state->debug_state.debug_menu_active = !game_state->debug_state.debug_menu_active;
    }

    if (game_state->debug_state.debug_menu_active) {
        ui_core_begin_frame(&game_state->ui);

        game_update_and_render_ui(&game_state->ui, game_state, game_memory, frame_data);

        Matrix4 proj = mat4_orthographic(frame_data.window_size, Y_IS_DOWN);
        RenderBatch *ui_rb = rb_list_push_new(rbs, proj, Y_IS_DOWN, &game_memory->temporary_memory);
        ui_core_end_frame(&game_state->ui, frame_data.input, ui_rb, &game_state->asset_list, platform_code);
    }
}

void game_initialize(GameState *game_state, GameMemory *game_memory)
{
    // TODO: move to world_initialize
    game_state->world.world_arena = la_create(la_allocator(&game_memory->permanent_memory), WORLD_ARENA_SIZE);

    game_state->world.previous_frame_collisions = collision_table_create(&game_state->world.world_arena);
    game_state->world.current_frame_collisions = collision_table_create(&game_state->world.world_arena);

    game_state->sb = str_builder_allocate(32, la_allocator(&game_memory->permanent_memory));
    game_state->debug_state.average_fps = 60.0f;

    UIStyle default_ui_style = {
        .font = game_state->asset_list.default_font
    };

    ui_core_initialize(&game_state->ui, default_ui_style, &game_memory->permanent_memory);

    for (s32 y = 0; y < 8; ++y) {
	for (s32 x = 0; x < 8; ++x) {
	    tilemap_insert_tile(&game_state->world.tilemap, (Vector2i){x, y}, TILE_FLOOR,
		&game_state->world.world_arena);
	}
    }

    Rectangle tilemap_area = tilemap_get_bounding_box(&game_state->world.tilemap);
    es_initialize(&game_state->world.entities, tilemap_area);

    for (s32 i = 0; i < 2; ++i) {
        EntityID id = es_create_entity(&game_state->world.entities);
        Entity *entity = es_get_entity(&game_state->world.entities, id);
        entity->position = v2((f32)(64 * (i * 2)), 64.0f * ((f32)i * 2.0f));

        ASSERT(!es_has_component(entity, ColliderComponent));

        ColliderComponent *collider = es_add_component(entity, ColliderComponent);
        collider->size = v2(16.0f, 16.0f);

        HealthComponent *hp = es_add_component(entity, HealthComponent);
        hp->health.hitpoints = 10;

        ASSERT(es_has_component(entity, ColliderComponent));
        ASSERT(es_has_component(entity, HealthComponent));

#if 1
        ParticleSpawner *spawner = es_add_component(entity, ParticleSpawner);
        spawner->action_when_done = PS_WHEN_DONE_DO_NOTHING;
        ParticleSpawnerConfig config = {
            .particle_color = (RGBA32){0.2f, 0.9f, 0.1f, 0.4f},
            .particle_size = 4.0f,
            .particles_per_second = 250.0f,
            .total_particles_to_spawn = 10.0f,
            .particle_speed = 100.0f,
            .particle_lifetime = 1.0f
        };

        particle_spawner_initialize(spawner, config);

#endif
        SpriteComponent *sprite = es_add_component(entity, SpriteComponent);
        sprite->texture = game_state->asset_list.default_texture;
        sprite->size = v2(32, 32);

        StatsComponent *stats = es_add_component(entity, StatsComponent);
        stats->resistances.base_resistances.values[DMG_KIND_FIRE] = 10;
        stats->resistances.flat_bonuses.values[DMG_KIND_FIRE] = 5;
    }
}
