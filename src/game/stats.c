#include "stats.h"
#include "base/utils.h"
#include "components/component.h"
#include "components/status_effect.h"
#include "damage.h"
#include "entity.h"
#include "entity_system.h"
#include "item.h"

static StatValue initialize_stat_value(NumericModifierType mod_type)
{
    StatValue result = 0;

    if (mod_type == NUMERIC_MOD_MULTIPLICATIVE_PERCENTAGE) {
	result = 100;
    }

    return result;
}

StatValue modify_stat_by_percentage(StatValue lhs, StatValue percentage)
{
    // TODO: round up or down?
    f32 factor = (f32)percentage / 100.0f;
    f32 value = (f32)lhs * factor;
    StatValue result = (StatValue)value;

    return result;
}

static StatValue accumulate_modifier_value(StatValue lhs, StatValue rhs, NumericModifierType mod_type)
{
    StatValue result = lhs;

    switch (mod_type) {
	case NUMERIC_MOD_FLAT_ADDITIVE:
	case NUMERIC_MOD_ADDITIVE_PERCENTAGE: {
	    result += rhs;
	} break;

	case NUMERIC_MOD_MULTIPLICATIVE_PERCENTAGE: {
	    result = modify_stat_by_percentage(lhs, rhs);
	} break;

	INVALID_DEFAULT_CASE;
    }

    return result;
}

StatValue apply_modifier(StatValue base, StatValue modifier, NumericModifierType mod_type)
{
    StatValue result = base;

    switch (mod_type) {
	case NUMERIC_MOD_FLAT_ADDITIVE: {
	    result += modifier;
	} break;

	case NUMERIC_MOD_ADDITIVE_PERCENTAGE:
	case NUMERIC_MOD_MULTIPLICATIVE_PERCENTAGE: {
	    result = modify_stat_by_percentage(base, modifier);
	} break;

	INVALID_DEFAULT_CASE;
    }

    return result;
}

static b32 modifier_is_relevant(Modifier mod, Stat stat, NumericModifierType mod_type)
{
    b32 result = (mod.target_stat == stat) && (mod.modifier_type == mod_type);

    return result;
}

static StatValue sum_status_effect_modifiers_of_type(Entity *entity, Stat stat,
    NumericModifierType mod_type)
{
    StatValue result = initialize_stat_value(mod_type);

    StatusEffectComponent *status_effects = es_get_component(entity, StatusEffectComponent);

    if (status_effects) {
	for (s32 i = 0; i < status_effects->effect_count; ++i) {
	    StatusEffect effect = status_effects->effects[i];

	    if (modifier_is_relevant(effect.modifier, stat, mod_type)) {
		result = accumulate_modifier_value(result, effect.modifier.value, mod_type);
	    }
	}
    }

    return result;
}

static StatValue sum_modifiers_in_slot(Equipment *eq, Stat stat, EquipmentSlot slot,
    NumericModifierType mod_type, ItemSystem *item_sys)
{
    StatValue result = initialize_stat_value(mod_type);

    ItemID item_id = get_equipped_item_in_slot(eq, slot);

    if (!item_id_is_null(item_id)) {
	Item *item = item_sys_get_item(item_sys, item_id);

	ASSERT(item_has_prop(item, ITEM_PROP_EQUIPPABLE));

	if (item_has_prop(item, ITEM_PROP_HAS_MODIFIERS)) {
	    for (s32 i = 0; i < item->modifiers.modifier_count; ++i) {
		Modifier mod = item->modifiers.modifiers[i];

		if (modifier_is_relevant(mod, stat, mod_type)) {
		    result = accumulate_modifier_value(result, mod.value, mod_type);
		}
	    }
	}
    }

    return result;
}

static StatValue sum_equipment_modifiers_of_type(Entity *entity, Stat stat,
    NumericModifierType mod_type, ItemSystem *item_sys)
{
    StatValue result = initialize_stat_value(mod_type);

    EquipmentComponent *equipment = es_get_component(entity, EquipmentComponent);

    if (equipment) {
	for (EquipmentSlot slot = 0; slot < EQUIP_SLOT_COUNT; ++slot) {
	    StatValue slot_mods = sum_modifiers_in_slot(&equipment->equipment, stat, slot,
		mod_type, item_sys);

	    result = accumulate_modifier_value(result, slot_mods, mod_type);
	}
    }

    return result;
}

StatValue get_total_stat_modifier_of_type(Entity *entity, Stat stat, NumericModifierType mod_type,
    ItemSystem *item_sys)
{
    StatValue equipment_mods = sum_equipment_modifiers_of_type(entity, stat, mod_type, item_sys);
    StatValue status_mods = sum_status_effect_modifiers_of_type(entity, stat, mod_type);

    StatValue result = initialize_stat_value(mod_type);

    result = accumulate_modifier_value(result, equipment_mods, mod_type);
    result = accumulate_modifier_value(result, status_mods, mod_type);

    if (mod_type == NUMERIC_MOD_ADDITIVE_PERCENTAGE) {
	// Additive percentages need to get 100% added implicitly at the end so that
	// increasing by 50% actually means a multiplier of 1.5
	result += 100;
    }

    return result;
}

static StatValue get_default_stat_value(Stat stat)
{
    switch (stat) {
	case STAT_CAST_SPEED: return 100;
	default:	      return 0;
    }
}

StatValue get_total_stat_value(Entity *entity, Stat stat, ItemSystem *item_sys)
{
    StatValue result = 0;
    StatsComponent *stats = es_get_component(entity, StatsComponent);

    if (stats) {
	result = get_base_stat_value(stats->stats, stat);
    } else {
	result = get_default_stat_value(stat);
    }

    for (NumericModifierType mod_type = 0; mod_type < NUMERIC_MOD_TYPE_COUNT; ++mod_type) {
	StatValue total_mods = get_total_stat_modifier_of_type(entity, stat, mod_type, item_sys);

	result = apply_modifier(result, total_mods, mod_type);
    }

    return result;
}
