#include "damage.h"
#include "base/utils.h"
#include "components/component.h"
#include "status_effect.h"
#include "entity_system.h"
#include "equipment.h"
#include "item.h"
#include "item_system.h"
#include "modifier.h"
#include "stats.h"

static Stat get_resistance_stat_for_damage_type(DamageType dmg_type)
{
    switch (dmg_type) {
#define DAMAGE_TYPE(name, dmg_stat, res_stat, color) case DMG_TYPE_##name: return res_stat;
	DAMAGE_TYPE_LIST
#undef DAMAGE_TYPE
	case DMG_TYPE_COUNT: ASSERT(0);
    }

    ASSERT(0);

    return 0;
}

static Stat get_stat_affecting_damage_type(DamageType dmg_type)
{
    switch (dmg_type) {
#define DAMAGE_TYPE(name, dmg_stat, res_stat, color) case DMG_TYPE_##name: return dmg_stat;
	DAMAGE_TYPE_LIST
#undef DAMAGE_TYPE

	case DMG_TYPE_COUNT: ASSERT(0);
    }

    ASSERT(0);

    return 0;
}

Damage calculate_damage_received(Entity *entity, DamageInstance dmg, ItemSystem *item_sys)
{
    Damage result = dmg.damage;

    for (DamageType type = 0; type < DMG_TYPE_COUNT; ++type) {
	Stat resistance_stat = get_resistance_stat_for_damage_type(type);

	StatValue resistance_to_type = get_total_stat_value(entity, resistance_stat, item_sys);
	resistance_to_type -= get_damage_value(dmg.penetration, type);

	StatValue base_damage = get_damage_value(dmg.damage, type);
	StatValue final_value = modify_stat_by_percentage(base_damage, 100 - resistance_to_type);

	set_damage_value(&result, type, final_value);
    }

    return result;
}

Damage calculate_damage_dealt(Damage base_damage, Entity *entity, ItemSystem *item_sys)
{
    Damage result = base_damage;

    for (DamageType dmg_type = 0; dmg_type < DMG_TYPE_COUNT; ++dmg_type) {
	Stat stat = get_stat_affecting_damage_type(dmg_type);

	for (NumericModifierType mod_type = 0; mod_type < NUMERIC_MOD_TYPE_COUNT; ++mod_type) {
	    // TODO: if stats etc can modify base damage, make sure to to change this to
	    // get_total_stat_value_of_type
	    StatValue total_mods = get_total_stat_modifier_of_type(entity, stat, mod_type, item_sys);

	    StatValue current_value = get_damage_value(result, dmg_type);
	    StatValue new_value = apply_modifier(current_value, total_mods, mod_type);
	    set_damage_value(&result, dmg_type, new_value);
	}
    }

    return result;
}

Damage roll_damage_in_range(DamageRange damage_range)
{
    // TODO: make caster stats influence damage roll
    Damage result = {0};

    for (DamageType type = 0; type < DMG_TYPE_COUNT; ++type) {
	StatValue low = get_damage_value(damage_range.low_roll, type);
	StatValue high = get_damage_value(damage_range.high_roll, type);

	StatValue roll = rng_s64(low, high);
	set_damage_value(&result, type, roll);
    }

    return result;
}

StatValue calculate_damage_sum(Damage damage)
{
    StatValue result = 0;

    for (DamageType type = 0; type < DMG_TYPE_COUNT; ++type) {
	result += get_damage_value(damage, type);
    }

    return result;
}

void set_damage_value(Damage *damage, DamageType type, StatValue new_value)
{
    ASSERT(type >= 0);
    ASSERT(type < DMG_TYPE_COUNT);

    damage->type_values[type] = new_value;
}

void set_damage_range_for_type(DamageRange *range, DamageType type, StatValue low, StatValue high)
{
    set_damage_value(&range->low_roll, type, low);
    set_damage_value(&range->high_roll, type, high);
}

StatValue get_damage_value(Damage damage, DamageType type)
{
    ASSERT(type >= 0);
    ASSERT(type < DMG_TYPE_COUNT);

    StatValue result = damage.type_values[type];

    return result;
}
