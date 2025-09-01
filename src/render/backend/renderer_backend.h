#ifndef RENDERER_BACKEND_H
#define RENDERER_BACKEND_H

#include "base/allocator.h"
#include "base/string8.h"
#include "base/matrix.h"
#include "render/vertex.h"
#include "image.h"
#include "assets.h"

// TODO: deinit backend?
// TODO: only end_frame/flush needed

typedef struct RendererBackend RendererBackend;

RendererBackend *renderer_backend_initialize(Allocator allocator);
ShaderAsset     *renderer_backend_create_shader(String shader_source, Allocator allocator);
void             renderer_backend_destroy_shader(ShaderAsset *shader);
TextureAsset    *renderer_backend_create_texture(Image image, Allocator allocator);
void             renderer_backend_destroy_texture(TextureAsset *texture);
void             renderer_backend_use_shader(ShaderAsset *shader);
void             renderer_backend_bind_texture(TextureAsset *texture);
void             renderer_backend_set_mat4_uniform(ShaderAsset *shader, String uniform_name, Matrix4 matrix);
void             renderer_backend_begin_frame(RendererBackend *backend);
void             renderer_backend_end_frame(RendererBackend *backend);
void             renderer_backend_draw_triangle(RendererBackend *backend, Vertex a, Vertex b, Vertex c);
void             renderer_backend_draw_quad(RendererBackend *backend, Vertex a, Vertex b, Vertex c, Vertex d);

#endif //RENDERER_BACKEND_H
