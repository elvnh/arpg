#include <string.h>

#include "image.h"
#include "base/linear_arena.h"
#include "base/utils.h"
#include "os/thread_context.h"
#include "os/file.h"

#include "stb_image.h"

Image img_load_png_from_memory(void *data, s32 size, Allocator allocator)
{
    ASSERT(data);
    ASSERT(size > 0);

    s32 width, height, channels;
    byte *img_data = stbi_load_from_memory(data, size, &width, &height, &channels, 0);
    ASSERT(img_data);

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

Image img_load_png_from_file(String path, Allocator allocator)
{
    LinearArena scratch_arena = thread_ctx_get_temp_arena();
    Allocator scratch = arena_create_allocator(&scratch_arena);

    String terminated = str_null_terminate(path, scratch);
    ReadFileResult file = os_read_entire_file(terminated, scratch);

    return img_load_png_from_memory(file.file_data, cast_ssize_to_s32(file.file_size), allocator);
}
