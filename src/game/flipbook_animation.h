#ifndef FLIPBOOK_ANIMATION_H
#define FLIPBOOK_ANIMATION_H

#include "base/vector.h"
#include "entity/entity_state.h"
#include "sprite.h"

#define MAX_FLIPBOOK_FRAMES 8

struct AnimationInstance;
struct Entity;
struct RenderBatch;
struct LinearArena;
struct AnimationComponent;
struct PhysicsComponent;
struct World;

typedef enum {
    ANIM_ON_END_DO_NOTHING,
    ANIM_ON_END_REPEAT,
    ANIM_ON_END_TRANSITION_TO_STATE,
} AnimationOnEnd;

typedef enum {
    FLIPBOOK_NULL = 0,
    FLIPBOOK_PLAYER_IDLE,
    FLIPBOOK_PLAYER_WALKING,
    FLIPBOOK_PLAYER_ATTACKING,
    FLIPBOOK_COUNT,
} FlipbookID;

typedef struct {
    Sprite sprite;
    f32 duration;
} FlipbookFrame;

typedef struct {
    FlipbookFrame frames[MAX_FLIPBOOK_FRAMES];
    s32 frame_count;

    AnimationOnEnd on_end_behaviour;
    EntityState state_transition_when_done;
} Flipbook;

typedef struct {
    FlipbookID animation_id;
    s32 current_frame;
    f32 current_frame_elapsed_time;
    f32 animation_speed_factor;
} FlipbookInstance;

void initialize_flipbook_animations(void);
void update_flipbook_animation(struct World *world, struct Entity *entity,
    struct PhysicsComponent *physics, FlipbookInstance *instance, f32 dt);
FlipbookInstance begin_flipbook_animation(FlipbookID next_anim, f32 speed_factor);
FlipbookInstance begin_flipbook_animation_with_duration(
    FlipbookID anim, f32 duration, f32 speed_factor);
FlipbookFrame get_current_flipbook_frame(FlipbookInstance *anim_instance);

#endif //FLIPBOOK_ANIMATION_H
