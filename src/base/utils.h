#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

#include "typedefs.h"

void abort();

#define ArrayCount(arr) (ssize)(sizeof(arr) / sizeof(*arr))
#define Kilobytes(n) (n * 1024)
#define Megabytes(n) (Kilobytes(n) * 1024)
#define IsPow2(n) (((n) != 0) && (((n) & ((n) - 1)) == 0))
#define Max(a, b) ((a) > (b) ? (a) : (b))
#define MemZero(ptr, size) memset((ptr), 0, size)
#define Assert(expr) do { if (!(expr)) { AssertImpl(#expr, CurrentFunctionName, CurrentFileName, CurrentLine); } } while (0)

#if defined(__GNUC__)
    #define AlignOf(t)          __alignof__(t)
    #define CurrentFunctionName __func__
    #define CurrentFileName     __FILE__
    #define CurrentLine         __LINE__
#else
    #error Unsupported compiler
#endif

static inline void AssertImpl(const char *expr, const char *function_name, const char *file_name, s32 line_nr)
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

static inline void *ByteOffset(void *ptr, ssize offset)
{
    return (void *)((byte *)ptr + offset);
}

static inline bool MultiplicationOverflows_ssize(ssize a, ssize b)
{
    if ((a > 0) && (b > 0) && (a > (S_SIZE_MAX / b))) {
        return true;
    }

    return false;
}

static inline bool AdditionOverflows_ssize(ssize a, ssize b)
{
    if (a > (S_SIZE_MAX - b)) {
        return true;
    }

    return false;
}

static inline s64 Align(s64 value, s64 alignment)
{
    return (value - 1 + alignment) & -alignment;
}

static inline bool IsAligned(s64 value, s64 alignment)
{
    Assert(IsPow2(alignment));
    return (value % alignment) == 0;
}

// TODO: get rid of this
static inline usize Cast_s64_usize(s64 value)
{
    Assert(value >= 0);
    Assert((u64)value <= USIZE_MAX);

    return (usize)value;
}

#endif //UTILS_H
