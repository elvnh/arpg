#include "asset_manager.h"
#include "base/linear_arena.h"
#include "base/string8.h"
#include "base/utils.h"
#include "renderer/renderer_backend.h"
#include "base/image.h"
#include "platform.h"

#define ASSET_ARENA_SIZE MB(8)

typedef struct {
    AssetID    id;
    AssetSlot *slot;
} AssetSlotWithID;

static AssetSlotWithID acquire_asset_slot(AssetManager *assets, String asset_path)
{
    AssetID id = assets->next_asset_id++;
    ASSERT(id < MAX_REGISTERED_ASSETS);

    AssetSlot *slot = &assets->registered_assets[id];
    slot->asset_path = str_copy(asset_path, fl_allocator(&assets->asset_arena));

    AssetSlotWithID result = {
        .id = id,
        .slot = slot
    };

    return result;
}

static void assign_asset_slot_data(AssetSlot *slot, AssetKind kind, void *data)
{
    slot->kind = kind;

    switch (kind) {
        case ASSET_KIND_TEXTURE: {
            slot->as.texture_asset = data;
        } break;

        case ASSET_KIND_SHADER: {
            slot->as.shader_asset = data;
        } break;

        INVALID_DEFAULT_CASE;
    }
}

static void *get_asset_data(AssetManager *assets, AssetID id, AssetKind kind)
{
    ASSERT(id < assets->next_asset_id);

    AssetSlot *asset = &assets->registered_assets[id];
    ASSERT(asset->kind == kind);

    switch (kind) {
	case ASSET_KIND_SHADER:  return asset->as.shader_asset;
	case ASSET_KIND_TEXTURE: return asset->as.texture_asset;
    }

    ASSERT(false);

    return 0;
}


AssetManager assets_initialize(Allocator parent_allocator)
{
    AssetManager result = {0};
    result.asset_arena = fl_create(parent_allocator, ASSET_ARENA_SIZE);

    return result;
}

AssetSlot *assets_get_asset_by_path(AssetManager *assets, String path)
{
    // TODO: if number of assets grow large, linear search could become slow, but should be fine for now
    for (ssize i = 0; i < assets->next_asset_id; ++i) {
        AssetSlot *slot = &assets->registered_assets[i];

        if (str_equal(slot->asset_path, path)) {
            return slot;
        }
    }

    return 0;
}


static ShaderAsset *load_asset_data_shader(AssetManager *assets, String path, LinearArena *scratch)
{
    ShaderAsset *result = 0;
    String shader_source = platform_read_entire_file_as_string(path, la_allocator(scratch), scratch);

    if (shader_source.data) {
        ShaderAsset *shader = renderer_backend_create_shader(shader_source, fl_allocator(&assets->asset_arena));

        result = shader;
    }

    return result;
}

ShaderHandle assets_register_shader(AssetManager *assets, String path, LinearArena *scratch)
{
    AssetSlotWithID slot_and_id = acquire_asset_slot(assets, path);
    ShaderAsset *shader = load_asset_data_shader(assets, path, scratch);
    ASSERT(shader);

    assign_asset_slot_data(slot_and_id.slot, ASSET_KIND_SHADER, shader);

    return (ShaderHandle){slot_and_id.id};
}

static TextureAsset *load_asset_data_texture(AssetManager *assets, String path, LinearArena *scratch)
{
    TextureAsset *result = 0;
    Span file_contents = platform_read_entire_file(path, la_allocator(scratch), scratch);

    if (file_contents.data && file_contents.size) {
        Image image = image_decode_png(file_contents, la_allocator(scratch));

        if (image.data) {
            TextureAsset *texture = renderer_backend_create_texture(image, fl_allocator(&assets->asset_arena));

            result = texture;
        }
    }

    return result;
}

TextureHandle assets_register_texture(AssetManager *assets, String path, LinearArena *scratch)
{
    AssetSlotWithID slot_and_id = acquire_asset_slot(assets, path);
    TextureAsset *texture = load_asset_data_texture(assets, path, scratch);
    ASSERT(texture);

    assign_asset_slot_data(slot_and_id.slot, ASSET_KIND_TEXTURE, texture);

    return (TextureHandle){slot_and_id.id};
}

ShaderAsset *assets_get_shader(AssetManager *assets, ShaderHandle handle)
{
    ShaderAsset *result = get_asset_data(assets, handle.id, ASSET_KIND_SHADER);

    return result;
}

TextureAsset *assets_get_texture(AssetManager *assets, TextureHandle handle)
{
    TextureAsset *result = get_asset_data(assets, handle.id, ASSET_KIND_TEXTURE);

    return result;
}

static b32 assets_reload_shader(AssetManager *assets, AssetSlot *slot, LinearArena *scratch)
{
    ShaderAsset *new_shader = load_asset_data_shader(assets, slot->asset_path, scratch);

    if (new_shader) {
        renderer_backend_destroy_shader(slot->as.shader_asset, fl_allocator(&assets->asset_arena));
        assign_asset_slot_data(slot, ASSET_KIND_SHADER, new_shader);

        return true;
    }

    return false;
}

static b32 assets_reload_texture(AssetManager *assets, AssetSlot *slot, LinearArena *scratch)
{
    TextureAsset *new_texture = load_asset_data_texture(assets, slot->asset_path, scratch);

    if (new_texture) {
        renderer_backend_destroy_texture(slot->as.texture_asset, fl_allocator(&assets->asset_arena));
        assign_asset_slot_data(slot, ASSET_KIND_TEXTURE, new_texture);

        return true;
    }

    return false;
}

b32 assets_reload_asset(AssetManager *assets, AssetSlot *slot, LinearArena *scratch)
{
    switch (slot->kind) {
        case ASSET_KIND_SHADER: return assets_reload_shader(assets, slot, scratch);

        case ASSET_KIND_TEXTURE: return assets_reload_texture(assets, slot, scratch);

        INVALID_DEFAULT_CASE;
    }

    return false;
}
