#define _GNU_SOURCE
#define _DEFAULT_SOURCE

#include "base/string8.h"
#include "input.h"
#include "platform.h"
#include "platform/input_event.h"

#include <GLFW/glfw3.h>
#include <dirent.h>
#include <fcntl.h>
#include <fenv.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* Window */
struct WindowHandle {
    GLFWwindow *window;
};

static void framebuffer_size_callback(GLFWwindow *window, s32 width, s32 height)
{
    (void)window;
    glViewport(0, 0, width, height);
}

WindowHandle *platform_create_window(s32 width, s32 height, const char *title,
    u32 window_flags, Allocator allocator)
{
    if (!glfwInit()) {
        return 0;
    }

    if (window_flags & WINDOW_FLAG_NON_RESIZABLE) {
        glfwWindowHint(GLFW_RESIZABLE, false);
    }

    GLFWwindow *window = glfwCreateWindow(width, height, title, 0, 0);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

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

Vector2i platform_get_window_size(WindowHandle *window)
{
    Vector2i result = {0};
    glfwGetWindowSize(window->window, &result.x, &result.y);

    return result;
}

/* Input */
static s32 get_glfw_key_equivalent(Key key)
{
    BEGIN_EXHAUSTIVE_SWITCH;
    switch (key) {
#define INPUT_KEY(key)                                                                   \
    case key:                                                                            \
        return GLFW_##key;
        INPUT_KEY_LIST
#undef INPUT_KEY

        case MOUSE_LEFT:
            return GLFW_MOUSE_BUTTON_LEFT;
        case MOUSE_RIGHT:
            return GLFW_MOUSE_BUTTON_RIGHT;

            INVALID_CASE(KEY_COUNT);
            INVALID_DEFAULT_CASE;
    }
    END_EXHAUSTIVE_SWITCH;

    ASSERT(false);

    return 0;
}

static Keystate get_current_keystate(Key key, WindowHandle *window)
{
    s32 glfw_key = get_glfw_key_equivalent(key);
    s32 state = 0;

    if ((key == MOUSE_LEFT) || (key == MOUSE_RIGHT)) {
        state = glfwGetMouseButton(window->window, glfw_key);
    } else {
        state = glfwGetKey(window->window, glfw_key);
    }

    if (state == GLFW_PRESS) {
        return KEYSTATE_PRESSED;
    }

    return KEYSTATE_UP;
}

static f32 g_scroll_delta;

static void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    (void)window;
    (void)xoffset;
    g_scroll_delta = (f32)yoffset;
}

static void platform_update_input(PlatformInput *input, WindowHandle *window)
{
    memcpy(input->previous_keystates, input->keystates, KEY_COUNT * sizeof(*input->keystates));

    input->scroll_delta = g_scroll_delta;
    g_scroll_delta = 0.0f;

    double x, y;
    glfwGetCursorPos(window->window, &x, &y);

    input->mouse_position = v2((f32)x, (f32)y);

    for (Key key = 0; key < KEY_COUNT; ++key) {
        Keystate current = get_current_keystate(key, window);
        Keystate previous = input->previous_keystates[key];
        Keystate result = current;

        if ((current == KEYSTATE_PRESSED)
            && ((previous == KEYSTATE_PRESSED) || (previous == KEYSTATE_HELD))) {
            result = KEYSTATE_HELD;
        } else if ((current == KEYSTATE_UP)
                   && ((previous == KEYSTATE_PRESSED) || (previous == KEYSTATE_HELD))) {
            result = KEYSTATE_RELEASED;
        }

        if ((key == MOUSE_LEFT) && (result == KEYSTATE_PRESSED)) {
            input->mouse_click_position = input->mouse_position;
        }

        input->keystates[key] = result;
    }
}

InputEvents platform_poll_input_events(PlatformInput *input, WindowHandle *window)
{
    InputEvents result = {0};

    platform_update_input(input, window);
    result.scroll_delta = input->scroll_delta;
    result.mouse_position = input->mouse_position;
    result.mouse_click_position = input->mouse_click_position;

    for (Key key = 0; key < KEY_COUNT; ++key) {
        Keystate state = input->keystates[key];

        if (state != KEYSTATE_UP) {
            ASSERT(result.count < ARRAY_COUNT(result.data));

            InputEvent event = {0};
            event.key = key;
            event.keystate = state;
            result.data[result.count++] = event;
        }
    }

    return result;
}

void platform_initialize_input(PlatformInput *input, struct WindowHandle *window)
{
    input->mouse_click_position = v2(-1.0f, -1.0f);
    glfwSetScrollCallback(window->window, scroll_callback);
}

/* Time */
Timestamp platform_get_time(void)
{
    struct timespec ts;
    s32 gettime_result = clock_gettime(CLOCK_REALTIME, &ts);
    ASSERT(gettime_result == 0);

    Timestamp result = {.seconds = ts.tv_sec, .nanoseconds = ts.tv_nsec};

    return result;
}

f32 platform_get_seconds_since_launch(void)
{
    f32 result = (f32)glfwGetTime();

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

/* Misc */
void platform_trap_on_fp_exceptions(void)
{
    feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
}
