#include "wanderer_assets.h"
#include "renderer.h"

#include "config.h"

u32 actor_model_animation_group_find_animation_set_for_direction( actor_model_animation_group* group,
                                                                  vec2 direction ){
    // This is f**king stupid.
    vec2 avaliable_direction_selections[ANIMATION_SET_DIRECTION_COUNT - 1] =
        {
            (vec2){  cosf(degrees_to_radians(0.0f)),  sinf(degrees_to_radians(0.0f)) },
            (vec2){  cosf(degrees_to_radians(45.0f)), sinf(degrees_to_radians(45.0f)) },
            (vec2){  cosf(degrees_to_radians(90.0f)), sinf(degrees_to_radians(90.0f)) },
                                  
            (vec2){ cosf(degrees_to_radians(135.0f)), sinf(degrees_to_radians(135.0f)) },
            (vec2){ cosf(degrees_to_radians(180.0f)), sinf(degrees_to_radians(180.0f)) },
            (vec2){ cosf(degrees_to_radians(225.0f)), sinf(degrees_to_radians(225.0f)) },
                                  
            (vec2){ cosf(degrees_to_radians(270.0f)), sinf(degrees_to_radians(270.0f)) },
            (vec2){ cosf(degrees_to_radians(315.0f)), sinf(degrees_to_radians(315.0f)) },
            (vec2){ cosf(degrees_to_radians(360.0f)), sinf(degrees_to_radians(360.0f)) },
        };
    // prefer a directional sprite over override sprite which will be used
    // as a fallback.
    f32 current_best_match = INFINITY;
    u32 best_animation_set_index = 0;

    actor_model_animation_group* animation_group = group;

    for( unsigned animation_set_index = 0;
         animation_set_index < animation_group->animation_set_count;
         ++animation_set_index ){
        actor_model_animation_set* current_animation_set = 
            &animation_group->animation_sets[animation_set_index];
        if( current_animation_set->direction != ANIMATION_SET_DIRECTION_OVERRIDE ){
            vec2 selected_direction = avaliable_direction_selections[current_animation_set->direction];
            vec2 lookat_direction = direction;

            f32 direction_distance_x = ( lookat_direction.x - selected_direction.x );
            f32 direction_distance_y = ( lookat_direction.y - selected_direction.y );
            f32 direction_distance_sq = ( (direction_distance_x) * (direction_distance_x) ) +
                ( (direction_distance_y) * (direction_distance_y) );

            if( direction_distance_sq < current_best_match ){
                current_best_match = direction_distance_sq;
                best_animation_set_index = animation_set_index;
            }
        }else{
            if( current_best_match == INFINITY ){
                best_animation_set_index = animation_set_index;
            }
        }
    }
    
    return best_animation_set_index;
}

#if 0
//TODO(jerry): do when needed.
void actor_model_animation_group_find_animation_sets_for_direction( actor_model_animation_group* group,
                                                                    vec2 direction,
                                                                    size_t list_size,
                                                                    u32* list ){
}
#endif

game_asset_handle game_asset_null( void ){
    return (game_asset_handle){ .id = 0 };
}

// There will be collisions if I try this
// with fonts simply because fonts don't hash the same way.
//
// Technically I'm never going to be mapping for a font anyways
// so I don't think it's a real problem. The only thing I'd potential
// hash for is a texture or sound which has one invariant so I'm safe.

static u64 fnv_1a_hash_string( const char* string, size_t string_length ){
    u64 prime = 0x01000193;
    u64 seed  = 0x811C9DC5;
    u64 hash_value = seed;

    // maybe unroll?
    for( unsigned index = 0; index < string_length; ++index ){
       hash_value = (string[index] ^ hash_value) * prime;
    }

    return hash_value;
}

void game_assets_init( game_assets* assets, renderer* renderer ){
    assets->renderer_context = renderer;
    renderer->assets = assets;
    memset(assets->assets, 0, sizeof(game_asset) * MAX_GAME_ASSETS_IN_GAME);

    memory_pool_init( &assets->asset_memory_pool, MB(8) );
}

void game_assets_finish( game_assets* assets ){
    memory_pool_finish( &assets->asset_memory_pool );
}

game_asset_handle game_asset_load_font( game_assets* assets, const char* font_path, f32 size ){
    u64 string_hash = fnv_1a_hash_string( font_path, strlen(font_path) );
    string_hash %= MAX_GAME_ASSETS_IN_GAME;
    game_asset* target_slot = &assets->assets[ string_hash ];
    
    if( target_slot->type == GAME_ASSET_FONT ){
        fprintf(stderr, "font asset already exists, load font will return existing\n");
        return (game_asset_handle){ .id = string_hash };
    }

    game_asset new_asset = {
        .type = GAME_ASSET_FONT,
        .font = 
        {
            .renderer_id = 0,
            .size = size
        }
    };

    fprintf(stderr, "loading %s as hash %u\n", font_path, string_hash);
    strncpy( new_asset.name, font_path, 255 );

    u8* font_buffer = load_file_into_buffer( font_path );
    u64 file_size = get_file_size( font_path );
    assert(target_slot->type == GAME_ASSET_NONE);
    *target_slot = new_asset;
    target_slot->font.renderer_id =
        renderer_load_font(assets->renderer_context,
                font_buffer,
                file_size,
                size);

    memory_deallocate( font_buffer );

    return (game_asset_handle){ .id = string_hash };
}

game_asset_handle game_asset_load_actor_model( game_assets* assets, const char* model_path ){
    u64 string_hash = fnv_1a_hash_string( model_path, strlen(model_path) );
    string_hash %= MAX_GAME_ASSETS_IN_GAME;
    game_asset* target_slot = &assets->assets[string_hash];
    
    if( target_slot->type == GAME_ASSET_ACTOR_MODEL ){
        fprintf(stderr, "actor model asset already exists, load font will return existing\n");
        return (game_asset_handle){ .id = string_hash };
    }

    game_asset* new_asset = target_slot;
    new_asset->type = GAME_ASSET_ACTOR_MODEL;
    actor_model* model = &new_asset->actor_model;

    config_info actor_model_info = config_parse_from_file( model_path );
#if 0
    config_info_dump_as_text_to_stderr( &actor_model_info );
#endif

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

        if( draw_scale->value.type == CONFIG_VARIABLE_NUMBER ){
            model->draw_scale = config_variable_try_float( draw_scale );
        }else{
            try_parse = false;
        }
    }

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

                    unsigned animation_set_blocks_count = config_block_count_variables_of_type( block_content, CONFIG_VARIABLE_BLOCK );

                    selected_animation_group->animation_set_count = animation_set_blocks_count;
                    selected_animation_group->animation_sets =
                        memory_pool_allocate( &assets->asset_memory_pool,
                                              sizeof(actor_model_animation_set) *
                                              selected_animation_group->animation_set_count );

                    actor_model_animation_set* current_animation_set = selected_animation_group->animation_sets;
                    for( unsigned animation_set_block_index = 0;
                         animation_set_block_index < block_content->count;
                         ++animation_set_block_index ){
                        actor_model_animation_set* new_animation_set = current_animation_set++;

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

                        new_animation_set->direction =
                            get_actor_model_animation_set_direction_from_string( config_variable_try_string(animation_set_direction) );

                        new_animation_set->spritesheet_id =
                            game_asset_load_spritesheet( assets, config_variable_try_string(animation_set_spritesheet_file) );
                    }

                    current_animation_set = selected_animation_group->animation_sets;
                    for( unsigned animation_set_block_index = 0;
                         animation_set_block_index < block_content->count;
                         ++animation_set_block_index ){
                        config_variable* animation_set_block = &block_content->items[animation_set_block_index];
                        config_variable_block* animation_set_block_content = animation_set_block->value.as_block;

                        actor_model_animation_set* animation_set = current_animation_set++;
                        unsigned animation_frame_blocks_in_set_block = config_block_count_variables_of_type( animation_set_block_content, CONFIG_VARIABLE_BLOCK );

                        animation_set->animation_frame_count = animation_frame_blocks_in_set_block;
                        animation_set->frames = 
                            memory_pool_allocate( &assets->asset_memory_pool,
                                                  sizeof(actor_model_animation_frame) *
                                                  animation_set->animation_frame_count );

                        fprintf(stderr, "allocated %d frames for animation_set...\n", animation_set->animation_frame_count);

                        actor_model_animation_frame* current_animation_frame = animation_set->frames;
                        for( unsigned animation_set_block_item_index = 0;
                             animation_set_block_item_index < animation_set_block_content->count;
                             ++animation_set_block_item_index ){
                            config_variable* current_animation_set_block_item =
                                &animation_set_block_content->items[animation_set_block_item_index];

                            if( current_animation_set_block_item->value.type != CONFIG_VARIABLE_BLOCK ){
                                continue;
                            }else{
                                if( current_animation_set_block_item->value.type == CONFIG_VARIABLE_BLOCK ){
                                    actor_model_animation_frame* new_frame = current_animation_frame++;
                                    strncpy( new_frame->name, current_animation_set_block_item->name, NEW_ACTOR_MODEL_NAME_SIZE );

                                    config_variable_block* actor_animation_frame_block_content =
                                        current_animation_set_block_item->value.as_block;

                                    for( unsigned animation_frame_block_item_index = 0;
                                         animation_frame_block_item_index < actor_animation_frame_block_content->count;
                                         ++animation_frame_block_item_index ){
                                        config_variable* animation_frame_origin_x = config_block_find_first_variable( actor_animation_frame_block_content, "origin_x" );
                                        config_variable* animation_frame_origin_y = config_block_find_first_variable( actor_animation_frame_block_content, "origin_y" );
                                        config_variable* animation_frame_time_to_next_frame = config_block_find_first_variable( actor_animation_frame_block_content, "time_to_next_frame ");

                                        new_frame->origin_x = config_variable_try_float( animation_frame_origin_x );
                                        new_frame->origin_y = config_variable_try_float( animation_frame_origin_y );
                                        new_frame->time_to_next_frame = config_variable_try_float( animation_frame_time_to_next_frame );

                                        config_variable* animation_frame_texture_resource =
                                            config_block_find_first_variable( actor_animation_frame_block_content, "spritesheet_index" );

                                        if( animation_frame_texture_resource ){
                                            new_frame->spritesheet_index = config_variable_try_integer( animation_frame_texture_resource );
                                        }else{
                                            // texture_file
                                            animation_frame_texture_resource =
                                                config_block_find_first_variable( actor_animation_frame_block_content, "texture_file" );
                                            new_frame->texture_id =
                                                game_asset_load_bitmap( assets, config_variable_try_string(animation_frame_texture_resource) );
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
    return (game_asset_handle){ .id = string_hash };
}

static void game_asset_internal_build_tiled_spritesheet( game_asset* spritesheet_asset, 
                                                         u32 rows,
                                                         u32 columns,
                                                         u32 tile_width,
                                                         u32 tile_height ){
    game_asset_spritesheet_rect* rect_cursor;
    for( u64 current_row = 0; current_row < rows; ++current_row ){
        for( u64 current_col = 0; current_col < columns; ++current_col ){
            rect_cursor = &spritesheet_asset->spritesheet.sprite_rects[spritesheet_asset->spritesheet.sprite_rect_count++];
            // TODO(jerry): does not account for sprite padding
            game_asset_spritesheet_rect sprite_rect = {
                .x = current_col * tile_width,
                .y = current_row * tile_height,
                .w = tile_width,
                .h = tile_height
            };

#if 0
            fprintf(stderr, "new spritesheet rect (%d, %d, %d, %d)\n",
                    sprite_rect.x,
                    sprite_rect.y,
                    sprite_rect.w,
                    sprite_rect.h);
#endif
            *rect_cursor = sprite_rect;
        }
    }
}

game_asset_handle game_asset_load_spritesheet( game_assets* assets,
                                               const char* spritesheet_info_path ){
    char hashed_string_id[1000] = {};
    strcpy(hashed_string_id, spritesheet_info_path);
    // strcat(hashed_string_id, ":SPRITESHEET");
    u64 string_hash = fnv_1a_hash_string( hashed_string_id, strlen(hashed_string_id) );
    string_hash %= MAX_GAME_ASSETS_IN_GAME;
    game_asset* target_slot = &assets->assets[ string_hash ];

    if( target_slot->type == GAME_ASSET_SPRITESHEET ){
        return (game_asset_handle){ .id = string_hash };
    }else if( target_slot->type != GAME_ASSET_NONE ){
        return (game_asset_handle){ .id = 0 };
    }else if( target_slot->type == GAME_ASSET_NONE ){
        // .
    }

    game_asset new_asset = { .type = GAME_ASSET_SPRITESHEET };
    strncpy( new_asset.name, spritesheet_info_path, 128 );

    typedef enum spritesheet_parse_mode{
        GAME_ASSET_LOAD_SPRITESHEET_PARSE_MODE_NONE,
        GAME_ASSET_LOAD_SPRITESHEET_PARSE_MODE_TILED_ATLAS,
        GAME_ASSET_LOAD_SPRITESHEET_PARSE_MODE_ATLAS,
        GAME_ASSET_LOAD_SPRITESHEET_PARSE_MODE_TYPE
    }spritesheet_parse_mode;

    // the only time I actually use the enum type.
    spritesheet_parse_mode parse_mode = GAME_ASSET_LOAD_SPRITESHEET_PARSE_MODE_NONE;

    config_info spritesheet_config = config_parse_from_file( spritesheet_info_path );

    assert( config_info_find_first_variable( &spritesheet_config, "atlas_info" ) == &spritesheet_config.variables[0] );
    assert( config_info_find_first_variable( &spritesheet_config, "image_file" ) == &spritesheet_config.variables[1] );

    {
        config_variable* atlas_info_variable = &spritesheet_config.variables[0];
        config_variable_value atlas_info = atlas_info_variable->value;
        if( atlas_info.type == CONFIG_VARIABLE_SYMBOL ){
            char* symbol_value = atlas_info.value;

            if( strcmp(symbol_value, "TILED_ATLAS") == 0 ){
                parse_mode = GAME_ASSET_LOAD_SPRITESHEET_PARSE_MODE_TILED_ATLAS;
            }else if( strcmp(symbol_value, "ATLAS") == 0 ){
                parse_mode = GAME_ASSET_LOAD_SPRITESHEET_PARSE_MODE_ATLAS;
            }
        }
    }

    {
        config_variable* image_file_variable = &spritesheet_config.variables[1];
        config_variable_value bitmap_path = image_file_variable->value;
        if( bitmap_path.type == CONFIG_VARIABLE_STRING ){
            new_asset.spritesheet.matching_bitmap = 
                game_asset_load_bitmap( assets, bitmap_path.value );
        }
    }

    switch( parse_mode ){
        case GAME_ASSET_LOAD_SPRITESHEET_PARSE_MODE_TILED_ATLAS:
        {
            u32 tile_rows = 0;
            u32 tile_columns = 0;
            u32 tile_width = 0;
            u32 tile_height = 0;
            // TODO(jerry) : UNUSED?
            i16 tile_padding_x = 0;
            i16 tile_padding_y = 0;

            tile_rows = config_variable_try_integer(config_info_find_first_variable( &spritesheet_config, "rows" ));
            tile_columns = config_variable_try_integer(config_info_find_first_variable( &spritesheet_config, "columns" ));
            tile_width = config_variable_try_integer(config_info_find_first_variable( &spritesheet_config, "tile_width" ));
            tile_height = config_variable_try_integer(config_info_find_first_variable( &spritesheet_config, "tile_height" ));

            new_asset.spritesheet.sprite_rects =
                memory_pool_allocate( &assets->asset_memory_pool,
                                      (tile_columns *
                                       tile_rows) *
                                      sizeof(game_asset_spritesheet_rect) );

            game_asset_internal_build_tiled_spritesheet( &new_asset,
                                                         tile_rows,
                                                         tile_columns,
                                                         tile_width,
                                                         tile_height );
        }
        break;
        case GAME_ASSET_LOAD_SPRITESHEET_PARSE_MODE_ATLAS:
        {
            config_variable* atlas_rects_list = config_info_find_first_variable( &spritesheet_config, "atlas_rects" );
            if( atlas_rects_list->value.type == CONFIG_VARIABLE_LIST ){
                config_variable_list* list_of_rects = atlas_rects_list->value.as_list;

                new_asset.spritesheet.sprite_rects =
                    memory_pool_allocate( &assets->asset_memory_pool,
                                          (list_of_rects->count) *
                                          sizeof(game_asset_spritesheet_rect) );

                for( u64 list_index = 0; list_index < list_of_rects->count; ++list_index ){
                    config_variable_value* current_item = &list_of_rects->items[list_index];

                    if( current_item->type == CONFIG_VARIABLE_LIST ){
                        config_variable_list* rectangle = current_item->as_list;
                        if( rectangle->count == 4 ){
                            f32 rect_x = config_variable_value_try_float(&rectangle->items[0]);
                            f32 rect_y = config_variable_value_try_float(&rectangle->items[1]);
                            f32 rect_w = config_variable_value_try_float(&rectangle->items[2]);
                            f32 rect_h = config_variable_value_try_float(&rectangle->items[3]);

                            game_asset_spritesheet_rect sprite_rect = {
                                .x = rect_x,
                                .y = rect_y,
                                .w = rect_w,
                                .h = rect_h
                            };

#if 0
                            fprintf(stderr, "new spritesheet rect (%d, %d, %d, %d)\n",
                                    sprite_rect.x,
                                    sprite_rect.y,
                                    sprite_rect.w,
                                    sprite_rect.h);
#endif
                            new_asset.spritesheet.sprite_rects[new_asset.spritesheet.sprite_rect_count++] = sprite_rect;
                        }else{
                            fprintf(stderr, "incorrect list item count. Skipping rect\n");
                        }
                    }
                }
            }
        }
        break;
    }

    config_info_free_all_fields( &spritesheet_config );
    
    *target_slot = new_asset;
    return (game_asset_handle){ .id = string_hash };
}

game_asset_handle game_asset_make_spritesheet( game_assets* assets,
                                               const char* spritesheet_bitmap_path,
                                               game_asset_load_spritesheet_settings load_settings ){
    game_asset_handle bitmap_asset_handle = game_asset_load_bitmap( assets, spritesheet_bitmap_path );
    char hashed_string_id[1000] = {};
    strcpy(hashed_string_id, spritesheet_bitmap_path);
    strcat(hashed_string_id, ":SPRITESHEET");
    u64 string_hash = fnv_1a_hash_string( hashed_string_id, strlen(hashed_string_id) );
    string_hash %= MAX_GAME_ASSETS_IN_GAME;
    game_asset* target_slot = &assets->assets[ string_hash ];

    if( target_slot->type == GAME_ASSET_SPRITESHEET ){
        return (game_asset_handle){ .id = string_hash };
    }else if( target_slot->type != GAME_ASSET_NONE ){
        return (game_asset_handle){ .id = 0 };
    }else if( target_slot->type == GAME_ASSET_NONE ){
        // .
    }

    game_asset new_asset = {
        .type = GAME_ASSET_SPRITESHEET,
        .spritesheet = {
            .matching_bitmap = bitmap_asset_handle
        }
    };

    switch( load_settings.for_spritesheet_type ){
        case GAME_ASSET_SPRITESHEET_TYPE_TILED:
        {
            // until I change the memory scheme for this subsystem.
            game_asset_internal_build_tiled_spritesheet( &new_asset,
                                                         load_settings.tiled.rows,
                                                         load_settings.tiled.columns,
                                                         load_settings.tiled.tile_width,
                                                         load_settings.tiled.tile_height );
        }
        break;
        case GAME_ASSET_SPRITESHEET_TYPE_ATLAS:
        {
            stub_important_code("GAME_ASSET_SPRITESHEET_TYPE_ATLAS not done");
        }
        break;
    }

    *target_slot = new_asset;
    return (game_asset_handle){ .id = string_hash };
}

game_asset_handle game_asset_load_bitmap( game_assets* assets, const char* bitmap_path ){
    u64 string_hash = fnv_1a_hash_string( bitmap_path, strlen(bitmap_path) );
    string_hash %= MAX_GAME_ASSETS_IN_GAME;
    game_asset* target_slot = &assets->assets[ string_hash ];

    fprintf(stderr, "type present(%s (%d x %d)) : %s at index %u\n",
            target_slot->name,
            target_slot->bitmap.width,
            target_slot->bitmap.height,
            game_asset_type_strings[target_slot->type],
            string_hash);

    if( target_slot->type == GAME_ASSET_BITMAP ){
        fprintf(stderr, "bitmap asset already exists, load bitmap will return existing\n");
        return (game_asset_handle){ .id = string_hash };
    }else if( target_slot->type != GAME_ASSET_NONE ){
        fprintf(stderr, "Conflict found... ");
        fprintf(stderr, "type present : %s at index %u\n",
                game_asset_type_strings[target_slot->type], string_hash);
        return (game_asset_handle){ .id = 0 };
    }else if( target_slot->type == GAME_ASSET_NONE ){
        fprintf(stderr, "loading new bitmap.\n");
    }

    fprintf(stderr, "loading %s as hash %u\n", bitmap_path, string_hash);
    bitmap_image* loaded_bitmap = load_image( bitmap_path );
    game_asset new_asset = {
        .type = GAME_ASSET_BITMAP,
        .bitmap = 
        {
            .renderer_id = renderer_create_texture_from_bitmap( assets->renderer_context, loaded_bitmap ),
            .width = loaded_bitmap->width,
            .height = loaded_bitmap->height
        }
    };

#ifdef DEBUG_BUILD
    strncpy( new_asset.name, bitmap_path, 255 );
#endif

    free_bitmap_image( loaded_bitmap );

    *target_slot = new_asset;

    return (game_asset_handle){ .id = string_hash };
}

void game_asset_unload_asset_with_key( game_assets* assets, const char* key ){
    u64 string_hash = fnv_1a_hash_string( key, strlen(key) );
    string_hash %= MAX_GAME_ASSETS_IN_GAME;
    game_asset_unload_asset_with_handle( assets, 
            (game_asset_handle){ .id = string_hash } );
}

// is this even necessary? I think I just pre-emptively typed this.
game_asset* game_asset_get_from_key( game_assets* assets, const char* key ){
    u64 string_hash = fnv_1a_hash_string( key, strlen(key) );
    string_hash %= MAX_GAME_ASSETS_IN_GAME;
    return game_asset_get_from_handle( assets, (game_asset_handle){ .id = string_hash } );
}

static game_asset the_null_asset = {};
game_asset* game_asset_get_from_handle( game_assets* assets, game_asset_handle asset_handle ){
    game_asset* asset = &assets->assets[ asset_handle.id ];
#if 0
#ifdef DEBUG_BUILD
    fprintf(stderr, "type present : %s at index %u\n",
            game_asset_type_strings[asset->type],
            asset_handle.id);
#endif
#endif
#if 0
#ifdef DEBUG_BUILD
    fprintf(stderr, "asset path name : %s\n", asset->name);
#endif
#endif
    if( asset->type != GAME_ASSET_NONE ){
        return asset;
    }

    return &the_null_asset;
}

// we don't properly evict resources sadly.
// at the moment I'm not actually sure if I ever will?
// If I ever do free stuff. Free list might come in handy.
void game_asset_unload_asset_with_handle( game_assets* assets, game_asset_handle asset_handle ){
    game_asset* asset = &assets->assets[ asset_handle.id ];

    stub_important_code("not implemented yet");
    // TODO(jerry)
    switch( asset->type ){
        case GAME_ASSET_FONT:
        {
        }
        break;
        case GAME_ASSET_BITMAP:
        {
        }
        break;
        case GAME_ASSET_SPRITESHEET:
        {
        }
        break;
        case GAME_ASSET_ACTOR_MODEL:
        {
        }
        break;
        default:
        {
        }
        break;
    }
}
