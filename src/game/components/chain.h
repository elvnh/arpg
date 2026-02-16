#ifndef CHAIN_H
#define CHAIN_H

#include "entity/entity_id.h"

struct Entity;
struct EntitySystem;

typedef struct {
    EntityID next_link_entity_id;
    EntityID chained_off_entity_id;
} ChainComponent;

void push_link_to_front_of_chain(struct EntitySystem *es, ChainComponent *root, ChainComponent *next);
b32 has_chained_off_entity(struct EntitySystem *es, ChainComponent *root, EntityID id);
struct Entity *get_next_entity_in_chain(struct EntitySystem *es, ChainComponent *link);

#endif //CHAIN_H
