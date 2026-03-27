#ifndef INPUT_EVENT_H
#define INPUT_EVENT_H

#include "input.h"

#include <string.h>

// TODO: pass in mouse pos in FrameInput

// TODO: mouse events?
typedef struct {
    Key key;
    Keystate keystate;
} InputEvent;

typedef struct {
    InputEvent data[8];
    s32 count;

    // TODO: should mouse position really be a part of event struct?
    // should probably be in FrameInput
    Vector2 mouse_position;
    Vector2 mouse_click_position;
    f32 scroll_delta;
} InputEvents;

// TODO: rename to FrameInput
typedef struct FrameInput {
    f32 dt;
    Vector2i window_size;

    InputEvents input_events;
} FrameInput;

static inline InputEvent *find_input_event(InputEvents *events, Key key, Keystate state_mask)
{
    InputEvent *result = 0;

    for (s32 i = 0; i < events->count; ++i) {
        InputEvent *event = &events->data[i];

        if ((event->key == key) && (has_flag(state_mask, event->keystate))) {
            result = event;
            break;
        }
    }

    return result;
}

static inline b32 check_keystate(InputEvents *events, Key key, Keystate state_mask)
{
    b32 result = find_input_event(events, key, state_mask) != 0;

    return result;
}

static inline b32 consume_keystate(InputEvents *events, Key key, Keystate state_mask)
{
    InputEvent *event = find_input_event(events, key, state_mask);
    b32 result = event != 0;

    if (event) {
        ssize remaining = events->count - (event - events->data) - 1;
        memmove(event, event + 1, (usize)remaining);

        --events->count;
    }

    return result;
}

static inline b32 check_key_pressed(InputEvents *events, Key key)
{
    b32 result = check_keystate(events, key, KEYSTATE_PRESSED);

    return result;
}

static inline b32 check_key_held(InputEvents *events, Key key)
{
    b32 result = check_keystate(events, key, KEYSTATE_HELD);

    return result;
}

static inline b32 check_key_released(InputEvents *events, Key key)
{
    b32 result = check_keystate(events, key, KEYSTATE_RELEASED);

    return result;
}

static inline b32 check_key_down(InputEvents *events, Key key)
{
    b32 result = check_keystate(events, key, KEYSTATE_PRESSED | KEYSTATE_HELD);

    return result;
}

static inline b32 consume_key_pressed(InputEvents *events, Key key)
{
    b32 result = consume_keystate(events, key, KEYSTATE_PRESSED);

    return result;
}

static inline b32 consume_key_held(InputEvents *events, Key key)
{
    b32 result = consume_keystate(events, key, KEYSTATE_HELD);

    return result;
}

static inline b32 consume_key_released(InputEvents *events, Key key)
{
    b32 result = consume_keystate(events, key, KEYSTATE_RELEASED);

    return result;
}

static inline b32 consume_key_down(InputEvents *events, Key key)
{
    b32 result = consume_keystate(events, key, KEYSTATE_PRESSED | KEYSTATE_HELD);

    return result;
}

static inline Vector2 get_mouse_pos(InputEvents *events)
{
    Vector2 result = events->mouse_position;
    return result;
}

static inline Vector2 get_mouse_click_pos(InputEvents *events)
{
    Vector2 result = events->mouse_click_position;
    return result;
}

static inline f32 consume_scroll_delta(InputEvents *events)
{
    f32 result = events->scroll_delta;
    events->scroll_delta = 0.0f;

    return result;
}

#endif // INPUT_EVENT_H
