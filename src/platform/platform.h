#ifndef PLATFORM_H
#define PLATFORM_H

#include "asset.h"
#include "base/linear_arena.h"
#include "base/string8.h"
#include "base/vector.h"
#include "base/span.h"
#include "base/timestamp.h"

/* Window */
typedef struct WindowHandle WindowHandle;

typedef Vector2 (PlatformGetTextDimensions)(FontHandle, String, s32);
typedef f32     (PlatformGetTextNewlineAdvance)(FontHandle, s32);
typedef f32     (PlatformGetFontBaselineOffset)(FontHandle, s32);

typedef struct {
    PlatformGetTextDimensions *get_text_dimensions;
    PlatformGetTextNewlineAdvance *get_text_newline_advance; // TODO: not needed?
    PlatformGetFontBaselineOffset *get_font_baseline_offset;
} PlatformCode;

enum {
    WINDOW_FLAG_NON_RESIZABLE = FLAG(0),
};

WindowHandle  *platform_create_window(s32 width, s32 height, const char *title, u32 window_flags, Allocator allocator);
void           platform_destroy_window(WindowHandle *handle);
bool           platform_window_should_close(WindowHandle *handle);
void           platform_poll_events(WindowHandle *window);
Vector2i       platform_get_window_size(WindowHandle *window);

/* Input */
struct Input;

void           platform_update_input(struct Input *input, struct WindowHandle *window);
void           platform_initialize_input(struct Input *input, struct WindowHandle *window);

/* Path */
String         platform_get_executable_path(Allocator allocator);
String         platform_get_executable_directory(Allocator allocator, LinearArena *scratch_arena);
bool           platform_path_is_absolute(String path);
String         platform_get_absolute_path(String path, Allocator allocator, LinearArena *scratch_arena);
String         platform_get_canonical_path(String path, Allocator allocator, LinearArena *scratch_arena);
String         platform_get_working_directory(Allocator allocator);
void           platform_change_working_directory(String path);
String         platform_get_parent_path(String path, Allocator allocator, LinearArena *scratch_arena);
String         platform_get_filename(String path);

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
Timestamp      platform_get_time(void);
f32            platform_get_seconds_since_launch(void);

/* Mutex */
typedef struct {
    void *handle;
} Mutex;

Mutex mutex_create(Allocator allocator);
void  mutex_destroy(Mutex mutex, Allocator allocator);
void  mutex_lock(Mutex mutex);
void  mutex_release(Mutex mutex);

/* Atomic */
static inline s32 atomic_load_s32(s32 *ptr)
{
    ASSERT(ptr);
#if __GNUC__
    s32 result = __atomic_load_n(ptr, __ATOMIC_RELAXED);
    return result;
#else
#  error
#endif
}

static inline void atomic_store_s32(s32 *ptr, s32 new_value)
{
    ASSERT(ptr);
#if __GNUC__
    __atomic_store_n(ptr, new_value, __ATOMIC_RELAXED);
#else
#  error
#endif
}

/* Misc */
void platform_trap_on_fp_exceptions(void);

#endif //PLATFORM_H
