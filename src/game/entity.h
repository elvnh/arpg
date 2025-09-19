#ifndef ENTITY_H
#define ENTITY_H

#include "base/vector.h"
#include "base/rgba.h"

#define MAX_ENTITIES 32

/*
  TODO:
  - Removing entities
  - Entity slot generations
  - Dense array of alive entity ids
  - Components
*/

typedef s32 EntityIndex;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    Vector2 size;
    RGBA32 color;
} Entity;

typedef struct {
    Entity entity;
} EntitySlot;

typedef struct {
    EntityIndex slot_index;
} EntityID;

typedef struct {
    EntityID ids[MAX_ENTITIES];
    ssize head;
    ssize tail;
} EntityIDQueue;

typedef struct {
    EntitySlot entities[MAX_ENTITIES];    
    EntityIDQueue free_id_queue;
} EntityStorage;

void     es_initialize(EntityStorage *es);
EntityID es_create_entity(EntityStorage *es);
Entity  *es_get_entity(EntityStorage *es, EntityID id);

#endif //ENTITY_H
