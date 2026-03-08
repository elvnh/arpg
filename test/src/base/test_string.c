#include "base/string8.h"
#include "test_macros.h"

TEST_CASE(string_equal)
{
    String a = str("hello ");
    String b = str("world");
    String c = str("hello ");

    REQUIRE(!str_equal(a, b));
    REQUIRE(str_equal(a, c));
}

TEST_CASE(string_equal_zero_length)
{
    String a = str("");
    String b = str("abc");
    String c = str("");

    REQUIRE(!str_equal(a, b));
    REQUIRE(str_equal(a, c));
}

TEST_CASE(string_equal_null)
{
    String a = {0};
    String b = str("abc");
    String c = {0};

    REQUIRE(!str_equal(a, b));
    REQUIRE(str_equal(a, c));
}

TEST_CASE(string_concat)
{
    LinearArena arena = la_create(default_allocator, MB(1));

    String a = str("hello ");
    String b = str("world");

    String c = str_concat(a, b, la_allocator(&arena));
    REQUIRE(str_equal(c, str("hello world")));
    REQUIRE(c.data != a.data);
    REQUIRE(c.data != b.data);

    la_destroy(&arena);
}

TEST_CASE(string_copy)
{
    LinearArena arena = la_create(default_allocator, MB(1));
    Allocator allocator = la_allocator(&arena);

    String lit = str("abcdef");
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
    String lit = str_copy(str("abcdef"), allocator);
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
    String lit = str_copy(str("abcdef"), allocator);

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

    String lit = str("abcdef");
    String terminated = str_null_terminate(lit, allocator);

    REQUIRE(lit.data != terminated.data);
    REQUIRE(str_equal(lit, terminated));
    REQUIRE(terminated.data[terminated.length] == '\0');

    la_destroy(&arena);
}

TEST_CASE(string_starts_with)
{
    REQUIRE(str_starts_with(str("abc"), str("a")));
    REQUIRE(str_starts_with(str("abc"), str("ab")));
    REQUIRE(str_starts_with(str("abc"), str("abc")));
    REQUIRE(!str_starts_with(str("abc"), str("abcd")));
    REQUIRE(!str_starts_with(str("abc"), str("b")));
}

TEST_CASE(string_ends_with)
{
    String str = str("abcdef");

    REQUIRE(str_ends_with(str, str("f")));
    REQUIRE(str_ends_with(str, str("ef")));
    REQUIRE(str_ends_with(str, str("def")));
    REQUIRE(str_ends_with(str, str("cdef")));
    REQUIRE(str_ends_with(str, str("bcdef")));
    REQUIRE(str_ends_with(str, str("abcdef")));

    REQUIRE(!str_ends_with(str, str("a")));
    REQUIRE(!str_ends_with(str, str("e")));
    REQUIRE(!str_ends_with(str, str("aabcdef")));
}

TEST_CASE(string_find_first_occurence)
{
    String a = str("abac");
    REQUIRE(str_find_first_occurence(a, str("a")) == 0);
    REQUIRE(str_find_first_occurence(a, str("b")) == 1);
    REQUIRE(str_find_first_occurence(a, str("c")) == 3);
    REQUIRE(str_find_first_occurence(a, str("d")) == -1);

    REQUIRE(str_find_first_occurence(a, str("ab")) == 0);
    REQUIRE(str_find_first_occurence(a, str("ba")) == 1);
    REQUIRE(str_find_first_occurence(a, str("bac")) == 1);
    REQUIRE(str_find_first_occurence(a, str("ac")) == 2);
}

TEST_CASE(string_find_last_occurence)
{
    String a = str("abac");

    REQUIRE(str_find_last_occurence(a, str("a")) == 2);
    REQUIRE(str_find_last_occurence(a, str("c")) == 3);
    REQUIRE(str_find_last_occurence(a, str("b")) == 1);
    REQUIRE(str_find_last_occurence(a, str("d")) == -1);

    REQUIRE(str_find_last_occurence(a, str("ab")) == 0);
    REQUIRE(str_find_last_occurence(a, str("ba")) == 1);
    REQUIRE(str_find_last_occurence(a, str("bac")) == 1);
    REQUIRE(str_find_last_occurence(a, str("ac")) == 2);
    REQUIRE(str_find_last_occurence(a, str("acb")) == -1);
}

TEST_CASE(string_create_span)
{
    String a = str("abcdef");
    String span = str_create_span(a, 1, 3);

    REQUIRE(str_equal(span, str("bcd")));
}

TEST_CASE(string_create_span_up_to_end)
{
    String a = str("abcdef");
    String span = str_create_span(a, 2, 4);

    REQUIRE(str_equal(span, str("cdef")));
}

TEST_CASE(string_create_span_entire_string)
{
    String a = str("a");
    String span = str_create_span(a, 0, 1);

    REQUIRE(str_equal(span, str("a")));
}

TEST_CASE(string_create_span_only_last_char)
{
    String a = str("ab");
    String span = str_create_span(a, 1, 1);

    REQUIRE(str_equal(span, str("b")));
}

TEST_CASE(string_common_prefix_length)
{
    REQUIRE(str_get_common_prefix_length(str("abcdef"), str("a")) == 1);
    REQUIRE(str_get_common_prefix_length(str("abcdef"), str("ab")) == 2);
    REQUIRE(str_get_common_prefix_length(str("abcdef"), str("abc")) == 3);
    REQUIRE(str_get_common_prefix_length(str("abcdef"), str("abd")) == 2);
    REQUIRE(str_get_common_prefix_length(str("abcdef"), str("abcdef")) == 6);

    REQUIRE(str_get_common_prefix_length(str("abcdef"), str("b")) == 0);
    REQUIRE(str_get_common_prefix_length(str("abcdef"), str("c")) == 0);
    REQUIRE(str_get_common_prefix_length(str("abcdef"), str("f")) == 0);
}

TEST_CASE(string_cut_single_char)
{
    Cut cut = str_cut(str("abc!def"), str("!"));
    REQUIRE(cut.ok);
    REQUIRE(str_equal(cut.head, str("abc")));
    REQUIRE(str_equal(cut.tail, str("def")));
}

TEST_CASE(string_cut_multi_char)
{
    Cut cut = str_cut(str("abc!!def"), str("!!"));
    REQUIRE(cut.ok);
    REQUIRE(str_equal(cut.head, str("abc")));
    REQUIRE(str_equal(cut.tail, str("def")));
}

TEST_CASE(string_cut_separator_not_present)
{
    Cut cut = str_cut(str("abcdef"), str("!!"));
    REQUIRE(!cut.ok);
    REQUIRE(str_equal(cut.head, str("abcdef")));
}

TEST_CASE(string_cut_separator_at_start)
{
    Cut cut = str_cut(str("!!abcdef"), str("!!"));
    REQUIRE(cut.ok);
    REQUIRE(str_is_empty(cut.head));
    REQUIRE(str_equal(cut.tail, str("abcdef")));
}

TEST_CASE(string_cut_separator_at_end)
{
    Cut cut = str_cut(str("abcdef!!"), str("!!"));
    REQUIRE(cut.ok);
    REQUIRE(str_equal(cut.head, str("abcdef")));
    REQUIRE(str_is_empty(cut.tail));
}

TEST_CASE(string_cut_empty_string)
{
    Cut cut = str_cut(null_string, str("!"));
    REQUIRE(!cut.ok);
    REQUIRE(str_is_empty(cut.head));
    REQUIRE(str_is_empty(cut.tail));
}

TEST_CASE(string_is_empty)
{
    REQUIRE(str_is_empty(str("")));
    REQUIRE(str_is_empty((String){0}));
    REQUIRE(str_is_empty(null_string));

    REQUIRE(!str_is_empty(str("a")));
}
