#ifndef SCRATCH_H
#define SCRATCH_H

#include "linear_arena.h"
#include "typedefs.h"

LinearArena temp_arena_begin(void);
void temp_arena_end(LinearArena *arena);

#endif // SCRATCH_H
