#include "damage.h"
#include "base/utils.h"
#include "components/component.h"
#include "components/status_effect.h"
#include "entity_system.h"
#include "equipment.h"
#include "item.h"
#include "item_manager.h"
#include "modifier.h"

// TODO: use this in mods too
typedef struct {
    DamageValue value;
    DamageKind type;
} TypedDamageValue;

static DamageValue *get_damage_reference_of_type(DamageValues *damages, DamageKind type)
{
    ssize index = (ssize)type;
    ASSERT((index >= 0) && (index < DMG_KIND_COUNT));

    DamageValue *result = &damages->values[index];

    return result;
}


void set_damage_value_of_type(DamageValues *damages, DamageKind kind, DamageValue new_value)
{
    DamageValue *old_value = get_damage_reference_of_type(damages, kind);
    *old_value = new_value;
}

static DamageValues damage_types_add(DamageValues lhs, DamageValues rhs)
{
    DamageValues result = lhs;

    for (DamageKind type = 0; type < DMG_KIND_COUNT; ++type) {
        DamageValue *lhs_ptr = get_damage_reference_of_type(&result, type);
        DamageValue rhs_value = get_damage_value_for_type(rhs, type);

        *lhs_ptr += rhs_value;
    }

    return result;
}

static DamageValues damage_types_multiply(DamageValues lhs, DamageValues rhs)
{
    DamageValues result = lhs;

    for (DamageKind type = 0; type < DMG_KIND_COUNT; ++type) {
        DamageValue *lhs_ptr = get_damage_reference_of_type(&result, type);
        DamageValue rhs_value = get_damage_value_for_type(rhs, type);

        f32 n = 1.0f + (f32)(rhs_value) / 100.0f;

        *lhs_ptr = (DamageValue)((f32)(*lhs_ptr) * n);
    }

    return result;
}

static TypedDamageValue get_relevant_modifier_value(StatModifier modifier, ModifierKind mod_category,
    NumericModifierType type)
{
    TypedDamageValue result = {0};

    if (modifier.kind == mod_category) {
	switch (modifier.kind) {
	    case MODIFIER_DAMAGE: {
		DamageModifier dmg_mod = modifier.as.damage_modifier;

		if (dmg_mod.applied_during_phase == type) {
		    result.value = dmg_mod.value;
		    result.type = dmg_mod.affected_damage_kind;
		}
	    } break;

	    case MODIFIER_RESISTANCE: {
		ResistanceModifier res_mod = modifier.as.resistance_modifier;

		// NOTE: resistances are only additive
		if (type == NUMERIC_MOD_FLAT_ADDED) {
		    result.value = res_mod.value;
		    result.type = res_mod.type;
		} else {
		    ASSERT(0);
		}
	    } break;

	    INVALID_DEFAULT_CASE;
	}
    }

    return result;
}

static DamageValues apply_damage_values_as(DamageValues lhs, DamageValues rhs, NumericModifierType type)
{
    DamageValues result = {0};

    if (type == NUMERIC_MOD_FLAT_ADDED) {
	result = damage_types_add(lhs, rhs);
    } else if (type == NUMERIC_MOD_MULTIPLICATIVE_PERCENTAGE) {
	result = damage_types_multiply(lhs, rhs);
    } else {
	ASSERT(0);
    }

    return result;
}

static DamageValues apply_damage_value_as(DamageValues lhs, TypedDamageValue value, NumericModifierType type)
{
    DamageValues extended_rhs = {0};
    set_damage_value_of_type(&extended_rhs, value.type, value.value);

    DamageValues result = apply_damage_values_as(lhs, extended_rhs, type);

    return result;
}

static DamageValues get_modifiers_in_slot(Equipment *eq, EquipmentSlot slot,
    NumericModifierType type, ModifierKind mod_category, ItemManager *item_mgr)
{
    DamageValues result = {0};

    ItemID item_id = get_equipped_item_in_slot(eq, slot);

    if (!item_id_is_null(item_id)) {
	Item *item = item_mgr_get_item(item_mgr, item_id);

	ASSERT(item_has_prop(item, ITEM_PROP_EQUIPPABLE));

	if (item_has_prop(item, ITEM_PROP_HAS_MODIFIERS)) {
	    for (s32 i = 0; i < item->modifiers.modifier_count; ++i) {
		TypedDamageValue item_mod_value = get_relevant_modifier_value(
		    item->modifiers.modifiers[i],
		    mod_category,
		    type);

		result = apply_damage_value_as(result, item_mod_value, type);
	    }
	}
    }

    return result;
}

static DamageValues get_status_effect_modifiers_of_type(Entity *entity, NumericModifierType type,
    ModifierKind mod_category)
{
    DamageValues result = {0};
    StatusEffectComponent *status_effects = es_get_component(entity, StatusEffectComponent);

    if (status_effects) {
	for (s32 i = 0; i < status_effects->effect_count; ++i) {
            StatusEffect effect = status_effects->effects[i];
            ASSERT(effect.time_remaining >= 0.0f);

            TypedDamageValue effect_mod_value = get_relevant_modifier_value(
		effect.modifier, mod_category, type);

	    result = apply_damage_value_as(result, effect_mod_value, type);
	}
    }

    return result;
}

static DamageValues get_equipment_modifiers_of_type(Entity *entity, NumericModifierType type,
    ModifierKind mod_category, ItemManager *item_mgr)
{
    DamageValues result = {0};

    EquipmentComponent *equipment = es_get_component(entity, EquipmentComponent);

    if (equipment) {
	for (EquipmentSlot slot = 0; slot < EQUIP_SLOT_COUNT; ++slot) {
	    DamageValues slot_mods = get_modifiers_in_slot(
		&equipment->equipment, slot, type, mod_category, item_mgr);

	    result = apply_damage_values_as(result, slot_mods, type);
	}
    }

    return result;
}

DamageValues get_numeric_modifiers_of_type(Entity *entity, NumericModifierType type,
    ModifierKind mod_category, ItemManager *item_mgr)
{
    DamageValues equipment_mods = get_equipment_modifiers_of_type(entity, type, mod_category, item_mgr);
    DamageValues status_mods = get_status_effect_modifiers_of_type(entity, type, mod_category);

    DamageValues result = {0};

    result = apply_damage_values_as(result, equipment_mods, type);
    result = apply_damage_values_as(result, status_mods, type);

    return result;
}

DamageValue calculate_damage_sum(DamageValues damage)
{
    DamageValue result = 0;

    for (DamageKind kind = 0; kind < DMG_KIND_COUNT; ++kind) {
	result += get_damage_value_for_type(damage, kind);
    }

    return result;
}

DamageValue get_damage_value_for_type(DamageValues damages, DamageKind type)
{
    DamageValue result = *get_damage_reference_of_type(&damages, type);
    return result;
}

DamageValues calculate_damage_dealt(DamageValues base_damage, Entity *entity, ItemManager *item_mgr)
{
    DamageValues flat_added_mods = get_numeric_modifiers_of_type(
	entity, NUMERIC_MOD_FLAT_ADDED, MODIFIER_DAMAGE, item_mgr);
    DamageValues multiplicative_mods = get_numeric_modifiers_of_type(
	entity, NUMERIC_MOD_MULTIPLICATIVE_PERCENTAGE, MODIFIER_DAMAGE, item_mgr);

    DamageValues result = base_damage;
    result = apply_damage_values_as(result, flat_added_mods, NUMERIC_MOD_FLAT_ADDED);
    result = apply_damage_values_as(result, multiplicative_mods, NUMERIC_MOD_MULTIPLICATIVE_PERCENTAGE);

    return result;
}

DamageValues calculate_resistances_after_boosts(Entity *entity, ItemManager *item_mgr)
{
    DamageValues result = {0};

    StatsComponent *stats = es_get_component(entity, StatsComponent);
    if (stats) {
	result = stats->base_resistances;
    }

    DamageValues res_mods = get_numeric_modifiers_of_type(entity, NUMERIC_MOD_FLAT_ADDED,
	MODIFIER_RESISTANCE, item_mgr);

    result = apply_damage_values_as(result, res_mods, NUMERIC_MOD_FLAT_ADDED);

    return result;
}

static DamageValue calculate_damage_of_type_after_resistance(DamageInstance damage,
    DamageValues resistances, DamageKind dmg_type)
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

    for (DamageKind dmg_kind = 0; dmg_kind < DMG_KIND_COUNT; ++dmg_kind) {
	DamageValue dmg = calculate_damage_of_type_after_resistance(damage, resistances, dmg_kind);
	set_damage_value_of_type(&result, dmg_kind, dmg);
    }

    return result;
}

DamageValues roll_damage_in_range(DamageRange damage_range)
{
    // TODO: make caster stats influence damage roll
    DamageValues result = {0};

    for (DamageKind type = 0; type < DMG_KIND_COUNT; ++type) {
	DamageValue low = get_damage_value_for_type(damage_range.low_roll, type);
	DamageValue high = get_damage_value_for_type(damage_range.high_roll, type);

	DamageValue roll = rng_s64(low, high);
	set_damage_value_of_type(&result, type, roll);
    }

    return result;
}

void set_damage_range_for_type(DamageRange *range, DamageKind type, DamageValue low, DamageValue high)
{
    set_damage_value_of_type(&range->low_roll, type, low);
    set_damage_value_of_type(&range->high_roll, type, high);
}
