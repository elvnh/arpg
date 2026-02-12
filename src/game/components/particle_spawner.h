#ifndef PARTICLE_SPAWNER_H
#define PARTICLE_SPAWNER_H

#include "particle.h"

// TODO: can be generalized?
typedef enum {
    PS_WHEN_DONE_DO_NOTHING = 0,
    PS_WHEN_DONE_REMOVE_COMPONENT, // TODO: should this be default?
    PS_WHEN_DONE_REMOVE_ENTITY,
} ParticleSpawnerWhenDone;

typedef enum {
    PS_SPAWN_DISTRIBUTED,
    PS_SPAWN_ALL_AT_ONCE,
} ParticleSpawnerKind;

typedef enum {
    PS_FLAG_INFINITE    = FLAG(0),
    PS_FLAG_EMITS_LIGHT = FLAG(1),
} ParticleSpawnerFlag;

// TODO: particle velocity
// TODO: reduce size of this
typedef struct {
    ParticleSpawnerKind kind;
    ParticleSpawnerFlag flags;

    RGBA32 particle_color;
    f32 particle_size;
    f32 particle_lifetime;
    f32 particle_speed;

    /* TODO: don't need to store both total_particles_to_spawn
       and particles_left_to_spawn
     */
    s32 total_particles_to_spawn;
    f32 particles_per_second;
} ParticleSpawnerConfig;

typedef struct {
    ParticleSpawnerConfig config;
    ParticleSpawnerWhenDone action_when_done;
    s32 particles_left_to_spawn;
    f32 particle_timer;
} ParticleSpawner;

void update_particle_spawner(struct World *world, struct Entity *entity, ParticleSpawner *ps,
    struct PhysicsComponent *physics, f32 dt);
void initialize_particle_spawner(ParticleSpawner *ps, ParticleSpawnerConfig config);
b32  particle_spawner_is_finished(ParticleSpawner *ps);

#endif //PARTICLE_SPAWNER_H
