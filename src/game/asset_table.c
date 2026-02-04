#include "asset_table.h"
#include "base/utils.h"

static AssetTable *g_asset_table;

AssetTable *get_asset_table(void)
{
    ASSERT(g_asset_table);
    return g_asset_table;
}

void set_global_asset_table(AssetTable *assets)
{
    ASSERT(assets);
    g_asset_table = assets;
}
