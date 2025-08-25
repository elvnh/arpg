#ifndef FILE_H
#define FILE_H

#include "base/string8.h"
#include "base/linear_arena.h"

typedef struct {
    byte  *file_data;
    ssize  file_size;
} ReadFileResult;

ReadFileResult Platform_ReadEntireFile(String path, Allocator allocator);
ssize          Platform_GetFileSize(String path, LinearArena scratch);

#endif //FILE_H
