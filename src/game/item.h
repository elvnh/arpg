#ifndef ITEM_H
#define ITEM_H

struct World;
struct Entity;

typedef enum {
    ITEM_NULL,
    ITEM_SWORD,
    ITEM_SHIELD,
} Item;

struct Entity *make_entity_from_item(struct World *world, Item item);

#endif // ITEM_H
