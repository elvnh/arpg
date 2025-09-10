#define _DEFAULT_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <GLFW/glfw3.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>

#include "base/string8.h"

#include "platform.h"
#include "input.h"

/* Window */
struct WindowHandle {
    GLFWwindow *window;
};

WindowHandle *platform_create_window(s32 width, s32 height, const char *title, u32 window_flags, Allocator allocator)
{
    if (!glfwInit()) {
        return 0;
    }

    if (window_flags & WINDOW_FLAG_NON_RESIZABLE) {
        glfwWindowHint(GLFW_RESIZABLE, false);
    }

    GLFWwindow *window = glfwCreateWindow(width, height, title, 0, 0);

    if (!window) {
        glfwTerminate();
        return 0;
    }

    glfwMakeContextCurrent(window);

    WindowHandle *handle = allocate_item(allocator, WindowHandle);
    handle->window = window;

    return handle;
}

void platform_destroy_window(WindowHandle *handle)
{
    glfwDestroyWindow(handle->window);
    glfwTerminate();
}

bool platform_window_should_close(WindowHandle *handle)
{
    return glfwWindowShouldClose(handle->window);
}

void platform_poll_events(WindowHandle *window)
{
    glfwSwapBuffers(window->window);
    glfwPollEvents();
}

/* Input */
static s32 get_glfw_key_equivalent(Key key)
{
    switch (key) {
#define INPUT_KEY(key) case key: return GLFW_##key;
        INPUT_KEY_LIST
#undef INPUT_KEY
        INVALID_DEFAULT_CASE;
    }

    ASSERT(false);

    return 0;
}

static Keystate get_current_keystate(Key key, WindowHandle *window)
{
    s32 glfw_key = get_glfw_key_equivalent(key);
    s32 state = glfwGetKey(window->window, glfw_key);

    if (state == GLFW_PRESS) {
        return KEYSTATE_PRESSED;
    }

    return KEYSTATE_UP;
}

void platform_update_input(Input *input, WindowHandle *window)
{
    memcpy(input->previous_keystates, input->keystates, KEY_COUNT * sizeof(*input->keystates));

    for (Key key = 0; key < KEY_COUNT; ++key) {
        Keystate current = get_current_keystate(key, window);
        Keystate previous = input->previous_keystates[key];
        Keystate result = current;

        if ((current == KEYSTATE_PRESSED) && ((previous == KEYSTATE_PRESSED) || (previous == KEYSTATE_HELD))) {
            result = KEYSTATE_HELD;
        } else if ((current == KEYSTATE_UP) && ((previous == KEYSTATE_PRESSED) || (previous == KEYSTATE_HELD))) {
            result = KEYSTATE_RELEASED;
        }

        input->keystates[key] = result;
    }
}

/* Paths */
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
    ssize bytes_written = readlink("/proc/self/exe", result.data, cast_ssize_to_usize(result.length));

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
    String directory_path = platform_get_parent_path(executable_path, allocator, scratch_arena);

    return directory_path;
}

bool platform_path_is_absolute(String path)
{
    return str_starts_with(path, str_lit("/"));
}

String platform_get_absolute_path(String path, Allocator allocator, LinearArena *scratch_arena)
{
    if (platform_path_is_absolute(path)) {
        return path;
    }

    Allocator scratch = la_allocator(scratch_arena);

    String working_dir = str_concat(platform_get_working_directory(scratch), str_lit("/"), scratch);
    String result = str_concat(working_dir, path, allocator);

    return result;
}

String platform_get_canonical_path(String path, Allocator allocator, LinearArena *scratch_arena)
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

String platform_get_working_directory(Allocator allocator)
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
    ssize last_slash_pos = str_find_last_occurence(absolute, str_lit("/"));
    ASSERT(last_slash_pos != -1);

    String result = {
        .data = absolute.data,
        .length = MAX(1, last_slash_pos) // In case path is only a /
    };

    return result;
}

String platform_get_filename(String path)
{
    ssize slash_index = str_find_last_occurence(path, str_lit("/"));

    if (slash_index == -1) {
        return path;
    }

    String result = str_create_span(path, slash_index + 1, path.length - slash_index - 1);

    return result;
}

/* File */
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

    ssize bytes_read = read(fd, file_data, cast_ssize_to_usize(file_size));
    ASSERT(bytes_read == file_size);

    // TODO: is null terminating really necessary?
    file_data[file_size] = '\0';

    result.data = file_data;
    result.size = file_size;

  done:
    close(fd);

    return result;
}

String platform_read_entire_file_as_string(String path, Allocator allocator, LinearArena *scratch)
{
    Span file_contents = platform_read_entire_file(path, allocator, scratch);
    String result = { (char *)file_contents.data, file_contents.size };

    return result;
}

ssize platform_get_file_size(String path, LinearArena *scratch)
{
    FileInfo info = platform_get_file_info(path, scratch);

    return info.file_size;
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

    FileInfo result = {-1, -1, {0}};

    if ((stat_result == -1) || !S_ISREG(st.st_mode)) {
        return result;
    }

    Timestamp mod_time = {
        .seconds = st.st_mtim.tv_sec,
        .nanoseconds = st.st_mtim.tv_nsec
    };

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

void platform_for_each_file_in_dir(String directory, void (*callback)(String), LinearArena *scratch)
{
    String null_terminated = str_null_terminate(directory, la_allocator(scratch));
    DIR *dir = opendir(null_terminated.data);
    ASSERT(dir);

    for (struct dirent *entry = readdir(dir); entry; entry = readdir(dir)) {
        String name = { entry->d_name, (ssize)strlen(entry->d_name) };
        String full_path = str_concat(str_concat(directory, str_lit("/"), la_allocator(scratch)),
            name, la_allocator(scratch));

        if (entry->d_type == DT_DIR) {
            if (!str_equal(name, str_lit(".")) && !str_equal(name, str_lit(".."))) {
                platform_for_each_file_in_dir(full_path, callback, scratch);
            }
        } else {
            callback(full_path);
        }
    }

    closedir(dir);
}

/* Time */
Timestamp platform_get_time()
{
    struct timespec ts;
    s32 gettime_result = clock_gettime(CLOCK_REALTIME, &ts);
    ASSERT(gettime_result == 0);

    Timestamp result = {
        .seconds = ts.tv_sec,
        .nanoseconds = ts.tv_nsec
    };

    return result;
}

/* Mutex */
Mutex mutex_create(Allocator allocator)
{
    void *handle = allocate_item(allocator, pthread_mutex_t);
    pthread_mutex_init(handle, 0);

    Mutex result = {handle};

    return result;
}

void mutex_destroy(Mutex mutex, Allocator allocator)
{
    ASSERT(mutex.handle);
    pthread_mutex_destroy(mutex.handle);
    deallocate(allocator, mutex.handle);
}

void mutex_lock(Mutex mutex)
{
    pthread_mutex_lock(mutex.handle);
}

void mutex_release(Mutex mutex)
{
    pthread_mutex_unlock(mutex.handle);
}
