#include "collision.h"

b32 circle_intersects( circle a, circle b ){
    f32 center_distance = ( a.x - b.x ) * ( a.x - b.x ) +
                          ( a.y - b.y ) * ( a.y - b.y );

    center_distance = sqrtf( center_distance );

    return ( center_distance < a.radius + b.radius );
}

b32 rectangle_intersects( rectangle a, rectangle b ){
    b32 x_intersect = (a.x < b.x + b.w) && (a.x + a.w > b.x);
    b32 y_intersect = (a.y < b.y + b.h) && (a.y + a.h > b.y);

    return x_intersect && y_intersect;
}

b32 point_intersects_rectangle( point a, rectangle b ){
    b32 x_intersect = (a.x < b.x + b.w) && (a.x > b.x);
    b32 y_intersect = (a.y < b.y + b.h) && (a.y > b.y);

    return x_intersect && y_intersect;
}

b32 point_intersects_circle( point a, circle b ){
    f32 center_distance = ( a.x - b.x ) * ( a.x - b.x ) +
                          ( a.y - b.y ) * ( a.y - b.y );

    center_distance = sqrtf( center_distance );

    return ( center_distance < b.radius );
}
