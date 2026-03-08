#include "flipbook_animation.h"

#include "asset_table.h"
#include "base/utils.h"
#include "components/component.h"
#include "entity/entity.h"
#include "entity/entity_system.h"
#include "renderer/frontend/render_batch.h"
#include "world/world.h"

typedef struct AnimationTable {
    Flipbook animations[FLIPBOOK_COUNT];
} AnimationTable;

static AnimationTable g_animation_table;

static Flipbook animation_player_idle(void)
{
    Flipbook result = {0};

    SpriteRotationBehaviour rotate_behaviour = SPRITE_MIRROR_HORIZONTALLY_BASED_ON_DIR;
    Vector2 size = v2(32, 64);

    FlipbookFrame frame_1 = {
        sprite_create(texture_handle(PLAYER_IDLE1), size, rotate_behaviour), 0.75f};
    FlipbookFrame frame_2 = {
        sprite_create(texture_handle(PLAYER_IDLE2), size, rotate_behaviour), 0.75f};

    result.frames[result.frame_count++] = frame_1;
    result.frames[result.frame_count++] = frame_2;
    result.on_end_behaviour = ANIM_ON_END_REPEAT;

    return result;
}

static Flipbook animation_player_walking(void)
{
    Flipbook result = {0};

    SpriteRotationBehaviour rotate_behaviour = SPRITE_MIRROR_HORIZONTALLY_BASED_ON_DIR;
    Vector2 size = v2(32, 64);

    FlipbookFrame frame_1 = {
        sprite_create(texture_handle(PLAYER_WALKING1), size, rotate_behaviour), 0.5f};
    FlipbookFrame frame_2 = {
        sprite_create(texture_handle(PLAYER_WALKING2), size, rotate_behaviour), 0.5f};

    result.frames[result.frame_count++] = frame_1;
    result.frames[result.frame_count++] = frame_2;
    result.on_end_behaviour = ANIM_ON_END_REPEAT;

    return result;
}

static Flipbook animation_player_attack(void)
{
    Flipbook result = {0};

    SpriteRotationBehaviour rotate_behaviour = SPRITE_MIRROR_HORIZONTALLY_BASED_ON_DIR;
    Vector2 size = v2(32, 64);

    FlipbookFrame frame_1 = {
        sprite_create(texture_handle(PLAYER_ATTACK1), size, rotate_behaviour), 0.125f};

    FlipbookFrame frame_2 = {
        sprite_create(texture_handle(PLAYER_ATTACK2), size, rotate_behaviour), 0.125f};

    FlipbookFrame frame_3 = {
        sprite_create(texture_handle(PLAYER_ATTACK3), size, rotate_behaviour), 0.1f};

    result.frames[result.frame_count++] = frame_1;
    result.frames[result.frame_count++] = frame_2;
    result.frames[result.frame_count++] = frame_3;

    result.on_end_behaviour = ANIM_ON_END_TRANSITION_TO_STATE;
    result.state_transition_when_done = state_idle();

    return result;
}

void initialize_flipbook_animations(void)
{
    g_animation_table.animations[FLIPBOOK_PLAYER_IDLE] = animation_player_idle();
    g_animation_table.animations[FLIPBOOK_PLAYER_WALKING] = animation_player_walking();
    g_animation_table.animations[FLIPBOOK_PLAYER_ATTACKING] = animation_player_attack();
}

static Flipbook *anim_get_by_id(FlipbookID id)
{
    ASSERT(id != FLIPBOOK_NULL);
    ASSERT(id >= 0);
    ASSERT(id < ARRAY_COUNT(g_animation_table.animations));
    Flipbook *result = &g_animation_table.animations[id];

    return result;
}

void update_flipbook_animation(World *world, Entity *entity, PhysicsComponent *physics,
    FlipbookInstance *instance, f32 dt)
{
    Flipbook *anim = anim_get_by_id(instance->animation_id);

    ASSERT(instance->current_frame < anim->frame_count);
    FlipbookFrame curr_frame = anim->frames[instance->current_frame];

    SpriteComponent *sprite = es_get_or_add_component(entity, SpriteComponent);
    sprite->sprite = curr_frame.sprite;

    instance->current_frame_elapsed_time += dt * instance->animation_speed_factor;

    if (instance->current_frame_elapsed_time >= curr_frame.duration) {
        instance->current_frame_elapsed_time = 0.0f;

        instance->current_frame += 1;

        ASSERT(instance->current_frame <= anim->frame_count);

        if (instance->current_frame == anim->frame_count) {
            // Animations without special behaviour on animation end should stay on the last frame
            instance->current_frame = MIN(instance->current_frame, anim->frame_count - 1);

            switch (anim->on_end_behaviour) {
                case ANIM_ON_END_REPEAT: {
                    instance->current_frame = 0;
                } break;

                case ANIM_ON_END_TRANSITION_TO_STATE: {
                    entity_force_transition_to_state(
                        world, entity, physics, anim->state_transition_when_done);
                } break;

                default: {
                } break;
            }
        }
    }
}

FlipbookInstance begin_flipbook_animation(FlipbookID next_anim, f32 speed_factor)
{
    ASSERT(next_anim != FLIPBOOK_NULL);

    FlipbookInstance result = {0};

    result.animation_id = next_anim;
    result.current_frame = 0;
    result.current_frame_elapsed_time = 0;
    result.animation_speed_factor = speed_factor;

    return result;
}

f32 get_animation_duration(FlipbookID anim)
{
    Flipbook *a = anim_get_by_id(anim);

    f32 result = 0.0f;

    for (ssize i = 0; i < a->frame_count; ++i) {
        result += a->frames[i].duration;
    }

    return result;
}

FlipbookInstance begin_flipbook_animation_with_duration(
    FlipbookID anim, f32 duration, f32 speed_factor)
{
    f32 anim_duration = get_animation_duration(anim);
    f32 total_speed_factor = (anim_duration / duration) * speed_factor;

    FlipbookInstance result = begin_flipbook_animation(anim, total_speed_factor);

    return result;
}

FlipbookFrame get_current_flipbook_frame(FlipbookInstance *instance)
{
    Flipbook *anim = anim_get_by_id(instance->animation_id);
    ASSERT(instance->current_frame < anim->frame_count);
    FlipbookFrame frame = anim->frames[instance->current_frame];

    return frame;
}
