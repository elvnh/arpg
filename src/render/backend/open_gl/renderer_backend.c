#include <GL/glew.h>

#include "base/allocator.h"
#include "base/string8.h"
#include "base/utils.h"
#include "render/backend/renderer_backend.h"
#include "render/vertex.h"
#include "os/thread_context.h"

#define MAX_RENDERER_VERTICES 512

#define POSITION_ATTRIBUTE    0
#define UV_ATTRIBUTE          1
#define COLOR_ATTRIBUTE       2

#define VERTEX_SHADER_DIRECTIVE    str_lit("#vertex")
#define FRAGMENT_SHADER_DIRECTIVE  str_lit("#fragment")

struct RendererBackend {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;

    Vertex  vertices[MAX_RENDERER_VERTICES]; // TODO: heap allocate
    s32     vertex_count;

    GLuint  indices[MAX_RENDERER_VERTICES]; // TODO: heap allocate
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

    fprintf(stderr, "%s\nSeverity: 0x%x\nMessage: %s\n",
      (type == GL_DEBUG_TYPE_ERROR ? "** OpenGL ERROR **" : "** OpenGL INFO **"),
      severity, message);

    if (type == GL_DEBUG_TYPE_ERROR)
        abort();
}

RendererBackend *renderer_backend_initialize(Allocator allocator)
{
    RendererBackend *state = allocate_item(allocator, RendererBackend);
    ASSERT(state->vertex_count == 0);
    ASSERT(state->index_count == 0);

    glewInit();

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(gl_error_callback, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

    glBindVertexArray(0);

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

        Allocator scratch = thread_ctx_get_allocator();
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

    const char *const vertex_src_ptr = split_result.vertex_shader.data;
    s32 vertex_src_length = safe_cast_ssize_s32(split_result.vertex_shader.length);

    const char *const frag_src_ptr = split_result.fragment_shader.data;
    s32 frag_src_length = safe_cast_ssize_s32(split_result.fragment_shader.length);

    glShaderSource(vertex_shader_id, 1, &vertex_src_ptr, &vertex_src_length);
    glCompileShader(vertex_shader_id);

    glShaderSource(frag_shader_id, 1, &frag_src_ptr, &frag_src_length);
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

    ShaderAsset *handle = allocate_item(allocator, ShaderAsset);
    handle->native_handle = program_id;

    return handle;
}

void renderer_backend_destroy_shader(ShaderAsset *shader)
{
    glDeleteProgram(shader->native_handle);
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

    GLenum channel = (image.channels == 4) ? GL_RGBA : GL_RGB;

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

void renderer_backend_destroy_texture(TextureAsset *texture)
{
    glDeleteTextures(1, &texture->native_handle);
}

void renderer_backend_use_shader(ShaderAsset *handle)
{
    glUseProgram(handle->native_handle);
}

void renderer_backend_bind_texture(TextureAsset *texture)
{
    glBindTexture(GL_TEXTURE_2D, texture->native_handle);
}

void renderer_backend_set_mat4_uniform(ShaderAsset *shader, String uniform_name, Matrix4 matrix)
{
    String terminated = str_null_terminate(uniform_name, thread_ctx_get_allocator());
    GLint location = glGetUniformLocation(shader->native_handle, terminated.data);
    ASSERT(location != -1);

    glUniformMatrix4fv(location, 1, GL_FALSE, mat4_ptr(&matrix));
}

static void flush_if_needed(RendererBackend *backend, s32 vertices_to_draw, ssize indices_to_draw)
{
    if (((backend->vertex_count + vertices_to_draw) > MAX_RENDERER_VERTICES)
     || ((backend->index_count + indices_to_draw) > MAX_RENDERER_VERTICES)) {
        renderer_backend_end_frame(backend);
    }
}

void renderer_backend_begin_frame(RendererBackend *backend)
{
    glClear(GL_COLOR_BUFFER_BIT);

    backend->vertex_count = 0;
    backend->index_count = 0;
}

void renderer_backend_end_frame(RendererBackend *backend)
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
    flush_if_needed(backend, 4, 4);

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
