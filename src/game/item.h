#ifndef ITEM_H
#define ITEM_H

#include "base/vector.h"
#include "components/modifier.h"
#include "platform/asset.h"

struct World;
struct Entity;

#define BASE_ITEM_LIST                          \
    BASE_ITEM(SWORD, "Sword", 2, 3)

// clang-format off
#define BASE_ITEM(name, spelling, inv_w, inv_h) ITEM_##name,
typedef enum {
    BASE_ITEM_LIST
    ITEM_COUNT,
} BaseItemID;
#undef BASE_ITEM
// clang-format on

typedef struct {
    BaseItemID base_item;
    ItemModifiers modifiers;
} Item;

typedef struct {
    char *name;
    Vector2i inventory_size;

    TextureHandle sprite;
    Vector2 sprite_size;
} BaseItemInfo;

struct Entity *make_entity_from_item(struct World *world, Item item);

#endif // ITEM_H
