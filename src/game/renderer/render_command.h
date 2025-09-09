#ifndef RENDER_COMMAND_H
#define RENDER_COMMAND_H

#include "base/utils.h"
#include "base/rectangle.h"
#include "base/string8.h"

typedef enum {
    RENDER_CMD_SPRITE,
    RENDER_CMD_RECTANGLE,
    RENDER_CMD_CIRCLE,
} RenderCmdKind;

typedef enum {
    SETUP_CMD_SET_UNIFORM_FLOAT,
    SETUP_CMD_SET_UNIFORM_VEC4,
} SetupCmdKind;

typedef struct SetupCmdHeader {
    SetupCmdKind kind;
    struct SetupCmdHeader *next;
} SetupCmdHeader;

typedef struct {
    RenderCmdKind kind;
    SetupCmdHeader *first_setup_command;
} RenderCmdHeader;

/* Render command types */
typedef struct {
    RenderCmdHeader header;
    Rectangle rect;
} SpriteCmd;

typedef struct {
    RenderCmdHeader header;
    Rectangle rect;
    RGBA32 color;
} RectangleCmd;

typedef struct {
    RenderCmdHeader header;
    Vector2 position;
    RGBA32 color;
    f32 radius;
} CircleCmd;

/* Setup command types */
typedef struct {
    SetupCmdHeader header;
    String uniform_name;
    Vector4 vector;
} SetupCmdUniformVec4;

#endif //RENDER_COMMAND_H
