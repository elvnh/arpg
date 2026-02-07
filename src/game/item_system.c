#include "item_system.h"
#include "base/free_list_arena.h"
#include "item.h"

void push_item_id(ItemSystem *item_sys, ItemID id)
{
    ring_push(&item_sys->id_queue, &id);
}

void item_sys_initialize(ItemSystem *item_sys)
{
    // TODO: be sure to change to regular init if this becomes heap allocated
    ring_initialize_static(&item_sys->id_queue);

    u32 first_item_id = NULL_ITEM_ID.id + 1;

    for (u32 i = first_item_id; i < MAX_ITEMS + 1; ++i) {
        ItemID id = { .id = i, .generation = 0 };
        push_item_id(item_sys, id);
    }
}

static ItemID get_new_item_id(ItemSystem *item_sys)
{
    ASSERT(!ring_is_empty(&item_sys->id_queue));
    ItemID id = ring_pop_load(&item_sys->id_queue);

    return id;
}

static ItemStorageSlot *get_item_storage_slot(ItemSystem *item_sys, ItemID id)
{
    ItemStorageSlot *result = &item_sys->item_slots[id.id - 1];
    ASSERT(result->generation == id.generation);

    return result;
}

Item *item_sys_get_item(ItemSystem *item_sys, ItemID id)
{
    Item *result = 0;

    if (!item_id_is_null(id)) {
        ItemStorageSlot *slot = get_item_storage_slot(item_sys, id);

        result = &slot->item;
    }

    return result;
}

ItemWithID item_sys_create_item(ItemSystem *item_sys)
{
    ItemID id = get_new_item_id(item_sys);
    Item *item = item_sys_get_item(item_sys, id);
    mem_zero(item, SIZEOF(*item));

    ItemWithID result = {
        .id = id,
        .item = item
    };

    return result;
}

void item_sys_set_item_name(ItemSystem *item_sys, Item *item, String name, LinearArena *world_arena)
{
    // NOTE: for now, just leak memory if item already has a name
    // TODO: add string interning so each unique string is only stored once
    item->name = str_copy(name, la_allocator(world_arena));
}

void item_sys_destroy_item(ItemSystem *item_sys, ItemID id)
{
    ItemStorageSlot *slot = get_item_storage_slot(item_sys, id);

    // TODO: how to handle generation overflow?
    ItemID new_id = {
        .id = id.id,
        .generation = id.generation + 1,
    };

    slot->generation = new_id.generation;

    push_item_id(item_sys, new_id);
}
