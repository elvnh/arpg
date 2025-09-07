#ifndef IMAGE_DECODE_H
#define IMAGE_DECODE_H

#include "base/allocator.h"
#include "base/string8.h"
#include "base/linear_arena.h"
#include "base/span.h"
#include "base/image.h"

// TODO: shouldn't be in platform layer

Image platform_decode_png(Span span, Allocator allocator);

#endif //IMAGE_DECODE_H
