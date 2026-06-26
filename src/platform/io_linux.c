#define _GNU_SOURCE
#include "platform.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fenv.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// TODO: Get rid of the allocator + scratch parameters

Span platform_read_entire_file(String path, Allocator allocator, LinearArena *scratch)
{
    Span result = {0};

    String null_terminated = str_null_terminate(path, la_allocator(scratch));
    s32 fd = open(null_terminated.data, O_RDONLY);

    ssize file_size = platform_get_file_size(null_terminated, scratch);

    if ((fd == -1) || (file_size == -1) || add_overflows_ssize(file_size, 1)) {
        goto done;
    }

    ssize alloc_size = file_size + 1;
    byte *file_data = allocate_array(allocator, byte, alloc_size);
    ASSERT(file_data);

    ssize bytes_read = read(fd, file_data, ssize_to_usize(file_size));
    ASSERT(bytes_read == file_size);

    // TODO: is null terminating really necessary?
    file_data[file_size] = '\0';

    result.data = file_data;
    result.size = file_size;

done:
    close(fd);

    return result;
}

String platform_read_entire_file_as_string(String path, Allocator allocator,
    LinearArena *scratch)
{
    Span file_contents = platform_read_entire_file(path, allocator, scratch);
    String result = {(char *)file_contents.data, file_contents.size};

    return result;
}

ssize platform_get_file_size(String path, LinearArena *scratch)
{
    FileInfo info = platform_get_file_info(path, scratch);

    return info.file_size;
}

String platform_get_working_directory(Allocator allocator)
{
    String result = str_allocate(PATH_MAX + 1, allocator);

    char *getcwd_result = getcwd(result.data, ssize_to_usize(result.length));
    ASSERT(getcwd_result == result.data);

    if (!getcwd_result) {
        deallocate(allocator, result.data);
    } else {
        ssize str_length = str_get_null_terminated_length(result);
        result.length = str_length;
    }

    return result;
}

void platform_change_working_directory(String path)
{
    Allocator scratch = default_allocator;
    String terminated = str_null_terminate(path, scratch);
    s32 result = chdir(terminated.data);
    ASSERT(result == 0);
}

String platform_get_parent_path(String path, Allocator allocator, LinearArena *scratch_arena)
{
    String absolute = platform_get_absolute_path(path, allocator, scratch_arena);
    ssize last_slash_pos = str_find_last_occurence(absolute, str("/"));
    ASSERT(last_slash_pos != -1);

    String result = {
        .data = absolute.data,
        .length = MAX(1, last_slash_pos) // In case path is only a /
    };

    return result;
}

String platform_get_relative_parent_path(String path)
{
    String result = {0};

    ssize last_slash_pos = str_find_last_occurence(path, str("/"));

    if (last_slash_pos != -1) {
        result.data = path.data;
        result.length = MAX(1, last_slash_pos); // In case path is only a /
    }

    return result;
}

String platform_get_filename(String path)
{
    ssize slash_index = str_find_last_occurence(path, str("/"));

    if (slash_index == -1) {
        return path;
    }

    String result = str_create_span(path, slash_index + 1, path.length - slash_index - 1);

    return result;
}

b32 platform_file_exists(String path, LinearArena *scratch)
{
    String null_terminated = str_null_terminate(path, la_allocator(scratch));

    s32 result = access(null_terminated.data, F_OK);

    return (result == 0);
}

FileInfo platform_get_file_info(String path, LinearArena *scratch)
{
    String null_terminated = str_null_terminate(path, la_allocator(scratch));
    struct stat st;
    s32 stat_result = stat(null_terminated.data, &st);

    FileInfo result = {(FileType)-1, -1, {0}};

    if ((stat_result == -1) || !S_ISREG(st.st_mode)) {
        return result;
    }

    Timestamp mod_time = {.seconds = st.st_mtim.tv_sec, .nanoseconds = st.st_mtim.tv_nsec};

    FileType type = FILE_TYPE_OTHER;

    if (S_ISREG(st.st_mode)) {
        type = FILE_TYPE_FILE;
    } else if (S_ISDIR(st.st_mode)) {
        type = FILE_TYPE_DIRECTORY;
    }

    result.type = type;
    result.file_size = st.st_size;
    result.last_modification_time = mod_time;

    return result;
}

void platform_for_each_file_in_dir(String directory, void (*callback)(String),
    LinearArena *scratch)
{
    String null_terminated = str_null_terminate(directory, la_allocator(scratch));
    DIR *dir = opendir(null_terminated.data);
    ASSERT(dir);

    for (struct dirent *entry = readdir(dir); entry; entry = readdir(dir)) {
        String name = {entry->d_name, (ssize)strlen(entry->d_name)};

        // TODO: use new format function for constructing paths
        String full_path = str_concat(str_concat(directory, str("/"), la_allocator(scratch)),
            name, la_allocator(scratch));

        if (entry->d_type == DT_DIR) {
            if (!str_equal(name, str(".")) && !str_equal(name, str(".."))) {
                platform_for_each_file_in_dir(full_path, callback, scratch);
            }
        } else {
            callback(full_path);
        }
    }

    closedir(dir);
}

String platform_get_executable_path(Allocator allocator)
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
    ssize bytes_written =
        readlink("/proc/self/exe", result.data, ssize_to_usize(result.length));

    if (bytes_written == -1) {
        deallocate(allocator, result.data);

        return (String){0};
    }

    result.length = bytes_written;

    return result;
}

String platform_get_executable_directory(Allocator allocator, LinearArena *scratch_arena)
{
    String executable_path = platform_get_executable_path(allocator);
    String directory_path =
        platform_get_parent_path(executable_path, allocator, scratch_arena);

    return directory_path;
}

bool platform_path_is_absolute(String path)
{
    return str_starts_with(path, str("/"));
}

String platform_get_absolute_path(String path, Allocator allocator, LinearArena *scratch_arena)
{
    if (platform_path_is_absolute(path)) {
        return path;
    }

    Allocator scratch = la_allocator(scratch_arena);

    String working_dir =
        str_concat(platform_get_working_directory(scratch), str("/"), scratch);
    String result = str_concat(working_dir, path, allocator);

    return result;
}

String platform_get_canonical_path(String path, Allocator allocator,
    LinearArena *scratch_arena)
{
    Allocator scratch = la_allocator(scratch_arena);

    String absolute = platform_get_absolute_path(path, scratch, scratch_arena);
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

String platform_make_relative_to(String a, String b, Allocator allocator)
{
    if (!str_ends_with(a, str("/"))) {
        // TODO: free this allocation
        a = str_concat(a, str("/"), allocator);
    }

    String result = str_concat(a, b, allocator);

    return result;
}

b32 platform_write_to_file(String path, const void *data, ssize count, Allocator allocator)
{
    b32 result = false;

    LinearArena scratch = la_create(allocator, 1024);

    String parent_dir = platform_get_relative_parent_path(path);
    b32 parent_dirs_exist = true;

    if (parent_dir.data) {
        // Ensure that any parent directories in the path are created
        parent_dirs_exist = platform_create_directory(parent_dir, allocator);
    }

    if (parent_dirs_exist) {
        String nt_path = str_null_terminate(path, la_allocator(&scratch));

        int fd = open(nt_path.data, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);

        if (fd != -1) {
            ssize_t bytes_written = write(fd, data, ssize_to_usize(count));

            if (bytes_written == count) {
                result = true;
            }
        }

        close(fd);
    }

    la_destroy(&scratch);

    return result;
}

static b32 platform_create_directory_recursive(String path, Allocator allocator)
{
    b32 result = false;

    if (path.length > 0) {
        String parent = platform_get_relative_parent_path(path);
        platform_create_directory_recursive(parent, allocator);

        String nt_path = str_null_terminate(path, allocator);
        int mkdir_result = mkdir(nt_path.data, S_IRWXU);

        // Don't count as a failure if a part of the path already exists.
        // TODO: this will keep going even if mkdir failed because a FILE
        // rather than directory with this name existed. Fix that.
        if ((mkdir_result == -1) && (errno != EEXIST)) {
            result = false;
        } else {
            result = true;
        }
    }

    return result;
}

b32 platform_create_directory(String path, Allocator allocator)
{
    // TODO: validate that path is valid
    LinearArena scratch = la_create(allocator, KB(8));

    b32 result = platform_create_directory_recursive(path, allocator);

    la_destroy(&scratch);

    return result;
}
