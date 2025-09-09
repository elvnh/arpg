#ifndef RENDERER_BACKEND_H
#define RENDERER_BACKEND_H

#include "base/allocator.h"
#include "base/string8.h"
#include "base/matrix.h"
#include "base/vertex.h"
#include "base/image.h"
#include "base/linear_arena.h"

// TODO: deinit backend?
// TODO: only end_frame/flush needed
// TODO: free memory allocated when destroying assets

typedef struct ShaderAsset  ShaderAsset;
typedef struct TextureAsset TextureAsset;

typedef struct RendererBackend RendererBackend;

RendererBackend *renderer_backend_initialize(Allocator allocator);
ShaderAsset     *renderer_backend_create_shader(String shader_source, Allocator allocator);
void             renderer_backend_destroy_shader(ShaderAsset *shader, Allocator allocator);
TextureAsset    *renderer_backend_create_texture(Image image, Allocator allocator);
void             renderer_backend_destroy_texture(TextureAsset *texture, Allocator allocator);
void             renderer_backend_use_shader(ShaderAsset *shader);
void             renderer_backend_bind_texture(TextureAsset *texture);
void             renderer_backend_set_global_projection(RendererBackend *backend, Matrix4 matrix);
void             renderer_backend_set_uniform_vec4(ShaderAsset *shader, String uniform_name, Vector4 vec,
                                                   LinearArena *scratch);
void             renderer_backend_clear(RendererBackend *backend); // TODO: provide clear color
void             renderer_backend_flush(RendererBackend *backend);
void             renderer_backend_draw_triangle(RendererBackend *backend, Vertex a, Vertex b, Vertex c);
void             renderer_backend_draw_quad(RendererBackend *backend, Vertex a, Vertex b, Vertex c, Vertex d);

#endif //RENDERER_BACKEND_H
