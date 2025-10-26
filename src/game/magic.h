#ifndef MAGIC_H
#define MAGIC_H

#include "base/vector.h"
#include "asset.h"
#include "damage.h"

typedef enum {
    SPELL_FIREBALL,
    SPELL_COUNT,
} SpellID;

typedef struct {
    Damage base_damage;
    TextureHandle texture;
    Vector2 sprite_size;
    /*
      base dmg
      proj? channeling?
      base speed
      sprite
     */
} Spell;

typedef struct SpellArray {
    Spell spells[SPELL_COUNT];
} SpellArray;

struct Entity;
struct World;
struct AssetList;

void magic_initialize(SpellArray *spells, const struct AssetList *asset_list);
void magic_cast_spell(struct World *world, SpellID id, struct Entity *caster, Vector2 position, Vector2 velocity);
void magic_set_global_spell_array(SpellArray *spells);

#endif //MAGIC_H
