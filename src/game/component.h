#ifndef COMPONENT_H
#define COMPONENT_H

#include "damage.h"
#include "particle.h"
#include "asset.h"
#include "base/rgba.h"

#define COMPONENT_LIST                          \
    COMPONENT(ColliderComponent)                \
    COMPONENT(HealthComponent)                  \
    COMPONENT(ParticleSpawner)                  \
    COMPONENT(SpriteComponent)                  \
    COMPONENT(LifetimeComponent)                \
    COMPONENT(OnDeathComponent)                 \
    COMPONENT(StatsComponent)                   \

#define ES_IMPL_COMP_ENUM_NAME(type) COMP_##type
#define ES_IMPL_COMP_FIELD_NAME(type) component_##type
#define ES_IMPL_COMP_ENUM_BIT_VALUE(e) ((u64)1 << (e))

#define component_flag(type) ES_IMPL_COMP_ENUM_BIT_VALUE(ES_IMPL_COMP_ENUM_NAME(type))

typedef u64 ComponentBitset;

typedef enum {
    #define COMPONENT(type) ES_IMPL_COMP_ENUM_NAME(type),
        COMPONENT_LIST
        COMPONENT_COUNT,
    #undef COMPONENT
} ComponentType;

typedef enum {
    ON_COLLIDE_STOP,
    ON_COLLIDE_PASS_THROUGH,
    ON_COLLIDE_BOUNCE,
    ON_COLLIDE_DEAL_DAMAGE,
    ON_COLLIDE_DIE,
    //ON_COLLIDE_EXPLODE,
} CollideEffectKind;

typedef enum {
    OBJECT_KIND_ENTITIES = (1 << 0),
    OBJECT_KIND_TILES = (1 << 1),
} ObjectKind;

typedef enum {
    COLL_RETRIGGER_WHENEVER,
    COLL_RETRIGGER_NEVER,
    COLL_RETRIGGER_AFTER_NON_CONTACT,
} CollisionRetriggerBehaviour;

typedef struct {
    CollideEffectKind kind;
    ObjectKind affects_object_kinds;
    b32 affects_same_faction_entities; // TODO: make into bitset of ignored factions
    CollisionRetriggerBehaviour retrigger_behaviour;

    union {
        struct {
            Damage damage;
        } deal_damage;
    } as;
} OnCollisionEffect;

typedef struct {
    // TODO: offset from pos
    Vector2 size;

    struct {
        OnCollisionEffect effects[4];
        s32 count;
    } on_collide_effects;
} ColliderComponent;

static inline void add_collide_effect(ColliderComponent *c, OnCollisionEffect effect)
{
    ASSERT(c->on_collide_effects.count < ARRAY_COUNT(c->on_collide_effects.effects));
    ssize index = c->on_collide_effects.count++;
    c->on_collide_effects.effects[index] = effect;

}

static inline void add_blocking_collide_effect(ColliderComponent *c, ObjectKind affected_objects, b32 affect_same_faction)
{
    OnCollisionEffect effect = {0};
    effect.kind = ON_COLLIDE_STOP;
    effect.affects_object_kinds = affected_objects;
    effect.affects_same_faction_entities = affect_same_faction;

    add_collide_effect(c, effect);
}

static inline void add_damage_collision_effect(ColliderComponent *c, Damage damage,
    ObjectKind affected_objects, b32 affect_same_faction)
{
    OnCollisionEffect effect = {0};
    effect.kind = ON_COLLIDE_DEAL_DAMAGE;
    effect.affects_object_kinds = affected_objects;
    effect.as.deal_damage.damage = damage;
    effect.affects_same_faction_entities = affect_same_faction;
    effect.retrigger_behaviour = COLL_RETRIGGER_AFTER_NON_CONTACT;

    add_collide_effect(c, effect);
}

static inline void add_die_collision_effect(ColliderComponent *c, ObjectKind affected_objects, b32 affect_same_faction)
{
    OnCollisionEffect effect = {0};
    effect.kind = ON_COLLIDE_DIE;
    effect.affects_object_kinds = affected_objects;
    effect.affects_same_faction_entities = affect_same_faction;

    add_collide_effect(c, effect);
}

static inline void add_bounce_collision_effect(ColliderComponent *c, ObjectKind affected_objects, b32 affect_same_faction)
{
    OnCollisionEffect effect = {0};
    effect.kind = ON_COLLIDE_BOUNCE;
    effect.affects_object_kinds = affected_objects;
    effect.affects_same_faction_entities = affect_same_faction;

    add_collide_effect(c, effect);
}

static inline void add_passthrough_collision_effect(ColliderComponent *c, ObjectKind affected_objects)
{
    OnCollisionEffect effect = {0};
    effect.kind = ON_COLLIDE_PASS_THROUGH;
    effect.affects_object_kinds = affected_objects;
    effect.affects_same_faction_entities = true;

    add_collide_effect(c, effect);
}

typedef struct {
    Health health;
} HealthComponent;

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

static inline void particle_spawner_initialize(ParticleSpawner *ps, ParticleSpawnerConfig config)
{
    ring_initialize_static(&ps->particle_buffer);
    ps->config = config;
    ps->particles_left_to_spawn = config.total_particles_to_spawn;
}

typedef struct {
    TextureHandle texture;
    Vector2 size;
} SpriteComponent;

typedef struct {
    f32 time_to_live;
} LifetimeComponent;

typedef enum {
    DEATH_EFFECT_SPAWN_PARTICLES,
} OnDeathEffectKind;

typedef struct {
    ParticleSpawnerConfig config;
} DeathEffectSpawnParticles;

// TODO: multiple on death effects
typedef struct {
    OnDeathEffectKind kind;

    union {
        DeathEffectSpawnParticles spawn_particles;
    } as;
} OnDeathComponent;

typedef struct {
    ResistanceStats resistances;
} StatsComponent;

#endif //COMPONENT_H
