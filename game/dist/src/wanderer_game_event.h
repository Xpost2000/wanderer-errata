#ifndef WANDERER_GAME_EVENT_H
#define WANDERER_GAME_EVENT_H

#include "common.h"
#include "wanderer_types_forward_declarations.h"

#define MAX_GAME_EVENTS 256
/*
 * ADDENDUM(jerry):
 * It's a bad idea for me to use this as
 * a general purpose game structure thing because
 * most of the things that fire events might not need
 * this type of info, or need extra types of info.
 * So basically other events translate into these events.
 *
 * These are general game events that
 * anything should be able to fire off
 *
 * although for the most part only the
 * dialogue system is going to use it
 * at least at this point.
 */
enum game_event_type{
    /*TODO: These are all temporary for testing purposes*/
    GAME_EVENT_CRITICAL_PLOT_POINT_REACHED,
    GAME_EVENT_SPAWN_FLOATING_MESSAGE,
    GAME_EVENT_PRINT_TEST,

    GAME_EVENT_SET_VARIABLE,

    GAME_EVENT_TYPES_COUNT
};

typedef struct game_event{
    i16 event_type;

    i32 used_times;
    i32 max_use_times;

    union{
        struct{
            char* variable_name;
            i32 new_value;
        }set_variable;

        struct{
            char* message_contents;
            f32 start_x;
            f32 start_y;

            f32 life_time;
        }floating_message;

        struct{
            char* what; 
        }print_test;    
    };
}game_event;

typedef struct game_event_stack{
    u64 top_index;
    game_event events[MAX_GAME_EVENTS];
}game_event_stack;

game_event* game_event_stack_get_start( game_event_stack* stack );
game_event* game_event_stack_get_end( game_event_stack* stack );
void game_event_stack_push_many( game_event_stack* stack, game_event* events, u64 amount );
void game_event_stack_push( game_event_stack* stack, game_event* event );
game_event game_event_stack_pop( game_event_stack* stack );

#endif
