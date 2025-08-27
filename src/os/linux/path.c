#define _POSIX_C_SOURCE 200112L

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/limits.h>

#include "os/path.h"
#include "base/allocator.h"
#include "base/string8.h"

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


    char *result_path = allocate_array(allocator, char, PATH_MAX);
    ssize bytes_written = readlink("/proc/self/exe", result_path, PATH_MAX);

    if (bytes_written == -1) {
        deallocate(allocator, result_path);
    } else {
        try_resize_allocation(allocator, result_path, PATH_MAX, bytes_written);
    }

    String result = {
        .data = result_path,
        .length = bytes_written
    };

    return result;
}
