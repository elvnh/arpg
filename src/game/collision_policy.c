#include "collision_policy.h"
#include "world.h"

static void execute_collision_policy(World *world, Entity *entity, CollisionPolicy policy,
    CollisionInfo collision, EntityPairIndex collision_pair_index, b32 should_block)
{
    // NOTE: This function handles both entity vs tilemap and entity vs entity collisions.
    // If the collision is vs a tile, pass ENTITY_PAIR_INDEX_FIRST as collision_pair_index.

    switch (policy) {
	case COLLISION_POLICY_STOP: {
	    if (should_block) {
		if (collision_pair_index == ENTITY_PAIR_INDEX_FIRST) {
		    entity->position = collision.new_position_a;
		    entity->velocity = collision.new_velocity_a;

		} else {
		    entity->position = collision.new_position_b;
		    entity->velocity = collision.new_velocity_b;
		}
	    }
	} break;

	case COLLISION_POLICY_BOUNCE: {
	    if (should_block) {
		if (collision_pair_index == ENTITY_PAIR_INDEX_FIRST) {
		    entity->position = collision.new_position_a;
		    entity->velocity = v2_reflect(entity->velocity, collision.collision_normal);
		} else {
		    entity->position = collision.new_position_b;
		    entity->velocity = v2_reflect(entity->velocity, v2_neg(collision.collision_normal));
		}
	    }
	} break;

	case COLLISION_POLICY_DIE: {
	    world_kill_entity(world, entity);
	} break;


	case COLLISION_POLICY_PASS_THROUGH: {

	} break;

	INVALID_DEFAULT_CASE;
    }
}

void execute_entity_vs_tilemap_collision_policy(World *world, Entity *entity, CollisionInfo collision)
{
    ColliderComponent *collider = es_get_component(entity, ColliderComponent);
    ASSERT(collider);

    CollisionPolicy policy = collider->tilemap_collision_policy;

    b32 should_block = policy != COLLISION_POLICY_PASS_THROUGH;

    execute_collision_policy(world, entity, policy, collision, ENTITY_PAIR_INDEX_FIRST, should_block);
}

void execute_entity_vs_entity_collision_policy(World *world, Entity *entity, Entity *other,
    CollisionInfo collision, EntityPairIndex collision_index)
{
    ASSERT(entity);
    ASSERT(other);

    ColliderComponent *collider = es_get_component(entity, ColliderComponent);
    ColliderComponent *other_collider = es_get_component(other, ColliderComponent);

    CollisionPolicy our_policy_for_them =
	collider->per_faction_collision_policies[other->faction];
    CollisionPolicy their_policy_for_us =
	other_collider->per_faction_collision_policies[entity->faction];

    b32 should_block = (our_policy_for_them != COLLISION_POLICY_PASS_THROUGH)
	&& (their_policy_for_us != COLLISION_POLICY_PASS_THROUGH);

    // NOTE: we only execute OUR behaviour for THEM, this function is expected to be called
    // twice for each collision pair
    execute_collision_policy(world, entity, our_policy_for_them, collision,
	collision_index, should_block);
}
