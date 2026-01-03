#include "animation.h"
#include "base/utils.h"
#include "components/component.h"
#include "entity.h"
#include "entity_system.h"
#include "renderer/render_batch.h"

static Animation animation_player_idle(const AssetList *asset_table)
{
    Animation result = {0};

    SpriteRotationBehaviour rotate_behaviour = SPRITE_MIRROR_HORIZONTALLY_BASED_ON_DIR;
    Vector2 size = v2(32, 64);

    AnimationFrame frame_1 = {sprite_create(asset_table->player_idle1, size, rotate_behaviour), 2.75f};
    AnimationFrame frame_2 = {sprite_create(asset_table->player_idle2, size, rotate_behaviour), 2.75f};

    result.frames[result.frame_count++] = frame_1;
    result.frames[result.frame_count++] = frame_2;
    result.is_repeating = true;

    return result;
}

static Animation animation_player_walking(const AssetList *asset_table)
{
    Animation result = {0};

    SpriteRotationBehaviour rotate_behaviour = SPRITE_MIRROR_HORIZONTALLY_BASED_ON_DIR;
    Vector2 size = v2(32, 64);

    AnimationFrame frame_1 = {sprite_create(asset_table->player_walking1, size, rotate_behaviour), 2.75f};
    AnimationFrame frame_2 = {sprite_create(asset_table->player_walking2, size, rotate_behaviour), 2.75f};

    result.frames[result.frame_count++] = frame_1;
    result.frames[result.frame_count++] = frame_2;
    result.is_repeating = true;

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

        // Animations without special behaviour on animation end should stay on the last frame
        anim_instance->current_frame = MIN(anim_instance->current_frame, anim->frame_count - 1);
    }

}

void anim_render_instance(AnimationTable *anim_table, AnimationInstance *anim_instance,
    Entity *owning_entity, RenderBatch *rb, const AssetList *assets, LinearArena *scratch)
{
    AnimationFrame current_frame = anim_get_current_frame(anim_instance, anim_table);

    SpriteModifiers sprite_mods = sprite_get_modifiers(owning_entity, current_frame.sprite.rotation_behaviour);

    Rectangle sprite_rect = { owning_entity->position, current_frame.sprite.size };
    rb_push_sprite(rb, scratch, current_frame.sprite.texture, sprite_rect, sprite_mods.rotation,
	sprite_mods.flip, RGBA32_WHITE, assets->texture_shader, RENDER_LAYER_ENTITIES);
}

void anim_transition_to_animation(struct AnimationInstance *anim_instance, AnimationID next_anim)
{
    ASSERT(next_anim != ANIM_NULL);

    anim_instance->animation_id = next_anim;
    anim_instance->current_frame = 0;
    anim_instance->current_frame_elapsed_time = 0;
}

AnimationFrame anim_get_current_frame(AnimationInstance *anim_instance,
    AnimationTable *anim_table)
{
    Animation *anim = anim_get_by_id(anim_table, anim_instance->animation_id);
    ASSERT(anim_instance->current_frame < anim->frame_count);
    AnimationFrame frame = anim->frames[anim_instance->current_frame];

    return frame;
}

AnimationInstance *anim_get_current_animation(Entity *entity, AnimationComponent *anim_comp)
{
    ASSERT(entity->state < ENTITY_STATE_COUNT);
    ASSERT(entity);
    ASSERT(anim_comp);
    ASSERT(es_has_component(entity, AnimationComponent));

    AnimationInstance *result = &anim_comp->state_animations[entity->state];

    return result;
}
