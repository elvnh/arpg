#include "base/linear_arena.h"
#include "base/list.h"
#include "test_macros.h"
#include "base/sl_list.h"

typedef struct s32_SLNode {
    struct s32_SLNode *next;
    s32 data;
} s32_SLNode;

typedef struct {
    s32_SLNode *head;
    s32_SLNode *tail;
} s32_SLList;

TEST_CASE(sl_list_push_front_in_empty_list)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    s32_SLList list = {0};

    s32_SLNode *node = la_allocate_item(&arena, s32_SLNode);
    sl_list_push_front(&list, node);

    REQUIRE(list_head(&list) == node);
    REQUIRE(list_tail(&list) == node);

    la_destroy(&arena);
}

TEST_CASE(sl_list_push_back_in_empty_list)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    s32_SLList list = {0};

    s32_SLNode *node = la_allocate_item(&arena, s32_SLNode);
    sl_list_push_back(&list, node);

    REQUIRE(list_head(&list) == node);
    REQUIRE(list_tail(&list) == node);

    la_destroy(&arena);
}

TEST_CASE(sl_list_push_back_in_non_empty_list)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    s32_SLList list = {0};

    s32_SLNode *node1 = la_allocate_item(&arena, s32_SLNode);
    sl_list_push_back(&list, node1);

    s32_SLNode *node2 = la_allocate_item(&arena, s32_SLNode);
    sl_list_push_back(&list, node2);

    REQUIRE(list_head(&list) == node1);
    REQUIRE(list_tail(&list) == node2);

    la_destroy(&arena);
}

TEST_CASE(sl_list_push_front_in_non_empty_list)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    s32_SLList list = {0};

    s32_SLNode *node1 = la_allocate_item(&arena, s32_SLNode);
    sl_list_push_back(&list, node1);

    s32_SLNode *node2 = la_allocate_item(&arena, s32_SLNode);
    sl_list_push_front(&list, node2);

    REQUIRE(list_head(&list) == node2);
    REQUIRE(list_tail(&list) == node1);

    la_destroy(&arena);
}

TEST_CASE(sl_list_pop_in_one_element_list)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    s32_SLList list = {0};

    s32_SLNode *node = la_allocate_item(&arena, s32_SLNode);
    sl_list_push_back(&list, node);

    sl_list_pop(&list);

    REQUIRE(list_is_empty(&list));

    la_destroy(&arena);
}

TEST_CASE(sl_list_pop_in_two_element_list)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    s32_SLList list = {0};

    s32_SLNode *node1 = la_allocate_item(&arena, s32_SLNode);
    sl_list_push_back(&list, node1);

    s32_SLNode *node2 = la_allocate_item(&arena, s32_SLNode);
    sl_list_push_back(&list, node2);

    sl_list_pop(&list);

    REQUIRE(list_head(&list) == node2);
    REQUIRE(list_tail(&list) == node2);

    la_destroy(&arena);
}
