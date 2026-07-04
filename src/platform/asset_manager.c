#include "asset_manager.h"

#include "base/image.h"
#include "base/span.h"
#include "binary_asset.h"
#include "generated/binary_asset_paths.c"
#include "platform.h"
#include "renderer/backend/renderer_backend.h"

// TODO: remove old implementation and rename this file
// TODO: fix allocation of scratch arenas

static b32 create_texture_asset(AssetManager *mgr, BinaryTexture *texture, TextureHandle2 id)
{
    ASSERT(id >= 0);
    ASSERT(id < ARRAY_COUNT(mgr->textures));

    Image image = {0};
    image.data = texture->bitmap;
    image.width = texture->width;
    image.height = texture->height;
    image.channels = 4;

    TextureAsset2 *asset = renderer_backend_create_texture2(image, fl_allocator(&mgr->arena));

    b32 result = asset != 0;

    if (result) {
        mgr->textures[id] = asset;
    }

    return result;
}

b32 asset_mgr_initialize(AssetManager *mgr, Allocator allocator)
{
    mgr->arena = fl_create(allocator, MB(32));

    Allocator asset_allocator = fl_allocator(&mgr->arena);
    LinearArena scratch = la_create(asset_allocator, MB(16));
    Allocator scratch_allocator = la_allocator(&scratch);

    ssize asset_count = ARRAY_COUNT(binary_asset_files);

    for (ssize i = 0; i < asset_count; ++i) {
        // TODO: clean this mess up
        String relative_asset_path = str_from_c_str((char *)binary_asset_files[i]);
        String absolute_asset_path =
            str_concat(str("assets/"), relative_asset_path, allocator);
        absolute_asset_path =
            str_concat(str_concat(platform_get_executable_directory(allocator, &scratch),
                           str("/"), allocator),
                absolute_asset_path, allocator);

        Span asset_data =
            platform_read_entire_file(absolute_asset_path, scratch_allocator, &scratch);

        if (!asset_data.data) {
            ASSERT(0);
        } else {
            BinaryAsset *bin_asset = asset_data.data;

            BEGIN_EXHAUSTIVE_SWITCH;
            switch (bin_asset->kind) {
                case ASSET_TEXTURE: {
                    BinaryTexture *texture = (BinaryTexture *)bin_asset;
                    b32 create_result =
                        create_texture_asset(mgr, texture, bin_asset->asset_id);
                    ASSERT(create_result);
                } break;

                    INVALID_DEFAULT_CASE;
            }
            END_EXHAUSTIVE_SWITCH;
        }
    }

    la_destroy(&scratch);

    return 1;
}
