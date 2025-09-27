#ifndef ASSET_H
#define ASSET_H

#include "base/typedefs.h"

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
