#ifndef MODIFIER_H
#define MODIFIER_H

#include "base/format.h"
#include "damage.h"

typedef struct {
    NumericModifierType numeric_mod_type;

    // TODO: use TypedDamageValue here
    DamageType damage_type;
    DamageValue value;
} DamageModifier;

typedef struct {
    DamageType type;
    DamageValue value;
} ResistanceModifier;

typedef struct {
    ModifierCategory category; // TODO: rename to category

    union {
        DamageModifier damage_modifier;
        ResistanceModifier resistance_modifier;
    } as;
} Modifier;

static inline Modifier create_damage_modifier(NumericModifierType boost_kind,
    DamageType type, DamageValue value)
{
    Modifier result = {0};
    result.category = MOD_CATEGORY_DAMAGE;

    DamageModifier *dmg_mod = &result.as.damage_modifier;
    dmg_mod->numeric_mod_type = boost_kind;
    dmg_mod->damage_type = type;
    dmg_mod->value = value;

    return result;
}

static inline String modifier_to_string(Modifier modifier, Allocator alloc)
{
    String result = {0};

    switch (modifier.category) {
	case MOD_CATEGORY_DAMAGE: {
	    DamageModifier dmg_mod = modifier.as.damage_modifier;

	    if (dmg_mod.numeric_mod_type == NUMERIC_MOD_FLAT_ADDITIVE) {
		result = str_concat(str_lit("+"), s64_to_string(dmg_mod.value, alloc), alloc);
	    } else if (dmg_mod.numeric_mod_type == NUMERIC_MOD_ADDITIVE_PERCENTAGE) {
		result = str_concat(s64_to_string(dmg_mod.value, alloc), str_lit("% increased"), alloc);
	    } else if (dmg_mod.numeric_mod_type == NUMERIC_MOD_MULTIPLICATIVE_PERCENTAGE) {
		f32 damage_value = (f32)dmg_mod.value / 100.0f;

		result = str_concat(f32_to_string(damage_value, 2, alloc), str_lit("x"), alloc);
	    } else {
		ASSERT(0);
	    }

	    result = str_concat(result, str_lit(" "), alloc);
	    result = str_concat(result, damage_type_to_string(dmg_mod.damage_type), alloc);
	    result = str_concat(result, str_lit(" damage"), alloc);
	} break;

	case MOD_CATEGORY_RESISTANCE: {
	    ASSERT(0);
	} break;
    }

    return result;
}

#endif //MODIFIER_H
