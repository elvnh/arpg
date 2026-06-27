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

static inline s64 *value_as_int(Value *value)
{
    s64 *result = 0;

    if (value) {
        ASSERT(value->kind == VALUE_INT);
        result = &value->as.integer;
    }

    return result;
}

static inline String *value_as_string(Value *value)
{
    String *result = 0;

    if (value) {
        ASSERT(value->kind == VALUE_STRING);
        result = &value->as.string;
    }

    return result;
}

static inline String *value_as_identifier(Value *value)
{
    String *result = 0;

    if (value) {
        ASSERT(value->kind == VALUE_IDENTIFIER);
        result = &value->as.identifier;
    }

    return result;
}

static inline ValueList *value_as_list(Value *value)
{
    ValueList *result = 0;

    if (value) {
        ASSERT(value->kind == VALUE_LIST);
        result = &value->as.list;
    }

    return result;
}

static inline Record *value_as_record(Value *value)
{
    Record *result = 0;

    if (value) {
        ASSERT(value->kind == VALUE_RECORD);
        result = &value->as.record;
    }

    return result;
}

static inline Value *record_get(Record *record, String name)
{
    Value *result = 0;

    if (record) {
        for (Attribute *attr = list_head(record); attr; attr = list_next(attr)) {
            if (str_equal(attr->name, name)) {
                result = attr->value;
                break;
            }
        }
    }

    return result;
}

static inline b32 record_has_field_of_type(Record *rec, String name, ValueKind type)
{
    b32 result = false;

    Value *val = record_get(rec, name);

    if (val) {
        result = val->kind == type;
    }

    return result;
}

#endif // VALUE_H
