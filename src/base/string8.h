#ifndef STRING8_H
#define STRING8_H

#include "allocator.h"

#define str_literal(str) (String){ .data = (str), .length = ARRAY_COUNT((str)) - 1 }
#define null_string (String) { .data = 0, .length = 0 }

typedef struct String {
    char  *data;
    ssize  length;
} String;

String str_concat(String a, String b, Allocator alloc);
bool   str_equal(String a, String b);
String str_null_terminate(String str, Allocator alloc);
String str_copy(String str, Allocator alloc);
bool   str_starts_with(String str, String substr);
ssize  str_find_first_occurence(String str, char ch);
ssize  str_find_last_occurence(String str, char ch);
String str_allocate(ssize length, Allocator allocator);
ssize  str_get_null_terminated_length(String str);

#endif //STRING8_H
