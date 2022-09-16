#ifndef WANDERER_GUI_H
#define WANDERER_GUI_H

// TODO(jerry): The selectable list does not scroll. Should probably do that.
// TODO(jerry): Opt for using a "dynamic" linear allocation scheme
// I think I can get away with it because I rarely actually need the
// state in here, any important state will remain as fixed nums anyways.
#define MAX_GUI_WIDGETS 4096
#define MAX_GUI_RADIO_BUTTON_GROUPS MAX_GUI_WIDGETS
#define MAX_GUI_LAYOUT_STACK_MAX 32
#define MAX_GUI_CHILDREN_AREA_STACK_MAX 32
#define MAX_GUI_SELECTABLE_LIST_ITEM 256

#include "common.h"
#include "renderer.h"
#include "input.h"
#include "memory_pool.h"

#include "wanderer_assets.h"

// I've come to notice that all the widgets
// are basically the exact same data and only
// differ in behavior lol.
enum gui_widget_type{
    GUI_WIDGET_BUTTON,
    GUI_WIDGET_CHECK_BOX_BUTTON,
    GUI_WIDGET_RADIO_BUTTON,
    GUI_WIDGET_DRAGGABLE,
    GUI_WIDGET_TEXTLINE_EDIT,
    GUI_WIDGET_SELECTABLE_LIST,

    GUI_WIDGET_TYPE_COUNT
};

// for "extended widgets"
// in this case that's the file selector.
enum gui_result {
    GUI_RESULT_CANCEL, /*explicit cancel*/
    GUI_RESULT_NONE, /*nothing happened. dummy return*/
    GUI_RESULT_OKAY,
    GUI_RESULT_TYPE
};

/*
 * NOTE(jerry):
 * GUI_LAYOUT_*_SIZERS
 * are dummy enums. They just
 * designate whether this type of
 * layout allows you to manually size
 * things ( pad out their "children" )
 */
enum gui_layout_type{
    /*enum marking*/
    GUI_LAYOUT_MANUAL_PADDERS,
    GUI_LAYOUT_MANUAL_SIZERS,
    /*used in gui layout_padding functions*/

    GUI_LAYOUT_VERTICAL,
    GUI_LAYOUT_HORIZONTAL,

    /*enum marking*/
    GUI_LAYOUT_AUTO_SIZERS,
    /*used in gui_layout_padding functions*/

    GUI_LAYOUT_FIXED_HORIZONTAL,
    GUI_LAYOUT_FIXED_VERTICAL,
    GUI_LAYOUT_GRID,

    /*more enum marking*/
    GUI_LAYOUT_AUTO_PADDED,
    GUI_LAYOUT_TYPE_COUNT
};

typedef struct gui_selection_list{
    f32 x;
    f32 y;

    f32 w;
    f32 h;

    struct{
        b32 present;
        u32 value;
    }selection_item;

    u32* selectable_index_ptr;
    u32 item_count;

    b32 should_have_scroll;
    f32 scroll_y;

    b32 scroll_bar_focused;
}gui_selection_list;

typedef struct gui_checkbox_button{
    f32 x;
    f32 y;

    f32 w;
    f32 h;

    b32 clicked;
    b32 toggled;
}gui_checkbox_button;

typedef struct gui_radio_button_group{
    i32 focused_id;
    i32 next_id;
}gui_radio_button_group;

typedef struct gui_radio_button{
    f32 x;
    f32 y;

    f32 w;
    f32 h;

    char* text;

    b32 clicked;

    i32 id_in_group;
}gui_radio_button;

typedef struct gui_button{
    f32 x;
    f32 y;

    f32 w;
    f32 h;

    char* text;

    b32 clicked;
}gui_button;

enum gui_draggable_type{
    GUI_DRAGGABLE_NONE,

    GUI_DRAGGABLE_VEC2,
    GUI_DRAGGABLE_FLOAT,

    GUI_DRAGGABLE_TYPES
};

typedef struct gui_draggable_vec2{
    f32 x;
    f32 y;
}gui_draggable_vec2;

typedef struct gui_draggable{
    f32 x;
    f32 y;

    f32 w;
    f32 h;

    char* text;

    b32 focused;

    u8 type;
    union{
        gui_draggable_vec2 vec2_data;
        f32 float_data;
    }start_data;
}gui_draggable;

// place holder widget that needs to just
// report itself to the layout system.
typedef struct gui_textline_edit{
    f32 x;
    f32 y;

    f32 w;
    f32 h;
    
    b32 focused;
}gui_textline_edit;

typedef struct gui_widget{
    /*
    * It is only now I realize I'm actually inconsistent with this naming
     * it's either *_type or type......
     * TODO: standardize that naming scheme!
     */
    i32 type;
    u32 id; // index.

    struct gui_widget* parent;

    union{
        gui_button button;
        gui_radio_button radio_button;
        gui_checkbox_button checkbox_button;
        gui_draggable draggable;
        gui_textline_edit line_edit;
        gui_selection_list selection_list;
    };
}gui_widget;

typedef struct gui_vertical_layout{
    f32 vertical_element_height;
    f32 margin_top;
    f32 margin_bottom;
}gui_vertical_layout;

typedef struct gui_horizontal_layout{
    f32 horizontal_element_width;
    f32 margin_left;
    f32 margin_right;
}gui_horizontal_layout;

typedef struct gui_grid_layout{
    /*in element units*/
    u32 grid_w;
    u32 grid_h;

    f32 element_width;
    f32 element_height;

    f32 margin_right;
    f32 margin_top;
}gui_grid_layout;

typedef struct gui_layout{
    i32 layout_type;

    f32 start_x;
    f32 start_y;

    f32 next_x;
    f32 next_y;

    /*Margin is not done correctly. TODO?*/
    union{
        gui_vertical_layout vertical;
        gui_horizontal_layout horizontal;
        gui_grid_layout grid;
    };
}gui_layout;

typedef struct gui_state{
    renderer* renderer;
    render_layer* layer;

    memory_pool* scratch_pool;

    game_input* input;

    game_assets* assets;
    game_asset_handle font_texture;

    // more accurate to say this is the
    // start frame...
    b32 last_mouse_frame_clicked;

    i32 last_mouse_frame_x;
    i32 last_mouse_frame_y;

    u64 widget_index;
    // used for cheapening one check at the moment...
    u64 focused_widget_index;

    gui_widget* last_parentable_widget;

    u64 widgets_allocated;
    gui_widget widgets[MAX_GUI_WIDGETS];
    gui_radio_button_group radio_groups[MAX_GUI_WIDGETS];

    u64 gui_layout_stack_top;
    gui_layout layout_stack[MAX_GUI_LAYOUT_STACK_MAX];
}gui_state;

void gui_begin_frame( gui_state* state, renderer* renderer, render_layer* layer, game_input* input );
void gui_set_font( gui_state* state, game_asset_handle font_id );
void gui_end_frame( gui_state* state );
u8 gui_do_file_selector( gui_state* state, b32* opened, char* directory, size_t directory_string_length, char* selected_directory, size_t selected_directory_string_length );
b32 gui_do_button( gui_state* state, char* text, f32 x, f32 y );
b32 gui_do_blank_button( gui_state* state, f32 x, f32 y, f32 w, f32 h );
b32 gui_do_text_button( gui_state* state, char* text, f32 x, f32 y );
void gui_do_check_box_button( gui_state* state, b32* target, f32 x, f32 y );
void gui_do_radio_button( gui_state* state, char* text, f32 x, f32 y, i32* target, i32 group_id );
void gui_do_draggable_vec2( gui_state* state, char* text, f32 x, f32 y, f32* target_x, f32* target_y, f32 min_x, f32 max_x, f32 min_y, f32 max_y );
void gui_do_draggable_float( gui_state* state, char* text, f32 x, f32 y, f32* target, f32 min, f32 max );
void gui_do_textline_edit( gui_state* state, char* target_text, u64 target_max_length, f32 x, f32 y );
// TODO(jerry):
// should I make a real collapsable group system?
// this hack seems to do okay enough...... Maybe when I need
// a real hierarchy I'll know...
bool gui_do_collapsable_group( gui_state* state, char* group_name, b32* opened, f32 x, f32 y );

void gui_quad_element( gui_state* state, f32 x, f32 y, f32 w, f32 h, f32 r, f32 g, f32 b, f32 a );
void gui_text_element( gui_state* state, char* text, f32 x, f32 y, f32 r, f32 g, f32 b, f32 a );
void gui_wrapped_text_element( gui_state* state, char* text, f32 x, f32 y, f32 bounds_w, f32 bounds_h, f32 r, f32 g, f32 b, f32 a );

void gui_begin_selection_list( gui_state* state, f32 x, f32 y, f32 w, f32 h, u32* selected_index );
void gui_selection_list_item( gui_state* state, char* name );
void gui_end_selection_list( gui_state* state );

void gui_layout_vertical_padding( gui_state* state, f32 padding );
void gui_layout_horizontal_padding( gui_state* state, f32 padding );

void gui_begin_grid_layout( gui_state* state, f32 start_x, f32 start_y, u32 grid_w, u32 grid_h, f32 element_width, f32 element_height, f32 margin_right, f32 margin_top );
void gui_begin_fixed_horizontal_layout( gui_state* state, f32 start_x, f32 start_y, f32 horizontal_element_width, f32 margin_left, f32 margin_right );
void gui_begin_fixed_vertical_layout( gui_state* state, f32 start_x, f32 start_y, f32 vertical_element_height, f32 margin_top, f32 margin_bottom );

void gui_begin_horizontal_layout( gui_state* state, f32 start_x, f32 start_y, f32 margin_left, f32 margin_right );
void gui_begin_vertical_layout( gui_state* state, f32 start_x, f32 start_y, f32 margin_top, f32 margin_bottom );

#if 0
void gui_begin_vertical_scrollarea( gui_state* state, f32 x, f32 y, f32 w, f32 h );
void gui_end_vertical_scrollarea( gui_state* state );
#endif

f32 gui_layout_next_x( gui_state* state );
f32 gui_layout_next_y( gui_state* state );
void gui_end_layout( gui_state* state );

#endif
