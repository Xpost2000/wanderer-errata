#include "renderer.h"

/*
 * Renderer agnostic files.
 * Basically where all the render command stuff goes
 * because that's not really api specific.
 *
 * NOTE(jerry): Asset loading code also goes here,
 * and the specific backends do their own thing?
 */

void renderer_get_wrapped_text_size( renderer* renderer, 
                                     game_asset_handle font_id, 
                                     char* text,
                                     f32 bounds_w,
                                     f32 bounds_h, /*TODO(jerry): I don't actually use this. Remove it.*/
                                     f32* width,
                                     f32* height,
                                     f32* size ){
    game_asset* font_asset = game_asset_get_from_handle( renderer->assets, font_id );

    if( font_asset->type != GAME_ASSET_FONT ){
        return;
    }

    u32 lookup_index = font_asset->font.renderer_id;
    struct font_entry* selected_font = &renderer->fonts.list[ lookup_index ];

    u32 glyph_begin = selected_font->glyph_range_start;
    u32 glyph_end = selected_font->glyph_range_end;

    char* text_start = text;

    f32 greatest_text_width = 0.f;
    f32 text_height = 0.f;
    f32 text_width = 0.f;

    /*NOTE(jerry): 
      The text height calculation is completely irrelvant.
      All glyphs have the same pixel height. I should probably
      do some real clean-up soon.
    */
    {
        f32 current_text_height = 0.f;
        while( *text ){
            if( *text >= glyph_begin && *text < glyph_end ){
                struct glyph_texture* glyph = &selected_font->glyphs[ *text ];

                f32 glyph_ascent = glyph->glyph_ascent * glyph->glyph_scale_factor;
                f32 glyph_descent = glyph->glyph_descent * glyph->glyph_scale_factor;

                f32 glyph_linegap = glyph->line_gap * glyph->glyph_scale_factor;

                current_text_height = glyph_ascent - glyph_descent + glyph_linegap;

                if( current_text_height > text_height ){
                    text_height = current_text_height;
                }
            }
            text++;
        }

        text = text_start;
    }

    u32 page_count = 1;
    {
        while( *text ){
            if( *text == '\n' || text_width > bounds_w ){
                text_height += selected_font->size;

                if( text_height >= bounds_h ){
                    page_count++;
                }

                text_width = 0;
            }else if( *text >= glyph_begin && *text < glyph_end ){
                struct glyph_texture* glyph = &selected_font->glyphs[ *text ];

                f32 glyph_advance_width = glyph->advance_width * glyph->glyph_scale_factor;
                f32 glyph_left_side_bearing = glyph->left_side_bearing * glyph->glyph_scale_factor;

                text_width += glyph_advance_width + glyph_left_side_bearing;

                if( text_width > greatest_text_width ){
                    greatest_text_width = text_width;
                }
            }

            text++;
        }
    }

    if( width ){
        *width = bounds_w * page_count;
    }

if( size ){
        *size = (f32)(selected_font->size);
    }

    if( height ){
        *height = text_height;
    }
}

void renderer_get_text_size( renderer* renderer, 
                             game_asset_handle font_id, 
                             char* text,
                            f32* width,
                            f32* height,
                            f32* size ){
    game_asset* font_asset = game_asset_get_from_handle( renderer->assets, font_id );

    if( font_asset->type != GAME_ASSET_FONT ){
        return;
    }

    u32 lookup_index = font_asset->font.renderer_id;
    struct font_entry* selected_font = &renderer->fonts.list[ lookup_index ];

    u32 glyph_begin = selected_font->glyph_range_start;
    u32 glyph_end = selected_font->glyph_range_end;

    char* text_start = text;

    f32 text_height = 0.f;
    f32 text_width = 0.f;

    {
        f32 current_text_height = 0.f;
        while( *text ){
            if( *text >= glyph_begin && *text < glyph_end ){
                struct glyph_texture* glyph = &selected_font->glyphs[ *text ];

                f32 glyph_ascent = glyph->glyph_ascent * glyph->glyph_scale_factor;
                f32 glyph_descent = glyph->glyph_descent * glyph->glyph_scale_factor;

                f32 glyph_linegap = glyph->line_gap * glyph->glyph_scale_factor;

                current_text_height = glyph_ascent - glyph_descent + glyph_linegap;

                if( current_text_height > text_height ){
                    text_height = current_text_height;
                }
            }
            text++;
        }

        text = text_start;
    }

    {
        while( *text ){
            if( *text == '\n' ){
                text_height += selected_font->size;
            }else if( *text >= glyph_begin && *text < glyph_end ){
                struct glyph_texture* glyph = &selected_font->glyphs[ *text ];

                f32 glyph_advance_width = glyph->advance_width * glyph->glyph_scale_factor;
                f32 glyph_left_side_bearing = glyph->left_side_bearing * glyph->glyph_scale_factor;

                text_width += glyph_advance_width + glyph_left_side_bearing;
            }

            text++;
        }
    }

    if( width ){
        *width = text_width;
    }

    if( size ){
        *size = (f32)(selected_font->size);
    }

    if( height ){
        *height = text_height;
    }
}

// Render command stuff
void render_layer_camera_set_position( render_layer* layer, f32 x, f32 y ){
    layer->camera.x = x;
    layer->camera.y = y;
}

void render_layer_camera_set_scale( render_layer* layer, f32 scale ){
    layer->camera.scale = scale;
}

void render_layer_camera_reset( render_layer* layer ){
}

vec2 render_layer_screen_to_camera( render_layer* layer, vec2 screen ){
    //NOTE(jerry): This is a bad hack
    //I mean it is mathematically correct, but uh I wouldn't count on this.
    vec2 projected;

    f32 camera_x = layer->camera.x;
    f32 camera_y = layer->camera.y;

    projected.x = screen.x - camera_x;
    projected.y = screen.y - camera_y;

    projected = mul_vec2_scalar( projected, (1.0f / layer->camera.scale) );

    return projected;
}

render_command render_command_load_matrix( u16 matrix_id, f32* matrix ){
    render_command cmd = {};

    memcpy( cmd.info.matrix.matrix, matrix, sizeof(cmd.info.matrix.matrix) );

    assert( matrix_id >= 0 && matrix_id < RENDERER_MATRIX_COUNT );

    switch( matrix_id ){
        case RENDERER_TEXTURE_MATRIX:
        {
            cmd.command = RENDER_COMMAND_LOAD_TEXTUREMATRIX;
        }
        break;
        case RENDERER_PROJECTION_MATRIX:
        {
            cmd.command = RENDER_COMMAND_LOAD_PROJECTIONMATRIX;
        }
        break;
        case RENDERER_MODELVIEW_MATRIX:
        {
            cmd.command = RENDER_COMMAND_LOAD_MODELVIEWMATRIX;
        }
        break;
    }

    return cmd;
}

render_command render_command_text( char* text, game_asset_handle font_id,
                                    f32 x, f32 y, f32 scale, 
                                    f32 r, f32 g, f32 b, f32 a ){
    // C11 syntax is pretty sick.
    render_command cmd = 
    {
        .command = RENDER_COMMAND_DRAW_TEXT,
        .info =
        {
            .text = 
            {
                .text = text,
                .x = x,
                .y = y,

                .scale = scale,

                .r = r,
                .g = g,
                .b = b,
                .a = a,

                .font_id = font_id
            }
        }
    };

    return cmd;
}

render_command render_command_text_justified( char* text, game_asset_handle font_id,
                                              f32 scale, f32 r, f32 g, f32 b, f32 a,
                                    f32 bounds_x, f32 bounds_y, 
                                    f32 bounds_w, f32 bounds_h,
                                    u16 justification_type ){
    render_command cmd = render_command_text( text, font_id, 0, 0, scale, r, g, b, a );

    cmd.info.text.format_rect.x = bounds_x;
    cmd.info.text.format_rect.y = bounds_y;
    cmd.info.text.format_rect.w = bounds_w;
    cmd.info.text.format_rect.h = bounds_h;
    cmd.info.text.justification_type = justification_type;

    return cmd;
}

render_command render_command_text_wrapped( char* text, game_asset_handle font_id,
                                            f32 scale, f32 r, f32 g, f32 b, f32 a,
                                    f32 bounds_x, f32 bounds_y, 
                                    f32 bounds_w, f32 bounds_h ){
    render_command cmd = render_command_text( text, font_id, bounds_x, bounds_y, scale, r, g, b, a );

    cmd.info.text.wrapped = true;
    cmd.info.text.format_rect.x = bounds_x;
    cmd.info.text.format_rect.y = bounds_y;
    cmd.info.text.format_rect.w = bounds_w;
    cmd.info.text.format_rect.h = bounds_h;

    return cmd;
}

render_command render_command_quad( f32 x, f32 y, f32 w, f32 h,
                                    f32 r, f32 g, f32 b, f32 a ){
    render_command cmd = 
    {
        .command = RENDER_COMMAND_DRAW_QUAD,
        .info =
        {
            .quad =
            {
                .x = x,
                .y = y,
                .w = w,
                .h = h,

                .r = r,
                .g = g,
                .b = b,
                .a = a
            }
        }
    };

    return cmd;
}

render_command render_command_textured_quad( f32 x, f32 y, f32 w, f32 h,
                                             f32 r, f32 g, f32 b, f32 a,
                                             game_asset_handle texture_id ){
    render_command cmd = 
    {
        .command = RENDER_COMMAND_DRAW_TEXTURED_QUAD,
        .info = 
        {
            .textured_quad =
            {
                .x = x,
                .y = y,
                .w = w,
                .h = h,

                .r = r,
                .g = g,
                .b = b,
                .a = a,

                .uv_x = 0,
                .uv_y = 0,
                .uv_w = 1,
                .uv_h = 1,

                .texture_id = texture_id
            }
        }
    };

    return cmd;
}

render_command render_command_start_scissor( u32 x, u32 y, u32 w, u32 h ){
    render_command cmd =
    {
        .command = RENDER_COMMAND_SCISSOR_ENABLE,
        .info =
        {
            .scissor =
            {
                .x = x,
                .y = y,
                .w = w,
                .h = h
            }
        }
    };

    return cmd;
}
render_command render_command_end_scissor( void ){
    render_command cmd =
    {
        .command = RENDER_COMMAND_SCISSOR_DISABLE
    };

    return cmd;
}

render_command render_command_clear_buffer( u16 buffer_flags, f32 r, f32 g, f32 b, f32 a ){
    render_command cmd =
    {
        .command = RENDER_COMMAND_CLEAR_BUFFERS,
        .info =
        {
            .clear_buffer =
            {
                .buffer_flags = buffer_flags,
                .r = r,
                .g = g,
                .b = b,
                .a = a
            }
        }
    };

    return cmd;
}

render_command render_layer_push_command( render_layer* layer, 
                                          render_command command ){
    if( layer->commands + 1 < MAX_RENDERER_COMMANDS ){
        layer->command_list[ layer->commands++ ] = command;
    }

    return command;
}

void render_layer_clear_color_buffer( render_layer* layer, f32 r, f32 g, f32 b, f32 a ){
    render_layer_push_command( layer, render_command_clear_buffer( RENDERER_CLEAR_BUFFER_COLOR, r, g, b, a ) );
}

void render_layer_push_text( render_layer* layer,
                             char* text, game_asset_handle font_id,
                             f32 x, f32 y, f32 r, 
                             f32 g, f32 b, f32 a ){
    render_command text_cmd = 
        render_command_text( text, font_id, x, y, 1, r, g, b, a );
    render_layer_push_command( layer, text_cmd );
}

void render_layer_push_text_justified( render_layer* layer,
                                       char* text, game_asset_handle font_id,
                                       f32 r, f32 g, f32 b, f32 a,
                                       f32 x, f32 y, f32 bounds_w, f32 bounds_h,
                                       u16 justification_type ){
    render_command text_cmd = 
        render_command_text_justified( 
                text, font_id, 1, r, g, b, a, x, y, bounds_w, bounds_h, justification_type );
    render_layer_push_command( layer, text_cmd );
}

void render_layer_push_text_wrapped( render_layer* layer,
                                     char* text, game_asset_handle font_id,
                                     f32 r, f32 g, f32 b, f32 a,
                                     f32 x, f32 y, f32 bounds_w, f32 bounds_h ){
    render_command text_cmd = 
        render_command_text_wrapped( text, font_id, 1, r, g, b, a, x, y, bounds_w, bounds_h );
    render_layer_push_command( layer, text_cmd );
}

void render_layer_push_quad( render_layer* layer, 
                             f32 x, f32 y, 
                             f32 w, f32 h, 
                             f32 r, f32 g, f32 b, f32 a ){
    render_command quad = 
        render_command_quad( x, y, w, h, r, g, b, a );
    render_layer_push_command( layer, quad );
}

void render_layer_push_textured_quad( render_layer* layer, 
                                      f32 x, f32 y, 
                                      f32 w, f32 h, 
                                      f32 r, f32 g, f32 b, f32 a, 
                                      game_asset_handle texture_id ){
    render_command quad = 
        render_command_textured_quad( x, y, w, h, r, g, b, a, texture_id );
    render_layer_push_command( layer, quad );
}
