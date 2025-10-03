#ifndef FORMAT_H
#define FORMAT_H

#include <stdio.h>

#include "base/linear_arena.h"
#include "base/string8.h"

ssize digit_count_s64(s64 n)
{
    // TODO: other number bases
    if (n == 0) {
        return 1;
    }

    if (n < 0) {
        n = -n;
    }

    s64 result = (ssize)(log((f64)n) / log((f64)10)) + 1;
    return result;
}

static inline String s32_to_string(s32 number, Allocator allocator)
{
    b32 is_negative = number < 0;
    ssize num_chars = digit_count_s64(number) + is_negative;

    ssize alloc_size = num_chars + 1;
    String result = str_allocate(alloc_size, allocator);
    s32 chars_written = snprintf(result.data, (usize)alloc_size, "%d", number);
    ASSERT(chars_written == num_chars);

    result.length = num_chars;

    return result;
}

static inline String f32_to_string(f32 number, s32 precision, Allocator allocator)
{
    s64 as_int = (s64)number;
    b32 is_negative = number < 0.0f;
    ssize num_chars = digit_count_s64(as_int) + (precision != 0) + precision + is_negative;

    ssize alloc_size = num_chars + 1;
    String result = str_allocate(alloc_size, allocator);
    s32 chars_written = snprintf(result.data, (usize)alloc_size, "%.*f", precision, (f64)number);
    ASSERT(chars_written == num_chars);

    result.length = num_chars;

    return result;
}

#endif //FORMAT_H
