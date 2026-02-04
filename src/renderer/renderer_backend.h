#ifndef RENDERER_BACKEND_H
#define RENDERER_BACKEND_H

#include "base/allocator.h"
#include "base/string8.h"
#include "base/matrix.h"
#include "base/vector.h"
#include "base/vertex.h"
#include "base/image.h"
#include "base/linear_arena.h"

#include "render_target.h"

// TODO: deinit backend?
// TODO: only end_frame/flush needed
// TODO: free memory allocated when destroying assets

typedef enum {
    STENCIL_FUNCTION_ALWAYS,
    STENCIL_FUNCTION_EQUAL,
    STENCIL_FUNCTION_NOT_EQUAL,
} StencilFunction;

typedef enum {
    STENCIL_OP_KEEP,
    STENCIL_OP_REPLACE,
} StencilOperation;

typedef struct ShaderAsset  ShaderAsset;
typedef struct TextureAsset TextureAsset;

typedef struct RendererBackend RendererBackend;

RendererBackend *renderer_backend_initialize(Vector2i window_dims, Allocator allocator);
ShaderAsset     *renderer_backend_create_shader(String shader_source, Allocator allocator);
void             renderer_backend_destroy_shader(ShaderAsset *shader, Allocator allocator);
TextureAsset    *renderer_backend_create_texture(Image image, Allocator allocator);
void             renderer_backend_destroy_texture(TextureAsset *texture, Allocator allocator);
void             renderer_backend_use_shader(ShaderAsset *shader);
void             renderer_backend_bind_texture(TextureAsset *texture);
void             renderer_backend_set_global_projection(RendererBackend *backend, Matrix4 matrix);
void             renderer_backend_set_uniform_vec4(ShaderAsset *shader, String uniform_name, Vector4 vec,
                                                   LinearArena *scratch);
void             renderer_backend_set_uniform_float(ShaderAsset *shader, String uniform_name, f32 value,
                                                   LinearArena *scratch);
void             renderer_backend_clear_color_buffer(RGBA32 color);
void             renderer_backend_flush(RendererBackend *backend);
void             renderer_backend_draw_triangle(RendererBackend *backend, Vertex a, Vertex b, Vertex c);
void             renderer_backend_draw_quad(RendererBackend *backend, Vertex a, Vertex b, Vertex c, Vertex d);
void             renderer_backend_change_framebuffer(RendererBackend *backend, FrameBuffer render_target);
void             renderer_backend_change_to_main_framebuffer(RendererBackend *backend);
void             renderer_backend_blend_framebuffers(RendererBackend *backend, FrameBuffer a, FrameBuffer b,
                                                     ShaderAsset *shader);
void             renderer_backend_set_blend_function(BlendFunction function);
void             renderer_backend_draw_framebuffer_as_texture(RendererBackend *backend, FrameBuffer render_target,
                                                              ShaderAsset *shader);
void             renderer_backend_begin_frame(RendererBackend *backend);
void             renderer_backend_enable_stencil_writes(void);
void             renderer_backend_disable_stencil_writes(void);
void             renderer_backend_enable_color_buffer_writes(RendererBackend *backend);
void             renderer_backend_disable_color_buffer_writes(RendererBackend *backend);
void             renderer_backend_set_stencil_function(RendererBackend *backend, StencilFunction function, s32 arg);
void             renderer_backend_set_stencil_pass_operation(RendererBackend *backend, StencilOperation op);

#endif //RENDERER_BACKEND_H
