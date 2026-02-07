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

static ItemIDSlot *get_item_id_slot_at_index(ItemSystem *item_sys, ItemIndex index)
{
    ASSERT(index >= 0);
    ASSERT(index < MAX_ITEMS);

    ItemIDSlot *result = 0;

    if (index > -1) {
        result = &item_sys->item_ids[index];
    }

    return result;
}

static b32 item_id_is_valid(ItemSystem *item_sys, ItemID id)
{
    b32 result = (id.index >= 0) && (id.index < MAX_ITEMS)
        && (id.generation >= 1)
        && (get_item_id_slot_at_index(item_sys, id.index)->generation == id.generation);

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

Item *create_item_at_index(ItemSystem *item_sys, ItemIndex index)
{
    ASSERT(index >= 0);
    ASSERT(index < MAX_ITEMS);

    Item *item = &item_sys->items[index];
    *item = zero_struct(Item);

    ItemIDSlot *slot = get_item_id_slot_at_index(item_sys, index);
    item->id.index = index;
    item->id.generation = slot->generation;

    return item;
}


static ItemIndex pop_from_free_item_id_list(ItemSystem *item_sys)
{
    ItemIndex result = item_sys->first_free_id_index;
    ItemIDSlot *id_slot = get_item_id_slot_at_index(item_sys, result);

    remove_generational_id_from_list(item_sys, item_ids, id_slot);

    return result;
}

 ItemWithID item_sys_create_item(ItemSystem *item_sys)
{
    ItemIndex index = pop_from_free_item_id_list(item_sys);
    Item *item = create_item_at_index(item_sys, index);

    ASSERT(!item_id_is_null(item->id) && "create_item_at_index should set ID");

    ItemWithID result = {
        .id = item->id,
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
    ItemIDSlot *slot = get_item_id_slot_at_index(item_sys, id.index);

    bump_generation_counter(slot, LAST_ITEM_GENERATION);
    push_generational_id_to_free_list(item_sys, item_ids, id.index);
}

b32 item_sys_item_exists(ItemSystem *item_sys, ItemID item_id)
{
    b32 result = item_sys_try_get_item(item_sys, item_id) != 0;

    return result;
}
