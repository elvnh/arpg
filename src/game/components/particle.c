#include "particle.h"
#include "entity_system.h"
#include "base/random.h"
#include "world.h"

void component_update_particle_spawner(Entity *entity, ParticleSpawner *ps, Vector2 position, f32 dt)
{
    s32 particles_to_spawn_this_frame = 0;

    switch (ps->config.kind) {
        case PS_SPAWN_DISTRIBUTED: {
            ps->particle_timer += ps->config.particles_per_second * dt;

	    if (ps->config.infinite) {
		particles_to_spawn_this_frame = (s32)ps->particle_timer;
	    } else {
		particles_to_spawn_this_frame = MIN((s32)ps->particle_timer, ps->particles_left_to_spawn);
		ps->particles_left_to_spawn -= particles_to_spawn_this_frame;
	    }

            ps->particle_timer -= (f32)particles_to_spawn_this_frame;
        } break;

        case PS_SPAWN_ALL_AT_ONCE: {
	    ASSERT(!ps->config.infinite);

            particles_to_spawn_this_frame = ps->particles_left_to_spawn;
            ps->particles_left_to_spawn = 0;
        } break;
    }

    Rectangle entity_bounds = world_get_entity_bounding_box(entity);

    for (s32 i = 0; i < particles_to_spawn_this_frame; ++i) {
        Vector2 dir = rng_direction(PI * 2);

        f32 min_speed = ps->config.particle_speed * 0.5f;
        f32 max_speed = ps->config.particle_speed * 1.5f;
        f32 speed = rng_f32(min_speed, max_speed);

	// TODO: this doesn't look very good
	Vector2 particle_position = rng_position_in_rect(entity_bounds);

        // TODO: color variance
        Particle new_particle = {
            .timer = 0.0f,
            .lifetime = ps->config.particle_lifetime,
            .position = particle_position,
            .velocity = v2_mul_s(dir, speed)
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

// Particle spawners aren't considered done until the have spawned all remaining particles
// and all particles in flight have died
b32 particle_spawner_is_finished(ParticleSpawner *ps)
{
    b32 result = !ps->config.infinite && (ring_length(&ps->particle_buffer) == 0)
        && (ps->particles_left_to_spawn <= 0);

    return result;
}

void particle_spawner_initialize(ParticleSpawner *ps, ParticleSpawnerConfig config)
{
    ring_initialize_static(&ps->particle_buffer);
    ps->config = config;
    ps->particles_left_to_spawn = config.total_particles_to_spawn;
}
