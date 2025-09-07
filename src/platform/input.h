#ifndef INPUT_H
#define INPUT_H

#include "base/utils.h"

typedef enum {
    KEYSTATE_UP = 0,
    KEYSTATE_PRESSED,
    KEYSTATE_HELD,
    KEYSTATE_RELEASED,
} Keystate;

typedef enum {
    KEY_A,
    KEY_COUNT
} Key;

typedef struct {
    Keystate keystates[KEY_COUNT];
    Keystate previous_keystates[KEY_COUNT];
} Input;

// TODO: move all platform functions to same file
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
