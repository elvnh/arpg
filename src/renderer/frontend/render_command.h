#ifndef RENDER_COMMAND_H
#define RENDER_COMMAND_H

#include "base/polygon.h"
#include "base/utils.h"
#include "base/rectangle.h"
#include "base/string8.h"
#include "base/triangle.h"
#include "platform/asset.h"

// TODO: X macro to avoid defining new commands in multiple places

struct ParticleBufferNew;

#define RENDER_COMMAND_LIST                     \
    RENDER_COMMAND(RectangleCmd)                \
    RENDER_COMMAND(ClippedRectangleCmd)         \
    RENDER_COMMAND(OutlinedRectangleCmd)        \
    RENDER_COMMAND(CircleCmd)                   \
    RENDER_COMMAND(TriangleCmd)                 \
    RENDER_COMMAND(OutlinedTriangleCmd)         \
    RENDER_COMMAND(LineCmd)                     \
    RENDER_COMMAND(TextCmd)                     \
    RENDER_COMMAND(ParticleGroupCmd)            \
    RENDER_COMMAND(PolygonCmd)                  \
    RENDER_COMMAND(TriangleFanCmd)              \

#define RENDER_SETUP_COMMAND_LIST               \
    RENDER_SETUP_COMMAND(SetupCmdUniformVec4)   \
    RENDER_SETUP_COMMAND(SetupCmdUniformFloat)  \

#define RENDER_COMMAND_ENUM_NAME(type) RENDER_CMD_##type
#define RENDER_SETUP_COMMAND_ENUM_NAME(type) RENDER_SETUP_CMD_##type

typedef enum {
#define RENDER_COMMAND(type) RENDER_COMMAND_ENUM_NAME(type),
    RENDER_COMMAND_LIST
#undef RENDER_COMMAND
} RenderCmdKind;

typedef enum {
#define RENDER_SETUP_COMMAND(name) RENDER_SETUP_COMMAND_ENUM_NAME(name),
    RENDER_SETUP_COMMAND_LIST
#undef RENDER_SETUP_COMMAND
} SetupCmdKind;

typedef struct SetupCmdHeader {
    SetupCmdKind kind;
    struct SetupCmdHeader *next;
    String uniform_name;
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
    f32 rotation_in_radians;
} RectangleCmd;

typedef struct {
    RenderCmdHeader header;
    Triangle triangle;
    RGBA32 color;
} TriangleCmd;

typedef struct {
    RenderCmdHeader header;
    Triangle triangle;
    RGBA32 color;
    f32 thickness;
} OutlinedTriangleCmd;

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
} ParticleGroupCmd;

typedef struct {
    RenderCmdHeader header;
    TriangulatedPolygon polygon;
    RGBA32 color;
} PolygonCmd;

typedef struct {
    RenderCmdHeader header;
    TriangleFan triangle_fan;
    RGBA32 color;
} TriangleFanCmd;

/* Setup command types */
typedef struct {
    SetupCmdHeader header;
    Vector4 value;
} SetupCmdUniformVec4;

typedef struct {
    SetupCmdHeader header;
    f32 value;
} SetupCmdUniformFloat;

#endif //RENDER_COMMAND_H
