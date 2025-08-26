#ifndef FILE_H
#define FILE_H

#include "base/string8.h"
#include "base/linear_arena.h"

typedef struct {
    byte  *file_data;
    ssize  file_size;
} ReadFileResult;

ReadFileResult platform_read_entire_file(String path, Allocator allocator);
ssize          platform_get_file_size(String path, LinearArena scratch);

#endif //FILE_H
