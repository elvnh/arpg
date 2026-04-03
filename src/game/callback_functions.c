#include "callback_functions.h"

#include "base/linear_arena.h"
#include "world/world.h"

void callback_spawn_particles(CallbackUserData *user_data, EventData event_data,
    LinearArena *frame_arena)
{
    (void)frame_arena;

    ParticleSpawnerSetup *setup = &user_data->particle_spawner_data;

    Entity *self = es_get_entity(&event_data.world->entity_system, event_data.receiver_id);

    PhysicsComponent *self_physics = es_try_get_component(self, PhysicsComponent);
    ASSERT(self_physics
           && "Physics was removed inbetween death and callback getting called, "
              "probably shouldn't happen?");

    if (self_physics) {
        Rectangle self_bounds = world_get_entity_bounding_box(self, self_physics);

        Chunk *chunk =
            get_chunk_at_position(&event_data.world->map_chunks, self_physics->position);
        ASSERT(chunk);

        spawn_particles_in_chunk(chunk, self_bounds, setup->config,
            setup->total_particle_count);
    }
}

static Entity *try_get_chain_target(World *world, Entity *self, ChainComponent *self_chain,
    Vector2 position, Rectangle search_area, Entity *chained_off_entity,
    LinearArena *frame_arena)
{
    EntityIDList nearby_entities =
        qt_get_entities_in_area(&world->quad_tree, search_area, frame_arena);

    // TODO: break out getting closest entity into function
    Entity *closest_entity = 0;
    f32 closest_entity_dist = INFINITY;

    GetHostileFactionResult hostile_faction_result = get_hostile_faction(self->faction);
    EntityFaction hostile_faction = hostile_faction_result.hostile_faction;
    ASSERT(hostile_faction_result.ok);
    // get current chain count
    // get chain area

    for (EntityIDNode *curr = list_head(&nearby_entities); curr; curr = list_next(curr)) {
        Entity *curr_entity = es_get_entity(&world->entity_system, curr->id);
        PhysicsComponent *curr_entity_physics =
            es_get_component(curr_entity, PhysicsComponent);

        // TODO: clean this up
        if ((curr_entity->faction == hostile_faction) && (curr_entity != chained_off_entity)) {
            if (!has_chained_off_entity(&world->entity_system, self_chain, curr_entity->id)) {
                f32 dist = v2_dist(curr_entity_physics->position, position);

                if (dist < closest_entity_dist) {
                    closest_entity = curr_entity;
                    closest_entity_dist = dist;
                }
            }
        }
    }

    return closest_entity;
}

void chain_collision_callback(CallbackUserData *user_data, EventData event_data,
    LinearArena *frame_arena)
{
    SpellCallbackData *cb_data = &user_data->spell_data;
    ASSERT(cb_data->as.chain.chains_remaining >= 0);

    EntitySystem *es = &event_data.world->entity_system;
    Entity *self = es_get_entity(es, event_data.receiver_id);

    Entity *collide_target = es_get_entity(es, event_data.as.hostile_collision.collided_with);
    PhysicsComponent *self_physics = es_get_component(self, PhysicsComponent);

    // NOTE: the spell must be the root of the chain
    ChainComponent *chain = es_get_component(self, ChainComponent);

    // NOTE: We check if we have already chained off this entity BEFORE we kill
    // the entity due to no chains remaining, since we don't want to die on this one
    // if we've already chained off it, even if we happen to be out of chains
    if (has_chained_off_entity(es, chain, collide_target->id)) {
        return;
    }

    if (cb_data->as.chain.chains_remaining == 0) {
        world_kill_entity(event_data.world, self, frame_arena);
        return;
    }

    --cb_data->as.chain.chains_remaining;

    f32 search_area_size = cb_data->as.chain.search_area_size;
    Vector2 search_area_dims = {search_area_size, search_area_size};
    Rectangle search_area = {
        v2_sub(self_physics->position, v2_div_s(search_area_dims, 2.0f)),
        search_area_dims,
    };

    Entity *closest_entity = try_get_chain_target(event_data.world, self, chain,
        self_physics->position, search_area, collide_target, frame_arena);

    if (closest_entity) {
        PhysicsComponent *closest_entity_physics =
            es_get_component(closest_entity, PhysicsComponent);
        Vector2 target_pos = closest_entity_physics->position;
        Vector2 target_dir = v2_norm(v2_sub(target_pos, self_physics->position));
        f32 current_speed = v2_mag(self_physics->velocity);

        self_physics->velocity = v2_mul_s(target_dir, current_speed);

        EntityWithID chain_link_entity =
            world_spawn_entity(event_data.world, self_physics->position, self->faction);
        ChainComponent *chain_link =
            es_add_component(chain_link_entity.entity, ChainComponent);
        chain_link->chained_off_entity_id = collide_target->id;

        push_link_to_front_of_chain(es, chain, chain_link);
    }
}
