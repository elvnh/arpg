#ifndef CHAIN_H
#define CHAIN_H

#include "entity/entity_id.h"

/*
  ChainComponents are (for now) used for spells that chain off collided entities.
  Entities which are linked together have their lifetimes linked, so when one chain link dies,
  all entities it is linked with die too.
 */

struct Entity;
struct EntitySystem;

typedef struct {
    EntityID next_link_entity_id;
    EntityID prev_link_entity_id;
    EntityID chained_off_entity_id;
} ChainComponent;

void push_link_to_front_of_chain(struct EntitySystem *es, ChainComponent *root, ChainComponent *next);
b32 has_chained_off_entity(struct EntitySystem *es, ChainComponent *root, EntityID id);
struct Entity *get_next_entity_in_chain(struct EntitySystem *es, ChainComponent *link);
struct Entity *get_previous_entity_in_chain(struct EntitySystem *es, ChainComponent *link);
void remove_link_from_chain(struct EntitySystem *es, ChainComponent *link);


#endif //CHAIN_H
