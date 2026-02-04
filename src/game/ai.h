#ifndef AI_H
#define AI_H

#include "entity/entity_id.h"

struct World;
struct Entity;
struct AIComponent;

typedef enum {
    AI_STATE_IDLE,
    AI_STATE_CHASING,
} AIStateKind;

typedef struct {
    AIStateKind kind;

    union {
	struct {
	    EntityID target;
	} chasing;
    } as;
} AIState;

void entity_update_ai(struct World *world, struct Entity *entity, struct AIComponent *ai);

#endif //AI_H
