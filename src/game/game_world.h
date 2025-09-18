#ifndef GAME_WORLD_H
#define GAME_WORLD_H

#include "entity.h"
#include "tilemap.h"
#include "camera.h"

typedef struct {
    Camera camera;
    Tilemap tilemap;
    EntityStorage entities;
} GameWorld;

#endif //GAME_WORLD_H
