#include "particle_spawner.h"
#include "world/world.h"

b32 particle_spawner_is_finished(ParticleSpawner *ps)
{
    // NOTE: particle spawners no longer need to stay alive while particles are in flight
    // since the particles are stored in chunk particle buffers
    b32 result = !has_flag(ps->config.flags, PS_FLAG_INFINITE) && (ps->particles_left_to_spawn <= 0);

    return result;
}

void initialize_particle_spawner(ParticleSpawner *ps, ParticleSpawnerConfig config, s32 particle_count)
{
    ps->config = config;
    ps->particles_left_to_spawn = particle_count;
}

static s32 calculate_particles_to_spawn_this_frame(ParticleSpawner *ps, f32 dt)
{
    s32 result = 0;

    if (has_flag(ps->config.flags, PS_FLAG_SPAWN_ALL_AT_ONCE)) {
        ASSERT(!has_flag(ps->config.flags, PS_FLAG_INFINITE));

        result = ps->particles_left_to_spawn;
        ps->particles_left_to_spawn = 0;
    } else {
        ps->particle_timer += ps->config.particles_per_second * dt;

        if (has_flag(ps->config.flags, PS_FLAG_INFINITE)) {
            result = (s32)ps->particle_timer;
        } else {
            result = MIN((s32)ps->particle_timer, ps->particles_left_to_spawn);
            ps->particles_left_to_spawn -= result;
        }

        ps->particle_timer -= (f32)result;
    }

    return result;
}

void update_particle_spawner(World *world, Entity *entity, ParticleSpawner *ps, PhysicsComponent *physics, f32 dt)
{
    ASSERT(ps->config.particle_size > 0.0f);
    ASSERT(ps->config.particle_speed > 0.0f);
    ASSERT(ps->config.particles_per_second > 0);
    ASSERT(ps->config.particle_color.a > 0);

    ASSERT(physics);

    s32 particles_to_spawn = calculate_particles_to_spawn_this_frame(ps, dt);

    Chunk *entity_chunk = get_chunk_at_position(&world->map_chunks, physics->position);
    ParticleBuffer *particle_buffer = has_flag(ps->config.flags, PS_FLAG_EMITS_LIGHT)
        ? &entity_chunk->particle_buffers.light_emitting_particles
        : &entity_chunk->particle_buffers.normal_particles;

    Rectangle entity_bounds = world_get_entity_bounding_box(entity, physics);
    ASSERT(entity_chunk);

    for (s32 i = 0; i < particles_to_spawn; ++i) {
        Vector2 dir = rng_direction(PI * 2);

        f32 min_speed = ps->config.particle_speed * 0.25f;
        f32 max_speed = ps->config.particle_speed * 2.0f;
        f32 speed = rng_f32(min_speed, max_speed);

        f32 lifetime_variance = 0.6f;
        f32 min_lifetime = ps->config.particle_lifetime * (1.0f - lifetime_variance);
        f32 max_lifetime = ps->config.particle_lifetime * (1.0f + lifetime_variance);
        f32 lifetime = rng_f32(min_lifetime, max_lifetime);

	// TODO: this doesn't look very good
	Vector2 particle_position = rng_position_in_rect(entity_bounds);

        // TODO: color variance
        Particle new_particle = {
            .timer = 0.0f,
            .lifetime = lifetime,
            .position = particle_position,
            .velocity = v2_mul_s(dir, speed),
            .color = ps->config.particle_color,
            .size = ps->config.particle_size,
        };

        ring_push_overwrite(particle_buffer, &new_particle);
    }

    if ((ps->action_when_done == PS_WHEN_DONE_REMOVE_COMPONENT) && particle_spawner_is_finished(ps)) {
        es_remove_component(entity, ParticleSpawner);
    }
}
