#ifndef ASSET_H
#define ASSET_H

#include "base/typedefs.h"

#define NULL_ASSET_ID  0
#define NULL_TEXTURE   ((TextureHandle) {NULL_ASSET_ID})
#define NULL_FONT      ((FontHandle)    {NULL_ASSET_ID})

typedef u32 AssetID;

typedef struct {
    AssetID id;
} ShaderHandle;

typedef struct {
    AssetID id;
} TextureHandle;

typedef struct {
    AssetID id;
} FontHandle;

typedef struct AssetList {
    ShaderHandle texture_shader;
    ShaderHandle shape_shader;
    TextureHandle default_texture;
    TextureHandle fireball_texture;
    TextureHandle spark_texture;
    FontHandle default_font;

    TextureHandle player_idle1;
    TextureHandle player_idle2;
} AssetList;

#endif //ASSET_H
