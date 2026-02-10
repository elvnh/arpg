#ifndef MODIFIER_H
#define MODIFIER_H

#include "base/format.h"
#include "base/linear_arena.h"
#include "stats.h"

/*
  TODO:
  - Modifier setting stat to exact value rather than modifying
 */

typedef struct {
    Stat target_stat;
    StatValue value;
    NumericModifierType modifier_type;
} Modifier;

typedef struct {
    Modifier modifiers[6];
    s32 modifier_count;
} ItemModifiers;

static inline void add_item_modifier(ItemModifiers *mods, Modifier mod)
{
    ASSERT(mods->modifier_count < ARRAY_COUNT(mods->modifiers));
    mods->modifiers[mods->modifier_count++] = mod;
}

static inline Modifier create_modifier(Stat stat, StatValue value, NumericModifierType mod_type)
{
    Modifier result = {0};

    result.target_stat = stat;
    result.value = value;
    result.modifier_type = mod_type;

    return result;
}

static inline String modifier_to_string(Modifier modifier, LinearArena *arena)
{
    String result = {0};

    switch (modifier.modifier_type) {
        case NUMERIC_MOD_FLAT_ADDITIVE: {
            result = format(arena, "+%ld", modifier.value);
        } break;

        case NUMERIC_MOD_ADDITIVE_PERCENTAGE: {
            result = format(arena, "%ld%% increased", modifier.value);
        } break;

        case NUMERIC_MOD_MULTIPLICATIVE_PERCENTAGE: {
            result = format(arena, "%.2fx", (f64)modifier.value / 100.0);
        } break;

        INVALID_DEFAULT_CASE;
    }

    result = format(arena, FMT_STR" "FMT_STR,
        FMT_STR_ARG(result), FMT_STR_ARG(stat_to_string(modifier.target_stat)));

    return result;
}

#endif //MODIFIER_H
