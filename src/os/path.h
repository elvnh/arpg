#ifndef PATH_H
#define PATH_H

#include "base/string8.h"

// NOTE: Paths should never end with /, only append slash if also appending something else to the path.

String  os_get_executable_directory(Allocator allocator);
bool    os_path_is_absolute(String path);
String  os_get_absolute_path(String path, Allocator allocator);
String  os_get_canonical_path(String path, Allocator allocator);
String  os_get_working_directory(Allocator allocator);
String  os_get_parent_path(String path, Allocator allocator);

#endif //PATH_H
