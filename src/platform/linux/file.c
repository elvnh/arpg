#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "platform/file.h"
#include "base/utils.h"

// NOTE: path must be null terminated
ssize Platform_GetFileSizeImpl(const char *path)
{
    struct stat st;
    s32 result = stat(path, &st);

    if ((result == -1) || !S_ISREG(st.st_mode)) {
        Assert(false);

        return -1;
    }

    return st.st_size;
}

ReadFileResult Platform_ReadEntireFile(String path, Allocator allocator)
{
    String null_terminated = String_NullTerminate(path, allocator);
    ssize file_size = Platform_GetFileSizeImpl(null_terminated.data);

    if ((file_size == -1) || AdditionOverflows_ssize(file_size, 1)) {
        return (ReadFileResult){0};
    }

    ssize alloc_size = file_size + 1;

    byte *file_data = AllocArray(allocator, byte, alloc_size);
    Assert(file_data);

    s32 fd = open(null_terminated.data, O_RDONLY);

    if (fd == -1) {
        return (ReadFileResult){0};
    }

    ssize bytes_read = read(fd, file_data, Cast_s64_usize(file_size));
    Assert(bytes_read == file_size);

    // TODO: is null terminating really necessary?
    file_data[file_size] = '\0';

    ReadFileResult result = {
        .file_data = file_data,
        .file_size = file_size
    };

    return result;
}

ssize Platform_GetFileSize(String path, LinearArena scratch)
{
    String null_terminated = String_NullTerminate(path, LinearArena_Allocator(&scratch));


    return Platform_GetFileSizeImpl(null_terminated.data);
}
