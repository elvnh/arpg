#ifndef SPRITE_H
#define SPRITE_H

#include "base/rectangle.h"
#include "platform/asset.h"

typedef enum {
    SPRITE_ROTATE_NONE,
    SPRITE_ROTATE_BASED_ON_DIR,
    SPRITE_MIRROR_HORIZONTALLY_BASED_ON_DIR,
} SpriteRotationBehaviour;

// TODO: make push_sprite take this as parameter
typedef struct {
    float rotation;
    RectangleFlip flip;
} SpriteModifiers;

typedef struct {
    TextureHandle texture;
    Vector2 size;
    RGBA32 color;
    SpriteRotationBehaviour rotation_behaviour;
} Sprite;

static inline Sprite sprite_create(TextureHandle texture, Vector2 size, SpriteRotationBehaviour rot_behaviour)
{
    Sprite result = (Sprite) {
	.texture = texture,
	.size = size,
	.rotation_behaviour = rot_behaviour,
	.color = RGBA32_WHITE
    };

    return result;
}

static inline Sprite sprite_create_colored(TextureHandle texture, Vector2 size,
    SpriteRotationBehaviour rot_behaviour, RGBA32 color)
{
    Sprite result = sprite_create(texture, size, rot_behaviour);
    result.color = color;

    return result;
}


static inline SpriteModifiers
sprite_get_modifiers(Vector2 direction, SpriteRotationBehaviour rotation_behaviour)
{
    f32 rotation = 0.0f;
    f32 dir_angle = (f32)atan2(direction.y, direction.x);

    RectangleFlip flip = RECT_FLIP_NONE;

    if (rotation_behaviour == SPRITE_ROTATE_BASED_ON_DIR) {
	rotation = dir_angle;
    } else if (rotation_behaviour == SPRITE_MIRROR_HORIZONTALLY_BASED_ON_DIR) {
	b32 should_flip = (dir_angle > PI_2) || (dir_angle < -PI_2);

	if (should_flip) {
	    flip = RECT_FLIP_HORIZONTALLY;
	}
    }

    SpriteModifiers result = {
	.flip = flip,
	.rotation = rotation
    };

    return result;
}


#endif //SPRITE_H
