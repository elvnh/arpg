#ifndef CHAIN_H
#define CHAIN_H

#include "entity/entity_id.h"

struct Entity;
struct EntitySystem;

typedef enum {
    CHAIN_KIND_ROOT,
    CHAIN_KIND_LINK,
} ChainElementKind;

// TODO: better names
typedef enum {
    CHAIN_BEHAVIOUR_CONTINUE_CURRENT_CHAIN, /* Chain off the current link */
    CHAIN_BEHAVIOUR_BEGIN_NEW_CHAIN,        /* Create a new chain */
} ChainingBehaviour;

typedef struct {
    EntityID next_link_entity_id;


    EntityID chain_root_id; // Null if this is the root

    /* If this link was created after chaining off an entity,
       we save that entities ID, otherwise it's null.
       This is used to ensure that a chain doesn't return to an entity
       after chaining off it.
     */
    EntityID chained_off_entity_id;
} ChainComponent;

void append_chain_link(struct EntitySystem *es, ChainComponent *root, ChainComponent *next);
b32 has_chained_off_entity(struct EntitySystem *es, ChainComponent *root, EntityID id);
struct Entity *get_next_entity_in_chain(struct EntitySystem *es, ChainComponent *link);

#endif //CHAIN_H
