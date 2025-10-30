#include "damage.h"
#include "base/utils.h"
#include "components/component.h"
#include "components/status_effect.h"
#include "entity_system.h"
#include "equipment.h"
#include "item.h"
#include "item_manager.h"
#include "modifier.h"

// NOTE: the ordering of these affects the order of calculations
typedef enum {
    DMG_CALC_PHASE_ADDITIVE,
    DMG_CALC_PHASE_MULTIPLICATIVE,
    DMG_CALC_PHASE_COUNT,
} DamageCalculationPhase;

static DamageTypes damage_types_add(DamageTypes lhs, DamageTypes rhs)
{
    DamageTypes result = lhs;

    for (DamageKind type = 0; type < DMG_KIND_COUNT; ++type) {
        DamageValue *lhs_ptr = get_damage_reference_of_type(&result, type);
        DamageValue rhs_value = get_damage_value_of_type(rhs, type);

        *lhs_ptr += rhs_value;
    }

    return result;
}

static DamageTypes damage_types_multiply(DamageTypes lhs, DamageTypes rhs)
{
    DamageTypes result = lhs;

    for (DamageKind type = 0; type < DMG_KIND_COUNT; ++type) {
        DamageValue *lhs_ptr = get_damage_reference_of_type(&result, type);
        DamageValue rhs_value = get_damage_value_of_type(rhs, type);

        f32 n = 1.0f + (f32)(rhs_value) / 100.0f;

        *lhs_ptr = (DamageValue)((f32)(*lhs_ptr) * n);
    }

    return result;
}

static DamageTypes apply_damage_value_modifier(DamageTypes value, Modifier modifier,
    ModifierKind interesting_mods, DamageCalculationPhase phase)
{
    DamageTypes result = value;

    b32 is_interesting = (modifier.kind & interesting_mods) != 0;

    if (is_interesting) {
        switch (modifier.kind) {
            case MODIFIER_DAMAGE: {
                if (phase == DMG_CALC_PHASE_ADDITIVE) {
                    result = damage_types_add(result, modifier.as.damage_modifier.additive_modifiers);
                } else {
                    result = damage_types_multiply(result, modifier.as.damage_modifier.multiplicative_modifiers);
                }
            } break;

            case MODIFIER_RESISTANCE: {
                if (phase == DMG_CALC_PHASE_ADDITIVE) {
                    result = damage_types_add(result, modifier.as.resistance_modifier);
                } else {
                    ASSERT(0);
                }
            } break;
        }
    }

    return result;
}

static DamageTypes apply_damage_value_status_effects(DamageTypes value, struct Entity *entity,
    ModifierKind interesting_mods, DamageCalculationPhase phase)
{
    DamageTypes result = value;

    StatusEffectComponent *status_effects = es_get_component(entity, StatusEffectComponent);

    if (status_effects) {
        for (s32 i = 0; i < status_effects->effect_count; ++i) {
            StatusEffect *effect = &status_effects->effects[i];
            ASSERT(effect->expiry_time >= 0.0f);

            result = apply_damage_value_modifier(result, effect->modifier, interesting_mods, phase);
        }
    }

    return result;
}

static DamageTypes apply_damage_modifiers_in_slot(DamageTypes value, Equipment *eq,
    EquipmentSlot slot, ModifierKind interesting_mods, DamageCalculationPhase phase,
    ItemManager *item_mgr)
{
    DamageTypes result = value;

    ItemID item_id = get_equipped_item_in_slot(eq, slot);

    Item *item = item_mgr_get_item(item_mgr, item_id);

    if (item && item_has_prop(item, ITEM_PROP_HAS_MODIFIERS)) {
        ASSERT(item_has_prop(item, ITEM_PROP_EQUIPMENT));

        // TODO: break this out into function and unit test
        for (s32 i = 0; i < item->modifiers.modifier_count; ++i) {
            Modifier mod = item->modifiers.modifiers[i];

            result = apply_damage_value_modifier(result, mod, interesting_mods, phase);
        }
    }

    return result;
}

static DamageTypes apply_damage_value_equipment_modifiers(DamageTypes value, struct Entity *entity,
    ModifierKind interesting_mods, DamageCalculationPhase phase, ItemManager *item_mgr)
{
    DamageTypes result = value;
    EquipmentComponent *equipment = es_get_component(entity, EquipmentComponent);

    if (equipment) {
        // TODO: make it possible to for loop over all slots
        result = apply_damage_modifiers_in_slot(result, &equipment->equipment, EQUIP_SLOT_HEAD,
            interesting_mods, phase, item_mgr);
    }

    return result;
}

// TODO: I don't like that ItemManager is a parameter here
DamageTypes calculate_damage_after_boosts(DamageTypes damage, struct Entity *entity, struct ItemManager *item_mgr)
{
    DamageTypes result = damage;

    for (DamageCalculationPhase phase = 0; phase < DMG_CALC_PHASE_COUNT; ++phase) {
        result = apply_damage_value_status_effects(result, entity, MODIFIER_DAMAGE, phase);
        result = apply_damage_value_equipment_modifiers(result, entity, MODIFIER_DAMAGE, phase, item_mgr);
    }

    return result;
}

// TODO: make this not require base_resistances as parameter
DamageTypes calculate_resistances_after_boosts(DamageTypes base_resistances, struct Entity *entity,
    struct ItemManager *item_mgr)
{
    DamageTypes result = base_resistances;

    // NOTE: resistances only have an additive phase
    result = apply_damage_value_status_effects(result, entity, MODIFIER_RESISTANCE, DMG_CALC_PHASE_ADDITIVE);
    result = apply_damage_value_equipment_modifiers(result, entity, MODIFIER_RESISTANCE, DMG_CALC_PHASE_ADDITIVE, item_mgr);

    return result;
}
