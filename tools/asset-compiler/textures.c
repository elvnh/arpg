#include "textures.h"

#include "base/image.h"
#include "base/sl_list.h"
#include "base/span.h"
#include "platform/platform.h"
#include "stb_image.h"

#include <string.h>

static Image make_bitmap(Span span, LinearArena *arena)
{
    ASSERT(span.data);
    ASSERT(span.size > 0);

    s32 width = 0, height = 0, channels = 0;
    byte *img_data =
        stbi_load_from_memory(span.data, (s32)span.size, &width, &height, &channels, 4);

    if (!img_data) {
        return (Image){0};
    }

    ssize image_size = width * height * channels;

    byte *img_copy = la_allocate_array(arena, byte, image_size);
    memcpy(img_copy, img_data, (usize)image_size);
    free(img_data);

    Image result = {.data = img_copy, .width = width, .height = height, .channels = channels};

    return result;
}

static void validate_texture_record(Record *rec)
{
    // TODO: proper errors
    if (!record_has_field_of_type(rec, str("name"), VALUE_IDENTIFIER)) {
        ASSERT(0 && "No texture name")
    }

    if (!record_has_field_of_type(rec, str("file"), VALUE_STRING)) {
        ASSERT(0 && "No texture file")
    }
}

CompiledAssetList compile_textures(ValueList *textures, String assets_dir, LinearArena *arena)
{
    CompiledAssetList result = {0};
    Allocator allocator = la_allocator(arena);

    String texture_dir = platform_make_relative_to(assets_dir, str("sprites"), allocator);

    for (Value *val = list_head(textures); val; val = list_next(val)) {
        Record *rec = value_as_record(val);

        validate_texture_record(rec);

        Value *name_attr = record_get(rec, str("name"));
        Value *file_attr = record_get(rec, str("file"));

        String *name = value_as_identifier(name_attr);
        String *file = value_as_string(file_attr);

        String relative_path = platform_make_relative_to(texture_dir, *file, allocator);
        String abs_path = platform_get_canonical_path(relative_path, allocator, arena);

        Span image_file = platform_read_entire_file(abs_path, allocator, arena);

        if (!image_file.data) {
            ASSERT(0 && "Failed to load image file");
        } else {
            Image bitmap = make_bitmap(image_file, arena);

            if (!bitmap.data) {
                ASSERT(0 && "Failed to create bitmap");
            } else {
                ASSERT(bitmap.channels == 4);

                // TODO: helper for allocating with flexible array member
                ssize bitmap_size = bitmap.width * bitmap.height * 4; // Channels are always 4
                ssize asset_size = SIZEOF(SerializedTexture) + bitmap_size;

                SerializedTexture *serialized_texture =
                    allocate_aligned(allocator, 1, asset_size, ALIGNOF(SerializedTexture));
                serialized_texture->header = (AssetHeader){ASSET_TEXTURE};
                serialized_texture->width = bitmap.width;
                serialized_texture->height = bitmap.height;

                memcpy(serialized_texture->bitmap, bitmap.data, ssize_to_usize(bitmap_size));

                CompiledAsset *asset = la_allocate_item(arena, CompiledAsset);
                asset->name = *name;
                asset->path = *file;
                asset->asset = &serialized_texture->header;

                sl_list_push_back(&result, asset);
            }
        }
    }

    return result;
}
