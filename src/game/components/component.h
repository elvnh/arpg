#ifndef COMPONENT_H
#define COMPONENT_H

#include "base/rgba.h"
#include "base/utils.h"
#include "damage.h"
#include "item.h"
#include "particle.h"
#include "asset.h"
#include "collider.h"
#include "status_effect.h"
#include "equipment.h"
#include "inventory.h"
#include "animation.h"
#include "entity_state.h"

#define COMPONENT_LIST                          \
    COMPONENT(ColliderComponent)                \
    COMPONENT(HealthComponent)                  \
    COMPONENT(ParticleSpawner)                  \
    COMPONENT(SpriteComponent)                  \
    COMPONENT(LifetimeComponent)                \
    COMPONENT(OnDeathComponent)                 \
    COMPONENT(StatsComponent)                   \
    COMPONENT(StatusEffectComponent)            \
    COMPONENT(InventoryComponent)               \
    COMPONENT(EquipmentComponent)               \
    COMPONENT(AnimationComponent)               \

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

typedef struct {
    Health health;
} HealthComponent;

typedef struct {
    Sprite sprite;
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
    DamageTypes base_resistances;
} StatsComponent;

typedef struct {
    Inventory inventory;
} InventoryComponent;

typedef struct {
    Equipment equipment;
} EquipmentComponent;

// TODO: allow resetting animation when starting it anew

typedef struct {
    AnimationInstance state_animations[ENTITY_STATE_COUNT];
} AnimationComponent;

#endif //COMPONENT_H
