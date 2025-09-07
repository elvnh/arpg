#ifndef IMAGE_H
#define IMAGE_H

#include "typedefs.h"
#include "span.h"
#include "allocator.h"

typedef struct {
    byte  *data;
    s32    width;
    s32    height;
    s32    channels;
} Image;

Image image_decode_png(Span span, Allocator allocator);

#endif //IMAGE_H
