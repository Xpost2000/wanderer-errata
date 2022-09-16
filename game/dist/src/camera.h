#ifndef CAMERA_STATE_H
#define CAMERA_STATE_H

#include "common.h"

#include "vec2.h"

typedef struct camera_state{
    f32 x;
    f32 y;
    f32 scale;
}camera_state;

typedef struct camera_transition{
    camera_state from;
    camera_state to;

    f32 time;
    b32 started;
}camera_transition;

void camera_transition_move_to( camera_transition* transition, vec2 from, vec2 to );

#endif
