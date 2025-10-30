#ifndef MODIFIER_H
#define MODIFIER_H

#include "damage.h"

typedef enum {
    MODIFIER_DAMAGE = (1 << 0),
    MODIFIER_RESISTANCE = (1 << 1),
} ModifierKind;

typedef struct {
    ModifierKind kind;

    union {
        DamageValueModifiers damage_modifier;
        DamageTypes resistance_modifier;
    } as;
} Modifier;

#endif //MODIFIER_H
