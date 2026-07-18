#include "platform/path.h"

#include "platform/platform.h"
#include "test_macros.h"

#define GET_SRC_FILE_DIR(allocator) \
    platform_get_canonical_path(platform_get_parent_path(str(__FILE__), (allocator)), (allocator))

#define TEST_INPUT_SUBDIR(name, allocator)                          \
    path_to_str(                                                    \
    path_append(                                                    \
        path_from_str(GET_SRC_FILE_DIR((allocator)), (allocator)),  \
        str(name),                                                  \
        (allocator)),                                               \
        (allocator))

TEST_CASE(path_from_str_basic)
{
    Path p = path_from_str(str("home/foo"), default_allocator);

    REQUIRE(p.count == 2);
    REQUIRE_STR_EQ(p.items[0], str("home"));
    REQUIRE_STR_EQ(p.items[1], str("foo"));
}

TEST_CASE(path_from_str_leading_slash)
{
    Path p = path_from_str(str("/home/foo"), default_allocator);

    REQUIRE(p.count == 3);
    REQUIRE_STR_EQ(p.items[0], str("/"));
    REQUIRE_STR_EQ(p.items[1], str("home"));
    REQUIRE_STR_EQ(p.items[2], str("foo"));
}

TEST_CASE(path_from_str_trailing_slash)
{
    Path p = path_from_str(str("home/foo/"), default_allocator);

    REQUIRE(p.count == 2);
    REQUIRE_STR_EQ(p.items[0], str("home"));
    REQUIRE_STR_EQ(p.items[1], str("foo"));
}

TEST_CASE(path_from_str_leading_and_trailing_slash)
{
    Path p = path_from_str(str("/home/foo/"), default_allocator);

    REQUIRE(p.count == 3);
    REQUIRE_STR_EQ(p.items[0], str("/"));
    REQUIRE_STR_EQ(p.items[1], str("home"));
    REQUIRE_STR_EQ(p.items[2], str("foo"));
}

TEST_CASE(path_from_str_single_component)
{
    Path p = path_from_str(str("home"), default_allocator);

    REQUIRE(p.count == 1);
    REQUIRE_STR_EQ(p.items[0], str("home"));
}

TEST_CASE(path_from_str_single_component_trailing_slash)
{
    Path p = path_from_str(str("home/"), default_allocator);

    REQUIRE(p.count == 1);
    REQUIRE_STR_EQ(p.items[0], str("home"));
}

TEST_CASE(path_from_str_single_component_root)
{
    Path p = path_from_str(str("/"), default_allocator);

    REQUIRE(p.count == 1);
    REQUIRE_STR_EQ(p.items[0], str("/"));
}

TEST_CASE(path_from_str_root_subdir)
{
    Path p = path_from_str(str("/home"), default_allocator);

    REQUIRE(p.count == 2);
    REQUIRE_STR_EQ(p.items[0], str("/"));
    REQUIRE_STR_EQ(p.items[1], str("home"));
}

TEST_CASE(path_from_str_root_subdir_trailing_slash)
{
    Path p = path_from_str(str("/home/"), default_allocator);

    REQUIRE(p.count == 2);
    REQUIRE_STR_EQ(p.items[0], str("/"));
    REQUIRE_STR_EQ(p.items[1], str("home"));
}

TEST_CASE(path_to_str_basic)
{
    Path p = path_from_str(str("home"), default_allocator);
    String s = path_to_str(p, default_allocator);

    REQUIRE_STR_EQ(s, str("home"));
}

TEST_CASE(path_to_str_root)
{
    Path p = path_from_str(str("/"), default_allocator);
    String s = path_to_str(p, default_allocator);

    REQUIRE_STR_EQ(s, str("/"));
}

TEST_CASE(path_to_str_multiple)
{
    Path p = path_from_str(str("/home/foo"), default_allocator);
    String s = path_to_str(p, default_allocator);

    REQUIRE_STR_EQ(s, str("/home/foo"));
}

TEST_CASE(path_to_str_multiple_trailing_slash)
{
    Path p = path_from_str(str("/home/foo/"), default_allocator);
    String s = path_to_str(p, default_allocator);

    REQUIRE_STR_EQ(s, str("/home/foo"));
}

TEST_CASE(path_append_inplace_basic)
{
    Path p = path_from_str(str("/home"), default_allocator);
    path_append_inplace(&p, str("foo"), default_allocator);
    String s = path_to_str(p, default_allocator);

    REQUIRE_STR_EQ(s, str("/home/foo"));
}

TEST_CASE(path_append_inplace_trailing_slash)
{
    Path p = path_from_str(str("/home"), default_allocator);
    path_append_inplace(&p, str("foo/"), default_allocator);
    String s = path_to_str(p, default_allocator);

    REQUIRE_STR_EQ(s, str("/home/foo"));
}

TEST_CASE(path_append_inplace_root_to_empty)
{
    Path p = path_create(16, default_allocator);
    path_append_inplace(&p, str("/"), default_allocator);
    String s = path_to_str(p, default_allocator);

    REQUIRE_STR_EQ(s, str("/"));
}

TEST_CASE(path_append_inplace_non_root_to_empty)
{
    Path p = path_create(16, default_allocator);
    path_append_inplace(&p, str("foo"), default_allocator);
    String s = path_to_str(p, default_allocator);

    REQUIRE_STR_EQ(s, str("foo"));
}

TEST_CASE(path_append_inplace_trailing_slash_to_empty)
{
    Path p = path_create(16, default_allocator);
    path_append_inplace(&p, str("foo/"), default_allocator);
    String s = path_to_str(p, default_allocator);

    REQUIRE_STR_EQ(s, str("foo"));
}

TEST_CASE(path_append_inplace_leading_slash_to_empty)
{
    Path p = path_create(16, default_allocator);
    path_append_inplace(&p, str("/foo"), default_allocator);
    String s = path_to_str(p, default_allocator);

    REQUIRE_STR_EQ(s, str("/foo"));
}

TEST_CASE(path_append)
{
    Path a = path_from_str(str("foo"), default_allocator);
    Path b = path_append(a, str("bar"), default_allocator);

    String a_str = path_to_str(a, default_allocator);
    String b_str = path_to_str(b, default_allocator);

    REQUIRE_STR_EQ(a_str, str("foo"));
    REQUIRE_STR_EQ(b_str, str("foo/bar"));
}

TEST_CASE(path_make_relative_to_entire_base_is_common_prefix)
{
    String a = TEST_INPUT_SUBDIR("foo", default_allocator);
    String b = TEST_INPUT_SUBDIR("foo/bar", default_allocator);

    String c = path_make_relative_to(b, a, default_allocator);
    REQUIRE_STR_EQ(c, str("bar"));
}

TEST_CASE(path_make_relative_to_part_of_base_is_common_prefix)
{
    String a = TEST_INPUT_SUBDIR("foo/bar", default_allocator);
    String b = TEST_INPUT_SUBDIR("foo/baz", default_allocator);

    String c = path_make_relative_to(b, a, default_allocator);
    REQUIRE_STR_EQ(c, str("../baz"));
}

TEST_CASE(path_make_relative_to_path_is_common_prefix)
{
    String base = TEST_INPUT_SUBDIR("foo/bar", default_allocator);
    String p = TEST_INPUT_SUBDIR("foo", default_allocator);

    String result = path_make_relative_to(p, base, default_allocator);
    REQUIRE_STR_EQ(result, str(".."));
}

TEST_CASE(path_make_relative_to_equal_paths)
{
    String base = TEST_INPUT_SUBDIR("foo", default_allocator);
    String p = TEST_INPUT_SUBDIR("foo", default_allocator);

    String result = path_make_relative_to(p, base, default_allocator);
    REQUIRE_STR_EQ(result, str(""));
}

TEST_CASE(path_make_relative_base_deep)
{
    String base = TEST_INPUT_SUBDIR("foo/bar/quux", default_allocator);
    String p = TEST_INPUT_SUBDIR("foo/baz", default_allocator);

    String result = path_make_relative_to(p, base, default_allocator);
    REQUIRE_STR_EQ(result, str("../../baz"));
}

TEST_CASE(path_make_relative_base_deep2)
{
    String base = TEST_INPUT_SUBDIR("foo/bar/quux", default_allocator);
    String p = TEST_INPUT_SUBDIR("foo", default_allocator);

    String result = path_make_relative_to(p, base, default_allocator);
    REQUIRE_STR_EQ(result, str("../.."));
}

TEST_CASE(path_make_relative_base_deep3)
{
    String base = TEST_INPUT_SUBDIR("foo/bar/quux", default_allocator);
    String p = TEST_INPUT_SUBDIR("abc", default_allocator);

    String result = path_make_relative_to(p, base, default_allocator);
    REQUIRE_STR_EQ(result, str("../../../abc"));
}

TEST_CASE(path_make_relative_dot_slash)
{
    String base = TEST_INPUT_SUBDIR("foo/bar/quux", default_allocator);
    String p = TEST_INPUT_SUBDIR("./", default_allocator);

    String result = path_make_relative_to(p, base, default_allocator);
    REQUIRE_STR_EQ(result, str("../../.."));
}
