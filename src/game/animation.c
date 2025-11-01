#include "animation.h"
#include "base/utils.h"

static Animation animation_player_idle(const AssetList *asset_table)
{
    Animation result = {0};

    AnimationFrame frame_1 = {asset_table->player_idle1, 1.0f};
    AnimationFrame frame_2 = {asset_table->player_idle2, 1.0f};

    result.frames[result.frame_count++] = frame_1;
    result.frames[result.frame_count++] = frame_2;
    result.is_repeating = true;

    return result;
}

void anim_initialize(AnimationTable *table, const AssetList *asset_table)
{
    table->animations[ANIM_PLAYER_IDLE] = animation_player_idle(asset_table);
}

Animation *anim_get_by_id(AnimationTable *table, AnimationID id)
{
    ASSERT(id >= 0);
    ASSERT(id < ARRAY_COUNT(table->animations));
    Animation *result = &table->animations[id];

    return result;
}
