#include "item.h"
#include "test_macros.h"
#include "game/item_system.h"

static ItemSystem *allocate_item_system(void)
{
    ItemSystem *result = calloc(1, sizeof(ItemSystem));
    item_sys_initialize(result);

    return result;
}

static void free_item_system(ItemSystem *item_sys)
{
    free(item_sys);
}

TEST_CASE(item_sys_basic)
{
    ItemSystem *item_sys = allocate_item_system();

    ItemWithID item = item_sys_create_item(item_sys);
    REQUIRE(item.item);
    REQUIRE(item_sys_item_exists(item_sys, item.id));
    REQUIRE(item_ids_equal(item.id, item.item->id));
    REQUIRE(str_is_empty(item.item->name));

    free_item_system(item_sys);
}

TEST_CASE(item_sys_destroy)
{
    ItemSystem *item_sys = allocate_item_system();

    ItemWithID item = item_sys_create_item(item_sys);
    item_sys_destroy_item(item_sys, item.id);

    REQUIRE(!item_sys_item_exists(item_sys, item.id));

    free_item_system(item_sys);
}

TEST_CASE(item_sys_faked_id)
{
    ItemSystem *item_sys = allocate_item_system();

    ItemID item_id = {1234, 1234};
    REQUIRE(!item_sys_item_exists(item_sys, item_id));

    free_item_system(item_sys);
}

TEST_CASE(item_sys_zeroed_id)
{
    ItemSystem *item_sys = allocate_item_system();

    ItemID item_id = {0};
    REQUIRE(!item_sys_item_exists(item_sys, item_id));

    free_item_system(item_sys);
}

TEST_CASE(item_sys_create_max)
{
    ItemSystem *item_sys = allocate_item_system();

    ItemID item_ids[MAX_ITEMS] = {0};

    for (ssize i = 0; i < MAX_ITEMS; ++i) {
        ItemWithID item = item_sys_create_item(item_sys);
        item_ids[i] = item.id;
    }

    for (ssize i = 0; i < MAX_ITEMS; ++i) {
        REQUIRE(item_sys_item_exists(item_sys, item_ids[i]));
        Item *item = item_sys_get_item(item_sys, item_ids[i]);
        REQUIRE(str_is_empty(item->name));
    }


    free_item_system(item_sys);
}

TEST_CASE(item_sys_create_destroy_max)
{
    ItemSystem *item_sys = allocate_item_system();

    ItemID item_ids[MAX_ITEMS] = {0};

    for (ssize i = 0; i < MAX_ITEMS; ++i) {
        ItemWithID item = item_sys_create_item(item_sys);
        item_ids[i] = item.id;
    }

    for (ssize i = 0; i < MAX_ITEMS; ++i) {
        item_sys_destroy_item(item_sys, item_ids[i]);
    }

    for (ssize i = 0; i < MAX_ITEMS; ++i) {
        REQUIRE(!item_sys_item_exists(item_sys, item_ids[i]));
    }


    free_item_system(item_sys);
}

TEST_CASE(item_sys_create_destroy_multi_generations)
{
    ItemSystem *item_sys = allocate_item_system();

    ItemID item_ids[MAX_ITEMS] = {0};

    for (ssize gen = 0; gen < 3; ++gen) {
        for (ssize i = 0; i < MAX_ITEMS; ++i) {
            ItemWithID item = item_sys_create_item(item_sys);
            item_ids[i] = item.id;

            REQUIRE(item_sys_item_exists(item_sys, item.id));
            REQUIRE(str_is_empty(item.item->name));
        }

        for (ssize i = 0; i < MAX_ITEMS; ++i) {
            item_sys_destroy_item(item_sys, item_ids[i]);
            REQUIRE(!item_sys_item_exists(item_sys, item_ids[i]));
        }
    }

    free_item_system(item_sys);
}

TEST_CASE(item_sys_set_item_name)
{
    ItemSystem *item_sys = allocate_item_system();
    LinearArena arena = la_create(default_allocator, 256);

    ItemWithID item_with_id = item_sys_create_item(item_sys);
    Item *item = item_sys_get_item(item_sys, item_with_id.id);

    REQUIRE(str_is_empty(item->name));

    String name = str_lit("item");
    item_sys_set_item_name(item, name, &arena);

    REQUIRE(name.data != item->name.data);
    REQUIRE(str_equal(name, item->name));

    la_destroy(&arena);
    free_item_system(item_sys);
}
