#ifndef COLLISION_EVENT_H
#define COLLISION_EVENT_H

#include "base/typedefs.h"
#include "entity/entity.h"

struct World;
struct LinearArena;

// CollisionEvents are used to record each unique collision that occurs each frame to avoid
// executing collision resolution and collision effects more than once.
typedef struct CollisionEvent {
    struct CollisionEvent *next;
    EntityPair ordered_entity_pair;
} CollisionEvent;

typedef struct {
    CollisionEvent *head;
    CollisionEvent *tail;
} CollisionEventList;

typedef struct {
    CollisionEventList *table;
    ssize               table_size;
    LinearArena         arena;
} CollisionEventTable;

CollisionEventTable collision_event_table_create(struct LinearArena *parent_arena);
CollisionEvent *collision_event_table_find(CollisionEventTable *table, EntityID a, EntityID b);
void collision_event_table_insert(CollisionEventTable *table, EntityID a, EntityID b);
b32 entities_intersected_this_frame(struct World *world, EntityID a, EntityID b);
b32 entities_intersected_previous_frame(struct World *world, EntityID a, EntityID b);

#endif //COLLISION_EVENT_H
