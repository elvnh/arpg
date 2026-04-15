#include "particle.h"

#include "asset_table.h"
#include "base/random.h"
#include "components/component.h"
#include "entity/entity_system.h"
#include "renderer/frontend/render_batch.h"
#include "world/chunk.h"
#include "world/world.h"

static void update_particle_buffer(ParticleBuffer *buffer, f32 dt)
{
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
}

void update_particle_buffers(ParticleBuffers *buffers, f32 dt)
{
    update_particle_buffer(&buffers->normal_particles, dt);
    update_particle_buffer(&buffers->light_emitting_particles, dt);
}

void render_particle_buffers(ParticleBuffers *buffers, RenderBatches rbs,
    struct LinearArena *arena)
{
    if (ring_length(&buffers->normal_particles) > 0) {
        draw_particles(rbs.world_rb, arena, &buffers->normal_particles,
            shader_handle(SHAPE_SHADER), RENDER_LAYER_PARTICLES);
    }

    if (ring_length(&buffers->light_emitting_particles) > 0) {
        draw_particles(rbs.lighting_rb, arena, &buffers->light_emitting_particles,
            shader_handle(SHAPE_SHADER), RENDER_LAYER_PARTICLES);
    }
}

void clear_particle_buffers(ParticleBuffers *buffers)
{
    ring_clear(&buffers->normal_particles);
    ring_clear(&buffers->light_emitting_particles);
}
