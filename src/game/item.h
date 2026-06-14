#ifndef ITEM_H
#define ITEM_H

struct World;
struct Entity;

#include "components/modifier.h"

// TODO: rename to BaseItemID
typedef enum {
    ITEM_SWORD,
    ITEM_SHIELD,
    ITEM_COUNT,
} ItemID;

typedef struct {
    ItemID base_item;
    ItemModifiers modifiers;
} Item;

struct Entity *make_entity_from_item(struct World *world, Item item);

#endif // ITEM_H
