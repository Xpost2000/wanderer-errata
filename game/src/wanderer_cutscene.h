#ifndef WANDERER_CUTSCENE_H
#define WANDERER_CUTSCENE_H

#include "common.h"
#include "camera.h"

#define MAX_CUTSCENE_ACTIONS 2000

typedef struct actor actor;
typedef struct game_state game_state;

// Override the current think state
// and run the cutscene's wanted think state.
enum cutscene_actor_action_type{
    CUTSCENE_ACTOR_ACTION_NONE,

    CUTSCENE_ACTOR_ACTION_MOVE,

    CUTSCENE_ACTOR_ACTION_TYPES
};

enum cutscene_action_type{
    CUTSCENE_ACTION_NONE,

    CUTSCENE_ACTION_ACTOR_ACTION,
    CUTSCENE_ACTION_CAMERA_TRANSITION,
    CUTSCENE_ACTION_SHOW_FLOATING_MESSAGE,

    CUTSCENE_ACTION_TYPE_COUNT
};

typedef struct cutscene_action_show_floating_message{
    f32 message_x;
    f32 message_y;

    f32 message_lifetime;
    char* message_string;
}cutscene_action_show_floating_message;

typedef struct cutscene_action_camera_transition{
    camera_state from;
    camera_state to;

    b32 relative_end_position;
}cutscene_action_camera_transition;

typedef struct cutscene_action_actor_move{
    f32 start_x;
    f32 start_y;

    f32 end_x;
    f32 end_y;

    /*
     * if this is true, 
     * end_x and y are relative positions
     * to start_x and y. Otherwise they
     * are absolute positions.
     *
     * unit is pixels.
     */
    b32 relative_end_position;
}cutscene_action_actor_move;


typedef struct cutscene_action_actor_action{
    u16 type;
    actor* target;

    union{
        cutscene_action_actor_move move;
    };
}cutscene_action_actor_action;

typedef struct cutscene_action{
    u16 type;
    /*
     * At the moment all cutscene
     * actions are _relative_ offsets
     * so they need to know what they're offseting
     * from so this first frame flag just lets me
     * setup their _anchor_ position to update from.
     */
    b32 first_frame;

    /*if true, only continue when this event isn't finished.*/
    b32 wait_for_finish;
    f32 wait_time;

    union{
        cutscene_action_actor_action actor_action;
        cutscene_action_camera_transition camera_transition;
        cutscene_action_show_floating_message show_floating_message;
    };
}cutscene_action;

/*
 * NOTE(jerry):
 *
 * I haven't thought to do this yet,
 * but some cutscenes might not occur on the
 * same map as the player, and I should probably
 * do that. It's not hard to do thankfully.
 *
 * Although I should probably make all the game stuff
 * use handles instead of pointers so I can actually
 * serialize all of this.
 *
 * I'm not doing the relative pointer calculation stuff.
 *
 * ROBUSTNESS(jerry):
 *
 * Cutscenes in this current moment can be interrupted,
 * ( does not block game input and stuff. )
 *
 * Make sure I can't do that when the demo comes around.
 */
typedef struct cutscene_info{
    char* name;

    b32 running;
    u32 current_action;

    b32 timed;
    f32 scene_time;

    b32 return_to_original_camera_pos;
    camera_state before;

    u32 action_count;
    cutscene_action actions[MAX_CUTSCENE_ACTIONS];
}cutscene_info;

void cutscene_info_set_sequence_time( cutscene_info* info, f32 time_set );

void cutscene_info_push_floating_message( cutscene_info* info, f32 message_lifetime, f32 message_x, f32 message_y, char* message_string );
void cutscene_info_push_camera_transition( cutscene_info* info, f32 offset_x, f32 offset_y );
void cutscene_info_push_move_actor( cutscene_info* info, actor* target, f32 offset_x, f32 offset_y );

void cutscene_info_wait_for_current_event_to_finish( cutscene_info* info );
void cutscene_info_wait_time( cutscene_info* info, f32 time_to_wait );
void cutscene_info_run( cutscene_info* info, game_state* state, f32 delta_time );

#endif
