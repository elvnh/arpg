#ifndef MAGIC_H
#define MAGIC_H

typedef enum {
    SPELL_FIREBALL,
    SPELL_COUNT,
} SpellID;

struct Entity;
struct World;
struct AssetList;

void magic_initialize(const struct AssetList *asset_list);
void magic_cast_spell(struct World *world, SpellID id, struct Entity *caster);

#endif //MAGIC_H
