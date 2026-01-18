#ifndef LINE_OF_SIGHT_H
#define LINE_OF_SIGHT_H

#include "base/vector.h"

struct Tilemap;

Vector2i find_first_wall_along_path(struct Tilemap *tilemap, Vector2 origin, Vector2 dir);

#endif //LINE_OF_SIGHT_H
