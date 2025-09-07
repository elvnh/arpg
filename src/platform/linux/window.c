#include <GLFW/glfw3.h>
#include <string.h>

#include "platform/input.h"
#include "platform/window.h"

struct WindowHandle {
    GLFWwindow *window;
};

WindowHandle *os_create_window(s32 width, s32 height, const char *title, u32 window_flags, Allocator allocator)
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

void os_destroy_window(WindowHandle *handle)
{
    glfwDestroyWindow(handle->window);
    glfwTerminate();
}

bool os_window_should_close(WindowHandle *handle)
{
    return glfwWindowShouldClose(handle->window);
}

void os_poll_events(WindowHandle *window)
{
    glfwSwapBuffers(window->window);
    glfwPollEvents();
}

static s32 get_glfw_key_equivalent(Key key)
{
    switch (key) {
        case KEY_A: return GLFW_KEY_A;
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

void input_update(Input *input, WindowHandle *window)
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
