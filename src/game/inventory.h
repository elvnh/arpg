#ifndef INVENTORY_H
#define INVENTORY_H

typedef struct {
    ItemID items[32];
    s32 item_count;
} Inventory;

// TODO: return success/fail
static inline void inventory_add_item(Inventory *inv, ItemID item)
{
    ASSERT(!item_id_is_null(item));

    ASSERT(inv->item_count < ARRAY_COUNT(inv->items));
    inv->items[inv->item_count++] = item;
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

static inline b32 inventory_contains_item(Inventory *inv, ItemID item)
{
    ASSERT(!item_id_is_null(item));

    b32 result = false;

    for (ssize i = 0; i < inv->item_count; ++i) {
	if (item_ids_equal(inv->items[i], item)) {
	    result = true;
	    break;
	}
    }

    return result;
}

static inline void inventory_remove_item(Inventory *inv, ItemID item_to_remove)
{
    ASSERT(!item_id_is_null(item_to_remove));
    ASSERT(inventory_contains_item(inv, item_to_remove));

    for (ssize i = 0; i < inv->item_count; ++i) {
	ItemID item = inv->items[i];

	if (item_ids_equal(item, item_to_remove)) {
	    inventory_remove_item_at_index(inv, i);
	}
    }
}


#endif //INVENTORY_H
