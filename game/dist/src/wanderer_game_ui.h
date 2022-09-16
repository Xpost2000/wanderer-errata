#ifndef WANDERER_GAME_UI
#define WANDERER_GAME_UI

#include "common.h"
#include "renderer.h"

#include "wanderer_debug.h"
#include "wanderer_game_event.h"

#include "wanderer_dialogue.h"
#include "wanderer_game_cursor.h"
#include "wanderer_actor.h"

#include "wanderer_lists.h"
#include "wanderer_game_input.h"

#include "wanderer_gui.h"

#include "camera.h"

#include "wanderer_cutscene.h"
#include "wanderer_tilemap.h"

#include "collision.h"

enum game_ui_state_mode{
    GAME_UI_STATE_NONE,
    GAME_UI_STATE_MAINMENU,
    GAME_UI_STATE_INGAME, 

    GAME_UI_STATE_DIALOGUE,
    GAME_UI_STATE_JOURNAL,
    GAME_UI_STATE_CHARACTER_RECORD,

    GAME_UI_STATE_INVENTORY_PLAYER,
    GAME_UI_STATE_INVENTORY_LOOTING,
    GAME_UI_STATE_INVENTORY_TRADING,

    GAME_UI_STATE_PAUSE_MENU,
    GAME_UI_STATE_CUTSCENE,

    GAME_UI_STATE_COUNT
};

#define MAX_JOURNAL_ENTRIES 4096

/*ROBUSTNESS(jerry): Needs string table.*/
typedef struct game_journal_entry{
    char* entry_title;
    char* entry_text;
}game_journal_entry;

typedef struct game_journal{
    u64 entry_count;
    game_journal_entry entries[MAX_JOURNAL_ENTRIES];
}game_journal;

typedef struct character_sheet_ui_state{
    b32 opened;
}character_sheet_ui_state;

typedef struct journal_ui_state{
    b32 opened;
    u32 current_page;
}journal_ui_state;

typedef struct inventory_trading_ui_state{
}inventory_trading_ui_state;

// Should note I really want to be operating on inventories
// not loot targets because that makes me do more work. For now
// I'll use loot targets but we'll change to inventories basically
// right after.
enum inventory_looting_target_type {
    INVENTORY_LOOTING_TARGET_ACTOR,
    INVENTORY_LOOTING_TARGET_CONTAINER,
    INVENTORY_LOOTING_TARGET_TYPE_COUNT
};
typedef struct inventory_looting_ui_state{
    u8 target_type;
    u32 target_index;
}inventory_looting_ui_state;

typedef struct inventory_player_ui_state{
}inventory_player_ui_state;

typedef struct main_menu_ui_state{
}main_menu_ui_state;

enum game_ui_widget_type{
    GAME_UI_WIDGET_TYPE_NONE,
    GAME_UI_WIDGET_TYPE_RECTANGLE,
    GAME_UI_WIDGET_TYPE_IMAGE,
    GAME_UI_WIDGET_TYPE_WINDOW,
    GAME_UI_WIDGET_TYPE_BUTTON,
    GAME_UI_WIDGET_TYPE_TEXT,

    // Whenever I can, I want to replace these with real gui
    // elements, but for now I need these to be widgets...
    // basically this is a 1-1 copy of whatever I had before.
    GAME_UI_WIDGET_TYPE_PLACEHOLDER_DIALOGUE,
    GAME_UI_WIDGET_TYPE_PLACEHOLDER_JOURNAL,
    GAME_UI_WIDGET_TYPE_PLACEHOLDER_PLAYER_INVENTORY,
    GAME_UI_WIDGET_TYPE_PLACEHOLDER_LOOT_INVENTORY,
    GAME_UI_WIDGET_TYPE_PLACEHOLDER_TRADING,

    GAME_UI_WIDGET_TYPE_COUNT
};

typedef struct game_ui_widget_window{
    u8 skin;
    colorf color;
}game_ui_widget_window;

typedef struct game_ui_widget_text{
    game_asset_handle font;

    char* text;
    colorf color;
}game_ui_widget_text;

typedef struct game_ui_widget_image{
    u32 atlas_index;
    colorf color;
}game_ui_widget_image;

typedef struct game_ui_widget_rectangle{
    colorf color;
}game_ui_widget_rectangle;

// NOTE(jerry):
// because only atlas buttons exist this
// will only account for atlas_widget buttons
typedef struct game_ui_widget game_ui_widget;
typedef void (*game_ui_widget_button_callback)(game_ui_widget*, void*);
typedef struct game_ui_widget_button{
    u32 atlas_index;
    game_ui_widget_button_callback on_click;
}game_ui_widget_button;

typedef struct game_ui_widget game_ui_widget;
typedef void (*game_ui_widget_update_callback)(game_ui_widget*, game_state*, game_input*, f32);
typedef struct game_ui_widget{
    u8 type;

    vec2 position;
    vec2 size;

    game_ui_widget* parent;
    b32 show;

    union{
        game_ui_widget_window window;
        game_ui_widget_button button;
        game_ui_widget_text text;
        game_ui_widget_rectangle rectangle;
        game_ui_widget_image image;
    };

    u32 children_count;
    game_ui_widget* children;

    game_ui_widget_update_callback update;
}game_ui_widget;

typedef struct game_ui_screen{
    game_ui_widget root;
    void* userdata;
}game_ui_screen;

// should maybe make this a stack.
typedef struct game_ui_state{
    memory_pool ui_memory;

    u32 virtual_width;
    u32 virtual_height;

    u16 mode;
    b32 touching_ui;

    game_cursor cursor;
    
    main_menu_ui_state main_menu;
    journal_ui_state journal;
    character_sheet_ui_state character_sheet;

    inventory_player_ui_state player_inventory;
    inventory_looting_ui_state loot_inventory;

    item_slot dragged_item;

    game_ui_screen ui_screens[GAME_UI_STATE_COUNT];
}game_ui_state;

void game_ui_initialize( game_state* state );
void game_ui_finish( game_state* state );

void game_ui_update_render( game_state* state, renderer* renderer, game_input* input, f32 delta_time );
void game_ui_disable(game_state* state);

void game_ui_toggle_pause( game_state* state );

void game_ui_open_loot_container(game_state* state, u32 container_index);
void game_ui_open_loot_actor(game_state* state, u32 actor_index);
void game_ui_toggle_player_inventory( game_state* state );
void game_ui_toggle_journal( game_state* state );
void game_ui_toggle_character_sheet( game_state* state );

void game_ui_show_combat_initial_elements(game_state* state);
void game_ui_show_combat_finished_elements(game_state* state);

#endif
