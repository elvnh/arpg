#ifndef FILE_H
#define FILE_H

#include "base/string8.h"
#include "base/linear_arena.h"

typedef struct {
    byte  *file_data;
    ssize  file_size;
} ReadFileResult;

ReadFileResult os_read_entire_file(String path, Allocator allocator);
String         os_read_entire_file_as_string(String path, Allocator allocator);
ssize          os_get_file_size(String path, LinearArena *scratch);

#endif //FILE_H
