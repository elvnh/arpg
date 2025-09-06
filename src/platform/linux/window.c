#include <GLFW/glfw3.h>

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
