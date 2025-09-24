#include "game.h"
#include "asset.h"
#include "base/allocator.h"
#include "base/free_list_arena.h"
#include "base/linear_arena.h"
#include "base/rectangle.h"
#include "base/rgba.h"
#include "base/maths.h"
#include "base/utils.h"
#include "base/vector.h"
#include "base/line.h"
#include "game/component.h"
#include "game/entity.h"
#include "game/game_world.h"
#include "game/quad_tree.h"
#include "game/tilemap.h"
#include "input.h"
#include "collision.h"
#include "renderer/render_batch.h"

static void entity_render(Entity *entity, RenderBatch *rb, const AssetList *assets, LinearArena *scratch)
{
    if (es_has_components(entity, component_flag(PhysicsComponent) | component_flag(ColliderComponent))) {
        PhysicsComponent *physics = es_get_component(entity, PhysicsComponent);
        ColliderComponent *collider = es_get_component(entity, ColliderComponent);

        Rectangle rect = {
            .position = physics->position,
            .size = collider->size
        };

        rb_push_rect(rb, scratch, rect, entity->color, assets->shader2, RENDER_LAYER_ENTITIES);
    }
}

static Vector2i world_to_tile_coords(Vector2 world_coords)
{
    Vector2i result = {
        (s32)(world_coords.x / TILE_SIZE),
        (s32)(world_coords.y / TILE_SIZE)
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

static Rectangle get_entity_rect(PhysicsComponent *physics, ColliderComponent *collider)
{
    Rectangle result = {
        physics->position,
        collider->size
    };

    return result;
}

static b32 entities_should_collide(Entity *a, Entity *b)
{
    (void)a;
    (void)b;
    // TODO: collision rules
    return true;
}


static b32 entity_id_equal(EntityID lhs, EntityID rhs)
{
    b32 result = (lhs.slot_index == rhs.slot_index) && (lhs.generation == rhs.generation);

    return result;
}

static b32 entity_id_less_than(EntityID lhs, EntityID rhs)
{
    b32 result = lhs.slot_index < rhs.slot_index;

    return result;
}

static CollisionRule collision_rule_create(EntityID a, EntityID b)
{
    if (entity_id_less_than(b, a)) {
        EntityID tmp = a;
        a = b;
        b = tmp;
    }

    CollisionRule result = {0};
    result.a = a;
    result.b = b;

    return result;
}

static ssize collision_rule_hash_index(CollisionRule rule, const CollisionRuleTable *table)
{
    ssize length = ARRAY_COUNT(table->collision_rules);
    u64 hash = 0; // TODO: hash

    ssize result = hash_index(hash, length);

    return result;
}

static CollisionRule *collision_rule_find(GameWorld *world, EntityID a, EntityID b)
{
    CollisionRule rule = collision_rule_create(a, b);
    ssize index = collision_rule_hash_index(rule, &world->collision_rules);

    CollisionRule *current_rule = world->collision_rules.collision_rules[index];
    while (current_rule) {
        if (entity_id_equal(current_rule->a, a) && entity_id_equal(current_rule->b, b)) {
            break;
        }

	current_rule = current_rule->next;
    }

    return current_rule;
}

static void collision_rule_add(GameWorld *world, EntityID a, EntityID b, b32 should_collide)
{
    // TODO: remove collision rules when entity dies
    // TODO: reuse already allocated rules
    ASSERT(!entity_id_equal(a, b));

    CollisionRule *rule = world->collision_rules.first_free_node;
    if (!rule) {
	rule = la_allocate_item(&world->world_arena, CollisionRule);
    } else {
	world->collision_rules.first_free_node = rule->next;
    }

    *rule = collision_rule_create(a, b);
    rule->should_collide = should_collide;

    ASSERT(!collision_rule_find(world, rule->a, rule->b));

    ssize index = collision_rule_hash_index(*rule, &world->collision_rules);
    CollisionRule *rule_in_slot = world->collision_rules.collision_rules[index];

    if (rule_in_slot) {
        rule->next = rule_in_slot;
    }

    world->collision_rules.collision_rules[index] = rule;
}

static void remove_collision_rules_with_entity(CollisionRuleTable *table, EntityID a)
{
    for (s32 i = 0; i < ARRAY_COUNT(table->collision_rules); ++i) {
	CollisionRule *curr_rule = table->collision_rules[i];
	CollisionRule *prev_rule = 0;

	while (curr_rule) {
	    if (entity_id_equal(curr_rule->a, a) || entity_id_equal(curr_rule->b, a)) {
		if (prev_rule) {
		    prev_rule->next = curr_rule->next;
		} else {
		    table->collision_rules[i] = curr_rule->next;
		}

		curr_rule->next = table->first_free_node;
		table->first_free_node = curr_rule;
	    }

	    prev_rule = curr_rule;
	    curr_rule = curr_rule->next;
	}
    }
}

static void entity_vs_entity_collision(GameWorld *world, Entity *a, PhysicsComponent *physics_a,
    ColliderComponent *collider_a, Entity *b, PhysicsComponent *physics_b, ColliderComponent *collider_b,
    f32 *movement_fraction_left, f32 dt)
{
    EntityID id_a = es_get_id_of_entity(&world->entities, a);
    EntityID id_b = es_get_id_of_entity(&world->entities, b);

    CollisionRule *rule_for_entities = collision_rule_find(world, id_a, id_b);
    b32 should_collide = !rule_for_entities || rule_for_entities->should_collide;

    if (should_collide) {
        Rectangle rect_a = get_entity_rect(physics_a, collider_a);
        Rectangle rect_b = get_entity_rect(physics_b, collider_b);

        CollisionInfo collision = collision_rect_vs_rect(*movement_fraction_left, rect_a, rect_b,
            physics_a->velocity, physics_b->velocity, dt);

        if (!collider_a->non_blocking && !collider_b->non_blocking) {
            physics_a->position = collision.new_position_a;
            physics_b->position = collision.new_position_b;

            physics_a->velocity = collision.new_velocity_a;
            physics_b->velocity = collision.new_velocity_b;

            *movement_fraction_left = collision.movement_fraction_left;
        }

        if (collision.are_colliding) {
            if (es_has_component(a, DamageComponent)) {
                printf("Damage A: %d\n", es_get_component(a, DamageComponent)->damage);
            }

            if (es_has_component(b, DamageComponent)) {
                printf("Damage B: %d\n", es_get_component(b, DamageComponent)->damage);
            }
        }
    }
}

static void entity_vs_tilemap_collision(PhysicsComponent *physics, ColliderComponent *collider,
    GameWorld *world, f32 *movement_fraction_left, f32 dt)
{
    Vector2 old_entity_pos = physics->position;
    Vector2 new_entity_pos = v2_add(physics->position, v2_mul_s(physics->velocity, dt));
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

    for (s32 y = min_tile_y - 1; y <= max_tile_y + 1; ++y) {
        for (s32 x = min_tile_x - 1; x <= max_tile_x + 1; ++x) {
            Vector2i tile_coords = {x, y};
            Tile *tile = tilemap_get_tile(&world->tilemap, tile_coords);

            if (!tile || (tile->type == TILE_WALL)) {
                Rectangle entity_rect = get_entity_rect(physics, collider);
                Rectangle tile_rect = {
                    tile_to_world_coords(tile_coords),
                    {(f32)TILE_SIZE, (f32)TILE_SIZE },
                };

                CollisionInfo collision = collision_rect_vs_rect(*movement_fraction_left, entity_rect,
		    tile_rect, physics->velocity, V2_ZERO, dt);

                physics->position = collision.new_position_a;
                physics->velocity = collision.new_velocity_a;
                *movement_fraction_left = collision.movement_fraction_left;

                ASSERT(v2_eq(collision.new_position_b, tile_rect.position));
                ASSERT(v2_eq(collision.new_velocity_b, V2_ZERO));
            }
        }
    }
}

static void handle_collision_and_movement(GameWorld *world, f32 dt)
{
    for (EntityIndex i = 0; i < world->entities.alive_entity_count; ++i) {
        // TODO: this seems bug prone

        EntityID id_a = world->entities.alive_entity_ids[i];
        Entity *a = es_get_entity(&world->entities,id_a);
        PhysicsComponent *physics_a = es_get_component(a, PhysicsComponent);

        if (physics_a) {
            ColliderComponent *collider_a = es_get_component(a, ColliderComponent);
            f32 movement_fraction_left = 1.0f;

            if (collider_a) {
                entity_vs_tilemap_collision(physics_a, collider_a, world, &movement_fraction_left, dt);

                for (EntityIndex j = i + 1; j < world->entities.alive_entity_count; ++j) {
                    EntityID id_b = world->entities.alive_entity_ids[j];
                    Entity *b = es_get_entity(&world->entities, id_b);

                    PhysicsComponent *physics_b = es_get_component(b, PhysicsComponent);
                    ColliderComponent *collider_b = es_get_component(b, ColliderComponent);

                    if (!physics_b || !collider_b) {
                        continue;
                    }

                    if (entities_should_collide(a, b)) {
                        entity_vs_entity_collision(world, a, physics_a, collider_a, b, physics_b, collider_b,
                            &movement_fraction_left, dt);
                    }
                }
            }

            ASSERT(movement_fraction_left >= 0.0f);
            ASSERT(movement_fraction_left <= 1.0f);

            Vector2 to_move_this_frame = v2_mul_s(v2_mul_s(physics_a->velocity, movement_fraction_left), dt);
            physics_a->position = v2_add(physics_a->position, to_move_this_frame);

            es_set_entity_position(&world->entities, a, physics_a->position, &world->world_arena);
        }
    }
}

static void spawn_projectile(GameWorld *world, Vector2 pos, EntityID spawner_id)
{
    // TODO: return id and entity pointer when creating
    EntityID id = es_create_entity(&world->entities);
    Entity *entity = es_get_entity(&world->entities, id);
    entity->color = RGBA32_RED;

    PhysicsComponent *physics = es_add_component(entity, PhysicsComponent);
    physics->position = pos;
    physics->velocity = v2(100.0f, 0.0f);

    ColliderComponent *collider = es_add_component(entity, ColliderComponent);
    collider->size = v2(16.0f, 16.0f);
    collider->non_blocking = true;

    DamageComponent *damage = es_add_component(entity, DamageComponent);
    damage->damage = 10;

    es_set_entity_area(&world->entities, entity, (Rectangle){physics->position, collider->size},
	&world->world_arena);

    collision_rule_add(world, spawner_id, id, false);

    remove_collision_rules_with_entity(&world->collision_rules, id);

    collision_rule_add(world, spawner_id, id, false);
}

static void world_update(GameWorld *world, const Input *input, f32 dt)
{
    ASSERT(world->entities.alive_entity_count >= 1);

    EntityID player_id = world->entities.alive_entity_ids[0];
    Entity *player = es_get_entity(&world->entities, player_id);
    PhysicsComponent *physics = es_get_component(player, PhysicsComponent);

    if (physics) {
        Vector2 target = physics->position;

        if (es_has_component(player, ColliderComponent)) {
            ColliderComponent *collider = es_get_component(player, ColliderComponent);
            target = v2_add(target, v2_div_s(collider->size, 2));
        }

        if (input_is_key_pressed(input, KEY_G)) {
            spawn_projectile(world, physics->position, player_id);
        }

        camera_set_target(&world->camera, target);
    }

    camera_zoom(&world->camera, (s32)input->scroll_delta);
    camera_update(&world->camera, dt);

    f32 speed = 250.0f;

    if (input_is_key_held(input, KEY_LEFT_SHIFT)) {
        speed *= 3.0f;
    }

    {
	Vector2 dir = {0};

	if (input_is_key_down(input, KEY_W)) {
	    dir.y = 1.0f;
	} else if (input_is_key_down(input, KEY_S)) {
	    dir.y = -1.0f;
	}

	if (input_is_key_down(input, KEY_A)) {
	    dir.x = -1.0f;
	} else if (input_is_key_down(input, KEY_D)) {
	    dir.x = 1.0f;
	}

        physics->velocity = v2_mul_s(v2_norm(dir), speed);
    }

    handle_collision_and_movement(world, dt);
}

static void world_render(GameWorld *world, RenderBatch *rb, const AssetList *assets, FrameData frame_data,
    LinearArena *frame_arena)
{
    // TODO: only render visible part of tilemap
    s32 min_x = world->tilemap.min_x;
    s32 max_x = world->tilemap.max_x;
    s32 min_y = world->tilemap.min_y;
    s32 max_y = world->tilemap.max_y;

    for (s32 y = min_y; y <= max_y; ++y) {
        for (s32 x = min_x; x <= max_x; ++x) {
            Vector2i tile_coords = {x, y};
            Tile *tile = tilemap_get_tile(&world->tilemap, tile_coords);
            RGBA32 color;

            if (tile) {
                switch (tile->type) {
                    case TILE_FLOOR: {
                        color = (RGBA32){0.1f, 0.5f, 0.9f, 1.0f};
                    } break;

                    case TILE_WALL: {
                        color = (RGBA32){1.0f, 0.2f, 0.1f, 1.0f};
                    } break;

                    INVALID_DEFAULT_CASE;
                }
            } else {
                color = RGBA32_WHITE;
            }

            Rectangle tile_rect = {
                .position = tile_to_world_coords(tile_coords),
                .size = { (f32)TILE_SIZE, (f32)TILE_SIZE }
            };

            rb_push_rect(rb, frame_arena, tile_rect, color, assets->shader2, RENDER_LAYER_TILEMAP);
        }
    }

    for (EntityIndex i = 0; i < world->entities.alive_entity_count; ++i) {
        EntityID id = world->entities.alive_entity_ids[i];
        Entity *entity = es_get_entity(&world->entities, id);

        entity_render(entity, rb, assets, frame_arena);
    }
}

static void game_update(GameState *game_state, const Input *input, f32 dt, LinearArena *frame_arena)
{
    (void)frame_arena;

    if (input_is_key_pressed(input, KEY_ESCAPE)) {
        DEBUG_BREAK;
    }

    world_update(&game_state->world, input, dt);
}

static void render_tree(QuadTree *tree, RenderBatch *rb, LinearArena *arena,
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
	rb_push_outlined_rect(rb, arena, tree->area, color, 4.0f, assets->shader2, 2);

    }
}

static void game_render(GameState *game_state, RenderBatchList *rbs, const AssetList *assets,
    FrameData frame_data, LinearArena *frame_arena)
{
    Matrix4 proj = camera_get_matrix(game_state->world.camera, frame_data.window_width,
	frame_data.window_height);
    RenderBatch *rb = rb_list_push_new(rbs, proj, frame_arena);

    world_render(&game_state->world, rb, assets, frame_data, frame_arena);
    render_tree(&game_state->world.entities.quad_tree, rb, frame_arena, assets, 0);

    rb_push_rect(rb, frame_arena, (Rectangle){{0, 0}, {2, 2}}, RGBA32_RED, assets->shader2, 3);

    rb_sort_entries(rb);
}

void game_update_and_render(GameState *game_state, RenderBatchList *rbs, const AssetList *assets,
    FrameData frame_data, GameMemory *game_memory)
{
    game_update(game_state, frame_data.input, frame_data.dt, &game_memory->temporary_memory);
    game_render(game_state, rbs, assets, frame_data, &game_memory->temporary_memory);
}

#define WORLD_ARENA_SIZE MB(2)

void game_initialize(GameState *game_state, GameMemory *game_memory)
{
    game_state->world.world_arena = la_create(la_allocator(&game_memory->permanent_memory), WORLD_ARENA_SIZE);

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
        Entity *player = es_get_entity(&game_state->world.entities, id);

        ASSERT(!es_has_component(player, PhysicsComponent));
        ASSERT(!es_has_component(player, ColliderComponent));

        player->color = RGBA32_BLUE;
        PhysicsComponent *physics = es_add_component(player, PhysicsComponent);
        physics->position = v2((f32)(16 * (i * 2)), 16.0f * ((f32)i * 2.0f));

        ColliderComponent *collider = es_add_component(player, ColliderComponent);
        collider->size = v2(16.0f, 16.0f);

        ASSERT(es_has_component(player, PhysicsComponent));
        ASSERT(es_has_component(player, ColliderComponent));

	Rectangle rect = get_entity_rect(physics, collider);
	es_set_entity_area(&game_state->world.entities, player, rect, &game_state->world.world_arena);
    }
}
