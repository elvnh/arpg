#include "particle.h"
#include "components/component.h"
#include "entity/entity_arena.h"
#include "entity/entity_system.h"
#include "base/random.h"
#include "world/world.h"
#include "renderer/frontend/render_batch.h"
#include "asset_table.h"

ParticleBuffer *get_particle_spawner_buffer(Entity *entity, ParticleSpawner *ps)
{
    ParticleBuffer *result = entity_arena_get(&entity->entity_arena,
        ps->particle_buffer_arena_index);

    return result;
}

void update_particle_spawner(Entity *entity, ParticleSpawner *ps, PhysicsComponent *physics, f32 dt)
{
    ASSERT(ps->config.particle_size > 0.0f);
    ASSERT(ps->config.particle_speed > 0.0f);
    ASSERT(ps->config.particles_per_second > 0);

    ASSERT(physics);
    ASSERT(es_has_component(entity, PhysicsComponent));

    ParticleBuffer *buffer = get_particle_spawner_buffer(entity, ps);

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

    Rectangle entity_bounds = world_get_entity_bounding_box(entity, physics);

    for (s32 i = 0; i < particles_to_spawn_this_frame; ++i) {
        Vector2 dir = rng_direction(PI * 2);

        f32 min_speed = ps->config.particle_speed * 0.25f;
        f32 max_speed = ps->config.particle_speed * 2.0f;
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

        ring_push_overwrite(buffer, &new_particle);
    }

    for (ssize i = 0; i < ring_length(buffer); ++i) {
        Particle *p = ring_at(buffer, i);

        if (p->timer > p->lifetime) {
            ring_swap_remove(buffer, i);
        }

        p->timer += dt;
        p->position = v2_add(p->position, v2_mul_s(p->velocity, dt));
        p->velocity = v2_add(p->velocity, v2(0, -1.0f));
        p->velocity = v2_add(p->velocity, v2_mul_s(p->velocity, -0.009f));
    }

    if ((ps->action_when_done == PS_WHEN_DONE_REMOVE_COMPONENT)
        && particle_spawner_is_finished(entity, ps)) {
        es_remove_component(entity, ParticleSpawner);
    }
}

void render_particle_spawner(World *world, Entity *entity, ParticleSpawner *ps, RenderBatches rbs, LinearArena *arena)
{
    ParticleBuffer *buffer = get_particle_spawner_buffer(entity, ps);

    if (ps->config.particle_texture.id == NULL_TEXTURE.id) {
        ASSERT(ps->config.particle_color.a != 0.0f);

        if (ps->config.emits_light) {
            for (ssize i = 0; i < ring_length(buffer); ++i) {
                Particle *p = ring_at(buffer, i);
                // TODO: don't have the particles fade out linearly
                f32 intensity = 1.0f - p->timer / p->lifetime;

                render_light_source(world, rbs.lighting_rb, p->position, ps->config.light_source,
                    intensity, arena);
            }
        } else {
            draw_particles(rbs.world_rb, arena, buffer, ps->config.particle_color,
                ps->config.particle_size, shader_handle(SHAPE_SHADER), RENDER_LAYER_PARTICLES);
        }


    } else {
        // If color isn't set, assume it to be white
        RGBA32 particle_color = ps->config.particle_color;
        if (particle_color.a == 0.0f) {
            particle_color = RGBA32_WHITE;
        }

        draw_textured_particles(rbs.world_rb, arena, buffer,
            texture_handle(DEFAULT_TEXTURE), particle_color, ps->config.particle_size,
            shader_handle(TEXTURE_SHADER), RENDER_LAYER_PARTICLES);
    }
}

void particle_spawner_initialize(Entity *entity, ParticleSpawner *ps, ParticleSpawnerConfig config)
{
    // TODO: instead have a function for creating a particle spawner and only pass entity,
    // not pointer to already existing particle spawner
    ASSERT((void *)ps >= (void *)entity && (void *)ps < (void *)(entity + 1)
        && "Particle spawner doesn't belong to entity");

    ASSERT(config.infinite || config.total_particles_to_spawn > 0);

    EntityArenaAllocation particle_buffer_allocation =
        entity_arena_allocate_item(&entity->entity_arena, ParticleBuffer);

    ParticleBuffer *buffer = particle_buffer_allocation.ptr;

    ring_initialize_static(buffer);
    ps->particle_buffer_arena_index = particle_buffer_allocation.index;
    ps->config = config;
    ps->particles_left_to_spawn = config.total_particles_to_spawn;
}

// Particle spawners aren't considered done until the have spawned all remaining particles
// and all particles in flight have died
b32 particle_spawner_is_finished(Entity *entity, ParticleSpawner *ps)
{
    ParticleBuffer *buffer = get_particle_spawner_buffer(entity, ps);

    b32 result = !ps->config.infinite && (ring_length(buffer) == 0)
        && (ps->particles_left_to_spawn <= 0);

    return result;
}

ParticleSpawner *copy_particle_spawner(struct Entity *dst_entity,
    struct Entity *src_entity, ParticleSpawner *src_ps)
{
    ASSERT(es_has_component(src_entity, ParticleSpawner));

    ParticleSpawner *dst_ps = 0;

    if (es_has_component(dst_entity, ParticleSpawner)) {
        dst_ps = es_get_component(dst_entity, ParticleSpawner);
    } else {
        dst_ps = es_add_component(dst_entity, ParticleSpawner);
        particle_spawner_initialize(dst_entity, dst_ps, src_ps->config);
    }

    EntityArenaIndex buffer_index = dst_ps->particle_buffer_arena_index;
    *dst_ps = *src_ps;
    dst_ps->particle_buffer_arena_index = buffer_index;

    ParticleBuffer *dst_buffer = get_particle_spawner_buffer(dst_entity, dst_ps);
    ParticleBuffer *src_buffer = get_particle_spawner_buffer(src_entity, src_ps);
    *dst_buffer = *src_buffer;

    return dst_ps;
}
