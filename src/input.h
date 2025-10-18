#ifndef INPUT_H
#define INPUT_H

#include "base/utils.h"
#include "base/vector.h"

typedef enum {
    KEYSTATE_UP = 0,
    KEYSTATE_PRESSED,
    KEYSTATE_HELD,
    KEYSTATE_RELEASED,
} Keystate;

#define INPUT_KEY_LIST                          \
    INPUT_KEY(KEY_W)                            \
    INPUT_KEY(KEY_A)                            \
    INPUT_KEY(KEY_S)                            \
    INPUT_KEY(KEY_D)                            \
    INPUT_KEY(KEY_LEFT)                         \
    INPUT_KEY(KEY_UP)                           \
    INPUT_KEY(KEY_RIGHT)                        \
    INPUT_KEY(KEY_DOWN)                         \
    INPUT_KEY(KEY_ESCAPE)                       \
    INPUT_KEY(KEY_LEFT_SHIFT)                   \
    INPUT_KEY(KEY_G)                            \
    INPUT_KEY(KEY_T)                            \

#define INPUT_KEY(key) key,
typedef enum {
    INPUT_KEY_LIST

    MOUSE_LEFT,
    MOUSE_RIGHT,

    KEY_COUNT
} Key;
#undef INPUT_KEY

typedef struct Input {
    Keystate keystates[KEY_COUNT];
    Keystate previous_keystates[KEY_COUNT];
    f32 scroll_delta;
    Vector2 mouse_position;
    Vector2 mouse_click_position;
} Input;

typedef struct {
    f32 dt;
    const Input *input;
    Vector2i window_size;
} FrameData;

static inline void input_initialize(Input *input)
{
    input->mouse_click_position = v2(-1.0f, -1.0f);
}

static inline Keystate input_get_key(const Input *input, Key key)
{
    ASSERT(key >= 0);
    ASSERT(key < KEY_COUNT);

    Keystate result = input->keystates[key];
    return result;
}

static inline b32 input_is_key_pressed(const Input *input, Key key)
{
    Keystate keystate = input_get_key(input, key);
    b32 result = keystate == KEYSTATE_PRESSED;

    return result;
}

static inline b32 input_is_key_held(const Input *input, Key key)
{
    Keystate keystate = input_get_key(input, key);
    b32 result = keystate == KEYSTATE_HELD;

    return result;
}

static inline b32 input_is_key_released(const Input *input, Key key)
{
    Keystate keystate = input_get_key(input, key);
    b32 result = keystate == KEYSTATE_RELEASED;

    return result;
}

static inline b32 input_is_key_down(const Input *input, Key key)
{
    Keystate keystate = input_get_key(input, key);
    b32 result = (keystate == KEYSTATE_PRESSED) || (keystate == KEYSTATE_HELD);

    return result;
}

#endif //INPUT_H
