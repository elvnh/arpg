#ifndef UTILS_H
#define UTILS_H

#include "typedefs.h"

void abort();

#define ArrayCount(arr) (ssize)(sizeof(arr) / sizeof(*arr))
#define Assert(expr) do { if (!(expr)) { abort(); } } while (0)
#define Kilobytes(n) (n * 1024)
#define Megabytes(n) (Kilobytes(n) * 1024)
#define IsPow2(n) (((n) != 0) && (((n) & ((n) - 1)) == 0))

#if defined(__GNUC__)
    #define AlignOf(t) __alignof__(t)
#else
    #error Unsupported compiler
#endif

static inline bool MultiplicationOverflows_s64(s64 a, s64 b)
{
    if ((a > 0) && (b > 0) && (a > (S64_MAX / b))) {
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
