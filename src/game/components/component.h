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
    DamageTypes base_resistances;
} StatsComponent;

typedef struct {
    ItemID items[32];
    s32 item_count;
} Inventory;

typedef struct {
    Inventory inventory;
} InventoryComponent;

static inline void inventory_add_item(Inventory *inv, ItemID id)
{
    ASSERT(inv->item_count < ARRAY_COUNT(inv->items));
    inv->items[inv->item_count++] = id;
}

static inline ItemID inventory_get_item_at_index(Inventory *inv, ssize index)
{
    ASSERT(index < inv->item_count);
    ItemID result = inv->items[index];

    return result;
}

static inline void inventory_remove_item_at_index(Inventory *inv, ssize index)
{
    ASSERT(index < inv->item_count);
    inv->items[index] = inv->items[inv->item_count - 1];
    inv->item_count -= 1;
}

typedef struct {
    ItemID head;
} Equipment;

static inline b32 can_equip_item_in_slot(Item *item, EquipmentSlot slot)
{
    b32 item_is_equipment = item_has_prop(item, ITEM_PROP_EQUIPMENT);
    b32 result = item_is_equipment && ((item->equipment.slot & slot) != 0);

    return result;
}

typedef struct {
    b32 item_was_replaced;
    ItemID replaced_item;
} EquipResult;

static inline EquipResult equip_item_in_slot(Equipment *eq, ItemID item, EquipmentSlot slot)
{
    // TODO: pass item pointer, get item id from pointer
    //ASSERT(can_equip_item_in_slot(item *item, EquipmentSlot slot))

    EquipResult result = {0};

    switch (slot) {
        case EQUIP_SLOT_HEAD: {
            result.item_was_replaced = !item_ids_equal(eq->head, NULL_ITEM_ID);
            result.replaced_item = eq->head;
            eq->head = item;
        } break;

        INVALID_DEFAULT_CASE;
    }

    return result;
}


typedef struct {
    Equipment equipment;
} EquipmentComponent;

#endif //COMPONENT_H
