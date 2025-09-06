#ifndef VERTEX_H
#define VERTEX_H

#include "base/vector.h"
#include "base/rgba.h"

typedef struct {
    Vector2  position;
    Vector2  uv;
    RGBA32   color;
} Vertex;

#endif //VERTEX_H
