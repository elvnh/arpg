#ifndef ASSETS_H
#define ASSETS_H

#include "base/free_list_arena.h"
#include "base/linear_arena.h"
#include "base/string8.h"

#define MAX_REGISTERED_ASSETS 256

typedef struct ShaderAsset  ShaderAsset;
typedef struct TextureAsset TextureAsset;

typedef u32 AssetID;

typedef struct {
    AssetID id;
} ShaderHandle;

typedef struct {
    AssetID id;
} TextureHandle;

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
} Asset;

// TODO: free list arena
typedef struct {
    AssetID       next_asset_id;
    Asset         registered_assets[MAX_REGISTERED_ASSETS];
    FreeListArena asset_arena;
} AssetSystem;

AssetSystem    assets_initialize(Allocator parent_allocator);
ShaderHandle   assets_register_shader(AssetSystem *assets, String path, LinearArena scratch);
ShaderAsset   *assets_get_shader(AssetSystem *assets, ShaderHandle handle);
TextureHandle  assets_register_texture(AssetSystem *assets, String path, LinearArena scratch);
TextureAsset  *assets_get_texture(AssetSystem *assets, TextureHandle handle);


#endif //ASSETS_H
