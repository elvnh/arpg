#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limits.h>

#define MAX_SIGNED_TYPE_VALUE(type) ((ssize)(((u64)1 << (CHAR_BIT * sizeof(type) - 1)) - 1))
#define MIN_SIGNED_TYPE_VALUE(type) (-MAX_SIGNED_TYPE_VALUE(type) - 1)
#define MAX_UNSIGNED_TYPE_VALUE(type) ((type)-1)

#define USIZE_MAX    MAX_UNSIGNED_TYPE_VALUE(usize)
#define U64_MAX      MAX_UNSIGNED_TYPE_VALUE(u64)
#define U32_MAX      MAX_UNSIGNED_TYPE_VALUE(u32)
#define U16_MAX      MAX_UNSIGNED_TYPE_VALUE(u16)
#define U8_MAX       MAX_UNSIGNED_TYPE_VALUE(u8)

#define S_SIZE_MAX   MAX_SIGNED_TYPE_VALUE(ssize)
#define S_SIZE_MIN   MIN_SIGNED_TYPE_VALUE(ssize)
#define S64_MAX      MAX_SIGNED_TYPE_VALUE(s64)
#define S64_MIN      MIN_SIGNED_TYPE_VALUE(s64)
#define S32_MAX      MAX_SIGNED_TYPE_VALUE(s32)
#define S32_MIN      MIN_SIGNED_TYPE_VALUE(s32)
#define S16_MAX      MAX_SIGNED_TYPE_VALUE(s16)
#define S16_MIN      MIN_SIGNED_TYPE_VALUE(s16)
#define S8_MAX       MAX_SIGNED_TYPE_VALUE(s8)
#define S8_MIN       MIN_SIGNED_TYPE_VALUE(s8)

typedef int8_t    s8;
typedef int16_t   s16;
typedef int32_t   s32;
typedef int64_t   s64;

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;

typedef size_t    usize;
typedef ptrdiff_t ssize;

typedef unsigned char byte;
typedef s8  b8;
typedef s32 b32;

typedef float  f32;
typedef double f64;

#endif //TYPEDEFS_H
