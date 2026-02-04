#include "render_command_interpreter.h"
#include "base/rectangle.h"
#include "base/rgba.h"
#include "base/triangle.h"
#include "base/vector.h"
#include "platform/font.h"
#include "renderer/frontend/render_command.h"
#include "renderer/frontend/render_key.h"
#include "renderer/frontend/render_target.h"
#include "renderer/backend/renderer_backend.h"
#include "game/components/particle.h"

#define INVALID_RENDERER_STATE (RendererState){{(AssetID)-1}, {(AssetID)-1}}

typedef struct {
    ShaderHandle shader;
    TextureHandle texture;
} RendererState;

static RendererState get_state_needed_for_entry(RenderKey key, AssetSystem *assets)
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

static RendererState switch_renderer_state(RendererState new_state, AssetSystem *assets,
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
	.uv = {0, 1},
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

static void execute_render_command(RenderEntry *entry, RenderBatch *rb, RendererState *current_state,
    RendererBackend *backend, AssetSystem *assets, LinearArena *scratch)
{
    RenderCmdHeader *header = entry->data;

    RendererState needed_state = get_state_needed_for_entry(entry->key, assets);

    if (renderer_state_change_needed(*current_state, needed_state)) {
        renderer_backend_flush(backend);
        *current_state = switch_renderer_state(needed_state, assets, *current_state, backend);
    }

    for (SetupCmdHeader *setup_cmd = header->first_setup_command; setup_cmd; setup_cmd = setup_cmd->next) {
        switch (setup_cmd->kind) {
            case SETUP_CMD_SET_UNIFORM_VEC4: {
                SetupCmdUniformVec4 *cmd = (SetupCmdUniformVec4 *)setup_cmd;
                ShaderAsset *shader = assets_get_shader(assets, current_state->shader);

                renderer_backend_set_uniform_vec4(shader, cmd->header.uniform_name, cmd->value, scratch);
            } break;

            case SETUP_CMD_SET_UNIFORM_FLOAT: {
                SetupCmdUniformFloat *cmd = (SetupCmdUniformFloat *)setup_cmd;
                ShaderAsset *shader = assets_get_shader(assets, current_state->shader);

                renderer_backend_set_uniform_float(shader, cmd->header.uniform_name, cmd->value, scratch);
            } break;
                INVALID_DEFAULT_CASE;
        }
    }

    switch (entry->data->kind) {
        // TODO: can these switch cases be simplified?
        case RENDER_COMMAND_ENUM_NAME(RectangleCmd): {
            RectangleCmd *cmd = (RectangleCmd *)entry->data;
            RectangleVertices verts = rect_get_vertices(cmd->rect, cmd->color, rb->y_direction);
            ASSERT(rect_is_valid(cmd->rect));

            f32 rotation = cmd->rotation_in_radians;

            if (cmd->flip & RECT_FLIP_HORIZONTALLY) {
                Vector2 tmp = verts.top_left.uv;
                verts.top_left.uv = verts.top_right.uv;
                verts.top_right.uv = tmp;

                tmp = verts.bottom_left.uv;
                verts.bottom_left.uv = verts.bottom_right.uv;
                verts.bottom_right.uv = tmp;
            }

            if (cmd->flip & RECT_FLIP_VERTICALLY) {
                Vector2 tmp = verts.top_left.uv;
                verts.top_left.uv = verts.bottom_left.uv;
                verts.bottom_left.uv = tmp;

                tmp = verts.top_right.uv;
                verts.top_right.uv = verts.bottom_right.uv;
                verts.bottom_right.uv = tmp;
            }

            // TODO: rotate around origin instead?
            Vector2 origin = rect_center(cmd->rect);

            verts.top_left.position = v2_rotate_around_point(verts.top_left.position, rotation, origin);
            verts.top_right.position = v2_rotate_around_point(verts.top_right.position, rotation, origin);
            verts.bottom_right.position = v2_rotate_around_point(verts.bottom_right.position, rotation, origin);
            verts.bottom_left.position = v2_rotate_around_point(verts.bottom_left.position, rotation, origin);

            renderer_backend_draw_quad(backend, verts.top_left, verts.top_right,
                verts.bottom_right, verts.bottom_left);
        } break;

        case RENDER_COMMAND_ENUM_NAME(ClippedRectangleCmd): {
            ClippedRectangleCmd *cmd = (ClippedRectangleCmd *)entry->data;

            Rectangle rect = cmd->rect;
            Rectangle viewport = cmd->viewport_rect;
            RGBA32 color = cmd->color;

            ClippedRectangleVertices verts = rect_get_clipped_vertices(rect, viewport, color, rb->y_direction);

            if (verts.is_visible) {
                renderer_backend_draw_quad(
                    backend,
                    verts.vertices.top_left,
                    verts.vertices.top_right,
                    verts.vertices.bottom_right,
                    verts.vertices.bottom_left
                );
            }
        } break;

        case RENDER_COMMAND_ENUM_NAME(TriangleCmd): {
            TriangleCmd *cmd = (TriangleCmd *)entry->data;
            Triangle triangle = cmd->triangle;

            TriangleVertices verts = triangle_get_vertices(triangle, cmd->color);

            renderer_backend_draw_triangle(backend, verts.a, verts.b, verts.c);
        } break;

        case RENDER_COMMAND_ENUM_NAME(OutlinedTriangleCmd): {
            OutlinedTriangleCmd *cmd = (OutlinedTriangleCmd *)entry->data;
            Triangle triangle = cmd->triangle;
            RGBA32 color = cmd->color;
            f32 thickness = cmd->thickness;

            render_line(backend, triangle.a, triangle.b, thickness, color);
            render_line(backend, triangle.b, triangle.c, thickness, color);
            render_line(backend, triangle.c, triangle.a, thickness, color);
        } break;

        case RENDER_COMMAND_ENUM_NAME(OutlinedRectangleCmd): {
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

        case RENDER_COMMAND_ENUM_NAME(CircleCmd): {
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

        case RENDER_COMMAND_ENUM_NAME(LineCmd): {
            LineCmd *cmd = (LineCmd *)entry->data;

            render_line(backend, cmd->start, cmd->end, cmd->thickness, cmd->color);
        } break;

        case RENDER_COMMAND_ENUM_NAME(TextCmd): {
            TextCmd *cmd = (TextCmd *)entry->data;

            FontHandle font_handle = (FontHandle){(u32)render_key_extract_font(entry->key)};
            FontAsset *font_asset = assets_get_font(assets, font_handle);

            RGBA32 color = cmd->color;
            s32 text_size = cmd->size;
            String text = cmd->text;
            b32 is_clipped = cmd->is_clipped;
            Rectangle clip_rect = cmd->clip_rect;

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
                    RenderedGlyphInfo verts = font_get_glyph_vertices(font_asset, ' ', cursor,
                        text_size, color, rb->y_direction);

                    cursor.x += verts.advance_x * 4;
                } else {
                    RenderedGlyphInfo verts = {0};

                    if (is_clipped) {
                        verts = font_get_clipped_glyph_vertices(
                            font_asset, glyph, cursor, clip_rect, text_size, color, rb->y_direction
                        );
                    } else {
                        verts = font_get_glyph_vertices(
                            font_asset, glyph, cursor, text_size, color, rb->y_direction
                        );
                    }

                    if (verts.is_visible) {
                        // TODO: overload for this that takes RectangleVertices
                        renderer_backend_draw_quad(
                            backend,
                            verts.vertices.top_left,
                            verts.vertices.top_right,
                            verts.vertices.bottom_right,
                            verts.vertices.bottom_left
                        );
                    }

                    cursor.x += verts.advance_x;
                }
            }
        } break;

        case RENDER_COMMAND_ENUM_NAME(ParticleGroupCmd): {
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

        case RENDER_COMMAND_ENUM_NAME(PolygonCmd): {
            PolygonCmd *cmd = (PolygonCmd *)entry->data;
            RGBA32 color = cmd->color;

            for (PolygonTriangle *tri = list_head(&cmd->polygon); tri; tri = list_next(tri)) {
                TriangleVertices verts = triangle_get_vertices(tri->triangle, color);

                renderer_backend_draw_triangle(backend, verts.a, verts.b, verts.c);
            }

        } break;

        case RENDER_COMMAND_ENUM_NAME(TriangleFanCmd): {
            TriangleFanCmd *cmd = (TriangleFanCmd *)entry->data;

            RGBA32 color = cmd->color;
            Vector2 center = cmd->triangle_fan.center;
            ssize count = cmd->triangle_fan.count;

            for (ssize elem = 0; elem < count; ++elem) {
                TriangleFanElement curr = cmd->triangle_fan.items[elem];
                Triangle triangle = {center, curr.a, curr.b};

                TriangleVertices verts = triangle_get_vertices(triangle, color);
                renderer_backend_draw_triangle(backend, verts.a, verts.b, verts.c);
            }
        } break;
    }

    // If this render entry had setup commands we need to flush afterwards since
    // those may set uniform variables and we don't want those to leak into next render entry
    if (header->first_setup_command) {
        renderer_backend_flush(backend);
    }
}

void execute_render_commands(RenderBatch *rb, AssetSystem *assets,
    RendererBackend *backend, LinearArena *scratch)
{
    // NOTE: The color buffer should always be cleared, even if there is nothing to draw
    // as some batches may always need the background color to be drawn
    renderer_backend_change_framebuffer(backend, rb->render_target);

    if (!rgba32_eq(rb->clear_color, RGBA32_TRANSPARENT)) {
        renderer_backend_clear_color_buffer(rb->clear_color);
    }

    if (rb->entry_count == 0) {
        return;
    }

    if (rb->stencil_batch) {
        // This batch has a stencil pass, run it before rendering this batch
        RenderBatch *stencil = rb->stencil_batch;
        ASSERT(stencil->render_target == rb->render_target);
        ASSERT(!stencil->stencil_batch && "A stencil batch can't have a stencil batch of it's own");

        // The stencil pass should only write to stencil buffer, not color buffer
        renderer_backend_enable_stencil_writes();
        renderer_backend_disable_color_buffer_writes(backend);

        execute_render_commands(stencil, assets, backend, scratch);

        // We should only write to color buffer, not stencil buffer
        renderer_backend_disable_stencil_writes();
        renderer_backend_enable_color_buffer_writes(backend);

        renderer_backend_set_stencil_function(backend, stencil->stencil_func, stencil->stencil_func_arg);
    } else {
        // Either we are a stencil batch or a batch without a stencil pass, either way we should
        // always pass stencil test
        renderer_backend_set_stencil_function(backend, STENCIL_FUNCTION_ALWAYS, rb->stencil_func_arg);

        // If we are a stencil batch, stencil buffer writes will be enabled. If not, they're disabled
        // so this won't affect anything
        renderer_backend_set_stencil_pass_operation(backend, rb->stencil_op);
    }

    RendererState current_state = get_state_needed_for_entry(rb->entries[0].key, assets);
    switch_renderer_state(current_state, assets, INVALID_RENDERER_STATE, backend);

    renderer_backend_set_global_projection(backend, rb->projection);
    renderer_backend_set_blend_function(rb->blend_function);

    for (ssize i = 0; i < rb->entry_count; ++i) {
        RenderEntry *entry = &rb->entries[i];

        execute_render_command(entry, rb, &current_state, backend, assets, scratch);
    }

    renderer_backend_flush(backend);
}
