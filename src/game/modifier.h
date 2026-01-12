#ifndef MODIFIER_H
#define MODIFIER_H

#include "base/format.h"
#include "damage.h"

typedef struct {
    NumericModifierType numeric_mod_type;
    TypedDamageValue value;
} DamageModifier;

typedef struct {
    TypedDamageValue value;
} ResistanceModifier;

typedef struct {
    ModifierTarget target_stat;

    union {
        DamageModifier damage_modifier;
        ResistanceModifier resistance_modifier;
    } as;
} Modifier;

static inline Modifier create_damage_modifier(NumericModifierType boost_kind,
    DamageType type, DamageValue value)
{
    Modifier result = {0};
    result.target_stat = MOD_TARGET_DAMAGE;

    DamageModifier *dmg_mod = &result.as.damage_modifier;
    dmg_mod->numeric_mod_type = boost_kind;
    dmg_mod->value.type = type;
    dmg_mod->value.value = value;

    return result;
}

static inline String modifier_to_string(Modifier modifier, Allocator alloc)
{
    String result = {0};

    switch (modifier.target_stat) {
	case MOD_TARGET_DAMAGE: {
	    DamageModifier dmg_mod = modifier.as.damage_modifier;

	    if (dmg_mod.numeric_mod_type == NUMERIC_MOD_FLAT_ADDITIVE) {
		result = str_concat(str_lit("+"), s64_to_string(dmg_mod.value.value, alloc), alloc);
	    } else if (dmg_mod.numeric_mod_type == NUMERIC_MOD_ADDITIVE_PERCENTAGE) {
		result = str_concat(s64_to_string(dmg_mod.value.value, alloc), str_lit("% increased"), alloc);
	    } else if (dmg_mod.numeric_mod_type == NUMERIC_MOD_MULTIPLICATIVE_PERCENTAGE) {
		f32 damage_value = (f32)dmg_mod.value.value / 100.0f;

		result = str_concat(f32_to_string(damage_value, 2, alloc), str_lit("x"), alloc);
	    } else {
		ASSERT(0);
	    }

	    result = str_concat(result, str_lit(" "), alloc);
	    result = str_concat(result, damage_type_to_string(dmg_mod.value.type), alloc);
	    result = str_concat(result, str_lit(" damage"), alloc);
	} break;

	case MOD_TARGET_RESISTANCE: {
	    ASSERT(0);
	} break;
    }

    return result;
}

#endif //MODIFIER_H
