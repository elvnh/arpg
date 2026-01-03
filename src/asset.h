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

#endif //ASSET_H
