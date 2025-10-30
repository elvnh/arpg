#ifndef INVENTORY_H
#define INVENTORY_H

typedef struct {
    ItemID items[32];
    s32 item_count;
} Inventory;

static inline void inventory_add_item(Inventory *inv, ItemID id)
{
    ASSERT(inv->item_count < ARRAY_COUNT(inv->items));
    inv->items[inv->item_count++] = id;
}

static inline ItemID inventory_get_item_at_index(Inventory *inv, ssize index)
{
    ASSERT(index < inv->item_count);
    ItemID result = inv->items[index];

    return result;
}

static inline void inventory_remove_item_at_index(Inventory *inv, ssize index)
{
    ASSERT(index < inv->item_count);
    inv->items[index] = inv->items[inv->item_count - 1];
    inv->item_count -= 1;
}

#endif //INVENTORY_H
