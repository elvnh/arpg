#ifndef PARTICLE_H
#define PARTICLE_H

#include "base/ring_buffer.h"
#include "base/typedefs.h"
#include "base/vector.h"
#include "base/rgba.h"
#include "platform/asset.h"
#include "light.h"
#include "renderer/frontend/render_target.h"

struct Entity;
struct LinearArena;
struct RenderBatch;
struct World;
struct PhysicsComponent;

// TODO: reduce the size of this
typedef struct Particle {
    Vector2 position;
    Vector2 velocity;
    f32 timer;
    f32 lifetime;
    RGBA32 color;
    f32 size;
} Particle;

DEFINE_STATIC_RING_BUFFER(Particle, ParticleBuffer, 1024);

typedef struct {
    ParticleBuffer normal_particles;
    ParticleBuffer light_emitting_particles;
} ParticleBuffers;

void update_particle_buffers(ParticleBuffers *buffer, f32 dt);
void render_particle_buffers(ParticleBuffers *buffer, RenderBatches rbs, struct LinearArena *arena);

#endif //PARTICLE_H
