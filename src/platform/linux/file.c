#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "platform/file.h"
#include "base/utils.h"

// NOTE: path must be null terminated
ssize platform_get_file_size_impl(const char *path)
{
    struct stat st;
    s32 result = stat(path, &st);

    if ((result == -1) || !S_ISREG(st.st_mode)) {
        ASSERT(false);

        return -1;
    }

    return st.st_size;
}

ReadFileResult platform_read_entire_file(String path, Allocator allocator)
{
    String null_terminated = str_null_terminate(path, allocator);
    ssize file_size = platform_get_file_size_impl(null_terminated.data);

    if ((file_size == -1) || add_overflows_ssize(file_size, 1)) {
        return (ReadFileResult){0};
    }

    ssize alloc_size = file_size + 1;

    byte *file_data = allocate_array(allocator, byte, alloc_size);
    ASSERT(file_data);

    s32 fd = open(null_terminated.data, O_RDONLY);

    if (fd == -1) {
        return (ReadFileResult){0};
    }

    ssize bytes_read = read(fd, file_data, cast_s64_to_usize(file_size));
    ASSERT(bytes_read == file_size);

    // TODO: is null terminating really necessary?
    file_data[file_size] = '\0';

    ReadFileResult result = {
        .file_data = file_data,
        .file_size = file_size
    };

    return result;
}

ssize platform_get_file_size(String path, LinearArena scratch)
{
    String null_terminated = str_null_terminate(path, arena_create_allocator(&scratch));


    return platform_get_file_size_impl(null_terminated.data);
}
