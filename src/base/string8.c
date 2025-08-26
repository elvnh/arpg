#include <string.h>

#include "string8.h"
#include "utils.h"

String str_concat(String a, String b, Allocator alloc)
{
    const ssize total_size = a.length + b.length;
    const usize a_length = cast_s64_to_usize(a.length);
    const usize b_length = cast_s64_to_usize(b.length);

    char *new_string = 0;
    if (try_extend_allocation(alloc, a.data, a.length, total_size)) {
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

    if (try_extend_allocation(alloc, str.data, str.length, str.length + 1)) {
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
