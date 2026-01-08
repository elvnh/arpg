#ifndef PARTICLE_H
#define PARTICLE_H

#include "base/ring_buffer.h"
#include "base/typedefs.h"
#include "base/vector.h"
#include "base/rgba.h"
#include "asset.h"

struct Entity;

typedef struct Particle {
    f32 timer;
    f32 lifetime;
    Vector2 position;
    Vector2 velocity;
} Particle;

DEFINE_STATIC_RING_BUFFER(Particle, ParticleBuffer, 1024);

// TODO: can be generalized
typedef enum {
    PS_WHEN_DONE_DO_NOTHING = 0,
    PS_WHEN_DONE_REMOVE_COMPONENT, // TODO: should this be default?
    PS_WHEN_DONE_REMOVE_ENTITY,
} ParticleSpawnerWhenDone;

typedef enum {
    PS_SPAWN_DISTRIBUTED,
    PS_SPAWN_ALL_AT_ONCE,
} ParticleSpawnerKind;

// TODO: particle velocity
typedef struct {
    ParticleSpawnerKind kind;
    TextureHandle particle_texture;
    RGBA32 particle_color;
    f32 particle_size;
    f32 particle_lifetime;
    f32 particle_speed;
    s32 total_particles_to_spawn;
    b32 infinite; // TODO: use particle flags instead?

    f32 particles_per_second;
} ParticleSpawnerConfig;

// TODO: different kinds of spawners: infinite particles etc,
// allow configuring angle of particles
typedef struct {
    ParticleBuffer particle_buffer;
    f32 particle_timer;
    s32 particles_left_to_spawn;
    ParticleSpawnerConfig config;
    ParticleSpawnerWhenDone action_when_done;
} ParticleSpawner;

void particle_spawner_initialize(ParticleSpawner *ps, ParticleSpawnerConfig config);
b32 particle_spawner_is_finished(ParticleSpawner *ps);

// TODO: don't update particle spawners when out of sight of player since they don't affect gameplay
void component_update_particle_spawner(struct Entity *entity, ParticleSpawner *ps, f32 dt);

#endif //PARTICLE_H
