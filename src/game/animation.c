#include "animation.h"
#include "base/utils.h"
#include "components/component.h"
#include "entity.h"
#include "renderer/render_batch.h"

static Animation animation_player_idle(const AssetList *asset_table)
{
    Animation result = {0};

    AnimationFrame frame_1 = {asset_table->player_idle1, 2.75f};
    AnimationFrame frame_2 = {asset_table->player_idle2, 2.75f};

    result.frames[result.frame_count++] = frame_1;
    result.frames[result.frame_count++] = frame_2;
    result.is_repeating = true;

    result.rotation_behaviour = SPRITE_ROTATE_NONE;
    result.sprite_size = v2(32, 64);

    return result;
}

static Animation animation_player_walking(const AssetList *asset_table)
{
    Animation result = {0};

    AnimationFrame frame_1 = {asset_table->player_walking1, 0.5f};
    AnimationFrame frame_2 = {asset_table->player_walking2, 0.5f};

    result.frames[result.frame_count++] = frame_1;
    result.frames[result.frame_count++] = frame_2;
    result.is_repeating = true;

    result.rotation_behaviour = SPRITE_ROTATE_NONE;
    result.sprite_size = v2(32, 64);

    return result;
}

void anim_initialize(AnimationTable *table, const AssetList *asset_table)
{
    table->animations[ANIM_PLAYER_IDLE] = animation_player_idle(asset_table);
    table->animations[ANIM_PLAYER_WALKING] = animation_player_walking(asset_table);
}

Animation *anim_get_by_id(AnimationTable *table, AnimationID id)
{
    ASSERT(id != ANIM_NULL);
    ASSERT(id >= 0);
    ASSERT(id < ARRAY_COUNT(table->animations));
    Animation *result = &table->animations[id];

    return result;
}

void anim_update_instance(AnimationTable *anim_table, AnimationInstance *anim_instance, f32 dt)
{
    Animation *anim = anim_get_by_id(anim_table, anim_instance->animation_id);
    ASSERT(anim_instance->current_frame < anim->frame_count);

    AnimationFrame curr_frame = anim->frames[anim_instance->current_frame];

    anim_instance->current_frame_elapsed_time += dt;

    if (anim_instance->current_frame_elapsed_time >= curr_frame.duration) {
        anim_instance->current_frame_elapsed_time = 0.0f;

        anim_instance->current_frame += 1;

	ASSERT(anim_instance->current_frame <= anim->frame_count);

        if (anim_instance->current_frame == anim->frame_count) {
	    if (anim->is_repeating) {
		anim_instance->current_frame = 0;
	    } else if (anim->transition_to_animation_on_end != ANIM_NULL) {
		anim_transition_to_animation(anim_instance, anim->transition_to_animation_on_end);
	    }
        }

        // Animations without special behaviour on animation end  should stay on the last frame
        anim_instance->current_frame = MIN(anim_instance->current_frame, anim->frame_count - 1);
    }

}

void anim_render_instance(AnimationTable *anim_table, AnimationInstance *anim_instance,
    Entity *owning_entity, RenderBatch *rb, const AssetList *assets, LinearArena *scratch)
{
    Animation *anim = anim_get_by_id(anim_table, anim_instance->animation_id);
    AnimationFrame current_frame = anim->frames[anim_instance->current_frame];

    f32 rotation = 0.0f;
    f32 dir_angle = (f32)atan2(owning_entity->direction.y, owning_entity->direction.x);
    RectangleFlip flip = RECT_FLIP_NONE;

    if (anim->rotation_behaviour == SPRITE_ROTATE_BASED_ON_DIR) {
	rotation = dir_angle;
    } else if (anim->rotation_behaviour == SPRITE_MIRROR_HORIZONTALLY_BASED_ON_DIR) {
	b32 should_flip = (dir_angle > PI_2) || (dir_angle < -PI_2);

	if (should_flip) {
	    flip = RECT_FLIP_HORIZONTALLY;
	}
    }

    Rectangle sprite_rect = { owning_entity->position, anim->sprite_size };
    rb_push_sprite(rb, scratch, current_frame.texture, sprite_rect, rotation, flip,
	RGBA32_WHITE, assets->texture_shader, RENDER_LAYER_ENTITIES);
}

void anim_transition_to_animation(struct AnimationInstance *anim_instance, AnimationID next_anim)
{
    ASSERT(next_anim != ANIM_NULL);

    anim_instance->animation_id = next_anim;
    anim_instance->current_frame = 0;
    anim_instance->current_frame_elapsed_time = 0;
}
