#include "renderer_dispatch.h"
#include "base/rectangle.h"
#include "font.h"
#include "game/renderer/render_command.h"
#include "game/renderer/render_key.h"
#include "game/particle.h"
#include "renderer/renderer_backend.h"

typedef struct {
    ShaderHandle shader;
    TextureHandle texture;
} RendererState;

static RendererState get_state_needed_for_entry(RenderKey key, AssetManager *assets)
{
    RendererState result = {0};

    RenderKey shader_id = render_key_extract_shader(key);
    RenderKey texture_id = render_key_extract_texture(key);
    RenderKey font_id = render_key_extract_font(key);

    if (font_id != NULL_FONT.id) {
        ASSERT(texture_id == NULL_TEXTURE.id);
        FontAsset *font_asset = assets_get_font(assets, (FontHandle){(u32)font_id});
        TextureHandle font_texture_handle = font_get_texture_handle(font_asset);

        result.texture = font_texture_handle;
    } else {
        result.texture = (TextureHandle){(u32)texture_id};
    }

    result.shader = (ShaderHandle){(u32)shader_id};

    return result;
}

static b32 renderer_state_change_needed(RendererState lhs, RendererState rhs)
{
    return (lhs.shader.id != rhs.shader.id) || (lhs.texture.id != rhs.texture.id);
}

static RendererState switch_renderer_state(RendererState new_state, AssetManager *assets,
    RendererState old_state, RendererBackend *backend)
{
    (void)backend;

    if (new_state.texture.id != old_state.texture.id) {
        if (new_state.texture.id != NULL_TEXTURE.id) {
            TextureAsset *texture = assets_get_texture(assets, new_state.texture);
            renderer_backend_bind_texture(texture);
        }
    }

    if (new_state.shader.id != old_state.shader.id) {
        ShaderAsset *shader = assets_get_shader(assets, new_state.shader);

        renderer_backend_use_shader(shader);
    }

    return new_state;
}

#define INVALID_RENDERER_STATE (RendererState){{(AssetID)-1}, {(AssetID)-1}}

// TODO: move elsewhere
static void render_line(RendererBackend *backend, Vector2 start, Vector2 end, f32 thickness, RGBA32 color)
{
    Vector2 dir_r = v2_mul_s(v2_norm(v2_sub(end, start)), thickness / 2.0f);
    Vector2 dir_l = v2_neg(dir_r);
    Vector2 dir_u = { -dir_r.y, dir_r.x };
    Vector2 dir_d = v2_neg(dir_u);

    Vector2 tl = v2_add(v2_add(start, dir_l), dir_u);
    Vector2 tr = v2_add(end, v2_add(dir_r, dir_u));
    Vector2 br = v2_add(end, v2_add(dir_d, dir_r));
    Vector2 bl = v2_add(start, v2_add(dir_d, dir_l));

    // TODO: UV constants to make this easier to remember
    Vertex vtl = {
	.position = tl,
	.uv = {0,1},
	.color = color
    };

    Vertex vtr = {
	.position = tr,
	.uv = {1, 1},
	.color = color
    };

    Vertex vbr = {
	.position = br,
	.uv = {1, 0},
	.color = color
    };

    Vertex vbl = {
	.position = bl,
	.uv = {0, 0},
	.color = color
    };

    renderer_backend_draw_quad(backend, vtl, vtr, vbr, vbl);
}

void execute_render_commands(RenderBatch *rb, AssetManager *assets,
    RendererBackend *backend, LinearArena *scratch)
{
    if (rb->entry_count == 0) {
        return;
    }

    RendererState current_state = get_state_needed_for_entry(rb->entries[0].key, assets);
    switch_renderer_state(current_state, assets, INVALID_RENDERER_STATE, backend);

    renderer_backend_set_global_projection(backend, rb->projection);

    for (ssize i = 0; i < rb->entry_count; ++i) {
        RenderEntry *entry = &rb->entries[i];
        RenderCmdHeader *header = entry->data;

        RendererState needed_state = get_state_needed_for_entry(entry->key, assets);

        if (renderer_state_change_needed(current_state, needed_state)) {
            renderer_backend_flush(backend);
            current_state = switch_renderer_state(needed_state, assets, current_state, backend);
        }

        for (SetupCmdHeader *setup_cmd = header->first_setup_command; setup_cmd; setup_cmd = setup_cmd->next) {
            switch (setup_cmd->kind) {
                case SETUP_CMD_SET_UNIFORM_VEC4: {
                    SetupCmdUniformVec4 *cmd = (SetupCmdUniformVec4 *)setup_cmd;
                    ShaderAsset *shader = assets_get_shader(assets, current_state.shader);

                    renderer_backend_set_uniform_vec4(shader, cmd->uniform_name, cmd->vector, scratch);
                } break;

                INVALID_DEFAULT_CASE;
            }
        }

        switch (entry->data->kind) {
            case RENDER_CMD_RECTANGLE: {
                RectangleCmd *cmd = (RectangleCmd *)entry->data;
                RectangleVertices verts = rect_get_vertices(cmd->rect, cmd->color, rb->y_direction);

                ASSERT(rect_is_valid(cmd->rect));

		renderer_backend_draw_quad(backend, verts.top_left, verts.top_right,
		    verts.bottom_right, verts.bottom_left);
            } break;

            case RENDER_CMD_CROPPED_RECTANGLE: {
                CroppedRectangleCmd *cmd = (CroppedRectangleCmd *)entry->data;
                ASSERT(cmd->visible_rect.size.x > 0.0f && cmd->visible_rect.size.y > 0.0f);

                Rectangle rect = cmd->visible_rect;
                Vector2 sprite_size = cmd->uv_rect_size;
                RGBA32 color = cmd->color;

                f32 uv_top =
                    (rb->y_direction == Y_IS_UP)
                    ? 1.0f - (rect.size.y / sprite_size.y)
                    : 1.0f;
                f32 uv_bottom =
                    (rb->y_direction == Y_IS_DOWN)
                    ? 1.0f - (rect.size.y / sprite_size.y)
                    : 1.0f;

                f32 uv_left = 0.0f;
                f32 uv_right = rect.size.x / sprite_size.x;

                Vertex a = {
                    rect_top_left(rect),
                    {uv_left, uv_top},
                    color
                };

                Vertex b = {
                    rect_top_right(rect),
                    {uv_right, uv_top},
                    color
                };

                Vertex c = {
                    rect_bottom_right(rect),
                    {uv_right, uv_bottom},
                    color
                };

                Vertex d = {
                    rect_bottom_left(rect),
                    {uv_left, uv_bottom},
                    color
                };

                renderer_backend_draw_quad(backend, a, b, c, d);
            } break;

            case RENDER_CMD_CROPPED_RECTANGLE2: {
                CroppedRectangleCmd2 *cmd = (CroppedRectangleCmd2 *)entry->data;

                Rectangle visible_rect = cmd->visible_rect;
                Rectangle uv_rect = cmd->uv_rect;
                RGBA32 color = cmd->color;

                Rectangle overlap = rect_overlap_area(visible_rect, uv_rect);

                if (rect_is_valid(overlap)) {
                    f32 rel_left = rect_top_left(overlap).x - visible_rect.position.x;
                    f32 rel_right = rect_top_right(overlap).x - visible_rect.position.x;
                    f32 rel_bottom = rect_bottom_right(overlap).y - visible_rect.position.y;
                    f32 rel_top = rect_top_left(overlap).y - visible_rect.position.y;

                    // TODO: fix for y is up
                    f32 uv_left =
                        (overlap.position.x >= uv_rect.position.x)
                        ? 0.0f
                        : rel_left / uv_rect.size.x;
                    f32 uv_right =
                        (rect_top_right(overlap).x <= rect_top_right(uv_rect).x)
                        ? 1.0f
                        : rel_right / uv_rect.size.x;
                    f32 uv_top =
                        (rect_top_left(overlap).y <= rect_top_left(uv_rect).y)
                        ? 0.0f
                        : (rel_top / uv_rect.size.y);
                    f32 uv_bottom =
                        (rect_bottom_left(overlap).y >= rect_bottom_left(uv_rect).y)
                        ? 1.0f
                        : (rel_bottom / uv_rect.size.y);

#if 0
                    f32 uv_top =
                        (rect_top_left(overlap).x <= )
                        (rb->y_direction == Y_IS_UP) ?
                        (1.0f - rel_top / uv_rect.size.y)
                        : (rel_top / uv_rect.size.y);
                    f32 uv_bottom =
                        (rb->y_direction == Y_IS_UP) ?
                        (1.0f - rel_bottom / uv_rect.size.y)
                        : (rel_bottom / uv_rect.size.y);
#endif

                    Vertex a = {
                        rect_top_left(overlap),
                        {uv_left, uv_top},
                        color
                    };

                    Vertex b = {
                        rect_top_right(overlap),
                        {uv_right, uv_top},
                        color
                    };

                    Vertex c = {
                        rect_bottom_right(overlap),
                        {uv_right, uv_bottom},
                        color
                    };

                    Vertex d = {
                        rect_bottom_left(overlap),
                        {uv_left, uv_bottom},
                        color
                    };

                    renderer_backend_draw_quad(backend, a, b, c, d);
                }

            } break;

            case RENDER_CMD_OUTLINED_RECTANGLE: {
                OutlinedRectangleCmd *cmd = (OutlinedRectangleCmd *)entry->data;
                ASSERT(cmd->rect.size.x > 0.0f && cmd->rect.size.y > 0.0f);

		RGBA32 color = cmd->color;
		f32 thick = cmd->thickness;

		Vector2 tl = rect_top_left(cmd->rect);
		Vector2 tr = rect_top_right(cmd->rect);
		Vector2 br = rect_bottom_right(cmd->rect);
		Vector2 bl = rect_bottom_left(cmd->rect);

		// TODO: These lines overlap which looks wrong when color is transparent
		render_line(backend, tl, v2_sub(tr, v2(thick, 0.0f)), thick, color);
		render_line(backend, tr, v2_add(br, v2(0.0f, thick)), thick, color);
		render_line(backend, br, v2_add(bl, v2(thick, 0.0f)), thick, color);
		render_line(backend, bl, v2_sub(tl, v2(0.0f, thick)), thick, color);

	    } break;

            case RENDER_CMD_CIRCLE: {
                CircleCmd *cmd = (CircleCmd *)entry->data;

                s32 segments = 64;
                f32 radius = cmd->radius;
                f32 posx = cmd->position.x;
                f32 posy = cmd->position.y;
                RGBA32 color = cmd->color;

                Vertex a = { {posx, posy}, {0.5f, 0.5f}, color };

                f32 step_angle = (2.0f * PI) / (f32)segments;

                for (s32 k = 0; k < segments; ++k) {
                    f32 segment = (f32)k;

                    f32 x1 = posx + radius * cosf(step_angle * segment);
                    f32 y1 = posy + radius * sinf(step_angle * segment);
                    f32 tx1 = (x1 / radius + 1.0f) * 0.5f;
                    f32 ty1 = 1.0f - ((y1 / radius + 1.0f) * 0.5f);

                    f32 x2 = posx + radius * cosf(step_angle * (segment + 1));
                    f32 y2 = posy + radius * sinf(step_angle * (segment + 1));
                    f32 tx2 = (x2 / radius + 1.0f) * 0.5f;
                    f32 ty2 = 1.0f - ((y2 / radius + 1.0f) * 0.5f);


                    Vertex b = { {x1, y1}, {tx1, ty1}, color};
                    Vertex c = { {x2, y2}, {tx2, ty2}, color};

                    renderer_backend_draw_triangle(backend, a, b, c);
                }
            } break;

            case RENDER_CMD_LINE: {
                LineCmd *cmd = (LineCmd *)entry->data;

		render_line(backend, cmd->start, cmd->end, cmd->thickness, cmd->color);
            } break;

            case RENDER_CMD_TEXT: {
                TextCmd *cmd = (TextCmd *)entry->data;

                FontHandle font_handle = (FontHandle){(u32)render_key_extract_font(entry->key)};
                FontAsset *font_asset = assets_get_font(assets, font_handle);

                RGBA32 color = cmd->color;
                s32 text_size = cmd->size;
                String text = cmd->text;

                f32 newline_advance = font_get_newline_advance(font_asset, text_size);

                Vector2 start_position = cmd->position;
                Vector2 cursor = start_position;

                for (s32 ch = 0; ch < cmd->text.length; ++ch) {
                    char glyph = text.data[ch];

                    if (glyph == '\n') {
                        cursor.x = start_position.x;

                        if (rb->y_direction == Y_IS_UP) {
                            cursor.y -= newline_advance;
                        } else {
                            cursor.y += newline_advance;
                        }
                    } else if (glyph == '\t') {
                        GlyphVertices verts = font_get_glyph_vertices(font_asset, ' ', cursor,
                            text_size, color, rb->y_direction);

                        cursor.x += verts.advance_x * 4;
                    } else {
                        GlyphVertices verts = font_get_glyph_vertices(font_asset, glyph, cursor,
                            text_size, color, rb->y_direction);

                        renderer_backend_draw_quad(backend, verts.top_left, verts.top_right,
                            verts.bottom_right, verts.bottom_left);

                        cursor.x += verts.advance_x;
                    }
                }
            } break;

            case RENDER_CMD_PARTICLES: {
                ParticleGroupCmd *cmd = (ParticleGroupCmd *)entry->data;

                ParticleBuffer *particles = cmd->particles;
                Vector2 particle_dims = v2(cmd->particle_size, cmd->particle_size);
                RGBA32 base_color = cmd->color;

                for (ssize p = 0; p < ring_length(particles); ++p) {
                    Particle *particle = ring_at(particles, p);

                    Rectangle rect = {particle->position, particle_dims};
                    f32 a = base_color.a - (particle->timer / particle->lifetime) * base_color.a;
                    a = CLAMP(a, 0.0f, 1.0f);

                    RGBA32 color = {base_color.r, base_color.g, base_color.b, a};

                    RectangleVertices verts = rect_get_vertices(rect, color, Y_IS_UP);

                    renderer_backend_draw_quad(
                        backend,
                        verts.top_left,
                        verts.top_right,
                        verts.bottom_right,
                        verts.bottom_left
                    );
                }
            } break;

           INVALID_DEFAULT_CASE;
        }
    }

    renderer_backend_flush(backend);
}
