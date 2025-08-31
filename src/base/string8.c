#include <string.h>

#include "string8.h"
#include "utils.h"

String str_concat(String a, String b, Allocator alloc)
{
    const ssize total_size = a.length + b.length;
    const usize a_length = cast_s64_to_usize(a.length);
    const usize b_length = cast_s64_to_usize(b.length);

    char *new_string = 0;
    if (try_resize_allocation(alloc, a.data, a.length, total_size)) {
        new_string = a.data;
    } else {
        new_string = allocate_array(alloc, char, total_size);

        memcpy(new_string, a.data, a_length);
    }

    memcpy(new_string + a_length, b.data,  b_length);

    String result = {
        .data = new_string,
        .length = total_size
    };

    return result;
}

bool str_equal(String a, String b)
{
    if (a.length != b.length) {
        return false;
    }

    return (memcmp(a.data, b.data, cast_s64_to_usize(a.length)) == 0);
}

String str_null_terminate(String str, Allocator alloc)
{
    if (add_overflows_ssize(str.length, 1)) {
        ASSERT(false);
        return null_string;
    }

    char *new_string = 0;

    if (try_resize_allocation(alloc, str.data, str.length, str.length + 1)) {
        new_string = str.data;
    } else {
        new_string = allocate_array(alloc, char, str.length + 1);
        memcpy(new_string, str.data, cast_s64_to_usize(str.length));
    }

    new_string[str.length] = '\0';

    String result = {
        .data = new_string,
        .length = str.length
    };

    return result;
}

String str_copy(String str, Allocator alloc)
{
    char *new_string = allocate_array(alloc, char, str.length);
    memcpy(new_string, str.data, cast_s64_to_usize(str.length));

    String result = {
        .data = new_string,
        .length = str.length
    };

    return result;
}

bool str_starts_with(String str, String substr)
{
    ASSERT(str.data);
    ASSERT(str.length);

    ASSERT(substr.data);
    ASSERT(substr.length);

    if (substr.length > str.length) {
        return false;
    }

    return (memcmp(str.data, substr.data, cast_s64_to_usize(substr.length)) == 0);
}

bool str_ends_with(String str, String substr)
{
    if (substr.length > str.length) {
        return false;
    }

    String span = str_create_span(str, str.length - substr.length, substr.length);

    return str_equal(span, substr);
}

ssize str_find_first_occurence(String str, String pattern)
{
    for (s64 i = 0; i <= str.length - pattern.length; ++i) {
        String substr = str_create_span(str, i, pattern.length);

        if (str_equal(substr, pattern)) {
            return i;
        }
    }

    return -1;
}

ssize str_find_last_occurence(String str, String pattern)
{
    for (s64 i = str.length - pattern.length; i >= 0; --i) {
        String substr = str_create_span(str, i, pattern.length);

        if (str_equal(substr, pattern)) {
            return i;
        }
    }

    return -1;
}

String str_allocate(ssize length, Allocator allocator)
{
    char *data = allocate_array(allocator, char, length);

    String result = {
        .data = data,
        .length = length
    };

    return result;
}

ssize str_get_null_terminated_length(String str)
{
    return str_find_first_occurence(str, str_literal("\0"));
}

String str_create_span(String str, ssize start_index, ssize length)
{
    ASSERT(str.data);
    ASSERT(str.length);
    ASSERT(start_index + length <= str.length);

    String result = {
        .data = str.data + start_index,
        .length = length
    };

    return result;
}

ssize str_get_common_prefix_length(String a, String b)
{
    ssize min_length = MIN(a.length, b.length);

    ssize i;
    for (i = 0; i < min_length; ++i) {
        if (a.data[i] != b.data[i]) {
            break;
        }
    }

    return i;
}
