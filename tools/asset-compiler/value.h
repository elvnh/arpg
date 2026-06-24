#ifndef VALUE_H
#define VALUE_H

#include "base/linear_arena.h"

typedef enum {
    VALUE_INT,
    VALUE_STRING,
    VALUE_IDENTIFIER,
    VALUE_LIST,
    VALUE_RECORD,
} ValueKind;

typedef struct Attribute {
    String name;
    struct Value *value;
    struct Attribute *next;
} Attribute;

typedef struct {
    Attribute *head;
    Attribute *tail;
} Record;

typedef struct {
    struct Value *head;
    struct Value *tail;
} ValueList;

// TODO: rename to value
typedef struct Value {
    ValueKind kind;

    union {
        s64 integer;
        String string;
        String identifier;
        Record record;
        ValueList list;
    } as;

    struct Value *next;
} Value;

static inline Value *value_make(LinearArena *arena, ValueKind kind)
{
    Value *result = la_allocate_item(arena, Value);
    result->kind = kind;

    return result;
}

#endif // VALUE_H
