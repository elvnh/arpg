#include <GL/glew.h>

#include "base/allocator.h"
#include "base/rectangle.h"
#include "base/rgba.h"
#include "base/string8.h"
#include "base/utils.h"
#include "base/vector.h"
#include "camera.h"
#include "renderer/render_target.h"
#include "renderer/renderer_backend.h"
#include "base/vertex.h"

#define MAX_RENDERER_VERTICES 512

#define POSITION_ATTRIBUTE    0
#define UV_ATTRIBUTE          1
#define COLOR_ATTRIBUTE       2

#define VERTEX_SHADER_DIRECTIVE    str_lit("#vertex")
#define FRAGMENT_SHADER_DIRECTIVE  str_lit("#fragment")

#define GLOBAL_UNIFORMS_BINDING_POINT 0
#define GLOBAL_UNIFORMS_NAME          "GlobalUniforms"

#define GLSL_VERSION_STRING "#version 330\n"
#define GLOBAL_UNIFORMS_SOURCE "layout (std140) uniform "GLOBAL_UNIFORMS_NAME" { " \
    "    uniform mat4 u_proj; "                                                    \
    "}; "

static const char base_vertex_shader_source[] =
    GLSL_VERSION_STRING
    "layout (location = 0) in vec2 a_world_pos; "
    "layout (location = 1) in vec2 a_uv; "
    "layout (location = 2) in vec4 a_color; "
    GLOBAL_UNIFORMS_SOURCE
    "void set_position() "
    "{ "
    "    gl_Position.xy = (u_proj * vec4(a_world_pos.xy, 0.0f, 1.0f)).xy;"
    "    gl_Position.zw = vec2(0.0f, 1.0f); "
    "} ";

static const char base_fragment_shader_source[] =
    GLSL_VERSION_STRING
    GLOBAL_UNIFORMS_SOURCE;

typedef struct {
    Matrix4 projection;
} GlobalShaderUniforms;

typedef struct {
    GLuint framebuffer_object;
    GLuint renderbuffer_object;
    GLuint color_buffer;
} BackendFramebuffer;

struct RendererBackend {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLuint ubo;

    BackendFramebuffer framebuffers[FRAME_BUFFER_COUNT];

    Vertex  vertices[MAX_RENDERER_VERTICES];
    s32     vertex_count;

    GLuint  indices[MAX_RENDERER_VERTICES];
    s32     index_count;
};

struct ShaderAsset {
    GLuint native_handle;
};

struct TextureAsset {
    GLuint native_handle;
};

static void GLAPIENTRY gl_error_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar* message, const void* user_param)
{
    (void)source;
    (void)id;
    (void)length;
    (void)user_param;

    if (type == GL_DEBUG_TYPE_ERROR) {
        fprintf(stderr, "%s\nSeverity: 0x%x\nMessage: %s\n",
            "** OpenGL ERROR **", severity, message);
    }
}

static BackendFramebuffer *get_framebuffer(RendererBackend *backend, FrameBuffer render_target)
{
    ASSERT(render_target >= 0);
    ASSERT(render_target < FRAME_BUFFER_COUNT);
    BackendFramebuffer *result = &backend->framebuffers[render_target];

    return result;
}

static GLuint get_framebuffer_object(RendererBackend *backend, FrameBuffer render_target)
{
    BackendFramebuffer *fb = get_framebuffer(backend, render_target);
    GLuint result = fb->framebuffer_object;

    return result;
}

static GLuint get_render_target_texture(RendererBackend *backend, FrameBuffer render_target)
{
    BackendFramebuffer *fb = get_framebuffer(backend, render_target);
    GLuint result = fb->color_buffer;

    return result;
}

static void create_framebuffer(RendererBackend *backend, FrameBuffer render_target, Vector2i window_dims)
{
    BackendFramebuffer *fb = get_framebuffer(backend, render_target);

    glGenFramebuffers(1, &fb->framebuffer_object);

    // TODO: make sure to call glViewport if dimensions change
    // Color attachment
    glGenTextures(1, &fb->color_buffer);
    glBindTexture(GL_TEXTURE_2D, fb->color_buffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window_dims.x, window_dims.y, 0, GL_RGBA,
        GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Renderbuffer (stencil) attachment
    glGenRenderbuffers(1, &fb->renderbuffer_object);
    glBindRenderbuffer(GL_RENDERBUFFER, fb->renderbuffer_object);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, window_dims.x, window_dims.y);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, fb->framebuffer_object);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb->color_buffer, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fb->renderbuffer_object);

    ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

RendererBackend *renderer_backend_initialize(Vector2i window_dims, Allocator allocator)
{
    RendererBackend *state = allocate_item(allocator, RendererBackend);
    ASSERT(state->vertex_count == 0);
    ASSERT(state->index_count == 0);

    glewInit();

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(gl_error_callback, 0);

    glEnable(GL_BLEND);
    glEnable(GL_STENCIL_TEST);

    glDisable(GL_DEPTH_TEST);

    glGenVertexArrays(1, &state->vao);
    glBindVertexArray(state->vao);

    glGenBuffers(1, &state->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, state->vbo);
    glBufferData(GL_ARRAY_BUFFER, ARRAY_COUNT(state->vertices) * sizeof(*state->vertices), 0, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &state->ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ARRAY_COUNT(state->indices) * sizeof(*state->indices), 0, GL_DYNAMIC_DRAW);

    s32 stride = (s32)sizeof(*state->vertices);

    glVertexAttribPointer(POSITION_ATTRIBUTE, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(Vertex, position));
    glEnableVertexAttribArray(POSITION_ATTRIBUTE);

    glVertexAttribPointer(UV_ATTRIBUTE, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(Vertex, uv));
    glEnableVertexAttribArray(UV_ATTRIBUTE);

    glVertexAttribPointer(COLOR_ATTRIBUTE, 4, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(Vertex, color));
    glEnableVertexAttribArray(COLOR_ATTRIBUTE);

    /* UBO */
    glGenBuffers(1, &state->ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, state->ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(GlobalShaderUniforms), 0, GL_DYNAMIC_DRAW);
    glBindBufferRange(GL_UNIFORM_BUFFER, GLOBAL_UNIFORMS_BINDING_POINT, state->ubo, 0, sizeof(GlobalShaderUniforms));

    glBindVertexArray(0);

    /* Frame buffers */
    for (FrameBuffer target = 0; target < FRAME_BUFFER_COUNT; ++target) {
        create_framebuffer(state, target, window_dims);
    }

    return state;
}

typedef struct {
    String vertex_shader;
    String fragment_shader;
    bool   ok;
} SplitShaderSource;

static bool assert_shader_compilation_successful(GLuint shader)
{
    GLint is_compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &is_compiled);

    if (is_compiled == GL_FALSE) {
        GLint max_length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_length);
        ASSERT(!add_overflows_s32(max_length, 1));

        // TODO: scratch
        Allocator scratch = default_allocator;
        char *data = allocate_array(scratch, char, max_length + 1);

        glGetShaderInfoLog(shader, max_length, &max_length, data);
        glDeleteShader(shader);

        fprintf(stderr, "\n*** SHADER COMPILATION ERROR ***\n\n%s", data);

        return false;
    }

    return true;
}

static SplitShaderSource split_shader_source(String source)
{
    SplitShaderSource result = {0};

    ssize vertex_index = str_find_first_occurence(source, VERTEX_SHADER_DIRECTIVE);
    ssize fragment_index = str_find_first_occurence(source, FRAGMENT_SHADER_DIRECTIVE);

    if ((vertex_index == -1) || (fragment_index == -1)) {
        return result;
    }

    ssize vertex_end = 0;
    ssize fragment_end = 0;

    if (vertex_index < fragment_index) {
        vertex_end = fragment_index;
        fragment_end = source.length;
    } else {
        vertex_end = source.length;
        fragment_end = vertex_index;
    }

    // Skip the directives since they're invalid GLSL code
    vertex_index += VERTEX_SHADER_DIRECTIVE.length;
    fragment_index += FRAGMENT_SHADER_DIRECTIVE.length;

    String vertex_src = str_create_span(source, vertex_index, vertex_end - vertex_index);
    String fragment_src = str_create_span(source, fragment_index, fragment_end - fragment_index);

    result.vertex_shader = vertex_src;
    result.fragment_shader = fragment_src;
    result.ok = true;

    return result;
}


ShaderAsset *renderer_backend_create_shader(String shader_source, Allocator allocator)
{
    SplitShaderSource split_result = split_shader_source(shader_source);

    if (!split_result.ok) {
        return 0;
    }

    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    GLuint frag_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    const char *vertex_srcs[] = { base_vertex_shader_source, split_result.vertex_shader.data };
    const s32 vertex_srcs_lengths[] = {
        ARRAY_COUNT(base_vertex_shader_source) - 1,
        ssize_to_s32(split_result.vertex_shader.length)
    };

    const char *frag_srcs[] = { base_fragment_shader_source, split_result.fragment_shader.data };
    const s32 frag_srcs_lengths[] = {
        ARRAY_COUNT(base_fragment_shader_source) - 1,
        ssize_to_s32(split_result.fragment_shader.length)
    };

    glShaderSource(vertex_shader_id, 2, vertex_srcs, vertex_srcs_lengths);
    glCompileShader(vertex_shader_id);

    glShaderSource(frag_shader_id, 2, frag_srcs, frag_srcs_lengths);
    glCompileShader(frag_shader_id);

    if (!assert_shader_compilation_successful(vertex_shader_id)
     || !assert_shader_compilation_successful(frag_shader_id)) {
        // TODO: does anything need to be deleted here?
        return 0;
    }

    GLuint program_id = glCreateProgram();

    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, frag_shader_id);
    glLinkProgram(program_id);

    glDetachShader(program_id, vertex_shader_id);
    glDetachShader(program_id, frag_shader_id);

    u32 global_uniforms_index = glGetUniformBlockIndex(program_id, GLOBAL_UNIFORMS_NAME);
    glUniformBlockBinding(program_id, global_uniforms_index, GLOBAL_UNIFORMS_BINDING_POINT);

    ShaderAsset *handle = allocate_item(allocator, ShaderAsset);
    handle->native_handle = program_id;

    return handle;
}

void renderer_backend_destroy_shader(ShaderAsset *shader, Allocator allocator)
{
    glDeleteProgram(shader->native_handle);
    deallocate(allocator, shader);
}

TextureAsset *renderer_backend_create_texture(Image image, Allocator allocator)
{
    GLuint texture_id;

    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLenum channel = 0;
    switch (image.channels) {
        case 4: {
            channel = GL_RGBA;
        } break;

        case 3: {
            channel = GL_RGB;
        } break;

        case 1: {
            channel = GL_RED; // TODO: is this correct?
        } break;

        INVALID_DEFAULT_CASE;
    }

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        image.channels,
        image.width,
        image.height,
        0,
        channel,
        GL_UNSIGNED_BYTE,
        image.data
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    TextureAsset *handle = allocate_item(allocator, TextureAsset);
    handle->native_handle = texture_id;

    return handle;
}

void renderer_backend_destroy_texture(TextureAsset *texture, Allocator allocator)
{
    glDeleteTextures(1, &texture->native_handle);
    deallocate(allocator, texture);
}

void renderer_backend_use_shader(ShaderAsset *handle)
{
    glUseProgram(handle->native_handle);
}

void renderer_backend_bind_texture(TextureAsset *texture)
{
    glBindTexture(GL_TEXTURE_2D, texture->native_handle);
}

static GLint get_uniform_location(ShaderAsset *shader, String uniform_name, LinearArena *scratch)
{
    String terminated = str_null_terminate(uniform_name, la_allocator(scratch));

    GLint location = glGetUniformLocation(shader->native_handle, terminated.data);
    // TODO: instead log error on failure
    ASSERT(location != -1);

    return location;
}
void renderer_backend_set_global_projection(RendererBackend *backend, Matrix4 matrix)
{
    glBindBuffer(GL_UNIFORM_BUFFER, backend->ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, offsetof(GlobalShaderUniforms, projection), sizeof(Matrix4), matrix.data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void renderer_backend_set_uniform_vec4(ShaderAsset *shader, String uniform_name, Vector4 vec, LinearArena *scratch)
{
    GLint location = get_uniform_location(shader, uniform_name, scratch);

    glUniform4fv(location, 1, &vec.x);
}

void renderer_backend_set_uniform_float(ShaderAsset *shader, String uniform_name, f32 value, LinearArena *scratch)
{
    GLint location = get_uniform_location(shader, uniform_name, scratch);

    glUniform1f(location, value);
}

static void flush_if_needed(RendererBackend *backend, s32 vertices_to_draw, ssize indices_to_draw)
{
    if (((backend->vertex_count + vertices_to_draw) > MAX_RENDERER_VERTICES)
     || ((backend->index_count + indices_to_draw) > MAX_RENDERER_VERTICES)) {
        renderer_backend_flush(backend);
    }
}


void renderer_backend_clear_color_buffer(RGBA32 color)
{
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void renderer_backend_flush(RendererBackend *backend)
{
    glBindBuffer(GL_ARRAY_BUFFER, backend->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, (usize)backend->vertex_count * sizeof(*backend->vertices),
        backend->vertices);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, backend->ebo);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, (usize)backend->index_count * sizeof(*backend->indices),
        backend->indices);

    glBindVertexArray(backend->vao);
    glDrawElements(GL_TRIANGLES, backend->index_count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    backend->vertex_count = 0;
    backend->index_count = 0;
}

void renderer_backend_draw_triangle(RendererBackend *backend, Vertex a, Vertex b, Vertex c)
{
    flush_if_needed(backend, 3, 3);

    u32 start_count = (u32)backend->vertex_count;

    backend->vertices[backend->vertex_count++] = a;
    backend->vertices[backend->vertex_count++] = b;
    backend->vertices[backend->vertex_count++] = c;

    backend->indices[backend->index_count++] = start_count + 0;
    backend->indices[backend->index_count++] = start_count + 1;
    backend->indices[backend->index_count++] = start_count + 2;
}

void renderer_backend_draw_quad(RendererBackend *backend, Vertex a, Vertex b, Vertex c, Vertex d)
{
    flush_if_needed(backend, 4, 6);

    u32 start_count = (u32)backend->vertex_count;

    backend->vertices[backend->vertex_count++] = a;
    backend->vertices[backend->vertex_count++] = b;
    backend->vertices[backend->vertex_count++] = c;
    backend->vertices[backend->vertex_count++] = d;

    backend->indices[backend->index_count++] = start_count + 0;
    backend->indices[backend->index_count++] = start_count + 2;
    backend->indices[backend->index_count++] = start_count + 3;
    backend->indices[backend->index_count++] = start_count + 0;
    backend->indices[backend->index_count++] = start_count + 1;
    backend->indices[backend->index_count++] = start_count + 2;
}

void renderer_backend_change_framebuffer(RendererBackend *backend, FrameBuffer render_target)
{
    GLuint fbo = get_framebuffer_object(backend, render_target);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void renderer_backend_change_to_main_framebuffer(RendererBackend *backend)
{
    (void)backend;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renderer_backend_blend_framebuffers(RendererBackend *backend, FrameBuffer a,
    FrameBuffer b, ShaderAsset *shader)
{
    // Bind main framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    renderer_backend_set_stencil_function(backend, STENCIL_FUNCTION_ALWAYS, 0);
    renderer_backend_set_blend_function(BLEND_FUNCTION_MULTIPLICATIVE);

    // Render a rectangle in screen space covering the entire screen
    Rectangle rect = {{-1, -1}, {2, 2}};
    RectangleVertices verts = rect_get_vertices(rect, RGBA32_WHITE, Y_IS_DOWN);

    renderer_backend_draw_quad(backend, verts.top_left, verts.top_right,
        verts.bottom_right, verts.bottom_left);

    GLuint texture_a = get_render_target_texture(backend, a);
    GLuint texture_b = get_render_target_texture(backend, b);

    GLint loc_a = glGetUniformLocation(shader->native_handle, "u_texture_a");
    GLint loc_b = glGetUniformLocation(shader->native_handle, "u_texture_b");
    ASSERT(loc_a != -1);
    ASSERT(loc_b != -1);

    renderer_backend_use_shader(shader);

    // Bind the two texture samplers to separate texture units
    glBindTexture(GL_TEXTURE_2D, texture_b);

    glUniform1i(loc_a, 0);
    glUniform1i(loc_b, 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_a);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture_b);

    // Reset to the default texture unit so the active texture doesn't leak into next frame
    glActiveTexture(GL_TEXTURE0);

    renderer_backend_flush(backend);
}

void renderer_backend_draw_framebuffer_as_texture(RendererBackend *backend, FrameBuffer render_target,
    ShaderAsset *shader)
{
    renderer_backend_change_to_main_framebuffer(backend);
    renderer_backend_use_shader(shader);
    renderer_backend_set_blend_function(BLEND_FUNCTION_MULTIPLICATIVE);
    renderer_backend_set_stencil_function(backend, STENCIL_FUNCTION_ALWAYS, 0);

    GLuint texture = get_render_target_texture(backend, render_target);
    glBindTexture(GL_TEXTURE_2D, texture);

    Rectangle rect = {{-1, -1}, {2, 2}};
    RectangleVertices verts = rect_get_vertices(rect, RGBA32_WHITE, Y_IS_DOWN);

    renderer_backend_draw_quad(backend, verts.top_left, verts.top_right,
        verts.bottom_right, verts.bottom_left);

    renderer_backend_flush(backend);

    glBindTexture(GL_TEXTURE_2D, 0);
}

static void clear_all_buffers(RGBA32 color)
{
    glClearColor(color.r, color.g, color.b, color.a);
    glClearStencil(0);

    renderer_backend_enable_stencil_writes();
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    renderer_backend_disable_stencil_writes();
}

void renderer_backend_begin_frame(RendererBackend *backend)
{
    renderer_backend_change_to_main_framebuffer(backend);
    clear_all_buffers(RGBA32_BLACK);

    for (FrameBuffer target = 0; target < FRAME_BUFFER_COUNT; ++target) {
        renderer_backend_change_framebuffer(backend, target);

        // Offscreen buffers are cleared to transparent so that they can be
        // sampled as textures
        clear_all_buffers(RGBA32_TRANSPARENT);
    }
}

void renderer_backend_set_blend_function(BlendFunction function)
{
    switch (function) {
        case BLEND_FUNCTION_ADDITIVE: {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        } break;

        case BLEND_FUNCTION_MULTIPLICATIVE: {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        } break;
    }
}

void renderer_backend_enable_stencil_writes()
{
    glStencilMask(0xFF);
}

void renderer_backend_disable_stencil_writes()
{
    glStencilMask(0x00);
}

void renderer_backend_set_stencil_pass_operation(RendererBackend *backend, StencilOperation op)
{
    (void)backend;

    GLenum gl_enum = 0;

    switch (op) {
        case STENCIL_OP_REPLACE: {
            gl_enum = GL_REPLACE;
        } break;

        case STENCIL_OP_KEEP: {
            gl_enum = GL_KEEP;
        } break;

        INVALID_DEFAULT_CASE;
    }

    glStencilOp(gl_enum, gl_enum, gl_enum);
}
void renderer_backend_set_stencil_function(RendererBackend *backend, StencilFunction function, s32 arg)
{
    (void)backend;

    GLenum gl_enum = 0;

    switch (function) {
        case STENCIL_FUNCTION_ALWAYS: {
            gl_enum = GL_ALWAYS;
        } break;

        case STENCIL_FUNCTION_EQUAL: {
            gl_enum = GL_EQUAL;
        } break;

        case STENCIL_FUNCTION_NOT_EQUAL: {
            gl_enum = GL_NOTEQUAL;
        } break;

        INVALID_DEFAULT_CASE;
    }

    glStencilFunc(gl_enum, arg, 0xFF);
}

void renderer_backend_enable_color_buffer_writes(RendererBackend *backend)
{
    (void)backend;
    glColorMask(true, true, true, true);
}

void renderer_backend_disable_color_buffer_writes(RendererBackend *backend)
{
    (void)backend;
    glColorMask(false, false, false, false);
}
