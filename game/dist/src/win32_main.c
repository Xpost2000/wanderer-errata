#include <windows.h>
#include "input.h"

#include "renderer.h"

#include "common.h"
#include "wanderer.h"

#include "bmp_loader.h"

#include "opengl_functions.h"
#include "opengl_functions.c"

#include "memory_pool.h"

static bool running = true;
static bool handle_relative_mouse_mode = false;

static i32 before_relative_mode_mouse_location_x = 0;
static i32 before_relative_mode_mouse_location_y = 0;

#define virtual_key_case( key, mapped ) \
    case key:\
    {\
        return mapped;\
    }\
    break

static i32 map_win32_vk_code_to_input_key( i32 win32_vk_code ){
    switch( win32_vk_code ){
        virtual_key_case( VK_ESCAPE, INPUT_KEY_ESCAPE );

        virtual_key_case( VK_UP, INPUT_KEY_UP );
        virtual_key_case( VK_LEFT, INPUT_KEY_LEFT );
        virtual_key_case( VK_RIGHT, INPUT_KEY_RIGHT );
        virtual_key_case( VK_DOWN, INPUT_KEY_DOWN );
        virtual_key_case( VK_RETURN, INPUT_KEY_RETURN );

        virtual_key_case( '0', INPUT_KEY_0 );
        virtual_key_case( '1', INPUT_KEY_1 );
        virtual_key_case( '2', INPUT_KEY_2 );
        virtual_key_case( '3', INPUT_KEY_3 );
        virtual_key_case( '4', INPUT_KEY_4 );
        virtual_key_case( '5', INPUT_KEY_5 );
        virtual_key_case( '6', INPUT_KEY_6 );
        virtual_key_case( '7', INPUT_KEY_7 );
        virtual_key_case( '8', INPUT_KEY_8 );
        virtual_key_case( '9', INPUT_KEY_9 );

        virtual_key_case( 'A', INPUT_KEY_A );
        virtual_key_case( 'B', INPUT_KEY_B );
        virtual_key_case( 'C', INPUT_KEY_C );
        virtual_key_case( 'D', INPUT_KEY_D );
        virtual_key_case( 'E', INPUT_KEY_E );
        virtual_key_case( 'F', INPUT_KEY_F );
        virtual_key_case( 'G', INPUT_KEY_G );
        virtual_key_case( 'H', INPUT_KEY_H );
        virtual_key_case( 'I', INPUT_KEY_I );
        virtual_key_case( 'J', INPUT_KEY_J );
        virtual_key_case( 'K', INPUT_KEY_K );
        virtual_key_case( 'L', INPUT_KEY_L );
        virtual_key_case( 'M', INPUT_KEY_M );
        virtual_key_case( 'N', INPUT_KEY_N );
        virtual_key_case( 'O', INPUT_KEY_O );
        virtual_key_case( 'P', INPUT_KEY_P );
        virtual_key_case( 'Q', INPUT_KEY_Q );
        virtual_key_case( 'R', INPUT_KEY_R );
        virtual_key_case( 'S', INPUT_KEY_S );
        virtual_key_case( 'T', INPUT_KEY_T );
        virtual_key_case( 'U', INPUT_KEY_U );
        virtual_key_case( 'V', INPUT_KEY_V );
        virtual_key_case( 'W', INPUT_KEY_W );
        virtual_key_case( 'X', INPUT_KEY_X );
        virtual_key_case( 'Y', INPUT_KEY_Y );
        virtual_key_case( 'Z', INPUT_KEY_Z );

        virtual_key_case( VK_SHIFT, INPUT_KEY_LSHIFT );
        virtual_key_case( VK_LSHIFT, INPUT_KEY_LSHIFT );
        virtual_key_case( VK_RSHIFT, INPUT_KEY_RSHIFT );

        virtual_key_case( VK_LMENU, INPUT_KEY_LALT );
        virtual_key_case( VK_RMENU, INPUT_KEY_RALT );

        virtual_key_case( VK_OEM_102, INPUT_KEY_BACKSLASH );
        virtual_key_case( VK_OEM_2, INPUT_KEY_FORWARDSLASH );
        virtual_key_case( VK_OEM_COMMA, INPUT_KEY_COMMA );
        virtual_key_case( VK_BACK, INPUT_KEY_BACKSPACE );
        virtual_key_case( VK_SPACE, INPUT_KEY_SPACE );
        virtual_key_case( VK_OEM_1, INPUT_KEY_SEMICOLON );
        virtual_key_case( VK_TAB, INPUT_KEY_TAB );
        virtual_key_case( VK_OEM_PERIOD, INPUT_KEY_PERIOD );
    }
}

static void platform_show_cursor( void ){
    ShowCursor( TRUE );
}

static void platform_hide_cursor( void ){
    ShowCursor( FALSE );
}

// make windows happy.
// but we're not really using this.
// except for non-game relevant system messages.
LRESULT CALLBACK
win32_window_proc( HWND hwnd,
                   UINT umsg,
                   WPARAM wparam,
                   LPARAM lparam )
{
    LRESULT message_handled = 0;

    switch( umsg ){
        case WM_SIZE:
        case WM_PAINT:
        {
        }
        break;
        case WM_DESTROY:
        case WM_CLOSE:
        {
            running = false;
        }
        break;
        default:
        {
            message_handled = DefWindowProc( hwnd, umsg, wparam, lparam );
        }
        break;
    }

    return message_handled;
}

void* get_gl_function( const char* proc_name ){
    void* gl_func = (void*) wglGetProcAddress( proc_name );

    if( gl_func == NULL ||
        (gl_func == (void*)0x1)||
        (gl_func == (void*)0x2)||
        (gl_func == (void*)0x3)||
        (gl_func == (void*)-1))
    {
        HMODULE gl_dll = LoadLibraryA( "opengl32.dll" );
        gl_func = (void*)GetProcAddress( gl_dll, proc_name );
    }

    return gl_func;
}

typedef struct{
    HWND window;
    WNDCLASS window_class;

    PIXELFORMATDESCRIPTOR pixel_format_descriptor;
    HGLRC gl_context;
}win32_window;

static void init_win32_window( win32_window* window ){
    char* window_class_name = window->window_class.lpszClassName;
    HINSTANCE hInstance = window->window_class.hInstance;

    RegisterClass( &window->window_class );

    window->window = CreateWindowEx(
        0,
        window_class_name,
        "Wanderer [Win32]",
        WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX,

        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1440,
        900,

        NULL, NULL,
        hInstance,
        NULL
    );

    HDC window_dc = GetDC( window->window );
    i32 pixel_format = ChoosePixelFormat(
        window_dc,
        &window->pixel_format_descriptor );

    SetPixelFormat( window_dc,
                    pixel_format,
                    &window->pixel_format_descriptor );

    window->gl_context = wglCreateContext( window_dc );
    wglMakeCurrent( window_dc, window->gl_context );

    /*TODO(jerry): Create dummy context then query for version support!
      This is not particularly safe right now!
      
      Or rather this is a bad idea.
    */
    init_gl_functions();

    ShowWindow( window->window, SW_SHOWNORMAL );
}

static void destroy_win32_window( win32_window* window ){
    wglMakeCurrent( NULL, NULL );
    wglDeleteContext( window->gl_context );

    DestroyWindow( window->window );
    UnregisterClass( window->window_class.lpszClassName,
                     window->window_class.hInstance );
}

static HWND game_window_handle;
void platform_move_mouse( int x, int y ){
    fprintf(stderr, "Win32 platform move mouse\n");
    POINT absolute_screen_coordinates =
    {
        .x = x,
        .y = y
    };
    ClientToScreen( game_window_handle, &absolute_screen_coordinates );
    SetCursorPos( absolute_screen_coordinates.x,
                  absolute_screen_coordinates.y );
#if 0
    fprintf(stderr,
            "target_coordinates( %d, %d )\n",
            x,
            y);

    fprintf(stderr,
            "absolute_screen_coordinates( %d, %d )\n",
            absolute_screen_coordinates.x,
            absolute_screen_coordinates.y);
#endif
}

#if 0
#include "string_test.c"
#else
int WinMain( HINSTANCE hInstance,
             HINSTANCE hPrevInstance,
             LPSTR lpCmdLine,
             int nShowCmd )
{
#if 1
    assert( i32_clamp( 4, 2, 4 ) == 4 );
    assert( i32_clamp( -3, 2, 4 ) == 2 );
#endif
    static const char* window_class_name = "WandererWindowClassName";

    win32_window window ={
        .window_class =
        {
            .style = (CS_OWNDC | CS_HREDRAW | CS_VREDRAW),
            .lpfnWndProc = win32_window_proc,
            .hInstance = hInstance,
            .lpszClassName = window_class_name,
            .hCursor = LoadCursorA( NULL, IDC_ARROW )
        },

        .pixel_format_descriptor =
        {
            .nSize = sizeof( PIXELFORMATDESCRIPTOR ),
            .nVersion = 1,
            .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
            .iPixelType = PFD_TYPE_RGBA,
            .cColorBits = 32,
            .cDepthBits = 24,
            .cStencilBits = 8,
            .cAuxBuffers = 00,
            .iLayerType = PFD_MAIN_PLANE
        }
    };

    init_win32_window( &window );

    game_window_handle = window.window;

    u64 memory_pool_size = 
        sizeof(game_input) + sizeof(game_state) + sizeof(renderer) + MB(8);

    memory_pool main_memory = {};
    memory_pool_init( &main_memory, memory_pool_size );

    platform_hide_cursor();

    game_input* input  = memory_pool_allocate( &main_memory, sizeof(game_input) );
    game_state* state  = memory_pool_allocate( &main_memory, sizeof(game_state) );
    renderer* renderer = memory_pool_allocate( &main_memory, sizeof(renderer) );

	state->scratch_memory =
		(void*)(main_memory.memory + (memory_pool_size - MB(16)));

#if 0
    renderer_init( &renderer );
#endif

#if 0
    state.r = 0.1;
    state.g = 0.1;
    state.b = 0.1;
#endif

    MSG message;

    LARGE_INTEGER start, end, elapsed;
    LARGE_INTEGER freq_counter;

    elapsed.QuadPart = 0;

    QueryPerformanceFrequency( &freq_counter );
    // no bogus values to start with.
    QueryPerformanceCounter( &start );
    QueryPerformanceCounter( &end );

    while( running && 
           state->mode != GAME_STATE_QUIT ){
        GetMessage( &message, NULL, 0, 0 );

        TranslateMessage( &message );

        UINT message_type = message.message;
        WPARAM wparam = message.wParam;
        LPARAM lparam = message.lParam;
    
        switch( message_type ){
            case WM_CHAR:
            {
                // To match parity with SDL2 I need to
                // deal with IME... Yeah no I'll worry about
                // that later.
                if( input->text_edit_mode ){
                    if( wparam >= 32 && wparam < 127 ){
                        input->text_edit.text[0] = wparam;
                        input->text_edit.text[1] = 0;
                        input->text_edit.cursor++;
                    }
                }
            }
            break;
            case WM_SIZE:
            case WM_PAINT:
            {
                RECT window_rect;

                #ifdef USE_GETWINDOWRECT
                GetWindowRect( window.window, &window_rect );
                #else
                GetClientRect( window.window, &window_rect );
                #endif

                state->screen_width = window_rect.right - window_rect.left;
                state->screen_height = window_rect.bottom - window_rect.top;
            }
            break;
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MOUSEMOVE:
            {
                /*NOTE(jerry): seems slightly inaccurate due to border.
                  I'll check behavior with an SDL2 program to see if that's what it
                  does otherwise I might have to correct it.
                 */
                bool left_button   = (wparam & MK_LBUTTON);
                bool middle_button = (wparam & MK_MBUTTON);
                bool right_button  = (wparam & MK_RBUTTON);

                POINT cursor_point;
                GetCursorPos( &cursor_point );
                ScreenToClient( window.window, &cursor_point );

                int32_t mouse_x = cursor_point.x;
                int32_t mouse_y = cursor_point.y;

                input->current.mouse.left = left_button;
                input->current.mouse.middle = middle_button;
                input->current.mouse.right = right_button;
 
                input->current.mouse.x = mouse_x;
                input->current.mouse.y = mouse_y;

                if( input->relative_mouse_mode ){
                    if( !handle_relative_mouse_mode ){
                        handle_relative_mouse_mode = true;
                        before_relative_mode_mouse_location_x = input->current.mouse.x;
                        before_relative_mode_mouse_location_y = input->current.mouse.y;

                        RECT window_rect;
                        GetWindowRect( game_window_handle, &window_rect );
                        ClipCursor( &window_rect );
                    }
                    i32 mouse_relative_x = before_relative_mode_mouse_location_x - mouse_x;
                    i32 mouse_relative_y = before_relative_mode_mouse_location_y - mouse_y;

                    input->current.mouse.delta_x = -mouse_relative_x;
                    input->current.mouse.delta_y = -mouse_relative_y;

#if 0
                    fprintf(stderr, "mouse_relative(%d, %d)\n", mouse_relative_x, mouse_relative_y);
#endif

                    platform_move_mouse( before_relative_mode_mouse_location_x, before_relative_mode_mouse_location_y );
                }else{
                    if( handle_relative_mouse_mode ){
                        handle_relative_mouse_mode = false;
                        ClipCursor(NULL);
#if 0
                        before_relative_mode_mouse_location_x = input->current.mouse.x;
                        before_relative_mode_mouse_location_y = input->current.mouse.y;

                        platform_move_mouse( before_relative_mode_mouse_location_x,
                                             before_relative_mode_mouse_location_y );
#endif
                    }

                    input->current.mouse.delta_x = 0;
                    input->current.mouse.delta_y = 0;
                }

            }
            break;
            case WM_KEYUP:
            case WM_KEYDOWN:
            {
                int32_t vk_code = wparam;

                vk_code = map_win32_vk_code_to_input_key( vk_code );

                if( message_type == WM_KEYDOWN ){
                    input->current.keys[ vk_code ].down = true;

                    if( input->text_edit_mode ){
                        if( vk_code == INPUT_KEY_BACKSPACE ){
                            input->text_edit.cursor--;
                            input->text_edit.cursor = i32_max( input->text_edit.cursor, 0 );
                        }
                    }
                }else if( message_type == WM_KEYUP ){
                    input->current.keys[ vk_code ].down = false;
                }
            }
            break;
            default:
            {
                DispatchMessage( &message );
            }
            break;
        }

        /*NOTE(jerry): Delta time calculations*/
        input->relative_mouse_mode = false;
        input_update( input );
        update_render_game( state, renderer, input, (elapsed.QuadPart / 1000.f) );

        // wtf
        if( input->text_edit_mode ){
            memset( input->text_edit.text, 0, 32 );
        }

        QueryPerformanceCounter( &end );

        elapsed.QuadPart = end.QuadPart - start.QuadPart;
        // convert to micro seconds
        elapsed.QuadPart *= 1000;
        // get ticks per second.
        elapsed.QuadPart /= freq_counter.QuadPart;

        start = end;
#if 0
        printf("%3.3f\n", (f32)(elapsed.QuadPart/1000.f) );
#endif

        HDC window_dc = GetDC( window.window );
        SwapBuffers( window_dc );
    }

    game_finish( state );

    platform_show_cursor();
    memory_pool_finish( &main_memory );

    destroy_win32_window( &window );
    return 0;
}
#endif
