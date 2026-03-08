#ifndef STATS_H
#define STATS_H

#include "base/string8.h"
#include "base/typedefs.h"

struct Entity;
struct ItemSystem;
struct EntitySystem;

/*
  TODO:
  - Ensure that stats are clamped, for example negative movement speed doesn't make sense
  - X macros to define enum, stat name and default value in one place
  - Unique modifiers that aren't just numeric
 */

typedef enum {
    STAT_FIRE_DAMAGE,
    STAT_LIGHTNING_DAMAGE,

    // NOTE: resistances are flat additive bonuses
    STAT_FIRE_RESISTANCE,
    STAT_LIGHTNING_RESISTANCE,

    STAT_CAST_SPEED,
    STAT_MOVEMENT_SPEED,
    STAT_ACTION_SPEED,

    STAT_HEALTH,

    STAT_COUNT,
} Stat;

typedef enum {
    NUMERIC_MOD_FLAT_ADDITIVE,
    NUMERIC_MOD_ADDITIVE_PERCENTAGE,
    NUMERIC_MOD_MULTIPLICATIVE_PERCENTAGE,
    NUMERIC_MOD_TYPE_COUNT,
} NumericModifierType;

typedef s64 StatValue;

typedef struct {
    StatValue values[STAT_COUNT];
} StatValues;

StatValue get_total_stat_value(struct EntitySystem *es, struct Entity *entity, Stat stat);
StatValue get_total_stat_modifier_of_type(struct EntitySystem *es, struct Entity *entity,
    Stat stat, NumericModifierType mod_type);
StatValue get_total_cast_speed(struct EntitySystem *es, struct Entity *entity);
StatValue get_total_movement_speed(struct EntitySystem *es, struct Entity *entity);
f32 get_total_movement_speed_value(struct EntitySystem *es, struct Entity *entity);
StatValue apply_modifier(StatValue lhs, StatValue rhs, NumericModifierType mod_type);
StatValue modify_stat_by_percentage(StatValue lhs, StatValue percentage);
StatValues create_base_stats(void);

static inline void set_stat_value(StatValues *stats, Stat stat, StatValue value)
{
    ASSERT(stat >= 0);
    ASSERT(stat < STAT_COUNT);
    stats->values[stat] = value;
}

static inline StatValue *get_stat_reference(StatValues *stats, Stat stat)
{
    ASSERT(stat >= 0);
    ASSERT(stat < STAT_COUNT);

    StatValue *result = &stats->values[stat];

    return result;
}

static inline StatValue get_base_stat_value(StatValues stats, Stat stat)
{
    StatValue result = *get_stat_reference(&stats, stat);

    return result;
}

static inline String stat_to_string(Stat stat)
{
    // clang-format off
    BEGIN_EXHAUSTIVE_SWITCH;
    switch (stat) {
        case STAT_FIRE_DAMAGE:          return str("Fire damage");
        case STAT_LIGHTNING_DAMAGE:     return str("Lightning damage");
        case STAT_FIRE_RESISTANCE:      return str("Fire resistance");
        case STAT_LIGHTNING_RESISTANCE: return str("Lightning resistance");
        case STAT_CAST_SPEED:           return str("Cast speed");
        case STAT_MOVEMENT_SPEED:       return str("Movement speed");
        case STAT_ACTION_SPEED:         return str("Action speed");
        case STAT_HEALTH:               return str("Health");

            INVALID_CASE(STAT_COUNT);
            INVALID_DEFAULT_CASE;
    }
    END_EXHAUSTIVE_SWITCH;
    // clang-format on

    ASSERT(0);
    return str("");
}

static inline f32 stat_value_percentage_as_factor(StatValue value)
{
    f32 result = (f32)value / 100.0f;

    return result;
}

#endif //STATS_H
