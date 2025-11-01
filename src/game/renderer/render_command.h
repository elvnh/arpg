#ifndef RENDER_COMMAND_H
#define RENDER_COMMAND_H

#include "base/utils.h"
#include "base/rectangle.h"
#include "base/string8.h"
#include "asset.h"

// TODO: X macro to avoid defining new commands in multiple places

#define RENDER_COMMAND_LIST                     \
    RENDER_COMMAND(RectangleCmd)                \
    RENDER_COMMAND(ClippedRectangleCmd)         \
    RENDER_COMMAND(OutlinedRectangleCmd)        \
    RENDER_COMMAND(CircleCmd)                   \
    RENDER_COMMAND(LineCmd)                     \
    RENDER_COMMAND(TextCmd)                     \
    RENDER_COMMAND(ParticleGroupCmd)            \

#define RENDER_COMMAND_ENUM_NAME(type) RENDER_CMD_##type

typedef enum {
#define RENDER_COMMAND(type) RENDER_COMMAND_ENUM_NAME(type),
    RENDER_COMMAND_LIST
#undef RENDER_COMMAND
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
    RGBA32 color;
    RectangleFlip flip;
} RectangleCmd;

typedef struct {
    RenderCmdHeader header;
    Rectangle rect;
    Rectangle viewport_rect;
    RGBA32 color;
} ClippedRectangleCmd;

typedef struct {
    RenderCmdHeader header;
    Rectangle rect;
    RGBA32 color;
    f32 thickness;
} OutlinedRectangleCmd;

typedef struct {
    RenderCmdHeader header;
    Vector2 position;
    RGBA32 color;
    f32 radius;
} CircleCmd;

typedef struct {
    RenderCmdHeader header;
    Vector2 start;
    Vector2 end;
    RGBA32 color;
    f32 thickness;
} LineCmd;

typedef struct {
    RenderCmdHeader header;
    String text;
    Vector2 position;
    RGBA32 color;
    s32 size;
    FontHandle font;
    b32 is_clipped; // TODO: should clipped text be a different command?
    Rectangle clip_rect;
} TextCmd;

typedef struct {
    RenderCmdHeader header;
    struct ParticleBuffer *particles;
    f32 particle_size;
    RGBA32 color;
} ParticleGroupCmd;

/* Setup command types */
typedef struct {
    SetupCmdHeader header;
    String uniform_name;
    Vector4 vector;
} SetupCmdUniformVec4;

#endif //RENDER_COMMAND_H
