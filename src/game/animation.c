#include "animation.h"
#include "base/utils.h"
#include "components/component.h"
#include "entity.h"
#include "entity_system.h"
#include "renderer/render_batch.h"
#include "asset_table.h"
#include "world.h"

static AnimationTable *g_animation_table;

static Animation animation_player_idle()
{
    Animation result = {0};

    SpriteRotationBehaviour rotate_behaviour = SPRITE_MIRROR_HORIZONTALLY_BASED_ON_DIR;
    Vector2 size = v2(32, 64);

    AnimationFrame frame_1 = {sprite_create(get_asset_table()->player_idle1, size, rotate_behaviour), 0.75f};
    AnimationFrame frame_2 = {sprite_create(get_asset_table()->player_idle2, size, rotate_behaviour), 0.75f};

    result.frames[result.frame_count++] = frame_1;
    result.frames[result.frame_count++] = frame_2;
    result.on_end_behaviour = ANIM_ON_END_REPEAT;

    return result;
}

static Animation animation_player_walking()
{
    Animation result = {0};

    SpriteRotationBehaviour rotate_behaviour = SPRITE_MIRROR_HORIZONTALLY_BASED_ON_DIR;
    Vector2 size = v2(32, 64);

    AnimationFrame frame_1 = {sprite_create(get_asset_table()->player_walking1, size, rotate_behaviour), 0.5f};
    AnimationFrame frame_2 = {sprite_create(get_asset_table()->player_walking2, size, rotate_behaviour), 0.5f};

    result.frames[result.frame_count++] = frame_1;
    result.frames[result.frame_count++] = frame_2;
    result.on_end_behaviour = ANIM_ON_END_REPEAT;

    return result;
}

static Animation animation_player_attack()
{
    Animation result = {0};

    SpriteRotationBehaviour rotate_behaviour = SPRITE_MIRROR_HORIZONTALLY_BASED_ON_DIR;
    Vector2 size = v2(32, 64);

    AnimationFrame frame_1 = {
	sprite_create(get_asset_table()->player_attack1, size, rotate_behaviour), 0.125f
    };

    AnimationFrame frame_2 = {
	sprite_create(get_asset_table()->player_attack2, size, rotate_behaviour), 0.125f
    };

    AnimationFrame frame_3 = {
	sprite_create(get_asset_table()->player_attack3, size, rotate_behaviour), 0.1f
    };

    result.frames[result.frame_count++] = frame_1;
    result.frames[result.frame_count++] = frame_2;
    result.frames[result.frame_count++] = frame_3;

    result.on_end_behaviour = ANIM_ON_END_TRANSITION_TO_STATE;
    result.state_transition_when_done = state_idle();

    return result;
}

void anim_initialize(AnimationTable *anim_table)
{
    anim_set_global_animation_table(anim_table);

    g_animation_table->animations[ANIM_PLAYER_IDLE] = animation_player_idle();
    g_animation_table->animations[ANIM_PLAYER_WALKING] = animation_player_walking();
    g_animation_table->animations[ANIM_PLAYER_ATTACKING] = animation_player_attack();
}

static Animation *anim_get_by_id(AnimationID id)
{
    ASSERT(id != ANIM_NULL);
    ASSERT(id >= 0);
    ASSERT(id < ARRAY_COUNT(g_animation_table->animations));
    Animation *result = &g_animation_table->animations[id];

    return result;
}

void anim_update_instance(World *world, Entity *entity, AnimationInstance *anim_instance, f32 dt)
{
    Animation *anim = anim_get_by_id(anim_instance->animation_id);
    ASSERT(anim_instance->current_frame < anim->frame_count);

    AnimationFrame curr_frame = anim->frames[anim_instance->current_frame];

    anim_instance->current_frame_elapsed_time += dt;

    if (anim_instance->current_frame_elapsed_time >= curr_frame.duration) {
        anim_instance->current_frame_elapsed_time = 0.0f;

        anim_instance->current_frame += 1;

	ASSERT(anim_instance->current_frame <= anim->frame_count);

        if (anim_instance->current_frame == anim->frame_count) {
	    // Animations without special behaviour on animation end should stay on the last frame
	    anim_instance->current_frame = MIN(anim_instance->current_frame, anim->frame_count - 1);

	    switch (anim->on_end_behaviour) {
		case ANIM_ON_END_REPEAT: {
		    anim_instance->current_frame = 0;
		} break;

		case ANIM_ON_END_TRANSITION_TO_STATE: {
		    entity_transition_to_state(world, entity, anim->state_transition_when_done);
		} break;

		default: {
		} break;
	    }

        }


    }

}

void anim_render_instance(AnimationInstance *anim_instance, Entity *owning_entity, RenderBatch *rb,
    LinearArena *scratch)
{
    AnimationFrame current_frame = anim_get_current_frame(anim_instance);

    SpriteModifiers sprite_mods = sprite_get_modifiers(owning_entity->direction,
	current_frame.sprite.rotation_behaviour);

    Rectangle sprite_rect = { owning_entity->position, current_frame.sprite.size };
    rb_push_sprite(rb, scratch, current_frame.sprite.texture, sprite_rect, sprite_mods,
	get_asset_table()->texture_shader, RENDER_LAYER_ENTITIES);
}

void anim_transition_to_animation(struct AnimationInstance *anim_instance, AnimationID next_anim)
{
    ASSERT(next_anim != ANIM_NULL);

    anim_instance->animation_id = next_anim;
    anim_instance->current_frame = 0;
    anim_instance->current_frame_elapsed_time = 0;
}

AnimationFrame anim_get_current_frame(AnimationInstance *anim_instance)
{
    Animation *anim = anim_get_by_id(anim_instance->animation_id);
    ASSERT(anim_instance->current_frame < anim->frame_count);
    AnimationFrame frame = anim->frames[anim_instance->current_frame];

    return frame;
}

/* AnimationInstance *anim_get_current_animation(Entity *entity, AnimationComponent *anim_comp) */
/* { */
/*     ASSERT(entity->state < ENTITY_STATE_COUNT); */
/*     ASSERT(entity); */
/*     ASSERT(anim_comp); */
/*     ASSERT(es_has_component(entity, AnimationComponent)); */

/*     AnimationInstance *result = &anim_comp->state_animations[entity->state]; */

/*     return result; */
/* } */

void anim_set_global_animation_table(AnimationTable *anim_table)
{
    g_animation_table = anim_table;
}
