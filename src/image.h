#ifndef IMAGE_H
#define IMAGE_H

#include "base/allocator.h"
#include "base/string8.h"
#include "base/linear_arena.h"

typedef struct {
    byte  *data;
    s32    width;
    s32    height;
    s32    channels;
} Image;

Image img_load_png_from_memory(void *data, s32 size, Allocator allocator);
Image img_load_png_from_file(String path, Allocator allocator, LinearArena scratch_arena);

#endif //IMAGE_H
