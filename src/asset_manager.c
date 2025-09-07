#include "asset_manager.h"
#include "base/linear_arena.h"
#include "base/utils.h"
#include "renderer/renderer_backend.h"
#include "base/image.h"
#include "platform.h"

#define ASSET_ARENA_SIZE MB(8)

static AssetID create_asset(AssetManager *assets, AssetKind kind, void *data)
{
    AssetID id = assets->next_asset_id++;
    ASSERT(id < MAX_REGISTERED_ASSETS);

    AssetSlot *asset = &assets->registered_assets[id];
    asset->kind = kind;

    switch (kind) {
	case ASSET_KIND_SHADER: {
	    asset->as.shader_asset = data;
	} break;

	case ASSET_KIND_TEXTURE: {
	    asset->as.texture_asset = data;
	} break;
    }

    return id;
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

static ShaderAsset *load_asset_data_shader(AssetManager *assets, String path, LinearArena *scratch)
{
    String shader_source = platform_read_entire_file_as_string(path, la_allocator(scratch), scratch);
    ShaderAsset *shader = renderer_backend_create_shader(shader_source, fl_allocator(&assets->asset_arena));

    return shader;
}

AssetManager assets_initialize(Allocator parent_allocator)
{
    AssetManager result = {0};
    result.asset_arena = fl_create(parent_allocator, ASSET_ARENA_SIZE);

    return result;
}

ShaderHandle assets_register_shader(AssetManager *assets, String path, LinearArena *scratch)
{
    ShaderAsset *shader = load_asset_data_shader(assets, path, scratch);
    ASSERT(shader);

    AssetID id = create_asset(assets, ASSET_KIND_SHADER, shader);

    return (ShaderHandle){id};
}

ShaderAsset *assets_get_shader(AssetManager *assets, ShaderHandle handle)
{
    ShaderAsset *result = get_asset_data(assets, handle.id, ASSET_KIND_SHADER);

    return result;
}

static TextureAsset *load_asset_data_texture(AssetManager *assets, String path, LinearArena *scratch)
{
    Span file_contents = platform_read_entire_file(path, la_allocator(scratch), scratch);
    Image image = image_decode_png(file_contents, la_allocator(scratch));

    TextureAsset *texture = renderer_backend_create_texture(image, fl_allocator(&assets->asset_arena));

    return texture;
}

TextureHandle  assets_register_texture(AssetManager *assets, String path, LinearArena *scratch)
{
    TextureAsset *texture = load_asset_data_texture(assets, path, scratch);
    ASSERT(texture);

    AssetID id = create_asset(assets, ASSET_KIND_TEXTURE, texture);

    return (TextureHandle){id};
}

TextureAsset *assets_get_texture(AssetManager *assets, TextureHandle handle)
{
    TextureAsset *result = get_asset_data(assets, handle.id, ASSET_KIND_TEXTURE);

    return result;
}
