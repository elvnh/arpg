#include "damage.h"
#include "base/utils.h"
#include "components/component.h"
#include "components/status_effect.h"
#include "entity_system.h"
#include "equipment.h"
#include "item.h"
#include "item_system.h"
#include "modifier.h"

// TODO: use this in mods too
typedef struct {
    DamageValue value;
    DamageType type;
} TypedDamageValue;

typedef struct {
    TypedDamageValue value;
    b32 ok;
} GetModValueResult;

static DamageValue *get_damage_reference_for_type(DamageValues *damages, DamageType type)
{
    ssize index = (ssize)type;
    ASSERT((index >= 0) && (index < DMG_TYPE_COUNT));

    DamageValue *result = &damages->values[index];

    return result;
}

void set_damage_value_for_type(DamageValues *damages, DamageType kind, DamageValue new_value)
{
    DamageValue *old_value = get_damage_reference_for_type(damages, kind);
    *old_value = new_value;
}

// This function is used to ensure that if we are currently accumulating multiplicative
// mods, the baseline is 100
static DamageValues initialize_damage_values(NumericModifierType type)
{
    DamageValues result = {0};

    if (type == NUMERIC_MOD_MULTIPLICATIVE_PERCENTAGE) {
	for (DamageType dmg_type = 0; dmg_type < DMG_TYPE_COUNT; ++dmg_type) {
	    set_damage_value_for_type(&result, dmg_type, 100);
	}
    }

    return result;
}

static DamageValues change_damage_value_by_term(DamageValues lhs, DamageValues rhs)
{
    DamageValues result = lhs;

    for (DamageType type = 0; type < DMG_TYPE_COUNT; ++type) {
        DamageValue *lhs_ptr = get_damage_reference_for_type(&result, type);
        DamageValue rhs_value = get_damage_value_for_type(rhs, type);

        *lhs_ptr += rhs_value;
    }

    return result;
}

static DamageValues change_damage_value_by_percentage(DamageValues lhs, DamageValues rhs)
{
    DamageValues result = lhs;

    for (DamageType type = 0; type < DMG_TYPE_COUNT; ++type) {
        DamageValue *lhs_ptr = get_damage_reference_for_type(&result, type);
        DamageValue rhs_value = get_damage_value_for_type(rhs, type);

        f32 factor = (f32)rhs_value / 100.0f;
	f32 value = (f32)(*lhs_ptr) * factor;

        *lhs_ptr = (DamageValue)value;
    }

    return result;
}

static DamageValues add_scalar_to_damage(DamageValues lhs, DamageValue value)
{
    DamageValues result = lhs;

    for (DamageType type = 0; type < DMG_TYPE_COUNT; ++type) {
	result.values[type] += value;
    }

    return result;
}

static DamageValues accumulate_mod_value(DamageValues lhs, DamageValues rhs, NumericModifierType type)
{
    DamageValues result = {0};

    switch (type) {
	case NUMERIC_MOD_FLAT_ADDITIVE:
	case NUMERIC_MOD_ADDITIVE_PERCENTAGE: {
	    result = change_damage_value_by_term(lhs, rhs);
	} break;

	case NUMERIC_MOD_MULTIPLICATIVE_PERCENTAGE: {
	    result = change_damage_value_by_percentage(lhs, rhs);
	} break;

	INVALID_DEFAULT_CASE;
    }

    return result;
}

static DamageValues accumulate_mod_value_single(DamageValues lhs, TypedDamageValue value,
    NumericModifierType type)
{
    DamageValues extended_rhs = initialize_damage_values(type);
    set_damage_value_for_type(&extended_rhs, value.type, value.value);

    DamageValues result = accumulate_mod_value(lhs, extended_rhs, type);

    return result;
}

static GetModValueResult try_get_mod_value_of_type(Modifier modifier, ModifierCategory mod_category,
    NumericModifierType type)
{
    GetModValueResult result = {0};

    if (modifier.kind == mod_category) {
	switch (modifier.kind) {
	    case MOD_CATEGORY_DAMAGE: {
		DamageModifier dmg_mod = modifier.as.damage_modifier;

		if (dmg_mod.applied_during_phase == type) {
		    result.value.value = dmg_mod.value;
		    result.value.type = dmg_mod.affected_damage_kind;
		    result.ok = true;
		}
	    } break;

	    case MOD_CATEGORY_RESISTANCE: {
		ResistanceModifier res_mod = modifier.as.resistance_modifier;

		// NOTE: resistances are only additive
		if (type == NUMERIC_MOD_FLAT_ADDITIVE) {
		    result.value.value = res_mod.value;
		    result.value.type = res_mod.type;

		    result.ok = true;
		} else {
		    ASSERT(0);
		}
	    } break;

	    INVALID_DEFAULT_CASE;
	}
    }

    return result;
}

static DamageValues get_modifiers_in_slot(Equipment *eq, EquipmentSlot slot,
    NumericModifierType type, ModifierCategory mod_category, ItemSystem *item_sys)
{
    DamageValues result = initialize_damage_values(type);

    ItemID item_id = get_equipped_item_in_slot(eq, slot);

    if (!item_id_is_null(item_id)) {
	Item *item = item_sys_get_item(item_sys, item_id);

	ASSERT(item_has_prop(item, ITEM_PROP_EQUIPPABLE));

	if (item_has_prop(item, ITEM_PROP_HAS_MODIFIERS)) {
	    for (s32 i = 0; i < item->modifiers.modifier_count; ++i) {
		GetModValueResult item_mod_value = try_get_mod_value_of_type(
		    item->modifiers.modifiers[i], mod_category, type);

		if (item_mod_value.ok) {
		    result = accumulate_mod_value_single(result, item_mod_value.value, type);
		}
	    }
	}
    }

    return result;
}

static DamageValues get_status_effect_modifiers_of_type(Entity *entity, NumericModifierType type,
    ModifierCategory mod_category)
{
    DamageValues result = initialize_damage_values(type);
    StatusEffectComponent *status_effects = es_get_component(entity, StatusEffectComponent);

    if (status_effects) {
	for (s32 i = 0; i < status_effects->effect_count; ++i) {
            StatusEffect effect = status_effects->effects[i];
            ASSERT(effect.time_remaining >= 0.0f);

            GetModValueResult effect_mod_value = try_get_mod_value_of_type(
		effect.modifier, mod_category, type);

	    if (effect_mod_value.ok) {
		result = accumulate_mod_value_single(result, effect_mod_value.value, type);
	    }
	}
    }

    return result;
}

static DamageValues get_equipment_modifiers_of_type(Entity *entity, NumericModifierType type,
    ModifierCategory mod_category, ItemSystem *item_sys)
{
    DamageValues result = initialize_damage_values(type);

    EquipmentComponent *equipment = es_get_component(entity, EquipmentComponent);

    if (equipment) {
	for (EquipmentSlot slot = 0; slot < EQUIP_SLOT_COUNT; ++slot) {
	    DamageValues slot_mods = get_modifiers_in_slot(
		&equipment->equipment, slot, type, mod_category, item_sys);

	    result = accumulate_mod_value(result, slot_mods, type);
	}
    }

    return result;
}

DamageValues get_numeric_modifiers_of_type(Entity *entity, NumericModifierType type,
    ModifierCategory mod_category, ItemSystem *item_sys)
{
    DamageValues equipment_mods = get_equipment_modifiers_of_type(entity, type, mod_category, item_sys);
    DamageValues status_mods = get_status_effect_modifiers_of_type(entity, type, mod_category);

    DamageValues result = initialize_damage_values(type);

    result = accumulate_mod_value(result, equipment_mods, type);
    result = accumulate_mod_value(result, status_mods, type);

    if (type == NUMERIC_MOD_ADDITIVE_PERCENTAGE) {
	// Additive percentages need to get 100% added implicitly at the end so that
	// increasing by 50% actually means a multiplier of 1.5
	result = add_scalar_to_damage(result, 100);
    }

    return result;
}

DamageValue calculate_damage_sum(DamageValues damage)
{
    DamageValue result = 0;

    for (DamageType kind = 0; kind < DMG_TYPE_COUNT; ++kind) {
	result += get_damage_value_for_type(damage, kind);
    }

    return result;
}

DamageValue get_damage_value_for_type(DamageValues damages, DamageType type)
{
    DamageValue result = *get_damage_reference_for_type(&damages, type);
    return result;
}

DamageValues calculate_damage_dealt(DamageValues base_damage, Entity *entity, ItemSystem *item_sys)
{
    DamageValues flat_added_mods = get_numeric_modifiers_of_type(
	entity, NUMERIC_MOD_FLAT_ADDITIVE, MOD_CATEGORY_DAMAGE, item_sys);
    DamageValues multiplicative_mods = get_numeric_modifiers_of_type(
	entity, NUMERIC_MOD_MULTIPLICATIVE_PERCENTAGE, MOD_CATEGORY_DAMAGE, item_sys);
    DamageValues additive_mods = get_numeric_modifiers_of_type(
	entity, NUMERIC_MOD_ADDITIVE_PERCENTAGE, MOD_CATEGORY_DAMAGE, item_sys);

    DamageValues result = base_damage;

    result = change_damage_value_by_term(result, flat_added_mods);
    result = change_damage_value_by_percentage(result, additive_mods);
    result = change_damage_value_by_percentage(result, multiplicative_mods);

    return result;
}

DamageValues calculate_resistances_after_boosts(Entity *entity, ItemSystem *item_sys)
{
    DamageValues result = {0};

    StatsComponent *stats = es_get_component(entity, StatsComponent);

    if (stats) {
	result = stats->base_resistances;
    }

    DamageValues res_mods = get_numeric_modifiers_of_type(entity, NUMERIC_MOD_FLAT_ADDITIVE,
	MOD_CATEGORY_RESISTANCE, item_sys);

    result = change_damage_value_by_term(result, res_mods);

    return result;
}

static DamageValue calculate_damage_of_type_after_resistance(DamageInstance damage,
    DamageValues resistances, DamageType dmg_type)
{
    DamageValue damage_amount = get_damage_value_for_type(damage.types, dmg_type);
    DamageValue resistance = get_damage_value_for_type(resistances, dmg_type);

    resistance -= get_damage_value_for_type(damage.penetration, dmg_type);

    // TODO: round up or down?
    DamageValue result = (DamageValue)((f64)damage_amount * (1.0 - (f64)resistance / 100.0));

    return result;
}

DamageValues calculate_damage_received(DamageValues resistances, DamageInstance damage)
{
    DamageValues result = {0};

    for (DamageType dmg_kind = 0; dmg_kind < DMG_TYPE_COUNT; ++dmg_kind) {
	DamageValue dmg = calculate_damage_of_type_after_resistance(damage, resistances, dmg_kind);
	set_damage_value_for_type(&result, dmg_kind, dmg);
    }

    return result;
}

DamageValues roll_damage_in_range(DamageRange damage_range)
{
    // TODO: make caster stats influence damage roll
    DamageValues result = {0};

    for (DamageType type = 0; type < DMG_TYPE_COUNT; ++type) {
	DamageValue low = get_damage_value_for_type(damage_range.low_roll, type);
	DamageValue high = get_damage_value_for_type(damage_range.high_roll, type);

	DamageValue roll = rng_s64(low, high);
	set_damage_value_for_type(&result, type, roll);
    }

    return result;
}

void set_damage_range_for_type(DamageRange *range, DamageType type, DamageValue low, DamageValue high)
{
    set_damage_value_for_type(&range->low_roll, type, low);
    set_damage_value_for_type(&range->high_roll, type, high);
}
