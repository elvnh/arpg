#include "assets.h"
#include "base/linear_arena.h"
#include "base/thread_context.h"
#include "platform/file.h"
#include "renderer/renderer_backend.h"
#include "image.h"

#define ASSET_ARENA_SIZE MB(8)

static AssetID create_asset(AssetSystem *assets, AssetKind kind, void *data)
{
    AssetID id = assets->next_asset_id++;
    ASSERT(id < MAX_REGISTERED_ASSETS);

    Asset *asset = &assets->registered_assets[id];
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

static void *get_asset_data(AssetSystem *assets, AssetID id, AssetKind kind)
{
    ASSERT(id < assets->next_asset_id);

    Asset *asset = &assets->registered_assets[id];
    ASSERT(asset->kind == kind);

    switch (kind) {
	case ASSET_KIND_SHADER:  return asset->as.shader_asset;
	case ASSET_KIND_TEXTURE: return asset->as.texture_asset;
    }

    ASSERT(false);

    return 0;
}

static ShaderAsset *load_asset_data_shader(AssetSystem *assets, String path, LinearArena scratch)
{
    String shader_source = os_read_entire_file_as_string(path, la_allocator(&scratch));
    ShaderAsset *shader = renderer_backend_create_shader(shader_source, fl_allocator(&assets->asset_arena));

    return shader;
}

AssetSystem assets_initialize(Allocator parent_allocator)
{
    AssetSystem result = {0};
    result.asset_arena = fl_create(parent_allocator, ASSET_ARENA_SIZE);

    return result;
}

ShaderHandle assets_register_shader(AssetSystem *assets, String path, LinearArena scratch)
{
    ShaderAsset *shader = load_asset_data_shader(assets, path, scratch);
    ASSERT(shader);

    AssetID id = create_asset(assets, ASSET_KIND_SHADER, shader);

    return (ShaderHandle){id};
}

ShaderAsset *assets_get_shader(AssetSystem *assets, ShaderHandle handle)
{
    ShaderAsset *result = get_asset_data(assets, handle.id, ASSET_KIND_SHADER);

    return result;
}

static TextureAsset *load_asset_data_texture(AssetSystem *assets, String path, LinearArena scratch)
{
    Image image = img_load_png_from_file(path, la_allocator(&scratch), scratch);

    if (!image.data) {
	ASSERT(false);
	return 0;
    }

    TextureAsset *texture = renderer_backend_create_texture(image, fl_allocator(&assets->asset_arena));

    return texture;
}

TextureHandle  assets_register_texture(AssetSystem *assets, String path, LinearArena scratch)
{
    TextureAsset *texture = load_asset_data_texture(assets, path, scratch);
    ASSERT(texture);

    AssetID id = create_asset(assets, ASSET_KIND_TEXTURE, texture);

    return (TextureHandle){id};
}

TextureAsset *assets_get_texture(AssetSystem *assets, TextureHandle handle)
{
    TextureAsset *result = get_asset_data(assets, handle.id, ASSET_KIND_TEXTURE);

    return result;
}
