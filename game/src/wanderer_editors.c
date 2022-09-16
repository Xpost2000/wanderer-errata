/*
 * TODO(jerry):
 * Renderer Note: 
 * I need to be able to unload the current texture,
 * otherwise it's a memory leak.
 *
 * - New actor model format that supports multiple directions
 *   and fixed animation sets:
 *
 *   WALK,
 *   IDLE,
 *   IDLE_ATTACK_READY,
 *   ATTACK,
 *   SPELL_CAST,
 *   DYING,
 *   DEAD,
 *   INTERACT
 *
 *   or something like that anyways.
 *
 */

#define TEST_SCENE_TILEMAP_WIDTH 5
#define TEST_SCENE_TILEMAP_HEIGHT 5

static char* default_working_directory = "data/";

// The test scene should probably draw from a real in-game situation
// with real game actors that are swapped to use the model currently
// being editted..
#include "config.h"
#include "wanderer_assets.h"

void actor_model_write_to_disk( game_assets* assets,
                                actor_model* model,
                                char* as_name ){
    config_info actor_model_config_info = {};

    config_info_push_variable( &actor_model_config_info,
                               config_variable_new_string( "name", model->name ) );
    config_info_push_variable( &actor_model_config_info,
                               config_variable_new_symbol( "logical_scale", actor_model_scale_strings[model->model_scale] ) );
    config_info_push_variable( &actor_model_config_info,
                               config_variable_new_number_from_f32( "draw_scale", model->draw_scale ) );
    
    for( unsigned animation_group_index = 0;
         animation_group_index < ACTOR_ANIMATION_GROUP_COUNT;
         ++animation_group_index ){
        actor_model_animation_group* current_animation_group =
            &model->animation_groups[ animation_group_index ];

        config_variable animation_group_block =
            config_variable_new_block( actor_model_animation_group_names_strings[animation_group_index] );
        
        for( unsigned animation_set_index = 0;
             animation_set_index < current_animation_group->animation_set_count;
             ++animation_set_index ){
            actor_model_animation_set* current_animation_set =
                &current_animation_group->animation_sets[ animation_set_index ];

            // TODO(jerry): only autogenerate set names. Never use the "defined" ones.
            config_variable animation_set_block;
            {
                char generated_name[128] = {};
                snprintf(generated_name, 255, "anim_group_%d", animation_set_index);
                char* name_of_set = current_animation_set->name;

                if( strlen(name_of_set) == 0 ){
                    name_of_set = generated_name;
                }

                animation_set_block =
                    config_variable_new_block( current_animation_set->name );
            }


            // list of enabled bits.
            config_variable flags_list = config_variable_new_list( "flags" );
            for( unsigned bit = 0; bit < 8; ++bit ){
                if( (current_animation_set->flags) & (1 << bit) ){
                    config_variable_list_push_item( flags_list.value.as_list,
                                                    config_variable_value_new_symbol( actor_model_animation_set_flag_strings[bit] ) );
                }
            }
            config_variable_block_push_item( animation_set_block.value.as_block, flags_list );

            config_variable_block_push_item( animation_set_block.value.as_block,
                                             config_variable_new_symbol( "direction",
                                                                         actor_model_animation_set_direction_strings[current_animation_set->direction] ));

            game_asset* spritesheet_asset =
                game_asset_get_from_handle(assets, current_animation_set->spritesheet_id);
            {

                if( spritesheet_asset->type == GAME_ASSET_NONE ){
                    // nothing...
                }else if( spritesheet_asset->type == GAME_ASSET_SPRITESHEET ){
                    const char* pathname_of_asset = spritesheet_asset->name;

                    config_variable_block_push_item( animation_set_block.value.as_block,
                                                     config_variable_new_string("spritesheet_file",
                                                                                pathname_of_asset) );
                }
            }

            for( unsigned animation_frame_index = 0;
                 animation_frame_index < current_animation_set->animation_frame_count;
                 ++animation_frame_index ){
                actor_model_animation_frame* current_animation_frame =
                    &current_animation_set->frames[ animation_frame_index ];

                char generated_name[128] = {};
                snprintf(generated_name, 255, "anim_frame_%d", animation_frame_index);
                char* name_of_frame = current_animation_frame->name;

                if( strlen(name_of_frame) == 0 ){
                    name_of_frame = generated_name;
                }

                config_variable animation_frame_block =
                    config_variable_new_block(name_of_frame);

                // ANIMATION_FRAME_PROPERTY INSERTION
                {
                    config_variable_block_push_item( animation_frame_block.value.as_block,
                                                     config_variable_new_number_from_f32( "origin_x",
                                                                                          current_animation_frame->origin_x ) );
                    config_variable_block_push_item( animation_frame_block.value.as_block,
                                                     config_variable_new_number_from_f32( "origin_y",
                                                                                          current_animation_frame->origin_y ) );
                    config_variable_block_push_item( animation_frame_block.value.as_block,
                                                     config_variable_new_number_from_f32( "time_to_next_frame",
                                                                                          current_animation_frame->time_to_next_frame ));
                }

                if( spritesheet_asset->type == GAME_ASSET_SPRITESHEET ){
                    // I know spritesheet_index is an u32
                    config_variable_block_push_item( animation_frame_block.value.as_block,
                                                     config_variable_new_number_from_i32( "spritesheet_index",
                                                                                          current_animation_frame->spritesheet_index ) );
                }else{
                    game_asset* texture_asset =
                        game_asset_get_from_handle(assets, current_animation_frame->texture_id);
                    if( texture_asset->type == GAME_ASSET_BITMAP ){
                        config_variable_block_push_item( animation_frame_block.value.as_block,
                                                         config_variable_new_string( "texture_file",
                                                                                     texture_asset->name ) );
                    }
                }
                fprintf(stderr, "item count of animation_frame : %d\n", animation_frame_block.value.as_block->count);
                config_variable_block_push_item( animation_set_block.value.as_block,
                                                 animation_frame_block );
            }

            fprintf(stderr, "item count of animation_set : %d\n", animation_set_block.value.as_block->count);
            config_variable_block_push_item( animation_group_block.value.as_block,
                                             animation_set_block );
        }

        config_info_push_variable( &actor_model_config_info,
                                   animation_group_block );
    }

    config_info_dump_as_text_to_stderr( &actor_model_config_info );
    config_info_dump_as_text_into_file( &actor_model_config_info, as_name );
    config_info_free_all_fields( &actor_model_config_info );
}

#define generate_enum_from_string_fn( enum_name, counter )                         \
    static u32 get_##enum_name##_from_string( const char* string ){ \
        for( u32 string_index = 0;                                      \
             string_index < counter;                                    \
             ++string_index ){                                          \
            if( strcmp( string, enum_name##_strings[string_index] ) == 0 ){ \
                return string_index;                                    \
            }                                                           \
        }                                                               \
        return 0;                                                       \
    }

generate_enum_from_string_fn(actor_model_animation_set_flag, ANIMATION_SET_FLAG_COUNT)
generate_enum_from_string_fn(actor_model_animation_set_direction, ANIMATION_SET_DIRECTION_COUNT)
generate_enum_from_string_fn(actor_model_scale, ACTOR_MODEL_SCALE_COUNT)
generate_enum_from_string_fn(actor_model_animation_group_names, ACTOR_ANIMATION_GROUP_COUNT)

// The asset system will store it's own version of the models which will
// actually be used for stuff.
// It will use the same parsing code probably...
// TODO(jerry): Make this clear the current model, and build a new one.
void actor_model_load_from_disk( game_assets* assets,
                                 actor_model* model,
                                 char* file_name ){
    config_info actor_model_info = config_parse_from_file( file_name );
    config_info_dump_as_text_to_stderr( &actor_model_info );

    config_variable* name =
        config_info_find_first_variable( &actor_model_info, "name" );
    config_variable* logical_scale =
        config_info_find_first_variable( &actor_model_info, "logical_scale" );
    config_variable* draw_scale =
        config_info_find_first_variable( &actor_model_info, "draw_scale" );

    b32 try_parse = true;
    if( !(name && logical_scale && draw_scale) ){
        try_parse = false;
    }

    // parse the free standing variables I know about.
    {
        if( name->value.type == CONFIG_VARIABLE_STRING ){
            strncpy( model->name,
                     name->value.value,
                     NEW_ACTOR_MODEL_NAME_SIZE );
        }else{
            try_parse = false;
        }

        if( logical_scale->value.type == CONFIG_VARIABLE_SYMBOL ){
            u8 actor_model_scale_string_index =
                get_actor_model_scale_from_string(logical_scale->value.value);

            model->model_scale = actor_model_scale_string_index;
        }else{
            try_parse = false;
        }

        // Should I care about doing real symbol comparison like lisp?
        // (pointer thingy?)
        if( draw_scale->value.type == CONFIG_VARIABLE_NUMBER ){
            model->draw_scale = config_variable_try_float( draw_scale );
        }else{
            try_parse = false;
        }
    }

    //NOTE(jerry): This one's a bit of a doozy.
    if( try_parse ){
        for( unsigned variable_index = 0;
             variable_index < actor_model_info.count;
             ++variable_index ){
            config_variable* current_variable =
                &actor_model_info.variables[variable_index];
            // skip the variables we already parsed.
            if( current_variable == name ||
                current_variable == logical_scale ||
                current_variable == draw_scale ){
                continue;
            }else{
                // This should be an animation group block.
                u8 animation_group_index =
                    get_actor_model_animation_group_names_from_string(current_variable->name);
                actor_model_animation_group* selected_animation_group =
                    &model->animation_groups[animation_group_index];
                // parse animation set blocks.
                {
                    // this is the animation_set block... Yeah I need to split this up.
                    // this is unreadable.
                    config_variable_value* value = &current_variable->value;
                    config_variable_block* block_content = value->as_block;

#ifdef DEBUG_BUILD
                    fprintf(stderr, "Should be animation_group dump:\n\n");
                    config_variable_dump_as_text_to_stderr( current_variable );
#endif

                    for( unsigned animation_set_block_index = 0;
                         animation_set_block_index < block_content->count;
                         ++animation_set_block_index ){
                        actor_model_animation_set* new_animation_set =
                            actor_model_animation_group_push_blank_animation_set( selected_animation_group );

                        config_variable* animation_set_block = &block_content->items[animation_set_block_index];
                        config_variable_block* animation_set_block_content = animation_set_block->value.as_block;

                        strncpy( new_animation_set->name, animation_set_block->name, NEW_ACTOR_MODEL_NAME_SIZE );

                        config_variable* animation_set_flags = config_block_find_first_variable( animation_set_block_content, "flags" );
                        config_variable* animation_set_direction = config_block_find_first_variable( animation_set_block_content, "direction" );
                        config_variable* animation_set_spritesheet_file = config_block_find_first_variable( animation_set_block_content, "spritesheet_file" );

                        if( animation_set_flags ){
                            if( animation_set_flags->value.type == CONFIG_VARIABLE_LIST ){
                                config_variable_list* list_of_flags = animation_set_flags->value.as_list;

                                u8 bit_flags = 0;
                                for( unsigned flag_list_item_index = 0;
                                     flag_list_item_index < list_of_flags->count;
                                     ++flag_list_item_index ){
                                    config_variable_value* flag_list_item = &list_of_flags->items[ flag_list_item_index ];
                                    bit_flags |= (1 << (get_actor_model_animation_set_flag_from_string(flag_list_item->value)));
                                }
                                    
                                new_animation_set->flags = bit_flags;
                            }
                        }

                        if( animation_set_direction ){
                            new_animation_set->direction =
                                get_actor_model_animation_set_direction_from_string( animation_set_direction->value.value );
                        }

                        if( animation_set_spritesheet_file ){
                            char* spritesheet_file = animation_set_spritesheet_file->value.value;
                            new_animation_set->spritesheet_id =
                                game_asset_load_spritesheet( assets, spritesheet_file );
                        }

                        for( unsigned animation_set_block_item_index = 0;
                             animation_set_block_item_index < animation_set_block_content->count;
                             ++animation_set_block_item_index ){
                            config_variable* current_animation_set_block_item =
                                &animation_set_block_content->items[animation_set_block_item_index];

                            if( current_animation_set_block_item == animation_set_flags ||
                                current_animation_set_block_item == animation_set_direction ||
                                current_animation_set_block_item == animation_set_spritesheet_file ){
                                continue;
                            }else{
                                if( current_animation_set_block_item->value.type == CONFIG_VARIABLE_BLOCK ){
                                    actor_model_animation_frame* new_frame =
                                        actor_model_animation_set_push_blank_animation_frame( new_animation_set );
                                    strncpy( new_frame->name, current_animation_set_block_item->name, NEW_ACTOR_MODEL_NAME_SIZE );

                                    config_variable_block* actor_animation_frame_block_content =
                                        current_animation_set_block_item->value.as_block;

                                    for( unsigned animation_frame_block_item_index = 0;
                                         animation_frame_block_item_index < actor_animation_frame_block_content->count;
                                         ++animation_frame_block_item_index ){
                                        config_variable* animation_frame_origin_x =
                                            config_block_find_first_variable( actor_animation_frame_block_content, "origin_x" );
                                        config_variable* animation_frame_origin_y =
                                            config_block_find_first_variable( actor_animation_frame_block_content, "origin_y" );
                                        config_variable* animation_frame_time_to_next_frame =
                                            config_block_find_first_variable( actor_animation_frame_block_content, "time_to_next_frame ");

                                        if( animation_frame_origin_x ){
                                            if( animation_frame_origin_x->value.type == CONFIG_VARIABLE_NUMBER ){
                                                new_frame->origin_x = config_variable_try_float( animation_frame_origin_x );
                                            }
                                        }

                                        if( animation_frame_origin_y ){
                                            if( animation_frame_origin_y->value.type == CONFIG_VARIABLE_NUMBER ){
                                                new_frame->origin_y = config_variable_try_float( animation_frame_origin_y );
                                            }
                                        }

                                        if( animation_frame_time_to_next_frame ){
                                            if( animation_frame_time_to_next_frame->value.type == CONFIG_VARIABLE_NUMBER ){
                                                new_frame->time_to_next_frame = config_variable_try_float( animation_frame_time_to_next_frame );
                                            }
                                        }

                                        config_variable* animation_frame_texture_resource =
                                            config_block_find_first_variable( actor_animation_frame_block_content, "spritesheet_index" );

                                        if( animation_frame_texture_resource ){
                                            // spritesheet index
                                            if( animation_frame_texture_resource->value.type == CONFIG_VARIABLE_NUMBER ){
                                                new_frame->spritesheet_index = config_variable_try_integer( animation_frame_texture_resource );
                                            }
                                        }else{
                                            // texture_file
                                            animation_frame_texture_resource =
                                                config_block_find_first_variable( actor_animation_frame_block_content, "texture_file" );
                                            if( animation_frame_texture_resource ){
                                                if( animation_frame_texture_resource->value.type == CONFIG_VARIABLE_STRING ){
                                                    new_frame->texture_id =
                                                        game_asset_load_bitmap( assets, animation_frame_texture_resource->value.value );
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    config_info_free_all_fields( &actor_model_info );
}

static void editor_open_file_browser( game_model_editor_state* state, char* working_directory ){
    state->file_browser_open = true;
    strncpy( state->working_directory, working_directory, PLATFORM_FILE_NAME_MAX);
}

static void model_editor_draw_model( game_state* state,
        game_model_editor_state* editor_state ) {
    actor_model* model = &editor_state->current_actor_model;

    actor_model_animation_group* animation_group = &model->animation_groups[ editor_state->current_animation_group ];
    actor_model_animation_set* animation_set = 0;
    actor_model_animation_frame* animation_frame = 0;

    if( animation_group->animation_set_count ){
        if( editor_state->model_scene_type == GAME_MODEL_EDITOR_TEST_SCENE_ISOLATED ){
            animation_set = &animation_group->animation_sets[ editor_state->current_animation_set ];
            if( animation_set->animation_frame_count ){
                animation_frame = &animation_set->frames[ editor_state->current_frame ];
            }
        }else{
            vec2 lookat = v2( editor_state->lookat_target_direction_x, editor_state->lookat_target_direction_y );
            u32 best_animation_set_index = actor_model_animation_group_find_animation_set_for_direction( animation_group, lookat );

            animation_set = &animation_group->animation_sets[ best_animation_set_index ];

            if( animation_set->animation_frame_count ){
                animation_frame = &animation_set->frames[ editor_state->current_frame ];
            }
        }
    }

    // draw current model
    if( animation_frame ){
        f32 x_pixels;
        f32 y_pixels;
        f32 w_pixels;
        f32 h_pixels;

        render_command drawn_frame = {};

        game_asset* spritesheet_asset = 
            game_asset_get_from_handle( &state->assets, animation_set->spritesheet_id );

        u32 frame_width = 0;
        u32 frame_height = 0;

        if( spritesheet_asset->type == GAME_ASSET_NONE ){
            game_asset* frame_bitmap_asset = game_asset_get_from_handle( &state->assets,
                                                                         animation_frame->texture_id );
            frame_width = frame_bitmap_asset->bitmap.width;
            frame_height = frame_bitmap_asset->bitmap.height;
        }else if( spritesheet_asset->type == GAME_ASSET_SPRITESHEET ){
            game_asset_spritesheet_rect sprite_rect = 
                spritesheet_asset->spritesheet.sprite_rects[animation_frame->spritesheet_index];
            frame_width = sprite_rect.w;
            frame_height = sprite_rect.h;
        }

        w_pixels = (frame_width * model->draw_scale);
        h_pixels = (frame_height * model->draw_scale);
        
        // This will result in points that are always on the isometric grid.
        vec2 pixels_as_isometric = project_cartesian_to_isometric( 
                (vec2)
                {
                    2 * half_tile_width,
                    2 * half_tile_width,
                });
        {
            // pivot calculations
            {
                f32 pivot_x = (w_pixels) * animation_frame->origin_x;
                f32 pivot_y = (h_pixels) * animation_frame->origin_y;

                x_pixels = (pixels_as_isometric.x) - (pivot_x);
                y_pixels = (pixels_as_isometric.y) - (pivot_y);
            }
        }

        if( spritesheet_asset->type == GAME_ASSET_NONE ){
            drawn_frame = render_command_textured_quad( x_pixels,
                                                        y_pixels,
                                                        w_pixels,
                                                        h_pixels,
                                                        1, 1, 1, 1,
                                                        animation_frame->texture_id );
        }else if( spritesheet_asset->type == GAME_ASSET_SPRITESHEET ){
            game_asset_spritesheet_rect sprite_rect = 
                spritesheet_asset->spritesheet.sprite_rects[animation_frame->spritesheet_index];
            drawn_frame = render_command_textured_quad( x_pixels,
                                                        y_pixels,
                                                        w_pixels,
                                                        h_pixels,
                                                        1, 1, 1, 1,
                                                        spritesheet_asset->spritesheet.matching_bitmap );
            game_asset* spritesheet_bitmap_asset = 
                game_asset_get_from_handle( &state->assets, 
                        spritesheet_asset->spritesheet.matching_bitmap );
            f32 spritesheet_width = spritesheet_bitmap_asset->bitmap.width;
            f32 spritesheet_height = spritesheet_bitmap_asset->bitmap.height;
            drawn_frame.info.textured_quad.uv_x = sprite_rect.x / spritesheet_width;
            drawn_frame.info.textured_quad.uv_y = sprite_rect.y / spritesheet_height;
            drawn_frame.info.textured_quad.uv_w = sprite_rect.w / spritesheet_width;
            drawn_frame.info.textured_quad.uv_h = sprite_rect.h / spritesheet_height;
        }

        render_layer_push_command( &state->game_layer, drawn_frame );

        render_layer_push_command( &state->game_layer, 
                render_command_quad( 
                    pixels_as_isometric.x,
                    pixels_as_isometric.y,
                    2,
                    2,
                    1, 1, 1, 1
                    ) );
    }
}

static void model_editor_draw_test_scene( game_state* state,
                                          renderer* renderer,
                                          game_input* input,
                                          f32 delta_time ){
    game_model_editor_state* editor_state = &state->model_editor;
    render_layer_camera_set_position( &state->game_layer, 300, 500 );
    render_layer_camera_set_scale( &state->game_layer, state->camera.scale );

    static char test_tilemap[TEST_SCENE_TILEMAP_HEIGHT][TEST_SCENE_TILEMAP_WIDTH] = {
        {1, 1, 1, 1, 1},
        {1, 0, 0, 0, 1},
        {1, 0, 0, 0, 1},
        {1, 0, 0, 0, 1},
        {1, 1, 1, 1, 1},
    };

    for( unsigned y = 0; y < TEST_SCENE_TILEMAP_HEIGHT; ++y ){
        for( unsigned x = 0; x < TEST_SCENE_TILEMAP_WIDTH; ++x ){
            game_asset_handle tile_texture = {};

            f32 pos_x;
            f32 pos_y;

            f32 draw_x;
            f32 draw_y;
            // hard coding this... Not happy about it but
            // what can I do?
            u32 image_width = 32;
            u32 image_height = 64;

            f32 draw_w = image_width;
            f32 draw_h = image_height;
            const f32 half_draw_w = (draw_w * 0.5);

            {
                vec2 map_to_iso_units = project_cartesian_to_isometric( 
                        (vec2)
                        {
                        x * half_draw_w,
                        y * half_draw_w
                        });

                vec2 pixels_as_isometric = map_to_iso_units;

                f32 pivot_x = (draw_w) * 0.5;
                f32 pivot_y = (draw_h) * 0.835;

                pos_x = pixels_as_isometric.x;
                pos_y = pixels_as_isometric.y;

                draw_x = pixels_as_isometric.x - (pivot_x);
                draw_y = pixels_as_isometric.y - (pivot_y);
            }

            switch( test_tilemap[y][x] ){
                case 1:
                {
                    tile_texture = state->dawnblocker_block;

                    render_command draw_command = 
                        render_command_textured_quad( 
                                draw_x, 
                                draw_y, 
                                draw_w,
                                draw_h,
                                1, 0, 0, 1, tile_texture );
                    render_layer_push_command( &state->game_layer, draw_command ); 
                }
                break;
                case 0:
                {
                    tile_texture = state->dawnblocker_floor;
                    render_command draw_command =
                        render_command_textured_quad( 
                                draw_x,
                                draw_y,
                                draw_w,
                                draw_h,
                                0, 0.2, 0.11, 1, tile_texture );
                    render_layer_push_command( &state->game_layer, draw_command );
                }
                break;
            }
        }
    }

    model_editor_draw_model( state, editor_state );
}

static void model_editor_draw_isolated_scene( game_state* state,
                                              renderer* renderer,
                                              game_input* input,
                                              f32 delta_time ){
    game_model_editor_state* editor_state = &state->model_editor;
    render_layer_camera_set_position( &state->game_layer, 225, 500 );
    render_layer_camera_set_scale( &state->game_layer, state->camera.scale );
    model_editor_draw_model( state, editor_state );
}

static void model_editor_update_render_file_selection_gui( game_state* state,
                                                           renderer* renderer,
                                                           game_input* input,
                                                           actor_model* model,
                                                           actor_model_animation_group* animation_group,
                                                           actor_model_animation_set* animation_set,
                                                           actor_model_animation_frame* animation_frame,
                                                           f32 delta_time ){
    game_model_editor_state* editor_state = &state->model_editor;
    char selected_path_directory[PLATFORM_FILE_NAME_MAX] = {};

    switch( gui_do_file_selector( &state->gui_state,
                                  &editor_state->file_browser_open,
                                  editor_state->working_directory,
                                  PLATFORM_FILE_NAME_MAX,
                                  selected_path_directory, 
                                  PLATFORM_FILE_NAME_MAX ) ){
        case GUI_RESULT_OKAY:
        {
            if( editor_state->file_browser_mode == MODEL_EDITOR_FILE_BROWSER_OPEN_MODEL_FRAME_MODE ||
                editor_state->file_browser_mode == MODEL_EDITOR_FILE_BROWSER_NEW_MODEL_FRAME_MODE ){
                fprintf(stderr, "Selected directory: %s", selected_path_directory);

                char* proper_file_name = memory_pool_allocate( &state->strings_scratch, 255 );
                snprintf(proper_file_name, 255, "%s/%s", editor_state->working_directory, selected_path_directory);
                fprintf(stderr, "loading %s\n", proper_file_name);

                strncpy( animation_frame->name, proper_file_name, PLATFORM_FILE_NAME_MAX );
                animation_frame->texture_id = game_asset_load_bitmap( &state->assets, proper_file_name );
                {
                    game_asset* texture_asset = game_asset_get_from_handle( &state->assets, animation_frame->texture_id );
                    animation_frame->width =  texture_asset->bitmap.width;
                    animation_frame->height = texture_asset->bitmap.height;
                }
            }

            if( editor_state->file_browser_mode == MODEL_EDITOR_FILE_BROWSER_OPEN_ANIMATION_SET_SPRITESHEET ){
                fprintf(stderr, "Selected directory: %s", selected_path_directory);

                char* proper_file_name = memory_pool_allocate( &state->strings_scratch, 255 );
                snprintf(proper_file_name, 255, "%s/%s", editor_state->working_directory, selected_path_directory);
                fprintf(stderr, "loading %s\n", proper_file_name);

                animation_set->spritesheet_id = game_asset_load_spritesheet( &state->assets, proper_file_name );
                // avoid crashing maybe.
                editor_state->current_frame = 0;
                editor_state->current_animation_set = 0;
            }

            if( editor_state->file_browser_mode == MODEL_EDITOR_FILE_BROWSER_READ_MODEL_MODE ){
            }
        }
        break;
        case GUI_RESULT_CANCEL:
        {
            if( editor_state->file_browser_mode == MODEL_EDITOR_FILE_BROWSER_NEW_MODEL_FRAME_MODE ){
                actor_model_animation_set_remove_animation_frame( animation_set, editor_state->current_frame );
            }
        }
        break;
        case GUI_RESULT_NONE:
        {
        }
        break;
    }
}

static void update_render_model_editor_main_screen( game_state* state,
                                                    renderer* renderer,
                                                    game_input* input,
                                                    actor_model* model,
                                                    actor_model_animation_group* animation_group,
                                                    actor_model_animation_set* animation_set,
                                                    actor_model_animation_frame* animation_frame,
                                                    f32 delta_time ){
    state->ui_info.mode = GAME_UI_STATE_NONE;
    game_model_editor_state* editor_state = &state->model_editor;

    gui_begin_vertical_layout( &state->gui_state, 40, 20, 5, 5 );
    gui_do_radio_button( &state->gui_state, "MDL EDIT SCENE",
            gui_layout_next_x( &state->gui_state ),
            gui_layout_next_y( &state->gui_state ),
            &editor_state->model_scene_type, 0 );
    gui_do_radio_button( &state->gui_state, "MDL TEST SCENE", 
            gui_layout_next_x( &state->gui_state ),
            gui_layout_next_y( &state->gui_state ),
            &editor_state->model_scene_type, 0 );
    char* current_target_lookat_string = memory_pool_allocate( &state->strings_scratch, 84 );
    snprintf(current_target_lookat_string, 84,
             "LOOK DIR (%3.3f, %3.3f) : %3.3f degs",
             editor_state->lookat_target_direction_x,
             editor_state->lookat_target_direction_y,
             angle_from_direction(editor_state->lookat_target_direction_y,
                                  editor_state->lookat_target_direction_x));
    {
        gui_do_draggable_vec2( &state->gui_state,
                current_target_lookat_string,
                gui_layout_next_x( &state->gui_state ),
                gui_layout_next_y( &state->gui_state ),
                &editor_state->lookat_target_direction_x,
                &editor_state->lookat_target_direction_y,
                -1, 1,
                -1, 1);
    }
    gui_begin_vertical_layout( &state->gui_state,
                               gui_layout_next_x( &state->gui_state ),
                               gui_layout_next_y( &state->gui_state ), 5, 5 );
    {
        gui_begin_horizontal_layout( &state->gui_state, 
                                     gui_layout_next_x( &state->gui_state ),
                                     gui_layout_next_y( &state->gui_state ),
                                     5, 5);
        for( unsigned i = 0; i < 3; ++i ){
            char* num_str = memory_pool_allocate( &state->strings_scratch, 16 );
            f32 angle = i * 45.0f;
            snprintf(num_str, 16, "%3.3f deg", angle);
            if( gui_do_button( &state->gui_state,
                               num_str,
                               gui_layout_next_x( &state->gui_state ),
                               gui_layout_next_y( &state->gui_state ) ) ){
                editor_state->lookat_target_direction_x = cosf(degrees_to_radians(angle));
                editor_state->lookat_target_direction_y = sinf(degrees_to_radians(angle));
            }
        }
        gui_end_layout( &state->gui_state );
        gui_layout_vertical_padding( &state->gui_state, 24.5f );
        gui_begin_horizontal_layout( &state->gui_state, 
                                     gui_layout_next_x( &state->gui_state ),
                                     gui_layout_next_y( &state->gui_state ),
                                     5, 5);
        for( unsigned i = 0; i < 3; ++i ){
            char* num_str = memory_pool_allocate( &state->strings_scratch, 16 );
            f32 angle = (i+3) * 45.0f;
            snprintf(num_str, 16, "%3.1f deg", angle);
            if( gui_do_button( &state->gui_state,
                               num_str,
                               gui_layout_next_x( &state->gui_state ),
                               gui_layout_next_y( &state->gui_state ) ) ){
                editor_state->lookat_target_direction_x = cosf(degrees_to_radians(angle));
                editor_state->lookat_target_direction_y = sinf(degrees_to_radians(angle));
            }
        }
        gui_end_layout( &state->gui_state );
        gui_layout_vertical_padding( &state->gui_state, 24.5f );
        gui_begin_horizontal_layout( &state->gui_state, 
                                     gui_layout_next_x( &state->gui_state ),
                                     gui_layout_next_y( &state->gui_state ),
                                     5, 5);
        for( unsigned i = 0; i < 3; ++i ){
            char* num_str = memory_pool_allocate( &state->strings_scratch, 16 );
            f32 angle = (i+6) * 45.0f;
            snprintf(num_str, 16, "%3.1f deg", angle);
            if( gui_do_button( &state->gui_state,
                               num_str,
                               gui_layout_next_x( &state->gui_state ),
                               gui_layout_next_y( &state->gui_state ) ) ){
                editor_state->lookat_target_direction_x = cosf(degrees_to_radians(angle));
                editor_state->lookat_target_direction_y = sinf(degrees_to_radians(angle));
            }
        }
        gui_end_layout( &state->gui_state );
    }
    gui_end_layout( &state->gui_state );
#if 0
    if( gui_do_button( &state->gui_state,
                "RESET DIRECTION",
                gui_layout_next_x( &state->gui_state ),
                gui_layout_next_y( &state->gui_state ) ) ){
        stub_important_code("RESET DIRECTION");
    }
    gui_end_layout( &state->gui_state );
#endif

    gui_begin_vertical_layout( &state->gui_state, state->screen_width - 560, 10, 5, 5 );
    static b32 show_model_properties = true;
    static b32 show_frame_properties = true;

    gui_layout_vertical_padding( &state->gui_state, 10.5f );
    gui_begin_horizontal_layout( &state->gui_state, 
            gui_layout_next_x( &state->gui_state ),
            gui_layout_next_y( &state->gui_state ),
            5, 5);
    {
        gui_text_element( &state->gui_state, "MODEL: ",
                gui_layout_next_x( &state->gui_state ),
                gui_layout_next_y( &state->gui_state ),
                1, 1, 1, 1 );
        gui_do_textline_edit( &state->gui_state, 
                model->name, NEW_ACTOR_MODEL_NAME_SIZE,
                gui_layout_next_x( &state->gui_state ),
                gui_layout_next_y( &state->gui_state ) - 20 );
    }
    gui_end_layout( &state->gui_state );
    gui_layout_vertical_padding( &state->gui_state, 10.5f );
    {
        char* model_current_anim_set_name_string = memory_pool_allocate( &state->strings_scratch, 255 );
        snprintf(model_current_anim_set_name_string, 255, "ANIM GROUP: %s", actor_model_animation_group_names_strings[editor_state->current_animation_group]);
        gui_text_element( &state->gui_state, 
                model_current_anim_set_name_string,
                gui_layout_next_x( &state->gui_state ),
                gui_layout_next_y( &state->gui_state ),
                1, 1, 1, 1 );
    }
    gui_begin_horizontal_layout( &state->gui_state, 
            gui_layout_next_x( &state->gui_state ),
            gui_layout_next_y( &state->gui_state ),
            5, 1);
    {
        if( gui_do_button( &state->gui_state,
                    "NEXT ANIM GROUP",
                    gui_layout_next_x( &state->gui_state ),
                    gui_layout_next_y( &state->gui_state ) ) ){
            editor_state->current_animation_group++;
        }

        if( gui_do_button( &state->gui_state,
                    "PREV ANIM GROUP",
                    gui_layout_next_x( &state->gui_state ),
                    gui_layout_next_y( &state->gui_state ) ) ){
            if( editor_state->current_animation_group != 0 ){
                editor_state->current_animation_group--;
            }
        }
    }
    gui_end_layout( &state->gui_state );
    gui_begin_horizontal_layout( &state->gui_state, 
            gui_layout_next_x( &state->gui_state ),
            gui_layout_next_y( &state->gui_state ),
            5, 5);
    {
        if( gui_do_button( &state->gui_state,
                    "+ ANIM SET",
                    gui_layout_next_x( &state->gui_state ),
                    gui_layout_next_y( &state->gui_state ) ) ){
            // switch to animation_set....
            // Find index?
            actor_model_animation_group_push_blank_animation_set( animation_group );
        }

        if( gui_do_button( &state->gui_state,
                    "- ANIM SET",
                    gui_layout_next_x( &state->gui_state ),
                    gui_layout_next_y( &state->gui_state ) ) ){
            actor_model_animation_group_remove_animation_set( animation_group, editor_state->current_animation_set );
        }
    }
    gui_end_layout( &state->gui_state );

    if( animation_set ){
        {
            gui_layout_vertical_padding( &state->gui_state, 10.0f );
            gui_begin_horizontal_layout( &state->gui_state, 
                    gui_layout_next_x( &state->gui_state ),
                    gui_layout_next_y( &state->gui_state ),
                    2, 1);
            gui_text_element( &state->gui_state, "ANIM SET: ",
                    gui_layout_next_x( &state->gui_state ),
                    gui_layout_next_y( &state->gui_state ),
                    1, 1, 1, 1 );
            gui_text_element( &state->gui_state, 
                                 animation_set->name,
                                 gui_layout_next_x( &state->gui_state ),
                                 gui_layout_next_y( &state->gui_state ),
                                 1, 1, 1, 1 );
            gui_end_layout( &state->gui_state );
        }
        if( animation_frame ){
            char* animation_frame_name = memory_pool_allocate( &state->strings_scratch, 255 );
            snprintf(animation_frame_name, "Model: %s", animation_frame->name);
            gui_text_element( &state->gui_state, animation_frame_name,
                    gui_layout_next_x( &state->gui_state ),
                    gui_layout_next_y( &state->gui_state ),
                    1, 1, 1, 1 );

#if 0
            char* current_frame_texture_width_string = memory_pool_allocate( &state->strings_scratch, 64 );
            char* current_frame_texture_height_string = memory_pool_allocate( &state->strings_scratch, 64 );

            snprintf(current_frame_texture_width_string, 64, "Frame Width: %d",
                    animation_frame->width);
            snprintf(current_frame_texture_height_string, 64, "Frame Height: %d",
                    animation_frame->height);

            gui_text_element( &state->gui_state, 
                    current_frame_texture_width_string,
                    gui_layout_next_x( &state->gui_state ),
                    gui_layout_next_y( &state->gui_state ),
                    1, 1, 1, 1 );

            gui_text_element( &state->gui_state, 
                    current_frame_texture_height_string,
                    gui_layout_next_x( &state->gui_state ),
                    gui_layout_next_y( &state->gui_state ),
                    1, 1, 1, 1 );
#endif

            char* current_frame_origin_string = memory_pool_allocate( &state->strings_scratch, 84 );
            snprintf(current_frame_origin_string, 84,
                    "ORIGIN XY % (%3.3f, %3.3f)",
                    animation_frame->origin_x,
                    animation_frame->origin_y );
            {
                gui_do_draggable_vec2( &state->gui_state,
                        current_frame_origin_string,
                        gui_layout_next_x( &state->gui_state ),
                        gui_layout_next_y( &state->gui_state ),
                        &animation_frame->origin_x,
                        &animation_frame->origin_y,
                        -2, 2,
                        -2, 2);
            }
            char* current_model_draw_scale_string = memory_pool_allocate( &state->strings_scratch, 84 );
            snprintf(current_model_draw_scale_string, 84,
                    "DRAW SCALE (%3.3f)",
                    model->draw_scale);
            {
                gui_do_draggable_float( &state->gui_state,
                        current_model_draw_scale_string,
                        gui_layout_next_x( &state->gui_state ), 
                        gui_layout_next_y( &state->gui_state ),
                        &model->draw_scale,
                        0, 4 );
            }
            gui_layout_vertical_padding( &state->gui_state, 3 );
            char* current_frame_time_to_next_frame_string = memory_pool_allocate( &state->strings_scratch, 64 );
            {
                snprintf(current_frame_time_to_next_frame_string,
                        64, "ANIM TIME(s): %3.3f",
                        animation_frame->time_to_next_frame);
                gui_do_draggable_float( &state->gui_state, 
                        current_frame_time_to_next_frame_string,
                        gui_layout_next_x( &state->gui_state ),
                        gui_layout_next_y( &state->gui_state ),
                        &animation_frame->time_to_next_frame,
                        0, 30.0f );
            }
        }
        gui_begin_horizontal_layout( &state->gui_state, 
                gui_layout_next_x( &state->gui_state ),
                gui_layout_next_y( &state->gui_state ),
                2, 2);
        {
            if( gui_do_button( &state->gui_state,
                        "+ ANIM FRAME",
                        gui_layout_next_x( &state->gui_state ),
                        gui_layout_next_y( &state->gui_state ) ) ){
                actor_model_animation_set_push_blank_animation_frame( animation_set );
                editor_state->current_frame++;

                game_asset* set_spritesheet = game_asset_get_from_handle( &state->assets, animation_set->spritesheet_id );
                if( set_spritesheet->type != GAME_ASSET_SPRITESHEET ){
                    editor_open_file_browser( editor_state, default_working_directory );
                    editor_state->file_browser_mode = MODEL_EDITOR_FILE_BROWSER_NEW_MODEL_FRAME_MODE;
                }
            }

            if( gui_do_button( &state->gui_state,
                        "- ANIM FRAME",
                        gui_layout_next_x( &state->gui_state ),
                        gui_layout_next_y( &state->gui_state ) ) ){
                actor_model_animation_set_remove_animation_frame( animation_set, editor_state->current_frame );
            }
        }
        gui_end_layout( &state->gui_state );
        gui_begin_horizontal_layout( &state->gui_state, 
                gui_layout_next_x( &state->gui_state ),
                gui_layout_next_y( &state->gui_state ),
                5, 5);
        {
            if( gui_do_button( &state->gui_state,
                        "NEXT ANIM SET",
                        gui_layout_next_x( &state->gui_state ),
                        gui_layout_next_y( &state->gui_state ) ) ){
                editor_state->current_animation_set++; 
            }

            if( gui_do_button( &state->gui_state,
                        "PREV ANIM SET",
                        gui_layout_next_x( &state->gui_state ),
                        gui_layout_next_y( &state->gui_state ) ) ){
                if( editor_state->current_animation_set != 0 ){
                    editor_state->current_animation_set--; 
                }
            }
        }
        gui_end_layout( &state->gui_state );

        gui_begin_horizontal_layout( &state->gui_state, 
                gui_layout_next_x( &state->gui_state ),
                gui_layout_next_y( &state->gui_state ),
                5, 5);
        {
            if( gui_do_button( &state->gui_state,
                        "NEXT ANIM FRAME",
                        gui_layout_next_x( &state->gui_state ),
                        gui_layout_next_y( &state->gui_state ) ) ){
                editor_state->current_frame++;
            }

            if( gui_do_button( &state->gui_state,
                        "PREV ANIM FRAME",
                        gui_layout_next_x( &state->gui_state ),
                        gui_layout_next_y( &state->gui_state ) ) ){
                if( editor_state->current_frame != 0 ){
                    editor_state->current_frame--;
                }
            }
        }
        gui_end_layout( &state->gui_state );
        gui_begin_horizontal_layout( &state->gui_state, 
                gui_layout_next_x( &state->gui_state ),
                gui_layout_next_y( &state->gui_state ),
                5, 5);
        {
            if( gui_do_button( &state->gui_state,
                        "DIRECTION FLAGS",
                        gui_layout_next_x( &state->gui_state ),
                        gui_layout_next_y( &state->gui_state ) ) ){
                editor_state->flag_selection_screen_open = true;
                editor_state->flag_selection_mode = EDITOR_STATE_FLAG_SELECTION_SCREEN_ANIMATION_SET_DIRECTION_FLAGS;
            }

            if( gui_do_button( &state->gui_state,
                        "FLAGS & LOGIC SCALE",
                        gui_layout_next_x( &state->gui_state ),
                        gui_layout_next_y( &state->gui_state ) ) ){
                editor_state->flag_selection_screen_open = true;
                editor_state->flag_selection_mode = EDITOR_STATE_FLAG_SELECTION_SCREEN_ANIMATION_SET_FLAGS;
            }
        }
        gui_end_layout( &state->gui_state );
        if( gui_do_button( &state->gui_state,
                    "APPLY ALIGNMENT TO ALL FRAMES",
                    gui_layout_next_x( &state->gui_state ),
                    gui_layout_next_y( &state->gui_state ) ) ){
            stub_less_important("Not done, apply alignment to all frames.");
        }

        if( gui_do_button( &state->gui_state,
                    "APPLY ALIGNMENT TO ALL ANIM SETS",
                    gui_layout_next_x( &state->gui_state ),
                    gui_layout_next_y( &state->gui_state ) ) ){
            stub_less_important("Not done, apply alignment to all frames.");
        }

        if( gui_do_button( &state->gui_state,
                    "APPLY ALIGNMENT TO ALL ANIM GROUPS",
                    gui_layout_next_x( &state->gui_state ),
                    gui_layout_next_y( &state->gui_state ) ) ){
            stub_less_important("Not done, apply alignment to all frames.");
        }
    }

    gui_end_layout( &state->gui_state );

    gui_begin_horizontal_layout( &state->gui_state, 
            15, state->screen_height - 30,
            5, 5);
    // draw file toolbar
    {
        if( gui_do_button( &state->gui_state,
                    "LOAD",
                    gui_layout_next_x( &state->gui_state ),
                    gui_layout_next_y( &state->gui_state ) ) ){
            actor_model_clear_all_data( &editor_state->current_actor_model );
            actor_model_load_from_disk( &state->assets,
                    &editor_state->current_actor_model,
                    "asset_editor/models/test.mdl" );
        }

        if( gui_do_button( &state->gui_state,
                    "SAVE",
                    gui_layout_next_x( &state->gui_state ),
                    gui_layout_next_y( &state->gui_state ) ) ){
            actor_model_write_to_disk( &state->assets,
                    &editor_state->current_actor_model,
                    "asset_editor/models/test.mdl" );
        }

        if( gui_do_button( &state->gui_state,
                    "LOAD IMG",
                    gui_layout_next_x( &state->gui_state ),
                    gui_layout_next_y( &state->gui_state ) ) ){
            editor_open_file_browser( editor_state, default_working_directory );
            editor_state->file_browser_mode = MODEL_EDITOR_FILE_BROWSER_OPEN_MODEL_FRAME_MODE;
        }

        if( gui_do_button( &state->gui_state,
                    "LOAD SPRITESHEET",
                    gui_layout_next_x( &state->gui_state ),
                    gui_layout_next_y( &state->gui_state ) ) ){
            editor_open_file_browser( editor_state, default_working_directory );
            editor_state->file_browser_mode = MODEL_EDITOR_FILE_BROWSER_OPEN_ANIMATION_SET_SPRITESHEET;
        }

        // TODO(jerry): Make a scrolling atlas selector thing.
        if( animation_set ){
            game_asset* set_spritesheet = game_asset_get_from_handle( &state->assets, animation_set->spritesheet_id );
            if( set_spritesheet ){
                if( set_spritesheet->type == GAME_ASSET_SPRITESHEET ){
                    if( gui_do_button( &state->gui_state,
                                       "+ATLAS INDEX",
                                       gui_layout_next_x( &state->gui_state ),
                                       gui_layout_next_y( &state->gui_state ) ) ){
                        if( animation_frame ){
                            animation_frame->spritesheet_index++;
                        }
                    }

                    if( gui_do_button( &state->gui_state,
                                       "-ATLAS INDEX",
                                       gui_layout_next_x( &state->gui_state ),
                                       gui_layout_next_y( &state->gui_state ) ) ){
                        if( animation_frame ){
                            if( animation_frame->spritesheet_index != 0 ){
                                animation_frame->spritesheet_index--;
                            }
                        }
                    }

                    if( animation_frame ){
                        animation_frame->spritesheet_index =
                            u32_clamp(animation_frame->spritesheet_index, 0,
                                      set_spritesheet->spritesheet.sprite_rect_count-1);
                    }
                }
            }
        }
    }

    gui_end_layout( &state->gui_state );
}

static void update_render_model_editor_flag_selection_screen_animation_set_direction_flags( 
        game_state* state,
        renderer* renderer,
        game_input* input,
        actor_model* model,
        actor_model_animation_group* animation_group,
        actor_model_animation_set* animation_set,
        actor_model_animation_frame* animation_frame,
        f32 delta_time ){
    game_model_editor_state* editor_state = &state->model_editor;

    if( animation_set ){
        gui_begin_vertical_layout( &state->gui_state, 100, 20, 5, 5 );

        gui_text_element( &state->gui_state,
                "DIRECTIONS ARE CLOCKWISE WITH ZERO AS RIGHT!",
                gui_layout_next_x( &state->gui_state ),
                gui_layout_next_y( &state->gui_state ),
                1.0, 0.0, 0.0, 1.0);

        for( unsigned direction = 0;
                direction < ANIMATION_SET_DIRECTION_COUNT;
                ++direction ){
            if( gui_do_button( &state->gui_state,
                        actor_model_animation_set_direction_strings[direction],
                        gui_layout_next_x( &state->gui_state ), 
                        gui_layout_next_y( &state->gui_state )) ){
                animation_set->direction = direction;
            }
        }
        gui_layout_vertical_padding( &state->gui_state, 12 );
        gui_text_element( &state->gui_state,
                "CURRENTLY SET DIRECTION FLAG!\n",
                gui_layout_next_x( &state->gui_state ),
                gui_layout_next_y( &state->gui_state ),
                1.0, 0.0, 0.0, 1.0 );
        gui_text_element( &state->gui_state,
                actor_model_animation_set_direction_strings[animation_set->direction],
                gui_layout_next_x( &state->gui_state ),
                gui_layout_next_y( &state->gui_state ),
                1.0, 1.0, 1.0, 1.0 );

        gui_end_layout( &state->gui_state );
    }

    if( gui_do_button( &state->gui_state,
        "BACK",
        100, 
        renderer->screen_height - 50) ){
        editor_state->flag_selection_screen_open = false;
    }
}

/*TODO(jerry): 
 * Not sure how relevant set flags will be... Not implementing until I provably need them
 * */
static void update_render_model_editor_flag_selection_screen_animation_set_flags( 
        game_state* state,
        renderer* renderer,
        game_input* input,
        actor_model* model,
        actor_model_animation_group* animation_group,
        actor_model_animation_set* animation_set,
        actor_model_animation_frame* animation_frame,
        f32 delta_time ){
    game_model_editor_state* editor_state = &state->model_editor;

    gui_begin_vertical_layout( &state->gui_state, 100, 20, 5, 5 );

    gui_text_element( &state->gui_state,
            "THESE MODEL SCALES ARE FOR THE LOGIC / COLLISION!",
            gui_layout_next_x( &state->gui_state ),
            gui_layout_next_y( &state->gui_state ),
            1.0, 0.0, 0.0, 1.0);
    gui_text_element( &state->gui_state,
            "CHANGE DRAW_SCALE FOR VISUALS!",
            gui_layout_next_x( &state->gui_state ),
            gui_layout_next_y( &state->gui_state ),
            1.0, 0.0, 0.0, 1.0);

    for( unsigned size = 0;
            size < ACTOR_MODEL_SCALE_COUNT;
            ++size ){
        if( gui_do_button( &state->gui_state,
                    actor_model_scale_strings[size],
                    gui_layout_next_x( &state->gui_state ), 
                    gui_layout_next_y( &state->gui_state )) ){
            model->model_scale = size;
        }
    }
    gui_layout_vertical_padding( &state->gui_state, 12 );
    gui_text_element( &state->gui_state,
            "CURRENTLY SET MODEL_SCALE FLAG!\n",
            gui_layout_next_x( &state->gui_state ),
            gui_layout_next_y( &state->gui_state ),
            1.0, 0.0, 0.0, 1.0 );
    gui_text_element( &state->gui_state,
            actor_model_scale_strings[model->model_scale],
            gui_layout_next_x( &state->gui_state ),
            gui_layout_next_y( &state->gui_state ),
            1.0, 1.0, 1.0, 1.0 );

    gui_end_layout( &state->gui_state );

    if( gui_do_button( &state->gui_state,
        "BACK",
        100, 
        renderer->screen_height - 50) ){
        editor_state->flag_selection_screen_open = false;
    }
}

static void update_render_model_editor( game_state* state,
                                        renderer* renderer,
                                        game_input* input,
                                        f32 delta_time ){
    render_layer_clear_color_buffer( &state->game_layer, 0.0, 0.0, 0.0, 1.0 );
    renderer_set_screen_dimensions( renderer, state->screen_width, state->screen_height );

    render_layer_camera_set_scale( &state->game_layer, state->camera.scale );
    render_layer_camera_set_scale( &state->ui_layer, 1 );
    game_model_editor_state* editor_state = &state->model_editor;

    state->strings_scratch = (memory_pool){};
    memory_sub_pool_init( &state->strings_scratch, state->scratch_memory, MB(16) );

    actor_model* model = &editor_state->current_actor_model;

    actor_model_animation_group* animation_group = &model->animation_groups[ editor_state->current_animation_group ];
    actor_model_animation_set* animation_set = 0;
    actor_model_animation_frame* animation_frame = 0;

    editor_state->current_animation_group =
        u32_clamp( editor_state->current_animation_group, 
                0, ACTOR_ANIMATION_GROUP_COUNT-1 );
    if( animation_group->animation_set_count ){
        animation_set = &animation_group->animation_sets[ editor_state->current_animation_set ];
        editor_state->current_animation_set =
            u32_clamp( editor_state->current_animation_set, 
                    0, animation_group->animation_set_count-1 );
        if( animation_set->animation_frame_count ){
            animation_frame = &animation_set->frames[ editor_state->current_frame ];
            editor_state->current_frame =
                u32_clamp( editor_state->current_frame, 
                        0, animation_set->animation_frame_count-1 );
        }
    }

    switch( editor_state->model_scene_type ){
        case GAME_MODEL_EDITOR_TEST_SCENE_ISOLATED:
        {
            model_editor_draw_isolated_scene( state, renderer, input, delta_time );
        }
        break;
        case GAME_MODEL_EDITOR_TEST_SCENE_WORLD:
        {
            model_editor_draw_test_scene( state, renderer, input, delta_time );
        }
        break;
        default:
        {
        }
        break;
    }

    if( input_is_key_pressed( input, INPUT_KEY_ESCAPE ) ){
        state->mode = GAME_STATE_MAIN_MENU;
    }

    gui_begin_frame( &state->gui_state, renderer, &state->ui_layer, input );
    gui_set_font( &state->gui_state, state->font_texture );

    if( !editor_state->file_browser_open ){
        if( !editor_state->flag_selection_screen_open ){
            update_render_model_editor_main_screen( state,
                    renderer,
                    input,
                    model,
                    animation_group,
                    animation_set,
                    animation_frame,
                    delta_time );
        }else{
            switch( editor_state->flag_selection_mode ){
                case EDITOR_STATE_FLAG_SELECTION_SCREEN_ANIMATION_SET_DIRECTION_FLAGS:
                {
                    // Yeah I'm not sure if I should consider making these names less
                    // verbose. I've just gotten used to this...
                    update_render_model_editor_flag_selection_screen_animation_set_direction_flags( state,
                            renderer,
                            input,
                            model,
                            animation_group,
                            animation_set,
                            animation_frame,
                            delta_time );
                }
                break;
                case EDITOR_STATE_FLAG_SELECTION_SCREEN_ANIMATION_SET_FLAGS:
                {
                    update_render_model_editor_flag_selection_screen_animation_set_flags( state,
                            renderer,
                            input,
                            model,
                            animation_group,
                            animation_set,
                            animation_frame,
                            delta_time );
                }
                break;
            }
        }
    }else{
        model_editor_update_render_file_selection_gui( state,
                                                       renderer,
                                                       input,
                                                       model,
                                                       animation_group,
                                                       animation_set,
                                                       animation_frame,
                                                       delta_time );
    }

    gui_end_frame( &state->gui_state );

    renderer_draw_render_layer( renderer, &state->game_layer );
    renderer_draw_render_layer( renderer, &state->ui_layer );
}

static void update_render_dialogue_editor( game_state* state,
        renderer* renderer,
        game_input* input,
        f32 delta_time ){
    render_layer_clear_color_buffer( &state->background, 0.0, 0.0, 0.0, 1.0 );
}
