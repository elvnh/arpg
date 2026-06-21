#ifndef INT_PARSE_H
#define INT_PARSE_H

#include "string8.h"
#include "typedefs.h"

typedef enum {
    NUM_BASE_DEC = 10,
    NUM_BASE_HEX = 16,
} NumberBase;

typedef struct {
    u64 value;
    b32 ok;
} MaybeU64;

typedef struct {
    s64 value;
    b32 ok;
} MaybeS64;

MaybeU64 parse_u64(String string, NumberBase base);
MaybeS64 parse_s64(String string, NumberBase base);

#endif // INT_PARSE_H
