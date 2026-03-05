#ifndef AI_H
#define AI_H

#include "entity/entity_id.h"

struct World;
struct Entity;

typedef enum {
    AI_STATE_IDLE,
    AI_STATE_CHASING,
} AIStateKind;

typedef enum {
    AI_BEHAVIOUR_NORMAL_ENEMY,
} AIBehaviour;

typedef struct {
    AIStateKind kind;

    union {
	struct {
	    EntityID target;
	} chasing;
    } as;
} AIState;

typedef struct {
    AIBehaviour behaviour;
    AIState current_state;
} AIComponent;

void entity_update_ai(struct World *world, struct Entity *entity, AIComponent *ai);

#endif //AI_H
