#include "damage.h"
#include "base/utils.h"
#include "components/status_effect.h"
#include "entity_system.h"
#include "item.h"
#include "item_manager.h"
#include "modifier.h"

static DamageValue *get_damage_reference_of_type(DamageValues *damages, DamageKind type)
{
    ssize index = (ssize)type;
    ASSERT((index >= 0) && (index < DMG_KIND_COUNT));

    DamageValue *result = &damages->values[index];

    return result;
}

static DamageValue calculate_damage_of_type_after_resistance(DamageInstance damage,
    DamageValues resistances, DamageKind dmg_type)
{
    DamageValue damage_amount = get_damage_value_of_type(damage.types, dmg_type);
    DamageValue resistance = get_damage_value_of_type(resistances, dmg_type);

    resistance -= get_damage_value_of_type(damage.penetration, dmg_type);

    // TODO: round up or down?
    DamageValue result = (DamageValue)((f64)damage_amount * (1.0 - (f64)resistance / 100.0));

    return result;
}

static DamageValues damage_types_add(DamageValues lhs, DamageValues rhs)
{
    DamageValues result = lhs;

    for (DamageKind type = 0; type < DMG_KIND_COUNT; ++type) {
        DamageValue *lhs_ptr = get_damage_reference_of_type(&result, type);
        DamageValue rhs_value = get_damage_value_of_type(rhs, type);

        *lhs_ptr += rhs_value;
    }

    return result;
}

static DamageValues damage_types_multiply(DamageValues lhs, DamageValues rhs)
{
    DamageValues result = lhs;

    for (DamageKind type = 0; type < DMG_KIND_COUNT; ++type) {
        DamageValue *lhs_ptr = get_damage_reference_of_type(&result, type);
        DamageValue rhs_value = get_damage_value_of_type(rhs, type);

        f32 n = 1.0f + (f32)(rhs_value) / 100.0f;

        *lhs_ptr = (DamageValue)((f32)(*lhs_ptr) * n);
    }

    return result;
}

static DamageValues damage_types_add_scalar(DamageValues lhs, DamageValue value,
    DamageKind type)
{
    // TODO: implement damage_types_add in terms of this
    DamageValues result = lhs;

    DamageValue *lhs_ptr = get_damage_reference_of_type(&result, type);
    *lhs_ptr += value;

    return result;
}

static DamageValues damage_types_multiply_scalar(DamageValues lhs, DamageValue value,
    DamageKind type)
{
    // TODO: implement damage_types_multiply in terms of this
    DamageValues result = lhs;

    DamageValue *lhs_ptr = get_damage_reference_of_type(&result, type);
    f32 n = 1.0f + (f32)(value) / 100.0f;

    *lhs_ptr = (DamageValue)((f32)(*lhs_ptr) * n);

    return result;
}

static DamageValues apply_damage_value_modifier(DamageValues value, StatModifier modifier,
    ModifierKind interesting_mods, NumericModifierType phase)
{
    DamageValues result = value;

    b32 is_interesting = (modifier.kind & interesting_mods) != 0;

    if (is_interesting) {
        switch (modifier.kind) {
            case MODIFIER_DAMAGE: {
		DamageModifier dmg_mod = modifier.as.damage_modifier;

		// TODO: break out into function
		if (dmg_mod.applied_during_phase == phase) {
		    if (phase == NUMERIC_MOD_FLAT_ADDED) {
			result = damage_types_add_scalar(result,
			    dmg_mod.value, dmg_mod.affected_damage_kind);
		    } else if (phase == NUMERIC_MOD_MULTIPLICATIVE_PERCENTAGE) {
			result = damage_types_multiply_scalar(result,
			    dmg_mod.value, dmg_mod.affected_damage_kind);
		    } else {
			ASSERT(0);
		    }
		}
            } break;

            case MODIFIER_RESISTANCE: {
                if (phase == NUMERIC_MOD_FLAT_ADDED) {
                    result = damage_types_add_scalar(result, modifier.as.resistance_modifier.value,
			modifier.as.resistance_modifier.type);
                } else {
                    ASSERT(0);
                }
            } break;
        }
    }

    return result;
}

static DamageValues apply_damage_value_status_effects(DamageValues value, struct Entity *entity,
    ModifierKind interesting_mods, NumericModifierType phase)
{
    DamageValues result = value;

    StatusEffectComponent *status_effects = es_get_component(entity, StatusEffectComponent);

    if (status_effects) {
        for (s32 i = 0; i < status_effects->effect_count; ++i) {
            StatusEffect *effect = &status_effects->effects[i];
            ASSERT(effect->time_remaining >= 0.0f);

            result = apply_damage_value_modifier(result, effect->modifier, interesting_mods, phase);
        }
    }

    return result;
}

static DamageValues apply_damage_modifiers_in_slot(DamageValues value, Equipment *eq,
    EquipmentSlot slot, ModifierKind interesting_mods, NumericModifierType phase,
    ItemManager *item_mgr)
{
    DamageValues result = value;

    ItemID item_id = get_equipped_item_in_slot(eq, slot);

    Item *item = item_mgr_get_item(item_mgr, item_id);

    if (item && item_has_prop(item, ITEM_PROP_HAS_MODIFIERS)) {
        ASSERT(item_has_prop(item, ITEM_PROP_EQUIPPABLE));

        // TODO: break this out into function and unit test
        for (s32 i = 0; i < item->modifiers.modifier_count; ++i) {
            StatModifier mod = item->modifiers.modifiers[i];

            result = apply_damage_value_modifier(result, mod, interesting_mods, phase);
        }
    }

    return result;
}

static DamageValues apply_damage_value_equipment_modifiers(DamageValues value, struct Entity *entity,
    ModifierKind interesting_mods, NumericModifierType phase, ItemManager *item_mgr)
{
    DamageValues result = value;
    EquipmentComponent *equipment = es_get_component(entity, EquipmentComponent);

    if (equipment) {
	for (EquipmentSlot slot = 0; slot < EQUIP_SLOT_COUNT; ++slot) {
	    result = apply_damage_modifiers_in_slot(result, &equipment->equipment, slot,
		interesting_mods, phase, item_mgr);
	}
    }

    return result;
}

// TODO: I don't like that ItemManager is a parameter here
DamageValues calculate_damage_after_boosts(DamageValues damage, struct Entity *entity, struct ItemManager *item_mgr)
{
    DamageValues result = damage;

    for (NumericModifierType phase = 0; phase < NUMERIC_MOD_TYPE_COUNT; ++phase) {
        result = apply_damage_value_status_effects(result, entity, MODIFIER_DAMAGE, phase);
        result = apply_damage_value_equipment_modifiers(result, entity, MODIFIER_DAMAGE, phase, item_mgr);
    }

    return result;
}

// TODO: make this not require base_resistances as parameter
DamageValues calculate_resistances_after_boosts(DamageValues base_resistances, struct Entity *entity,
    struct ItemManager *item_mgr)
{
    DamageValues result = base_resistances;

    // NOTE: resistances only have an additive phase
    result = apply_damage_value_status_effects(result, entity, MODIFIER_RESISTANCE, NUMERIC_MOD_FLAT_ADDED);
    result = apply_damage_value_equipment_modifiers(result, entity, MODIFIER_RESISTANCE, NUMERIC_MOD_FLAT_ADDED, item_mgr);

    return result;
}

DamageValues calculate_damage_dealt_from_damage_range(DamageRange damage_range)
{
    // TODO: make caster stats influence damage roll
    DamageValues result = {0};

    for (DamageKind type = 0; type < DMG_KIND_COUNT; ++type) {
	DamageValue low = get_damage_value_of_type(damage_range.low_roll, type);
	DamageValue high = get_damage_value_of_type(damage_range.high_roll, type);

	DamageValue roll = rng_s64(low, high);
	set_damage_value_of_type(&result, type, roll);
    }

    return result;
}

void set_damage_range_for_type(DamageRange *range, DamageKind type,
    DamageValue low, DamageValue high)
{
    set_damage_value_of_type(&range->low_roll, type, low);
    set_damage_value_of_type(&range->high_roll, type, high);
}

void set_damage_value_of_type(DamageValues *damages, DamageKind kind, DamageValue new_value)
{
    DamageValue *old_value = get_damage_reference_of_type(damages, kind);
    *old_value = new_value;
}

DamageInstance calculate_damage_received(DamageValues resistances, DamageInstance damage)
{
    DamageInstance result = {0};

    for (DamageKind dmg_kind = 0; dmg_kind < DMG_KIND_COUNT; ++dmg_kind) {
        result.types.values[dmg_kind] =
	    calculate_damage_of_type_after_resistance(damage, resistances, dmg_kind);
    }

    return result;
}


DamageValue calculate_damage_sum(DamageValues damage)
{
    DamageValue result = 0;

    for (DamageKind kind = 0; kind < DMG_KIND_COUNT; ++kind) {
	result += get_damage_value_of_type(damage, kind);
    }

    return result;
}

DamageValue get_damage_value_of_type(DamageValues damages, DamageKind type)
{
    DamageValue result = *get_damage_reference_of_type(&damages, type);
    return result;
}

String damage_type_to_string(DamageKind type)
{
    switch (type) {
	case DMG_KIND_FIRE: return str_lit("Fire");
	case DMG_KIND_LIGHTNING: return str_lit("Lightning");
    }

    ASSERT(0);

    return (String){0};
}
