#define _DEFAULT_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/limits.h>
#include <stdlib.h>

#include "platform/path.h"
#include "base/thread_context.h"

String os_get_executable_directory(Allocator allocator, LinearArena *scratch_arena)
{
    /*
      NOTE: This entire PATH_MAX business seems kind of bad. It's apparently
      not required to be defined and apparently isn't always correct which
      means the path might be truncated, which I believe isn't detectable since
      readlink() doesn't append a null byte.
      Also, if this is an allocator that doesn't support resizing, the allocation will
      stay at PATH_MAX bytes which is probably equal to 4096.
    */

    ssize buffer_size = PATH_MAX;
    String result = str_allocate(buffer_size, allocator);
    ssize bytes_written = readlink("/proc/self/exe", result.data, cast_ssize_to_usize(result.length));

    if (bytes_written == -1) {
        deallocate(allocator, result.data);
    } else {
        // NOTE: os_get_parent_path doesn't reallocate so we can just update
        // the size of the string
        result.length = bytes_written;

        String parent_dir = os_get_parent_path(result, allocator, scratch_arena);
        ASSERT(parent_dir.data == result.data);

        result.length = parent_dir.length;
    }

    return result;
}

bool os_path_is_absolute(String path)
{
    return str_starts_with(path, str_lit("/"));
}

String os_get_absolute_path(String path, Allocator allocator, LinearArena *scratch_arena)
{
    if (os_path_is_absolute(path)) {
        return path;
    }

    Allocator scratch = la_allocator(scratch_arena);

    String working_dir = str_concat(os_get_working_directory(scratch), str_lit("/"), scratch);
    String result = str_concat(working_dir, path, allocator);

    return result;
}

String os_get_canonical_path(String path, Allocator allocator, LinearArena *scratch_arena)
{
    Allocator scratch = la_allocator(scratch_arena);

    String absolute = os_get_absolute_path(path, scratch, scratch_arena);
    absolute = str_null_terminate(absolute, scratch);

    String canonical = str_allocate(PATH_MAX, allocator);
    char *realpath_result = realpath(absolute.data, canonical.data);

    if (!realpath_result) {
        deallocate(allocator, canonical.data);
        canonical = (String){0};
    } else {
        ASSERT(realpath_result == canonical.data);

        ssize path_length = str_get_null_terminated_length(canonical);
        canonical.length = path_length;
    }

    return canonical;
}

String os_get_working_directory(Allocator allocator)
{
    String result = str_allocate(PATH_MAX + 1, allocator);

    char *getcwd_result = getcwd(result.data, cast_ssize_to_usize(result.length));
    ASSERT(getcwd_result == result.data);

    if (!getcwd_result) {
        deallocate(allocator, result.data);
    } else {
        ssize str_length = str_get_null_terminated_length(result);
        result.length = str_length;
    }

    return result;
}

void os_change_working_directory(String path)
{
    Allocator scratch = default_allocator;
    String terminated = str_null_terminate(path, scratch);
    s32 result = chdir(terminated.data);
    ASSERT(result == 0);
}

String os_get_parent_path(String path, Allocator allocator, LinearArena *scratch_arena)
{
    String absolute = os_get_absolute_path(path, allocator, scratch_arena);
    ssize last_slash_pos = str_find_last_occurence(absolute, str_lit("/"));
    ASSERT(last_slash_pos != -1);

    String result = {
        .data = absolute.data,
        .length = MAX(1, last_slash_pos) // In case path is only a /
    };

    return result;
}
