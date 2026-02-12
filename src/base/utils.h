#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

#include "typedefs.h"

#define ARRAY_COUNT(arr) (ssize)(sizeof(arr) / sizeof(*arr))
#define ASSERT_BREAK(expr) do { if (!(expr)) { DEBUG_BREAK; } } while (0)
#define INVALID_DEFAULT_CASE default: ASSERT(0); break;
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define KB(n) (n * 1024)
#define MB(n) (KB(n) * 1024)
#define SIZEOF(t) ((ssize)(sizeof(t)))
#define CLAMP(n, low, high) ((n) < (low) ? (low) : ((n) > (high) ? (high) : (n)))
#define mem_zero(ptr, size) memset((ptr), 0, (usize)(size))
#define zero_array(ptr, count) (mem_zero(ptr, count * SIZEOF(*ptr)))
#define zero_struct(t) ((t){0})
#define FLAG(e) ((u64)(1u << (u64)(e)))
#define SQUARE(n) ((n) * (n))
#define has_flag(flags, flag) (((flags) & (flag)) != 0)
#define unset_flag(flags, flag) ((flags) &= ~(flag))
// TODO: define everything for non-debug builds

#define ASSERT(expr)                                    \
    do {                                                \
        if (!(expr)) {                                  \
            fprintf(stderr,                             \
                "\n*** ASSERTION FAILURE ***\n"         \
                "Expression: '%s'\n\n"                  \
                "%s:\n"                                 \
                "%s:%d:\n",                             \
                #expr, __func__, FILE_NAME, LINE);      \
            DEBUG_BREAK;                                \
        }                                               \
    } while (0)


#define UNIMPLEMENTED                                                                     \
    fprintf(stderr, "\n*** UNIMPLEMENTED ***\n%s:\n%s:%d:\n", __func__, FILE_NAME, LINE); \
    DEBUG_BREAK

#if defined(__GNUC__)
#    define ALIGNOF(t)              (ssize)__alignof__(t)
#    define ALIGNAS(n)              __attribute__ ((aligned ((n))))
#    define ALIGNAS_T(t)            __attribute__ ((aligned ((ALIGNOF(t)))))
#    define TYPEOF(e)               __typeof__(e)
#    define TYPES_EQUAL(t, u)       __builtin_types_compatible_p(t, u)
#    define STATIC_ASSERT(e)        ((e) ? 0 : static_assert_fail())
#    define STATIC_ASSERT_ATTRIBUTE __attribute__((error("Static assertion failed")))
#    define FILE_NAME               __FILE__
#    define LINE                    __LINE__
#else
#    error Unsupported compiler
#endif

#if defined(__x86_64__)
#    define DEBUG_BREAK __asm volatile("int3")
#else
#    error Unsupported architecture
#endif

#define EXPR_TYPES_EQUAL(expr1, expr2) (TYPES_EQUAL(TYPEOF(expr1), TYPEOF(expr2)))

STATIC_ASSERT_ATTRIBUTE
static inline int static_assert_fail(void)
{
    return 0;
}

static inline bool is_pow2(s64 n)
{
    return (n != 0) && ((n & (n - 1)) == 0);
}

// Returns a sequence of n consecutive bits in number shifted down to LSB
inline static u64 bit_span(u64 n, u32 index, u32 length)
{
    ASSERT(length >= 1);
    ASSERT(length <= 64);
    ASSERT(index < 64);

    if (length == 64) {
        return n;
    }

    ASSERT((index + length) < 64);

    u64 mask = ((u64)1 << (index + length)) - (u64)1;
    mask |= ((u64)1 << index) - (u64)1;

    u64 result = (n & mask) >> index;

    return result;
}

static inline void *byte_offset(void *ptr, ssize offset)
{
    return (void *)((byte *)ptr + offset);
}

static inline ssize ptr_diff(void *a, void *b)
{
    return (ssize)a - (ssize)b;
}

static inline bool multiply_overflows_ssize(ssize a, ssize b)
{
    if ((a > 0) && (b > 0) && (a > (S_SIZE_MAX / b))) {
        return true;
    }

    return false;
}

// TODO: create macro for these functions
static inline bool add_overflows_ssize(ssize a, ssize b)
{
    ASSERT(b >= 0);

    if (a > (S_SIZE_MAX - b)) {
        return true;
    }

    return false;
}

static inline bool add_overflows_s32(s32 a, s32 b)
{
    if (a > (S32_MAX - b)) {
        return true;
    }

    return false;
}

static inline bool is_aligned(s64 value, s64 alignment)
{
    //ASSERT(value >= 0);
    ASSERT(is_pow2(alignment));
    return (value % alignment) == 0;
}

static inline s64 align(s64 value, s64 alignment)
{
    ASSERT(is_pow2(alignment));
    ASSERT(!add_overflows_ssize(value - 1, alignment));

    ssize result = (value - 1 + alignment) & -alignment;
    ASSERT(result >= value);
    ASSERT(is_aligned(result, alignment));

    return result;
}

static inline usize ssize_to_usize(ssize value)
{
    ASSERT(value >= 0);
    ASSERT((usize)value <= USIZE_MAX);

    return (usize)value;
}

static inline s32 ssize_to_s32(ssize value)
{
    ASSERT((value >= S32_MIN) && (value <= S32_MAX));

    return (s32)value;
}

static inline b32 f32_in_range(f32 val, f32 low, f32 high, f32 epsilon)
{
    b32 result = (val >= (low - epsilon)) && (val <= (high + epsilon));

    return result;
}

static inline ssize mod_index(u64 hash, ssize array_size)
{
    ASSERT(is_pow2(array_size));

    ssize result = (ssize)(hash & (usize)(array_size - 1));
    return result;
}

#endif //UTILS_H
