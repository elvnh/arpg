#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

#include "typedefs.h"

#define ARRAY_COUNT(arr) (ssize)(sizeof(arr) / sizeof(*arr))
#define ASSERT(expr) do { if (!(expr)) { assert_impl(#expr, __func__, FILE_NAME, LINE); } } while (0)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define KB(n) (n * 1024)
#define MB(n) (KB(n) * 1024)
#define mem_zero(ptr, size) memset((ptr), 0, (size))

#define UNIMPLEMENTED                                                                     \
    fprintf(stderr, "\n*** UNIMPLEMENTED ***\n%s:\n%s:%d:\n", __func__, FILE_NAME, LINE); \
    abort()

#if defined(__GNUC__)
    #define ALIGNOF(t)   __alignof__(t)
    #define FILE_NAME    __FILE__
    #define LINE         __LINE__
#else
    #error Unsupported compiler
#endif

void abort();

static inline bool is_pow2(s64 n)
{
    return (n != 0) && ((n & (n - 1)) == 0);
}

static inline void assert_impl(const char *expr, const char *function_name, const char *file_name, s32 line_nr)
{
    fprintf(
        stderr,
        "\n*** ASSERTION FAILURE ***\n"
        "Expression: '%s'\n\n"
        "%s:\n"
        "%s:%d:\n",
        expr,
        function_name,
        file_name, line_nr);

    abort();
}

static inline void *byte_offset(void *ptr, ssize offset)
{
    return (void *)((byte *)ptr + offset);
}

static inline bool multiply_overflows_ssize(ssize a, ssize b)
{
    if ((a > 0) && (b > 0) && (a > (S_SIZE_MAX / b))) {
        return true;
    }

    return false;
}

static inline bool add_overflows_ssize(ssize a, ssize b)
{
    if (a > (S_SIZE_MAX - b)) {
        return true;
    }

    return false;
}

static inline s64 align_s64(s64 value, s64 alignment)
{
    return (value - 1 + alignment) & -alignment;
}

static inline bool is_aligned(s64 value, s64 alignment)
{
    ASSERT(is_pow2(alignment));
    return (value % alignment) == 0;
}

// TODO: get rid of this
static inline usize cast_s64_to_usize(s64 value)
{
    ASSERT(value >= 0);
    ASSERT((u64)value <= USIZE_MAX);

    return (usize)value;
}

#endif //UTILS_H
