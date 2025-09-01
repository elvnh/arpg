#ifndef FREE_LIST_ARENA_H
#define FREE_LIST_ARENA_H

#include "base/allocator.h"
#include "typedefs.h"

// typedef struct ArenaBuffer {

// } ArenaBuffer;

typedef struct {
    byte *memory;
    struct FreeBlock *head;
    struct FreeBlock *tail;

    ssize capacity;
    Allocator parent;
} FreeListArena;

FreeListArena fl_create(Allocator parent, ssize capacity);
void *fl_allocate(void *context, ssize item_count, ssize item_size, ssize alignment);
void  fl_deallocate(void *context, void *ptr);

#endif //FREE_LIST_ARENA_H
