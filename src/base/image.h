#ifndef IMAGE_H
#define IMAGE_H

#include "typedefs.h"

typedef struct {
    byte  *data;
    s32    width;
    s32    height;
    s32    channels;
} Image;

#endif //IMAGE_H
