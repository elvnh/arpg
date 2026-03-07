#ifndef VERTEX_H
#define VERTEX_H

#include "base/rgba.h"
#include "base/vector.h"

typedef struct {
    Vector2 position;
    Vector2 uv;
    RGBA32 color;
} Vertex;

#endif //VERTEX_H
