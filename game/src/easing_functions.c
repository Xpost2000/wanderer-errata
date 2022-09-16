#include "easing_functions.h"

f32 lerp( f32 a, f32 b, f32 t ){
    return (1 - t) * a + t * b;
}

f32 linear_ease( f32 b, f32 c, f32 d, f32 t ){
    return c * t / d + b;
}

f32 cubic_ease_in( f32 b, f32 c, f32 d, f32 t ){
    t /= d;

    return c * t * t * t + b;
}

f32 cubic_ease_out( f32 b, f32 c, f32 d, f32 t ){
    t /= d;

    t--;

    return c * ( t * t * t + 1 ) + b;
}

f32 quadratic_ease_in( f32 b, f32 c, f32 d, f32 t ){
    t /= d;

    return c * t * t + b;
}

f32 quadratic_ease_out( f32 b, f32 c, f32 d, f32 t ){
    t /= d;

    return (-c) * t * (t - 2) + b;
}
