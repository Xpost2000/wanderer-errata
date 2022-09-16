#ifndef RENDERER_H
#define RENDERER_H

#include "common.h"
#include "vec2.h"

#include "bmp_loader.h"

#include <stb_truetype.h>
#include "wanderer_assets.h"

// TODO(jerry): Implement texture atlasing
// scheme so this doesn't have to happen.
#define TEXTURES_MAX 4096
#define FONTS_MAX 128

#define MAX_RENDERER_COMMANDS 4096

enum render_command_type{
    RENDER_COMMAND_NONE,

    RENDER_COMMAND_DRAW_QUAD,
    RENDER_COMMAND_DRAW_TEXTURED_QUAD,
    RENDER_COMMAND_DRAW_TEXT,
    RENDER_COMMAND_CLEAR_BUFFERS,

    // match parity with original.
    // but probably shouldn't be touching...
    RENDER_COMMAND_LOAD_TEXTUREMATRIX,
    RENDER_COMMAND_LOAD_PROJECTIONMATRIX,
    RENDER_COMMAND_LOAD_MODELVIEWMATRIX,

    RENDER_COMMAND_SCISSOR_ENABLE,
    RENDER_COMMAND_SCISSOR_DISABLE,

    RENDER_COMMAND_COUNT
};

enum renderer_matrix{
    RENDERER_TEXTURE_MATRIX,
    RENDERER_PROJECTION_MATRIX,
    RENDERER_MODELVIEW_MATRIX,

    RENDERER_MATRIX_COUNT
};

enum text_command_justification{
    TEXT_COMMAND_JUSTIFICATION_NONE = 0,
    TEXT_COMMAND_JUSTIFICATION_LEFT = BIT(0),
    TEXT_COMMAND_JUSTIFICATION_CENTER = BIT(1),
    TEXT_COMMAND_JUSTIFICATION_RIGHT = BIT(2),
    TEXT_COMMAND_JUSTIFICATION_BOTTOM = BIT(3),
    TEXT_COMMAND_JUSTIFICATION_TOP = BIT(4),

    TEXT_COMMAND_JUSTFICATION_COUNT = 6
};

enum renderer_clear_buffer{
    RENDERER_CLEAR_BUFFER_NONE,
    RENDERER_CLEAR_BUFFER_COLOR = BIT(0),
    RENDERER_CLEAR_BUFFER_STENCIL = BIT(1),
    RENDERER_CLEAR_BUFFER_DEPTH = BIT(2),

    RENDERER_CLEAR_BUFFER_COUNT = 4
};

typedef struct render_command_clear_buffer{
    u16 buffer_flags;

    /*color buffer*/
    f32 r;
    f32 g;
    f32 b;
    f32 a;
}render_command_clear_buffer_info;

typedef struct render_command_textured_quad{
    f32 x;
    f32 y;
    f32 w;
    f32 h;

    f32 r;
    f32 g;
    f32 b;
    f32 a;

    f32 uv_x;
    f32 uv_y;
    f32 uv_w;
    f32 uv_h;

    game_asset_handle texture_id;
}render_command_textured_quad_info;

typedef struct render_command_quad{
    f32 x;
    f32 y;
    f32 w;
    f32 h;

    f32 r;
    f32 g;
    f32 b;
    f32 a;
}render_command_quad_info;

typedef struct render_command_text{
    f32 x;
    f32 y;

    f32 scale;

    f32 r;
    f32 g;
    f32 b;
    f32 a;

    game_asset_handle font_id;
    char* text;

    struct{
        f32 x;
        f32 y;
        f32 w;
        f32 h;
    }format_rect;

    u16 justification_type;

    b32 paged; /*page wrap? I just need it for the journal atm.*/
    b32 wrapped;
}render_command_text_info;

typedef struct render_command_scissor{
    u32 x;
    u32 y;
    u32 w;
    u32 h;
}render_command_scissor_info;

typedef struct render_command{
    u32 command;

    union{
        render_command_clear_buffer_info clear_buffer;
        render_command_textured_quad_info textured_quad;
        render_command_quad_info quad;
        render_command_text_info text;
        render_command_scissor_info scissor;

        struct{
            f32 matrix[16];
        }matrix;
    }info;
}render_command;

typedef struct render_layer{
    f32 texture_matrix[16];
    f32 modelview_matrix[16];
    f32 projection_matrix[16];

    struct camera{
        f32 x;
        f32 y;

        f32 scale;
    }camera;

    u64 commands;
    render_command command_list[MAX_RENDERER_COMMANDS];
}render_layer;

render_command render_command_load_matrix( u16 matrix_id, f32* matrix );

render_command render_command_start_scissor( u32 x, u32 y, u32 w, u32 h );
render_command render_command_end_scissor( void );

render_command render_command_clear_buffer( u16 buffer_flags, f32 r, f32 g, f32 b, f32 a );

/*I need to make helper functions that make rendering look less verbose.*/

render_command render_command_text( char* text, game_asset_handle font,
                                    f32 x, f32 y, f32 scale, 
                                    f32 r, f32 g, f32 b, f32 a );

render_command render_command_text_justified( char* text, game_asset_handle font,
                                              f32 scale, f32 r, f32 g, f32 b, f32 a,
                                              f32 bounds_x, f32 bounds_y, 
                                              f32 bounds_w, f32 bounds_h,
                                              u16 justification_type );

render_command render_command_text_wrapped( char* text, game_asset_handle font,
                                            f32 scale, f32 r, f32 g, f32 b, f32 a,
                                            f32 bounds_x, f32 bounds_y, 
                                            f32 bounds_w, f32 bounds_h );

render_command render_command_text_page_wrapped( char* text, game_asset_handle font,
                                                 f32 scale, f32 r, f32 g, f32 b, f32 a,
                                                 f32 bounds_x, f32 bounds_y, 
                                                 f32 bounds_w, f32 bounds_h );

render_command render_command_quad( f32 x, f32 y, f32 w, f32 h,
                                    f32 r, f32 g, f32 b, f32 a );

render_command render_command_textured_quad( f32 x, f32 y, f32 w, f32 h,
                                             f32 r, f32 g, f32 b, f32 a,
                                             game_asset_handle texture );

render_command render_layer_push_command( render_layer* layer, 
                                          render_command command );

vec2 render_layer_screen_to_camera( render_layer* layer, vec2 screen );

/*convenience functions for the most commonly used.*/
void render_layer_push_text( render_layer* layer,
                             char* text, game_asset_handle font,
                             f32 x, f32 y, f32 r, 
                             f32 g, f32 b, f32 a );

void render_layer_push_text_wrapped( render_layer* layer,
                                     char* text, game_asset_handle font,
                                     f32 r, f32 g, f32 b, f32 a,
                                     f32 bounds_x, f32 bounds_y, 
                                     f32 bounds_w, f32 bounds_h );

void render_layer_push_quad( render_layer* layer, 
                             f32 x, f32 y, 
                             f32 w, f32 h, 
                             f32 r, f32 g, f32 b, f32 a );

void render_layer_push_textured_quad( render_layer* layer, 
                                      f32 x, f32 y, 
                                      f32 w, f32 h, 
                                      f32 r, f32 g, f32 b, f32 a, 
                                      game_asset_handle texture_id );
/*convenience functions*/

void render_layer_camera_set_position( render_layer* layer, f32 x, f32 y );
void render_layer_camera_set_scale( render_layer* layer, f32 scale );
void render_layer_camera_reset( render_layer* layer );

/*this should be an opaque void pointer....*/
typedef struct texture{
    u32 handle;
}texture;

// Should also have a userdata pointer
// for OS dependent things?
typedef struct renderer{
    u32 screen_width;
    u32 screen_height;

    // back pointer for resources.
    game_assets* assets;

    /*
      Expecting these to be just permenantly
      part of game_assets in the future...
    */
    struct fonts_list{
        u64 count;
        struct font_entry{
            /*TODO: Go back to using atlases.*/
            /*Replacing to per character texture*/
            /*slower, but gradual changes.*/
            f32 size;

            u32 glyph_range_start;
            u32 glyph_range_end;

            /*include list of excluded code points?*/

            struct glyph_texture{
                u32 codepoint; /*debug related?*/
                u32 texture_id;

                i32 glyph_bitmap_left;
                i32 glyph_bitmap_right;
                i32 glyph_bitmap_top;
                i32 glyph_bitmap_bottom;

                i32 glyph_bitmap_width;
                i32 glyph_bitmap_height;

                i32 glyph_ascent;
                i32 glyph_descent;

                /*I don't store kerning information because 
                 *I don't want to calculate it?*/
                i32 advance_width; 
                /*offset to next horizontal position*/
                i32 left_side_bearing; 
                /*offset from current horizontal position to the left edge of character.*/
                i32 line_gap; 
                /*spacing between row's descent and the next descent*/
                f32 glyph_scale_factor;
            }glyphs[127]; // covers all of ASCII.
        }list[FONTS_MAX];
    }fonts;

    // extraneous... store handles instead....
    struct texture_list{
        u64 count;
        texture list[TEXTURES_MAX];
    }textures;
}renderer;

void renderer_get_text_size( renderer* renderer, game_asset_handle font, char* text, f32* width, f32* height, f32* size );
void renderer_get_wrapped_text_size( renderer* renderer, game_asset_handle font_id, char* text, f32 bounds_w, f32 bounds_h, f32* width, f32* height, f32* size );

void renderer_draw_render_layer( renderer* renderer, render_layer* layer );
void renderer_set_screen_dimensions( renderer* renderer, u32 width, u32 height );

/*TODO(jerry): Make convenience push command functions*/
void render_layer_clear_color_buffer( render_layer* layer, f32 r, f32 g, f32 b, f32 a );
/*end of that*/

/*Should not be responsible but okay.*/
u32 renderer_create_texture_from_bitmap( renderer* renderer, bitmap_image* bmp );
u32 renderer_load_font( renderer* renderer,
                        char* file_byte_buffer,
                        size_t size,
                        f32 font_size );

#endif
