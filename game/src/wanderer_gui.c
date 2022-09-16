#include "wanderer_gui.h"
#include "collision.h"
#include "platform.h"
#include <string.h>

/*
  TODO(jerry):
  The parent widget functionality solely exists so I can
  do selectable list, so technically parenting behavior does not work
  on anything but selectable lists...

  I don't think I myself will really expand on parented stuff so I'll probably
  just leave it incomplete to reduce workload.
 */

/*IMMEDIATE GUI STUFF*/
void gui_set_font( gui_state* state, game_asset_handle font_id ){
    state->font_texture = font_id;
}

void gui_begin_frame( gui_state* state,
                      renderer* renderer,
                      render_layer* layer,
                      game_input* input ){
    state->last_parentable_widget = NULL;
    state->widget_index = 0;
    state->renderer = renderer;
    state->assets = renderer->assets;
    state->layer = layer;
    state->input = input;

    for( u32 radio_group_index = 0;
         radio_group_index < MAX_GUI_WIDGETS;
         ++radio_group_index ){
        state->radio_groups[radio_group_index].next_id = 0;
    }

    if( input_is_mouse_left_down( state->input ) ){
        if( !state->last_mouse_frame_clicked ){
            state->last_mouse_frame_x = state->input->current.mouse.x;
            state->last_mouse_frame_y = state->input->current.mouse.y;

            state->last_mouse_frame_clicked = true;
        }
    }else{
        state->last_mouse_frame_clicked = false;
    }

    if( state->widgets[state->focused_widget_index].type == GUI_WIDGET_TEXTLINE_EDIT ){
        if( !state->widgets[state->focused_widget_index].line_edit.focused ){
            input_end_text_edit( state->input );
        }
    }
}

void gui_end_frame( gui_state* state ){
    state->widget_index = 0;
    state->gui_layout_stack_top = 0;
    state->layer = 0;
    state->input = 0;
    state->font_texture = (game_asset_handle){};
}

static void gui_internal_recalculate_current_layout( gui_state* state ){
    gui_layout* top_layout = &state->layout_stack[state->gui_layout_stack_top-1];

    if( top_layout ){
        switch( top_layout->layout_type ){
            case GUI_LAYOUT_FIXED_VERTICAL:
            case GUI_LAYOUT_VERTICAL:
            {
                top_layout->next_y += top_layout->vertical.vertical_element_height;
                top_layout->next_y += top_layout->vertical.margin_bottom + top_layout->vertical.margin_top;
            }
            break;
            case GUI_LAYOUT_FIXED_HORIZONTAL:
            case GUI_LAYOUT_HORIZONTAL:
            {
                top_layout->next_x += top_layout->horizontal.horizontal_element_width;
                top_layout->next_x += top_layout->horizontal.margin_left + top_layout->horizontal.margin_right;
            }
            break;
            case GUI_LAYOUT_GRID:
            {
                /*NOTE(jerry): Check for bounds?*/
                if( (top_layout->next_x - top_layout->start_x) >= 
                    ((top_layout->grid.grid_w-1) * top_layout->grid.element_width) ){
                    top_layout->next_x = top_layout->start_x;
                    top_layout->next_y += top_layout->grid.element_height 
                                        + top_layout->grid.margin_top;
                }else{
                    top_layout->next_x += top_layout->grid.element_width 
                                        + top_layout->grid.margin_right;
                }
            }
            break;
            default:
            {
            }
            break;
        }
    }
}

static void gui_internal_alloc_new_widget( gui_state* state ){
    while( state->widget_index >= state->widgets_allocated ){
        state->widgets_allocated++;
    }
}

static void gui_internal_pushed_widget( gui_state* state ){
    state->widget_index++;
    gui_internal_alloc_new_widget( state );
    gui_internal_recalculate_current_layout( state );
}

static void gui_internal_layout_inform_element_size( gui_state* state,
                                                     f32 w, f32 h ){
    gui_layout* top_layout = &state->layout_stack[state->gui_layout_stack_top-1];

    if( top_layout->layout_type < GUI_LAYOUT_AUTO_SIZERS ){
        switch( top_layout->layout_type ){
            case GUI_LAYOUT_VERTICAL:
            {
                top_layout->vertical.vertical_element_height = h;
            }
            break;
            case GUI_LAYOUT_HORIZONTAL:
            {
                top_layout->horizontal.horizontal_element_width = w;
            }
            break;
        }
    }
}

void gui_quad_element( gui_state* state, 
                       f32 x,
                       f32 y,
                       f32 w,
                       f32 h,
                       f32 r,
                       f32 g,
                       f32 b,
                       f32 a ){
    gui_internal_layout_inform_element_size( state, w, h );
    gui_internal_recalculate_current_layout( state );

    render_command quad_element = {};

    quad_element = render_command_quad( x, y, w, h, r, g, b, a );

    render_layer_push_command( state->layer, quad_element );
}

void gui_text_element( gui_state* state,
                       char* text,
                       f32 x,
                       f32 y,
                       f32 r,
                       f32 g,
                       f32 b,
                       f32 a ){
    f32 w;
    f32 h;

    renderer_get_text_size( state->renderer, state->font_texture, text, &w, &h, NULL );

    gui_internal_layout_inform_element_size( state, w, h );
    gui_internal_recalculate_current_layout( state );

    render_command text_element = {};

    text_element = render_command_text( text, state->font_texture, x, y, 1.0f, r, g, b, a );

    render_layer_push_command( state->layer, text_element );
}

void gui_wrapped_text_element( gui_state* state, 
                               char* text,
                               f32 x,
                               f32 y,
                               f32 bounds_w,
                               f32 bounds_h,
                               f32 r,
                               f32 g,
                               f32 b,
                               f32 a ){
    f32 w;
    f32 h;

    renderer_get_wrapped_text_size( state->renderer, state->font_texture, text, bounds_w, bounds_h, &w, &h, NULL );

    gui_internal_layout_inform_element_size( state, w, h );
    gui_internal_recalculate_current_layout( state );

    render_command wrapped_text_element = {};

    wrapped_text_element = render_command_text_wrapped( text, state->font_texture, 1.0f, r, g, b, a, x, y, bounds_w, bounds_h);

    render_layer_push_command( state->layer, wrapped_text_element );
}

static b32 gui_internal_button_touching_check( gui_state* state, 
                                               gui_widget* widget,
                                               f32 x,
                                               f32 y,
                                               f32 w,
                                               f32 h ){
    b32 mouse_touching = false;

    { 
        rectangle button_rectangle = 
            (rectangle) { 
                x,
                y,
                w,
                h 
            }; 
        rectangle mouse_rectangle = 
            (rectangle) { .x = state->last_mouse_frame_x,
                          .y = state->last_mouse_frame_y,
                          .w = 2,
                          .h = 2 
            };

        mouse_touching = rectangle_intersects( mouse_rectangle, button_rectangle );
    }

    return mouse_touching;
}

static b32 gui_internal_button_clicked_check( gui_state* state,
                                              gui_widget* widget,
                                              b32 mouse_touching ){
    i32 mouse_left_clicked = input_is_mouse_left_click( state->input );

    if( mouse_touching ){
        if( mouse_left_clicked ){
            if( !widget->button.clicked ){
                widget->button.clicked = true;

                return false;
            }
        }else{
            if( widget->button.clicked ){
                widget->button.clicked = false;

                return true;
            }else{
                widget->button.clicked = false;

                return false;
            }
        }
    }else{
        widget->button.clicked = false;
    }

    return false;
}

static void gui_internal_checkbox_button_on_click( gui_state* state,
                                                   gui_widget* widget ){
    i32 mouse_left_clicked = input_is_mouse_left_click( state->input );

    b32 mouse_touching = gui_internal_button_touching_check( 
            state, 
            widget, 
            widget->checkbox_button.x,
            widget->checkbox_button.y,
            widget->checkbox_button.w,
            widget->checkbox_button.h);

    if( mouse_touching ){
        if( mouse_left_clicked ){
            if( !widget->checkbox_button.clicked ){
                widget->checkbox_button.clicked = true;
            }
        }else{
            if( widget->checkbox_button.clicked ){
                widget->checkbox_button.clicked = false;
                widget->checkbox_button.toggled ^= 1;
            }else{
                widget->checkbox_button.clicked = false;
            }
        }
    }else{
        widget->checkbox_button.clicked = false;
    }
}

static gui_widget* gui_internal_button_push( gui_state* state,
                                             char* text,
                                             f32 x,
                                             f32 y,
                                             f32 w,
                                             f32 h ){
    gui_internal_layout_inform_element_size( state, w, h );
    gui_internal_pushed_widget( state );

    gui_widget* current_widget = &state->widgets[state->widget_index];
    {
        if( current_widget->type != GUI_WIDGET_BUTTON ){
            current_widget->type = GUI_WIDGET_BUTTON;
            current_widget->button.text = text;

            current_widget->id = state->widget_index;
        }

        current_widget->button.x = x;
        current_widget->button.y = y;
        current_widget->button.w = w;
        current_widget->button.h = h;
    }

    return current_widget;
}

static gui_widget* gui_internal_checkbox_button_push( gui_state* state,
                                                      f32 x,
                                                      f32 y,
                                                      f32 w,
                                                      f32 h ){
    gui_internal_layout_inform_element_size( state, w, h );
    gui_internal_pushed_widget( state );

    gui_widget* current_widget = &state->widgets[state->widget_index];
    {
        if( current_widget->type != GUI_WIDGET_CHECK_BOX_BUTTON ){
            current_widget->type = GUI_WIDGET_CHECK_BOX_BUTTON;

            current_widget->id = state->widget_index;
        }

        current_widget->checkbox_button.x = x;
        current_widget->checkbox_button.y = y;
        current_widget->checkbox_button.w = w;
        current_widget->checkbox_button.h = h;
    }

    return current_widget;
}

static void gui_internal_button_change_color_on_highlight( gui_state* state,
                                                           b32 mouse_touching,
                                                           f32* r,
                                                           f32* g,
                                                           f32* b,
                                                           f32* a ){
    i32 mouse_left_down = input_is_mouse_left_down( state->input );

    f32 highlight_r = 1.0f;
    f32 highlight_g = 0.8f;
    f32 highlight_b = 0.3f;
    f32 highlight_a = 1.0f;

    f32 active_r = 1.0f;
    f32 active_g = 0.0f;
    f32 active_b = 0.0f;
    f32 active_a = 1.0f;

    if( mouse_touching ){
        if( mouse_left_down ){
            if( r ){
                *r = active_r;
            }

            if( g ){
                *g = active_g;
            }

            if( b ){
                *b = active_b;
            }

            if( a ){
                *a = active_a;
            }
        }else{
            if( r ){
                *r = highlight_r;
            }

            if( g ){
                *g = highlight_g;
            }

            if( b ){
                *b = highlight_b;
            }

            if( a ){
                *a = highlight_a;
            }
        }
    }
}

b32 gui_do_button( gui_state* state, char* text, f32 x, f32 y ){
    f32 w;
    f32 h;

    f32 size;

    renderer_get_text_size( state->renderer, state->font_texture, text, &w, &h, &size );

    w *= 1.25;
    h *= 1.25;

    render_command button_box = {};
    button_box = render_command_quad( x, y, w, h, 0.1, 0.1, 0.1, 1.0 );

    gui_widget* current_widget = gui_internal_button_push( state, text, x, y, w, h );
    bool mouse_touching = gui_internal_button_touching_check( state, current_widget, x, y, w, h );
    
    {
        render_command button_text = {};

        float button_r = 1;
        float button_g = 1;
        float button_b = 1;

        gui_internal_button_change_color_on_highlight( state, mouse_touching, &button_r, &button_g, &button_b, NULL );
        button_text = render_command_text_justified( text,
                                                     state->font_texture,
                                                     1.0f,
                                                     button_r,
                                                     button_g,
                                                     button_b,
                                                     1.00f,
                                                     x, y, w, h,
                                                     TEXT_COMMAND_JUSTIFICATION_CENTER );

        render_layer_push_command( state->layer, button_box );
        render_layer_push_command( state->layer, button_text );
    }

    return gui_internal_button_clicked_check( state, current_widget, mouse_touching );
}

b32 gui_do_blank_button( gui_state* state, f32 x, f32 y, f32 w, f32 h ){
    gui_widget* current_widget = gui_internal_button_push( state, NULL, x, y, w, h );
    bool mouse_touching = gui_internal_button_touching_check( state, current_widget, x, y, w, h );

    {
        float button_r = 0.1;
        float button_g = 0.1;
        float button_b = 0.1;
        gui_internal_button_change_color_on_highlight( state, mouse_touching, &button_r, &button_g, &button_b, NULL );

        render_command button_box = {};

        button_box = render_command_quad( x, y, w, h, button_r, button_g, button_b, 1.0 );
        render_layer_push_command( state->layer, button_box );
    }

    return gui_internal_button_clicked_check( state, current_widget, mouse_touching );
}

static void gui_internal_do_scrollbar( gui_state* state,
                                       f32 x,
                                       f32 y,
                                       f32 w,
                                       f32 h,
                                       f32 y_overflow_ratio,
                                       f32* scroll_y,
                                       b32* scroll_bar_focused ){
    if( !scroll_y || !scroll_bar_focused ){ 
        return; 
    }

    const f32 scroll_bar_width = 10;
    const f32 scroll_bar_x = x + w - scroll_bar_width;
    f32 scroll_bar_y = y;
    f32 scroll_bar_height = h;

    rectangle scroll_bar_quad = 
        (rectangle) { 
            .x = scroll_bar_x,
            .y = scroll_bar_y,
            .w = scroll_bar_width,
            .h = scroll_bar_height
        };

    b32 mouse_touching = false;
    {
        rectangle mouse_rectangle =
            (rectangle) {
                .x = state->last_mouse_frame_x,
                .y = state->last_mouse_frame_y,
                .w = 2,
                .h = 2
            };

        mouse_touching = rectangle_intersects( mouse_rectangle, scroll_bar_quad );
    }

    f32 scroll_bar_r = 1.0;
    f32 scroll_bar_g = 1.0;
    f32 scroll_bar_b = 1.0;

    if( !(*scroll_bar_focused) ){
        if( mouse_touching && state->last_mouse_frame_clicked ){
            *scroll_bar_focused = true;
        }
    }

    if( !state->last_mouse_frame_clicked &&
        *scroll_bar_focused &&
        !state->input->relative_mouse_mode ){
        *scroll_bar_focused = false;
        // This is in accurate but whatever.
        platform_move_mouse( state->last_mouse_frame_x,
                state->last_mouse_frame_y );
    }

    if( *scroll_bar_focused ){
        state->input->relative_mouse_mode = true;

        scroll_bar_r = 1.0;
        scroll_bar_g = 0.0;
        scroll_bar_b = 0.0;

        {
            f32 mouse_delta_y = state->input->current.mouse.delta_y;
            f32 max_scroll_y_movement = h * y_overflow_ratio;
            *scroll_y += mouse_delta_y;
            *scroll_y = f32_clamp( *scroll_y, 0.0f, max_scroll_y_movement );
        }
    }

    render_command scroll_bar_quad_command = render_command_quad( 
            scroll_bar_x,
            scroll_bar_y,
            scroll_bar_width, 
            scroll_bar_height,
            scroll_bar_r, 
            scroll_bar_g,
            scroll_bar_b,
            1.0 );
    render_layer_push_command( state->layer, scroll_bar_quad_command );
}
    
// Walk one level up the linked list of widgets.
static void gui_internal_default_end_parent( gui_state* state ){
    gui_widget* parentable = state->last_parentable_widget;
    gui_widget* parent_of_parent = parentable->parent;

    state->last_parentable_widget = parent_of_parent;
}

static gui_widget* gui_internal_selection_list_push( gui_state* state,
                                                     f32 x,
                                                     f32 y,
                                                     f32 w,
                                                     f32 h,
                                                     u32* selected_index ){
    gui_internal_layout_inform_element_size( state, w, h );
    gui_internal_pushed_widget( state );

    gui_widget* current_widget = &state->widgets[state->widget_index];
    {
        current_widget->parent = state->last_parentable_widget;
        state->last_parentable_widget = current_widget;
        current_widget->selection_list.item_count = 0;
        if( current_widget->type != GUI_WIDGET_SELECTABLE_LIST ){
            current_widget->type = GUI_WIDGET_SELECTABLE_LIST;
            current_widget->id = state->widget_index;

            current_widget->selection_list.selection_item.present = false;
            current_widget->selection_list.selection_item.value = 0;

            current_widget->selection_list.should_have_scroll = false;
            current_widget->selection_list.scroll_y = 0.0f;
        }

        current_widget->selection_list.x = x;
        current_widget->selection_list.y = y;
        current_widget->selection_list.w = w;
        current_widget->selection_list.h = h;

        current_widget->selection_list.selectable_index_ptr = selected_index;
    }

    return current_widget;
}

void gui_begin_selection_list( gui_state* state,
                               f32 x,
                               f32 y,
                               f32 w,
                               f32 h,
                               u32* selected_index ){
    gui_widget* current_widget = gui_internal_selection_list_push( state, x, y, w, h, selected_index );
    render_layer_push_command( state->layer, render_command_start_scissor(x, y, w, h) );

    gui_quad_element( state,
                      x,
                      y,
                      w,
                      h,
                      0.10,
                      0.10,
                      0.10,
                      1.0 );

    f32 font_size;
    {
        game_asset* font = game_asset_get_from_handle( state->assets, state->font_texture );
        font_size = font->font.size;
    }
    gui_begin_vertical_layout( state, x, y, 1, 1 );
}

void gui_selection_list_item( gui_state* state, char* name ){
    gui_widget* selection_list_widget = state->last_parentable_widget;
    gui_selection_list* selection_list = &selection_list_widget->selection_list;

    f32 w;
    f32 h;
    f32 size;

    renderer_get_text_size( state->renderer, state->font_texture,
                            name, &w, &h, &size );

    f32 box_color_r = 0.1;
    f32 box_color_g = 0.1;
    f32 box_color_b = 0.1;

    u32 item_index = selection_list->item_count++;

    // I need better colorscheming stuff...
    if( selection_list->selection_item.present ){
        if( item_index == selection_list->selection_item.value ){
            box_color_r = 1.0 * 0.3;
            box_color_g = 0.8 * 0.3;
            box_color_b = 0.3 * 0.3;
        }
    }


    f32 shared_box_x = gui_layout_next_x( state );
    f32 shared_box_y = gui_layout_next_y( state ) - selection_list->scroll_y;

    render_command quad_element = {};
    quad_element = render_command_quad( shared_box_x,
                                        shared_box_y + size*0.35,
                                        w, h*0.8,
                                        box_color_r,
                                        box_color_g,
                                        box_color_b,
                                        1.0 );
    render_layer_push_command( state->layer, quad_element );

    rectangle selection_list_viewable_zone = 
        (rectangle) {
            .x = selection_list->x,
            .y = selection_list->y,
            .w = selection_list->w,
            .h = selection_list->h
        };
    rectangle selection_list_selection_item_quad =
        (rectangle) {
            .x = shared_box_x,
            .y = shared_box_y + size * 0.35,
            .w = w,
            .h = h * 0.8
        };

    // should I even try to handle double click.
    if ( gui_do_text_button( state,
                             name,
                             shared_box_x,
                             shared_box_y ) ){
        if( rectangle_intersects(selection_list_selection_item_quad,
                    selection_list_viewable_zone) ){
            if( !selection_list->scroll_bar_focused ){
                selection_list->selection_item.present = true;
                selection_list->selection_item.value = item_index;
            }
        }
    }

    if( selection_list->selection_item.present ){
        (*selection_list->selectable_index_ptr) = selection_list->selection_item.value;
    }
}

void gui_end_selection_list( gui_state* state ){
    gui_widget* selection_list_widget = state->last_parentable_widget;
    gui_selection_list* selection_list = &selection_list_widget->selection_list;
    // record scroll status for next frame
    // not proud of most of this but I just need everything to work.
    // The asset system still needs to control more stuff in order for me
    // to actually get this to work.
    // TODO(jerry): I should probably just read the layout system's current y.
    // Since that's technically more accurate.
    f32 occupied_y_pixels = 0.0f;
    {
        f32 size;
        renderer_get_text_size( state->renderer, state->font_texture,
                " ", NULL, NULL, &size );
        occupied_y_pixels = size * selection_list->item_count;
        if( occupied_y_pixels > selection_list->h ){
            if( !selection_list->should_have_scroll ){
                selection_list->should_have_scroll = true;
                selection_list->scroll_y = 0.0f;
            }
        }else{
            selection_list->should_have_scroll = false;
        }
    }
    gui_end_layout(state);
    render_layer_push_command( state->layer, render_command_end_scissor() );

    // handle scroll bar here.
    if( selection_list->should_have_scroll ){
        f32 y_overflow_ratio = ( occupied_y_pixels / selection_list->h );
        fprintf(stderr, "%3.3f\n", y_overflow_ratio);
        gui_internal_do_scrollbar( state,
                selection_list->x,
                selection_list->y,
                selection_list->w,
                selection_list->h,
                y_overflow_ratio,
                &selection_list->scroll_y,
                &selection_list->scroll_bar_focused ); 
    }

    gui_internal_default_end_parent( state );
}

// This is a prebaked widget.
// It is technically not a widget in and of itself.
// TODO(jerry): There are slight bugs over file traversal.
u8 gui_do_file_selector( gui_state* state,
                         b32* opened,
                         char* directory,
                         size_t directory_string_length,
                         char* selected_directory,
                         size_t selected_directory_string_length ){
    memset( selected_directory, 0, selected_directory_string_length );
    fprintf(stderr, "directory: %s, selected_directory: %s\n", directory, selected_directory);

    bool is_open = true;
    if( opened ){
        is_open = *opened;
    }

    if( is_open ){
        platform_directory searched_directory = {};
        searched_directory.capacity = 8192;
        searched_directory.files =
        memory_pool_allocate( state->scratch_pool, sizeof(platform_file_info) * 8192 );
        get_files_in_directory( directory, &searched_directory );

        const i32 screen_width = state->renderer->screen_width;
        const i32 screen_height = state->renderer->screen_width;

        gui_quad_element( state,
                          300, 100,
                          500, 540,
                          0.05,
                          0.05,
                          0.05,
                          1.0 );

        u32 selected_index = 0;
        gui_begin_selection_list( state, 330, 80, 430, 400, &selected_index );
        for( unsigned file_index = 0;
             file_index < searched_directory.count;
             ++file_index ){
            platform_file_info* current_file_item = &searched_directory.files[file_index];
            const unsigned temp_string_size = 255;
            char* item_print_string_name = memory_pool_allocate( state->scratch_pool, temp_string_size );

            switch( current_file_item->type ){
                case PLATFORM_FILE_TYPE_FILE:
                {
                    snprintf( item_print_string_name,
                              temp_string_size,
                              "<FILE> : %s : %d bytes",
                              current_file_item->file_name,
                              current_file_item->size );
                }
                break;
                case PLATFORM_FILE_TYPE_DIRECTORY:
                {
                    snprintf( item_print_string_name,
                              temp_string_size,
                              "<DIR> : %s",
                              current_file_item->file_name );
                }
                break;
            }

            gui_selection_list_item( state, item_print_string_name );
        }
        gui_end_selection_list( state );

        gui_begin_horizontal_layout( state, 300, 590, 5, 5 );
        if( gui_do_button( state,
                           "SELECT",
                           gui_layout_next_x( state ),
                           gui_layout_next_y( state ) ) ){
            if( opened ){
                platform_file_info* current_file_item = &searched_directory.files[selected_index];
                if( current_file_item->type == PLATFORM_FILE_TYPE_FILE ){
                    *opened = false;
                    strncpy( selected_directory,
                            current_file_item->file_name,
                            selected_directory_string_length );
                }else{
                    // TODO(jerry):
                    // This makes an extremely compelling argument for me to
                    // make some string manipulation and file path name
                    // stuff......
                    if( strcmp(current_file_item->file_name, "..") == 0 ){
                        u64 end_index = 0;
                        {
                            end_index = strlen(directory) - 1;
                        }
                        directory[end_index] = '\0';
                        end_index--;

                        do{
                            if( directory[end_index] == '/' ||
                                directory[end_index] == '\\' ){
                                break;
                            }

                            directory[end_index] = '\0';
                            end_index--;
                        }while( end_index != 0 );

                        directory[end_index] = '.';
                        directory[end_index+1] = '/';
                    }else if( strcmp(current_file_item->file_name, ".") == 0 ){
                        // do nothing
                    }else{
                        strcat(directory, current_file_item->file_name);
                        strcat(directory, "/");
                    }
                    return GUI_RESULT_NONE;
                }
                return GUI_RESULT_OKAY;
            }
        }

        if( gui_do_button( state,
                           "CLOSE",
                           gui_layout_next_x( state ),
                           gui_layout_next_y( state ) ) ){
            if( opened ){
                *opened = false;
            }

            return GUI_RESULT_CANCEL;
        }
        gui_end_layout( state );
    }else{
        return GUI_RESULT_NONE;
    }

    return GUI_RESULT_NONE;
}

b32 gui_do_text_button( gui_state* state, char* text, f32 x, f32 y ){
    f32 w;
    f32 h;

    f32 size;

    renderer_get_text_size( state->renderer, state->font_texture, text, &w, &h, &size );

    gui_widget* current_widget = gui_internal_button_push( state, text, x, y, w, h );
    bool mouse_touching = gui_internal_button_touching_check( state, current_widget, x, y, w, h );

    {
        render_command button_text = {};

        float button_r = 1;
        float button_g = 1;
        float button_b = 1;

        gui_internal_button_change_color_on_highlight( state, mouse_touching, &button_r, &button_g, &button_b, NULL );

        // crashes if I use current_widget->button.text.... HMMMMM
        button_text = render_command_text( text, 
                                           state->font_texture, 
                                           x, y, 1.0f, 
                                           button_r, button_g, button_b, 1.00f);
        render_layer_push_command( state->layer, button_text );
    }

    return gui_internal_button_clicked_check( state, current_widget, mouse_touching );
}

void gui_do_check_box_button( gui_state* state, b32* target, f32 x, f32 y ){
    const f32 w = 20;
    const f32 h = 20;
    gui_widget* current_widget = gui_internal_checkbox_button_push( state, x, y, w, h );

    render_command button_box = {};
    button_box = render_command_quad( x, y, w, h, 0.1, 0.1, 0.1, 1.0 );

    render_layer_push_command( state->layer, button_box );

    current_widget->checkbox_button.toggled = *target;

    gui_internal_checkbox_button_on_click( state, current_widget );

    // change on delta
    if( current_widget->checkbox_button.toggled != *target ){
        *target = current_widget->checkbox_button.toggled;
    }

    if( current_widget->checkbox_button.toggled ){
        render_command toggled_box = {};
        const f32 toggled_r = 1.0f;
        const f32 toggled_g = 0.8f;
        const f32 toggled_b = 0.3f;
        toggled_box = render_command_quad(
                x+w/4, 
                y+h/4,
                w/2,
                h/2, 
                toggled_r,
                toggled_g,
                toggled_b, 
                1.0 );

        render_layer_push_command( state->layer, toggled_box );
    }
}

static void gui_internal_pop_layout( gui_state* state ){
    state->gui_layout_stack_top--;
}

static gui_widget* gui_internal_radio_button_push( gui_state* state,
                                                   char* text,
                                                   f32 x,
                                                   f32 y,
                                                   f32 w,
                                                   f32 h, 
                                                   i32 group_id ){
    gui_internal_layout_inform_element_size( state, w, h );
    gui_internal_pushed_widget( state );

    gui_widget* current_widget = &state->widgets[state->widget_index];
    {
        if( current_widget->type != GUI_WIDGET_RADIO_BUTTON ){
            current_widget->type = GUI_WIDGET_RADIO_BUTTON;
            current_widget->radio_button.text = text;
        }

        current_widget->radio_button.x = x;
        current_widget->radio_button.y = y;
        current_widget->radio_button.w = w;
        current_widget->radio_button.h = h;

        {
            gui_radio_button_group* target_group =
                &state->radio_groups[group_id];
            current_widget->radio_button.id_in_group = 
                target_group->next_id++;
        }
        current_widget->id = state->widget_index;
    }

    return current_widget;
}

static gui_widget* gui_internal_draggable_push( gui_state* state,
                                                char* text,
                                                f32 x,
                                                f32 y,
                                                f32 w,
                                                f32 h ){
    gui_widget* current_widget = &state->widgets[state->widget_index];

    if( current_widget->type != GUI_WIDGET_DRAGGABLE ){
        current_widget->type = GUI_WIDGET_DRAGGABLE;

        current_widget->draggable.text = text;

        current_widget->id = state->widget_index;
    }

    current_widget->draggable.x = x;
    current_widget->draggable.y = y;
    current_widget->draggable.w = w;
    current_widget->draggable.h = h;

    return current_widget;
}

static void gui_internal_draggable_check_for_focus( gui_state* state,
                                                    gui_widget* draggable_widget ){
    bool mouse_touching = 
        // rename to generic rectangle check or something.
        gui_internal_button_touching_check( state,
                                            draggable_widget,
                                            draggable_widget->draggable.x,
                                            draggable_widget->draggable.y,
                                            draggable_widget->draggable.w,
                                            draggable_widget->draggable.h );

    if( mouse_touching && state->last_mouse_frame_clicked ){
        draggable_widget->draggable.focused = true;
    }else{
        if( draggable_widget->draggable.focused && !state->input->relative_mouse_mode ){
            draggable_widget->draggable.focused = false;
            platform_move_mouse( state->last_mouse_frame_x,
                                 state->last_mouse_frame_y );
            fprintf(stderr, "Reset position of cursor to : %d, %d\n",
                    state->last_mouse_frame_x,
                    state->last_mouse_frame_y);
        }
    }
}

static void gui_internal_draggable_vec2_update_new_starts( gui_widget* draggable,
                                                           f32 x,
                                                           f32 y ){
    draggable->draggable.start_data.vec2_data.x = x;
    draggable->draggable.start_data.vec2_data.y = y;
}

static gui_widget* gui_internal_draggable_vec2_push( gui_state* state,
                                                     char* text,
                                                     f32 x,
                                                     f32 y,
                                                     f32 w,
                                                     f32 h,
                                                     f32 start_data_x,
                                                     f32 start_data_y){
    gui_internal_layout_inform_element_size( state, w, h );
    gui_internal_pushed_widget( state );

    gui_widget* current_widget = gui_internal_draggable_push( state, text, x, y, w, h );
    {

        if( current_widget->draggable.type != GUI_DRAGGABLE_VEC2 ){
            current_widget->draggable.type = GUI_DRAGGABLE_VEC2;
        }
        gui_internal_draggable_vec2_update_new_starts( current_widget,
                                                       start_data_x,
                                                       start_data_y );
    }

    return current_widget;
}

static void gui_internal_draggable_float_update_new_start( gui_widget* draggable,
                                                           f32 data ){
    draggable->draggable.start_data.float_data = data;
}

static gui_widget* gui_internal_draggable_float_push( gui_state* state,
                                                      char* text,
                                                      f32 x,
                                                      f32 y,
                                                      f32 w,
                                                      f32 h,
                                                      f32 start_data ){
    gui_internal_layout_inform_element_size( state, w, h );
    gui_internal_pushed_widget( state );

    gui_widget* current_widget = gui_internal_draggable_push( state, text, x, y, w, h );

    {
        if( current_widget->draggable.type != GUI_DRAGGABLE_FLOAT ){
            current_widget->draggable.type = GUI_DRAGGABLE_FLOAT;
        }
        gui_internal_draggable_float_update_new_start( current_widget, start_data );
    }

    return current_widget;
}

void gui_do_draggable_float( gui_state* state,
                             char* text,
                             f32 x,
                             f32 y,
                             f32* target,
                             f32 min,
                             f32 max ){
    f32 w;
    f32 h;

    f32 size;

    renderer_get_text_size( state->renderer,
            state->font_texture,
            text, &w, &h, &size );

    w *= 1.3;
    h *= 1.25;

    gui_widget* current_widget = gui_internal_draggable_float_push( state, text,
                                                                    x, y, w, h,
                                                                    *target );

    render_command button_box = {};

    f32 r = 0.1;
    f32 g = 0.1;
    f32 b = 0.1;

    f32 text_r = 1.0;
    f32 text_g = 1.0;
    f32 text_b = 1.0;

    gui_internal_draggable_check_for_focus( state, current_widget );
    if( current_widget->draggable.focused ){
        state->input->relative_mouse_mode = true;
        r = 1.0f;
        g = 0.8f;
        b = 0.3f;

        text_r = 0.0;
        text_g = 0.0;
        text_b = 0.0;

        {
            f32 mouse_delta_x = state->input->current.mouse.delta_x;

            current_widget->draggable.start_data.float_data += ( mouse_delta_x * 0.0015f );
            current_widget->draggable.start_data.float_data =
            f32_clamp( current_widget->draggable.start_data.float_data, min, max );
            (*target) = current_widget->draggable.start_data.float_data;
        }
    }

    button_box = render_command_quad( x, y, w, h, r, g, b, 1.0 );

    {
        render_command button_text = {};
        button_text = render_command_text_justified( text, 
                state->font_texture,
                1.0f,
                text_r,
                text_g,
                text_b,
                1.00f,
                x,
                y,
                w,
                h,
                TEXT_COMMAND_JUSTIFICATION_CENTER );

        render_layer_push_command( state->layer, button_box );
        render_layer_push_command( state->layer, button_text );
    }
}

void gui_do_draggable_vec2( gui_state* state,
                            char* text,
                            f32 x,
                            f32 y,
                            f32* target_x,
                            f32* target_y,
                            // TODO(jerry): use these???
                            f32 min_x,
                            f32 max_x,
                            f32 min_y,
                            f32 max_y){
    f32 w;
    f32 h;

    f32 size;

    renderer_get_text_size( state->renderer,
            state->font_texture,
            text, &w, &h, &size );

    w *= 1.3;
    h *= 1.25;

    gui_widget* current_widget = gui_internal_draggable_vec2_push( state, 
            text, x, y, w, h, *target_x, *target_y );

    render_command button_box = {};

    f32 r = 0.1;
    f32 g = 0.1;
    f32 b = 0.1;

    f32 text_r = 1.0;
    f32 text_g = 1.0;
    f32 text_b = 1.0;

    gui_internal_draggable_check_for_focus( state, current_widget );
    if( current_widget->draggable.focused ){
        state->input->relative_mouse_mode = true;
        r = 1.0f;
        g = 0.8f;
        b = 0.3f;

        text_r = 0.0;
        text_g = 0.0;
        text_b = 0.0;

        {
            f32 mouse_delta_x = state->input->current.mouse.delta_x;
            f32 mouse_delta_y = state->input->current.mouse.delta_y;

            current_widget->draggable.start_data.vec2_data.x += ( mouse_delta_x * 0.0015f );
            current_widget->draggable.start_data.vec2_data.y += ( mouse_delta_y * 0.0015f );

            current_widget->draggable.start_data.vec2_data.x =
            f32_clamp( current_widget->draggable.start_data.vec2_data.x, min_x, max_x );
            current_widget->draggable.start_data.vec2_data.y =
            f32_clamp( current_widget->draggable.start_data.vec2_data.y, min_y, max_y );

            (*target_x) = current_widget->draggable.start_data.vec2_data.x;
            (*target_y) = current_widget->draggable.start_data.vec2_data.y;
        }
    }

    button_box = render_command_quad( x, y, w, h, r, g, b, 1.0 );

    {
        render_command button_text = {};
        button_text = render_command_text_justified( text, 
                state->font_texture,
                1.0f,
                text_r,
                text_g,
                text_b,
                1.00f,
                x,
                y,
                w,
                h,
                TEXT_COMMAND_JUSTIFICATION_CENTER );

        render_layer_push_command( state->layer, button_box );
        render_layer_push_command( state->layer, button_text );
    }
}

static gui_widget* gui_internal_text_line_edit_push( gui_state* state, 
                                                     f32 x, 
                                                     f32 y,
                                                     f32 w,
                                                     f32 h ){
    gui_internal_layout_inform_element_size( state, w, h );
    gui_internal_pushed_widget( state );

    gui_widget* current_widget = &state->widgets[state->widget_index];
    {
        if( current_widget->type != GUI_WIDGET_TEXTLINE_EDIT ){
            current_widget->type = GUI_WIDGET_TEXTLINE_EDIT;

            // blank state.
            current_widget->line_edit.focused = false;

            current_widget->id = state->widget_index;
        }

        current_widget->line_edit.x = x;
        current_widget->line_edit.y = y;
        current_widget->line_edit.w = w;
        current_widget->line_edit.h = h;
    }

    return current_widget;
}

// TODO(jerry):
// This isn't so great right now and needs to be fixed pretty
// imminently...
// does not have good resize visuals
void gui_do_textline_edit( gui_state* state,
                           char* target_text,
                           u64 target_max_length,
                           f32 x,
                           f32 y ){
    f32 w;
    f32 h;

    f32 size;

    renderer_get_text_size( state->renderer, state->font_texture, target_text, &w, &h, &size );
    gui_widget* current_widget = gui_internal_text_line_edit_push( state, x, y, w, h + 15 );

    render_command line_edit_box = {};

    f32 r = 0.2;
    f32 g = 0.2;
    f32 b = 0.2;

    f32 text_r = 1.0;
    f32 text_g = 1.0;
    f32 text_b = 1.0;

    bool mouse_touching = 
        // rename to generic rectangle check or something.
        gui_internal_button_touching_check( state, current_widget, x, y, w, h );

    {
        if( state->last_mouse_frame_clicked ){
            if( mouse_touching ){
                current_widget->line_edit.focused = true;
                state->focused_widget_index = current_widget->id;
                input_begin_text_edit( state->input );
                u64 length_of_string;
                char* ptr = target_text;
                {
                    while( *(ptr++) );
                    length_of_string = ptr - target_text;
                }
                state->input->text_edit.cursor = length_of_string-1;
            }else{
                current_widget->line_edit.focused = false;
            }
        }
    } 

    if( input_is_key_pressed( state->input, INPUT_KEY_RETURN ) ){
        current_widget->line_edit.focused = false;
    }

    if( current_widget->line_edit.focused && 
        state->input->text_edit.cursor < target_max_length ){
        r = 1.0f;
        g = 0.8f;
        b = 0.3f;

        text_r = 0.0;
        text_g = 0.0;
        text_b = 0.0;

        strncat( target_text, state->input->text_edit.text, 32 );
        target_text[state->input->text_edit.cursor] = 0;
    }

    line_edit_box = render_command_quad( x, y, w, h, r, g, b, 1.0 );

    {
        render_command edit_text = {};
        edit_text = render_command_text_justified( target_text, 
                state->font_texture,
                1.0f,
                text_r,
                text_g,
                text_b,
                1.00f,
                x,
                y,
                w,
                h,
                TEXT_COMMAND_JUSTIFICATION_LEFT | TEXT_COMMAND_JUSTIFICATION_CENTER );

        render_layer_push_command( state->layer, line_edit_box );
        render_layer_push_command( state->layer, edit_text );
    }
}
// bad linear scan, fast enough I guess.
static void gui_internal_radio_button_notify_all_in_group( gui_state* state,
                                                           i32 group_id,
                                                           i32 new_selected_id ){
    state->radio_groups[group_id].focused_id = new_selected_id;
}

static bool gui_internal_radio_button_is_selected_button( gui_state* state,
                                                          i32 group_id,
                                                          i32 id ){
    return (state->radio_groups[group_id].focused_id == id);
}

void gui_do_radio_button( gui_state* state, 
                          char* text,
                          f32 x,
                          f32 y,
                          i32* target,
                          i32 group_id ){
    f32 w;
    f32 h;

    f32 size;

    renderer_get_text_size( state->renderer,
            state->font_texture,
            text, &w, &h, &size );

    w *= 1.3;
    h *= 1.25;

    render_command button_box = {};
    button_box = render_command_quad( x, y, w, h, 0.1, 0.1, 0.1, 1.0 );

    gui_widget* current_widget = gui_internal_radio_button_push( state, text, x, y, w, h, group_id );
    bool mouse_touching = gui_internal_button_touching_check( state, current_widget, x, y, w, h );
    
    {
        render_command button_hole = {};
        button_hole = render_command_quad( x + 5, y + 5, 20, 20, 0, 0, 0, 1 );

        render_command button_text = {};
        button_text = render_command_text_justified( text, 
                state->font_texture,
                1.0f,
                1,
                1,
                1,
                1.00f,
                x,
                y,
                w,
                h,
                TEXT_COMMAND_JUSTIFICATION_RIGHT | TEXT_COMMAND_JUSTIFICATION_CENTER );

        render_layer_push_command( state->layer, button_box );
        render_layer_push_command( state->layer, button_hole );
        render_layer_push_command( state->layer, button_text );
        
        // this is not safe because this
        // depends on me not changing the unions
        // very much because all the buttons are
        // layed out in the exact same way so I can
        // use the same procedure on all of them.
        //
        // Like a poor man's polymorphism.
        b32 was_clicked = 
            gui_internal_button_clicked_check( state, current_widget, mouse_touching );

        if( was_clicked ){
            gui_internal_radio_button_notify_all_in_group( state, 
                    group_id, current_widget->radio_button.id_in_group );
        }

        b32 is_selected_button = 
            gui_internal_radio_button_is_selected_button( state,
                    group_id, current_widget->radio_button.id_in_group );

        if( is_selected_button ){
            render_command button_selected = {};

            button_selected = render_command_quad( x + 10, y + 10, 10, 10, 0, 1, 0, 1 );
            render_layer_push_command( state->layer, button_selected );

            *target = current_widget->radio_button.id_in_group;
        }
    }
}

bool gui_do_collapsable_group( gui_state* state, char* group_name, b32* opened, f32 x, f32 y ){
    if( gui_do_button( state, group_name, x, y ) ){
        *opened ^= 1;
    }

    return *opened;
}

static void gui_internal_push_layout( gui_state* state, gui_layout layout ){
    // allows nested layouts
    gui_internal_recalculate_current_layout( state );

    layout.next_x = layout.start_x;
    layout.next_y = layout.start_y;

    state->layout_stack[state->gui_layout_stack_top] = layout;
    state->gui_layout_stack_top++;
}

void gui_begin_fixed_horizontal_layout( gui_state* state, 
                                        f32 start_x, 
                                        f32 start_y, 
                                        f32 horizontal_element_width, 
                                        f32 margin_left, 
                                        f32 margin_right ){
    gui_layout horizontal_layout = {};

    horizontal_layout.layout_type = GUI_LAYOUT_FIXED_HORIZONTAL;

    horizontal_layout.start_x = start_x;
    horizontal_layout.start_y = start_y;

    horizontal_layout.horizontal.horizontal_element_width = horizontal_element_width;
    horizontal_layout.horizontal.margin_left = margin_left;
    horizontal_layout.horizontal.margin_right = margin_right;

    gui_internal_push_layout( state, horizontal_layout );
}

void gui_begin_fixed_vertical_layout( gui_state* state, 
                                      f32 start_x, 
                                      f32 start_y,
                                      f32 vertical_element_height,
                                      f32 margin_top,
                                      f32 margin_bottom ){
    gui_layout vertical_layout = {};

    vertical_layout.layout_type = GUI_LAYOUT_FIXED_VERTICAL;

    vertical_layout.start_x = start_x;
    vertical_layout.start_y = start_y;

    vertical_layout.vertical.vertical_element_height = vertical_element_height;
    vertical_layout.vertical.margin_top = margin_top;
    vertical_layout.vertical.margin_bottom = margin_bottom;

    gui_internal_push_layout( state, vertical_layout );
}

void gui_begin_grid_layout( gui_state* state, 
                            f32 start_x, 
                            f32 start_y, 
                            u32 grid_w, 
                            u32 grid_h, 
                            f32 element_width, 
                            f32 element_height, 
                            f32 margin_right, 
                            f32 margin_top ){
    gui_layout grid_layout = {};

    grid_layout.layout_type = GUI_LAYOUT_GRID;

    grid_layout.start_x = start_x;
    grid_layout.start_y = start_y;

    grid_layout.grid.grid_w = grid_w;
    grid_layout.grid.grid_h = grid_h;

    grid_layout.grid.element_width = element_width;
    grid_layout.grid.element_height = element_height;

    grid_layout.grid.margin_right = margin_right;
    grid_layout.grid.margin_top = margin_top;

    gui_internal_push_layout( state, grid_layout );
}

void gui_begin_horizontal_layout( gui_state* state, 
                                  f32 start_x, 
                                  f32 start_y, 
                                  f32 margin_left, 
                                  f32 margin_right ){
    gui_layout horizontal_layout = {};

    horizontal_layout.layout_type = GUI_LAYOUT_HORIZONTAL;

    horizontal_layout.start_x = start_x;
    horizontal_layout.start_y = start_y;

    horizontal_layout.horizontal.margin_left = margin_left;
    horizontal_layout.horizontal.margin_right = margin_right;

    gui_internal_push_layout( state, horizontal_layout );
}

void gui_begin_vertical_layout( gui_state* state, 
                                f32 start_x, 
                                f32 start_y,
                                f32 margin_top,
                                f32 margin_bottom ){
    gui_layout vertical_layout = {};

    vertical_layout.layout_type = GUI_LAYOUT_VERTICAL;

    vertical_layout.start_x = start_x;
    vertical_layout.start_y = start_y;

    vertical_layout.vertical.margin_top = margin_top;
    vertical_layout.vertical.margin_bottom = margin_bottom;

    gui_internal_push_layout( state, vertical_layout );
}

f32 gui_layout_next_x( gui_state* state ){
    gui_layout* top_layout = &state->layout_stack[state->gui_layout_stack_top-1];

    return top_layout->next_x;
}

f32 gui_layout_next_y( gui_state* state ){
    gui_layout* top_layout = &state->layout_stack[state->gui_layout_stack_top-1];

    return top_layout->next_y;
}

void gui_end_layout( gui_state* state ){
    gui_internal_pop_layout( state );
}

/*Add bounds checking?*/
void gui_layout_vertical_padding( gui_state* state, f32 padding ){
    gui_layout* top_layout = &state->layout_stack[state->gui_layout_stack_top-1];
    if( top_layout->layout_type < GUI_LAYOUT_AUTO_PADDED ){
        top_layout->next_y += padding;
    }
}

void gui_layout_horizontal_padding( gui_state* state, f32 padding ){
    gui_layout* top_layout = &state->layout_stack[state->gui_layout_stack_top-1];

    if( top_layout->layout_type < GUI_LAYOUT_AUTO_PADDED ){
        top_layout->next_x += padding;
    }
}
