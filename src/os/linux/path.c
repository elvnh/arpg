#define _DEFAULT_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/limits.h>
#include <stdlib.h>

#include "os/path.h"
#include "os/thread_context.h"

String os_get_executable_directory(Allocator allocator)
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
    ssize bytes_written = readlink("/proc/self/exe", result.data, cast_s64_to_usize(result.length));

    if (bytes_written == -1) {
        deallocate(allocator, result.data);
    } else {
        // NOTE: os_get_absolute_parent_path doesn't reallocate so we can just update
        // the size of the string
        result.length = bytes_written;

        String parent_dir = os_get_parent_path(result, allocator);
        ASSERT(parent_dir.data == result.data);

        bool resized = try_resize_allocation(allocator, result.data, buffer_size, parent_dir.length);
        (void)resized;
        ASSERT(resized);

        result.length = parent_dir.length;
    }

    return result;
}

bool os_path_is_absolute(String path)
{
    return str_starts_with(path, str_lit("/"));
}

String os_get_absolute_path(String path, Allocator allocator)
{
    if (os_path_is_absolute(path)) {
        return path;
    }

    Allocator scratch = thread_ctx_get_allocator();

    String working_dir = str_concat(os_get_working_directory(scratch), str_lit("/"), scratch);
    String result = str_concat(working_dir, path, allocator);

    return result;
}

String os_get_canonical_path(String path, Allocator allocator)
{
    Allocator scratch = thread_ctx_get_allocator();

    String absolute = str_null_terminate(os_get_absolute_path(path, scratch), scratch);

    String canonical = str_allocate(PATH_MAX, allocator);
    char *realpath_result = realpath(absolute.data, canonical.data);

    if (!realpath_result) {
        deallocate(allocator, canonical.data);
        canonical = (String){0};
    } else {
        ASSERT(realpath_result == canonical.data);

        ssize path_length = str_get_null_terminated_length(canonical);
        try_resize_allocation(allocator, canonical.data, canonical.length, path_length);

        canonical.length = path_length;
    }

    return canonical;
}

String os_get_working_directory(Allocator allocator)
{
    String result = str_allocate(PATH_MAX + 1, allocator);

    char *getcwd_result = getcwd(result.data, cast_s64_to_usize(result.length));
    ASSERT(getcwd_result == result.data);

    if (!getcwd_result) {
        deallocate(allocator, result.data);
    } else {
        ssize str_length = str_get_null_terminated_length(result);
        try_resize_allocation(allocator, result.data, result.length, str_length);

        result.length = str_length;
    }

    return result;
}

void os_change_working_directory(String path)
{
    String terminated = str_null_terminate(path, thread_ctx_get_allocator());
    s32 result = chdir(terminated.data);
    ASSERT(result == 0);
}

String os_get_parent_path(String path, Allocator allocator)
{
    String absolute = os_get_absolute_path(path, allocator);
    ssize last_slash_pos = str_find_last_occurence(absolute, str_lit("/"));
    ASSERT(last_slash_pos != -1);

    String result = {
        .data = absolute.data,
        .length = MAX(1, last_slash_pos) // In case path is only a /
    };

    return result;
}
