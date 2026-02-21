#include "callback_functions.h"
#include "base/linear_arena.h"
#include "world/world.h"

void callback_spawn_particles(CallbackUserData user_data, EventData event_data,
    LinearArena *frame_arena)
{
    (void)frame_arena;

    ParticleSpawnerSetup *setup = &user_data.particle_spawner_data;

    Entity *self = es_get_entity(&event_data.world->entity_system, event_data.receiver_id);
    PhysicsComponent *self_physics = es_get_component(self, PhysicsComponent);
    ASSERT(self_physics && "Physics was removed inbetween death and callback getting called, "
          "probably shouldn't happen?");

    Rectangle self_bounds = world_get_entity_bounding_box(self, self_physics);

    Chunk *chunk = get_chunk_at_position(&event_data.world->map_chunks, self_physics->position);
    ASSERT(chunk);

    spawn_particles_in_chunk(chunk, self_bounds, setup->config, setup->total_particle_count);
}
