#ifndef PATH_H
#define PATH_H

#include "base/linear_arena.h"
#include "base/string8.h"

typedef struct {
    String *items;
    ssize count;
    ssize capacity;
} Path;

// TODO: inconsistency in whether paths are stored as strings or Path.
// should always be Path, only converted to String when necessary.

Path path_create(ssize capacity, Allocator allocator);
void path_append_inplace(Path *path, String component, Allocator allocator);
Path path_append(Path path, String component, Allocator allocator);
Path path_from_str(String s, Allocator allocator);
String path_to_str(Path path, Allocator allocator);
String path_make_relative_to(String path, String base, Allocator allocator);

#endif // PATH_H
