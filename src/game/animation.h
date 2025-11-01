#ifndef ANIMATION_H
#define ANIMATION_H

#include "asset.h"

#define MAX_ANIMATION_FRAMES 8

typedef enum {
    ANIM_PLAYER_IDLE,
    ANIM_ANIMATION_COUNT,
} AnimationID;

typedef struct {
    TextureHandle texture;
    f32 duration;
} AnimationFrame;

typedef struct {
    AnimationFrame frames[MAX_ANIMATION_FRAMES];
    s32 frame_count;
    b32 is_repeating;
} Animation;

typedef struct {
    Animation animations[ANIM_ANIMATION_COUNT];
} AnimationTable;

void anim_initialize(AnimationTable *table, const AssetList *asset_table);
Animation *anim_get_by_id(AnimationTable *table, AnimationID id);

#endif //ANIMATION_H
