#ifndef TEXTURES_H
#define TEXTURES_H

#include "base/linear_arena.h"
#include "base/string8.h"
#include "compiled_asset.h"
#include "value.h"

CompiledAssetList compile_textures(ValueList *textures, String assets_dir, LinearArena *arena);

#endif // TEXTURES_H
