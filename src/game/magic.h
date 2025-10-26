#ifndef MAGIC_H
#define MAGIC_H

#include "base/vector.h"
#include "asset.h"
#include "damage.h"

typedef enum {
    SPELL_FIREBALL,
    SPELL_COUNT,
} SpellID;

typedef enum {
    SPELL_PROP_PROJECTILE = (1 << 0),
    SPELL_PROP_DAMAGING = (1 << 1),
    SPELL_PROP_SPRITE = (1 << 2),
    SPELL_PROP_BOUNCE_ON_TILES = (1 << 3),
} SpellProperties;

typedef struct {
    SpellProperties properties;

    /* Flag specific fields */
    struct {
	TextureHandle texture;
	Vector2 sprite_size;
    } sprite;

    struct {
	Damage base_damage;
    } damaging;

    struct {
	f32 projectile_speed;
	Vector2 collider_size;
    } projectile;

    /*
      damaging
      healing
      spell kind - projectile, channeling etc

      Damage
      damage roll

      Projectile
      projectile speed


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
void magic_cast_spell(struct World *world, SpellID id, struct Entity *caster,
    Vector2 position, Vector2 direction);
void magic_set_global_spell_array(SpellArray *spells);

#endif //MAGIC_H
