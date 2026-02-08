#include "asset_table.h"
#include "platform/asset.h"
#include "platform/asset_system.h"

static AssetTable *g_asset_table;

void set_global_asset_table(AssetTable *assets)
{
    ASSERT(assets);
    g_asset_table = assets;
}

TextureHandle texture_handle_impl(GameAsset asset)
{
    return get_texture_handle_from_table(g_asset_table, asset);
}

ShaderHandle shader_handle_impl(GameAsset asset)
{
    return get_shader_handle_from_table(g_asset_table, asset);
}

FontHandle font_handle_impl(GameAsset asset)
{
    return get_font_handle_from_table(g_asset_table, asset);
}
