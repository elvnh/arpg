#ifndef ENTITY_FACTION_H
#define ENTITY_FACTION_H

// NOTE: this is a separate file to avoid a circular include
// TODO: fix this in a more proper way

typedef enum {
    FACTION_NEUTRAL,
    FACTION_PLAYER,
    FACTION_ENEMY,
    FACTION_COUNT,
} EntityFaction;

#endif //ENTITY_FACTION_H
