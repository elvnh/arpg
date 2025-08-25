#ifndef STRING8_H
#define STRING8_H

#include "allocator.h"

#define String_Literal(str) (String){ .data = (str), .length = ArrayCount((str)) - 1 }
#define NullString (String) { .data = 0, .length = 0 }

typedef struct {
    char  *data;
    ssize  length;
} String;

String String_Concat(String a, String b, Allocator alloc);
bool   String_Equal(String a, String b);
String String_NullTerminate(String str, Allocator alloc);
String String_Copy(String str, Allocator alloc);

#endif //STRING8_H
