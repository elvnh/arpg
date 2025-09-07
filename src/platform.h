#ifndef PLATFORM_H
#define PLATFORM_H

#include "base/linear_arena.h"
#include "base/string8.h"
#include "base/span.h"
#include "base/timestamp.h"

/* Window */
typedef struct WindowHandle WindowHandle;

enum {
    WINDOW_FLAG_NON_RESIZABLE = (1 << 0),
};

WindowHandle  *platform_create_window(s32 width, s32 height, const char *title, u32 window_flags, Allocator allocator);
void           platform_destroy_window(WindowHandle *handle);
bool           platform_window_should_close(WindowHandle *handle);
void           platform_poll_events(WindowHandle *window);

/* Input */
struct Input;

void           platform_update_input(struct Input *input, struct WindowHandle *window);

/* Path */
String         platform_get_executable_path(Allocator allocator);
String         platform_get_executable_directory(Allocator allocator, LinearArena *scratch_arena);
bool           platform_path_is_absolute(String path);
String         platform_get_absolute_path(String path, Allocator allocator, LinearArena *scratch_arena);
String         platform_get_canonical_path(String path, Allocator allocator, LinearArena *scratch_arena);
String         platform_get_working_directory(Allocator allocator);
void           platform_change_working_directory(String path);
String         platform_get_parent_path(String path, Allocator allocator, LinearArena *scratch_arena);

/* File */
typedef enum {
    FILE_TYPE_FILE,
    FILE_TYPE_DIRECTORY,
    FILE_TYPE_OTHER,
} FileType;

typedef struct {
    FileType type;
    ssize file_size;
    Timestamp last_modification_time;
} FileInfo;

Span           platform_read_entire_file(String path, Allocator allocator, LinearArena *scratch);
String         platform_read_entire_file_as_string(String path, Allocator allocator, LinearArena *scratch);
ssize          platform_get_file_size(String path, LinearArena *scratch);
b32            platform_file_exists(String path, LinearArena *scratch);
FileInfo       platform_get_file_info(String path, LinearArena *scratch);
void           platform_for_each_file_in_dir(String directory, void (*callback)(String), LinearArena *scratch);

/* Time */
Timestamp      platform_get_time();


#endif //PLATFORM_H
