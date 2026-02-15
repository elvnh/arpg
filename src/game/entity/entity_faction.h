#ifndef ENTITY_FACTION_H
#define ENTITY_FACTION_H

// NOTE: this is a separate file to avoid a circular include
// TODO: fix this in a more proper way

#include "base/string8.h"

typedef enum {
    FACTION_NEUTRAL,
    FACTION_PLAYER,
    FACTION_ENEMY,
    FACTION_COUNT,
} EntityFaction;

static inline String entity_faction_to_string(EntityFaction faction)
{
    switch (faction) {
        case FACTION_NEUTRAL:  return str_lit("Neutral");
        case FACTION_PLAYER:   return str_lit("Player");
        case FACTION_ENEMY:    return str_lit("Enemy");
        case FACTION_COUNT:    break;
    }

    ASSERT(0);
    return null_string;
}

typedef struct {
    b32 ok;
    EntityFaction hostile_faction;
} GetHostileFactionResult;

static inline GetHostileFactionResult get_hostile_faction(EntityFaction our_faction)
{
    GetHostileFactionResult result = {0};

    if (our_faction == FACTION_PLAYER) {
        result.ok = true;
        result.hostile_faction = FACTION_ENEMY;
    } else if (our_faction == FACTION_ENEMY) {
        result.ok = true;
        result.hostile_faction = FACTION_PLAYER;
    }

    return result;
}

#endif //ENTITY_FACTION_H
