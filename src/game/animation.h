#ifndef ANIMATION_H
#define ANIMATION_H

#include "base/vector.h"
#include "asset_table.h"
#include "sprite.h"

#define MAX_ANIMATION_FRAMES 8

struct AnimationInstance;
struct Entity;
struct RenderBatch;
struct LinearArena;
struct AnimationComponent;

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

typedef struct AnimationTable {
    Animation animations[ANIM_ANIMATION_COUNT];
} AnimationTable;

void anim_initialize(AnimationTable *anim_table);
void anim_update_instance(struct AnimationInstance *anim_instance, f32 dt);
void anim_render_instance(struct AnimationInstance *anim_instance, struct Entity *owning_entity,
    struct RenderBatch *rb, struct LinearArena *scratch);
void anim_transition_to_animation(struct AnimationInstance *anim_instance, AnimationID next_anim);
AnimationFrame anim_get_current_frame(AnimationInstance *anim_instance);
AnimationInstance *anim_get_current_animation(struct Entity *entity, struct AnimationComponent *anim_comp);
void anim_set_global_animation_table(AnimationTable *anim_table);

#endif //ANIMATION_H
