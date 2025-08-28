#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "base/typedefs.h"
#include "base/string8.h"
#include "base/linear_arena.h"
#include "base/allocator.h"
#include "base/utils.h"
#include "base/linked_list.h"
#include "os/file.h"
#include "os/path.h"

static void tests_arena()
{
    {
        LinearArena arena = arena_create(default_allocator, 1024);

        s32 *ptr1 = arena_allocate_item(&arena, s32);
        s32 *ptr2 = arena_allocate_item(&arena, s32);

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

        arena_destroy(&arena);
    }

    {
        const s32 size = 1024;
        LinearArena arena = arena_create(default_allocator, size);

        byte *arr1 = arena_allocate_array(&arena, byte, size / 2);
        byte *arr2 = arena_allocate_array(&arena, byte, size / 2 + 1);

        ASSERT(arr1);
        ASSERT(arr2);

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
        const bool extended = arena_try_resize(&arena, ptr, 16, 32);
        ASSERT(extended);

        byte *ptr2 = arena_allocate_array(&arena, byte, 16);
        ASSERT(ptr2 == (ptr + 32));

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
        ASSERT(str_find_last_occurence(a, 'a') == 2);
        ASSERT(str_find_last_occurence(a, 'c') == 3);
        ASSERT(str_find_last_occurence(a, 'b') == 1);
        ASSERT(str_find_last_occurence(a, 'd') == -1);
    }

    {
        String a = str_literal("abac");
        ASSERT(str_find_first_occurence(a, 'a') == 0);
        ASSERT(str_find_first_occurence(a, 'b') == 1);
        ASSERT(str_find_first_occurence(a, 'c') == 3);
        ASSERT(str_find_first_occurence(a, 'd') == -1);
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
    typedef struct {
        ListNode node;
        s32 data;
    } Elem;

    {
        Elem elem = {
            .node = {0},
            .data = 123
        };

        Elem *elem_ptr = list_get_item(&elem.node, Elem, node);
        ASSERT(elem_ptr->data == 123);
    }

    {
        List list;
        list_init(&list);

        ASSERT(list_is_empty(&list));

        Elem elem;
        elem.data = 123;

        list_push_back(&list, &elem.node);
        ASSERT(!list_is_empty(&list));

        ListNode *node = list_front(&list);
        ASSERT(list_back(&list) == node);

        list_pop_back(&list);
        ASSERT(list_is_empty(&list));
    }

    {
        List list;
        list_init(&list);

        Elem elem;
        elem.data = 123;

        list_push_front(&list, &elem.node);
        ListNode *back = list_back(&list);
        ASSERT(list_get_item(back, Elem, node)->data == 123);
    }

    {
        List list;
        list_init(&list);

        Elem first;
        first.data = 0;

        Elem second;
        second.data = 1;

        Elem third;
        third.data = 2;

        list_push_back(&list, &first.node);
        list_push_back(&list, &second.node);
        list_push_back(&list, &third.node);

        ASSERT(list_get_item(list_front(&list), Elem, node)->data == 0);
        ASSERT(list_get_item(list_back(&list), Elem, node)->data == 2);

        ListNode *curr = list_begin(&list);
        for (s32 i = 0;; ++i) {
            Elem *item = list_get_item(curr, Elem, node);
            ASSERT(item->data == i);

            curr = list_next(curr);

            if (curr == list_end(&list)) {
                break;
            }
        }

        list_remove(&second.node);

        ASSERT(list_get_item(list_front(&list), Elem, node)->data == 0);
        ASSERT(list_get_item(list_back(&list), Elem, node)->data == 2);
    }
}

void tests_path()
{
    {
        ASSERT(os_path_is_absolute(str_literal("/foo")));
        ASSERT(os_path_is_absolute(str_literal("/")));
        ASSERT(!os_path_is_absolute(str_literal("~/code")));
        ASSERT(!os_path_is_absolute(str_literal("./")));
        ASSERT(!os_path_is_absolute(str_literal("../foo")));
    }
}

int main()
{
    LinearArena arena = arena_create(default_allocator, MB(4));
    Allocator alloc = arena_create_allocator(&arena);

    String executable = os_get_executable_directory(alloc);
    String path = os_get_absolute_path(str_literal("./a.out"), alloc);
    String parent = os_get_absolute_parent_path(path, alloc);
    String p = str_literal(".././");
    String abc = os_get_canonical_path(p, alloc);

    printf("%.*s\n", (s32)executable.length, executable.data);
    printf("%.*s\n", (s32)path.length, path.data);
    printf("%.*s\n", (s32)parent.length, parent.data);
    printf("%.*s\n", (s32)abc.length, abc.data);

    arena_destroy(&arena);

    tests_arena();
    tests_string();
    tests_utils();
    tests_file();
    tests_list();
    tests_path();

    return 0;
}
