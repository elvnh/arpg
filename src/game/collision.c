#include "collision.h"
#include "game/component.h"
#include "game/entity_system.h"
#include "world.h"
#include "base/list.h"
#include "base/sl_list.h"

#define INTERSECTION_TABLE_ARENA_SIZE (128 * SIZEOF(CollisionEvent))
#define INTERSECTION_TABLE_SIZE 512
#define INTERSECTION_EPSILON   0.00001f
#define COLLISION_MARGIN       0.015f

static void rect_collision_reset_velocities(Vector2 *a, Vector2 *b, RectangleSide side)
{
    if ((side == RECT_SIDE_LEFT) || (side == RECT_SIDE_RIGHT)) {
        // TODO: this makes diagonal movement into walls slower, instead transfer velocity from other axis
        a->x = 0.0f;
        b->x = 0.0f;
    } else {
        a->y = 0.0f;
        b->y = 0.0f;
    }
}

CollisionInfo collision_rect_vs_rect(f32 movement_fraction_left, Rectangle rect_a, Rectangle rect_b,
    Vector2 velocity_a, Vector2 velocity_b, f32 dt)
{
    ASSERT(movement_fraction_left >= 0.0f);

    CollisionInfo result = {
        .new_position_a = rect_a.position,
        .new_position_b = rect_b.position,
        .new_velocity_a = velocity_a,
        .new_velocity_b = velocity_b,
        .movement_fraction_left = movement_fraction_left,
        .collision_state = COLL_NOT_COLLIDING,
        .collision_normal = {0}
    };

    Rectangle md = rect_minkowski_diff(rect_a, rect_b);
    RectanglePoint penetration = rect_bounds_point_closest_to_point(md, V2_ZERO);

    if (rect_contains_point(md, V2_ZERO) && (v2_mag(penetration.point) > INTERSECTION_EPSILON)) {
        // Have already collided
        Vector2 new_pos = v2_sub(result.new_position_a, penetration.point);

	Vector2 collision_normal = {0};
	if (penetration.side == RECT_SIDE_TOP) {
	    collision_normal.y = -1;
	} else if (penetration.side == RECT_SIDE_BOTTOM) {
	    collision_normal.y = 1;
	} else if (penetration.side == RECT_SIDE_LEFT) {
	    collision_normal.x = 1;
	} else if (penetration.side == RECT_SIDE_RIGHT) {
	    collision_normal.x = -1;
	} else {
	    ASSERT(0);
	}

	result.new_position_a = v2_add(new_pos, v2_mul_s(collision_normal, COLLISION_MARGIN));
        result.collision_state = COLL_ARE_INTERSECTING;
        result.collision_normal = collision_normal;

        rect_collision_reset_velocities(&result.new_velocity_a, &result.new_velocity_b, penetration.side);
    } else {
	// Will collide this frame
        Vector2 a_movement = v2_mul_s(velocity_a, movement_fraction_left);
        Vector2 relative_movement = v2_mul_s(v2_sub(velocity_b, a_movement), dt);
        Line ray_line = { V2_ZERO, relative_movement };

        RectRayIntersection intersection = rect_shortest_ray_intersection(md, ray_line, INTERSECTION_EPSILON);

        if (intersection.is_intersecting) {
            Vector2 a_dist_to_move = v2_mul_s(velocity_a, intersection.time_of_impact * dt);
            Vector2 b_dist_to_move =  v2_mul_s(velocity_b, intersection.time_of_impact * dt);

            result.new_position_a = v2_add(rect_a.position, a_dist_to_move);
            result.new_position_b = v2_add(rect_b.position, b_dist_to_move);

            // Move slightly in opposite direction to prevent getting stuck
            result.new_position_a = v2_add(
		result.new_position_a,
		v2_mul_s(v2_norm(velocity_a), -COLLISION_MARGIN)
	    );
            result.new_position_b = v2_add(
		result.new_position_b,
		v2_mul_s(v2_norm(velocity_b), -COLLISION_MARGIN)
	    );

            rect_collision_reset_velocities(&result.new_velocity_a, &result.new_velocity_b,
		intersection.side_of_collision);

	    f32 remaining = result.movement_fraction_left - intersection.time_of_impact;
            result.movement_fraction_left = MAX(0.0f, remaining);
            result.collision_state = COLL_WILL_COLLIDE_THIS_FRAME;

            switch (intersection.side_of_collision) {
                case RECT_SIDE_TOP: {
                    result.collision_normal = v2(0, -1);
                } break;

                case RECT_SIDE_BOTTOM: {
                    result.collision_normal = v2(0, 1);
                } break;

                case RECT_SIDE_LEFT: {
                    result.collision_normal = v2(1, 0);
                } break;

                case RECT_SIDE_RIGHT: {
                    result.collision_normal = v2(1, 0);
                } break;
            }
        }
    }

    return result;
}

static inline ssize collision_cooldown_hashed_index(const CollisionCooldownTable *table, EntityID self,
    EntityID other, s32 effect_index)
{
    // TODO: better hash
    u64 hash = (entity_id_hash(self) ^ entity_id_hash(other)) + (u64)(effect_index * 17);
    ssize result = mod_index(hash, ARRAY_COUNT(table->table));

    return result;
}

CollisionEffectCooldown *collision_cooldown_find(CollisionCooldownTable *table, EntityID self, EntityID other,
    s32 effect_index)
{
    ssize index = collision_cooldown_hashed_index(table, self, other, effect_index);

    CollisionCooldownList *list = &table->table[index];

    CollisionEffectCooldown *current;
    for (current = list_head(list); current; current = list_next(current)) {
        if (entity_id_equal(current->owning_entity, self)
            && entity_id_equal(current->collided_entity, other)
            && (effect_index == current->collision_effect_index)) {
            break;
        }
    }

    return current;
}

void collision_cooldown_add(CollisionCooldownTable *table, EntityID self, EntityID other,
    s32 effect_index, LinearArena *arena)
{
    ASSERT(!entity_id_equal(self, other));

    if (!collision_cooldown_find(table, self, other, effect_index)) {
        CollisionEffectCooldown *cooldown = list_head(&table->free_node_list);

        if (!cooldown) {
            cooldown = la_allocate_item(arena, CollisionEffectCooldown);
        } else {
            list_pop_head(&table->free_node_list);
        }

        *cooldown = (CollisionEffectCooldown){0};
        cooldown->owning_entity = self;
        cooldown->collided_entity = other;
        cooldown->collision_effect_index = effect_index;

        ssize index = collision_cooldown_hashed_index(table, self, other, effect_index);
        list_push_back(&table->table[index], cooldown);
    }
}

void remove_expired_collision_cooldowns(struct World *world)
{
    for (ssize i = 0; i < ARRAY_COUNT(world->collision_effect_cooldowns.table); ++i) {
        CollisionCooldownList *exception_list = &world->collision_effect_cooldowns.table[i];

        for (CollisionEffectCooldown *exc = list_head(exception_list); exc;) {
            CollisionEffectCooldown *next = exc->next;

            Entity *owning = es_get_entity(&world->entities, exc->owning_entity);
            Entity *collided = es_get_entity(&world->entities, exc->collided_entity);
            ASSERT(owning);
            ASSERT(collided);

            EntityID owning_id = exc->owning_entity;
            EntityID collided_id = exc->collided_entity;

            ColliderComponent *collider = es_get_component(owning, ColliderComponent);
            ASSERT(collider);
            ASSERT(exc->collision_effect_index < ARRAY_COUNT(collider->on_collide_effects.effects));

            const OnCollisionEffect *effect = &collider->on_collide_effects.effects[exc->collision_effect_index];
            ASSERT(effect->retrigger_behaviour != COLL_RETRIGGER_WHENEVER);

            b32 should_remove = (owning->is_inactive || collided->is_inactive)
                || ((effect->retrigger_behaviour == COLL_RETRIGGER_AFTER_NON_CONTACT)
                    && !entities_intersected_this_frame(world, owning_id, collided_id));

            if (should_remove) {
                list_remove(exception_list, exc);
                list_push_back(&world->collision_effect_cooldowns.free_node_list, exc);
            }

            exc = next;
        }
    }
}

CollisionEventTable collision_event_table_create(LinearArena *parent_arena)
{
    CollisionEventTable result = {0};
    result.table = la_allocate_array(parent_arena, CollisionEventList, INTERSECTION_TABLE_SIZE);
    result.table_size = INTERSECTION_TABLE_SIZE;
    result.arena = la_create(la_allocator(parent_arena), INTERSECTION_TABLE_ARENA_SIZE);

    return result;
}

CollisionEvent *collision_event_table_find(CollisionEventTable *table, EntityID a, EntityID b)
{
    EntityPair searched_pair = entity_pair_create(a, b);
    u64 hash = entity_pair_hash(searched_pair);
    ssize index = mod_index(hash, table->table_size);

    CollisionEventList *list = &table->table[index];

    CollisionEvent *node = 0;
    for (node = list_head(list); node; node = list_next(node)) {
        if (entity_pair_equal(searched_pair, node->entity_pair)) {
            break;
        }
    }

    return node;
}

void collision_event_table_insert(CollisionEventTable *table, EntityID a, EntityID b)
{
    if (!collision_event_table_find(table, a, b)) {
        EntityPair entity_pair = entity_pair_create(a, b);
        u64 hash = entity_pair_hash(entity_pair);
        ssize index = mod_index(hash, table->table_size);

        CollisionEvent *new_node = la_allocate_item(&table->arena, CollisionEvent);
        new_node->entity_pair = entity_pair;

        CollisionEventList *list = &table->table[index];
        sl_list_push_back(list, new_node);
    } else {
        ASSERT(0);
    }
}
