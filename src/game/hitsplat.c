#include "hitsplat.h"
#include "base/format.h"
#include "base/ring_buffer.h"
#include "damage.h"
#include "world/world.h"
#include "renderer/frontend/render_batch.h"
#include "asset_table.h"
#include "platform/input.h"

void hitsplats_update(World *world, const FrameData *frame_data)
{
    for (s32 i = 0; i < ring_length(&world->active_hitsplats); ++i) {
        Hitsplat *hitsplat = ring_at(&world->active_hitsplats, i);

        if (hitsplat->timer >= hitsplat->lifetime) {
            ring_swap_remove(&world->active_hitsplats, i);

            if (ring_length(&world->active_hitsplats) == 0) {
                break;
            }
        }

        hitsplat->position = v2_add(hitsplat->position, v2_mul_s(hitsplat->velocity, frame_data->dt));
        hitsplat->timer += frame_data->dt;
    }
}

void hitsplats_render(World *world, RenderBatch *rb, LinearArena *frame_arena)
{
    for (s32 i = 0; i < ring_length(&world->active_hitsplats); ++i) {
        Hitsplat *hitsplat = ring_at(&world->active_hitsplats, i);

	ASSERT(hitsplat->damage.value > 0);

        String damage_str = format(frame_arena, "%ld", hitsplat->damage.value);

	f32 alpha = 1.0f - hitsplat->timer / hitsplat->lifetime;
	RGBA32 color = get_damage_type_color(hitsplat->damage.type);

	switch (hitsplat->damage.type) {
	    case DMG_TYPE_Fire: {
		color = RGBA32_RED;
	    } break;

	    case DMG_TYPE_Lightning: {
		color = RGBA32_YELLOW;
	    } break;

	    INVALID_DEFAULT_CASE;
	}

	color.a = alpha;

	draw_text(
	    rb, frame_arena, damage_str, hitsplat->position, color, 28,
	    get_asset_table()->texture_shader, get_asset_table()->default_font, 5);
    }
}

void hitsplats_create(World *world, Vector2 position, Damage damage)
{
    // NOTE: a single instance of damage can result in multiple hitsplats
    // if the instance contains multiple damage types
    for (DamageType type = 0; type < DMG_TYPE_COUNT; ++type) {
	StatValue value = damage.type_values[type];

	if (value > 0) {
	    // TODO: randomize size and position
	    // TODO: use function for updating position instead
	    Vector2 velocity = rng_direction(PI * 2);
	    velocity = v2_mul_s(velocity, 20.0f);

	    Hitsplat hitsplat = {
		.damage = {value, type},
		.position = position,
		.velocity = velocity,
		.lifetime = 2.0f,
	    };

            ring_push_overwrite(&world->active_hitsplats, &hitsplat);
	}
    }
}
