#ifndef MAGIC_H
#define MAGIC_H

#include "base/vector.h"
#include "base/string8.h"

struct SpellCasterComponent;
struct Entity;
struct World;

/*
  TODO:
  - Sparks stutter slightly when passing through entity
  - Clean up the different spell casting functions
  - Scaling of spell aoe etc by caster stats
  - Don't define the SpellProperty bit values in declaration, do bitshift when needed
  - Healing spells?
  - Channeling spells
  - Animated spells
  - Blizzards damage roll should be recalculated each hit
  - Use set_damage_range_for_type
  - Make it so that chaining spells reroll their damage each hit
 */

typedef enum {
    SPELL_FIREBALL,
    SPELL_SPARK,
    SPELL_ICE_SHARD,
    SPELL_ICE_SHARD_TRIGGER,
    SPELL_BLIZZARD,
    SPELL_CHAIN,
    SPELL_COUNT,
} SpellID;

void    magic_initialize(void);
void    magic_cast_spell(struct World *world, SpellID id, struct Entity *caster, Vector2 target_pos);
void    magic_add_to_spellbook(struct SpellCasterComponent *spellcaster, SpellID id);
String  spell_type_to_string(SpellID id);
SpellID get_spell_at_spellbook_index(struct SpellCasterComponent *spellcaster, ssize index);
f32     get_spell_cast_duration(SpellID id);

#endif //MAGIC_H
