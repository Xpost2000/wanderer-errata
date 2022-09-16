#include "camera.h"

void camera_transition_move_to( camera_transition* transition, vec2 from, vec2 to ){
    transition->from.x = from.x;
    transition->from.y = from.y;

    transition->to.x = to.x;
    transition->to.y = to.y;

    transition->started = true;
    transition->time = 0;
}
