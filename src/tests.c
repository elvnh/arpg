#include "base/list.h"
static void tests_arena()
{
    {
        LinearArena arena = arena_create(default_allocator, 1024);

        s32 *ptr1 = arena_allocate_item(&arena, s32);
        s32 *ptr2 = arena_allocate_item(&arena, s32);

        ASSERT(arena_get_memory_usage(&arena) == 8);

        *ptr1 = 123;
        *ptr2 = 456;

        ASSERT(*ptr1 == 123);
        ASSERT(is_aligned((ssize)ptr1, ALIGNOF(s32)));
        ASSERT(is_aligned((ssize)ptr2, ALIGNOF(s32)));

        *ptr1 = 0;

        ASSERT(*ptr2 == 456);

        arena_reset(&arena);
        s32 *p = arena_allocate_item(&arena, s32);
        ASSERT(p == ptr1);


        arena_destroy(&arena);
    }

    {
        const s32 size = 1024;
        LinearArena arena = arena_create(default_allocator, size);

        byte *arr1 = arena_allocate_array(&arena, byte, size / 2);
        byte *arr2 = arena_allocate_array(&arena, byte, size / 2);

        ASSERT(arr1);
        ASSERT(arr2);
        ASSERT(arena_get_memory_usage(&arena) == size);

        arena_destroy(&arena);
    }

    {
        const s32 size = 1024;
        LinearArena arena = arena_create(default_allocator, size);

        byte *arr1 = arena_allocate_array(&arena, byte, size / 2);
        byte *arr2 = arena_allocate_array(&arena, byte, size / 2 + 1);

        ASSERT(arr1);
        ASSERT(arr2);
        ASSERT(arena_get_memory_usage(&arena) == size + size / 2 + 1);

        byte *arr3 = arena_allocate_array(&arena, byte, 14);
        ASSERT(arr3);

        arena_reset(&arena);
        byte *ptr1 = arena_allocate_item(&arena, byte);
        ASSERT(ptr1 == arr1);

        byte *ptr2 = arena_allocate_array(&arena, byte, size);
        ASSERT(ptr2 == arr2);

        arena_destroy(&arena);
    }

    {
        const s32 size = 1024;
        LinearArena arena = arena_create(default_allocator, size);

        byte *first = arena_allocate_item(&arena, byte);
        byte *arr1 = arena_allocate(&arena, 1, 4, 32);

        ASSERT(first != (arr1 + 1));
        ASSERT(is_aligned((ssize)arr1, 32));

        byte *arr2 = arena_allocate(&arena, 1, 4, 256);
        ASSERT(is_aligned((ssize)arr2, 256));

        arena_destroy(&arena);
    }

    {
        const s32 size = 1024;
        LinearArena arena = arena_create(default_allocator, size);

        byte *arr1 = arena_allocate(&arena, 1, size, 256);
        ASSERT(is_aligned((ssize)arr1, 256));

        arena_destroy(&arena);
    }

    {
        // Zero out new allocations
        LinearArena arena = arena_create(default_allocator, 1024);

        byte *arr1 = arena_allocate_array(&arena, byte, 16);
        memset(arr1, 0xFF, 16);

        arena_reset(&arena);
        byte *arr2 = arena_allocate_array(&arena, byte, 16);
        ASSERT(arr2 == arr1);

        for (s32 i = 0; i < 16; ++i) {
            ASSERT(arr2[i] == 0);
        }

        arena_destroy(&arena);
    }

    {
        LinearArena arena = arena_create(default_allocator, 1024);

        byte *ptr = arena_allocate_array(&arena, byte, 16);
        ASSERT(arena_get_memory_usage(&arena) == 16);

        const bool extended = arena_try_resize(&arena, ptr, 16, 32);
        ASSERT(extended);
        ASSERT(arena_get_memory_usage(&arena) == 32);

        byte *ptr2 = arena_allocate_array(&arena, byte, 16);
        ASSERT(ptr2 == (ptr + 32));
        ASSERT(arena_get_memory_usage(&arena) == 48);
        arena_destroy(&arena);
    }

    {
        LinearArena arena = arena_create(default_allocator, 1024);

        byte *ptr = arena_allocate_array(&arena, byte, 16);
        arena_allocate_array(&arena, byte, 16);

        bool extended = arena_try_resize(&arena, ptr, 16, 32);
        ASSERT(!extended);

        arena_destroy(&arena);
    }

    {
        LinearArena arena = arena_create(default_allocator, 1024);

        byte *ptr = arena_allocate_array(&arena, byte, 16);
        bool extended = arena_try_resize(&arena, ptr, 16, 2000);
        ASSERT(!extended);

        arena_destroy(&arena);
    }

    {
        LinearArena arena = arena_create(default_allocator, 1024);

        byte *first = arena_allocate_array(&arena, byte, 16);
        bool resized = arena_try_resize(&arena, first, 16, 1);
        ASSERT(resized);

        byte *next = arena_allocate_item(&arena, byte);
        ASSERT(next == first + 1);

        arena_destroy(&arena);
    }

    {
        LinearArena arena = arena_create(default_allocator, 1024);
        byte *first = arena_allocate_array(&arena, byte, 256);

        arena_pop_to(&arena, first);
        byte *next = arena_allocate_array(&arena, byte, 256);
        ASSERT(next == first);

        arena_destroy(&arena);
    }

    {
        LinearArena arena = arena_create(default_allocator, 1024);
        byte *first = arena_allocate_array(&arena, byte, 256);
        byte *second = arena_allocate_array(&arena, byte, 1000);

        arena_pop_to(&arena, second);

        byte *third = arena_allocate_array(&arena, byte, 10);
        ASSERT(third == second);

        arena_pop_to(&arena, first);
        byte *fourth = arena_allocate_array(&arena, byte, 10);
        ASSERT(fourth == first);

        arena_destroy(&arena);
    }
}

static void tests_string()
{
    LinearArena arena = arena_create(default_allocator, MB(1));
    Allocator allocator = arena_create_allocator(&arena);

    {

        String a = str_literal("hello ");
        String b = str_literal("world");

        ASSERT(!str_equal(a, b));

        String c = str_concat(a, b, allocator);
        ASSERT(str_equal(c, str_literal("hello world")));
        ASSERT(c.data != a.data);
        ASSERT(c.data != b.data);

    }

    {
        String lit = str_literal("abcdef");
        String copy = str_copy(lit, allocator);

        ASSERT(str_equal(lit, copy));
        ASSERT(lit.data != copy.data);

        arena_allocate_item(&arena, byte); // To prevent from extending in place

        String terminated = str_null_terminate(copy, allocator);
        ASSERT(terminated.data != copy.data);
        ASSERT(terminated.data[terminated.length] == '\0');
        ASSERT(str_equal(copy, terminated));
    }

    {
        String lit = str_literal("abcdef");
        String copy = str_copy(lit, allocator);

        String terminated = str_null_terminate(copy, allocator);

        ASSERT(terminated.data == copy.data);
        ASSERT(terminated.data[terminated.length] == '\0');
        ASSERT(str_equal(copy, terminated));
    }

    {
        // Extend in place
        LinearArena ar = arena_create(default_allocator, 100);
        Allocator alloc = arena_create_allocator(&ar);
        String a = str_copy(str_literal("abc"), alloc);
        String b = str_literal("def");

        String c = str_concat(a, b, alloc);
        ASSERT(c.data == a.data);

        arena_destroy(&ar);
    }

    {
        // Fail to extend in place
        LinearArena ar = arena_create(default_allocator, 100);
        Allocator alloc = arena_create_allocator(&ar);
        String a = str_copy(str_literal("abc"), alloc);

        allocate_item(alloc, byte);

        String b = str_literal("def");

        String c = str_concat(a, b, alloc);
        ASSERT(str_equal(c, str_literal("abcdef")));

        ASSERT(c.data != a.data);

        arena_destroy(&ar);
    }

    {
        ASSERT(str_starts_with(str_literal("abc"), str_literal("a")));
        ASSERT(str_starts_with(str_literal("abc"), str_literal("ab")));
        ASSERT(str_starts_with(str_literal("abc"), str_literal("abc")));
        ASSERT(!str_starts_with(str_literal("abc"), str_literal("abcd")));
        ASSERT(!str_starts_with(str_literal("abc"), str_literal("b")));

    }

    {
        String a = str_literal("abac");
        ASSERT(str_find_last_occurence(a, str_literal("a")) == 2);
        ASSERT(str_find_last_occurence(a, str_literal("c")) == 3);
        ASSERT(str_find_last_occurence(a, str_literal("b")) == 1);
        ASSERT(str_find_last_occurence(a, str_literal("d")) == -1);

        ASSERT(str_find_last_occurence(a, str_literal("ab")) == 0);
        ASSERT(str_find_last_occurence(a, str_literal("ba")) == 1);
        ASSERT(str_find_last_occurence(a, str_literal("bac")) == 1);
        ASSERT(str_find_last_occurence(a, str_literal("ac")) == 2);
        ASSERT(str_find_last_occurence(a, str_literal("acb")) == -1);
    }

    {
        String a = str_literal("abac");
        ASSERT(str_find_first_occurence(a, str_literal("a")) == 0);
        ASSERT(str_find_first_occurence(a, str_literal("b")) == 1);
        ASSERT(str_find_first_occurence(a, str_literal("c")) == 3);
        ASSERT(str_find_first_occurence(a, str_literal("d")) == -1);

        ASSERT(str_find_first_occurence(a, str_literal("ab")) == 0);
        ASSERT(str_find_first_occurence(a, str_literal("ba")) == 1);
        ASSERT(str_find_first_occurence(a, str_literal("bac")) == 1);
        ASSERT(str_find_first_occurence(a, str_literal("ac")) == 2);
    }

    {
        String a = str_literal("abcdef");
        String span = str_create_span(a, 1, 3);

        ASSERT(str_equal(span, str_literal("bcd")));
    }

    {
        String a = str_literal("abcdef");
        String span = str_create_span(a, 2, 4);

        ASSERT(str_equal(span, str_literal("cdef")));
    }

    {
        String a = str_literal("a");
        String span = str_create_span(a, 0, 1);

        ASSERT(str_equal(span, str_literal("a")));
    }

    {
        String a = str_literal("ab");
        String span = str_create_span(a, 1, 1);

        ASSERT(str_equal(span, str_literal("b")));
    }

    {
        String str = str_literal("abcdef");
        ASSERT(str_ends_with(str, str_literal("f")));
        ASSERT(str_ends_with(str, str_literal("ef")));
        ASSERT(str_ends_with(str, str_literal("def")));
        ASSERT(str_ends_with(str, str_literal("cdef")));
        ASSERT(str_ends_with(str, str_literal("bcdef")));
        ASSERT(str_ends_with(str, str_literal("abcdef")));

        ASSERT(!str_ends_with(str, str_literal("a")));
        ASSERT(!str_ends_with(str, str_literal("e")));
        ASSERT(!str_ends_with(str, str_literal("aabcdef")));
    }

    {
        ASSERT(str_get_common_prefix_length(str_literal("abcdef"), str_literal("a")) == 1);
        ASSERT(str_get_common_prefix_length(str_literal("abcdef"), str_literal("ab")) == 2);
        ASSERT(str_get_common_prefix_length(str_literal("abcdef"), str_literal("abc")) == 3);
        ASSERT(str_get_common_prefix_length(str_literal("abcdef"), str_literal("abd")) == 2);
        ASSERT(str_get_common_prefix_length(str_literal("abcdef"), str_literal("abcdef")) == 6);

        ASSERT(str_get_common_prefix_length(str_literal("abcdef"), str_literal("b")) == 0);

    }



    arena_destroy(&arena);
}

static void tests_utils()
{
    {
        char str[] = "hello";
        ASSERT(ARRAY_COUNT(str) == 6);
    }

    ASSERT(is_pow2(2));
    ASSERT(is_pow2(4));
    ASSERT(is_pow2(8));
    ASSERT(is_pow2(16));

    ASSERT(!is_pow2(0));
    ASSERT(multiply_overflows_ssize(S64_MAX, 2));
    ASSERT(!multiply_overflows_ssize(S64_MAX, 1));
    ASSERT(!multiply_overflows_ssize(S64_MAX, 0));
    ASSERT(!multiply_overflows_ssize(0, 2));

    ASSERT(add_overflows_ssize(S_SIZE_MAX, 1));
    ASSERT(add_overflows_ssize(S_SIZE_MAX - 1, 2));
    ASSERT(!add_overflows_ssize(S_SIZE_MAX, 0));
    ASSERT(!add_overflows_ssize(0, 1));

    ASSERT(S_SIZE_MAX == S64_MAX);

    ASSERT(align_s64(5, 8) == 8);
    ASSERT(align_s64(8, 8) == 8);
    ASSERT(align_s64(16, 8) == 16);
    ASSERT(align_s64(17, 8) == 24);
    ASSERT(align_s64(0, 8) == 0);
    ASSERT(align_s64(4, 2) == 4);

    ASSERT(is_aligned(8, 8));
    ASSERT(is_aligned(8, 4));

    ASSERT(!is_aligned(4, 8));
    ASSERT(!is_aligned(5, 4));
    ASSERT(is_aligned(5, 1));

    ASSERT(MAX(1, 2) == 2);
    ASSERT(MAX(2, 1) == 2);
    ASSERT(MAX(-1, 10) == 10);

    {
        s32 *ptr = (s32 *)123;
        ASSERT((s64)byte_offset(ptr, 5) == 128);
        ASSERT((s64)byte_offset(ptr, -3) == 120);
    }
}

static void tests_file()
{
    {
        LinearArena arena = arena_create(default_allocator, MB(1));
        ssize file_size = os_get_file_size(str_literal(__FILE__), arena);

        ASSERT(file_size != -1);

        arena_destroy(&arena);
    }

    {
        // Check that scratch arena works
        LinearArena arena = arena_create(default_allocator, 1024);
        arena_allocate_array(&arena, byte, 1022);
        byte *last = arena_allocate_array(&arena, byte, 1);


        ssize file_size = os_get_file_size(str_literal(__FILE__), arena);
        ASSERT(file_size != -1);

        byte *next = arena_allocate_item(&arena, byte);
        ASSERT(next == last + 1);

        arena_destroy(&arena);
    }

    {
        LinearArena arena = arena_create(default_allocator, MB(1));
        ReadFileResult contents = os_read_entire_file(str_literal(__FILE__), arena_create_allocator(&arena));
        ASSERT(contents.file_data);
        ASSERT(contents.file_size != -1);

        arena_destroy(&arena);
    }
}

static void tests_list()
{
    typedef struct s32_node {
        LIST_LINKS(s32_node);
        s32 data;
    } s32_node;

    DEFINE_LIST(s32_node, List_s32);

    LinearArena arena = arena_create(default_allocator, MB(4));

    {
        List_s32 list = {0};
        ASSERT(list_is_empty(&list));

        s32_node *node1 = arena_allocate_item(&arena, s32_node);
        list_push_back(&list, node1);
        ASSERT(list_head(&list) == node1);
        ASSERT(list_tail(&list) == node1);
        ASSERT(!list_is_empty(&list));
        ASSERT(node1->next == 0);
        ASSERT(node1->prev == 0);

        s32_node *node2 = arena_allocate_item(&arena, s32_node);
        list_push_back(&list, node2);
        ASSERT(list_head(&list) == node1);
        ASSERT(list_tail(&list) == node2);
        ASSERT(!list_is_empty(&list));

        ASSERT(node1->next == node2);
        ASSERT(node1->prev == 0);
        ASSERT(node2->next == 0);
        ASSERT(node2->prev == node1);

        list_remove(&list, node1);
        ASSERT(list_head(&list) == node2);
        ASSERT(list_tail(&list) == node2);

        ASSERT(node2->next == 0);
        ASSERT(node2->prev == 0);

        list_remove(&list, node2);
        ASSERT(list_is_empty(&list));
    }

    {
        List_s32 list = {0};
        s32_node *node1 = arena_allocate_item(&arena, s32_node);
        list_push_back(&list, node1);
        s32_node *node2 = arena_allocate_item(&arena, s32_node);
        list_push_back(&list, node2);

        list_remove(&list, node2);
        ASSERT(list_head(&list) == node1);
        ASSERT(list_tail(&list) == node1);
    }

    {
        List_s32 list = {0};
        s32_node *node1 = arena_allocate_item(&arena, s32_node);
        node1->data = 0;
        list_push_front(&list, node1);
        ASSERT(list_head(&list) == node1);
        ASSERT(list_tail(&list) == node1);

        s32_node *node2 = arena_allocate_item(&arena, s32_node);
        node2->data = 1;
        list_push_front(&list, node2);
        ASSERT(list_head(&list) == node2);
        ASSERT(list_tail(&list) == node1);

        s32_node *node3 = arena_allocate_item(&arena, s32_node);
        node3->data = 2;
        list_push_front(&list, node3);
        ASSERT(list_head(&list) == node3);
        ASSERT(list_tail(&list) == node1);

        s32 counter = 2;
        for (s32_node *node = list_head(&list); node; node = list_next(node)) {
            ASSERT(node->data == counter);
            counter--;
        }

        counter = 0;
        for (s32_node *node = list_tail(&list); node; node = list_prev(node)) {
            ASSERT(node->data == counter);
            counter++;
        }
    }


    {
        List_s32 list = {0};
        s32_node *node1 = arena_allocate_item(&arena, s32_node);
        list_push_back(&list, node1);

        s32_node *node2 = arena_allocate_item(&arena, s32_node);
        list_push_back(&list, node2);

        s32_node *node3 = arena_allocate_item(&arena, s32_node);
        list_push_back(&list, node3);

        list_remove(&list, node2);
        ASSERT(node1->prev == 0);
        ASSERT(node1->next == node3);

        ASSERT(node3->prev == node1);
        ASSERT(node3->next == 0);

        list_pop_head(&list);
        ASSERT(list_head(&list) == node3);

        list_pop_tail(&list);
        ASSERT(list_is_empty(&list));
    }

    {
        List_s32 list_a = {0};
        s32_node *node1 = arena_allocate_item(&arena, s32_node);
        list_insert_after(&list_a, node1, list_a.tail);
        ASSERT(list_a.head == node1);
        ASSERT(list_a.tail == node1);
    }

    {
        List_s32 list = {0};
        s32_node *node1 = arena_allocate_item(&arena, s32_node);
        list_push_back(&list, node1);

        s32_node *node2 = arena_allocate_item(&arena, s32_node);
        list_insert_after(&list, node2, node1);
        ASSERT(list_head(&list) == node1);
        ASSERT(list_tail(&list) == node2);
    }

    arena_destroy(&arena);
}

void tests_path()
{
    LinearArena arena = arena_create(default_allocator, MB(2));
    Allocator alloc = arena_create_allocator(&arena);

    {
        ASSERT(os_path_is_absolute(str_literal("/foo")));
        ASSERT(os_path_is_absolute(str_literal("/")));
        ASSERT(!os_path_is_absolute(str_literal("~/code")));
        ASSERT(!os_path_is_absolute(str_literal("./")));
        ASSERT(!os_path_is_absolute(str_literal("../foo")));
    }

    {
        ASSERT(str_equal(os_get_parent_path(str_literal("/home/foo"), alloc), str_literal("/home")));
        ASSERT(str_equal(os_get_parent_path(str_literal("/home/foo/a.out"), alloc), str_literal("/home/foo")));
        ASSERT(str_equal(os_get_parent_path(str_literal("/home/foo/"), alloc), str_literal("/home/foo")));
        ASSERT(str_equal(os_get_parent_path(str_literal("/"), alloc), str_literal("/")));
    }



    arena_destroy(&arena);
}

static void run_tests()
{
    tests_arena();
    tests_string();
    tests_utils();
    tests_file();
    tests_list();
    tests_path();
}
