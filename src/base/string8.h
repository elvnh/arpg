#ifndef STRING8_H
#define STRING8_H

#include "allocator.h"
#include "base/list.h"

#define str_lit(str)   (String){ .data = (str), .length = ARRAY_COUNT((str)) - 1 }
#define null_string    (String){ .data = 0, .length = 0 }
#define empty_string   str_lit("")

typedef struct String {
    char  *data;
    ssize  length;
} String;

typedef struct StringNode {
    LIST_LINKS(StringNode);

    String data;
} StringNode;

DEFINE_LIST(StringNode, StringList);

typedef struct {
    String buffer;
    ssize  capacity;
} StringBuilder;

String  str_concat(String a, String b, Allocator alloc);
bool    str_equal(String a, String b);
String  str_null_terminate(String str, Allocator alloc);
String  str_copy(String str, Allocator alloc);
bool    str_starts_with(String str, String substr);
bool    str_ends_with(String str, String substr);
ssize   str_find_first_occurence(String str, String pattern);
ssize   str_find_first_occurence_from_index(String str, String pattern, ssize index);
ssize   str_find_last_occurence(String str, String pattern);
String  str_allocate(ssize length, Allocator allocator);
ssize   str_get_null_terminated_length(String str);
String  str_create_span(String str, ssize start_index, ssize length);
ssize   str_get_common_prefix_length(String a, String b);
String  str_substring_before_pattern(String str, String substr);
void    str_print(String str);

// TODO: move to different file
StringBuilder str_builder_allocate(ssize capacity, Allocator allocator);
void          str_builder_append(StringBuilder *sb, String str);
b32           str_builder_has_capacity_for(const StringBuilder *sb, String str);

#endif //STRING8_H
