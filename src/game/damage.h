#ifndef DAMAGE_H
#define DAMAGE_H

#include "base/random.h"
#include "base/string8.h"
#include "base/utils.h"
#include "stats.h"

struct ItemSystem;
struct Entity;

#define DAMAGE_TYPE_LIST /* (name, dmg_stat, res_stat, color) */                         \
    DAMAGE_TYPE(Fire, STAT_FIRE_DAMAGE, STAT_FIRE_RESISTANCE, RGBA32_RED)                \
    DAMAGE_TYPE(                                                                         \
        Lightning, STAT_LIGHTNING_DAMAGE, STAT_LIGHTNING_RESISTANCE, RGBA32_YELLOW)

typedef enum {
#define DAMAGE_TYPE(name, dmg_stat, res_stat, color) DMG_TYPE_##name,
    DAMAGE_TYPE_LIST
#undef DAMAGE_TYPE

        DMG_TYPE_COUNT,
} DamageType;

typedef struct {
    StatValue type_values[DMG_TYPE_COUNT];
} Damage;

typedef struct {
    StatValue value;
    DamageType type;
} TypedDamageValue;

typedef struct {
    Damage low_roll;
    Damage high_roll;
} DamageRange;

/* Represents a snapshotted damage range from which damage can be rolled.
 * These are created eg. when casting a spell, and if that spell hits
 * multiple enemies the damage will be rerolled each hit.
 *
 * TODO: better name
 */
typedef struct {
    DamageRange caster_modified_damage_range;
    Damage caster_modified_penetration;
} DamagePreset;

/* Represents an instance of damage that has been rolled and is ready to be dealt */
typedef struct {
    Damage damage;
    Damage penetration;
} DamageInstance;

Damage calculate_damage_dealt(struct EntitySystem *es, struct Entity *entity,
    Damage base_damage);
Damage calculate_damage_received(struct EntitySystem *es, struct Entity *entity,
    DamageInstance dmg);

StatValue calculate_damage_sum(Damage damage);
Damage roll_damage_in_range(DamageRange damage_range);
DamagePreset make_damage_preset(struct EntitySystem *es, struct Entity *damage_dealer,
    DamageRange range, Damage penetration);
DamageInstance roll_damage_from_preset(DamagePreset damage);
void set_damage_value(Damage *damage, DamageType type, StatValue new_value);
void set_damage_range_for_type(DamageRange *range, DamageType type, StatValue low,
    StatValue high);
StatValue get_damage_value(Damage damage, DamageType type);

static inline String damage_type_to_string(DamageType type)
{
    switch (type) {
#define DAMAGE_TYPE(name, dmg_stat, res_stat, color)                                     \
    case DMG_TYPE_##name:                                                                \
        return str(#name);

        DAMAGE_TYPE_LIST
#undef DAMAGE_TYPE

        case DMG_TYPE_COUNT:
            ASSERT(0);
    }

    ASSERT(0);

    return (String){0};
}

static inline RGBA32 get_damage_type_color(DamageType type)
{
    switch (type) {
#define DAMAGE_TYPE(name, dmg_stat, res_stat, color)                                     \
    case DMG_TYPE_##name:                                                                \
        return color;
        DAMAGE_TYPE_LIST
#undef DAMAGE_TYPE

        case DMG_TYPE_COUNT:
            ASSERT(0);
    }

    ASSERT(0);

    return RGBA32_WHITE;
}

#endif //DAMAGE_H
