/*
 * TODO(jerry):
 *
 * Improve the text edit interface....
 *
 */
#include "input.h"

int32_t input_get_key_mods( game_input* input, uint8_t keycode ){
    return input->current.keys[ keycode ].modifiers;
}

int32_t input_is_key_down( game_input* input, uint8_t keycode ){
    return input->current.keys[ keycode ].down;
}

int32_t input_is_key_pressed( game_input* input, uint8_t keycode ){
    return input->current.keys[ keycode ].pressed;
}

int32_t input_is_mouse_left_down( game_input* input ){
    return input->current.mouse.left;
}

int32_t input_is_mouse_middle_down( game_input* input ){
    return input->current.mouse.middle;
}

int32_t input_is_mouse_right_down( game_input* input ){
    return input->current.mouse.right;
}   

// NOTE(jerry): weird.
int32_t input_is_mouse_left_click( game_input* input ){
    return input->current.mouse.left_clicked;
}

int32_t input_is_mouse_middle_click( game_input* input ){
    return input->current.mouse.middle_clicked;
}

int32_t input_is_mouse_right_click( game_input* input ){
    return input->current.mouse.right_clicked;
}

void input_begin_text_edit( game_input* input ){
    input->text_edit_mode = true;
    input->text_edit.cursor = 0;
}

void input_end_text_edit( game_input* input ){
    input->text_edit_mode = false;
    input->text_edit.cursor = 0;
}

void input_update( game_input* input ){
    /*mouse_click*/
    {
        /*mouse_left*/
        {
            bool last_left_down = input->last.mouse.left;
            bool left_down = input->current.mouse.left;

            if( last_left_down == left_down ){
                input->current.mouse.left_clicked = false;
            }else if( !last_left_down && left_down ){
                input->current.mouse.left_clicked = false;
            }else if( last_left_down && !left_down ){
                input->current.mouse.left_clicked = true;
            }
        }

        /*mouse_middle*/
        {
            bool last_middle_down = input->last.mouse.middle;
            bool middle_down = input->current.mouse.middle;

            if(last_middle_down == middle_down){
                input->current.mouse.middle_clicked = false;
            }else if( !last_middle_down && middle_down ){
                input->current.mouse.middle_clicked = false;
            }else if( last_middle_down && !middle_down ){
                input->current.mouse.middle_clicked = true;
            }
        }

        /*mouse right*/
        {
            bool last_right_down = input->last.mouse.right;
            bool right_down = input->current.mouse.right;

            if( last_right_down == right_down ){
                input->current.mouse.right_clicked = false;
            }else if( !last_right_down && right_down ){
                input->current.mouse.right_clicked = false;
            }else if( last_right_down && !right_down ){
                input->current.mouse.right_clicked = true;
            }
        }
    }
    /*key_click*/
    {
        for( uint64_t keycode = 0;
             keycode < INPUT_KEY_COUNT;
             ++keycode ){
            bool last_key_down = input->last.keys[ keycode ].down;
            bool key_down = input->current.keys[ keycode ].down;

            if( last_key_down && key_down ){
                input->current.keys[ keycode ].pressed = false;
            }else if( !last_key_down && key_down ){
                input->current.keys[ keycode ].pressed = true;
            }else if( last_key_down && !key_down ){
                input->current.keys[ keycode ].pressed = false;
            }
        }
    }

    input->last = input->current;
}

int32_t input_get_mouse_x( game_input* input ){
    return input->current.mouse.x;
}

int32_t input_get_mouse_y( game_input* input ){
    return input->current.mouse.y;
}

int32_t input_get_relative_mouse_x( game_input* input ){
    return input->current.mouse.delta_x;
}

int32_t input_get_relative_mouse_y( game_input* input ){
    return input->current.mouse.delta_y;
}
