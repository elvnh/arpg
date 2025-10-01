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

typedef struct {
    ShaderHandle shader;
    ShaderHandle shader2;
    TextureHandle texture;
    TextureHandle white_texture;
    FontHandle default_font;
} AssetList;

#endif //ASSET_H
