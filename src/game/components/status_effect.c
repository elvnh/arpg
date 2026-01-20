#include "status_effect.h"
#include "entity_system.h"
#include "stats.h"
#include "components/component.h"

typedef enum {
    EFFECT_PROP_STAT_MODIFIER	= (1 << 0),
    EFFECT_PROP_STACKABLE	= (1 << 1),
} StatusEffectProperty;

typedef struct {
    StatusEffectProperty properties;
    f32 base_duration;

    Modifier stat_modifier;
    //s32 max_stack_size;
} StatusEffect;

StatusEffect g_status_effects[STATUS_EFFECT_COUNT];

static b32 effect_has_prop(StatusEffect *effect, StatusEffectProperty prop)
{
    b32 result = (effect->properties & prop) != 0;

    return result;
}

static StatusEffect *get_status_effect_by_id(StatusEffectID id)
{
    ASSERT(id >= 0);
    ASSERT(id < STATUS_EFFECT_COUNT);

    StatusEffect *result = &g_status_effects[id];

    return result;
}

static StatusEffect status_effect_frozen()
{
    StatusEffect result = {0};

    result.base_duration = 10.0f;

    result.properties = EFFECT_PROP_STAT_MODIFIER;

    // TODO: reduce action speed, ie both cast speed and movement speed
    result.stat_modifier = create_modifier(STAT_CAST_SPEED, -75, NUMERIC_MOD_FLAT_ADDITIVE);

    return result;
}

void initialize_status_effect_system()
{
    g_status_effects[STATUS_EFFECT_FROZEN] = status_effect_frozen();
}

static void add_to_status_effect_array(StatusEffectComponent *effects, StatusEffectID effect_id)
{
    ASSERT(effects->effect_count < ARRAY_COUNT(effects->effects));
    s32 index = effects->effect_count++;

    StatusEffect *effect = get_status_effect_by_id(effect_id);

    effects->effects[index].effect_id = effect_id;
    effects->effects[index].time_remaining = effect->base_duration;
}

void apply_status_effect(StatusEffectComponent *comp, StatusEffectID effect_id)
{
    StatusEffect *effect = get_status_effect_by_id(effect_id);
    ASSERT(effect->base_duration > 0.0f);

    // Add to effect list
    if (effect_has_prop(effect, EFFECT_PROP_STACKABLE)) {
	// TODO: max status effects
	add_to_status_effect_array(comp, effect_id);
    } else {
	b32 found = false;

	for (ssize i = 0; i < comp->effect_count; ++i) {
	    StatusEffectInstance *curr = &comp->effects[i];

	    if (curr->effect_id == effect_id) {
		// Renew the duration of the non-stackable effect
		curr->time_remaining = effect->base_duration;
		found = true;
		break;
	    }
	}

	if (!found) {
	    add_to_status_effect_array(comp, effect_id);
	}
    }

    // Perform effect specific actions
}

void try_apply_status_effect_to_entity(Entity *entity, StatusEffectID effect_id)
{
    StatusEffectComponent *comp = es_get_component(entity, StatusEffectComponent);

    if (!comp) {
	return;
    }

    apply_status_effect(comp, effect_id);
}

void update_status_effects(StatusEffectComponent *status_effects, f32 dt)
{
    for (s32 i = 0; i < status_effects->effect_count; ++i) {
        StatusEffectInstance *effect = &status_effects->effects[i];

        if (effect->time_remaining <= 0.0f) {
            *effect = status_effects->effects[status_effects->effect_count - 1];
            status_effects->effect_count -= 1;

            // TODO: check to see that this doesn't happen in any other places,
	    // i.e swap-removing and then continuing the loop body even if count reached 0
            if (status_effects->effect_count == 0) {
                break;
            }
        }

        effect->time_remaining -= dt;
    }
}

b32 status_effect_modifies_stat(StatusEffectID effect_id, Stat stat, NumericModifierType mod_type)
{
    b32 result = false;

    StatusEffect *effect = get_status_effect_by_id(effect_id);

    if (effect_has_prop(effect, EFFECT_PROP_STAT_MODIFIER)) {
	Modifier mod = effect->stat_modifier;
	result = (mod.target_stat == stat) && (mod.modifier_type == mod_type);
    }

    return result;
}

Modifier get_status_effect_stat_modifier(StatusEffectID effect_id)
{
    StatusEffect *effect = get_status_effect_by_id(effect_id);
    ASSERT(effect_has_prop(effect, EFFECT_PROP_STAT_MODIFIER));

    Modifier result = effect->stat_modifier;

    return result;
}

String status_effect_to_string(StatusEffectID effect_id)
{
    (void)effect_id;

    return str_lit("<unimplemented>");
}

// on tick
// on apply
// on remove
// ?
