#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "platform/file.h"
#include "base/utils.h"

ReadFileResult Platform_ReadEntireFile(String path, Allocator allocator)
{
    ssize file_size = Platform_GetFileSize(path, allocator);

    if ((file_size == -1) || AdditionOverflows_ssize(file_size, 1)) {
        return (ReadFileResult){0};
    }

    // TODO: null terminating twice here
    const ssize alloc_size = file_size + 1;
    String null_terminated = String_NullTerminate(path, allocator);

    byte *file_data = AllocArray(allocator, byte, alloc_size);
    Assert(file_data);

    s32 fd = open(null_terminated.data, O_RDONLY);

    if (fd == -1) {
        return (ReadFileResult){0};
    }

    ssize bytes_read = read(fd, file_data, Cast_s64_usize(file_size));
    Assert(bytes_read == file_size);

    file_data[file_size] = '\0';

    ReadFileResult result = {
        .file_data = file_data,
        .file_size = file_size
    };

    return result;
}

ssize Platform_GetFileSize(String path, Allocator allocator)
{
    String null_terminated = String_NullTerminate(path, allocator);

    struct stat st;
    s32 result = stat(null_terminated.data, &st);

    if ((result == -1) || !S_ISREG(st.st_mode)) {
        Assert(false);

        return -1;
    }

    return st.st_size;
}
