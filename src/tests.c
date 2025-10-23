#include "base/free_list_arena.h"
#include "base/rectangle.h"
#include "base/rgba.h"
#include "base/ring_buffer.h"
#include "base/sl_list.h"
#include "base/typedefs.h"
#include "base/string8.h"
#include "base/linear_arena.h"
#include "base/allocator.h"
#include "base/utils.h"
#include "base/matrix.h"
#include "base/image.h"
#include "base/free_list_arena.h"
#include "base/list.h"
#include "base/utils.h"
#include "base/maths.h"
#include "base/format.h"
#include "platform.h"
#include <stdint.h>

static void tests_arena()
{
    {
        LinearArena arena = la_create(default_allocator, 1024);

        s32 *ptr1 = la_allocate_item(&arena, s32);
        s32 *ptr2 = la_allocate_item(&arena, s32);

        ASSERT(la_get_memory_usage(&arena) == 8);

        *ptr1 = 123;
        *ptr2 = 456;

        ASSERT(*ptr1 == 123);
        ASSERT(is_aligned((ssize)ptr1, ALIGNOF(s32)));
        ASSERT(is_aligned((ssize)ptr2, ALIGNOF(s32)));

        *ptr1 = 0;

        ASSERT(*ptr2 == 456);

        la_reset(&arena);
        s32 *p = la_allocate_item(&arena, s32);
        ASSERT(p == ptr1);


        la_destroy(&arena);
    }

    {
        const s32 size = 1024;
        LinearArena arena = la_create(default_allocator, size);

        byte *arr1 = la_allocate_array(&arena, byte, size / 2);
        byte *arr2 = la_allocate_array(&arena, byte, size / 2);

        ASSERT(arr1);
        ASSERT(arr2);
        ASSERT(la_get_memory_usage(&arena) == size);

        la_destroy(&arena);
    }

    {
        const s32 size = 1024;
        LinearArena arena = la_create(default_allocator, size);

        byte *arr1 = la_allocate_array(&arena, byte, size / 2);
        byte *arr2 = la_allocate_array(&arena, byte, size / 2 + 1);

        ASSERT(arr1);
        ASSERT(arr2);
        ASSERT(la_get_memory_usage(&arena) == size + size / 2 + 1);

        byte *arr3 = la_allocate_array(&arena, byte, 14);
        ASSERT(arr3);

        la_reset(&arena);
        byte *ptr1 = la_allocate_item(&arena, byte);
        ASSERT(ptr1 == arr1);

        byte *ptr2 = la_allocate_array(&arena, byte, size);
        ASSERT(ptr2 == arr2);

        la_destroy(&arena);
    }

    {
        const s32 size = 1024;
        LinearArena arena = la_create(default_allocator, size);

        byte *first = la_allocate_item(&arena, byte);
        byte *arr1 = la_allocate(&arena, 1, 4, 32);

        ASSERT(first != (arr1 + 1));
        ASSERT(is_aligned((ssize)arr1, 32));

        byte *arr2 = la_allocate(&arena, 1, 4, 256);
        ASSERT(is_aligned((ssize)arr2, 256));

        la_destroy(&arena);
    }

    {
        const s32 size = 1024;
        LinearArena arena = la_create(default_allocator, size);

        byte *arr1 = la_allocate(&arena, 1, size, 256);
        ASSERT(is_aligned((ssize)arr1, 256));

        la_destroy(&arena);
    }

    {
        // Zero out new allocations
        LinearArena arena = la_create(default_allocator, 1024);

        byte *arr1 = la_allocate_array(&arena, byte, 16);
        memset(arr1, 0xFF, 16);

        la_reset(&arena);
        byte *arr2 = la_allocate_array(&arena, byte, 16);
        ASSERT(arr2 == arr1);

        for (s32 i = 0; i < 16; ++i) {
            ASSERT(arr2[i] == 0);
        }

        la_destroy(&arena);
    }

    {
        LinearArena arena = la_create(default_allocator, 1024);
        byte *first = la_allocate_array(&arena, byte, 256);

        la_pop_to(&arena, first);
        byte *next = la_allocate_array(&arena, byte, 256);
        ASSERT(next == first);

        la_destroy(&arena);
    }

    {
        LinearArena arena = la_create(default_allocator, 1024);
        byte *first = la_allocate_array(&arena, byte, 256);
        byte *second = la_allocate_array(&arena, byte, 1000);

        la_pop_to(&arena, second);

        byte *third = la_allocate_array(&arena, byte, 10);
        ASSERT(third == second);

        la_pop_to(&arena, first);
        byte *fourth = la_allocate_array(&arena, byte, 10);
        ASSERT(fourth == first);

        la_destroy(&arena);
    }
}

static void tests_string()
{
    LinearArena arena = la_create(default_allocator, MB(1));
    Allocator allocator = la_allocator(&arena);

    {

        String a = str_lit("hello ");
        String b = str_lit("world");

        ASSERT(!str_equal(a, b));

        String c = str_concat(a, b, allocator);
        ASSERT(str_equal(c, str_lit("hello world")));
        ASSERT(c.data != a.data);
        ASSERT(c.data != b.data);

    }

    {
        String lit = str_lit("abcdef");
        String copy = str_copy(lit, allocator);

        ASSERT(str_equal(lit, copy));
        ASSERT(lit.data != copy.data);

        la_allocate_item(&arena, byte); // To prevent from extending in place

        String terminated = str_null_terminate(copy, allocator);
        ASSERT(terminated.data != copy.data);
        ASSERT(terminated.data[terminated.length] == '\0');
        ASSERT(str_equal(copy, terminated));
    }

    {
        String lit = str_lit("abcdef");
        String copy = str_copy(lit, allocator);

        String terminated = str_null_terminate(copy, allocator);

        ASSERT(terminated.data[terminated.length] == '\0');
        ASSERT(str_equal(copy, terminated));
    }

    {
        ASSERT(str_starts_with(str_lit("abc"), str_lit("a")));
        ASSERT(str_starts_with(str_lit("abc"), str_lit("ab")));
        ASSERT(str_starts_with(str_lit("abc"), str_lit("abc")));
        ASSERT(!str_starts_with(str_lit("abc"), str_lit("abcd")));
        ASSERT(!str_starts_with(str_lit("abc"), str_lit("b")));

    }

    {
        String a = str_lit("abac");
        ASSERT(str_find_last_occurence(a, str_lit("a")) == 2);
        ASSERT(str_find_last_occurence(a, str_lit("c")) == 3);
        ASSERT(str_find_last_occurence(a, str_lit("b")) == 1);
        ASSERT(str_find_last_occurence(a, str_lit("d")) == -1);

        ASSERT(str_find_last_occurence(a, str_lit("ab")) == 0);
        ASSERT(str_find_last_occurence(a, str_lit("ba")) == 1);
        ASSERT(str_find_last_occurence(a, str_lit("bac")) == 1);
        ASSERT(str_find_last_occurence(a, str_lit("ac")) == 2);
        ASSERT(str_find_last_occurence(a, str_lit("acb")) == -1);
    }

    {
        String a = str_lit("abac");
        ASSERT(str_find_first_occurence(a, str_lit("a")) == 0);
        ASSERT(str_find_first_occurence(a, str_lit("b")) == 1);
        ASSERT(str_find_first_occurence(a, str_lit("c")) == 3);
        ASSERT(str_find_first_occurence(a, str_lit("d")) == -1);

        ASSERT(str_find_first_occurence(a, str_lit("ab")) == 0);
        ASSERT(str_find_first_occurence(a, str_lit("ba")) == 1);
        ASSERT(str_find_first_occurence(a, str_lit("bac")) == 1);
        ASSERT(str_find_first_occurence(a, str_lit("ac")) == 2);
    }

    {
        String a = str_lit("abcdef");
        String span = str_create_span(a, 1, 3);

        ASSERT(str_equal(span, str_lit("bcd")));
    }

    {
        String a = str_lit("abcdef");
        String span = str_create_span(a, 2, 4);

        ASSERT(str_equal(span, str_lit("cdef")));
    }

    {
        String a = str_lit("a");
        String span = str_create_span(a, 0, 1);

        ASSERT(str_equal(span, str_lit("a")));
    }

    {
        String a = str_lit("ab");
        String span = str_create_span(a, 1, 1);

        ASSERT(str_equal(span, str_lit("b")));
    }

    {
        String str = str_lit("abcdef");
        ASSERT(str_ends_with(str, str_lit("f")));
        ASSERT(str_ends_with(str, str_lit("ef")));
        ASSERT(str_ends_with(str, str_lit("def")));
        ASSERT(str_ends_with(str, str_lit("cdef")));
        ASSERT(str_ends_with(str, str_lit("bcdef")));
        ASSERT(str_ends_with(str, str_lit("abcdef")));

        ASSERT(!str_ends_with(str, str_lit("a")));
        ASSERT(!str_ends_with(str, str_lit("e")));
        ASSERT(!str_ends_with(str, str_lit("aabcdef")));
    }

    {
        ASSERT(str_get_common_prefix_length(str_lit("abcdef"), str_lit("a")) == 1);
        ASSERT(str_get_common_prefix_length(str_lit("abcdef"), str_lit("ab")) == 2);
        ASSERT(str_get_common_prefix_length(str_lit("abcdef"), str_lit("abc")) == 3);
        ASSERT(str_get_common_prefix_length(str_lit("abcdef"), str_lit("abd")) == 2);
        ASSERT(str_get_common_prefix_length(str_lit("abcdef"), str_lit("abcdef")) == 6);

        ASSERT(str_get_common_prefix_length(str_lit("abcdef"), str_lit("b")) == 0);

    }



    la_destroy(&arena);
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

    ASSERT(align(5, 8) == 8);
    ASSERT(align(8, 8) == 8);
    ASSERT(align(16, 8) == 16);
    ASSERT(align(17, 8) == 24);
    ASSERT(align(0, 8) == 0);
    ASSERT(align(4, 2) == 4);

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

    {
        ASSERT(bit_span(0xF, 0, 2) == 0x3);
        ASSERT(bit_span(0xFF0FF, 4, 12) == 0xF0F);
        ASSERT(bit_span(0x0, 0, 64) == 0);
        ASSERT(bit_span(USIZE_MAX, 0, 64) == USIZE_MAX);
        ASSERT(bit_span(USIZE_MAX, 0, 63) == (USIZE_MAX >> 1));
    }

    ASSERT(CLAMP(0, 1, 2) == 1);
    ASSERT(CLAMP(-1, 0, 2) == 0);
    ASSERT(CLAMP(-1, -2, 2) == -1);
    ASSERT(CLAMP(4, -2, 2) == 2);
}


static void tests_file()
{
    {
        LinearArena arena = la_create(default_allocator, MB(1));
        ssize file_size = platform_get_file_size(str_lit(__FILE__), &arena);

        ASSERT(file_size != -1);

        la_destroy(&arena);
    }

    {
        // Check that scratch arena works
        LinearArena arena = la_create(default_allocator, 1024);
        la_allocate_array(&arena, byte, 1022);

        ssize file_size = platform_get_file_size(str_lit(__FILE__), &arena);
        ASSERT(file_size != -1);

        la_destroy(&arena);
    }

    {
        LinearArena arena = la_create(default_allocator, MB(1));
        Span contents = platform_read_entire_file(str_lit(__FILE__), la_allocator(&arena), &arena);
        ASSERT(contents.data);
        ASSERT(contents.size != -1);

        la_destroy(&arena);
    }

    LinearArena scratch = la_create(default_allocator, MB(1));
    {
        ASSERT(platform_file_exists(str_lit(__FILE__), &scratch));
        ASSERT(!platform_file_exists(str_lit("/fjldksfjsdlköfj"), &scratch));
    }

    la_destroy(&scratch);
}

static void tests_list()
{
    typedef struct s32_node {
        LIST_LINKS(s32_node);
        s32 data;
    } s32_node;

    DEFINE_LIST(s32_node, List_s32);

    LinearArena arena = la_create(default_allocator, MB(4));

    {
        List_s32 list = {0};
        ASSERT(list_is_empty(&list));

        s32_node *node1 = la_allocate_item(&arena, s32_node);
        list_push_back(&list, node1);
        ASSERT(list_head(&list) == node1);
        ASSERT(list_tail(&list) == node1);
        ASSERT(!list_is_empty(&list));
        ASSERT(node1->next == 0);
        ASSERT(node1->prev == 0);

        s32_node *node2 = la_allocate_item(&arena, s32_node);
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
        s32_node *node1 = la_allocate_item(&arena, s32_node);
        list_push_back(&list, node1);
        s32_node *node2 = la_allocate_item(&arena, s32_node);
        list_push_back(&list, node2);

        list_remove(&list, node2);
        ASSERT(list_head(&list) == node1);
        ASSERT(list_tail(&list) == node1);
    }

    {
        List_s32 list = {0};
        s32_node *node1 = la_allocate_item(&arena, s32_node);
        node1->data = 0;
        list_push_front(&list, node1);
        ASSERT(list_head(&list) == node1);
        ASSERT(list_tail(&list) == node1);

        s32_node *node2 = la_allocate_item(&arena, s32_node);
        node2->data = 1;
        list_push_front(&list, node2);
        ASSERT(list_head(&list) == node2);
        ASSERT(list_tail(&list) == node1);

        s32_node *node3 = la_allocate_item(&arena, s32_node);
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
        s32_node *node1 = la_allocate_item(&arena, s32_node);
        list_push_back(&list, node1);

        s32_node *node2 = la_allocate_item(&arena, s32_node);
        list_push_back(&list, node2);

        s32_node *node3 = la_allocate_item(&arena, s32_node);
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
        s32_node *node1 = la_allocate_item(&arena, s32_node);
        list_insert_after(&list_a, node1, list_a.tail);
        ASSERT(list_a.head == node1);
        ASSERT(list_a.tail == node1);
	ASSERT(!node1->next);
	ASSERT(!node1->prev);
    }

    {
        List_s32 list = {0};
        s32_node *node1 = la_allocate_item(&arena, s32_node);
        list_push_back(&list, node1);

        s32_node *node2 = la_allocate_item(&arena, s32_node);
        list_insert_after(&list, node2, node1);
        ASSERT(list_head(&list) == node1);
        ASSERT(list_tail(&list) == node2);

	ASSERT(node1->next == node2);
	ASSERT(node1->prev == 0);

	ASSERT(node2->prev == node1);
	ASSERT(node2->next == 0);
    }

    {
        List_s32 list = {0};
        s32_node *node1 = la_allocate_item(&arena, s32_node);
        list_push_back(&list, node1);

        s32_node *node2 = la_allocate_item(&arena, s32_node);
        list_push_back(&list, node2);

        s32_node *node3 = la_allocate_item(&arena, s32_node);
        list_insert_after(&list, node3, node1);

        ASSERT(node1->next == node3);
        ASSERT(node1->prev == 0);

        ASSERT(node3->prev == node1);
        ASSERT(node3->next == node2);

        ASSERT(node2->prev == node3);
        ASSERT(node2->next == 0);
    }

    {
        List_s32 list = {0};
        s32_node *node1 = la_allocate_item(&arena, s32_node);
	node1->data = 0;
        list_push_back(&list, node1);

        s32_node *node2 = la_allocate_item(&arena, s32_node);
	node2->data = 2;
        list_push_back(&list, node2);

        s32_node *node3 = la_allocate_item(&arena, s32_node);
        list_insert_after(&list, node3, node1);
	node3->data = 1;

	s32 counter = 2;
	for (s32_node *curr = list_tail(&list); curr; curr = list_next(curr)) {
	    ASSERT(curr->data == counter);
	    --counter;
	}
    }

    {
        List_s32 list = {0};
        s32_node *node1 = la_allocate_item(&arena, s32_node);
        list_push_back(&list, node1);

        s32_node *node2 = la_allocate_item(&arena, s32_node);
        list_push_back(&list, node2);

        s32_node *node3 = la_allocate_item(&arena, s32_node);
        list_insert_after(&list, node3, node2);

	ASSERT(node1->next == node2);
	ASSERT(node1->prev == 0);

	ASSERT(node2->next == node3);
	ASSERT(node2->prev == node1);

	ASSERT(node3->next == 0);
	ASSERT(node3->prev == node2);

	ASSERT(list_head(&list) == node1);
	ASSERT(list_tail(&list) == node3);
    }


    {
        List_s32 list = {0};
        s32_node *node1 = la_allocate_item(&arena, s32_node);
        list_push_back(&list, node1);

        s32_node *node2 = la_allocate_item(&arena, s32_node);
        list_push_back(&list, node2);

        s32_node *node3 = la_allocate_item(&arena, s32_node);
        list_insert_before(&list, node3, node1);

	ASSERT(list_head(&list) == node3);

	ASSERT(list_next(node1) == node2);
	ASSERT(list_prev(node1) == node3);

	ASSERT(list_prev(node3) == 0);
	ASSERT(list_next(node3) == node1);

	ASSERT(list_prev(node2) == node1);
	ASSERT(list_next(node2) == 0);
    }

    {
        List_s32 list = {0};
        s32_node *node1 = la_allocate_item(&arena, s32_node);
        list_push_back(&list, node1);

        s32_node *node2 = la_allocate_item(&arena, s32_node);
        list_push_back(&list, node2);

        s32_node *node3 = la_allocate_item(&arena, s32_node);
        list_insert_before(&list, node3, node2);

	ASSERT(list_head(&list) == node1);

	ASSERT(list_prev(node1) == 0);
	ASSERT(list_next(node1) == node3);

	ASSERT(list_prev(node3) == node1);
	ASSERT(list_next(node3) == node2);

	ASSERT(list_prev(node2) == node3);
	ASSERT(list_next(node2) == 0);
    }

    la_destroy(&arena);
}

void tests_path()
{
    LinearArena arena = la_create(default_allocator, MB(2));
    Allocator alloc = la_allocator(&arena);

    {
        ASSERT(platform_path_is_absolute(str_lit("/foo")));
        ASSERT(platform_path_is_absolute(str_lit("/")));
        ASSERT(!platform_path_is_absolute(str_lit("~/code")));
        ASSERT(!platform_path_is_absolute(str_lit("./")));
        ASSERT(!platform_path_is_absolute(str_lit("../foo")));
    }

    {
        ASSERT(str_equal(platform_get_parent_path(str_lit("/home/foo"), alloc, &arena), str_lit("/home")));
        ASSERT(str_equal(platform_get_parent_path(str_lit("/home/foo/a.out"), alloc, &arena), str_lit("/home/foo")));
        ASSERT(str_equal(platform_get_parent_path(str_lit("/home/foo/"), alloc, &arena), str_lit("/home/foo")));
        ASSERT(str_equal(platform_get_parent_path(str_lit("/"), alloc, &arena), str_lit("/")));
    }

    {
        ASSERT(str_equal(platform_get_filename(str_lit("/home/bar.txt")), str_lit("bar.txt")));
        ASSERT(str_equal(platform_get_filename(str_lit("/home/foo/bar.txt")), str_lit("bar.txt")));
        ASSERT(str_equal(platform_get_filename(str_lit("/home/foo")), str_lit("foo")));
        ASSERT(str_equal(platform_get_filename(str_lit("foo.txt")), str_lit("foo.txt")));
    }

    la_destroy(&arena);
}

static void tests_free_list()
{
    {
	FreeListArena fl = fl_create(default_allocator, 1024);

	ASSERT(fl_get_available_memory(&fl) == 1024);
        ASSERT(fl_get_memory_usage(&fl) == 0);

	byte *p = fl_allocate(&fl, 1, 100, 4);

	ASSERT((fl_get_memory_usage(&fl) >= 100) && (fl_get_memory_usage(&fl) < 200));

	for (ssize i = 0; i < 100; ++i) {
	    ASSERT(p[i] == 0);
	}

	byte *p2 = fl_allocate(&fl, 1, 880, 4);

	ASSERT((fl_get_memory_usage(&fl) >= 980) && (fl_get_memory_usage(&fl) <= 1024));
        ASSERT((fl_get_available_memory(&fl) == 0));

	for (ssize i = 0; i < 880; ++i) {
	    ASSERT(p2[i] == 0);
	}

	fl_deallocate(&fl, p);

	ASSERT((fl_get_memory_usage(&fl) >= 880) && (fl_get_memory_usage(&fl) < 980));

	fl_deallocate(&fl, p2);

	ASSERT(fl_get_available_memory(&fl) == 1024);
	ASSERT(fl_get_memory_usage(&fl) == 0);

	fl_destroy(&fl);
    }

    {
	FreeListArena fl = fl_create(default_allocator, 1024);
	byte *p1 = fl_allocate(&fl, 1, 100, 4);

	fl_deallocate(&fl, p1);

	byte *p2 = fl_allocate(&fl, 1, 200, 4);

	ASSERT(p1 == p2);

	fl_destroy(&fl);
    }

    {
	FreeListArena fl = fl_create(default_allocator, MB(32));
	for (s32 i = 1; i <= 16; ++i) {
	    s32 alignment = (s32)pow(2, i);

	    byte *p = fl_allocate(&fl, 1, 100, alignment);

	    ASSERT(is_aligned((ssize)p, alignment));
	}

	fl_destroy(&fl);
    }

    {
	FreeListArena fl = fl_create(default_allocator, 1024);
	byte *p1 = fl_allocate(&fl, 1, 100, 4);
	byte *p2 = fl_allocate(&fl, 1, 100, 4);
	byte *p3 = fl_allocate(&fl, 1, 100, 4);

	ASSERT(fl_get_available_memory(&fl) <= 724);
	ASSERT(fl_get_memory_usage(&fl) >= 300);

	fl_deallocate(&fl, p1);
	fl_deallocate(&fl, p2);
	fl_deallocate(&fl, p3);

	ASSERT(fl_get_available_memory(&fl) == 1024);

	fl_destroy(&fl);
    }

    {
	FreeListArena fl = fl_create(default_allocator, 1024);
	byte *p1 = fl_allocate(&fl, 1, 100, 4);
	byte *p2 = fl_allocate(&fl, 1, 100, 4);
	byte *p3 = fl_allocate(&fl, 1, 100, 4);

	fl_deallocate(&fl, p2);
	fl_deallocate(&fl, p1);
	fl_deallocate(&fl, p3);

	ASSERT(fl_get_available_memory(&fl) == 1024);

	fl_destroy(&fl);
    }

    {
	FreeListArena fl = fl_create(default_allocator, 1024);
	byte *p1 = fl_allocate(&fl, 1, 100, 4);
	byte *p2 = fl_allocate(&fl, 1, 100, 4);
	byte *p3 = fl_allocate(&fl, 1, 100, 4);

	fl_deallocate(&fl, p3);
	fl_deallocate(&fl, p2);
	fl_deallocate(&fl, p1);

	ASSERT(fl_get_available_memory(&fl) == 1024);
        ASSERT(fl_get_memory_usage(&fl) == 0);

	fl_destroy(&fl);
    }

    {
	FreeListArena fl = fl_create(default_allocator, 1024);
	byte *p1 = fl_allocate(&fl, 1, 100, 4);
	byte *p2 = fl_allocate(&fl, 1, 100, 4);
	byte *p3 = fl_allocate(&fl, 1, 100, 4);

	fl_deallocate(&fl, p3);
	fl_deallocate(&fl, p1);
	fl_deallocate(&fl, p2);

	ASSERT(fl_get_available_memory(&fl) == 1024);

	fl_destroy(&fl);
    }

    {
	FreeListArena fl = fl_create(default_allocator, 1024);
	byte *p1 = fl_allocate(&fl, 1, 100, 4);
	byte *p2 = fl_allocate(&fl, 1, 100, 4);
	byte *p3 = fl_allocate(&fl, 1, 100, 4);

	fl_deallocate(&fl, p2);
	fl_deallocate(&fl, p3);
	fl_deallocate(&fl, p1);

	ASSERT(fl_get_available_memory(&fl) == 1024);

	fl_destroy(&fl);
    }

    {
	FreeListArena fl = fl_create(default_allocator, 1024);
	byte *p1 = fl_allocate(&fl, 1, 100, 4);
	byte *p2 = fl_allocate(&fl, 1, 100, 4);
	byte *p3 = fl_allocate(&fl, 1, 100, 4);

	fl_deallocate(&fl, p1);
	fl_deallocate(&fl, p3);
	fl_deallocate(&fl, p2);

	ASSERT(fl_get_available_memory(&fl) == 1024);

	fl_destroy(&fl);
    }

    {
	FreeListArena fl = fl_create(default_allocator, 1023);
	byte *p = fl_allocate(&fl, 1, 100, 4);
	byte *p2 = fl_allocate(&fl, 1, 880, 4);

	fl_deallocate(&fl, p);
	fl_deallocate(&fl, p2);

	fl_destroy(&fl);
    }

    {
	FreeListArena fl = fl_create(default_allocator, 1023);

	ssize size = 100;

	byte *p1 = fl_allocate(&fl, 1, size, 4);
	byte *p2 = fl_allocate(&fl, 1, size, 4);
	ASSERT(p1 < p2);

	memset(p1, 0xFF, (usize)size);
	memset(p2, 0xDE, (usize)size);

	ASSERT(p1[0] == 0xFF);
	ASSERT(p1[size - 1] == 0xFF);

	fl_destroy(&fl);
    }

    {
	FreeListArena fl = fl_create(default_allocator, 1023);

	ssize size = 100;

	byte *p1 = fl_allocate(&fl, 1, size, 4);
	byte *p2 = fl_allocate(&fl, 1, size, 4);
	ASSERT(p1 < p2);

	memset(p2, 0xDE, (usize)size);
	memset(p1, 0xFF, (usize)size);

	ASSERT(p2[0] == 0xDE);
	ASSERT(p2[size - 1] == 0xDE);

	fl_destroy(&fl);
    }

    {
        FreeListArena fl = fl_create(default_allocator, 1024);
        void *p1 = fl_allocate(&fl, 900, 1, 4);
        ASSERT(fl_get_memory_usage(&fl) >= 900 && fl_get_memory_usage(&fl) <= 1024);
        ASSERT(fl_get_available_memory(&fl) <= 124);

        void *p2 = fl_allocate(&fl, 900, 1, 4);
        ASSERT(fl_get_memory_usage(&fl) >= 1800 && fl_get_memory_usage(&fl) <= 2048);

        fl_deallocate(&fl, p1);
        fl_deallocate(&fl, p2);

        ASSERT(fl_get_memory_usage(&fl) == 0);

	fl_destroy(&fl);
    }

    {
        FreeListArena fl = fl_create(default_allocator, 1024);

        byte *p1 = fl_allocate(&fl, 1, 250, 4);
        byte *p2 = fl_allocate(&fl, 1, 250, 4);
        byte *p3 = fl_allocate(&fl, 1, 250, 4);

        byte *p4 = fl_allocate(&fl, 1, 250, 4);
        byte *p5 = fl_allocate(&fl, 1, 250, 4);
        byte *p6 = fl_allocate(&fl, 1, 250, 4);

        fl_deallocate(&fl, p1);
        fl_deallocate(&fl, p3);
        fl_deallocate(&fl, p2);

        ASSERT(fl_get_available_memory(&fl) >= 1024 && fl_get_available_memory(&fl) < 2048);

        fl_deallocate(&fl, p4);
        fl_deallocate(&fl, p6);
        fl_deallocate(&fl, p5);

        ASSERT(fl_get_available_memory(&fl) == 2048);
        ASSERT(fl_get_memory_usage(&fl) == 0);

        fl_destroy(&fl);
    }

    {
	FreeListArena fl = fl_create(default_allocator, 1024);

	byte *ptr1 = fl_allocate(&fl, 100, sizeof(byte), 4);
	byte *ptr2 = fl_allocate(&fl, 100, sizeof(byte), 4);

	memset(ptr1, 0xCD, 100 * sizeof(byte));
	byte *new_ptr1 = fl_reallocate(&fl, ptr1, 200, sizeof(byte), 4);

	ASSERT(new_ptr1 != ptr1);

	for (s32 i = 0; i < 100; ++i) {
	    ASSERT(new_ptr1[i] == 0xCD);
	}

	fl_deallocate(&fl, new_ptr1);
	fl_deallocate(&fl, ptr2);

	ASSERT(fl_get_memory_usage(&fl) == 0);
	ASSERT(fl_get_available_memory(&fl) == 1024);

	fl_destroy(&fl);
    }

    {
        // Allocating larger than entire buffer
	FreeListArena fl = fl_create(default_allocator, 1024);

	byte *ptr1 = fl_allocate(&fl, 1, 100, 4);
	byte *ptr2 = fl_allocate(&fl, 1, 100, 4);

	byte *new_ptr1 = fl_reallocate(&fl, ptr1, 100, 200, 4);

	ASSERT(new_ptr1);
	ASSERT(new_ptr1 != ptr1);

	fl_deallocate(&fl, new_ptr1);
	fl_deallocate(&fl, ptr2);

	ASSERT(fl_get_memory_usage(&fl) == 0);

	fl_destroy(&fl);
    }

    {
	FreeListArena fl = fl_create(default_allocator, 1024);

	for (s32 size = 2; size <= 100; ++size) {
	    byte *ptr1 = fl_allocate(&fl, size, 1, 4);
	    byte *new_ptr1 = fl_reallocate(&fl, ptr1, size / 2, 1, 4);

	    ASSERT(new_ptr1);
	    ASSERT(new_ptr1 == ptr1);

	    fl_deallocate(&fl, new_ptr1);

	    ASSERT(fl_get_memory_usage(&fl) == 0);
	}

	fl_destroy(&fl);
    }

    {
	FreeListArena fl = fl_create(default_allocator, 1024);

	for (s32 size = 100; size <= 500; ++size) {
	    byte *ptr1 = fl_allocate(&fl, 100, 1, 4);
	    byte *new_ptr1 = fl_reallocate(&fl, ptr1, size, 1, 4);

	    ASSERT(new_ptr1);
	    ASSERT(new_ptr1 == ptr1);

	    fl_deallocate(&fl, new_ptr1);

	    ASSERT(fl_get_memory_usage(&fl) == 0);
	}

	fl_destroy(&fl);
    }

    {
	FreeListArena fl = fl_create(default_allocator, 1024);

        byte *ptr1 = fl_alloc_array(&fl, byte, 100);
        memset(ptr1, 0xCD, 100); // To ensure filled with junk data

        byte *ptr2 = fl_alloc_array(&fl, byte, 50);
        fl_alloc_item(&fl, byte);
        byte *ptr3 = fl_realloc_array(&fl, byte, ptr2, 100);
        ASSERT(ptr2 != ptr3);

        for (s32 i = 50; i < 100; ++i) {
            ASSERT(ptr3[i] == 0);
        }

	fl_destroy(&fl);
    }
}

static void tests_sl_list()
{
    typedef struct s32_node {
        s32 data;
        struct s32_node *next;
    } s32_node;

    typedef struct {
        s32_node *head;
        s32_node *tail;
    } s32_list;

    LinearArena arena = la_create(default_allocator, MB(4));

    {
        s32_list list = {0};
        s32_node *node1 = la_allocate_item(&arena, s32_node);
        sl_list_push_front(&list, node1);
        ASSERT(list_head(&list) == node1);
        ASSERT(list_tail(&list) == node1);
        ASSERT(!list_is_empty(&list));
    }

    {
        s32_list list = {0};
        s32_node *node1 = la_allocate_item(&arena, s32_node);
        sl_list_push_back(&list, node1);
        ASSERT(list_head(&list) == node1);
        ASSERT(list_tail(&list) == node1);
        ASSERT(!list_is_empty(&list));
    }

    {
        s32_list list = {0};
        s32_node *node1 = la_allocate_item(&arena, s32_node);
        sl_list_push_front(&list, node1);
        sl_list_pop(&list);
        ASSERT(list_head(&list) == 0);
        ASSERT(list_tail(&list) == 0);
        ASSERT(list_is_empty(&list));
    }

    {
        s32_list list = {0};
        s32_node *node1 = la_allocate_item(&arena, s32_node);
        s32_node *node2 = la_allocate_item(&arena, s32_node);
        sl_list_push_back(&list, node1);
        sl_list_push_back(&list, node2);

        ASSERT(list_head(&list) == node1);
        ASSERT(list_tail(&list) == node2);

        sl_list_pop(&list);
        ASSERT(list_head(&list) == node2);
        ASSERT(list_tail(&list) == node2);
        ASSERT(!list_is_empty(&list));

        sl_list_pop(&list);
        ASSERT(list_is_empty(&list));
    }

    {
        s32_list list = {0};
        s32_node *node1 = la_allocate_item(&arena, s32_node);
        s32_node *node2 = la_allocate_item(&arena, s32_node);

        sl_list_push_front(&list, node1);
        sl_list_push_front(&list, node2);
        ASSERT(list_head(&list) == node2);
        ASSERT(list_tail(&list) == node1);
    }

    {
        s32_list list = {0};
        s32_node *node1 = la_allocate_item(&arena, s32_node);
        node1->data = 0;

        s32_node *node2 = la_allocate_item(&arena, s32_node);
        node2->data = 1;

        s32_node *node3 = la_allocate_item(&arena, s32_node);
        node3->data = 2;

        sl_list_push_back(&list, node1);
        sl_list_push_back(&list, node2);
        sl_list_push_back(&list, node3);

        s32 i = 0;
        for (s32_node *node = list_head(&list); node; node = list_next(node)) {
            ASSERT(node->data == i);
            ++i;
        }
    }

    {
        s32_list list = {0};
        s32_node *node1 = la_allocate_item(&arena, s32_node);
        node1->data = 0;

        s32_node *node2 = la_allocate_item(&arena, s32_node);
        node2->data = 1;

        s32_node *node3 = la_allocate_item(&arena, s32_node);
        node3->data = 2;

        sl_list_push_front(&list, node1);
        sl_list_push_front(&list, node2);
        sl_list_push_front(&list, node3);

        s32 i = 2;
        for (s32_node *node = list_head(&list); node; node = list_next(node)) {
            ASSERT(node->data == i);
            --i;
        }
    }

    la_destroy(&arena);
}

static void tests_format()
{
    LinearArena arena = la_create(default_allocator, MB(2));
    Allocator allocator = la_allocator(&arena);

    {
        s32 n = 0;
        String str = ssize_to_string(n, allocator);
        ASSERT(str_equal(str, str_lit("0")));
    }

    {
        s32 n = 10;
        String str = ssize_to_string(n, allocator);
        ASSERT(str_equal(str, str_lit("10")));
    }

    {
        s32 n = 12345;
        String str = ssize_to_string(n, allocator);
        ASSERT(str_equal(str, str_lit("12345")));
    }

    {
        s32 n = -1;
        String str = ssize_to_string(n, allocator);
        ASSERT(str_equal(str, str_lit("-1")));
    }

    {
        s32 n = -12345;
        String str = ssize_to_string(n, allocator);
        ASSERT(str_equal(str, str_lit("-12345")));
    }

    {
        f32 n = 0.0f;
        s32 prec = 1;
        String str = f32_to_string(n, prec, allocator);
        ASSERT(str_equal(str, str_lit("0.0")));
    }

    {
        f32 n = 0.0f;
        s32 prec = 2;
        String str = f32_to_string(n, prec, allocator);
        ASSERT(str_equal(str, str_lit("0.00")));
    }

    {
        f32 n = 0.001f;
        s32 prec = 2;
        String str = f32_to_string(n, prec, allocator);
        ASSERT(str_equal(str, str_lit("0.00")));
    }

    {
        f32 n = 0.001f;
        s32 prec = 3;
        String str = f32_to_string(n, prec, allocator);
        ASSERT(str_equal(str, str_lit("0.001")));
    }

    {
        f32 n = 3.14159f;
        s32 prec = 5;
        String str = f32_to_string(n, prec, allocator);
        ASSERT(str_equal(str, str_lit("3.14159")));
    }

    {
        f32 n = -3.14159f;
        s32 prec = 5;
        String str = f32_to_string(n, prec, allocator);
        ASSERT(str_equal(str, str_lit("-3.14159")));
    }

    la_destroy(&arena);
}

static void tests_types()
{
    // TODO: make these tests independent of architecture
    /* Signed */
    ASSERT(S_SIZE_MAX == LLONG_MAX);
    ASSERT(S_SIZE_MIN == LLONG_MIN);

    ASSERT(S64_MAX == LLONG_MAX);
    ASSERT(S64_MIN == LLONG_MIN);

    ASSERT(S32_MAX == INT_MAX);
    ASSERT(S32_MIN == INT_MIN);

    ASSERT(S16_MAX == SHRT_MAX);
    ASSERT(S16_MIN == SHRT_MIN);

    ASSERT(S8_MAX == SCHAR_MAX);
    ASSERT(S8_MIN == SCHAR_MIN);

    /* Unsigned */
    ASSERT(USIZE_MAX == SIZE_MAX);
    ASSERT(U64_MAX == ULLONG_MAX);
    ASSERT(U32_MAX == UINT_MAX);
    ASSERT(U16_MAX == USHRT_MAX);
    ASSERT(U8_MAX == UCHAR_MAX);
}


static void tests_ring()
{

    /* Static buffers */
    {
	DEFINE_STATIC_RING_BUFFER(int, StaticIntBuffer, 3);

	{
	    StaticIntBuffer buf;
	    ring_initialize_static(&buf);
	    ASSERT(buf.capacity == 3);
	    ASSERT(ring_is_empty(&buf));
	    ASSERT(!ring_is_full(&buf));
	    ASSERT(ring_length(&buf) == 0);
	}

	{
	    StaticIntBuffer buf;
	    ring_initialize_static(&buf);

	    // TODO: pushing items by value
	    s32 i = 123;
	    ring_push(&buf, &i);

	    ASSERT(ring_length(&buf) == 1);
	    ASSERT(!ring_is_empty(&buf));
	    ASSERT(!ring_is_full(&buf));
	    ASSERT(*ring_at(&buf, 0) == 123);
	    ASSERT(*ring_peek(&buf) == 123);
	    ASSERT(*ring_peek_tail(&buf) == 123);

	    ring_pop(&buf);
	    ASSERT(ring_length(&buf) == 0);
	    ASSERT(ring_is_empty(&buf));
	    ASSERT(!ring_is_full(&buf));
	}

	{
	    StaticIntBuffer buf;
	    ring_initialize_static(&buf);

	    for (s32 i = 0; i < buf.capacity; ++i) {
		ring_push(&buf, &i);
	    }

	    ASSERT(ring_length(&buf) == buf.capacity);
	    ASSERT(ring_is_full(&buf));
	    ASSERT(!ring_is_empty(&buf));

	    for (s32 i = 0; i < ring_length(&buf); ++i) {
		ASSERT(*ring_at(&buf, i) == i);
	    }

	    ring_pop(&buf);
	    ASSERT(ring_length(&buf) == buf.capacity - 1);
	    ASSERT(*ring_peek(&buf) == 1);

	    ring_pop(&buf);
	    ASSERT(*ring_peek(&buf) == 2);

	    ring_pop(&buf);
	    ASSERT(ring_is_empty(&buf));

	    s32 i = 0;
	    ring_push(&buf, &i);

	    ASSERT(ring_length(&buf) == 1);
	}

	{
	    StaticIntBuffer buf;
	    ring_initialize_static(&buf);

	    s32 i = 0;
	    ring_push(&buf, &i);

	    i = 1;
	    ring_push(&buf, &i);

	    ASSERT(ring_length(&buf) == 2);
	    ASSERT(*ring_at(&buf, 0) == 0);
	    ASSERT(*ring_at(&buf, 1) == 1);

	    ring_pop(&buf);
	    ASSERT(ring_length(&buf) == 1);
	    ASSERT(*ring_at(&buf, 0) == 1);
	}

	{
	    StaticIntBuffer buf;
	    ring_initialize_static(&buf);

	    s32 i = 0;
	    ring_push(&buf, &i);

	    i = 1;
	    ring_push(&buf, &i);

	    ring_pop(&buf);

	    i = 2;
	    ring_push(&buf, &i);

	    i = 3;
	    ring_push(&buf, &i);

	    ASSERT(*ring_at(&buf, 0) == 1);
	    ASSERT(*ring_at(&buf, 1) == 2);
	    ASSERT(*ring_at(&buf, 2) == 3);

	    ring_pop(&buf);
	    ring_pop(&buf);
	    ring_pop(&buf);

	    ASSERT(ring_is_empty(&buf));
	}

	{
	    StaticIntBuffer buf;
	    ring_initialize_static(&buf);

	    for (s32 i = 0; i < buf.capacity * 2; ++i) {
		ring_push_overwrite(&buf, &i);
	    }

	    ASSERT(ring_is_full(&buf));

	    for (s32 i = 0; i < ring_length(&buf); ++i) {
		ASSERT(*ring_at(&buf, i) == (i + buf.capacity));
	    }
	}

	{
	    StaticIntBuffer buf;
	    ring_initialize_static(&buf);

	    for (s32 i = 0; i < buf.capacity; ++i) {
		ring_push(&buf, &i);
	    }

	    ASSERT(*ring_peek_tail(&buf) == 2);
	    ring_pop_tail(&buf);

	    ASSERT(*ring_peek_tail(&buf) == 1);
	    ring_pop_tail(&buf);

	    ASSERT(*ring_peek_tail(&buf) == 0);
	    ring_pop_tail(&buf);

	    ASSERT(ring_is_empty(&buf));
	}

	{
	    StaticIntBuffer buf;
	    ring_initialize_static(&buf);

	    for (s32 i = 0; i < buf.capacity; ++i) {
		ring_push(&buf, &i);
	    }

	    ring_swap_remove(&buf, 0);
	    ASSERT(ring_length(&buf) == 2);

	    ASSERT(*ring_at(&buf, 0) == 2);
	    ASSERT(*ring_at(&buf, 1) == 1);
	}

	{
	    StaticIntBuffer buf;
	    ring_initialize_static(&buf);

	    for (s32 i = 0; i < buf.capacity; ++i) {
		ring_push(&buf, &i);
	    }

	    ring_swap_remove(&buf, 1);
	    ASSERT(ring_length(&buf) == 2);

	    ASSERT(*ring_at(&buf, 0) == 0);
	    ASSERT(*ring_at(&buf, 1) == 2);
	}

	{
	    StaticIntBuffer buf;
	    ring_initialize_static(&buf);

	    for (s32 i = 0; i < buf.capacity; ++i) {
		ring_push(&buf, &i);
	    }

	    ring_swap_remove(&buf, 2);
	    ASSERT(ring_length(&buf) == 2);

	    ASSERT(*ring_at(&buf, 0) == 0);
	    ASSERT(*ring_at(&buf, 1) == 1);
	}

	{
	    StaticIntBuffer buf;
	    ring_initialize_static(&buf);

	    for (s32 i = 0; i < buf.capacity; ++i) {
		ring_push(&buf, &i);
	    }

	    ring_swap_remove(&buf, 2);
	    ring_swap_remove(&buf, 1);

	    ASSERT(ring_length(&buf) == 1);
	    ASSERT(*ring_peek(&buf) == 0);

	    ring_swap_remove(&buf, 0);

	    ASSERT(ring_is_empty(&buf));
	}

	{
	    StaticIntBuffer buf;
	    ring_initialize_static(&buf);

	    for (s32 i = 0; i < buf.capacity; ++i) {
		s32 j = i + 10;
		ring_push(&buf, &j);
	    }

	    ASSERT(ring_pop_load(&buf) == 10);
	    ASSERT(ring_pop_load(&buf) == 11);
	    ASSERT(ring_pop_load(&buf) == 12);

	}
    }

    /* Heap buffers */
    {
	LinearArena arena = la_create(default_allocator, 4096);
	Allocator allocator = la_allocator(&arena);

	DEFINE_HEAP_RING_BUFFER(int, HeapIntBuffer);

	{
	    HeapIntBuffer buf;
	    ring_initialize(&buf, 4, allocator);
	    ASSERT(buf.capacity == 4);
	    ASSERT(buf.items);
	    ASSERT(ring_is_empty(&buf));
	}

	la_destroy(&arena);
    }
}

static void tests_maths()
{
    ASSERT(round_to_f32(9.3f, 0.5f) == 9.5f);
    ASSERT(round_to_f32(9.6f, 0.5f) == 9.5f);
    ASSERT(round_to_f32(9.2f, 0.5f) == 9.0f);

    ASSERT(round_to_f32(9.2f, 0.25f) == 9.25f);
    ASSERT(round_to_f32(9.6f, 0.25f) == 9.5f);

    ASSERT(round_to_f32(0.0f, 0.25f) == 0.0f);
    ASSERT(round_to_f32(0.1f, 0.25f) == 0.0f);
    ASSERT(round_to_f32(0.2f, 0.25f) == 0.25f);

    ASSERT(round_to_f32(-0.1f, 0.25f) == 0.0f);
    ASSERT(round_to_f32(-0.2f, 0.25f) == -0.25f);
}

static void run_tests()
{
    tests_arena();
    tests_string();
    tests_utils();
    tests_file();
    tests_list();
    tests_path();
    tests_free_list();
    tests_sl_list();
    tests_format();
    tests_types();
    tests_ring();
    tests_maths();
}
