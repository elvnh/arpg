#ifndef FILE_H
#define FILE_H

#include "base/string8.h"
#include "base/linear_arena.h"
#include "base/span.h"

Span    os_read_entire_file(String path, Allocator allocator, LinearArena *scratch);
String  os_read_entire_file_as_string(String path, Allocator allocator, LinearArena *scratch);
ssize   os_get_file_size(String path, LinearArena *scratch);

#endif //FILE_H
