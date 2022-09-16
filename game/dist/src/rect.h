#ifndef RECT_H
#define RECT_H

#include "vec2.h"

typedef struct rectangle{
    union{
        struct{
            f32 x;
            f32 y;
            f32 w;
            f32 h;
        };

        struct{
            vec2 position;
            vec2 size;
        };

        f32 data[4];
    };
}rectangle;

#endif
