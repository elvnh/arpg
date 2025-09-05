#include "assets.h"
#include "os/thread_context.h"
#include "os/file.h"
#include "render/backend/renderer_backend.h"
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

static ShaderAsset *load_asset_data_shader(AssetSystem *assets, String path)
{
    Allocator scratch = thread_ctx_get_allocator();
    String shader_source = os_read_entire_file_as_string(path, scratch);
    ShaderAsset *shader = renderer_backend_create_shader(shader_source, fl_allocator(&assets->asset_arena));

    return shader;
}

AssetSystem assets_initialize(Allocator parent_allocator)
{
    AssetSystem result = {0};
    result.asset_arena = fl_create(parent_allocator, ASSET_ARENA_SIZE);

    return result;
}

ShaderHandle assets_register_shader(AssetSystem *assets, String path)
{
    ShaderAsset *shader = load_asset_data_shader(assets, path);
    ASSERT(shader);

    AssetID id = create_asset(assets, ASSET_KIND_SHADER, shader);

    return (ShaderHandle){id};
}

ShaderAsset *assets_get_shader(AssetSystem *assets, ShaderHandle handle)
{
    ShaderAsset *result = get_asset_data(assets, handle.id, ASSET_KIND_SHADER);

    return result;
}

static TextureAsset *load_asset_data_texture(AssetSystem *assets, String path)
{
    Allocator scratch = thread_ctx_get_allocator();
    Image image = img_load_png_from_file(path, scratch);

    if (!image.data) {
	ASSERT(false);
	return 0;
    }

    TextureAsset *texture = renderer_backend_create_texture(image, fl_allocator(&assets->asset_arena));

    return texture;
}

TextureHandle assets_register_texture(AssetSystem *assets, String path)
{
    TextureAsset *texture = load_asset_data_texture(assets, path);
    ASSERT(texture);

    AssetID id = create_asset(assets, ASSET_KIND_TEXTURE, texture);

    return (TextureHandle){id};
}

TextureAsset *assets_get_texture(AssetSystem *assets, TextureHandle handle)
{
    TextureAsset *result = get_asset_data(assets, handle.id, ASSET_KIND_TEXTURE);

    return result;
}
