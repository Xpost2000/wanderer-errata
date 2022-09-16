#include "vec2.h"

#include <math.h>

vec2 v2( f32 x, f32 y ){
    vec2 result = {x, y};

    return result;
}   

vec2 add_vec2( vec2 a, vec2 b ){
    vec2 result = {
        a.x + b.x,
        a.y + b.y
    };

    return result;
}

vec2 add_vec2_scalar( vec2 a, f32 b ){
    vec2 result = {
        a.x + b,
        a.y + b
    };

    return result;
}

vec2 sub_vec2( vec2 a, vec2 b ){
    vec2 result = {
        a.x - b.x,
        a.y - b.y
    };

    return result;
}

vec2 sub_vec2_scalar( vec2 a, f32 b ){
    vec2 result = {
        a.x - b,
        a.y - b
    };

    return result;
}

f32 dot_vec2( vec2 a, vec2 b ){
    f32 result;

    result = ((a.x * b.x) + (a.y * b.y));

    return result;
}

vec2 mul_vec2_scalar( vec2 a, f32 b ){
    vec2 result = {
        a.x * b,
        a.y * b
    };

    return result;
}

f32 vec2_magnitude( vec2 a ){
    f32 result;

    f32 x_distance = a.x * a.x;
    f32 y_distance = a.y * a.y;

    result = sqrtf( x_distance + y_distance );

    return result;
}

vec2 vec2_normalize( vec2 a ){
    vec2 result;

    f32 mag = vec2_magnitude( a );

    result.x = a.x / mag;
    result.y = a.y / mag;

    return result;
}

vec2 vec2_perpendicular( vec2 a ){
    vec2 result;

    result.x = a.y;
    result.y = a.x;

    return result;
}

f32 vec2_distance( vec2 a, vec2 b ){
    f32 x_distance = (b.x - a.x);
    f32 y_distance = (b.y - a.y);

    x_distance *= x_distance;
    y_distance *= y_distance;

    return sqrtf( x_distance + y_distance );
}
