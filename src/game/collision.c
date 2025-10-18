#include "collision.h"
#include "base/list.h"

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

static inline ssize collision_rule_hashed_index(EntityPair pair, const CollisionExceptionTable *table)
{
    u64 hash = entity_pair_hash(pair);
    ssize result = mod_index(hash, ARRAY_COUNT(table->table));

    return result;
}

CollisionException *collision_exception_find(CollisionExceptionTable *table, EntityID a, EntityID b)
{
    EntityPair searched_pair = entity_pair_create(a, b);
    ssize index = collision_rule_hashed_index(searched_pair, table);

    CollisionExceptionList *list = &table->table[index];
    CollisionException *current_rule;
    for (current_rule = list_head(list); current_rule; current_rule = list_next(current_rule)) {
        if (entity_id_equal(current_rule->entity_pair.entity_a, searched_pair.entity_a)
	 && entity_id_equal(current_rule->entity_pair.entity_b, searched_pair.entity_b)) {
            break;
        }
    }

    return current_rule;
}

void collision_exception_add(CollisionExceptionTable *table, EntityID a, EntityID b, b32 should_collide,
    LinearArena *arena)
{
    // TODO: remove collision rules when entity dies
    ASSERT(!entity_id_equal(a, b));

    CollisionException *rule = list_head(&table->free_node_list);
    if (!rule) {
	rule = la_allocate_item(arena, CollisionException);
    } else {
	list_pop_head(&table->free_node_list);
    }

    *rule = (CollisionException){0};
    rule->entity_pair = entity_pair_create(a, b);
    rule->should_collide = should_collide;

    ASSERT(!collision_exception_find(table, rule->entity_pair.entity_a, rule->entity_pair.entity_b));

    ssize index = collision_rule_hashed_index(rule->entity_pair, table);
    list_push_back(&table->table[index], rule);
}

void remove_collision_exceptions_with_entity(CollisionExceptionTable *table, EntityID a)
{
    for (s32 i = 0; i < ARRAY_COUNT(table->table); ++i) {
        CollisionExceptionList *list = &table->table[i];

        for (CollisionException *rule = list_head(list); rule;) {
            CollisionException *next = list_next(rule);

	    if (entity_id_equal(rule->entity_pair.entity_a, a)
	     || entity_id_equal(rule->entity_pair.entity_b, a)) {

                list_remove(list, rule);
                list_push_back(&table->free_node_list, rule);

	    }

            rule = next;
        }
    }
}
