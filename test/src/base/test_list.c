#include "test_macros.h"
#include "base/list.h"

typedef struct s32_DLNode {
    struct s32_DLNode *prev;
    struct s32_DLNode *next;
    s32 data;
} s32_DLNode;

typedef struct {
    s32_DLNode *head;
    s32_DLNode *tail;
} s32_DLList;

TEST_CASE(list_is_empty_when_created)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    s32_DLList list = {0};
    REQUIRE(list_is_empty(&list));
    REQUIRE(list_head(&list) == 0);
    REQUIRE(list_tail(&list) == 0);

    la_destroy(&arena);
}

TEST_CASE(list_push_back)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    s32_DLList list = {0};

    s32_DLNode *node1 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node1);

    REQUIRE(!list_is_empty(&list));
    REQUIRE(list_head(&list) == node1);
    REQUIRE(list_tail(&list) == node1);

    s32_DLNode *node2 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node2);

    REQUIRE(list_head(&list) == node1);
    REQUIRE(list_tail(&list) == node2);

    la_destroy(&arena);
}

TEST_CASE(list_push_front)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    s32_DLList list = {0};

    s32_DLNode *node1 = la_allocate_item(&arena, s32_DLNode);
    list_push_front(&list, node1);

    REQUIRE(!list_is_empty(&list));
    REQUIRE(list_head(&list) == node1);
    REQUIRE(list_tail(&list) == node1);

    s32_DLNode *node2 = la_allocate_item(&arena, s32_DLNode);
    list_push_front(&list, node2);

    REQUIRE(list_head(&list) == node2);
    REQUIRE(list_tail(&list) == node1);

    la_destroy(&arena);
}

TEST_CASE(list_pop_head_one_element_list)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    s32_DLList list = {0};

    s32_DLNode *node1 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node1);
    list_pop_head(&list);

    REQUIRE(list_is_empty(&list));

    la_destroy(&arena);
}

TEST_CASE(list_pop_tail_one_element_list)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    s32_DLList list = {0};

    s32_DLNode *node1 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node1);
    list_pop_tail(&list);

    REQUIRE(list_is_empty(&list));

    la_destroy(&arena);
}

TEST_CASE(list_pop_head_two_element_list)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    s32_DLList list = {0};

    s32_DLNode *node1 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node1);

    s32_DLNode *node2 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node2);

    list_pop_head(&list);

    REQUIRE(!list_is_empty(&list));
    REQUIRE(list_head(&list) == node2);
    REQUIRE(list_tail(&list) == node2);

    la_destroy(&arena);
}

TEST_CASE(list_pop_tail_two_element_list)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    s32_DLList list = {0};

    s32_DLNode *node1 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node1);

    s32_DLNode *node2 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node2);

    list_pop_tail(&list);

    REQUIRE(!list_is_empty(&list));
    REQUIRE(list_head(&list) == node1);
    REQUIRE(list_tail(&list) == node1);

    la_destroy(&arena);
}

TEST_CASE(list_pop_to_empty)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    s32_DLList list = {0};

    s32_DLNode *node1 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node1);

    s32_DLNode *node2 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node2);

    list_pop_tail(&list);
    list_pop_tail(&list);

    REQUIRE(list_is_empty(&list));

    la_destroy(&arena);
}

TEST_CASE(list_pop_to_empty_then_push)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    s32_DLList list = {0};

    s32_DLNode *node1 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node1);

    s32_DLNode *node2 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node2);

    list_pop_tail(&list);
    list_pop_tail(&list);

    list_push_back(&list, node1);
    REQUIRE(list_head(&list) == node1);
    REQUIRE(list_tail(&list) == node1);

    la_destroy(&arena);
}

TEST_CASE(list_clear)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    s32_DLList list = {0};

    s32_DLNode *node1 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node1);

    s32_DLNode *node2 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node2);

    list_clear(&list);

    REQUIRE(list_is_empty(&list));

    la_destroy(&arena);
}

TEST_CASE(list_insert_after_in_one_element_list)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    s32_DLList list = {0};

    s32_DLNode *node1 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node1);

    s32_DLNode *node2 = la_allocate_item(&arena, s32_DLNode);
    list_insert_after(&list, node2, node1);

    REQUIRE(list_head(&list) == node1);
    REQUIRE(list_tail(&list) == node2);

    la_destroy(&arena);
}

TEST_CASE(list_insert_before_in_one_element_list)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    s32_DLList list = {0};

    s32_DLNode *node1 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node1);

    s32_DLNode *node2 = la_allocate_item(&arena, s32_DLNode);
    list_insert_before(&list, node2, node1);

    REQUIRE(list_head(&list) == node2);
    REQUIRE(list_tail(&list) == node1);

    la_destroy(&arena);
}

TEST_CASE(list_insert_after_head_in_two_element_list)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    s32_DLList list = {0};

    s32_DLNode *node1 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node1);

    s32_DLNode *node2 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node2);

    s32_DLNode *node3 = la_allocate_item(&arena, s32_DLNode);
    list_insert_after(&list, node3, node1);

    REQUIRE(list_head(&list) == node1);
    REQUIRE(list_tail(&list) == node2);

    list_pop_head(&list);

    REQUIRE(list_head(&list) == node3);

    la_destroy(&arena);
}

TEST_CASE(list_insert_before_head_in_two_element_list)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    s32_DLList list = {0};

    s32_DLNode *node1 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node1);

    s32_DLNode *node2 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node2);

    s32_DLNode *node3 = la_allocate_item(&arena, s32_DLNode);
    list_insert_before(&list, node3, node1);

    REQUIRE(list_head(&list) == node3);
    REQUIRE(list_tail(&list) == node2);

    list_pop_head(&list);

    REQUIRE(list_head(&list) == node1);

    la_destroy(&arena);
}

TEST_CASE(list_insert_after_tail_in_two_element_list)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    s32_DLList list = {0};

    s32_DLNode *node1 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node1);

    s32_DLNode *node2 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node2);

    s32_DLNode *node3 = la_allocate_item(&arena, s32_DLNode);
    list_insert_after(&list, node3, node2);

    REQUIRE(list_head(&list) == node1);
    REQUIRE(list_tail(&list) == node3);

    list_pop_tail(&list);

    REQUIRE(list_tail(&list) == node2);

    la_destroy(&arena);
}

TEST_CASE(list_insert_before_tail_in_two_element_list)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    s32_DLList list = {0};

    s32_DLNode *node1 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node1);

    s32_DLNode *node2 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node2);

    s32_DLNode *node3 = la_allocate_item(&arena, s32_DLNode);
    list_insert_before(&list, node3, node2);

    REQUIRE(list_head(&list) == node1);
    REQUIRE(list_tail(&list) == node2);

    list_pop_tail(&list);

    REQUIRE(list_tail(&list) == node3);

    la_destroy(&arena);
}

TEST_CASE(list_remove_in_single_element_list)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    s32_DLList list = {0};

    s32_DLNode *node1 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node1);

    list_remove(&list, node1);

    REQUIRE(list_is_empty(&list));

    la_destroy(&arena);
}

TEST_CASE(list_remove_head_in_two_element_list)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    s32_DLList list = {0};

    s32_DLNode *node1 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node1);

    s32_DLNode *node2 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node2);

    list_remove(&list, node1);

    REQUIRE(!list_is_empty(&list));
    REQUIRE(list_head(&list) == node2);
    REQUIRE(list_tail(&list) == node2);

    la_destroy(&arena);
}

TEST_CASE(list_remove_tail_in_two_element_list)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    s32_DLList list = {0};

    s32_DLNode *node1 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node1);

    s32_DLNode *node2 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node2);

    list_remove(&list, node2);

    REQUIRE(!list_is_empty(&list));
    REQUIRE(list_head(&list) == node1);
    REQUIRE(list_tail(&list) == node1);

    la_destroy(&arena);
}

TEST_CASE(list_remove_middle_in_three_element_list)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    s32_DLList list = {0};

    s32_DLNode *node1 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node1);

    s32_DLNode *node2 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node2);

    s32_DLNode *node3 = la_allocate_item(&arena, s32_DLNode);
    list_push_back(&list, node3);

    list_remove(&list, node2);

    REQUIRE(list_head(&list) == node1);
    REQUIRE(list_tail(&list) == node3);

    list_pop_head(&list);
    REQUIRE(list_head(&list) == node3);

    list_pop_head(&list);
    REQUIRE(list_is_empty(&list));

    la_destroy(&arena);
}
