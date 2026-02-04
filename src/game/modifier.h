#ifndef MODIFIER_H
#define MODIFIER_H

#include "base/format.h"
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

static inline Modifier create_modifier(Stat stat, StatValue value, NumericModifierType mod_type)
{
    Modifier result = {0};

    result.target_stat = stat;
    result.value = value;
    result.modifier_type = mod_type;

    return result;
}

static inline String modifier_to_string(Modifier modifier, Allocator alloc)
{
    String result = {0};
    String mod_value = s64_to_string(modifier.value, alloc);

    if (modifier.modifier_type == NUMERIC_MOD_FLAT_ADDITIVE) {
	result = str_concat(str_lit("+"), mod_value, alloc);
    } else if (modifier.modifier_type == NUMERIC_MOD_ADDITIVE_PERCENTAGE) {
	result = str_concat(mod_value, str_lit("% increased"), alloc);
    }  else if (modifier.modifier_type == NUMERIC_MOD_MULTIPLICATIVE_PERCENTAGE) {
	f32 percentage = (f32)modifier.value / 100.0f;

	result = str_concat(f32_to_string(percentage, 2, alloc), str_lit("x"), alloc);
    } else {
	ASSERT(0);
    }

    result = str_concat(result, str_lit(" "), alloc);
    result = str_concat(result, stat_to_string(modifier.target_stat), alloc);

    return result;
}

#endif //MODIFIER_H
