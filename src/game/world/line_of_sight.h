#ifndef LINE_OF_SIGHT_H
#define LINE_OF_SIGHT_H

#include "base/vector.h"

struct Tilemap;
struct Entity;
struct PhysicsComponent;

Vector2 find_first_wall_in_direction(struct Tilemap *tilemap, Vector2 origin, Vector2 dir);
b32 has_line_of_sight_to_entity(struct Entity *self, struct PhysicsComponent *self_physics,
                                struct Entity *other, struct Tilemap *tilemap);
Vector2 find_first_wall_on_path(Vector2 origin, Vector2 target, struct Tilemap *tilemap);

#endif //LINE_OF_SIGHT_H
