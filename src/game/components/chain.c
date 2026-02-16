#include "chain.h"
#include "base/utils.h"
#include "entity/entity_id.h"
#include "entity/entity_system.h"

static ChainComponent *get_next_link_in_chain(EntitySystem *es, ChainComponent *current)
{
    Entity *entity = get_next_entity_in_chain(es, current);
    ChainComponent *result = 0;

    // TODO: allow calling es_get_component on null entities
    if (entity) {
	result = es_get_component(entity, ChainComponent);
    }

    return result;
}

static ChainComponent *get_previous_link_in_chain(EntitySystem *es, ChainComponent *current)
{
    Entity *entity = get_previous_entity_in_chain(es, current);
    ChainComponent *result = 0;

    // TODO: allow calling es_get_component on null entities
    if (entity) {
	result = es_get_component(entity, ChainComponent);
    }

    return result;
}


void push_link_to_front_of_chain(EntitySystem *es, ChainComponent *root, ChainComponent *link_to_push)
{
    Entity *root_entity = es_get_component_owner(es, root, ChainComponent);
    Entity *link_to_push_entity = es_get_component_owner(es, link_to_push, ChainComponent);
    ASSERT(root_entity != link_to_push_entity);

    ChainComponent *next = get_next_link_in_chain(es, root);

    if (next) {
	next->prev_link_entity_id = link_to_push_entity->id;
    }

    link_to_push->next_link_entity_id = root->next_link_entity_id;
    link_to_push->prev_link_entity_id = root_entity->id;
    root->next_link_entity_id = link_to_push_entity->id;
}

b32 has_chained_off_entity(EntitySystem *es, ChainComponent *root, EntityID id)
{
    b32 result = false;

    ChainComponent *current_link = root;

    while (current_link) {
        ASSERT(current_link);

        if (entity_id_equal(current_link->chained_off_entity_id, id)) {
            result = true;
            break;
        }

        current_link = get_next_link_in_chain(es, current_link);
    }

    return result;
}

Entity *get_next_entity_in_chain(EntitySystem *es, ChainComponent *link)
{
    Entity *result = es_try_get_entity(es, link->next_link_entity_id);
    return result;
}

struct Entity *get_previous_entity_in_chain(EntitySystem *es, ChainComponent *link)
{
    Entity *result = es_try_get_entity(es, link->prev_link_entity_id);
    return result;
}

void remove_link_from_chain(EntitySystem *es, ChainComponent *link)
{
    ChainComponent *prev = get_previous_link_in_chain(es, link);
    ChainComponent *next = get_next_link_in_chain(es, link);

    if (prev) {
	prev->next_link_entity_id = link->next_link_entity_id;
    }

    if (next) {
	next->prev_link_entity_id = link->prev_link_entity_id;
    }

    link->next_link_entity_id = NULL_ENTITY_ID;
    link->prev_link_entity_id = NULL_ENTITY_ID;
    link->chained_off_entity_id = NULL_ENTITY_ID;
}
