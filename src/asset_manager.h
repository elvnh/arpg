#ifndef ASSETS_H
#define ASSETS_H

#include "asset.h"
#include "base/free_list_arena.h"
#include "base/linear_arena.h"
#include "base/string8.h"
#include "base/image.h"
#include "renderer/renderer_backend.h"

#define MAX_REGISTERED_ASSETS 256

typedef enum {
    ASSET_KIND_SHADER,
    ASSET_KIND_TEXTURE,
} AssetKind;

typedef struct {
    AssetKind kind;

    union {
	ShaderAsset  *shader_asset;
	TextureAsset *texture_asset;
    } as;
} AssetSlot;

typedef struct {
    AssetID       next_asset_id;
    AssetSlot    registered_assets[MAX_REGISTERED_ASSETS];
    FreeListArena asset_arena;
} AssetManager;

AssetManager   assets_initialize(Allocator parent_allocator);
ShaderHandle   assets_register_shader(AssetManager *assets, String path, LinearArena *scratch);
ShaderAsset   *assets_get_shader(AssetManager *assets, ShaderHandle handle);
TextureHandle  assets_register_texture(AssetManager *assets, String path, LinearArena *scratch);
TextureAsset  *assets_get_texture(AssetManager *assets, TextureHandle handle);

#endif //ASSETS_H
