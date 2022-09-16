#ifndef WANDERER_GAME_CURSOR_H
#define WANDERER_GAME_CURSOR_H

#include "wanderer_types_forward_declarations.h"
#include "wanderer_assets.h"

// As it pertains to the actual
// game world and not UI.
enum cursor_action{
    CURSOR_SELECT = 0,
    CURSOR_MOVEMENT = 0,
    CURSOR_DIALOGUE,
    CURSOR_ACTIVATE,
    CURSOR_ATTACK,

    CURSOR_ACTION_COUNT,
    CURSOR_UNKNOWN
};

/*
 * only exists to allow friendly fire from the player
 * and talking to aggressive npcs.
 *
 * All other actions are context based.
 */
enum cursor_override_state {
    CURSOR_OVERRIDE_STATE_NONE,
    CURSOR_OVERRIDE_STATE_ATTACK,
    CURSOR_OVERRIDE_STATE_DIALOGUE,

    CURSOR_OVERRIDE_STATE_COUNT
};

typedef struct game_cursor{
    u16 action_mode;
    u16 override_state;
    b32 hidden;

    union{
        struct{
            game_asset_handle cursor_texture_normal;
            game_asset_handle cursor_texture_attack;
            game_asset_handle cursor_texture_dialogue;
            game_asset_handle cursor_texture_interaction;
        };

        game_asset_handle cursor_textures[CURSOR_ACTION_COUNT];
    };
}game_cursor;

#endif
