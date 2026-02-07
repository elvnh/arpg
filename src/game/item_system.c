#include "item_system.h"
#include "base/free_list_arena.h"
#include "generational_id.h"
#include "item.h"

#define LAST_ITEM_GENERATION S32_MAX

void item_sys_initialize(ItemSystem *item_sys)
{
    for (ItemIndex i = 0; i < MAX_ITEMS; ++i) {
        ItemIDSlot *id_slot = &item_sys->item_ids[i];
        initialize_generational_id(id_slot, i, MAX_ITEMS);
    }
}

static ItemID get_new_item_id(ItemSystem *item_sys)
{
    ASSERT((item_sys->first_free_id_index >= 0) && "Ran out of item ids");

    ItemIDSlot *id_slot = &item_sys->item_ids[item_sys->first_free_id_index];
    ItemID result = {0};
    result.index = item_sys->first_free_id_index;
    result.generation = id_slot->generation;

    remove_generational_id_from_list(item_sys, item_ids, id_slot);

    return result;
}

static b32 item_id_is_valid(ItemSystem *item_sys, ItemID id)
{
    b32 result = (id.index >= 0) && (id.index < MAX_ITEMS)
        && (id.generation >= 1)
        && (item_sys->item_ids[id.index].generation == id.generation);

    return result;
}

Item *item_sys_try_get_item(ItemSystem *item_sys, ItemID id)
{
    Item *result = 0;

    if (item_id_is_valid(item_sys, id)) {
        result = &item_sys->items[id.index];
    }

    return result;
}

Item *item_sys_get_item(ItemSystem *item_sys, ItemID id)
{
    Item *result = item_sys_try_get_item(item_sys, id);
    ASSERT(result);

    return result;
}

ItemWithID item_sys_create_item(ItemSystem *item_sys)
{
    ItemID id = get_new_item_id(item_sys);
    Item *item = item_sys_get_item(item_sys, id);
    mem_zero(item, SIZEOF(*item));

    item->id = id;

    ItemWithID result = {
        .id = id,
        .item = item
    };

    return result;
}

void item_sys_set_item_name(Item *item, String name, LinearArena *world_arena)
{
    // NOTE: for now, just leak memory if item already has a name
    // TODO: add string interning so each unique string is only stored once
    item->name = str_copy(name, la_allocator(world_arena));
}

void item_sys_destroy_item(ItemSystem *item_sys, ItemID id)
{
    ItemIDSlot *slot = &item_sys->item_ids[id.index];

    bump_generation_counter(slot, LAST_ITEM_GENERATION);
    push_generational_id_to_free_list(item_sys, item_ids, id.index);
}

b32 item_sys_item_exists(ItemSystem *item_sys, ItemID item_id)
{
    b32 result = item_sys_try_get_item(item_sys, item_id) != 0;

    return result;
}
