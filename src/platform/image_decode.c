#include "image_decode.h"
#include "stb_image.h"

#include <string.h>

Image image_decode_png(Span span, Allocator allocator)
{
    ASSERT(span.data);
    ASSERT(span.size > 0);

    s32 width, height, channels;
    byte *img_data = stbi_load_from_memory(span.data, (s32)span.size, &width, &height, &channels, 0);

    if (!img_data) {
        return (Image){0};
    }

    ssize image_size = width * height * channels;

    // Copy over data to new buffer that belongs to provided allocator
    byte *img_copy = allocate_array(allocator, byte, image_size);
    memcpy(img_copy, img_data, (usize)image_size);
    free(img_data);

    Image result = {
        .data = img_copy,
        .width = width,
        .height = height,
        .channels = channels
    };

    return result;
}
