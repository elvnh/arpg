#include "path.h"

#include "base/dynamic_array.h"
#include "base/scratch.h"
#include "platform.h"

static ssize path_common_prefix_length(Path a, Path b)
{
    ssize min_length = MIN(a.count, b.count);
    ssize result = 0;
    for (ssize i = 0; i < min_length; ++i) {
        if (str_equal(a.items[i], b.items[i])) {
            ++result;
        }
    }

    return result;
}

Path path_create(ssize capacity, Allocator allocator)
{
    Path result = {0};
    da_init(&result, capacity, allocator);

    return result;
}

void path_append_inplace(Path *path, String component, Allocator allocator)
{
    ASSERT(((path->count == 0) || !str_starts_with(component, str("/")))
           && "Can't append / to non-empty path");

    String trimmed = component;

    // Trim slash from end of path component, unless it consists of a single slash
    if ((trimmed.length > 1) && str_ends_with(trimmed, str("/"))) {
        --trimmed.length;

        ASSERT(!str_ends_with(trimmed, str("/")) && "Multiple trailing slashes");
    }

    da_push(path, trimmed, allocator);
}

Path path_append(Path path, String component, Allocator allocator)
{
    Path result = {0};
    result.items = allocate_array(allocator, String, path.capacity);
    result.count = path.count;
    result.capacity = path.capacity;

    memcpy(result.items, path.items, ssize_to_usize(path.count) * sizeof(*path.items));

    path_append_inplace(&result, component, allocator);

    return result;
}

Path path_from_str(String s, Allocator allocator)
{
    Path result = path_create(32, allocator);

    Cut cut = str_cut(s, str("/"));

    // If path is absolute, a / is the first component
    if (cut.head.length == 0) {
        path_append_inplace(&result, str("/"), allocator);
        cut = str_cut(cut.tail, str("/"));
    }

    while (true) {
        // Skip trailing slashes
        if (cut.head.length > 0) {
            path_append_inplace(&result, cut.head, allocator);
        }

        if (!cut.ok) {
            break;
        }

        cut = str_cut(cut.tail, str("/"));
    }

    return result;
}

String path_to_str(Path path, Allocator allocator)
{
    String result = str("");

    for (ssize i = 0; i < path.count; ++i) {
        String component = path.items[i];
        result = str_concat(result, component, allocator);

        if (i != (path.count - 1) && !str_equal(component, str("/"))) {
            result = str_concat(result, str("/"), allocator);
        }
    }

    return result;
}

String path_make_relative_to(String path, String base, Allocator allocator)
{
    TempArena temp = temp_arena_begin();
    Allocator temp_allocator = la_allocator(&temp.arena);

    String canonical_path = platform_get_canonical_path(path, temp_allocator);
    String canonical_base = platform_get_canonical_path(base, temp_allocator);

    ASSERT(canonical_path.data);
    ASSERT(canonical_base.data);

    Path path_comps = path_from_str(canonical_path, temp_allocator);
    Path base_comps = path_from_str(canonical_base, temp_allocator);

    ssize common_prefix_length = path_common_prefix_length(path_comps, base_comps);
    ssize steps_up = base_comps.count - common_prefix_length; // TODO: name

    Path result_path = path_create(16, temp_allocator);

    for (ssize i = 0; i < steps_up; ++i) {
        path_append_inplace(&result_path, str(".."), temp_allocator);
    }

    for (ssize i = common_prefix_length; i < path_comps.count; ++i) {
        path_append_inplace(&result_path, path_comps.items[i], temp_allocator);
    }

    String result = path_to_str(result_path, allocator);

    temp_arena_end(temp);

    return result;
}
