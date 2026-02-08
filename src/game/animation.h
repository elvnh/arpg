#ifndef ANIMATION_H
#define ANIMATION_H

#include "base/vector.h"
#include "entity/entity_state.h"
#include "sprite.h"

#define MAX_ANIMATION_FRAMES 8

struct AnimationInstance;
struct Entity;
struct RenderBatch;
struct LinearArena;
struct AnimationComponent;
struct PhysicsComponent;
struct World;

typedef enum {
    ANIM_NULL = 0,
    ANIM_PLAYER_IDLE,
    ANIM_PLAYER_WALKING,
    ANIM_PLAYER_ATTACKING,
    ANIM_ANIMATION_COUNT,
} AnimationID;

typedef enum {
    ANIM_ON_END_DO_NOTHING,
    ANIM_ON_END_REPEAT,
    ANIM_ON_END_TRANSITION_TO_STATE,
} AnimationOnEnd;

typedef struct {
    Sprite sprite;
    f32 duration;
} AnimationFrame;

typedef struct {
    AnimationFrame frames[MAX_ANIMATION_FRAMES];
    s32 frame_count;

    AnimationOnEnd on_end_behaviour;
    EntityState state_transition_when_done;
} Animation;

typedef struct AnimationInstance {
    AnimationID animation_id;
    s32 current_frame;
    f32 current_frame_elapsed_time;
    f32 animation_speed_factor;
} AnimationInstance;

void anim_initialize(void);
void anim_update_instance(struct World *world, struct Entity *entity, struct PhysicsComponent *physics,
    struct AnimationInstance *anim_instance, f32 dt);
void anim_render_instance(struct AnimationInstance *anim_instance, struct PhysicsComponent *physics,
    struct RenderBatch *rb, struct LinearArena *scratch);
AnimationInstance anim_begin_animation(AnimationID next_anim, f32 speed_factor);
AnimationInstance anim_begin_animation_with_duration(AnimationID anim, f32 duration, f32 speed_factor);
AnimationFrame anim_get_current_frame(AnimationInstance *anim_instance);
//AnimationInstance *anim_get_current_animation(struct Entity *entity, struct AnimationComponent *anim_comp);

#endif //ANIMATION_H
