#ifndef SHARED_H
#define SHARED_H

#include "base/typedefs.h"

typedef u32 AssetID;

typedef struct {
    AssetID id;
} ShaderHandle;

typedef struct {
    AssetID id;
} TextureHandle;

typedef struct {
    ShaderHandle shader;
    ShaderHandle shader2;
    TextureHandle texture;
    TextureHandle white_texture;
} AssetList;

#endif //SHARED_H
