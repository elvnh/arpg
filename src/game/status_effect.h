#ifndef STATUS_EFFECT_H
#define STATUS_EFFECT_H

#include "base/utils.h"
#include "damage.h"
#include "modifier.h"

/*
  TODO:
  - Come up with a better way to iterate through active effects
    - debug.c
    - status_effect.c
    - stats.c
  - Only make stackable effects array as large as number of stackable effect types, same
    for non-stackable effects
 */

struct StatusEffectComponent;

typedef enum {
    STATUS_EFFECT_CHILLED,

    STATUS_EFFECT_COUNT,
} StatusEffectID;

typedef struct {
    f32 time_remaining;
} StatusEffectInstance;

typedef struct {
    StatusEffectInstance effects[64];
    ssize effect_count;
} StatusEffectStack;

typedef struct StatusEffectComponent {
    u64 active_effects_bitset;

    StatusEffectStack stackable_effects[STATUS_EFFECT_COUNT];
    StatusEffectInstance non_stackable_effects[STATUS_EFFECT_COUNT];
} StatusEffectComponent;

void     initialize_status_effect_system();
void     update_status_effects(struct StatusEffectComponent *status_effects, f32 dt);
void     apply_status_effect(struct StatusEffectComponent *comp, StatusEffectID effect_id);
void     try_apply_status_effect_to_entity(struct Entity *entity, StatusEffectID effect_id);
String	 status_effect_to_string(StatusEffectID effect_id);
b32	 has_status_effect(struct StatusEffectComponent *comp, StatusEffectID id);
b32      status_effect_is_stackable(StatusEffectID id);
Modifier get_status_effect_stat_modifier(StatusEffectID effect_id);
b32      status_effect_modifies_stat(StatusEffectID effect_id, Stat stat, NumericModifierType mod_type);

#endif //STATUS_EFFECT_H
