#define STB_TRUETYPE_IMPLEMENTATION

#include "renderer.h"
#include "mat4x4.h"

#include "platform.h"

#include "opengl_functions.h"

/*NOTE(jerry): opengl1_renderer.c*/
static char* check_opengl_errors( void ){
    /*
      NOTE(jerry): Move into it's own procedure or maybe a macro?
      something like that anyways.
          
      this is probably just going to be a function call that
      is conditionally compiled with an #ifdef debug or something

      bool check_opengl_errors( void );
    */
    switch(glGetError()){
        case GL_NO_ERROR:
        {
            return NULL;
        }
        break;
        case GL_INVALID_ENUM:
        {
            return "OpenGL reports an invalid enum";
        }
        break;
        case GL_INVALID_VALUE:
        {
            return "OpenGL reports an invalid value";
        }
        break;
        case GL_INVALID_OPERATION:
        {
            return "OpenGL reports an invalid operation";
        }
        break;
        default:
        {
            return "OpenGL reports an error";
        }
        break;
    }
    return NULL;
}

void renderer_set_screen_dimensions( renderer* renderer, u32 width, u32 height ){
    renderer->screen_width = width;
    renderer->screen_height = height;
}

// uses quick start stb stuff.
// NOTE(jerry): This is not a good idea but it gets stuff on the screen
u32 renderer_load_font( renderer* renderer,
                               char* file_byte_buffer,
                               size_t size,
                               f32 font_size ){
    struct fonts_list* fonts = &renderer->fonts;
    struct font_entry* current_font = &fonts->list[ fonts->count ];

    // NOTE(jerry): pls no malloc?
    char* font_file_buffer = file_byte_buffer;

    stbtt_fontinfo font_info;

    if( !stbtt_InitFont( &font_info, font_file_buffer, 0 ) ){
        fprintf(stderr, "font loading error.\n");
        return 0;
    }else{
        current_font->glyph_range_start = 0;
        current_font->glyph_range_end = 127;

        for( u32 glyph = current_font->glyph_range_start; 
                 glyph < current_font->glyph_range_end; 
                 ++glyph ){
            struct glyph_texture* current_glyph = &current_font->glyphs[glyph];
            current_glyph->codepoint = glyph;

            i32* glyph_bitmap_left = &current_glyph->glyph_bitmap_left;
            i32* glyph_bitmap_right = &current_glyph->glyph_bitmap_right;
            i32* glyph_bitmap_top = &current_glyph->glyph_bitmap_top;
            i32* glyph_bitmap_bottom = &current_glyph->glyph_bitmap_bottom;

            i32* glyph_bitmap_width = &current_glyph->glyph_bitmap_width;
            i32* glyph_bitmap_height = &current_glyph->glyph_bitmap_height;

            i32* glyph_ascent = &current_glyph->glyph_ascent;
            i32* glyph_descent = &current_glyph->glyph_descent;

            i32* advance_width = &current_glyph->advance_width; 
            i32* left_side_bearing = &current_glyph->left_side_bearing; 

            i32* line_gap = &current_glyph->line_gap; 

            f32* glyph_scale_factor = &current_glyph->glyph_scale_factor;

            u32 prebake_font_pixel_size = 128;

            *glyph_scale_factor = stbtt_ScaleForPixelHeight( &font_info, prebake_font_pixel_size );

            stbtt_GetFontVMetrics( &font_info, glyph_ascent, glyph_descent, line_gap );
            stbtt_GetCodepointHMetrics( &font_info, glyph, advance_width, left_side_bearing );

            stbtt_GetCodepointBitmapBox( &font_info, glyph, 1, 1,
                                         glyph_bitmap_left,
                                         glyph_bitmap_top,
                                         glyph_bitmap_right,
                                         glyph_bitmap_bottom );

            *glyph_bitmap_width = *glyph_bitmap_right - *glyph_bitmap_left;
            *glyph_bitmap_height = *glyph_bitmap_bottom - *glyph_bitmap_top;

            u32 render_bitmap_width = (*glyph_bitmap_width) * (*glyph_scale_factor);
            u32 render_bitmap_height = (*glyph_bitmap_height) * (*glyph_scale_factor);

            u8* glyph_bitmap = memory_allocate( render_bitmap_width * render_bitmap_height );

            stbtt_MakeCodepointBitmap( &font_info, glyph_bitmap, 
                                       render_bitmap_width, render_bitmap_height,
                                       render_bitmap_width, 
                                       (*glyph_scale_factor), 
                                       (*glyph_scale_factor), glyph );

            f32 new_scale_factor = ((f32)font_size) / (f32)prebake_font_pixel_size;
            *glyph_scale_factor *= new_scale_factor;

            glGenTextures( 1, &current_glyph->texture_id );

            glBindTexture( GL_TEXTURE_2D, current_glyph->texture_id );

            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

            // No mipmaps.
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0 );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0 );

            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

            glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
            glTexImage2D( GL_TEXTURE_2D, 
                          0, 
                          GL_ALPHA, 
                          render_bitmap_width, 
                          render_bitmap_height, 
                          0, 
                          GL_ALPHA, 
                          GL_UNSIGNED_BYTE, glyph_bitmap );
            glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );

            glBindTexture( GL_TEXTURE_2D, 0 );

            memory_deallocate( glyph_bitmap );
        }
    }

    current_font->size = font_size;

    return fonts->count++;
}

// should be phased out in the future for relying entirely on the game asset system.
u32 renderer_create_texture_from_bitmap( renderer* renderer, bitmap_image* bmp ){
    GLuint new_texture_id = 0;
    glGenTextures( 1, &new_texture_id );
    glBindTexture( GL_TEXTURE_2D, new_texture_id );

    glTexImage2D( GL_TEXTURE_2D,
                  0,
                  GL_RGBA,
                  bmp->width,
                  bmp->height,
                  0,
                  GL_RGBA,
                  GL_UNSIGNED_BYTE,
                  bmp->pixels );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

    // No mipmaps.
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0 );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0 );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

    {
        const char* error_string = check_opengl_errors();
        if( error_string ){
            fprintf( stderr, "glError: %s\n", error_string );
        }else{
            fprintf( stderr, "No errors found. Good.\n" );
        }
    }

    glBindTexture( GL_TEXTURE_2D, 0 );

    {
        renderer->textures.list[renderer->textures.count].handle = new_texture_id;
        renderer->textures.count++;
    }

    return new_texture_id;
}

void renderer_draw_render_layer( renderer* renderer, render_layer* layer ){
    render_command* command_list = layer->command_list;
    const u64 command_count = layer->commands;

    glEnable( GL_BLEND );
    glEnable( GL_TEXTURE_2D );

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glViewport( 0, 0, renderer->screen_width, renderer->screen_height );
    // sane defaults incase something was not set...
    {
        build_identity_matrix( layer->texture_matrix );
        build_identity_matrix( layer->modelview_matrix );
        build_identity_matrix( layer->projection_matrix );

        build_orthographic_matrix( 
                layer->projection_matrix,
                0, 0,
                renderer->screen_width,
                renderer->screen_height,
                -0.1f, 1000.0f );

        // weird hard limits.
        layer->camera.scale = f32_clamp( layer->camera.scale, 0.001f, 15.0f );
    }

    for( u64 command_index = 0;
         command_index < command_count;
         ++command_index ){
        render_command* current_command = &command_list[ command_index ];

        // rebuild view matrix
        {
            build_identity_matrix( layer->modelview_matrix );

            build_translation_matrix( layer->modelview_matrix,
                                      layer->camera.x,
                                      layer->camera.y,
                                      0.0f );

            build_scale_matrix( layer->modelview_matrix,
                                layer->camera.scale,
                                layer->camera.scale,
                                1.0f ); 
        }

        glMatrixMode( GL_TEXTURE );
        glLoadMatrixf( layer->texture_matrix );

        glMatrixMode( GL_MODELVIEW );
        glLoadMatrixf( layer->modelview_matrix );

        glMatrixMode( GL_PROJECTION );
        glLoadMatrixf( layer->projection_matrix );

        switch( current_command->command ){
            case RENDER_COMMAND_CLEAR_BUFFERS:
            {
                u16 flags = 0;
                
                if( current_command->info.clear_buffer.buffer_flags & RENDERER_CLEAR_BUFFER_COLOR ){
                    flags |= GL_COLOR_BUFFER_BIT;
                }

                if( current_command->info.clear_buffer.buffer_flags & RENDERER_CLEAR_BUFFER_STENCIL ){
                    flags |= GL_STENCIL_BUFFER_BIT;
                }

                if( current_command->info.clear_buffer.buffer_flags & RENDERER_CLEAR_BUFFER_DEPTH ){
                    flags |= GL_DEPTH_BUFFER_BIT;
                }

                f32 r = current_command->info.clear_buffer.r;
                f32 g = current_command->info.clear_buffer.g;
                f32 b = current_command->info.clear_buffer.b;
                f32 a = current_command->info.clear_buffer.a;

                glClear( flags );
                glClearColor( r, g, b, a );
            }
            break;
            case RENDER_COMMAND_DRAW_QUAD:
            case RENDER_COMMAND_DRAW_TEXTURED_QUAD:
            {
                f32 r = current_command->info.quad.r;
                f32 g = current_command->info.quad.g;
                f32 b = current_command->info.quad.b;
                f32 a = current_command->info.quad.a;

                f32 square_x = current_command->info.quad.x;
                f32 square_y = current_command->info.quad.y;
                f32 square_w = current_command->info.quad.w;
                f32 square_h = current_command->info.quad.h;

                f32 square_uv_x = current_command->info.textured_quad.uv_x;
                f32 square_uv_y = current_command->info.textured_quad.uv_y;
                f32 square_uv_w = current_command->info.textured_quad.uv_w;
                f32 square_uv_h = current_command->info.textured_quad.uv_h;

                u32 vertex_info_stride = 9;

                if( current_command->command == RENDER_COMMAND_DRAW_TEXTURED_QUAD ){
                    game_asset* texture_asset = game_asset_get_from_handle( renderer->assets, current_command->info.textured_quad.texture_id );
                    if( texture_asset ){
                        // skip this one.
                        if( texture_asset->type == GAME_ASSET_NONE ) {
                            continue;
                        }

                        if( texture_asset->type == GAME_ASSET_BITMAP ){
                            GLuint texture_id = texture_asset->bitmap.renderer_id;
                            glBindTexture( GL_TEXTURE_2D, texture_id );
                        }
                    } else {
                        continue;
                    }
                }

                float square[9*3*2] = {
                    square_x, square_y,          0.0,          square_uv_x, square_uv_y, r, g, b, a,
                    square_x, square_y+square_h, 0.0,          square_uv_x, square_uv_y + square_uv_h, r, g, b, a,
                    square_x+square_w, square_y, 0.0,          square_uv_x + square_uv_w, square_uv_y, r, g, b, a,

                    square_x+square_w, square_y, 0.0,          square_uv_x + square_uv_w, square_uv_y, r, g, b, a,
                    square_x, square_y+square_h, 0.0,          square_uv_x, square_uv_y + square_uv_h, r, g, b, a,
                    square_x+square_w, square_y+square_h, 0.0, square_uv_x + square_uv_w, square_uv_y + square_uv_h, r, g, b, a
                };

                glBegin( GL_TRIANGLES );

                for( int i = 0; i < 6; ++i ){
                    f32 vertex_x = square[i*vertex_info_stride+0];
                    f32 vertex_y = square[i*vertex_info_stride+1];
                    f32 vertex_z = square[i*vertex_info_stride+2];

                    // correction for projection which is top to bottom vs bottom to top.
                    f32 vertex_u = square[i*vertex_info_stride+3];
                    f32 vertex_v = -square[i*vertex_info_stride+4];

                    f32 vertex_r = square[i*vertex_info_stride+5];
                    f32 vertex_g = square[i*vertex_info_stride+6];
                    f32 vertex_b = square[i*vertex_info_stride+7];
                    f32 vertex_a = square[i*vertex_info_stride+8];

                    glTexCoord2f(vertex_u, vertex_v);
                    glColor4f(vertex_r, vertex_g, vertex_b, vertex_a);
                    glVertex3f(vertex_x, vertex_y, vertex_z);
                }

                glEnd();

                if( current_command->command == RENDER_COMMAND_DRAW_TEXTURED_QUAD ){
                    glBindTexture( GL_TEXTURE_2D, 0 );
                }
            }
            break;
            /*scissor test does not seem to scissor*/
            /*TODO fix.*/
            case RENDER_COMMAND_SCISSOR_ENABLE:
            {
                u32 scissor_x = current_command->info.scissor.x;
                u32 scissor_y = current_command->info.scissor.y;
                u32 scissor_w = current_command->info.scissor.w;
                u32 scissor_h = current_command->info.scissor.h;

                glEnable( GL_SCISSOR_TEST );
                glScissor( scissor_x, 
                           renderer->screen_height - scissor_h - scissor_y, 
                           scissor_w, 
                           scissor_h );
            }
            break;
            case RENDER_COMMAND_SCISSOR_DISABLE:
            {
                glDisable( GL_SCISSOR_TEST );
            }
            break;
            case RENDER_COMMAND_DRAW_TEXT:
            {
                char* text = current_command->info.text.text;
                f32 x = current_command->info.text.x;
                f32 y = current_command->info.text.y;

                f32 scale = current_command->info.text.scale;
                f32 r = current_command->info.text.r;
                f32 g = current_command->info.text.g;
                f32 b = current_command->info.text.b;
                f32 a = current_command->info.text.a;

                b32 word_wrap = current_command->info.text.wrapped;
                b32 page_wrap = current_command->info.text.paged;

                u16 justification_type = current_command->info.text.justification_type;

                game_asset_handle font_handle = current_command->info.text.font_id;
                game_asset* font_asset = game_asset_get_from_handle( renderer->assets, font_handle );
                // broken?
                if( font_asset->type != GAME_ASSET_FONT ){ continue; }
                GLuint font_id = font_asset->font.renderer_id;

                // So we have the right baseline
                y += font_asset->font.size;

                struct font_entry* font = &renderer->fonts.list[font_id];

                u32 glyph_range_start = font->glyph_range_start;
                u32 glyph_range_end = font->glyph_range_end;


                f32 format_rect_x = current_command->info.text.format_rect.x;
                f32 format_rect_y = current_command->info.text.format_rect.y;
                f32 format_rect_w = current_command->info.text.format_rect.w;
                f32 format_rect_h = current_command->info.text.format_rect.h;

                if( justification_type != TEXT_COMMAND_JUSTIFICATION_NONE ){
                    f32 justified_x = format_rect_x;
                    f32 justified_y = format_rect_y;

                    if( justification_type & TEXT_COMMAND_JUSTIFICATION_CENTER ){
                        /*Centered justification*/
                        f32 text_width;
                        f32 text_height;

                        renderer_get_text_size( renderer, font_handle, text, &text_width, &text_height, NULL );

                        f32 bounds_width_half_sized = format_rect_w / 2;
                        f32 bounds_height_half_sized = format_rect_h / 2;

                        f32 middle_x = format_rect_x + bounds_width_half_sized;
                        f32 middle_y = format_rect_y + bounds_height_half_sized;

                        f32 text_width_half_sized = text_width / 2.f;
                        f32 text_height_half_sized = text_height / 2.f;

                        justified_x = middle_x - text_width_half_sized;
                        // adjusted for baseline.
                        justified_y = middle_y + text_height_half_sized / 2.0f;
                    }

                    if( justification_type & TEXT_COMMAND_JUSTIFICATION_RIGHT ){
                        f32 text_width;
                        f32 text_height;

                        renderer_get_text_size( renderer, font_handle, text, &text_width, &text_height, NULL );

                        f32 right_x = format_rect_x + format_rect_w;

                        justified_x = right_x - text_width;
                    }

                    if( justification_type & TEXT_COMMAND_JUSTIFICATION_BOTTOM ){
                        f32 text_width;
                        f32 text_height;

                        renderer_get_text_size( renderer, font_handle, text, &text_width, &text_height, NULL );

                        f32 bottom_y = format_rect_y + format_rect_h;

                        justified_y = bottom_y;
                    }

                    x = justified_x;
                    y = justified_y;
                }

                f32 original_x = x;
                f32 original_y = y;

                if(word_wrap){
                    original_x = format_rect_x;
                    original_y = format_rect_y;
                }

                while( *text ){
                    // special case.
                    b32 found_new_line = (*text == '\n');
                    b32 should_line_wrap =
                    (word_wrap) && (x >= original_x + format_rect_w);

                    if( found_new_line || should_line_wrap ){
                        y += font->size * scale;

                        if( page_wrap ){
                            if( y >= format_rect_h ){
                                original_x += format_rect_w + 15;
                                y = original_y;
                            }
                        }

                        x = original_x; 
                    }

                    if( *text >= glyph_range_start && *text < glyph_range_end && *text != '\n' ){
                        struct glyph_texture* current_glyph_texture = &font->glyphs[*text];

                        f32 left_bearing_in_pixels = current_glyph_texture->left_side_bearing *
                                                     current_glyph_texture->glyph_scale_factor;

                        f32 advance_in_pixels = current_glyph_texture->advance_width *
                                                current_glyph_texture->glyph_scale_factor;

                        f32 width_in_pixels = current_glyph_texture->glyph_bitmap_width * 
                                              current_glyph_texture->glyph_scale_factor;

                        f32 height_in_pixels = current_glyph_texture->glyph_bitmap_height * 
                                               current_glyph_texture->glyph_scale_factor;

                        f32 bitmap_bottom_in_pixels = current_glyph_texture->glyph_bitmap_bottom *
                                                      current_glyph_texture->glyph_scale_factor;

                        f32 glyph_top_left_x = (x + left_bearing_in_pixels) * scale;
                        f32 glyph_top_left_y = (y + bitmap_bottom_in_pixels) * scale;

                        f32 glyph_top_right_x = (glyph_top_left_x + width_in_pixels * scale);
                        f32 glyph_top_right_y = glyph_top_left_y;

                        f32 glyph_bottom_left_x = (x + left_bearing_in_pixels) * scale;
                        f32 glyph_bottom_left_y = ((y + bitmap_bottom_in_pixels) - height_in_pixels) * scale;

                        f32 glyph_bottom_right_x = glyph_bottom_left_x + width_in_pixels * scale;
                        f32 glyph_bottom_right_y = glyph_bottom_left_y;

                        glBindTexture( GL_TEXTURE_2D, current_glyph_texture->texture_id );
                        glBegin( GL_QUADS );
                        {
                            glColor4f( r, g, b, a );
                            glTexCoord2f( 0, 0 );
                            glVertex2f( glyph_bottom_left_x, glyph_bottom_left_y );

                            glTexCoord2f( 0, 1 );
                            glColor4f( r, g, b, a );
                            glVertex2f( glyph_top_left_x, glyph_top_left_y );

                            glColor4f( r, g, b, a );
                            glTexCoord2f( 1, 1 );
                            glVertex2f( glyph_top_right_x, glyph_top_right_y );

                            glColor4f( r, g, b, a );
                            glTexCoord2f( 1, 0 );
                            glVertex2f( glyph_bottom_right_x, glyph_bottom_right_y );
                        }
                        glEnd();
                        x += advance_in_pixels;
                    }

                    text++;
                }
                glBindTexture(GL_TEXTURE_2D, 0);
            }
            break;
            case RENDER_COMMAND_LOAD_TEXTUREMATRIX:
            case RENDER_COMMAND_LOAD_PROJECTIONMATRIX:
            case RENDER_COMMAND_LOAD_MODELVIEWMATRIX:
            {
                f32* new_matrix = current_command->info.matrix.matrix;

                if( current_command->command == RENDER_COMMAND_LOAD_TEXTUREMATRIX ){
                    memcpy( layer->texture_matrix, 
                            new_matrix, 
                            sizeof( layer->texture_matrix ) );
                }else if( current_command->command == RENDER_COMMAND_LOAD_PROJECTIONMATRIX ){
                    memcpy( layer->projection_matrix, 
                            new_matrix, 
                            sizeof( layer->projection_matrix ) );
                }else if( current_command->command == RENDER_COMMAND_LOAD_MODELVIEWMATRIX ){
                    memcpy( layer->modelview_matrix, 
                            new_matrix, 
                            sizeof( layer->modelview_matrix ) );
                }
            }
            break;
        }
    }
}
/*NOTE(jerry): end of opengl1_renderer.c*/
