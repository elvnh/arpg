#ifndef RENDERER_BACKEND_H
#define RENDERER_BACKEND_H

#include "base/allocator.h"
#include "base/string8.h"
#include "render/vertex.h"

typedef struct RendererBackend RendererBackend;
typedef struct ShaderHandle ShaderHandle;

RendererBackend *renderer_backend_initialize(Allocator allocator);
void             renderer_backend_begin_frame(RendererBackend *state);
void             renderer_backend_end_frame(RendererBackend *state);
void             renderer_backend_draw_triangle(RendererBackend *state, Vertex a, Vertex b, Vertex c);
void             renderer_backend_draw_quad(RendererBackend *state, Vertex a, Vertex b, Vertex c, Vertex d);

ShaderHandle    *renderer_backend_compile_shader(String shader_source, Allocator allocator);

#endif //RENDERER_BACKEND_H
