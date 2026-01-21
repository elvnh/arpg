#ifndef STATUS_EFFECT_H
#define STATUS_EFFECT_H

#include "base/utils.h"
#include "damage.h"
#include "modifier.h"

struct StatusEffectComponent;

typedef enum {
    STATUS_EFFECT_FROZEN,

    STATUS_EFFECT_COUNT,
} StatusEffectID;

typedef struct {
    StatusEffectID effect_id;
    f32 time_remaining;
} StatusEffectInstance;

void     initialize_status_effect_system();
void     update_status_effects(struct StatusEffectComponent *status_effects, f32 dt);
void     apply_status_effect(struct StatusEffectComponent *comp, StatusEffectID effect_id);
void     try_apply_status_effect_to_entity(struct Entity *entity, StatusEffectID effect_id);
b32      status_effect_modifies_stat(StatusEffectID effect_id, Stat stat, NumericModifierType mod_type);
Modifier get_status_effect_stat_modifier(StatusEffectID effect_id); // TODO: return as ptr, null if none
String	 status_effect_to_string(StatusEffectID effect_id);

#endif //STATUS_EFFECT_H
