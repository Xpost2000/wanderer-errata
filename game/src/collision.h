#ifndef COLLISION_H
#define COLLISION_H

#include "common.h"

#include "rect.h"
#include "vec2.h"

// This is not a vector.
// although I will typedef it anyways.
typedef struct vec2 point;

typedef struct circle{
    f32 x;
    f32 y;

    f32 radius;
}circle;

b32 circle_intersects( circle a, circle b );
b32 rectangle_intersects( rectangle a, rectangle b );

b32 point_intersects_rectangle( point a, rectangle b );
b32 point_intersects_circle( point a, circle b );

#endif
