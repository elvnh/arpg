#ifndef ASSETS_H
#define ASSETS_H

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

// TODO: arena
typedef struct {
    AssetID  next_asset_id;
    Asset    registered_assets[MAX_REGISTERED_ASSETS];
} AssetSystem;

ShaderHandle   assets_register_shader(AssetSystem *assets, String path);
ShaderAsset   *assets_get_shader(AssetSystem *assets, ShaderHandle handle);
TextureHandle  assets_register_texture(AssetSystem *assets, String path);
TextureAsset  *assets_get_texture(AssetSystem *assets, TextureHandle handle);


#endif //ASSETS_H
