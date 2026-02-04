#ifndef VERTEX_BUFFER_H
#define VERTEX_BUFFER_H

#define VERTEX_BUFFER_CAPACITY 512

typedef struct {
    Vertex    vertices[VERTEX_BUFFER_CAPACITY];
    s32       vertex_count;

    uint32_t  indices[VERTEX_BUFFER_CAPACITY];
    s32       index_count;
} VertexBuffer;


static inline b32 vertex_buffer_flush_needed(VertexBuffer *vb, s32 vertices_to_draw, ssize indices_to_draw)
{
    b32 result = (vb->vertex_count + vertices_to_draw > ARRAY_COUNT(vb->vertices))
        || ((vb->index_count + indices_to_draw) > ARRAY_COUNT(vb->indices));

    return result;
}

static inline void vertex_buffer_push_triangle(VertexBuffer *vb, Vertex a, Vertex b, Vertex c)
{
    u32 start_count = (u32)vb->vertex_count;

    vb->vertices[vb->vertex_count++] = a;
    vb->vertices[vb->vertex_count++] = b;
    vb->vertices[vb->vertex_count++] = c;

    vb->indices[vb->index_count++] = start_count + 0;
    vb->indices[vb->index_count++] = start_count + 1;
    vb->indices[vb->index_count++] = start_count + 2;
}

static inline void vertex_buffer_push_quad(VertexBuffer *vb, Vertex a, Vertex b, Vertex c, Vertex d)
{
    u32 start_count = (u32)vb->vertex_count;

    vb->vertices[vb->vertex_count++] = a;
    vb->vertices[vb->vertex_count++] = b;
    vb->vertices[vb->vertex_count++] = c;
    vb->vertices[vb->vertex_count++] = d;

    vb->indices[vb->index_count++] = start_count + 0;
    vb->indices[vb->index_count++] = start_count + 2;
    vb->indices[vb->index_count++] = start_count + 3;
    vb->indices[vb->index_count++] = start_count + 0;
    vb->indices[vb->index_count++] = start_count + 1;
    vb->indices[vb->index_count++] = start_count + 2;
}

static inline void vertex_buffer_reset(VertexBuffer *vb)
{
    vb->vertex_count = 0;
    vb->index_count = 0;
}

#endif //VERTEX_BUFFER_H
