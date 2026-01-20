#ifndef STATS_H
#define STATS_H

#include "base/string8.h"
#include "base/typedefs.h"

struct Entity;
struct ItemSystem;

// TODO: function for initializing to base values, eg. cast speed to 100
// NOTE: flat added, additive percentage, multiplicative percentage etc are just
// different ways of modifying the same base stat
typedef enum {
    STAT_FIRE_DAMAGE,
    STAT_LIGHTNING_DAMAGE,

    STAT_FIRE_RESISTANCE,
    STAT_LIGHTNING_RESISTANCE,

    STAT_CAST_SPEED,
    STAT_MOVEMENT_SPEED,

    STAT_COUNT,
} Stat;

// TODO: unique modifiers that aren't just numeric
// NOTE: The order of these decide in which order they are applied
// NOTE: resistances are flat additive bonuses
typedef enum {
    NUMERIC_MOD_FLAT_ADDITIVE,
    NUMERIC_MOD_ADDITIVE_PERCENTAGE,
    NUMERIC_MOD_MULTIPLICATIVE_PERCENTAGE,
    NUMERIC_MOD_TYPE_COUNT,
} NumericModifierType;

typedef struct {
    Stat values[STAT_COUNT];
} StatValues;

typedef s64 StatValue;

StatValue get_total_stat_value(struct Entity *entity, Stat stat, struct ItemSystem *item_sys);
StatValue get_total_stat_modifier_of_type(struct Entity *entity, Stat stat, NumericModifierType mod_type,
					  struct ItemSystem *item_sys);
StatValue apply_modifier(StatValue lhs, StatValue rhs, NumericModifierType mod_type);
StatValue modify_stat_by_percentage(StatValue lhs, StatValue percentage);

static inline void set_stat_value(StatValues *stats, Stat stat, StatValue value)
{
    ASSERT(stat >= 0);
    ASSERT(stat < STAT_COUNT);
    stats->values[stat] = value;
}

static inline StatValue get_base_stat_value(StatValues stats, Stat stat)
{
    ASSERT(stat >= 0);
    ASSERT(stat < STAT_COUNT);

    StatValue result = stats.values[stat];

    return result;
}

static inline String stat_to_string(Stat stat)
{
    switch (stat) {
	case STAT_FIRE_DAMAGE:		  return str_lit("Fire damage");
	case STAT_LIGHTNING_DAMAGE:	  return str_lit("Lightning damage");
	case STAT_FIRE_RESISTANCE:	  return str_lit("Fire resistance");
	case STAT_LIGHTNING_RESISTANCE:   return str_lit("Lightning resistance");
	case STAT_CAST_SPEED:		  return str_lit("Cast speed");
	case STAT_MOVEMENT_SPEED:	  return str_lit("Movement speed");
	case STAT_COUNT:		  ASSERT(0);
    }

    ASSERT(0);
    return str_lit("");
}

#endif //STATS_H
