#ifndef INPUT_H
#define INPUT_H

#include "base/matrix.h"
#include "base/utils.h"
#include "base/vector.h"

// clang-format off
typedef enum {
    KEYSTATE_UP       = FLAG(0),
    KEYSTATE_PRESSED  = FLAG(1),
    KEYSTATE_HELD     = FLAG(2),
    KEYSTATE_RELEASED = FLAG(3),
} Keystate;
// clang-format on

#define INPUT_KEY_LIST                                                                   \
    INPUT_KEY(KEY_W)                                                                     \
    INPUT_KEY(KEY_A)                                                                     \
    INPUT_KEY(KEY_S)                                                                     \
    INPUT_KEY(KEY_D)                                                                     \
    INPUT_KEY(KEY_K)                                                                     \
    INPUT_KEY(KEY_LEFT)                                                                  \
    INPUT_KEY(KEY_UP)                                                                    \
    INPUT_KEY(KEY_RIGHT)                                                                 \
    INPUT_KEY(KEY_DOWN)                                                                  \
    INPUT_KEY(KEY_ESCAPE)                                                                \
    INPUT_KEY(KEY_LEFT_SHIFT)                                                            \
    INPUT_KEY(KEY_G)                                                                     \
    INPUT_KEY(KEY_T)                                                                     \
    INPUT_KEY(KEY_I)                                                                     \
    INPUT_KEY(KEY_O)                                                                     \
    INPUT_KEY(KEY_L)                                                                     \
    INPUT_KEY(KEY_Y)                                                    \
    INPUT_KEY(KEY_LEFT_CONTROL)

#define INPUT_KEY(key) key,
typedef enum {
    INPUT_KEY_LIST

        MOUSE_LEFT,
    MOUSE_RIGHT,

    KEY_COUNT
} Key;
#undef INPUT_KEY

typedef struct PlatformInput {
    Keystate keystates[KEY_COUNT];
    Keystate previous_keystates[KEY_COUNT];
    f32 scroll_delta;
    Vector2 mouse_position;
    Vector2 mouse_click_position;
} PlatformInput;

#endif //INPUT_H
