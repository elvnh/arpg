#include "platform/platform.h"

static void tests_file(void)
{
    {
        LinearArena arena = la_create(default_allocator, MB(1));
        ssize file_size = platform_get_file_size(str_lit(__FILE__), &arena);

        ASSERT(file_size != -1);
        ASSERT(file_size > 1);

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

void tests_path(void)
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

static void run_tests(void)
{
    tests_file();
    tests_path();
}
