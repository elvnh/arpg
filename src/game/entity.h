#ifndef ENTITY_H
#define ENTITY_H

#include "base/vector.h"
#include "base/rgba.h"

/*
  TODO:
  - Removing entities
  - Entity ID queue
  - Entity slot generations
  - Dense array of alive entity ids
  - Components
 */

typedef u32 EntityID_Type;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    Vector2 size;
    RGBA32 color;
} Entity;

typedef struct {
    Entity        entities[32];
    EntityID_Type entity_count;
} EntityStorage;

typedef struct {
    EntityID_Type id;
} EntityID;

EntityID es_create_entity(EntityStorage *es);
Entity  *es_get_entity(EntityStorage *es, EntityID id);

#endif //ENTITY_H
