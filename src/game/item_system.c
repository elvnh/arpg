#include "item_system.h"
#include "base/free_list_arena.h"
#include "item.h"

#define ITEM_SYSTEM_MEMORY_SIZE MB(8)

void push_item_id(ItemSystem *item_mgr, ItemID id)
{
    ring_push(&item_mgr->id_queue, &id);
}

void item_mgr_initialize(ItemSystem *item_mgr, Allocator allocator)
{
    item_mgr->item_system_memory = fl_create(allocator, ITEM_SYSTEM_MEMORY_SIZE);

    // TODO: be sure to change to regular init if this becomes heap allocated
    ring_initialize_static(&item_mgr->id_queue);

    u32 first_item_id = NULL_ITEM_ID.id + 1;

    for (u32 i = first_item_id; i < MAX_ITEMS + 1; ++i) {
        ItemID id = { .id = i, .generation = 0 };
        push_item_id(item_mgr, id);
    }
}

static ItemID get_new_item_id(ItemSystem *item_mgr)
{
    ASSERT(!ring_is_empty(&item_mgr->id_queue));
    ItemID id = ring_pop_load(&item_mgr->id_queue);

    return id;
}

static ItemStorageSlot *get_item_storage_slot(ItemSystem *item_mgr, ItemID id)
{
    ItemStorageSlot *result = &item_mgr->item_slots[id.id - 1];
    ASSERT(result->generation == id.generation);

    return result;
}

Item *item_mgr_get_item(ItemSystem *item_mgr, ItemID id)
{
    Item *result = 0;

    if (!item_id_is_null(id)) {
        ItemStorageSlot *slot = get_item_storage_slot(item_mgr, id);

        result = &slot->item;
    }

    return result;
}

ItemWithID item_mgr_create_item(ItemSystem *item_mgr)
{
    ItemID id = get_new_item_id(item_mgr);
    Item *item = item_mgr_get_item(item_mgr, id);
    mem_zero(item, SIZEOF(*item));

    ItemWithID result = {
        .id = id,
        .item = item
    };

    return result;
}

// TODO: make it possible to set literal as name so it doesn't have to be copied
void item_mgr_set_item_name(ItemSystem *item_mgr, Item *item, String name)
{
    if (item->name.data) {
	fl_deallocate(&item_mgr->item_system_memory, item->name.data);
    }

    item->name = str_copy(name, fl_allocator(&item_mgr->item_system_memory));
}

void item_mgr_destroy_item(ItemSystem *item_mgr, ItemID id)
{
    ItemStorageSlot *slot = get_item_storage_slot(item_mgr, id);

    // TODO: how to handle generation overflow?
    ItemID new_id = {
        .id = id.id,
        .generation = id.generation + 1,
    };

    slot->generation = new_id.generation;

    push_item_id(item_mgr, new_id);
}
