// removes noise from wanderer.c
static void actor_model_set_name( actor_model* model, char* name ){
    strncpy(model->name, name, NEW_ACTOR_MODEL_NAME_SIZE);
}

static void actor_model_animation_group_remove_all_animation_sets( actor_model_animation_group* animation_group ){
    for( unsigned animation_set_index = 0;
         animation_set_index < animation_group->animation_set_count;
         ++animation_set_index ){
        actor_model_animation_set* current_set = &animation_group->animation_sets[animation_set_index];
        memory_deallocate( current_set->frames );
    }
    memory_deallocate( animation_group->animation_sets );
    animation_group->animation_set_count = 0;
    animation_group->animation_sets = NULL;
}

// this is for the dynamically allocated data.
// this will never be called during game time for
// obvious reasons.
static void actor_model_clear_all_data( actor_model* model ) {
    for( unsigned animation_group_index = 0;
         animation_group_index < ACTOR_ANIMATION_GROUP_COUNT;
         ++animation_group_index ){
        actor_model_animation_group_remove_all_animation_sets( &model->animation_groups[animation_group_index] );
    }
}

// These push and remove functions are for the editor
// NOTE(jerry): Memory leak. The frames are never freed.
// TODO(jerry): Convert to linked list.
static void actor_model_animation_group_remove_animation_set( actor_model_animation_group* animation_group, u32 index ){
    animation_group->animation_set_count--;
    actor_model_animation_set* new_animation_sets_array =
        memory_allocate( sizeof(actor_model_animation_set) * animation_group->animation_set_count );
    unsigned new_index = 0;
    for( unsigned old_index = 0;
         old_index < animation_group->animation_set_count+1;
         ++old_index ){
        if( old_index == index ){
            continue;
        }else{
            new_animation_sets_array[new_index] = animation_group->animation_sets[old_index];
            new_index++;
        }
    }
    actor_model_animation_set* old_array = animation_group->animation_sets;
    memory_deallocate( old_array );

    animation_group->animation_sets = new_animation_sets_array;
}

static actor_model_animation_set* actor_model_animation_group_push_blank_animation_set( 
        actor_model_animation_group* animation_group 
        ){
    animation_group->animation_set_count++;
    actor_model_animation_set* result;

    if( !animation_group->animation_sets ){
        animation_group->animation_sets =
            memory_allocate( sizeof(actor_model_animation_set) );
        result = animation_group->animation_sets;
    }else{
        actor_model_animation_group* 
            new_group_array = memory_allocate( 
                    sizeof(actor_model_animation_set) *
                    animation_group->animation_set_count );
        memcpy(new_group_array, animation_group->animation_sets,
                sizeof(actor_model_animation_set) *
               (animation_group->animation_set_count - 1));
        memory_deallocate(animation_group->animation_sets);
        animation_group->animation_sets = new_group_array;

        result = &animation_group->animation_sets[animation_group->animation_set_count-1];
    }
    result->animation_frame_count = 0;
    result->flags = 0;
    result->direction = 0;
    result->spritesheet_id = game_asset_null();
    memset( result->name, 0, sizeof(result->name) );
    result->frames = NULL;

    return result;
}

// stupid dynamically growing array. I don't need much since
// it's really all done at load time.
static actor_model_animation_set* actor_model_animation_group_push_animation_set( 
        actor_model_animation_group* animation_group,
        char* name,
        game_asset_handle spritesheet_id,
        u8 flags,
        u8 direction ){
    actor_model_animation_set* result =
        actor_model_animation_group_push_blank_animation_set( animation_group );

    strncpy(result->name, name, NEW_ACTOR_MODEL_NAME_SIZE);
    result->flags = flags;
    result->direction = direction;
    result->animation_frame_count = 0;
    result->frames = NULL;
    result->spritesheet_id = spritesheet_id;

    return result;
}

static void actor_model_animation_set_remove_animation_frame( actor_model_animation_set* animation_set, u32 index ){
    animation_set->animation_frame_count--;
    actor_model_animation_frame* new_animation_frames_array =
        memory_allocate( sizeof(actor_model_animation_frame) * animation_set->animation_frame_count );
    unsigned new_index = 0;
    for( unsigned old_index = 0;
         old_index < animation_set->animation_frame_count+1;
         ++old_index ){
        if( old_index == index ){
            continue;
        }else{
            new_animation_frames_array[new_index] = animation_set->frames[old_index];
            new_index++;
        }
    }
    actor_model_animation_frame* old_array = animation_set->frames;
    memory_deallocate( old_array );

    animation_set->frames = new_animation_frames_array;
}

static actor_model_animation_frame* actor_model_animation_set_push_blank_animation_frame(
        actor_model_animation_set* animation_set
        ){
    animation_set->animation_frame_count++;
    actor_model_animation_frame* result;

    if( !animation_set->frames ){
        animation_set->frames = 
            memory_allocate( sizeof(actor_model_animation_frame) );
        result = animation_set->frames;
    }else{
        actor_model_animation_frame* 
            new_group_array = memory_allocate( 
                    sizeof(actor_model_animation_frame) *
                    animation_set->animation_frame_count );
        memcpy(new_group_array, animation_set->frames,
                sizeof(actor_model_animation_frame) *
               (animation_set->animation_frame_count - 1));
        memory_deallocate(animation_set->frames);
        animation_set->frames = new_group_array;

        result = &animation_set->frames[animation_set->animation_frame_count-1];
    }

    result->width = 0;
    result->height = 0;
    result->texture_id = (game_asset_handle){};
    result->origin_x = 0.0f;
    result->origin_y = 0.0f;
    result->time_to_next_frame = 0.0f;
    memset( result->name, 0, sizeof(result->name) );

    return result;
}

static actor_model_animation_frame* actor_model_animation_set_push_animation_frame(
        actor_model_animation_set* animation_set,
        game_assets* assets,
        char* path_for_texture_resource,
        f32 pivot_x,
        f32 pivot_y,
        f32 time_to_next_frame
        ){
    actor_model_animation_frame* result =
        actor_model_animation_set_push_blank_animation_frame( animation_set );

    strncpy(result->name, path_for_texture_resource, PLATFORM_FILE_NAME_MAX);
    result->texture_id = game_asset_load_bitmap( assets, path_for_texture_resource );
    {
        game_asset* texture_asset = game_asset_get_from_handle( assets, result->texture_id );
        result->width = texture_asset->bitmap.width;
        result->height = texture_asset->bitmap.height;
    }
    result->origin_x = pivot_x;
    result->origin_y = pivot_y;
    result->time_to_next_frame = time_to_next_frame;

    return result;
}

static actor_model_animation_frame* actor_model_animation_set_push_animation_frame_spritesheet(
        actor_model_animation_set* animation_set,
        game_assets* assets,
        u32 spritesheet_index,
        f32 pivot_x,
        f32 pivot_y,
        f32 time_to_next_frame
        ){
    actor_model_animation_frame* result =
        actor_model_animation_set_push_blank_animation_frame( animation_set );

    /* strncpy(result->name, "asdfghjkl", PLATFORM_FILE_NAME_MAX); */
    result->spritesheet_index = spritesheet_index;
    {
        game_asset* spritesheet_asset = 
            game_asset_get_from_handle( assets, animation_set->spritesheet_id );
        game_asset_spritesheet_rect sprite_rect = 
            spritesheet_asset->spritesheet.sprite_rects[spritesheet_index];

        result->width = sprite_rect.w;
        result->height = sprite_rect.h;
    }
    result->origin_x = pivot_x;
    result->origin_y = pivot_y;
    result->time_to_next_frame = time_to_next_frame;

    return result;
}
