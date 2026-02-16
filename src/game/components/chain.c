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

void push_link_to_front_of_chain(EntitySystem *es, ChainComponent *root, ChainComponent *next)
{
    Entity *root_entity = es_get_component_owner(es, root, ChainComponent);
    Entity *link_entity = es_get_component_owner(es, next, ChainComponent);
    ASSERT(root_entity != link_entity);

    next->next_link_entity_id = root->next_link_entity_id;
    root->next_link_entity_id = link_entity->id;
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
