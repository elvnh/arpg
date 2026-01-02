#ifndef ANIMATION_H
#define ANIMATION_H

#include "base/vector.h"
#include "asset.h"
#include "sprite.h"

#define MAX_ANIMATION_FRAMES 8

struct AnimationInstance;
struct Entity;
struct RenderBatch;
struct LinearArena;
struct AssetList;

typedef enum {
    ANIM_NULL = 0,
    ANIM_PLAYER_IDLE,
    ANIM_PLAYER_WALKING,
    ANIM_ANIMATION_COUNT,
} AnimationID;

typedef struct {
    Sprite sprite;
    f32 duration;
} AnimationFrame;

typedef struct {
    AnimationFrame frames[MAX_ANIMATION_FRAMES];
    s32 frame_count;

    b32 is_repeating;
    AnimationID transition_to_animation_on_end;
} Animation;

typedef struct AnimationInstance {
    AnimationID animation_id;
    s32 current_frame;
    f32 current_frame_elapsed_time;
} AnimationInstance;

typedef struct {
    Animation animations[ANIM_ANIMATION_COUNT];
} AnimationTable;

void anim_initialize(AnimationTable *table, const AssetList *asset_table);
Animation *anim_get_by_id(AnimationTable *table, AnimationID id);
void anim_update_instance(AnimationTable *anim_table, struct AnimationInstance *anim_instance, f32 dt);
void anim_render_instance(AnimationTable *anim_table, struct AnimationInstance *anim_instance,
    struct Entity *owning_entity, struct RenderBatch *rb, const struct AssetList *assets,
    struct LinearArena *scratch);
void anim_transition_to_animation(struct AnimationInstance *anim_instance, AnimationID next_anim);

#endif //ANIMATION_H
