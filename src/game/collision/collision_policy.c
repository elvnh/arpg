#include "collision_policy.h"
#include "base/vector.h"
#include "collision/collider.h"
#include "components/component.h"
#include "world/world.h"

static void execute_collision_policy(World *world, Entity *entity, PhysicsComponent *physics, CollisionPolicy policy,
    CollisionInfo collision, EntityPairIndex collision_pair_index, b32 should_block)
{
    // NOTE: This function handles both entity vs tilemap and entity vs entity collisions.
    // If the collision is vs a tile, pass ENTITY_PAIR_INDEX_FIRST as collision_pair_index.
    ASSERT(entity);
    ASSERT(entity || (collision_pair_index == ENTITY_PAIR_INDEX_FIRST));

    switch (policy) {
	case COLLISION_POLICY_STOP: {
	    if (should_block) {
		if (collision_pair_index == ENTITY_PAIR_INDEX_FIRST) {
		    physics->position = collision.new_position_a;
		    physics->velocity = collision.new_velocity_a;

		} else {
		    physics->position = collision.new_position_b;
		    physics->velocity = collision.new_velocity_b;
		}
	    }
	} break;

	case COLLISION_POLICY_BOUNCE: {
	    if (should_block) {
		if (collision_pair_index == ENTITY_PAIR_INDEX_FIRST) {
		    physics->position = collision.new_position_a;
		    physics->velocity = v2_reflect(physics->velocity, collision.collision_normal);
		} else {
		    physics->position = collision.new_position_b;
		    physics->velocity = v2_reflect(physics->velocity, v2_neg(collision.collision_normal));
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

void execute_entity_vs_tilemap_collision_policy(World *world, Entity *entity, PhysicsComponent *physics,
    ColliderComponent *collider, CollisionInfo collision)
{
    CollisionPolicy policy = collider->tilemap_collision_policy;

    b32 should_block = policy != COLLISION_POLICY_PASS_THROUGH;

    execute_collision_policy(world, entity, physics, policy, collision, ENTITY_PAIR_INDEX_FIRST, should_block);
}

void execute_entity_vs_entity_collision_policy(World *world,
    Entity *entity, PhysicsComponent *entity_physics, ColliderComponent *entity_collider,
    Entity *other,  ColliderComponent *other_collider,
    CollisionInfo collision, EntityPairIndex collision_index)
{
    ASSERT(entity);
    ASSERT(other);
    ASSERT(entity->faction >= 0);
    ASSERT(entity->faction < FACTION_COUNT);
    ASSERT(other->faction >= 0);
    ASSERT(other->faction < FACTION_COUNT);

    CollisionPolicy our_policy_for_them =
	entity_collider->per_faction_collision_policies[other->faction];
    CollisionPolicy their_policy_for_us =
	other_collider->per_faction_collision_policies[entity->faction];

    b32 should_block =
	(our_policy_for_them != COLLISION_POLICY_PASS_THROUGH)
	&& (their_policy_for_us != COLLISION_POLICY_PASS_THROUGH)
	&& (their_policy_for_us != COLLISION_POLICY_DIE);

    // NOTE: we only execute OUR behaviour for THEM, this function is expected to be called
    // twice for each collision pair
    execute_collision_policy(world, entity, entity_physics, our_policy_for_them, collision,
	collision_index, should_block);
}
