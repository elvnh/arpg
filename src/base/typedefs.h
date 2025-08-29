#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limits.h> // TODO: remove

#define PTR_SIZE sizeof(void*)

// TODO: implement own
#define S32_MAX INT_MAX
#define S32_MIN INT_MIN

#define S64_MAX (s64)(((u64)1 << (u64)63) - 1)
#define S_SIZE_MAX (ssize)(((u64)1 << (PTR_SIZE * 8 - 1)) - 1)
#define USIZE_MAX ((usize)-1)

typedef int8_t    s8;
typedef int16_t   s16;
typedef int32_t   s32;
typedef int64_t   s64;

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;

typedef size_t    usize;
typedef ptrdiff_t ssize; // TODO: ssize -> size?

typedef unsigned char byte;

typedef float  f32;
typedef double f64;

#endif //TYPEDEFS_H
