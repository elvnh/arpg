#ifndef IMAGE_DECODE_H
#define IMAGE_DECODE_H

#include "base/allocator.h"
#include "base/image.h"
#include "base/span.h"

Image image_decode_png(Span span, Allocator allocator);

#endif //IMAGE_DECODE_H
