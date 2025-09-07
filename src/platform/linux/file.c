#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "platform/file.h"
#include "base/utils.h"

// NOTE: path must be null terminated
ssize os_get_file_size_impl(const char *path)
{
    struct stat st;
    s32 result = stat(path, &st);

    if ((result == -1) || !S_ISREG(st.st_mode)) {
        ASSERT(false);

        return -1;
    }

    return st.st_size;
}

Span os_read_entire_file(String path, Allocator allocator, LinearArena *scratch)
{
    String null_terminated = str_null_terminate(path, la_allocator(scratch));
    ssize file_size = os_get_file_size_impl(null_terminated.data);

    Span result = {0};

    if ((file_size == -1) || add_overflows_ssize(file_size, 1)) {
        return result;
    }

    ssize alloc_size = file_size + 1;

    byte *file_data = allocate_array(allocator, byte, alloc_size);
    ASSERT(file_data);

    s32 fd = open(null_terminated.data, O_RDONLY);

    if (fd == -1) {
        deallocate(allocator, file_data);
        return result;
    }

    ssize bytes_read = read(fd, file_data, cast_ssize_to_usize(file_size));
    ASSERT(bytes_read == file_size);

    // TODO: is null terminating really necessary?
    file_data[file_size] = '\0';

    result.data = file_data;
    result.size = file_size;

    return result;
}

String os_read_entire_file_as_string(String path, Allocator allocator, LinearArena *scratch)
{
    Span file_contents = os_read_entire_file(path, allocator, scratch);
    String result = { (char *)file_contents.data, file_contents.size };

    return result;
}

ssize os_get_file_size(String path, LinearArena *scratch)
{
    String null_terminated = str_null_terminate(path, la_allocator(scratch));

    return os_get_file_size_impl(null_terminated.data);
}
