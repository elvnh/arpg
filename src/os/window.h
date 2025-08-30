#ifndef WINDOW_H
#define WINDOW_H

#include "base/allocator.h"

enum {
    WINDOW_FLAG_NON_RESIZABLE = (1 << 0),
    //WINDOW_FLAG_FULL_SCREEN,
};

typedef struct WindowHandle WindowHandle;

WindowHandle  *os_create_window(s32 width, s32 height, const char *title, u32 window_flags, Allocator allocator);
void           os_destroy_window(WindowHandle *handle);
bool           os_window_should_close(WindowHandle *handle);
void           os_poll_events(WindowHandle *window);

#endif //WINDOW_H
