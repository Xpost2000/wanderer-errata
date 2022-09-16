#ifndef VEC2_H
#define VEC2_H

#include "common.h"

typedef struct vec2{
    union{
        struct{
            f32 x;
            f32 y;
        };

        struct{
            f32 w;
            f32 h;
        };

        struct{
            f32 u;
            f32 v;
        };
        f32 data[2];
    };
}vec2;

vec2 v2( f32 x, f32 y );

vec2 add_vec2( vec2 a, vec2 b );
vec2 add_vec2_scalar( vec2 a, f32 b );

vec2 sub_vec2( vec2 a, vec2 b );
vec2 sub_vec2_scalar( vec2 a, f32 b );

f32 dot_vec2( vec2 a, vec2 b );
vec2 mul_vec2_scalar( vec2 a, f32 b );

f32 vec2_magnitude( vec2 a );

f32 vec2_distance( vec2 a, vec2 b );

vec2 vec2_normalize( vec2 a );

vec2 vec2_perpendicular( vec2 a );

#endif
