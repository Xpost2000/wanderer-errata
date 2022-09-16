#ifndef WANDERER_H
#define WANDERER_H

#include "strings.h"
#include "memory_pool.h"

#include "common.h"
#include "renderer.h"

#include "wanderer_debug.h"
#include "wanderer_game_event.h"

#include "wanderer_dialogue.h"
#include "wanderer_game_cursor.h"
#include "wanderer_actor.h"

#include "wanderer_lists.h"
#include "wanderer_game_input.h"

#include "wanderer_assets.h"

#include "wanderer_gui.h"

#include "camera.h"

#include "wanderer_cutscene.h"
#include "wanderer_tilemap.h"

#include "collision.h"

#include "wanderer_game_ui.h"
#include "wanderer_floating_messages.h"

#include "platform.h"

#define MAX_GAME_VARIABLES 32000

enum game_render_object_type{
    GAME_RENDER_OBJECT_NONE,

    GAME_RENDER_OBJECT_TILE,
    GAME_RENDER_OBJECT_ACTOR,
    GAME_RENDER_OBJECT_PROJECTILE,
    GAME_RENDER_OBJECT_CONTAINER,
    GAME_RENDER_OBJECT_ITEM_DROP,
    GAME_RENDER_OBJECT_SELECTED_TARGET_TILE,

    GAME_RENDER_OBJECT_TYPES
};

static const char* game_render_object_type_strings[GAME_RENDER_OBJECT_TYPES] =
{
    STRINGIFY(GAME_RENDER_OBJECT_NONE),
    STRINGIFY(GAME_RENDER_OBJECT_TILE),
    STRINGIFY(GAME_RENDER_OBJECT_ACTOR),
    STRINGIFY(GAME_RENDER_OBJECT_PROJECTILE),
    STRINGIFY(GAME_RENDER_OBJECT_CONTAINER),
    STRINGIFY(GAME_RENDER_OBJECT_ITEM_DROP),
    STRINGIFY(GAME_RENDER_OBJECT_SELECTED_TARGET_TILE)
};

// all the objects have enough information about
// themselves that I should be able to just recalculate
// what I want from them directly.
// tiles are a bit more iffy, because it depends on the level
// I load.... I might need to specialize for the tilemaps
// because of the way I do it here...
typedef struct game_render_object{
    u8 type;
    union{
        u32 index_to_object;
        // NOTE(jerry):
        // I know I can actually store a real index
        // by "packing" the coordinates into a 1D index...
        // This was just easier for the concept.
        // x and y for tilemap indexing
        struct{
            u32 x_index;
            u32 y_index;
        };
    };
}game_render_object;

// technically it should be game_render context
// or scene......
typedef struct game_render_list{
    tilemap* current_scene_map;

    u32 count;
    game_render_object* objects;
}game_render_list;

void game_render_list_push( game_render_list* render_list, game_render_object render_object );
void game_render_list_sort_render_objects( game_render_list* render_list, game_state* state );
void game_render_list_submit_all_render_commands( game_render_list* render_list, render_layer* layer, game_state* state );

enum game_state_mode{
    GAME_STATE_MAIN_MENU,
    GAME_STATE_GAME_PLAY,
    GAME_STATE_INTRO,

    // Should be separate tools
    GAME_STATE_EDITOR_MODEL_EDITOR,
    GAME_STATE_EDITOR_DIALOGUE_EDITOR,

    GAME_STATE_QUIT,

    GAME_STATE_COUNT
};

/*really just integer flags.*/
typedef struct game_variable{
    char name[64];
    union{ i32 value; };
}game_variable;

typedef struct game_variable_dictionary{
    u64 variable_count;
    game_variable variables[MAX_GAME_VARIABLES];
}game_variable_dictionary;

enum game_moused_over_selection_type{
    MOUSED_OVER_NOTHING,

    MOUSED_OVER_ACTOR,
    MOUSED_OVER_PICKUP_ITEM,
    MOUSED_OVER_CONTAINER,

    MOUSED_OVER_TYPE_COUNT
};

typedef struct game_moused_over_selection{
    u16 type;
    u64 selected_index;
}game_moused_over_selection;

typedef struct game_state_intro{
    /*empty for now*/
}game_state_intro;

enum game_state_main_menu_sub_mode{
    GAME_STATE_MAINMENU_MAIN,
    GAME_STATE_MAINMENU_SETTINGS,
    GAME_STATE_MAINMENU_CREDITS,
    GAME_STATE_MAINMENU_SAVEGAME,

    GAME_STATE_MAINMENU_SUB_MENU_TYPES,
};

enum actor_model_names{
    ACTOR_MODEL_NONE,
    ACTOR_MODEL_HUMAN,
    ACTOR_MODEL_ORC,
    ACTOR_MODEL_ORC_SOLDIER,
    ACTOR_MODEL_COUNT
};

enum game_model_editor_state_test_scene_type{
    GAME_MODEL_EDITOR_TEST_SCENE_ISOLATED,
    GAME_MODEL_EDITOR_TEST_SCENE_WORLD,

    GAME_MODEL_EDITOR_TEST_SCENE_TYPES
};

enum game_model_editor_file_browser_mode{
    MODEL_EDITOR_FILE_BROWSER_READ_MODEL_MODE,
    MODEL_EDITOR_FILE_BROWSER_WRITE_MODEL_MODE,
    MODEL_EDITOR_FILE_BROWSER_OPEN_MODEL_FRAME_MODE,
    MODEL_EDITOR_FILE_BROWSER_NEW_MODEL_FRAME_MODE,
    MODEL_EDITOR_FILE_BROWSER_OPEN_ANIMATION_SET_SPRITESHEET,

    MODEL_EDITOR_FILE_BROWSER_MODE_COUNT,
};

void actor_model_write_to_disk( game_assets* assets, actor_model* model, char* as_name );
void actor_model_load_from_disk( game_assets* assets, actor_model* model, char* file_name );

enum flag_selection_screen_mode{
    EDITOR_STATE_FLAG_SELECTION_SCREEN_NONE,
    EDITOR_STATE_FLAG_SELECTION_SCREEN_ANIMATION_SET_FLAGS,
    EDITOR_STATE_FLAG_SELECTION_SCREEN_ANIMATION_SET_DIRECTION_FLAGS,
    EDITOR_STATE_FLAG_SELECTION_MODE_COUNT
};

typedef struct game_model_editor_state{
    u8 model_scene_type;

    u32 current_frame;
    u32 current_animation_set;
    u32 current_animation_group;

    actor_model current_actor_model;

    u8 file_browser_mode;
    b32 file_browser_open;
    char working_directory[PLATFORM_FILE_NAME_MAX];

    b32 flag_selection_screen_open;
    u8 flag_selection_mode;

    f32 lookat_target_direction_x;
    f32 lookat_target_direction_y;
}game_model_editor_state;

#define MAX_COMBAT_ROUND_PARTICIPANTS 1024
typedef struct combat_participant {
    u32 id;
}combat_participant;

typedef struct game_combat_round {
    u16 participant_count;
    u16 current_participant;
    combat_participant participants[MAX_COMBAT_ROUND_PARTICIPANTS];
}game_combat_round;

combat_participant game_combat_round_get_current_participant(game_combat_round* combat_round);
actor* combat_participant_lookup_actor(actors_list* actors, combat_participant participant);

typedef struct game_state{
    i32 mode;
    b32 paused;

    game_assets assets;

#ifdef DEBUG_BUILD
    game_model_editor_state model_editor;
#endif

    game_asset_handle models[ACTOR_MODEL_COUNT];

    game_state_intro intro;

    // NOTE(jerry): This is technically graphics state
    // move it somewhere else maybe?
    i32 screen_width;
    i32 screen_height;

    u32 current_world_map;

    game_asset_handle ui_atlas;
    game_asset_handle ui_icon_atlas;

    game_asset_handle isometric_tile_a;
    game_asset_handle isometric_tile_b;
    game_asset_handle isometric_char_a;
    game_asset_handle isometric_char_b;
    game_asset_handle iso_gore_pile;

    game_asset_handle main_menu_asset_a;
    game_asset_handle main_menu_asset_b;
    game_asset_handle main_menu_asset_c;

    // thanks Surt from opengameart
    // 32 * 68 based on my own pixel measurements???
    // 3072 * 1920 for whole image.
    // 768 * 640 for the section I cropped and used.
    game_asset_handle dawnblocker_floor;
    game_asset_handle dawnblocker_block;

    game_asset_handle gothic_font;
    game_asset_handle font_texture;
    game_asset_handle bigger_font_texture;

    // from Clint Bellanger YAY FLARE!
    // 256 * 256 tiled...
    game_asset_handle oga_iso_skeleton_test;

    tilemap_list tilemaps;
    actors_list actors;

    // Turn into index
    dialogue_info* active_dialogue;
    cutscene_info* active_cutscene;

    u64 player_index;
    b32 player_is_in_combat;

    b32 camera_refocus_data_set;

    game_action_bindings bindings;

    camera_state camera;
    camera_transition transition;

    game_variable_dictionary variables;
    item_dictionary items;
    spell_dictionary spells;

    gui_state gui_state;
    game_journal journal;

    game_ui_state ui_info;
    game_combat_round combat_round;

    game_event_stack event_stack;
    floating_messages floating_messages;

    render_layer game_layer;
    render_layer game_text_layer;
    render_layer ui_layer;

    render_layer background;
    render_layer menu_ui;

    // I don't think this is a good idea to literally
    // just have that here.
    memory_pool strings_scratch; // Enough parts of the game_state require this.
    memory_pool render_list_memory;
    memory_pool dialogue_memory;
    void* scratch_memory;

    f32 death_sequence_timer;
    b32 playing_death_sequence;

    f32 real_time_turn_timer;
}game_state;

void show_game_cursor(game_state* state);
void hide_game_cursor(game_state* state);

void game_finish( game_state* state );
void update_render_game( game_state* state,
                         renderer* renderer,
                         game_input *input, f32 delta_time );
/*starts with zero.*/
void game_variable_dictionary_add_variable( game_variable_dictionary* dictionary, char* name );
void game_variable_dictionary_add_variable_default_value( game_variable_dictionary* dictionary, char* name, i32 start );
game_variable* game_variable_dictionary_find( game_variable_dictionary* dictionary, char* variable_name );
void floating_messages_on_actor(floating_messages* messages, actor* target, colorf message_color, char* message_string);

actor* game_get_player_actor( game_state* state );

b32 place_item_on_floor( actor* actor, item_slot item );

#endif
