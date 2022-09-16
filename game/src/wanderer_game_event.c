#include "wanderer_game_event.h"

// Game Event Stack
game_event* game_event_stack_get_start( game_event_stack* stack ){
    return stack->events;
}

game_event* game_event_stack_get_end( game_event_stack* stack ){
    if( stack->top_index == 0 ){
        // avoid overflow....
        return NULL;
    }else{
        return &stack->events[stack->top_index-1];
    }
}

void game_event_stack_push( game_event_stack* stack, game_event* event ){
    if( event->used_times + 1 <= event->max_use_times ||
        event->max_use_times == -1 ){
        stack->events[ stack->top_index++ ] = *event;
        event->used_times++;
    }
}

void game_event_stack_push_many( game_event_stack* stack, 
                                 game_event* events,
                                 u64 amount ){
    u64 event_index = 0;

    while( event_index < amount ){
        game_event_stack_push( stack, &events[event_index] );

        event_index++;
    }
}

game_event game_event_stack_pop( game_event_stack* stack ){
    if( stack->top_index == 0 ){
        game_event dummy = {};

        return dummy;
    }else{
        game_event popped_event = stack->events[ stack->top_index-1 ];
        stack->top_index--;

        return popped_event;
    }
}
