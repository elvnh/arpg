#ifndef FORMAT_H
#define FORMAT_H

#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>

#include "base/linear_arena.h"
#include "base/string8.h"
#include "base/vector.h"

#define FORMAT_MAX_STRING_LENGTH 1024

#ifdef __GNUC__
#    define FORMAT_ATTRIBUTE(fmt_arg_nr, var_arg_nr)            \
        __attribute__((format (printf, fmt_arg_nr, var_arg_nr)))
#else
#    error
#endif

static inline ssize digit_count_s64(s64 n)
{
    // TODO: other number bases
    if (n == 0) {
        return 1;
    }

    if (n < 0) {
        n = -n;
    }

    s64 result = (ssize)ceil(log((f64)n) / log((f64)10)) + 1;
    return result;
}

static inline String s64_to_string(s64 number, Allocator allocator)
{
    b32 is_negative = number < 0;
    ssize num_chars = digit_count_s64(number) + is_negative;

    ssize alloc_size = num_chars + 1;
    String result = str_allocate(alloc_size, allocator);
    s32 chars_written = snprintf(result.data, (usize)alloc_size, "%"PRId64, number);
    ASSERT(chars_written <= num_chars);

    result.length = chars_written;

    return result;
}

static inline String ptr_to_string(void *ptr, Allocator allocator)
{
    ssize length = sizeof(ptr) * 2 + 1;
    String result = str_allocate(length, allocator);
    s32 chars_written = snprintf(result.data, (usize)length, "%p", ptr);
    ASSERT(chars_written <= length);

    result.length = chars_written;

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
    ASSERT(chars_written <= num_chars);

    result.length = chars_written;

    return result;
}


static inline String v2_to_string(Vector2 v, Allocator allocator)
{
    String result = str_concat(
        str_lit("("),
        f32_to_string(v.x, 2, allocator),
        allocator
    );

    result = str_concat(
        result,
        str_concat(
            str_lit(", "),
            f32_to_string(v.y, 2, allocator),
            allocator),
        allocator
    );

    result = str_concat(
        result,
        str_lit(")"),
        allocator
    );

    return result;
}

FORMAT_ATTRIBUTE(2, 3)
static inline String format(LinearArena *arena, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);

    String result = str_allocate(FORMAT_MAX_STRING_LENGTH, la_allocator(arena));

    s32 chars_written = vsnprintf(result.data, (usize)result.length, fmt, va);
    ASSERT(chars_written > 0);
    ASSERT(chars_written < result.length);

    result.length = chars_written;
    la_pop_to(arena, result.data + result.length);

    va_end(va);

    return result;
}

#endif //FORMAT_H
