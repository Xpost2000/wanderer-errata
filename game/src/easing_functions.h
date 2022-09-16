#ifndef EASING_FUNCTIONS_H
#define EASING_FUNCTIONS_H

#include "common.h"

/*in the format of 
 * easing / interpolation functions.
 *
 * t = current_time,
 * b = start,
 * c = change in value,
 * d = duration
 *
 * TODO(jerry): 
 *      ease_func( start, end, time );
 *      With duration = 1;
 * */

/*classical lerp*/
f32 lerp( f32 a, f32 b, f32 t );
f32 linear_ease( f32 b, f32 c, f32 d, f32 t );
f32 cubic_ease_in( f32 b, f32 c, f32 d, f32 t );
f32 cubic_ease_out( f32 b, f32 c, f32 d, f32 t );
f32 quadratic_ease_in( f32 b, f32 c, f32 d, f32 t );
f32 quadratic_ease_out( f32 b, f32 c, f32 d, f32 t );

#endif
