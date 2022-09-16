#ifndef GAME_INPUT_H
#define GAME_INPUT_H

#include <stdbool.h>
#include <stdint.h>

#if 0
#pragma message "Please actually make sure you implement this entire set of keycodes...."
#pragma message "Don't forget about scan codes!"
#pragma message "The function keys are also missing..."
#pragma message "Missing a few keys... PrntScreen, PauseBreak etc..."
#endif

enum keycodes{
    INPUT_KEY_UNKNOWN,

    INPUT_KEY_A,
    INPUT_KEY_B,
    INPUT_KEY_C,
    INPUT_KEY_D,
    INPUT_KEY_E,
    INPUT_KEY_F,
    INPUT_KEY_G,
    INPUT_KEY_H,
    INPUT_KEY_I,
    INPUT_KEY_J,
    INPUT_KEY_K,
    INPUT_KEY_L,
    INPUT_KEY_M,
    INPUT_KEY_N,
    INPUT_KEY_O,
    INPUT_KEY_P,
    INPUT_KEY_Q,
    INPUT_KEY_R,
    INPUT_KEY_S,
    INPUT_KEY_T,
    INPUT_KEY_U,
    INPUT_KEY_V,
    INPUT_KEY_W,
    INPUT_KEY_X,
    INPUT_KEY_Y,
    INPUT_KEY_Z,

    INPUT_KEY_UP,
    INPUT_KEY_DOWN,
    INPUT_KEY_RIGHT,
    INPUT_KEY_LEFT,

    INPUT_KEY_0,
    INPUT_KEY_1,
    INPUT_KEY_2,
    INPUT_KEY_3,
    INPUT_KEY_4,
    INPUT_KEY_5,
    INPUT_KEY_6,
    INPUT_KEY_7,
    INPUT_KEY_8,
    INPUT_KEY_9,

    INPUT_KEY_MINUS,
    INPUT_KEY_TILDE,
    INPUT_KEY_EQUALS,
    INPUT_KEY_SEMICOLON,
    INPUT_KEY_QUOTE,
    INPUT_KEY_COMMA,
    INPUT_KEY_PERIOD,

    INPUT_KEY_RETURN,
    INPUT_KEY_BACKSPACE,
    INPUT_KEY_ESCAPE,

    INPUT_KEY_INSERT,
    INPUT_KEY_HOME,
    INPUT_KEY_PAGEUP,
    INPUT_KEY_PAGEDOWN,
    INPUT_KEY_DELETE,
    INPUT_KEY_END,

    INPUT_KEYPAD_0,
    INPUT_KEYPAD_1,
    INPUT_KEYPAD_2,
    INPUT_KEYPAD_3,
    INPUT_KEYPAD_4,
    INPUT_KEYPAD_5,
    INPUT_KEYPAD_6,
    INPUT_KEYPAD_7,
    INPUT_KEYPAD_8,
    INPUT_KEYPAD_9,

    INPUT_KEYPAD_LEFT,
    INPUT_KEYPAD_RIGHT,
    INPUT_KEYPAD_UP,
    INPUT_KEYPAD_DOWN,

    INPUT_KEYPAD_ASTERISK,
    INPUT_KEYPAD_BACKSLASH,
    INPUT_KEYPAD_MINUS,
    INPUT_KEYPAD_PLUS,
    INPUT_KEYPAD_PERIOD,

    INPUT_KEY_LEFT_BRACKET,
    INPUT_KEY_RIGHT_BRACKET,
    INPUT_KEY_FORWARDSLASH,
    INPUT_KEY_BACKSLASH,

    INPUT_KEY_TAB,
    INPUT_KEY_LSHIFT,
    INPUT_KEY_RSHIFT,

    INPUT_KEY_META,
    INPUT_KEY_SUPER,
    INPUT_KEY_SPACE,

    INPUT_KEY_LALT,
    INPUT_KEY_RALT,

    INPUT_LCTRL,
    INPUT_RCTRL,

    INPUT_KEY_COUNT
};

enum key_press_state{
    KEY_NOCHANGE,
    KEY_PRESSED,
    KEY_RELEASED,

    KEY_PRESS_STATES
};

typedef struct mouse_state{  
    bool left;
    bool middle;
    bool right;

    bool left_clicked;
    bool middle_clicked;
    bool right_clicked;

    // only updated during relative mode.
    int32_t delta_x;
    int32_t delta_y;

    int32_t x;
    int32_t y;
}mouse_state;

typedef struct input_key_state{
    uint8_t down;
    uint8_t pressed;
    uint8_t modifiers;
}input_key_state;

typedef struct text_input_state{
    char text[32];

    int cursor;
}text_input_state;

typedef struct game_input_state{
    input_key_state keys[ INPUT_KEY_COUNT ];
    mouse_state mouse;
}game_input_state;

typedef struct game_input{
    game_input_state last;
    game_input_state current;

    text_input_state text_edit;
    bool text_edit_mode;
    bool relative_mouse_mode;
}game_input;

/*All of these poll from the current state. Last state is manual.*/
int32_t input_get_key_mods( game_input* input, uint8_t keycode );
int32_t input_is_key_down( game_input* input, uint8_t keycode );
int32_t input_is_key_pressed( game_input* input, uint8_t keycode );

int32_t input_is_mouse_left_down( game_input* input );
int32_t input_is_mouse_middle_down( game_input* input );
int32_t input_is_mouse_right_down( game_input* input );

int32_t input_is_mouse_left_click( game_input* input );
int32_t input_is_mouse_middle_click( game_input* input );
int32_t input_is_mouse_right_click( game_input* input );

int32_t input_get_mouse_x( game_input* input );
int32_t input_get_mouse_y( game_input* input );

int32_t input_get_relative_mouse_x( game_input* input );
int32_t input_get_relative_mouse_y( game_input* input );

// This is for the button clicks primarily.
void input_update( game_input* input );
void input_begin_text_edit( game_input* input );
void input_end_text_edit( game_input* input );

#endif
