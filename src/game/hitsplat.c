#include "hitsplat.h"
#include "base/format.h"
#include "world.h"
#include "renderer/render_batch.h"

void hitsplats_update(World *world, const FrameData *frame_data)
{
    for (s32 i = 0; i < world->hitsplat_count; ++i) {
        Hitsplat *hitsplat = &world->active_hitsplats[i];

        if (hitsplat->timer >= hitsplat->lifetime) {
            world->active_hitsplats[i] = world->active_hitsplats[world->hitsplat_count - 1];
            world->hitsplat_count -= 1;

            if (world->hitsplat_count == 0) {
                break;
            }
        }

        hitsplat->position = v2_add(hitsplat->position, v2_mul_s(hitsplat->velocity, frame_data->dt));
        hitsplat->timer += frame_data->dt;
    }
}

void hitsplats_render(World *world, RenderBatch *rb, LinearArena *frame_arena)
{
    for (s32 i = 0; i < world->hitsplat_count; ++i) {
        Hitsplat *hitsplat = &world->active_hitsplats[i];

	for (DamageKind type = 0; type < DMG_KIND_COUNT; ++type) {
	    DamageValue value_of_type = get_damage_value_of_type(hitsplat->damage.types, type);
	    ASSERT(value_of_type >= 0);

	    if (value_of_type > 0) {
		String damage_str = ssize_to_string(value_of_type, la_allocator(frame_arena));

		f32 alpha = 1.0f - hitsplat->timer / hitsplat->lifetime;
		RGBA32 color = {0};

		switch (type) {
		    case DMG_KIND_FIRE: {
			color = RGBA32_RED;
		    } break;

		    case DMG_KIND_LIGHTNING: {
			color = RGBA32_YELLOW;
		    } break;

			INVALID_DEFAULT_CASE;
		}

		color.a = alpha;

		rb_push_text(
		    rb, frame_arena, damage_str, hitsplat->position, color, 32,
		    get_asset_table()->texture_shader, get_asset_table()->default_font, 5);
	    }
	}
    }
}

void hitsplat_create(World *world, Vector2 position, DamageInstance damage)
{
    ASSERT(world->hitsplat_count < ARRAY_COUNT(world->active_hitsplats));

    // TODO: randomize size and position
    // TODO: use function for updating position instead
    Vector2 velocity = rng_direction(PI * 2);
    velocity = v2_mul_s(velocity, 20.0f);

    Hitsplat hitsplat = {
        .damage = damage,
        .position = position,
        .velocity = velocity,
        .lifetime = 2.0f,
    };

    s32 index = world->hitsplat_count++;
    world->active_hitsplats[index] = hitsplat;
}
