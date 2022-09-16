/*
 * NOTE(jerry):
 * 
 * This is not platform_linux.c
 *
 * because technically linux doesn't have a proper...
 * Platform per say? It's really more SDL as the platform...
 */

#include <SDL2/SDL.h>

#include "input.h"

#include "renderer.h"

#include "common.h"
#include "wanderer.h"

#include "bmp_loader.h"
#include "opengl_functions.h"
#include "opengl_functions.c"

#include "platform.h"

#include "memory_pool.h"

static bool running = true;

#define virtual_key_case( key, mapped ) \
    case key:\
    {\
        return mapped;\
    }\
    break

static i32 map_sdl2_key_to_input_key( i32 sdl_vk_code ){
    switch( sdl_vk_code ){
        virtual_key_case( SDLK_ESCAPE, INPUT_KEY_ESCAPE );

        virtual_key_case( SDLK_UP, INPUT_KEY_UP );
        virtual_key_case( SDLK_LEFT, INPUT_KEY_LEFT );
        virtual_key_case( SDLK_RIGHT, INPUT_KEY_RIGHT );
        virtual_key_case( SDLK_DOWN, INPUT_KEY_DOWN );

        virtual_key_case( SDLK_0, INPUT_KEY_0 );
        virtual_key_case( SDLK_1, INPUT_KEY_1 );
        virtual_key_case( SDLK_2, INPUT_KEY_2 );
        virtual_key_case( SDLK_3, INPUT_KEY_3 );
        virtual_key_case( SDLK_4, INPUT_KEY_4 );
        virtual_key_case( SDLK_5, INPUT_KEY_5 );
        virtual_key_case( SDLK_6, INPUT_KEY_6 );
        virtual_key_case( SDLK_7, INPUT_KEY_7 );
        virtual_key_case( SDLK_8, INPUT_KEY_8 );
        virtual_key_case( SDLK_9, INPUT_KEY_9 );

        virtual_key_case( SDLK_a, INPUT_KEY_A );
        virtual_key_case( SDLK_b, INPUT_KEY_B );
        virtual_key_case( SDLK_c, INPUT_KEY_C );
        virtual_key_case( SDLK_d, INPUT_KEY_D );
        virtual_key_case( SDLK_e, INPUT_KEY_E );
        virtual_key_case( SDLK_f, INPUT_KEY_F );
        virtual_key_case( SDLK_g, INPUT_KEY_G );
        virtual_key_case( SDLK_h, INPUT_KEY_H );
        virtual_key_case( SDLK_i, INPUT_KEY_I );
        virtual_key_case( SDLK_j, INPUT_KEY_J );
        virtual_key_case( SDLK_k, INPUT_KEY_K );
        virtual_key_case( SDLK_l, INPUT_KEY_L );
        virtual_key_case( SDLK_m, INPUT_KEY_M );
        virtual_key_case( SDLK_n, INPUT_KEY_N );
        virtual_key_case( SDLK_o, INPUT_KEY_O );
        virtual_key_case( SDLK_p, INPUT_KEY_P );
        virtual_key_case( SDLK_q, INPUT_KEY_Q );
        virtual_key_case( SDLK_r, INPUT_KEY_R );
        virtual_key_case( SDLK_s, INPUT_KEY_S );
        virtual_key_case( SDLK_t, INPUT_KEY_T );
        virtual_key_case( SDLK_u, INPUT_KEY_U );
        virtual_key_case( SDLK_v, INPUT_KEY_V );
        virtual_key_case( SDLK_w, INPUT_KEY_W );
        virtual_key_case( SDLK_x, INPUT_KEY_X );
        virtual_key_case( SDLK_y, INPUT_KEY_Y );
        virtual_key_case( SDLK_z, INPUT_KEY_Z );

        virtual_key_case( SDLK_RETURN, INPUT_KEY_RETURN );

        virtual_key_case( SDLK_LSHIFT, INPUT_KEY_LSHIFT );
        virtual_key_case( SDLK_RSHIFT, INPUT_KEY_RSHIFT );

        virtual_key_case( SDLK_LALT, INPUT_KEY_LALT );
        virtual_key_case( SDLK_RALT, INPUT_KEY_RALT );

        virtual_key_case( SDLK_BACKSLASH, INPUT_KEY_BACKSLASH );
        virtual_key_case( SDLK_SLASH, INPUT_KEY_FORWARDSLASH );
        virtual_key_case( SDLK_COMMA, INPUT_KEY_COMMA );
        virtual_key_case( SDLK_BACKSPACE, INPUT_KEY_BACKSPACE );
        virtual_key_case( SDLK_SPACE, INPUT_KEY_SPACE );
        virtual_key_case( SDLK_SEMICOLON, INPUT_KEY_SEMICOLON );
        virtual_key_case( SDLK_TAB, INPUT_KEY_TAB );
        virtual_key_case( SDLK_PERIOD, INPUT_KEY_PERIOD );
#if 0
#pragma message "SDL2 Platform layer requires Some other non-letter keys... Ctrl Alt, Super, Meta..."
#pragma message "SDL2 Platform layer missing keycodes for &*@^:, basically the shift modified things."
#pragma message "SDL2 Platform layer requires keypad and also the insert, home stuff...."
#endif
    }

    return INPUT_KEY_UNKNOWN;
}

#undef virtual_key_case

void* get_gl_function( const char* proc_name ){
    void* gl_func = (void*) SDL_GL_GetProcAddress( proc_name );

    return gl_func;
}

static void platform_show_cursor( void ){
    SDL_ShowCursor( SDL_ENABLE );
}

static void platform_hide_cursor( void ){
    SDL_ShowCursor( SDL_DISABLE );
}

void platform_move_mouse( int x, int y ){
    SDL_SetRelativeMouseMode( SDL_FALSE );
    SDL_WarpMouseInWindow(NULL, x, y);
}

#if 0
#include "string_test.c"
#else
int main( int argc, char** argv ){
    SDL_Init( SDL_INIT_VIDEO | 
              SDL_INIT_AUDIO );

    SDL_Window* window;
    SDL_GLContext gl_context;

    fprintf(stderr, "sizeof(game_input): %lld\n", sizeof(game_input));
    fprintf(stderr, "sizeof(game_state): %lld\n", sizeof(game_state));
    fprintf(stderr, "sizeof(renderer): %lld\n", sizeof(renderer));

    u64 memory_pool_size = 
        sizeof(game_input) + sizeof(game_state) + sizeof(renderer) + MB(8);

    memory_pool main_memory = {};
    memory_pool_init( &main_memory, memory_pool_size );

    // why does the renderer have to be last?
    // this might have been a mistake made really early on
    // I cannot notice it though.... s**t.
    game_input* input  = memory_pool_allocate( &main_memory, sizeof(game_input) );
    game_state* state  = memory_pool_allocate( &main_memory, sizeof(game_state) );
    renderer* renderer = memory_pool_allocate( &main_memory, sizeof(renderer) );

    state->scratch_memory = (void*)(main_memory.memory) + (memory_pool_size - MB(8));

    platform_hide_cursor();

    window = SDL_CreateWindow( "Wanderer [SDL2]",
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               1440, 900,
                               SDL_WINDOW_OPENGL | 
                               SDL_WINDOW_SHOWN );
    // get an old opengl context.
    {
        SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
        SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
        SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
        SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );

        SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
        SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );

        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 1 );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 4 );

        SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, 0 );
    }

    gl_context = SDL_GL_CreateContext( window );

    init_gl_functions();

    u64 start = SDL_GetTicks();
    u64 end = SDL_GetTicks();
    u64 elapsed = 0;

    // should be done once at least before trying anything else.
    SDL_GL_GetDrawableSize( window,
            &state->screen_width,
            &state->screen_height );

    while( running && state->mode != GAME_STATE_QUIT ){
        SDL_Event event;    

        start = SDL_GetTicks();

        input->current.mouse.delta_x = 0;
        input->current.mouse.delta_y = 0;

        while( SDL_PollEvent(&event) ){
            switch( event.type ){
                case SDL_WINDOWEVENT:
                {
                    switch( event.window.event ){
                        // NOTE(jerry): Dummy events if I want to check up again.
                        case SDL_WINDOWEVENT_CLOSE:
                        case SDL_WINDOWEVENT_FOCUS_LOST:
                        case SDL_WINDOWEVENT_FOCUS_GAINED:
                        case SDL_WINDOWEVENT_LEAVE:
                        case SDL_WINDOWEVENT_ENTER:
                        case SDL_WINDOWEVENT_RESTORED:
                        case SDL_WINDOWEVENT_MAXIMIZED:
                        case SDL_WINDOWEVENT_MINIMIZED:
                        case SDL_WINDOWEVENT_MOVED:
                        case SDL_WINDOWEVENT_EXPOSED:
                        case SDL_WINDOWEVENT_HIDDEN:
                        case SDL_WINDOWEVENT_SHOWN:
                            {
                            }
                            break;
                        case SDL_WINDOWEVENT_RESIZED:
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                            {
                                SDL_GL_GetDrawableSize( window,
                                        &state->screen_width,
                                        &state->screen_height );
                            }
                            break;
                    }
                }
                break;
                case SDL_QUIT:
                {
                    running = false;
                }
                break;
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                {
                    i32 vk_code = event.key.keysym.sym;

                    vk_code = map_sdl2_key_to_input_key( vk_code );

                    if( event.type == SDL_KEYDOWN ){
                        input->current.keys[ vk_code ].down = true;

                        if( input->text_edit_mode ){
                            if( vk_code == INPUT_KEY_BACKSPACE ){
                                input->text_edit.cursor--;
                                input->text_edit.cursor = i32_max( input->text_edit.cursor, 0 );
                            }
                        }
                    }else if( event.type == SDL_KEYUP ){
                        input->current.keys[ vk_code ].down = false;
                    }
                }
                break;
                case SDL_MOUSEMOTION:
                {
                    i32 mouse_x = event.motion.x;
                    i32 mouse_y = event.motion.y;

                    i32 mouse_relative_x = event.motion.xrel;
                    i32 mouse_relative_y = event.motion.yrel;

                    input->current.mouse.x = mouse_x;
                    input->current.mouse.y = mouse_y;

                    if( input->relative_mouse_mode ){
                        input->current.mouse.delta_x = mouse_relative_x;
                        input->current.mouse.delta_y = mouse_relative_y;
#if 0
                        fprintf(stderr, "delta_x: %d\n", mouse_relative_x);
                        fprintf(stderr, "delta_y: %d\n", mouse_relative_y);
#endif
                    }else{
                        input->current.mouse.delta_x = 0;
                        input->current.mouse.delta_y = 0;
                    }
                }
                break;
                case SDL_MOUSEBUTTONUP:
                case SDL_MOUSEBUTTONDOWN:
                {
                    bool left_button;
                    bool middle_button;
                    bool right_button;

                    if( event.button.button == SDL_BUTTON_LEFT ){
                        left_button = (event.button.state == SDL_PRESSED);
                    }
                    if( event.button.button == SDL_BUTTON_MIDDLE ){
                        middle_button = (event.button.state == SDL_PRESSED);
                    }
                    if( event.button.button == SDL_BUTTON_RIGHT ){
                        right_button = (event.button.state == SDL_PRESSED);
                    }

                    input->current.mouse.left = left_button;
                    input->current.mouse.middle = middle_button;
                    input->current.mouse.right = right_button;
                }
                break;
                case SDL_TEXTINPUT:
                {
                    strcat(input->text_edit.text, event.text.text);
                    input->text_edit.cursor++;
                }
                break;
            }
        }

        f32 delta_time = elapsed / 1000.0f;

#if 0
        // NOTE(jerry):
        // super important platform layer test code?
        glBegin(GL_TRIANGLES);

        glVertex3f( -1.0, -1.0, 0.0 );
        glVertex3f( 0.0, 1.0, 0.0 );
        glVertex3f( 1.0, -1.0, 0.0 );

        glEnd();
#endif
        if( input->text_edit_mode ){
            if( !SDL_IsTextInputActive() ){
                SDL_StartTextInput();
            }
        }else{
            SDL_StopTextInput();
        }

        input->relative_mouse_mode = false;
        input_update( input );
        update_render_game( state, renderer, input, delta_time );

        // wtf
        if( input->text_edit_mode ){
            memset( input->text_edit.text, 0, 32 );
        }

        {
            SDL_bool enable_mode = SDL_FALSE;

            if( input->relative_mouse_mode ){
                enable_mode = SDL_TRUE;
#if 0
                fprintf(stderr, "should enable relative mouse mode\n");
#endif
            }

            if( SDL_SetRelativeMouseMode( enable_mode ) != -1 ){
                if( enable_mode == SDL_TRUE ){
#if 0
                    fprintf(stderr, "relative mouse mode on.\n");
#endif
                }
            }else{
                if( enable_mode == SDL_TRUE ){
#if 0
                    fprintf(stderr, "relative mouse mode failed... Fallback\n");
#endif
                }
            }
        }

        // Chances are I might want to cap for release to
        // ensure nothing explodes horribly ( although the parts 
        // that rely on delta time are very minimal )
#ifdef DEBUG_BUILD
        SDL_GL_SetSwapInterval( 0 );
#endif
        SDL_GL_SwapWindow( window );

        elapsed = start - end;
        end = start;
    }

    game_finish( state );

    platform_show_cursor();
    memory_pool_finish( &main_memory );

    SDL_GL_DeleteContext( gl_context );
    SDL_DestroyWindow( window );
    SDL_Quit();
    return 0;
}
#endif
