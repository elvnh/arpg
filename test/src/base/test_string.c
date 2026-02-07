#include "test_macros.h"
#include "base/string8.h"

TEST_CASE(string_equal)
{
    String a = str_lit("hello ");
    String b = str_lit("world");
    String c = str_lit("hello ");

    REQUIRE(!str_equal(a, b));
    REQUIRE(str_equal(a, c));
}

TEST_CASE(string_equal_zero_length)
{
    String a = str_lit("");
    String b = str_lit("abc");
    String c = str_lit("");

    REQUIRE(!str_equal(a, b));
    REQUIRE(str_equal(a, c));
}

TEST_CASE(string_equal_null)
{
    String a = {0};
    String b = str_lit("abc");
    String c = {0};

    REQUIRE(!str_equal(a, b));
    REQUIRE(str_equal(a, c));
}

TEST_CASE(string_concat)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    String a = str_lit("hello ");
    String b = str_lit("world");

    String c = str_concat(a, b, la_allocator(&arena));
    REQUIRE(str_equal(c, str_lit("hello world")));
    REQUIRE(c.data != a.data);
    REQUIRE(c.data != b.data);

    la_destroy(&arena);
}

TEST_CASE(string_copy)
{
    LinearArena arena = la_create(default_allocator, MB(1));
    Allocator allocator = la_allocator(&arena);

    String lit = str_lit("abcdef");
    String copy = str_copy(lit, allocator);

    REQUIRE(str_equal(lit, copy));
    REQUIRE(lit.data != copy.data);

    la_destroy(&arena);
}

TEST_CASE(string_null_terminate_maybe_grow_in_place)
{
    LinearArena arena = la_create(default_allocator, MB(1));
    Allocator allocator = la_allocator(&arena);

    // Copy to heap to allow writing non-null byte past end
    String lit = str_copy(str_lit("abcdef"), allocator);
    lit.data[lit.length] = 'C';
    String terminated = str_null_terminate(lit, allocator);

    REQUIRE(str_equal(lit, terminated));
    REQUIRE(terminated.data[terminated.length] == '\0');

    la_destroy(&arena);
}

TEST_CASE(string_null_terminate_no_grow_in_place)
{
    LinearArena arena = la_create(default_allocator, MB(1));
    Allocator allocator = la_allocator(&arena);

    // Copy to heap to allow writing non-null byte past end
    String lit = str_copy(str_lit("abcdef"), allocator);

    // Make allocation to prevent it from growing in place
    *la_allocate_item(&arena, byte) = 'C';

    String terminated = str_null_terminate(lit, allocator);

    REQUIRE(lit.data != terminated.data);
    REQUIRE(str_equal(lit, terminated));
    REQUIRE(terminated.data[terminated.length] == '\0');

    la_destroy(&arena);
}

TEST_CASE(string_null_terminate_literal)
{
    LinearArena arena = la_create(default_allocator, MB(1));
    Allocator allocator = la_allocator(&arena);

    String lit = str_lit("abcdef");
    String terminated = str_null_terminate(lit, allocator);

    REQUIRE(lit.data != terminated.data);
    REQUIRE(str_equal(lit, terminated));
    REQUIRE(terminated.data[terminated.length] == '\0');

    la_destroy(&arena);
}

TEST_CASE(string_starts_with)
{
    REQUIRE(str_starts_with(str_lit("abc"), str_lit("a")));
    REQUIRE(str_starts_with(str_lit("abc"), str_lit("ab")));
    REQUIRE(str_starts_with(str_lit("abc"), str_lit("abc")));
    REQUIRE(!str_starts_with(str_lit("abc"), str_lit("abcd")));
    REQUIRE(!str_starts_with(str_lit("abc"), str_lit("b")));
}

TEST_CASE(string_ends_with)
{
    String str = str_lit("abcdef");

    REQUIRE(str_ends_with(str, str_lit("f")));
    REQUIRE(str_ends_with(str, str_lit("ef")));
    REQUIRE(str_ends_with(str, str_lit("def")));
    REQUIRE(str_ends_with(str, str_lit("cdef")));
    REQUIRE(str_ends_with(str, str_lit("bcdef")));
    REQUIRE(str_ends_with(str, str_lit("abcdef")));

    REQUIRE(!str_ends_with(str, str_lit("a")));
    REQUIRE(!str_ends_with(str, str_lit("e")));
    REQUIRE(!str_ends_with(str, str_lit("aabcdef")));
}


TEST_CASE(string_find_first_occurence)
{
    String a = str_lit("abac");
    REQUIRE(str_find_first_occurence(a, str_lit("a")) == 0);
    REQUIRE(str_find_first_occurence(a, str_lit("b")) == 1);
    REQUIRE(str_find_first_occurence(a, str_lit("c")) == 3);
    REQUIRE(str_find_first_occurence(a, str_lit("d")) == -1);

    REQUIRE(str_find_first_occurence(a, str_lit("ab")) == 0);
    REQUIRE(str_find_first_occurence(a, str_lit("ba")) == 1);
    REQUIRE(str_find_first_occurence(a, str_lit("bac")) == 1);
    REQUIRE(str_find_first_occurence(a, str_lit("ac")) == 2);

}

TEST_CASE(string_find_last_occurence)
{
    String a = str_lit("abac");

    REQUIRE(str_find_last_occurence(a, str_lit("a")) == 2);
    REQUIRE(str_find_last_occurence(a, str_lit("c")) == 3);
    REQUIRE(str_find_last_occurence(a, str_lit("b")) == 1);
    REQUIRE(str_find_last_occurence(a, str_lit("d")) == -1);

    REQUIRE(str_find_last_occurence(a, str_lit("ab")) == 0);
    REQUIRE(str_find_last_occurence(a, str_lit("ba")) == 1);
    REQUIRE(str_find_last_occurence(a, str_lit("bac")) == 1);
    REQUIRE(str_find_last_occurence(a, str_lit("ac")) == 2);
    REQUIRE(str_find_last_occurence(a, str_lit("acb")) == -1);
}

TEST_CASE(string_create_span)
{
    String a = str_lit("abcdef");
    String span = str_create_span(a, 1, 3);

    REQUIRE(str_equal(span, str_lit("bcd")));
}

TEST_CASE(string_create_span_up_to_end)
{
    String a = str_lit("abcdef");
    String span = str_create_span(a, 2, 4);

    REQUIRE(str_equal(span, str_lit("cdef")));
}

TEST_CASE(string_create_span_entire_string)
{
    String a = str_lit("a");
    String span = str_create_span(a, 0, 1);

    REQUIRE(str_equal(span, str_lit("a")));
}

TEST_CASE(string_create_span_only_last_char)
{
    String a = str_lit("ab");
    String span = str_create_span(a, 1, 1);

    REQUIRE(str_equal(span, str_lit("b")));
}

TEST_CASE(string_common_prefix_length)
{
    REQUIRE(str_get_common_prefix_length(str_lit("abcdef"), str_lit("a")) == 1);
    REQUIRE(str_get_common_prefix_length(str_lit("abcdef"), str_lit("ab")) == 2);
    REQUIRE(str_get_common_prefix_length(str_lit("abcdef"), str_lit("abc")) == 3);
    REQUIRE(str_get_common_prefix_length(str_lit("abcdef"), str_lit("abd")) == 2);
    REQUIRE(str_get_common_prefix_length(str_lit("abcdef"), str_lit("abcdef")) == 6);

    REQUIRE(str_get_common_prefix_length(str_lit("abcdef"), str_lit("b")) == 0);
    REQUIRE(str_get_common_prefix_length(str_lit("abcdef"), str_lit("c")) == 0);
    REQUIRE(str_get_common_prefix_length(str_lit("abcdef"), str_lit("f")) == 0);
}

TEST_CASE(string_substring_before_pattern)
{
    REQUIRE(str_equal(str_substring_before_pattern(str_lit("abc!def"), str_lit("!")), str_lit("abc")));
    REQUIRE(str_equal(str_substring_before_pattern(str_lit("abc!def"), str_lit("!!")), str_lit("abc!def")));
    REQUIRE(str_equal(str_substring_before_pattern(str_lit("abc!!def"), str_lit("!!")), str_lit("abc")));
    REQUIRE(str_equal(str_substring_before_pattern(str_lit("!!abc"), str_lit("!!")), str_lit("")));
    REQUIRE(str_equal(str_substring_before_pattern(str_lit("abc"), str_lit("!!")), str_lit("abc")));
}

TEST_CASE(string_is_empty)
{
    REQUIRE(str_is_empty(str_lit("")));
    REQUIRE(str_is_empty((String){0}));
    REQUIRE(str_is_empty(null_string));

    REQUIRE(!str_is_empty(str_lit("a")));
}
