#ifndef WANDERER_GAME_ACTION_INPUT_H
#define WANDERER_GAME_ACTION_INPUT_H

#include "input.h"
#include "common.h"
/*
 * NOTE(jerry):
 *
 * These are the default bindings for input domains / contexts.
 */
enum game_action_binding_context_type{
    GAME_INPUT_MODE_ALL,

    GAME_INPUT_MODE_GAMEPLAY,
    GAME_INPUT_MODE_IN_INVENTORY,
    GAME_INPUT_MODE_IN_JOURNAL,
    GAME_INPUT_MODE_IN_DIALOGUE,
    GAME_INPUT_MODE_IN_CHARACTERSHEET,

    GAME_ACTION_BINDING_CONTEXT_COUNT
};

enum game_action_binding_action_type{
    GAME_ACTION_CAMERA_MOVE_UP,
    GAME_ACTION_CAMERA_MOVE_DOWN,
    GAME_ACTION_CAMERA_MOVE_LEFT,
    GAME_ACTION_CAMERA_MOVE_RIGHT,

    GAME_ACTION_CAMERA_RECENTER_ON_PLAYER,

    GAME_ACTION_PAUSE_GAME,

    GAME_ACTION_TOGGLE_JOURNAL,
    GAME_ACTION_TOGGLE_INVENTORY,
    GAME_ACTION_TOGGLE_CHARACTERSHEET,

    /*
     * tab key in some crpgs. 
     */
    GAME_ACTION_SHOW_WORLD_INFORMATION,

    GAME_ACTION_PLAYER_ENDS_COMBAT_TURN,

    GAME_ACTION_COUNT
};

enum game_input_key_type{
    GAME_INPUT_KEY_DOWN,
    GAME_INPUT_KEY_PRESSED,

    GAME_INPUT_KEY_TYPE_COUNT
};

typedef struct game_action_binding{
    u8 bound_to;
    u8 input_type;
    u32 key;
}game_action_binding;

typedef struct game_action_binding_array{
    u16 bindings_used;
    game_action_binding bindings[3];
}game_action_binding_array;

// should use linear allocator to reduce memory usage.
typedef struct game_action_binding_context{
    game_action_binding_array bindings[GAME_ACTION_COUNT];
}game_action_binding_context;

typedef struct game_action_bindings{
    game_action_binding_context binding_contexts[GAME_ACTION_BINDING_CONTEXT_COUNT];
    game_input* input;

    u16 current_context;
    b32 disable_input;
}game_action_bindings;

void game_action_bindings_set_current_context( game_action_bindings* bindings, u16 context );
void game_action_bindings_add_binding( game_action_bindings* bindings, u16 input_context, u16 bound_action, u16 key_bound, u16 input_type );
b32 game_action_bindings_action_active( game_action_bindings* bindings, u16 action );

#endif
