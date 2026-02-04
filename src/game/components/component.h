#ifndef COMPONENT_H
#define COMPONENT_H

#include "base/rgba.h"
#include "base/utils.h"
#include "component_id.h"
#include "collision/collision_policy.h"
#include "damage.h"
#include "item.h"
#include "light.h"
#include "particle.h"
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

#define COMPONENT_LIST                          \
    COMPONENT(ColliderComponent)                \
    COMPONENT(ParticleSpawner)                  \
    COMPONENT(SpriteComponent)                  \
    COMPONENT(LifetimeComponent)                \
    COMPONENT(StatsComponent)                   \
    COMPONENT(StatusEffectComponent)            \
    COMPONENT(InventoryComponent)               \
    COMPONENT(EquipmentComponent)               \
    COMPONENT(AnimationComponent)               \
    COMPONENT(GroundItemComponent)		\
    COMPONENT(SpellCasterComponent)		\
    COMPONENT(EventListenerComponent)		\
    COMPONENT(DamageFieldComponent)		\
    COMPONENT(AIComponent)			\
    COMPONENT(EffectApplierComponent)		\
    COMPONENT(HealthComponent)			\
    COMPONENT(LightEmitter)			\

#define ES_IMPL_COMP_ENUM_NAME(type) COMP_##type
#define ES_IMPL_COMP_FIELD_NAME(type) component_##type

typedef enum {
    #define COMPONENT(type) ES_IMPL_COMP_ENUM_NAME(type),
        COMPONENT_LIST
        COMPONENT_COUNT,
    #undef COMPONENT
} ComponentType;

typedef struct {
    Sprite sprite;
} SpriteComponent;

typedef struct {
    f32 time_to_live;
} LifetimeComponent;

typedef struct {
    StatValues stats;
} StatsComponent;

typedef struct {
    Inventory inventory;
} InventoryComponent;

typedef struct {
    Equipment equipment;
} EquipmentComponent;

typedef struct AnimationComponent {
    // TODO: don't store the animations per component
    AnimationID state_animations[ENTITY_STATE_COUNT];
    AnimationInstance current_animation;
} AnimationComponent;

typedef struct {
    ItemID item_id;
} GroundItemComponent;

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

#endif //COMPONENT_H
