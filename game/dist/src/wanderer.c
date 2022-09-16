#include "wanderer.h"
#include "platform.h"

#include "input.h"
#include "bmp_loader.h"

#include "mat4x4.h"
#include "vec2.h"

#include "easing_functions.h"

static game_combat_round game_build_combat_round(game_state* state);
static void game_finish_combat_round(game_state* state, game_combat_round* round);
static b32 combat_round_advance_to_next_participant(game_combat_round* round);

// used for new isometric drawing calculations
// to see if I can simplify everything.
const f32 camera_scale = 2.45;

// this is based off my current tileset (dawnblocker)
static const f32 tile_width = 32;
static const f32 tile_height = 64;
static const f32 half_tile_width = (tile_width * 0.5);
static const f32 tile_offset_x = (32) * 0.5;
static const f32 tile_offset_y = (64) * 0.835;

// time is always in seconds. Unless commented otherwise.
static const f32 default_floating_message_lifetime = 1.85f;
static const f32 death_sequence_length = 4.0f;
static const f32 length_of_real_time_turn = 3.0f;

/*NOTE(jerry): wanderer.c*/
static game_moused_over_selection game_mouse_pick( vec2 mouse_coordinates, game_state* state );

/*Should probably get rid of these globals.*/
static cutscene_info global_test_cutscene = {};

#include "wanderer_actor_model_dynarray_backing.c"

static vec2 project_cartesian_to_isometric( vec2 cartesian ){
    vec2 isometric = {};

    isometric.x = (cartesian.x - cartesian.y);
    isometric.y = (cartesian.x + cartesian.y) / (2);

    return isometric;
}

static vec2 project_isometric_to_cartesian( vec2 isometric ){
    vec2 cartesian = {};

    cartesian.x = ( 2 * isometric.y + isometric.x ) / 2;
    cartesian.y = ( 2 * isometric.y - isometric.x ) / 2;

    return cartesian;
}

void game_render_list_push( game_render_list* render_list, game_render_object render_object ){
    game_render_object* current_object = 
        &render_list->objects[render_list->count++];

    *current_object = render_object;
}

// No I don't like this either.
static float game_render_list_get_y_key_value( game_render_list* render_list, 
                                               game_state* state,
                                               game_render_object* render_object ){
    float key_y = 0.0f;
    switch( render_object->type ){
        case GAME_RENDER_OBJECT_CONTAINER:
        {
            tilemap* current_map = render_list->current_scene_map;
            container* current_container = tilemap_get_container(current_map, render_object->index_to_object);
            key_y = (current_container->position.y / (float)2) + 
                (current_container->position.x / (float)2);
        }
        break;
        case GAME_RENDER_OBJECT_ITEM_DROP:
        {
            tilemap* current_map = render_list->current_scene_map;
            map_item_pickup* current_pickup = tilemap_get_pickup(current_map, render_object->index_to_object);
            key_y = (current_pickup->y / (float)2) + 
                (current_pickup->x / (float)2);
        }
        break;
        case GAME_RENDER_OBJECT_TILE:
        {
            tilemap* current_map = render_list->current_scene_map;
            map_tile* current_tile = &current_map->tiles[render_object->y_index][render_object->x_index];
            if( current_tile->solid ) {
                key_y = (render_object->y_index / (float)2) + 
                    (render_object->x_index / (float)2);
            }
        }
        break;
        case GAME_RENDER_OBJECT_ACTOR:
        {
            actor* target_actor = actors_list_get_actor( &state->actors, render_object->index_to_object );
            key_y = (target_actor->position.y / (float)2) +
                (target_actor->position.x / (float)2);
        }
        break;
        case GAME_RENDER_OBJECT_PROJECTILE:
        default:
        {
#if 0
            fprintf(stderr, 
                    "Render Object : %s : not given sort key value.\n",
                    game_render_object_type_strings[render_object->type]);
#endif
        }
        break;
    }

    return key_y;
}

void game_render_list_sort_render_objects( game_render_list* render_list, game_state* state ){
    // until sort speed becomes a problem... I'm just going to do
    // the easiest sort I know.
    // bubble sort to the rescue?
    for( unsigned i = 0; i < render_list->count; ++i ){
        for( unsigned j = i+1; j < render_list->count; ++j ){
            game_render_object* first = &render_list->objects[i];
            game_render_object* second = &render_list->objects[j];

            float first_y = game_render_list_get_y_key_value( render_list, state, first );
            float second_y = game_render_list_get_y_key_value( render_list, state, second );

            // might need to reverse.
            if( first_y > second_y ){
                game_render_object hold = *first;
                *first = *second;
                *second = hold;
            }
        }
    }
}

void game_render_list_submit_all_render_commands( game_render_list* render_list, render_layer* layer, game_state* state ){
    for( unsigned render_object_index = 0;
            render_object_index < render_list->count;
            ++render_object_index ) {
        game_render_object* object = &render_list->objects[render_object_index];
        switch( object->type ){
            // TODO(jerry): This is just getting it to show up. Not actually final
            case GAME_RENDER_OBJECT_CONTAINER:
            {
                tilemap* current_map = render_list->current_scene_map;
                container* current_container = tilemap_get_container(current_map, object->index_to_object);
                f32 draw_x;
                f32 draw_y;

                {
                    vec2 map_to_iso_units = project_cartesian_to_isometric( 
                        v2(current_container->position.x * half_tile_width,
                           current_container->position.y * half_tile_width));

                    vec2 pixels_as_isometric = map_to_iso_units;

                    draw_x = pixels_as_isometric.x;
                    draw_y = pixels_as_isometric.y;
                }

                render_command draw_command = 
                    render_command_textured_quad( 
                            draw_x, 
                            draw_y, 
                            tile_width,
                            tile_height,
                            1, 1, 1, 1, state->dawnblocker_block );
                render_layer_push_command( layer, draw_command ); 
            }
            break;
            case GAME_RENDER_OBJECT_ITEM_DROP:
            {
                tilemap* current_map = render_list->current_scene_map;
                map_item_pickup* current_pickup = tilemap_get_pickup(current_map, object->index_to_object);
                f32 draw_x;
                f32 draw_y;

                {
                    vec2 map_to_iso_units = project_cartesian_to_isometric( 
                        v2(current_pickup->x * half_tile_width,
                           current_pickup->y * half_tile_width));

                    vec2 pixels_as_isometric = map_to_iso_units;

                    draw_x = pixels_as_isometric.x;
                    draw_y = pixels_as_isometric.y;
                }

                render_command draw_command = 
                    render_command_textured_quad( 
                            draw_x, 
                            draw_y, 
                            tile_width,
                            tile_height,
                            0, 1, 1, 1, state->dawnblocker_block );
                render_layer_push_command( layer, draw_command ); 
            }
            break;
            case GAME_RENDER_OBJECT_TILE:
            {
                tilemap* current_map = render_list->current_scene_map;
                map_tile* current_tile = &current_map->tiles[object->y_index][object->x_index];
                game_asset_handle tile_texture = {};

                f32 draw_x;
                f32 draw_y;
                // hard coding this... Not happy about it but
                // what can I do?

                {
                    vec2 map_to_iso_units = project_cartesian_to_isometric( 
                        v2(object->x_index * half_tile_width,
                           object->y_index * half_tile_width));

                    vec2 pixels_as_isometric = map_to_iso_units;

                    draw_x = pixels_as_isometric.x;
                    draw_y = pixels_as_isometric.y;
                }

                if( current_tile->solid ){
                    tile_texture = state->dawnblocker_block;

                    render_command draw_command = 
                        render_command_textured_quad( 
                                draw_x, 
                                draw_y, 
                                tile_width,
                                tile_height,
                                1, 0, 0, 1, tile_texture );
                    render_layer_push_command( layer, draw_command ); 
                }else{
                    tile_texture = state->dawnblocker_floor;

                    render_command draw_command =
                        render_command_textured_quad( 
                                draw_x,
                                draw_y,
                                tile_width,
                                tile_height,
                                0, 0.2, 0.11, 1, tile_texture );
                    render_layer_push_command( layer, draw_command );
                }
            }
            break;
            // This is a tile just aligned to it's bottom.
            // until I draw a selection thing.
            case GAME_RENDER_OBJECT_SELECTED_TARGET_TILE:
            {
                game_asset_handle tile_texture = {};

                f32 draw_x;
                f32 draw_y;

                f32 draw_w = tile_width * 1.4;
                f32 draw_h = tile_height * 1.4;

                actor* player = game_get_player_actor(state);
                vec2 walk_position = player->current_command.movement.position;

                {
                    vec2 map_to_iso_units = project_cartesian_to_isometric( 
                        v2((walk_position.x) * half_tile_width,
                           (walk_position.y) * half_tile_width));

                    vec2 pixels_as_isometric = map_to_iso_units;

                    f32 pivot_x = (draw_w) * 0.5;
                    f32 pivot_y = (draw_h) * 0.835;

                    draw_x = pixels_as_isometric.x - (pivot_x) + tile_offset_x;
                    draw_y = pixels_as_isometric.y - (pivot_y) + tile_offset_y;
                }

                tile_texture = state->dawnblocker_floor;
                render_command draw_command =
                    render_command_textured_quad( 
                        draw_x,
                        draw_y,
                        draw_w,
                        draw_h,
                        0, 0, 1.0, 0.3,
                        tile_texture );
                render_layer_push_command( layer, draw_command );
            }
            break;
            case GAME_RENDER_OBJECT_ACTOR:
            {
                actor* actor = actors_list_get_actor( &state->actors, object->index_to_object );
                actor_visual_info* visual = &actor->visual_info;

                f32 x_pixels;
                f32 y_pixels;
                f32 w_pixels;
                f32 h_pixels;

                game_asset* model_asset = game_asset_get_from_handle( &state->assets, visual->model_id );
                actor_model* model = NULL;

                if( model_asset->type == GAME_ASSET_ACTOR_MODEL ){
                    model = &model_asset->actor_model;
                }else{
                    printf("no model?\n");
                    printf("handle: %d\n", visual->model_id.id);
                    printf("the type was:\n%s\n", game_asset_type_strings[model_asset->type]);
                    continue;
                }

                // clamp values
                // prevent crashing.
                {
                    visual->animation = i32_clamp( visual->animation, 0,
                            ACTOR_ANIMATION_GROUP_COUNT-1 );
                    if (model->animation_groups[visual->animation].animation_set_count) {
                        visual->selected_set = i32_clamp( visual->selected_set, 0,
                                model->animation_groups[visual->animation].animation_set_count-1 );
                        visual->current_frame = i32_clamp( visual->current_frame, 0,
                                model->animation_groups[visual->animation].animation_sets[visual->selected_set].animation_frame_count-1 );
                    } else {
                        continue;
                    }
                }
                // work on pattern matcher.
                actor_model_animation_group* animation_group = &model->animation_groups[ visual->animation ];
                actor_model_animation_set* animation_set = &animation_group->animation_sets[ visual->selected_set ];

                if( animation_group->animation_set_count == 0 ){
                    animation_set = &model->animation_groups[ ACTOR_ANIMATION_GROUP_IDLE ].animation_sets[0];
                }

                actor_model_animation_frame* animation_frame = &animation_set->frames[ visual->current_frame ];

                u32 frame_width = 0;
                u32 frame_height = 0;

                game_asset* spritesheet_asset = game_asset_get_from_handle( &state->assets, animation_set->spritesheet_id );

                if( spritesheet_asset->type == GAME_ASSET_NONE ){
                    game_asset* frame_bitmap_asset = game_asset_get_from_handle( &state->assets, animation_frame->texture_id );

                    frame_width = frame_bitmap_asset->bitmap.width;
                    frame_height = frame_bitmap_asset->bitmap.height;
                }else if( spritesheet_asset->type == GAME_ASSET_SPRITESHEET ){
                    game_asset_spritesheet_rect sprite_rect = spritesheet_asset->spritesheet.sprite_rects[animation_frame->spritesheet_index];

                    frame_width = sprite_rect.w;
                    frame_height = sprite_rect.h;
                }

                {
                    w_pixels = (frame_width) * actor->visual_info.scale.w * model->draw_scale;
                    h_pixels = (frame_height) * actor->visual_info.scale.h * model->draw_scale;
                }

                vec2 pixels_as_isometric = project_cartesian_to_isometric( 
                    v2 ((actor->visual_info.position.x) * half_tile_width,
                        (actor->visual_info.position.y) * half_tile_width));
                {
                    // pivot calculations
                    f32 pivot_x = (w_pixels) * animation_frame->origin_x;
                    f32 pivot_y = (h_pixels) * animation_frame->origin_y;

                    x_pixels = pixels_as_isometric.x - (pivot_x) + tile_offset_x;
                    y_pixels = pixels_as_isometric.y - (pivot_y) + tile_offset_y;
                }

                f32 r = actor->visual_info.rgba.r;
                f32 g = actor->visual_info.rgba.g;
                f32 b = actor->visual_info.rgba.b;
                f32 a = actor->visual_info.rgba.a;

                render_command drawn_frame = {};

                if( spritesheet_asset->type == GAME_ASSET_NONE ){
                    drawn_frame = render_command_textured_quad( x_pixels,
                                                                y_pixels,
                                                                w_pixels,
                                                                h_pixels,
                                                                r, g, b, a,
                                                                animation_frame->texture_id );
                }else if( spritesheet_asset->type == GAME_ASSET_SPRITESHEET ){
                    game_asset_spritesheet_rect sprite_rect = 
                        spritesheet_asset->spritesheet.sprite_rects[animation_frame->spritesheet_index];
                    drawn_frame = render_command_textured_quad( x_pixels,
                                                                y_pixels,
                                                                w_pixels,
                                                                h_pixels,
                                                                r, g, b, a,
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

                render_layer_push_command( layer, drawn_frame );

/*                 render_layer_push_command( layer, */ 
/*                         render_command_quad( */ 
/*                             pixels_as_isometric.x + tile_offset_x, */
/*                             pixels_as_isometric.y + tile_offset_y, */
/*                             5, */
/*                             5, */
/*                             r, g, b, a) ); */
            }
            break;
            case GAME_RENDER_OBJECT_PROJECTILE:
            default:
            {
                fprintf(stderr, 
                        "Render Object : %s : Not rendered yet...\n",
                        game_render_object_type_strings[object->type]);
            }
            break;
        }
    }
}

/*NOTE(jerry): Render layer family. Sort of like.... Extension functions?*/
static void render_layer_push_floating_message_cmds( render_layer* layer, floating_messages* messages, game_state* state );

static void game_initialize( game_state* state, renderer* renderer );
static void game_initialize_world( game_state* state, renderer* renderer );
static void game_initialize_resources( game_state* state, renderer* renderer );
static void game_process_event_stack( game_event_stack* stack, game_state* state );
static bool game_begin_dialogue_with_actor( game_state* state, actor* speaking_with );

static vec2 project_mouse_from_isometric(render_layer* layer_to_project_from, game_input* input) {
    vec2 real_mouse_coords = v2(input_get_mouse_x(input), input_get_mouse_y(input));
    vec2 projected_mouse = render_layer_screen_to_camera(layer_to_project_from, real_mouse_coords);
    // assume the mouse coordinates are already in isometric
    // space. Convert the "isometric" coordinates to unit space,
    // and then project back to cartesian.
    vec2 pixels_as_isometric = (vec2){
        (projected_mouse.x - tile_offset_x) / half_tile_width,
        (projected_mouse.y - tile_offset_y) / half_tile_width
    };

    pixels_as_isometric = project_isometric_to_cartesian( pixels_as_isometric );

    projected_mouse = pixels_as_isometric;
    return projected_mouse;
}

static vec2 center_point_for_actor(game_state* state, actor* target) {
    f32 screen_width = state->screen_width;
    f32 screen_height = state->screen_height;

    vec2 center_point = v2((target->position.x - target->size.w/2) * half_tile_width,
                           (target->position.y - target->size.h/2) * half_tile_width);

    return center_point;
}

static void game_center_on_point(game_state* state, vec2 center_point) {
    f32 screen_width = state->screen_width;
    f32 screen_height = state->screen_height;

    center_point = project_cartesian_to_isometric(center_point);

    camera_transition_move_to( &state->transition,
            v2(state->camera.x, state->camera.y),
            v2((screen_width / 2) - center_point.x * state->camera.scale,
               (screen_height / 2) - center_point.y * state->camera.scale));
}

static void game_center_camera_on_actor(game_state* state, actor* focus_on_actor) {
    vec2 center_point = center_point_for_actor(state, focus_on_actor);
    game_center_on_point(state, center_point);
}

static void game_start_death_sequence(game_state* state) {
    if (!state->playing_death_sequence) {
        state->playing_death_sequence = true;
        state->death_sequence_timer = 0.0f;
        state->bindings.disable_input = true;
        state->paused = false;
        game_ui_disable(state);
    }
}

static void game_end_death_sequence(game_state* state) {
    if (state->playing_death_sequence) {
        state->playing_death_sequence = false;
        state->death_sequence_timer = 0.0f;
        state->bindings.disable_input = false;
        state->paused = false;
        state->mode = GAME_STATE_MAIN_MENU;
    }
}

// at the moment this just means notifying all characters to show their bars.
// should show more info like name and actual health points
static void game_render_world_information(game_state* state) {
    for (unsigned actor_index = 0; actor_index < state->actors.count; ++actor_index) {
        actor* current_actor = actors_list_get_actor(&state->actors, actor_index);
        actor_start_showing_info_bars(current_actor);
    }
}

void show_game_cursor(game_state* state) {
    state->ui_info.cursor.hidden = false;
}

void hide_game_cursor(game_state* state) {
    state->ui_info.cursor.hidden = true;
}

static void game_journal_add_entry( game_journal* journal, char* title, char* text ){
    game_journal_entry* journal_entry = &journal->entries[ journal->entry_count++ ];

    journal_entry->entry_title = title;
    journal_entry->entry_text = text;
}

void floating_messages_on_actor( floating_messages* messages,
                                 actor* target,
                                 colorf message_color,
                                 char* message_string ) {
    const f32 start_x = target->position.x;
    const f32 start_y = target->position.y;

    floating_messages_push_message( 
            messages,
            message_string,
            v2(start_x, start_y),
            message_color,
            default_floating_message_lifetime
            );
}

static void draw_cursor( game_state* state, game_input* input ){
    if( input->relative_mouse_mode ){
        // do not draw cursor if relative mouse mode is on.
        return;
    }
    
    render_command cursor_draw = {};
    const f32 cursor_size = 16;
    {
        if (!state->ui_info.cursor.hidden) {
            game_asset_handle cursor_texture = 
                state->ui_info.cursor.cursor_textures[state->ui_info.cursor.action_mode];
            cursor_draw = render_command_textured_quad( input_get_mouse_x(input),
                    input_get_mouse_y(input),
                    cursor_size, 
                    cursor_size,
                    1, 1, 1, 1,
                    cursor_texture );
        } else {
            return;
        }

    }
    render_layer_push_command( &state->ui_layer, cursor_draw );
}

// make this look better.
// UI will always draw over actors. This is not a mistake
static void game_render_draw_filled_bar(render_layer* ui_layer,
                                        vec2 position,
                                        vec2 size,
                                        f32 current_value,
                                        f32 maximum_value,
                                        colorf empty_color,
                                        colorf filled_color) {
    if (maximum_value == 0) {
        return;
    }
    f32 percentage = current_value / maximum_value;

    if (percentage <= 0.0f) {
        percentage = 0.0f;
    }

    render_layer_push_command(ui_layer, 
            render_command_quad( 
                position.x,
                position.y,
                size.x,
                size.y,
                empty_color.r,
                empty_color.g,
                empty_color.b,
                empty_color.a));

    render_layer_push_command(ui_layer, 
            render_command_quad( 
                position.x,
                position.y,
                size.x * percentage,
                size.y,
                filled_color.r,
                filled_color.g,
                filled_color.b,
                filled_color.a));
}

static void game_render_actor_ui_elements(camera_state* game_camera, render_layer* ui_layer, actor* target) {
    const static f32 bar_width = 100;
    const static f32 bar_height = 7.5f;

    vec2 pixels_as_isometric = project_cartesian_to_isometric( 
            v2 ((target->visual_info.position.x) * half_tile_width,
                (target->visual_info.position.y) * half_tile_width));

    const f32 actor_base_position_x = (pixels_as_isometric.x + tile_offset_x) * game_camera->scale;
    const f32 actor_base_position_y = (pixels_as_isometric.y + tile_offset_y) * game_camera->scale; 

    f32 health_color_alpha = (target->visual_info.health_bar_show_time) / (health_bar_max_show_time);
    f32 action_points_color_alpha = (target->visual_info.action_points_bar_show_time) / (action_points_bar_max_show_time);

    game_render_draw_filled_bar(ui_layer,
                                v2(actor_base_position_x - (bar_width / 2), actor_base_position_y),
                                v2(bar_width, bar_height),
                                target->health_points,
                                target->max_health_points,
                                (colorf){1, 0, 0, health_color_alpha},
                                (colorf){0, 1, 0, health_color_alpha});

    game_render_draw_filled_bar(ui_layer,
                                v2(actor_base_position_x - (bar_width / 2), actor_base_position_y + (bar_height)),
                                v2(bar_width, bar_height),
                                target->action_points,
                                target->max_action_points,
                                (colorf){0, 0, 0, action_points_color_alpha},
                                (colorf){0, 0, 1, action_points_color_alpha});
}

#ifdef DEBUG_BUILD
#include "wanderer_editors.c"
#endif

static void update_render_intro( game_state* state,
                                 renderer* renderer,
                                 game_input* input,
                                 f32 delta_time ){
    stub_important_code("no intro yet");
}

static bool game_try_to_open_dialogue(game_state* state, actor* target) {
    if (!actor_is_dead(target) && target != game_get_player_actor(state)) {
        state->ui_info.cursor.action_mode = CURSOR_DIALOGUE;
        if (game_begin_dialogue_with_actor(state, target)) {
            vec2 actor_center_point = center_point_for_actor(state, target);
            // accounts
            actor_center_point.x += (5 * half_tile_width);
            actor_center_point.y += (5 * half_tile_width);
            game_center_on_point(state, actor_center_point);
            return true;
        } else {
            return false;
        }
    }

    return false;
}

static void game_handle_mouse_input(game_state* state, 
                                    game_input* input,
                                    f32 delta_time) {
    i32 mouse_left_clicked = input_is_mouse_left_click( input );
    i32 mouse_middle_clicked = input_is_mouse_middle_click( input );
    i32 mouse_right_clicked = input_is_mouse_right_click( input );

    i32 mouse_left_down = input_is_mouse_left_down( input );
    i32 mouse_middle_down = input_is_mouse_middle_down( input );
    i32 mouse_right_down = input_is_mouse_right_down( input );

    vec2 projected_mouse = project_mouse_from_isometric(&state->game_layer, input);

    game_moused_over_selection moused_over = game_mouse_pick( projected_mouse, state );
    actor* player_actor = game_get_player_actor( state );
    game_cursor* cursor = &state->ui_info.cursor;

    if( state->ui_info.mode == GAME_UI_STATE_INGAME &&
        !state->ui_info.touching_ui ){
        // I would do this with a function pointer but
        // as per the style of the project everything else is a switch statement
        // so yeah.
        switch (cursor->override_state) {
            case CURSOR_OVERRIDE_STATE_ATTACK:
            {
                cursor->action_mode = CURSOR_ATTACK;
                if (mouse_left_clicked) {
                    if (moused_over.type == MOUSED_OVER_ACTOR) {
                        actor* over_actor = 
                            actors_list_get_actor( &state->actors, moused_over.selected_index );
                        // should probably open some menus to select
                        // type of attack like from spell or something...
                        if (!actor_is_dead(over_actor) && over_actor != game_get_player_actor(state)) {
                            actor_command_attack(game_get_player_actor(state), state, over_actor);
                        }
                    } else {
                        fprintf(stderr, "attack override did not click on actor\n");
                    }
                    cursor->override_state = CURSOR_OVERRIDE_STATE_NONE;
                }
            }
            break;
            case CURSOR_OVERRIDE_STATE_DIALOGUE:
            {
                cursor->action_mode = CURSOR_DIALOGUE;
                if (mouse_left_clicked) {
                    if (moused_over.type == MOUSED_OVER_ACTOR) {
                        actor* over_actor = 
                            actors_list_get_actor( &state->actors, moused_over.selected_index );

                        if (!game_try_to_open_dialogue(state, over_actor)) {
                            fprintf(stderr, "The actor is either in combat and cannot speak, or has no dialogue. Or you are in combat.\n");
                        } else {
                            input->current.mouse.left_clicked = false;
                        }
                    } else {
                        fprintf(stderr, "dialogue override did not click on actor\n");
                    }
                    cursor->override_state = CURSOR_OVERRIDE_STATE_NONE;
                }
            }
            break;
            case CURSOR_OVERRIDE_STATE_NONE:
            {
                cursor->action_mode = CURSOR_SELECT;
                switch( moused_over.type ){
                    case MOUSED_OVER_NOTHING:
                    {
                        if( mouse_left_clicked ){
                            actor_command_move_to( game_get_player_actor(state),
                                                   state,
                                                   v2(projected_mouse.x,
                                                      projected_mouse.y));
                        }

                        if( mouse_right_clicked ){
                            floating_messages_on_actor( 
                                    &state->floating_messages,
                                    player_actor,
                                    (colorf) {1, 1, 1, 1},
                                    "Preliminary test msg" );

                            char* test_entry_title = "Midnight Howling?";
                            char* test_entry_text = "According to one of the residents of this town, there have been reports of howling at night.\n\nWolves perhaps? Although why that would trouble a small town is beyond me, these citizens look fairly well armed for a pack of wolves to be threatening.";
                            game_journal_add_entry( &state->journal, test_entry_title, test_entry_text );
                        }
                    }
                    break;
                    case MOUSED_OVER_ACTOR:
                    {
                        actor* over_actor = 
                            actors_list_get_actor( &state->actors, moused_over.selected_index );

                        b32 is_aggressive = actor_is_aggressive_to(over_actor, game_get_player_actor(state));

                        if (!actor_is_dead(over_actor) && over_actor != game_get_player_actor(state)) {
                            if (is_aggressive) {
                                cursor->action_mode = CURSOR_ATTACK;
                            } else {
                                cursor->action_mode = CURSOR_DIALOGUE;
                            }
                        } else {
                            cursor->action_mode = CURSOR_SELECT;
                        }

                        // Figure out what to do about that camera situation
                        if( mouse_left_clicked ){
                            if (!is_aggressive) {
                                if (!game_try_to_open_dialogue(state, over_actor)) {
                                    fprintf(stderr, "The actor is either in combat and cannot speak, or has no dialogue. Or you are in combat.\n");
                                } else {
                                    input->current.mouse.left_clicked = false;
                                }
                            } else {
                                if (!actor_is_dead(over_actor) && over_actor != game_get_player_actor(state)) {
                                    actor_command_attack(game_get_player_actor(state), state, over_actor);
                                }
                            }
                        }
                    }
                    break;
                    case MOUSED_OVER_CONTAINER:
                    {
                        // SHOULD OPEN MENU!
                        // TEMPORARY STUFF SORT OF.
                        cursor->action_mode = CURSOR_ACTIVATE;

                        actor* player_actor = game_get_player_actor( state );

                        container* target_container = tilemap_get_container( player_actor->current_map, moused_over.selected_index );
                        // don't do this maybe.
                        b32 can_open = false;

                        if( mouse_left_clicked ){
                            can_open = true;
                            if( target_container->locked ){
                                can_open = false;
                                if( target_container->requires_key_item ){
                                    /* item_slot* key_item_in_inventory = */
                                    /*     actor_find_item_of_id( player_actor, target_container->key_id ); */

                                    /* if( key_item_in_inventory ){ */
                                    /*     target_container->locked = false; */
                                    /*     can_open = true; */
                                    /* }else{ */
                                    /*     floating_messages_on_actor( */ 
                                    /*             &state->floating_messages, */
                                    /*             player_actor, */
                                    /*             "You do not have the key to this container!" ); */
                                    /* } */
                                }else{
                                    // random chance of unlock.
                                    u16 unlock_roll = random_integer_ranged(1, 100);
                                    if( unlock_roll >= target_container->lock_level ){
                                        target_container->locked = false;
                                        can_open = true;
                                    }else{
                                        floating_messages_on_actor( 
                                                &state->floating_messages,
                                                player_actor,
                                                (colorf) {1, 0, 0, 1},
                                                "You could not unlock this container." );
                                    }
                                }
                            }
                        }

                        if( mouse_left_clicked && !can_open ){
                            can_open = true;
                            if( target_container->locked ){
                                u16 bash_roll = random_integer_ranged(1, 100) - 20;
                                if( bash_roll >= target_container->lock_level ){
                                    target_container->locked = false;
                                    can_open = true;
                                }else{
                                    floating_messages_on_actor( 
                                            &state->floating_messages,
                                            player_actor,
                                            (colorf) {1, 0, 0, 1},
                                            "Your bash failed." );
                                }
                            }
                        }

                        if( can_open ){
                            game_ui_open_loot_container( state, moused_over.selected_index );
                        } else {
                            /* stub_important_code("Implement game ui inventory stuff!\n"); */
                        }
                    }
                    break;
                    case MOUSED_OVER_PICKUP_ITEM:
                    {
                        map_item_pickup* over_pickup = 
                            tilemap_get_pickup( player_actor->current_map, moused_over.selected_index );

                        cursor->action_mode = CURSOR_ACTIVATE;

                        if( mouse_left_clicked ){
                            actor_pickup_item_from_map(player_actor, moused_over.selected_index);
                        }
                    }
                    break;
                }
            }
            break;
        }
    }
}

static void update_render_gameplay( game_state* state, 
                                    renderer* renderer, 
                                    game_input* input, 
                                    f32 delta_time ){
#ifdef DEBUG_BUILD
    i32 debug_change_level_pressed = input_is_key_pressed( input, INPUT_KEY_FORWARDSLASH );
    i32 e_pressed = input_is_key_pressed( input, INPUT_KEY_E );

    i32 one_pressed = input_is_key_pressed(input, INPUT_KEY_1);
    i32 two_pressed = input_is_key_pressed(input, INPUT_KEY_2);
    i32 three_pressed = input_is_key_pressed(input, INPUT_KEY_3);
    i32 four_pressed = input_is_key_pressed(input, INPUT_KEY_4);
#endif
    if (!state->playing_death_sequence) {
        if( state->active_cutscene ||
            state->active_dialogue ){
            state->bindings.disable_input = true;
        }else{
            state->bindings.disable_input = false;
        }
    }

    renderer_set_screen_dimensions( renderer, state->screen_width, state->screen_height );
    render_layer_camera_set_scale( &state->game_layer, state->camera.scale );
    render_layer_camera_set_scale( &state->game_text_layer, 1 );
    render_layer_camera_set_position( &state->game_layer, state->camera.x, state->camera.y );
    render_layer_camera_set_position( &state->game_text_layer, state->camera.x, state->camera.y );

    actor* player = game_get_player_actor( state );

    if( game_action_bindings_action_active( &state->bindings, GAME_ACTION_PAUSE_GAME ) ){
        game_ui_toggle_pause( state );
    }

#ifdef DEBUG_BUILD
    // TODO(jerry): replace with a special effect or something?
    if( one_pressed ){
        actor_hurt(player, state, pure_physical_damage(random_integer_ranged(5, 15), 0));
    }

    if( two_pressed ){
        actor_hurt(player, state, magic_damage(random_integer_ranged(5, 15)));
    }

    // first spell effects
    // then full on spells.
    if( three_pressed ){
        actor_push_effect(player, state, test_poison_spell_effect());
    }

    if( four_pressed ){
        actor_push_effect(player, state, test_delayed_poison_spell_effect());
    }

    if ( e_pressed ) {
        // TODO(jerry): Replace with a healing spell
        /* actor_heal(player, state, 20); */
        fprintf(stderr, "bekstebu?\n");
        actor_force_cast_spell(player, state, "TESTHEAL", make_spell_target_actor(player));
    }
#endif

    if( !state->bindings.disable_input ){
        const f32 camera_speed = 300.f;

        // camera
        {
            if( game_action_bindings_action_active( &state->bindings, GAME_ACTION_CAMERA_MOVE_UP ) ){
                state->camera.y += delta_time * camera_speed;
            }

            if( game_action_bindings_action_active( &state->bindings, GAME_ACTION_CAMERA_MOVE_DOWN ) ){
                state->camera.y -= delta_time * camera_speed;
            }

            if( game_action_bindings_action_active( &state->bindings, GAME_ACTION_CAMERA_MOVE_LEFT ) ){
                state->camera.x += delta_time * camera_speed;
            }

            if( game_action_bindings_action_active( &state->bindings, GAME_ACTION_CAMERA_MOVE_RIGHT ) ){
                state->camera.x -= delta_time * camera_speed;
            }
        }

        if( game_action_bindings_action_active( &state->bindings, GAME_ACTION_PLAYER_ENDS_COMBAT_TURN ) ){
            if (state->player_is_in_combat) {
                actor_end_turn(player);
            } else {
                stub_less_important("not in combat. do nothing");
            } 
        }

        if( game_action_bindings_action_active( &state->bindings, GAME_ACTION_TOGGLE_INVENTORY ) ){
            game_ui_toggle_player_inventory( state );
        }

        if( game_action_bindings_action_active( &state->bindings, GAME_ACTION_TOGGLE_JOURNAL ) ){
            game_ui_toggle_journal( state );
        }

        if( game_action_bindings_action_active( &state->bindings, GAME_ACTION_TOGGLE_CHARACTERSHEET ) ){
            game_ui_toggle_character_sheet( state );
        }

        if( game_action_bindings_action_active( &state->bindings, GAME_ACTION_CAMERA_RECENTER_ON_PLAYER ) ){
            game_center_camera_on_actor(state, player);
        }

        // 
        // mouse selection system,
        // and input.
        //
        game_handle_mouse_input(state, input, delta_time); 
    }

#ifdef DEBUG_BUILD
    if( debug_change_level_pressed ){
        state->current_world_map += 1;
        state->current_world_map %= state->tilemaps.current;

        game_get_player_actor( state )->current_map = tilemap_list_get_tilemap( &state->tilemaps, state->current_world_map );
    }
#endif

    if( !state->paused ){
        if( state->active_cutscene ){
            cutscene_info_run( state->active_cutscene, state, delta_time );

            if( !state->active_cutscene->running ){
                state->active_cutscene = NULL;
                /*
                  More accurately this should just disable all current AI
                  thinking...
                 */
            }
        }else{
            actor* player = game_get_player_actor(state);

            b32 was_in_combat_before = state->player_is_in_combat;
            b32 is_in_combat_now = false;
            
            // Game checks in-combat state.
            {
                for (u64 actor_index = 0; actor_index < state->actors.count; ++actor_index) {
                    actor* current_actor = 
                        actors_list_get_actor( &state->actors, actor_index );

                    if (current_actor != player &&
                        (current_actor->current_map == player->current_map)) {
                        if (!actor_is_dead(current_actor)) {
                            is_in_combat_now |=
                                actor_is_aggressive_to(current_actor, player);

                            if (is_in_combat_now) {
                                break;
                            }
                        }
                    }
                }
            }

            state->player_is_in_combat = is_in_combat_now;

            // Pre and Post combat stuff.
            if (!was_in_combat_before && is_in_combat_now) {
                game_ui_show_combat_initial_elements(state);
                state->combat_round = game_build_combat_round(state);
                // reset "turn" timer for when we get back into real time stuff.
                state->real_time_turn_timer = length_of_real_time_turn;
            }else if (was_in_combat_before && !is_in_combat_now) {
                game_ui_show_combat_finished_elements(state);
                game_finish_combat_round(state, &state->combat_round);
            }

            if (!is_in_combat_now) {
                for (u64 actor_index = 0; actor_index < state->actors.count; ++actor_index) {
                    actor* current_actor = 
                        actors_list_get_actor( &state->actors, actor_index );

                    actor_think(current_actor, state, delta_time);
                    if( current_actor->current_map == player->current_map ){
                        /* actor_process_trigger_interactions( state, current_actor ); */
                        actor_run_command(current_actor, state, delta_time);

                        if (state->real_time_turn_timer <= 0.0f) {
                            actor_turn_update(current_actor, state);
                        }
                    } 
                }

                if (state->real_time_turn_timer <= 0.0f) {
                    state->real_time_turn_timer = length_of_real_time_turn;
                }
                state->real_time_turn_timer -= delta_time;
            } else {
                combat_participant current_participant = game_combat_round_get_current_participant(&state->combat_round);
                actor* participating_actor = combat_participant_lookup_actor(&state->actors, current_participant);

                // should I do the map check here?
                // I mean I probably shouldn't even really do the map check...
                // but that requires later rewriting.
                if (participating_actor) {
                    // whenever the actor AI or player "submits" there actions.
                    // Which means they've run out of stuff to do.
                    // This means that their current command is finished and that they have sent
                    // a submitted message. Well it's a boolean since I don't do actual message passing
                    // in this engine.
                    // The last thing it takes for the turn to finish is for there to be no current projectiles on the screen.
                    // Since the engine is turn-based we know that the projectiles only last per turn.
                    // Once a turn is complete either there are newer projectiles or none left at all so we should probably check
                    // for no living projectiles.
                    // In the normal gameloop things just happen and things explode.
                    actor_on_turn_start_update(participating_actor, state);
                    actor_think(participating_actor, state, delta_time);

                    i32 result_of_run = actor_run_command(participating_actor, state, delta_time);

                    if (result_of_run == RUN_COMMAND_NO_ACTION_POINTS_TO_EXECUTE) {
                        floating_messages_on_actor(&state->floating_messages, participating_actor, (colorf){1, 0, 0, 1}, "Not enough action points to do action");
                    }
                        
                    if (actor_finished_combat_turn_actions(participating_actor)) {
                        b32 end_of_round = !combat_round_advance_to_next_participant(&state->combat_round);

                        actor_turn_update(participating_actor, state);
                        if (end_of_round) {
                            actor_on_turn_end_update(participating_actor, state);
#if 0 
                            for (u64 participant_index = 0;
                                 participant_index < state->combat_round.participant_count;
                                 ++participant_index) {
                                combat_participant participant = state->combat_round.participants[participant_index];
                                actor* current_actor = combat_participant_lookup_actor(&state->actors, participant);
                                actor_turn_update(current_actor, state);

                            }
#endif

                            state->combat_round = game_build_combat_round(state);
                        }
                    }
                }
            }

            if (actor_is_dead(player)) {
                game_start_death_sequence(state);
            }
        }

        game_process_event_stack( &state->event_stack, state );
        floating_messages_update( &state->floating_messages, delta_time );

        if( state->transition.started ){
            const f32 duration = 1.00f;

            state->camera.x = 
                cubic_ease_out( state->transition.from.x, 
                                state->transition.to.x - state->transition.from.x, 
                                duration, state->transition.time);
            state->camera.y = 
                cubic_ease_out( state->transition.from.y, 
                                state->transition.to.y - state->transition.from.y, 
                                duration, state->transition.time);

            state->transition.time += (delta_time);

            if( state->transition.time >= 1.f ){
                state->transition.started = false;
                state->transition.time = 0;
            }
        }
    }

    game_render_list render_list = {};
    render_list.current_scene_map = game_get_player_actor( state )->current_map;
    render_list.count = 0;
    render_list.objects = memory_pool_allocate( &state->render_list_memory, sizeof( game_render_object ) * 4096 );

    render_layer_clear_color_buffer( &state->game_layer, 0.0, 0.0, 0.0, 1.0 );
    {
        tilemap* current_tilemap = game_get_player_actor( state )->current_map;
        for( u32 y = 0; y < current_tilemap->height; ++y ){
            for( u32 x = 0; x < current_tilemap->width; ++x ){
                map_tile* current_tile = &current_tilemap->tiles[y][x];

                game_render_object tile_object;
                tile_object.type = GAME_RENDER_OBJECT_TILE;

                tile_object.x_index = x;
                tile_object.y_index = y;

                game_render_list_push( &render_list, tile_object );
            }
        }

#if 0
        for (u32 trigger_index = 0; trigger_index < TILEMAP_MAX_TRIGGERS; ++trigger_index) {
        }
#endif
        for (u32 container_index = 0; container_index < current_tilemap->container_count; ++container_index) {
            game_render_object container_object;
            container_object.type = GAME_RENDER_OBJECT_CONTAINER;
            container_object.index_to_object = container_index;

            game_render_list_push(&render_list, container_object);
        }

        for (u32 pickup_index = 0; pickup_index < TILEMAP_MAX_CONTAINERS; ++pickup_index) {
            map_item_pickup* current_pickup = &current_tilemap->pickups[pickup_index];

            if (current_pickup->contains_item) {
                game_render_object pickup_object;
                pickup_object.type = GAME_RENDER_OBJECT_ITEM_DROP;
                pickup_object.index_to_object = pickup_index;

                game_render_list_push(&render_list, pickup_object);
            }
        }
    }

    for( u64 actor_index = 0;
         actor_index < state->actors.count;
         ++actor_index ){
        actor* current_actor = 
            actors_list_get_actor( &state->actors, actor_index );

        if( current_actor->current_map == player->current_map ){
            /* actor_process_trigger_interactions( state, current_actor ); */
            game_render_object actor_object;

            actor_object.type = GAME_RENDER_OBJECT_ACTOR;
            actor_object.index_to_object = actor_index;

            game_render_list_push( &render_list, actor_object );
            game_render_actor_ui_elements(&state->camera, &state->game_text_layer, current_actor);
            actor_update_visual_info( current_actor, &state->assets, delta_time );
        }
    } 

    render_layer_push_floating_message_cmds( &state->game_text_layer, &state->floating_messages, state );

    if (actor_is_currently_walking(player)) {
        game_render_list_push( &render_list, (game_render_object){ .type = GAME_RENDER_OBJECT_SELECTED_TARGET_TILE } );
    }

    game_render_list_sort_render_objects( &render_list, state );
    game_render_list_submit_all_render_commands( &render_list, &state->game_layer, state );

    // This is here because I might have to render some info in the game layer atop the already sorted
    // game stuff.
    if( game_action_bindings_action_active( &state->bindings, GAME_ACTION_SHOW_WORLD_INFORMATION ) ){
        game_render_world_information(state); 
    }

    if (state->playing_death_sequence) {
        // fade to black...
        // and return to main menu.
        // in the future make a messagebox.
        state->death_sequence_timer += delta_time;

        if (state->death_sequence_timer >= death_sequence_length + 5.0f) {
            game_end_death_sequence(state);
        }

        f32 alpha = (state->death_sequence_timer - 2.0f) / death_sequence_length;

        if (alpha <= 0.0f) {
            alpha = 0.0f;
        }

        render_layer_push_quad(&state->ui_layer,
                0,
                0,
                state->screen_width,
                state->screen_height,
                0, 0, 0, alpha);
    }

    renderer_draw_render_layer( renderer, &state->game_layer );
    renderer_draw_render_layer( renderer, &state->game_text_layer );
}

static void update_render_main_menu( game_state* state, 
                                     renderer* renderer, 
                                     game_input* input, 
                                     f32 delta_time ){
    render_layer_clear_color_buffer( &state->ui_layer, 0.0, 0.0, 0.0, 1.0 );

    state->ui_info.mode = GAME_UI_STATE_MAINMENU;

    renderer_set_screen_dimensions( renderer, state->screen_width, state->screen_height );

    render_layer_camera_set_position( &state->background, 0, 0 );
    render_layer_camera_set_position( &state->menu_ui, 0, 0 );

    render_layer_camera_set_scale( &state->menu_ui, 1 );
    render_layer_camera_set_scale( &state->background, 1 );
}

void update_render_game( game_state* state,
                         renderer* renderer,
                         game_input *input,
                         f32 delta_time){
    static bool initialized = false;
    {
        if( !initialized ){
            initialized = true;

            game_initialize( state, renderer );
            seed_random(time(NULL));
            state->bindings.input = input;
        }
    }

    memset( &state->game_layer, 0, sizeof(state->game_layer) );
    memset( &state->game_text_layer, 0, sizeof(state->game_text_layer) );
    memset( &state->ui_layer, 0, sizeof(state->ui_layer) );
    memset( &state->background, 0, sizeof(state->background) );
    memset( &state->menu_ui, 0, sizeof(state->menu_ui) );
    
    render_layer_camera_set_position( &state->ui_layer, 0, 0 );
    render_layer_camera_set_scale( &state->ui_layer, 1 );

    state->strings_scratch = (memory_pool){};
    state->dialogue_memory = (memory_pool){};
    state->render_list_memory = (memory_pool){};
    {
        u64 mem_cursor = 0;
        // kind of static memory.
        memory_sub_pool_init( &state->dialogue_memory, state->scratch_memory + mem_cursor, MB(3));
        mem_cursor += MB(3);
        // frame memory
        memory_sub_pool_init( &state->render_list_memory, state->scratch_memory + mem_cursor, MB(3));
        mem_cursor += MB(3);
        memory_sub_pool_init( &state->strings_scratch, state->scratch_memory + mem_cursor, KB(32) );
        mem_cursor += KB(32);
    }

    if( state->mode == GAME_STATE_QUIT ){
        return;
    }

    switch( state->mode ){
        case GAME_STATE_MAIN_MENU:
        {
            update_render_main_menu( state, renderer, input, delta_time );
        }
        break;
        case GAME_STATE_EDITOR_MODEL_EDITOR:
        {
            update_render_model_editor( state, renderer, input, delta_time );
        }
        break;

        case GAME_STATE_EDITOR_DIALOGUE_EDITOR:
        {
            update_render_dialogue_editor( state, renderer, input, delta_time );
        }
        break;

        case GAME_STATE_INTRO:
        {
            update_render_intro( state, renderer, input, delta_time );
        }
        break;

        case GAME_STATE_GAME_PLAY:
        {
            update_render_gameplay( state, renderer, input, delta_time );
        }
        break;
    }

    game_ui_update_render( state, renderer, input, delta_time );
    draw_cursor( state, input );

    renderer_draw_render_layer( renderer, &state->ui_layer );
}

#include "config.h"

// not complete but goood enough for demo.
static item read_config_block_into_item(config_variable_block* block) {
    item new_item = {};

    config_variable* item_name =
        config_block_find_first_variable(block, "name");
    if (item_name) {
        item_set_name(&new_item, config_variable_try_string(item_name));
    } else {
        // err
    }

    config_variable* item_description =
        config_block_find_first_variable(block, "description");

    if (item_description) {
        item_set_description(&new_item, config_variable_try_string(item_description));
    } else {
        // err
    }

    config_variable* item_type =
        config_block_find_first_variable(block, "type");

    if (item_type) {
        const char* item_type_str = config_variable_try_string(item_type);
        new_item.type = map_string_to_item_type(item_type_str);
    } else {
        // err
    }

    config_variable* item_class_id =
        config_block_find_first_variable(block, "class_id");

    if (item_class_id) {
        const char* item_class_id_str = config_variable_try_string(item_class_id);
        new_item.class_id = map_string_to_item_class_id(item_class_id_str);
    } else {
        // probably not err.
    }

    // I don't do the check here since none of these are strings or requiring
    // a string, which could possibly backfire. The number conversions have fallbacks.
    config_variable* item_stack_max =
        config_block_find_first_variable(block, "stack_max");
    config_variable* item_weight =
        config_block_find_first_variable(block, "weight");
    config_variable* item_value =
        config_block_find_first_variable(block, "value");
    config_variable* item_atlas_icon =
        config_block_find_first_variable(block, "atlas_icon");
    {
        new_item.stack_max =
            config_variable_try_integer(item_stack_max);
        new_item.weight =
            config_variable_try_integer(item_weight);
        new_item.value =
            config_variable_try_integer(item_value);
        new_item.atlas_icon =
            config_variable_try_integer(item_atlas_icon);
    }

    /*
     * For now I'm silencing anything related to weapons and damage because
     * I need to change stuff.
     */
#if 0
    // I think I should check... But it doesn't really matter.
    config_variable* item_weapon_enchant_level =
        config_block_find_first_variable(block, "enchant_level");
    config_variable* item_weapon_physical_damage_type =
        config_block_find_first_variable(block, "physical_damage_type");
    config_variable* item_weapon_physical_damage_min =
        config_block_find_first_variable(block, "physical_damage_min");
    config_variable* item_weapon_physical_damage_max =
        config_block_find_first_variable(block, "physical_damage_max");
    config_variable* item_weapon_takes_ammunition =
        config_block_find_first_variable(block, "takes_ammunition");
    config_variable* item_weapon_ammunition_class_id =
        config_block_find_first_variable(block, "ammunition_class_id");
    {
        new_item.as_weapon.enchant_level =
            config_variable_try_integer(item_weapon_enchant_level);
        new_item.as_weapon.takes_ammunition =
            config_variable_try_integer(item_weapon_takes_ammunition);

        new_item.as_weapon.damage.physical.min =
            config_variable_try_integer(item_weapon_physical_damage_min);
        new_item.as_weapon.damage.physical.max =
            config_variable_try_integer(item_weapon_physical_damage_max);

        // TODO(jerry): Redefine items slightly to retrofit new damage types...
        if (item_weapon_physical_damage_type) {
            new_item.as_weapon.damage.physical_damage_type =
                map_string_to_physical_damage_type(item_weapon_physical_damage_type);
        } else {
            // err
        }

        if (item_weapon_ammunition_class_id) {
            new_item.as_weapon.ammunition_class_id =
                map_string_to_item_class_id(item_weapon_ammunition_class_id);
        } else {
            // probably err
        }
    }
#endif

    fprintf(stderr, "Finished parsing new item : %s\n", config_variable_try_string(item_name));
    return new_item;
}

static void game_initialize_spell_dictionary(game_state* state, renderer* renderer) {
    /*
      I'd be reading this from a file...
    */
    const u64 spells_to_allocate = 9000;
    spell_dictionary_init(&state->spells, spells_to_allocate);

    spell test_heal = {};
    test_heal.name = strings_make_static("Healing");
    test_heal.description = strings_make_static("This is a test spell okay?");
    test_heal.level = 1;
    test_heal.target_flags = MAGIC_EFFECT_TARGET_FLAGS_CASTER | MAGIC_EFFECT_TARGET_FLAGS_TARGET;
    test_heal.action_points_cost = 4;

    // me no approve of this malloc but this will work for now...
    // we'll allow the memory leak since it doesn't matter too much.
    test_heal.effect_count = 2;
    test_heal.effects = memory_allocate(sizeof(magic_effect) * test_heal.effect_count);
    test_heal.effects[0] = test_heal_spell_effect();
    /* test_heal.effects[1] = test_flash_visual_effect; */
    spell_dictionary_add_spell(&state->spells, test_heal, "TESTHEAL");
}

static void game_initialize_item_dictionary( game_state* state, renderer* renderer ) {
    config_info items_config = config_parse_from_file("data/items.config");

    // all blocks should be an item.
    // all of the top level items should be blocks.
    item_dictionary_initialize(&state->items, items_config.count);

    for (unsigned block_index = 0; block_index < items_config.count; ++block_index) {
        config_variable_block* current_block = items_config.variables[block_index].value.as_block;
        item read_item = read_config_block_into_item(current_block);
        item_dictionary_add_item(&state->items, read_item);
    }

    config_info_free_all_fields(&items_config);
} 

static void game_initialize( game_state* state, renderer* renderer ){
    fprintf(stderr, "sizeof(gameasset): %lld\n", sizeof(game_asset));
    /*Memory initialization*/
    {
        {
            // Actually this isn't true anymore, but I forgot to remove the comment
            // Now they're allocated based on a linear allocator.
            /*Dialogues are still f**king huge.... Much larger than I'd like personally....*/
            paged_memory_pool_init( &state->actors.actor_paged_pool, sizeof( struct actor ) * 10 );
            paged_memory_pool_init( &state->tilemaps.tilemap_paged_pool, sizeof( struct tilemap ) * 4 );

            state->actors.actor_paged_pool.tag_name = "Actor Paged Pool Memory";
            state->tilemaps.tilemap_paged_pool.tag_name = "Tilemaps Paged Pool Memory";

            strings_initialize();
            state->gui_state.scratch_pool = &state->strings_scratch;
        }
    }

    game_initialize_item_dictionary(state, renderer);
    game_initialize_spell_dictionary(state, renderer);

    // Input Contextualization Binder thing
    // this probably isn't needed.... Too late now!
    // NGL this was completely pointless but it's good to know since I know how to make this now.
    {
        game_action_bindings_set_current_context( &state->bindings, GAME_INPUT_MODE_GAMEPLAY );

        // MENU

        game_action_bindings_add_binding( &state->bindings, GAME_INPUT_MODE_ALL, 
                GAME_ACTION_PAUSE_GAME, INPUT_KEY_ESCAPE, GAME_INPUT_KEY_PRESSED );

        // CAMERA BINDINGS 
        game_action_bindings_add_binding( &state->bindings, GAME_INPUT_MODE_GAMEPLAY, 
                GAME_ACTION_CAMERA_MOVE_UP, INPUT_KEY_W, GAME_INPUT_KEY_DOWN );
        game_action_bindings_add_binding( &state->bindings, GAME_INPUT_MODE_GAMEPLAY, 
                GAME_ACTION_CAMERA_MOVE_UP, INPUT_KEY_UP, GAME_INPUT_KEY_DOWN );

        game_action_bindings_add_binding( &state->bindings, GAME_INPUT_MODE_GAMEPLAY, 
                GAME_ACTION_CAMERA_MOVE_DOWN, INPUT_KEY_S, GAME_INPUT_KEY_DOWN );
        game_action_bindings_add_binding( &state->bindings, GAME_INPUT_MODE_GAMEPLAY, 
                GAME_ACTION_CAMERA_MOVE_DOWN, INPUT_KEY_DOWN, GAME_INPUT_KEY_DOWN );

        game_action_bindings_add_binding( &state->bindings, GAME_INPUT_MODE_GAMEPLAY, 
                GAME_ACTION_CAMERA_MOVE_LEFT, INPUT_KEY_A, GAME_INPUT_KEY_DOWN );
        game_action_bindings_add_binding( &state->bindings, GAME_INPUT_MODE_GAMEPLAY, 
                GAME_ACTION_CAMERA_MOVE_LEFT, INPUT_KEY_LEFT, GAME_INPUT_KEY_DOWN );

        game_action_bindings_add_binding( &state->bindings, GAME_INPUT_MODE_GAMEPLAY, 
                GAME_ACTION_CAMERA_MOVE_RIGHT, INPUT_KEY_D, GAME_INPUT_KEY_DOWN );
        game_action_bindings_add_binding( &state->bindings, GAME_INPUT_MODE_GAMEPLAY, 
                GAME_ACTION_CAMERA_MOVE_RIGHT, INPUT_KEY_RIGHT, GAME_INPUT_KEY_DOWN );

        // GAMEPLAY

        game_action_bindings_add_binding( &state->bindings, GAME_INPUT_MODE_GAMEPLAY, 
                GAME_ACTION_PLAYER_ENDS_COMBAT_TURN, INPUT_KEY_SPACE, GAME_INPUT_KEY_PRESSED );
        game_action_bindings_add_binding( &state->bindings, GAME_INPUT_MODE_GAMEPLAY, 
                GAME_ACTION_SHOW_WORLD_INFORMATION, INPUT_KEY_TAB, GAME_INPUT_KEY_DOWN );

        game_action_bindings_add_binding( &state->bindings, GAME_INPUT_MODE_GAMEPLAY,
                GAME_ACTION_CAMERA_RECENTER_ON_PLAYER, INPUT_KEY_0, GAME_INPUT_KEY_PRESSED );

        game_action_bindings_add_binding( &state->bindings, GAME_INPUT_MODE_GAMEPLAY,
                GAME_ACTION_TOGGLE_INVENTORY, INPUT_KEY_I, GAME_INPUT_KEY_PRESSED );
        game_action_bindings_add_binding( &state->bindings, GAME_INPUT_MODE_IN_INVENTORY,
                GAME_ACTION_TOGGLE_INVENTORY, INPUT_KEY_I, GAME_INPUT_KEY_PRESSED );

        game_action_bindings_add_binding( &state->bindings, GAME_INPUT_MODE_GAMEPLAY,
                GAME_ACTION_TOGGLE_CHARACTERSHEET, INPUT_KEY_R, GAME_INPUT_KEY_PRESSED );
        game_action_bindings_add_binding( &state->bindings, GAME_INPUT_MODE_IN_CHARACTERSHEET,
                GAME_ACTION_TOGGLE_CHARACTERSHEET, INPUT_KEY_R, GAME_INPUT_KEY_PRESSED );

        game_action_bindings_add_binding( &state->bindings, GAME_INPUT_MODE_IN_JOURNAL,
                GAME_ACTION_TOGGLE_JOURNAL, INPUT_KEY_J, GAME_INPUT_KEY_PRESSED );
        game_action_bindings_add_binding( &state->bindings, GAME_INPUT_MODE_GAMEPLAY,
                GAME_ACTION_TOGGLE_JOURNAL, INPUT_KEY_J, GAME_INPUT_KEY_PRESSED );
    }

    state->camera.x = 0;
    state->camera.y = 0;
    state->camera.scale = camera_scale;

    game_initialize_resources( state, renderer );
    game_initialize_world( state, renderer );
}

static void game_initialize_resources( game_state* state, renderer* renderer ){
    // initialize engine back pointer crap.
    game_assets_init( &state->assets, renderer );
    {
        state->gothic_font =
            game_asset_load_font( &state->assets,
                                  "data/gothic_pixel.ttf", 32);
        state->bigger_font_texture = 
            game_asset_load_font( &state->assets,
                    "data/LiberationSans-Regular.ttf", 48 );

        state->font_texture =
            game_asset_load_font( &state->assets,
                    "data/LiberationSerif-Regular.ttf", 24 );
    }

    {
        // just to be easier they will still plain bitmaps
        // instead of actual cursor objects
        state->ui_info.cursor.cursor_textures[ CURSOR_MOVEMENT ] =
            game_asset_load_bitmap( &state->assets,
                    "data/wanderer_cursor_nrm.bmp" );
        state->ui_info.cursor.cursor_textures[ CURSOR_DIALOGUE ] =
            game_asset_load_bitmap( &state->assets,
                    "data/wanderer_cursor_talk.bmp" );
        state->ui_info.cursor.cursor_textures[ CURSOR_ACTIVATE ] =
            game_asset_load_bitmap( &state->assets,
                    "data/wanderer_cursor_interact.bmp" );
        state->ui_info.cursor.cursor_textures[ CURSOR_ATTACK ] =
            game_asset_load_bitmap( &state->assets,
                    "data/wanderer_cursor_combat.bmp" );
    }

    state->dawnblocker_floor = game_asset_load_bitmap( &state->assets, "data/dawnblocker_gray_floor.png" );
    state->dawnblocker_block = game_asset_load_bitmap( &state->assets, "data/dawnblocker_gray_wall.png" );
    state->main_menu_asset_a = game_asset_load_bitmap( &state->assets, "data/wanderer_menu_mask00.png" );
    state->main_menu_asset_b = game_asset_load_bitmap( &state->assets, "data/wanderer_menu_table00.png" );
    state->main_menu_asset_c = game_asset_load_bitmap( &state->assets, "data/wanderer_menu_decor_table_00.png" );
    state->isometric_tile_a  = game_asset_load_bitmap( &state->assets, "data/dawnblocker_gray_floor.png" );
    state->isometric_tile_b  = game_asset_load_bitmap( &state->assets, "data/test_topdown01.bmp" );
    state->iso_gore_pile     = game_asset_load_bitmap( &state->assets, "data/iso_gore_pile.png" );

    state->oga_iso_skeleton_test = game_asset_load_spritesheet( &state->assets, "data/test_goblin_atlas.txt" );
    // these have to be aligned properly... Or I have some other insideous issue I haven't noticed.... Hmmm.
    state->ui_icon_atlas = game_asset_load_spritesheet( &state->assets, "data/wanderer_ui_icons.txt" );
    state->ui_atlas = game_asset_load_spritesheet( &state->assets, "data/wanderer_ui_atlas.txt" );
    {
        state->models[ACTOR_MODEL_HUMAN] = game_asset_load_actor_model( &state->assets, "data/test_orc.mdl" );
        state->models[ACTOR_MODEL_ORC] = game_asset_load_actor_model( &state->assets, "data/test_orc.mdl" );
        state->models[ACTOR_MODEL_ORC_SOLDIER] = game_asset_load_actor_model( &state->assets, "data/test_human.mdl" );
    }

    game_ui_initialize( state );

#ifdef DEBUG_BUILD
    {
        state->model_editor.lookat_target_direction_x = 1;
        state->model_editor.lookat_target_direction_y = 0;
    }
#endif  
}

#include "wanderer_demo_world.c"

// Yeah this has got to go.
static void game_process_event_stack( game_event_stack* stack, game_state* state ){
    while( game_event_stack_get_end( stack ) ){
        game_event current_event = game_event_stack_pop( stack );
        /*
         * I wish there were reflection facilities in the
         * language.
         */
        switch( current_event.event_type ){
            case GAME_EVENT_SET_VARIABLE:
            {
                char* var_name = current_event.set_variable.variable_name;
                i32 var_value = current_event.set_variable.new_value;

                fprintf(stderr, "set var \'%s\'\n", var_name);

                game_variable* var =
                    game_variable_dictionary_find( &state->variables, var_name );

                if( !var ){
                    game_variable_dictionary_add_variable_default_value( &state->variables, 
                            var_name, var_value );
                }else{
                    var->value = var_value;
                }
            }
            break;
            case GAME_EVENT_CRITICAL_PLOT_POINT_REACHED:
            {
                actor* player = game_get_player_actor( state );

                f32 life_time = 1.5f;

                floating_messages_on_actor( 
                        &state->floating_messages,
                        player,
                        (colorf) {1, 1, 0, 1},
                        "Updating my journal" );

                char* test_entry_title = "Midnight Howling?";
                char* test_entry_text = "According to one of the residents of this town, there have been reports of howling at night.\n\nWolves perhaps? Although why that would trouble a small town is beyond me, these citizens look fairly well armed for a pack of wolves to be threatening.";
                game_journal_add_entry( &state->journal, test_entry_title, test_entry_text );
            }
            break;
            case GAME_EVENT_SPAWN_FLOATING_MESSAGE:
            {
#if 0
                char* message = current_event.floating_message.message_contents;
                f32 start_x = current_event.floating_message.start_x;
                f32 start_y = current_event.floating_message.start_y;
                f32 life_time = current_event.floating_message.life_time;

                floating_messages_push_message( &state->floating_messages,
                                                life_time,
                                                start_x,
                                                start_y,
                                                message );
#endif
            }
            break;
#if 0
            case GAME_EVENT_PRINT_TEST:
            {
                char* what = current_event.print_test.what;

                fprintf( stderr, "GAME_EVENT_PRINT_TEST: %s\n", what );
            }
            break;
#endif
        }
    }
}

static void render_layer_push_floating_message_cmds( render_layer* layer, floating_messages* messages, game_state* state ){
    for( u64 msg_index = 0; msg_index < MAX_FLOATING_MESSAGES; ++msg_index ){
            floating_message* current_message = &messages->messages[msg_index]; 

            b32 message_alive = current_message->lifetime > 0.0f;
            if (message_alive) {
                render_command cmd = {};

                // The game floating messages are in the game units.
                vec2 draw_coords = current_message->position;
                draw_coords = project_cartesian_to_isometric( draw_coords );

                draw_coords.x *= state->camera.scale;
                draw_coords.y *= state->camera.scale;

                const f32 half_draw_w = (32 * 0.5);

                draw_coords.x *= half_draw_w;
                draw_coords.y *= half_draw_w;

                /* draw_coords.y -= (60/2); */

                f32 alpha = current_message->lifetime / current_message->max_lifetime;
                cmd = render_command_text(current_message->message, 
                                          state->font_texture, 
                                          draw_coords.x,
                                          draw_coords.y + current_message->height,
                                          1.0f, 
                                          current_message->color.r,
                                          current_message->color.g,
                                          current_message->color.b,
                                          alpha);

                render_layer_push_command( layer, cmd );
            }
    }
}

actor* game_get_player_actor( game_state* state ){
    return actors_list_get_actor( &state->actors, state->player_index );
}

// NOTE(jerry): The config stuff is always the roughest crap.
// should've come up with a cleaner api

// to be fair parsing in any language except for lisp is a fucking nightmare,
/*
 * It was either this or use a json library and just do shit like

 dialogue_file["start_node"].exists;
 and just about as many if statements.

 There's just no way to win that...
 
 Everyone hates it. That's why we love binary. We can just fread it or serialize it
 trivially.
 
 The most practical solution is to metaprogram all of this based on some reflective state.
 I had the foresight to NOT do that because I didn't think I needed it. Going to rethink that
 in the future.
 */
static bool game_begin_dialogue_with_actor( game_state* state, actor* speaking_with ) {
    /*
     * check if I should try to begin dialogue,
     * I should be returning more context for display systems.
     */
    if (!speaking_with->has_dialogue || state->active_dialogue) {
        return false;
    }

    fprintf(stderr, "loading: %s\n", speaking_with->dialogue_file_reference);
    // first read in all dialogue_nodes
    // so I can get the names and match them.

    // then read the selectors and pair them up with the nodes
    // then read the choices.
    typedef struct associative_element {
        char* key;
        u32 value;
    }associative_element;

    // dumb look up for now.
    u32 lookup_count = 0;
    associative_element lookup[4096] = {};

    state->active_dialogue = memory_pool_allocate(&state->dialogue_memory,
                                                  sizeof(dialogue_info));
    state->active_dialogue->node_count = 0;

    // Dialogue config... Read at runtime, should probably
    // use binary format in release.
    {
        config_info dialogue_file = config_parse_from_file(speaking_with->dialogue_file_reference);
        u32 dialogue_node_count = config_info_count_variables_of_type(&dialogue_file, CONFIG_VARIABLE_BLOCK);

        // I don't have a freelist cause I didn't implement one,
        // A free list would have been very useful, since it would have made less redundant code...
        state->active_dialogue->nodes = memory_pool_allocate(&state->dialogue_memory,
                                                             sizeof(dialogue_node) * dialogue_node_count);

        config_variable* start_node_name =
            config_info_find_first_variable(&dialogue_file, "start_node");

        // build lookup table for referencing nodes.
        for( unsigned block_index = 0;
             block_index < dialogue_file.count;
             ++block_index ){
            u32 fake_node_counter = 0;
            config_variable* current_block =
                &dialogue_file.variables[block_index];
            lookup[lookup_count].key = current_block->name;
            lookup[lookup_count].value = block_index;
            lookup_count++;
        }

        for( unsigned block_index = 0;
             block_index < dialogue_file.count;
             ++block_index ){
            config_variable* current_block =
                &dialogue_file.variables[block_index];

            if( current_block->value.type == CONFIG_VARIABLE_BLOCK ){
                fprintf(stderr, "block name : %s\n", current_block->name);
                config_variable_block* current_block_content = current_block->value.as_block;

                config_variable* type =
                    config_block_find_first_variable(current_block_content, "type");

                if( !type ){
                    fprintf(stderr, "There is an error. Block has no type!");
                }

                const char* type_string = config_variable_try_string(type);

                if( string_compare(type_string, "DIALOGUE_NODE") == 0 ){
                    dialogue_node* current_node = dialogue_info_push_node(state->active_dialogue);
                    fprintf(stderr, "Found node!\n");
                    current_node->dialogue.speaker = speaking_with;

                    config_variable* target_speaker =
                        config_block_find_first_variable(current_block_content, "target_speaker");
                    config_variable* text_content =
                        config_block_find_first_variable(current_block_content, "text");
                    {
                        if( target_speaker ){
                            if( target_speaker->value.type == CONFIG_VARIABLE_SYMBOL ){
                                const char* target_speaker_symbol = config_variable_try_string(target_speaker);
                                if( string_compare(target_speaker_symbol, "ACTOR_ORIGINAL_SPEAKER") ){
                                    stub_less_important("ACTOR_ORIGINAL_SPEAKER IS NOT HANDLED!");
                                }else if( string_compare(target_speaker_symbol, "ACTOR_PLAYER") ){
                                    current_node->dialogue.speaker = game_get_player_actor(state);
                                }
                            }else if( target_speaker->value.type == CONFIG_VARIABLE_STRING ){
                                const char* target_speaker_string = config_variable_try_string(target_speaker);
                                // not handled yet.
                                stub_less_important("Actor lookup for speaker not handled.");
                            }
                        }else{
                            // eh.
                            fprintf(stderr, "Cannot find target speaker\n");
                        }

                        if( text_content ){
                            dialogue_node_set_text(current_node, config_variable_try_string(text_content));
                        }else{
                            fprintf(stderr, "no text content found\n");
                            // eh
                        }
                    }

                    u32 dialogue_choice_count = config_block_count_variables_of_type(current_block_content,
                                                                                     CONFIG_VARIABLE_BLOCK);
                    current_node->dialogue.choices = memory_pool_allocate(&state->dialogue_memory,
                                                                          sizeof(dialogue_choice) * dialogue_choice_count);

                    for( unsigned inner_block_index = 0;
                         inner_block_index < current_block_content->count;
                         ++inner_block_index ){
                        if( current_block_content->items[inner_block_index].value.type == CONFIG_VARIABLE_BLOCK ){
                            config_variable* choice_block_var = &current_block_content->items[inner_block_index];
                            config_variable_block* choice_block = choice_block_var->value.as_block;
                            {
                                dialogue_choice* current_choice;
                                if( string_compare(choice_block_var->name, "goodbye_choice") == 0 ){
                                    current_choice = dialogue_node_push_goodbye_choice(current_node);
                                }else if( string_compare(choice_block_var->name, "trade_menu_choice") == 0 ){
                                    current_choice = dialogue_node_push_trade_menu_choice(current_node);
                                }else{
                                    current_choice = dialogue_node_push_choice(current_node);
                                    config_variable* jump_to = config_block_find_first_variable(choice_block, "jump");

                                    if( jump_to ){
                                        for( unsigned lookup_index = 0;
                                             lookup_index < lookup_count;
                                             ++lookup_index ){
                                            if( string_compare(lookup[lookup_index].key, config_variable_try_string(jump_to)) == 0 ){
                                                current_choice->jump_to = lookup[lookup_index].value;
                                                break;
                                            }
                                        }
                                    }else{
                                        // error
                                    }
                                }
                                config_variable* choice_text = config_block_find_first_variable(choice_block, "text");
                                config_variable* event_list = config_block_find_first_variable(choice_block, "events");
                                if( choice_text ){
                                    dialogue_choice_set_text(current_choice, config_variable_try_string(choice_text));
                                }else{
                                    // error
                                    fprintf(stderr, "could not find text for choice?");
                                }

                                if( event_list ){
                                    fprintf(stderr, "found events for dialogue\n");
                                    if( event_list->value.type == CONFIG_VARIABLE_LIST ){
                                        fprintf(stderr, "events are in list. good\n");
                                        u32 event_count = event_list->value.as_list->count;
                                        current_choice->events = memory_pool_allocate(&state->dialogue_memory,
                                                                                      sizeof(dialogue_event) * event_count);
                                        for( unsigned list_item = 0;
                                             list_item < event_count;
                                             ++list_item ){
                                            config_variable_value* sub_list = &event_list->value.as_list->items[list_item];
                                            dialogue_event* current_event;

                                            if( sub_list->type == CONFIG_VARIABLE_LIST ){
                                                config_variable_value* type_of_event = &sub_list->as_list->items[0];
                                                if( type_of_event->type == CONFIG_VARIABLE_SYMBOL ){
                                                    if( string_compare(type_of_event->value, "SET_VARIABLE") == 0 ){
                                                        config_variable_value* game_variable_name = &sub_list->as_list->items[1];
                                                        config_variable_value* set_value = &sub_list->as_list->items[2];
                                                        config_variable_value* max_use_times = &sub_list->as_list->items[3];

                                                        current_event = dialogue_choice_push_set_variable_event(current_choice,
                                                                                                                config_variable_value_try_string(game_variable_name),
                                                                                                                config_variable_value_try_integer(set_value),
                                                                                                                config_variable_value_try_integer(max_use_times));
                                                    }else if( string_compare(type_of_event->value, "CRITICAL_PLOT_POINT") == 0 ){
                                                        current_event = dialogue_choice_push_critical_plot_point(current_choice);
                                                    }
                                                }else{
                                                    // error
                                                }
                                            }else{
                                                // error
                                            }
                                        }
                                    }else{
                                        // error
                                    }
                                }else{
                                    fprintf(stderr, "no events found for dialogue\n");
                                }
                            }
                        }
                    }
                }else if( string_compare(type_string, "BRANCHING_NODE") == 0 ){
                    fprintf(stderr, "Found branching node!\n");
                    dialogue_node* current_selector = dialogue_info_push_selector_node(state->active_dialogue);
                    u32 selector_condition_count = config_block_count_variables_of_type(current_block_content, CONFIG_VARIABLE_BLOCK);
                    current_selector->selector.conditions = memory_pool_allocate(&state->dialogue_memory,
                                                                                 sizeof(dialogue_selection_condition) * selector_condition_count);
                    for( unsigned selector_condition_block = 0;
                         selector_condition_block < current_block_content->count;
                         ++selector_condition_block ){
                        if( current_block_content->items[selector_condition_block].value.type == CONFIG_VARIABLE_BLOCK ){
                            config_variable_block* condition_block = current_block_content->items[selector_condition_block].value.as_block;
                            dialogue_selection_condition* current_condition = dialogue_selector_node_push_condition(current_selector);

                            config_variable* variable_name =
                                config_block_find_first_variable(condition_block, "variable");
                            config_variable* compare_type =
                                config_block_find_first_variable(condition_block, "compare_type");
                            config_variable* against =
                                config_block_find_first_variable(condition_block, "against");
                            config_variable* then_branch =
                                config_block_find_first_variable(condition_block, "then");
                            config_variable* else_branch =
                                config_block_find_first_variable(condition_block, "else");

                            if( compare_type &&
                                against &&
                                variable_name ){
                                u16 condition_flags = 0;

                                if( compare_type->value.type == CONFIG_VARIABLE_LIST ){
                                    for( unsigned list_item = 0;
                                         list_item < compare_type->value.as_list->count;
                                         ++list_item ){
                                        const char* compare_type_string = compare_type->value.as_list->items[list_item].value;

                                        if( string_compare(compare_type_string, "NOT_EQUAL_TO") == 0 ){
                                            condition_flags |= NOT_EQUAL_TO;
                                        } else if( string_compare(compare_type_string, "LESS_THAN") == 0 ){
                                            condition_flags |= LESS_THAN;
                                        } else if( string_compare(compare_type_string, "GREATER_THAN") == 0 ){
                                            condition_flags |= GREATER_THAN;
                                        } else if( string_compare(compare_type_string, "EQUAL_TO") == 0 ){
                                            condition_flags |= EQUAL_TO;
                                        }
                                    }
                                }else{
                                    const char* compare_type_string = config_variable_try_string(compare_type);

                                    if( string_compare(compare_type_string, "NOT_EQUAL_TO") == 0 ){
                                        condition_flags |= NOT_EQUAL_TO;
                                    } else if( string_compare(compare_type_string, "LESS_THAN") == 0 ){
                                        condition_flags |= LESS_THAN;
                                    } else if( string_compare(compare_type_string, "GREATER_THAN") == 0 ){
                                        condition_flags |= GREATER_THAN;
                                    } else if( string_compare(compare_type_string, "EQUAL_TO") == 0 ){
                                        condition_flags |= EQUAL_TO;
                                    }
                                }

                                dialogue_selection_condition_set_check_variable(current_condition,
                                                                                config_variable_try_string(variable_name),
                                                                                config_variable_try_integer(against),
                                                                                condition_flags);
                            }else{
                                // error.
                            }

                            if( then_branch ){
                                const char* then_branch_node_name = config_variable_try_string(then_branch);
                                for( unsigned lookup_index = 0;
                                     lookup_index < lookup_count;
                                     ++lookup_index ){
                                    if( string_compare(lookup[lookup_index].key, then_branch_node_name) == 0 ){
                                        dialogue_selection_condition_set_success_jump(current_condition, lookup[lookup_index].value);
                                        break;
                                    }
                                }
                            }else{
                                // error.
                            }

                            if( else_branch ){
                                const char* else_branch_node_name = config_variable_try_string(else_branch);
                                for( unsigned lookup_index = 0;
                                     lookup_index < lookup_count;
                                     ++lookup_index ){
                                    if( string_compare(lookup[lookup_index].key, else_branch_node_name) == 0 ){
                                        dialogue_selection_condition_set_failure_jump(current_condition, lookup[lookup_index].value);
                                        break;
                                    }
                                }
                            }else{
                                // ok.
                            }
                        }
                    }
                }
                    
            }else{
                // probably an error for dialogue files.
            }
        }

        if( start_node_name ){
            for( unsigned lookup_index = 0;
                 lookup_index < lookup_count;
                 ++lookup_index ){
                if( string_compare(lookup[lookup_index].key, start_node_name->value.value) == 0 ){
                    state->active_dialogue->current_node = lookup[lookup_index].value;
                    fprintf(stderr, "found start node to be : %d\n", state->active_dialogue->current_node);
                    break;
                }
            }
        }else{
            state->active_dialogue->current_node = 0;
        }

        config_info_free_all_fields(&dialogue_file);
    }

    state->active_dialogue->initiated = true;
    state->ui_info.mode = GAME_UI_STATE_DIALOGUE;
    return true;
}

static game_moused_over_selection game_mouse_pick( vec2 mouse_coordinates, game_state* state ){
    game_moused_over_selection selection = {};

    rectangle cursor_rect =
    { mouse_coordinates.x * tile_px_size, mouse_coordinates.y * tile_px_size, 1, 1 };

    // find actor.
    actors_list* list = &state->actors;
    actor* player = game_get_player_actor( state );
    assert( list && player );
    {
        for( u64 actor_index = 0; actor_index < list->count; ++actor_index ){
            actor* current_actor = actors_list_get_actor( list, actor_index );
            if( current_actor->current_map == player->current_map ){
                rectangle actor_rect =
                {
                    current_actor->position.x * tile_px_size - ((current_actor->size.w/2) * tile_px_size),
                    current_actor->position.y * tile_px_size - ((current_actor->size.h/2) * tile_px_size),
                    current_actor->size.w * tile_px_size,
                    current_actor->size.h * tile_px_size
                };

                if( rectangle_intersects( actor_rect, cursor_rect ) ){
                    selection.type = MOUSED_OVER_ACTOR;
                    selection.selected_index = actor_index;

                    return selection;
                }
            }
        }
    }
    // find item
    {
        tilemap* current_tilemap = player->current_map;
        assert(current_tilemap);
        for( u64 pickup_index = 0;
                 pickup_index < current_tilemap->pickup_count;
                 ++pickup_index ){
            map_item_pickup* current_pickup = &current_tilemap->pickups[pickup_index];
            rectangle pickup_rect =
            {
                current_pickup->x * tile_px_size,
                current_pickup->y * tile_px_size,
                current_pickup->w * tile_px_size,
                current_pickup->h * tile_px_size
            };

            if( rectangle_intersects( cursor_rect, pickup_rect ) && current_pickup->contains_item ){
                selection.type = MOUSED_OVER_PICKUP_ITEM;
                selection.selected_index = pickup_index;

                return selection;
            }
        }
    }
    // find container
    {
        tilemap* current_tilemap = player->current_map;
        assert(current_tilemap);
        for( u64 container_index = 0;
                 container_index < current_tilemap->container_count;
                 ++container_index ){
            container* current_container = &current_tilemap->containers[container_index];
            rectangle container_rect =
            {
                current_container->position.x * tile_px_size,
                current_container->position.y * tile_px_size,
                current_container->size.w * tile_px_size,
                current_container->size.h * tile_px_size
            };

            if( rectangle_intersects( cursor_rect, container_rect ) ){
                selection.type = MOUSED_OVER_CONTAINER;
                selection.selected_index = container_index;

                return selection;
            }
        }
    }

    return selection;
}

// Game Variable Dictionary
void game_variable_dictionary_add_variable( game_variable_dictionary* dictionary, 
                                                   char* name ){
    assert( dictionary != NULL );
    assert( name != NULL );
    assert( dictionary->variable_count + 1 < MAX_GAME_VARIABLES );

    game_variable_dictionary_add_variable_default_value( dictionary, name, -1 );
}

void game_variable_dictionary_add_variable_default_value( game_variable_dictionary* dictionary, 
                                                          char* name, 
                                                          i32 start ){
    assert( dictionary != NULL );
    assert( name != NULL );

    game_variable* var = &dictionary->variables[ dictionary->variable_count++ ];

    strncpy( var->name, name, 255 );
    var->value = start;
}

game_variable* game_variable_dictionary_find( game_variable_dictionary* dictionary, 
                                              char* variable_name ){
    assert( dictionary != NULL );
    assert( variable_name != NULL );

    for( u64 var_index = 0; var_index < dictionary->variable_count; ++var_index ){
        game_variable* candidate = &dictionary->variables[ var_index ];

        if( strcmp( candidate->name, variable_name ) == 0 ){
            return candidate;
        }
    }

    return NULL;
}

void game_finish( game_state* state ){
    item_dictionary_finish(&state->items);
    spell_dictionary_finish(&state->spells);
    memory_deallocate(state->items.items);
    game_ui_finish(state);
    strings_finish();
    game_assets_finish( &state->assets );
}

// combat round
static void game_combat_round_push_actor_id(game_combat_round* round, u32 actor_index) {
    round->participant_count++;
    round->participants[round->participant_count - 1] = (combat_participant){ .id = actor_index };
}

static game_combat_round game_build_combat_round(game_state* state) {
    stub_less_important("begin new turn-based round");
    game_combat_round new_round = {};

    for (u64 actor_index = 0; actor_index < state->actors.count; ++actor_index) {
        actor* current_actor = 
            actors_list_get_actor(&state->actors, actor_index);

        current_actor->finished_turn = false;
        current_actor->finished_first_turn = false;

        if (!actor_is_dead(current_actor)) {
            current_actor->action_points = current_actor->max_action_points;
            game_combat_round_push_actor_id(&new_round, actor_index);
        }
    }

    return new_round;
}

static void game_finish_combat_round(game_state* state, game_combat_round* round) {
    // Send a message to everyone.
    // Most notably XP rewards and other things.
    // In this case we don't do anything..
    stub_less_important("turnbased combat finished!");
}

// technically unsafe? But since this is only called during combat rounds we can guarantee
// at least two people are in a round (combat is determined by attacker-victim relationship)
combat_participant game_combat_round_get_current_participant(game_combat_round* combat_round) {
    return combat_round->participants[combat_round->current_participant];
}

actor* combat_participant_lookup_actor(actors_list* actors, combat_participant participant) {
    actor* result = actors_list_get_actor(actors, participant.id);

    return result;
}

/*
  TODO(jerry): 
  With spell effects which may be applied per turn. If someone dies, what's
  going to happen?
 */
static b32 combat_round_advance_to_next_participant(game_combat_round* round) {
    round->current_participant++;

    if (round->current_participant >= round->participant_count) {
        return false;
    } else {
        return true;
    }
}

#include "wanderer_assets.c"
