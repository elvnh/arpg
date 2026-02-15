#ifndef COMPONENT_H
#define COMPONENT_H

#include "base/rgba.h"
#include "base/utils.h"
#include "component_id.h"
#include "collision/collision_policy.h"
#include "components/chain.h"
#include "damage.h"
#include "light.h"
#include "particle_spawner.h"
#include "platform/asset.h"
#include "collision/collider.h"
#include "stats.h"
#include "status_effect.h"
#include "equipment.h"
#include "inventory.h"
#include "animation.h"
#include "entity/entity_state.h"
#include "magic.h"
#include "event_listener.h"
#include "ai.h"
#include "health.h"
#include "name.h"

#define COMPONENT_LIST                          \
    COMPONENT(PhysicsComponent)                 \
    COMPONENT(ColliderComponent)                \
    COMPONENT(ParticleSpawner)                  \
    COMPONENT(SpriteComponent)                  \
    COMPONENT(LifetimeComponent)                \
    COMPONENT(StatsComponent)                   \
    COMPONENT(StatusEffectComponent)            \
    COMPONENT(ItemModifiers)			\
    COMPONENT(AnimationComponent)               \
    COMPONENT(SpellCasterComponent)		\
    COMPONENT(EventListenerComponent)		\
    COMPONENT(DamageFieldComponent)		\
    COMPONENT(AIComponent)			\
    COMPONENT(EffectApplierComponent)		\
    COMPONENT(HealthComponent)			\
    COMPONENT(LightEmitter)			\
    COMPONENT(InventoryStorable)                \
    COMPONENT(Inventory)			\
    COMPONENT(Equippable)                       \
    COMPONENT(Equipment)			\
    COMPONENT(NameComponent)			\
    COMPONENT(ArcingComponent)			\
    COMPONENT(ChainComponent)			\

#define ES_IMPL_COMP_ENUM_NAME(type) COMP_##type
#define ES_IMPL_COMP_FIELD_NAME(type) component_##type

typedef enum {
    #define COMPONENT(type) ES_IMPL_COMP_ENUM_NAME(type),
        COMPONENT_LIST
        COMPONENT_COUNT,
    #undef COMPONENT
} ComponentType;

typedef struct PhysicsComponent {
    Vector2 position;
    Vector2 velocity;
    Vector2 direction;
} PhysicsComponent;

typedef struct {
    Sprite sprite;
} SpriteComponent;

typedef struct {
    f32 time_to_live;
} LifetimeComponent;

typedef struct {
    StatValues stats;
} StatsComponent;

typedef struct AnimationComponent {
    // TODO: don't store the animations per component
    AnimationID state_animations[ENTITY_STATE_COUNT];
    AnimationInstance current_animation;
} AnimationComponent;

typedef struct SpellCasterComponent {
    SpellID spellbook[SPELL_COUNT];
    ssize spell_count;
} SpellCasterComponent;

typedef struct {
    DamageInstance damage;
    RetriggerBehaviour retrigger_behaviour;
} DamageFieldComponent;

typedef struct {
    StatusEffectID effect;
    RetriggerBehaviour retrigger_behaviour;
} EffectApplierComponent;

typedef struct AIComponent {
    AIState current_state;
} AIComponent;

typedef struct {
    Health health;
} HealthComponent;

typedef struct {
    LightSource light;
} LightEmitter;

typedef struct StatusEffectComponent {
    u64 active_effects_bitset;

    StatusEffectStack stackable_effects[STATUS_EFFECT_COUNT];
    StatusEffectInstance non_stackable_effects[STATUS_EFFECT_COUNT];
} StatusEffectComponent;

typedef struct {
    EntityID target_entity;
    Vector2 last_known_target_position;
    f32 travel_speed;
    DamageInstance damage_on_target_reached;
} ArcingComponent;

static inline String component_id_to_string(ComponentID id)
{
    switch (id) {
#define COMPONENT(name) case component_id(name): return str_lit(#name);
        COMPONENT_LIST
#undef COMPONENT
    }

    ASSERT(0);
    return null_string;
}

#endif //COMPONENT_H
