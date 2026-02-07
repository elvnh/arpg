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

#endif //ENTITY_FACTION_H
