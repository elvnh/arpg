#include "collision.h"

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
        .movement_fraction_left = movement_fraction_left
    };

    Rectangle md = rect_minkowski_diff(rect_a, rect_b);
    RectanglePoint penetration = rect_bounds_point_closest_to_point(md, V2_ZERO);

    if (rect_contains_point(md, V2_ZERO) && (v2_mag(penetration.point) > INTERSECTION_EPSILON)) {
        // Have already collided
        Vector2 new_pos = v2_sub(result.new_position_a, penetration.point);

	Vector2 push_dir = {0};
	if (penetration.side == RECT_SIDE_TOP) {
	    push_dir.y = -1;
	} else if (penetration.side == RECT_SIDE_BOTTOM) {
	    push_dir.y = 1;
	} else if (penetration.side == RECT_SIDE_LEFT) {
	    push_dir.x = 1;
	} else if (penetration.side == RECT_SIDE_RIGHT) {
	    push_dir.x = -1;
	} else {
	    ASSERT(0);
	}

	result.new_position_a = v2_add(new_pos, v2_mul_s(push_dir, COLLISION_MARGIN));

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
        }
    }

    return result;
}
