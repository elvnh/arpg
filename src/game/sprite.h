#ifndef SPRITE_H
#define SPRITE_H

#include "base/rectangle.h"

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
    SpriteRotationBehaviour rotation_behaviour;
} Sprite;

static inline Sprite sprite_create(TextureHandle texture, Vector2 size, SpriteRotationBehaviour rot_behaviour)
{
    Sprite result = (Sprite) {
	.texture = texture,
	.size = size,
	.rotation_behaviour = rot_behaviour
    };

    return result;
}

#endif //SPRITE_H
