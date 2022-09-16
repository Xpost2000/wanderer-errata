/*
  TODO(jerry):
  Try to spoof less of the widgets that I have right now...
  
  For example the ui states shouldn't all be spoofed placeholders.
  There should be some legitimate widgets for them.
 */
// for the other defs.
#include "wanderer.h"
#include "wanderer_game_ui.h"

enum ingame_ui_bar_button_constants {
    DIALOGUE_OVERRIDE_BUTTON = 21,
    ATTACK_OVERRIDE_BUTTON = 22,
    USE_SKILL_OVERRIDE_BUTTON = 23,
    OPEN_JOURNAL_BUTTON = 24,
    OPEN_INVENTORY_BUTTON = 25,
    OPEN_CHARACTER_RECORD = 26,
    REST_BUTTON = 27
};

enum menu_ui_button_constants {
    MAIN_MENU_NEW_GAME_BUTTON = 28,
    MAIN_MENU_LOAD_GAME_BUTTON = 29,
    MAIN_MENU_QUIT_GAME_BUTTON = 30
};


static void game_ui_screen_update_render(game_state* state, game_ui_screen* screen, renderer* renderer, game_input* input, f32 delta_time);
static game_ui_widget* game_ui_widget_push_child( game_ui_widget* parent, game_ui_widget target );
static game_ui_widget game_widget_window(u8 skin, vec2 position, vec2 size);
static game_ui_widget game_widget_text(char* text, game_asset_handle font, vec2 position, colorf color);
static game_ui_widget game_widget_button(u32 atlas_index, vec2 position, vec2 size, game_ui_widget_button_callback callback);
static game_ui_widget game_widget_rectangle(vec2 position, vec2 size, colorf color);
static game_ui_widget game_widget_dialogue_placeholder(vec2 position);
static game_ui_widget game_widget_image(u32 atlas_index, vec2 position, vec2 size, colorf color);
static game_ui_widget game_widget_journal_placeholder(vec2 position);
static game_ui_widget game_widget_player_inventory(vec2 position);
static game_ui_widget game_widget_loot_inventory(vec2 position);

static void game_ui_open_loot_menu(game_state* state) {
    if (state->ui_info.mode != GAME_UI_STATE_INVENTORY_LOOTING) {
        state->ui_info.mode = GAME_UI_STATE_INVENTORY_LOOTING;
    } else {
        state->ui_info.mode = GAME_UI_STATE_INGAME;
    }
}

void game_ui_open_loot_container(game_state* state, u32 container_index) {
    game_ui_open_loot_menu(state);

    state->ui_info.loot_inventory.target_type = INVENTORY_LOOTING_TARGET_CONTAINER;
    state->ui_info.loot_inventory.target_index = container_index;
}

void game_ui_open_loot_actor(game_state* state, u32 actor_index) {
    game_ui_open_loot_menu(state);

    state->ui_info.loot_inventory.target_type = INVENTORY_LOOTING_TARGET_ACTOR;
    state->ui_info.loot_inventory.target_index = actor_index;
}

void game_ui_toggle_player_inventory(game_state* state) {
    if (state->ui_info.mode != GAME_UI_STATE_INVENTORY_PLAYER) {
        state->ui_info.mode = GAME_UI_STATE_INVENTORY_PLAYER;
    } else {
        state->ui_info.mode = GAME_UI_STATE_INGAME;
    }
}

void game_ui_toggle_journal( game_state* state ){
    if( state->ui_info.journal.opened == 1 ){
        state->ui_info.mode = GAME_UI_STATE_INGAME;
        state->ui_info.journal.opened = 0;
        game_action_bindings_set_current_context(
            &state->bindings, GAME_INPUT_MODE_GAMEPLAY );
    }else{
        state->ui_info.mode = GAME_UI_STATE_JOURNAL; 
        state->ui_info.journal.opened = 1;
        game_action_bindings_set_current_context(
            &state->bindings, GAME_INPUT_MODE_IN_JOURNAL );
    }
}

void game_ui_toggle_character_sheet( game_state* state ){
    if( state->ui_info.character_sheet.opened == 1 ){
        state->ui_info.mode = GAME_UI_STATE_INGAME;
        state->ui_info.character_sheet.opened = 0;
        game_action_bindings_set_current_context(
            &state->bindings, GAME_INPUT_MODE_GAMEPLAY );
    }else{
        state->ui_info.mode = GAME_UI_STATE_JOURNAL; 
        state->ui_info.character_sheet.opened = 1;
        game_action_bindings_set_current_context(
            &state->bindings, GAME_INPUT_MODE_IN_CHARACTERSHEET );
    }
}

void game_ui_disable(game_state* state) {
    state->ui_info.mode = GAME_UI_STATE_NONE;
}

void game_ui_toggle_pause( game_state* state ){
    if( state->ui_info.mode != GAME_UI_STATE_PAUSE_MENU ){
        state->ui_info.mode = GAME_UI_STATE_PAUSE_MENU;
        state->ui_info.journal.opened = 0;
        state->ui_info.character_sheet.opened = 0;
        state->paused = true;
    }else{
        state->ui_info.mode = GAME_UI_STATE_INGAME;
        state->paused = false;
        game_action_bindings_set_current_context(
                &state->bindings, GAME_INPUT_MODE_GAMEPLAY );
    }
}

// These are used to offset into the ui_atlas atlas.
enum scalable_ui_offsets{
    SCALABLE_UI_TOP_LEFT_OFFSET,
    SCALABLE_UI_TOP_MIDDLE_OFFSET,
    SCALABLE_UI_TOP_RIGHT_OFFSET,

    SCALABLE_UI_LEFT_OFFSET,
    SCALABLE_UI_MIDDLE_OFFSET,
    SCALABLE_UI_RIGHT_OFFSET,

    SCALABLE_UI_BOTTOM_LEFT_OFFSET,
    SCALABLE_UI_BOTTOM_MIDDLE_OFFSET,
    SCALABLE_UI_BOTTOM_RIGHT_OFFSET,

    SCALABLE_UI_OFFSET_COUNT
};

enum scalable_ui_skin_offsets{
    SCALABLE_DEFAULT_UI_SKIN = 0,
    SCALABLE_PAPER_UI_SKIN = 9,
    SCALABLE_UI_SKIN_COUNT = 2
};

const static b32 TESTING_NEW_UI = true;

// temporary until moved into real renderer.
static render_command render_command_spritesheet_quad( f32 x, f32 y,
                                                       f32 w, f32 h,
                                                       f32 r, f32 g, f32 b, f32 a,
                                                       game_asset_handle spritesheet,
                                                       u32 sprite_index,
                                                       game_assets* assets ){
    game_asset* spritesheet_asset = game_asset_get_from_handle( assets, spritesheet );

    game_asset_spritesheet_rect sprite_rect = 
        spritesheet_asset->spritesheet.sprite_rects[sprite_index];

    render_command drawn_frame = {};
    drawn_frame = render_command_textured_quad( x, y, w, h,
                                                r, g, b, a,
                                                spritesheet_asset->spritesheet.matching_bitmap );

    game_asset* spritesheet_bitmap_asset = game_asset_get_from_handle( assets, spritesheet_asset->spritesheet.matching_bitmap );

    f32 spritesheet_width = spritesheet_bitmap_asset->bitmap.width;
    f32 spritesheet_height = spritesheet_bitmap_asset->bitmap.height;
    drawn_frame.info.textured_quad.uv_x = (f32)sprite_rect.x / spritesheet_width;
    drawn_frame.info.textured_quad.uv_y = (f32)sprite_rect.y / spritesheet_height;
    drawn_frame.info.textured_quad.uv_w = (f32)sprite_rect.w / spritesheet_width;
    drawn_frame.info.textured_quad.uv_h = (f32)sprite_rect.h / spritesheet_height;

    return drawn_frame;
}

static vec2 ui_scale_to_virtual_screenspace( game_state* state, vec2 orig ){
    vec2 result = orig;

    result.x /= (f32)state->ui_info.virtual_width;
    result.y /= (f32)state->ui_info.virtual_height;

    result.x *= state->screen_width;
    result.y *= state->screen_height;

    return result;
}

// this is just pretty I guess. It's not a real window that does anything
const f32 ui_scale = 2;
// ugh...
static void game_ui_draw_ui_window_with_skin( game_state* state,
                                              u8 skin,
                                              vec2 position,
                                              vec2 size,
                                              colorf color ){
    game_ui_state* ui_state = &state->ui_info;
    render_layer* layer = &state->ui_layer;

    u32 base_offset = skin;
    f32 corner_size = 32 * ui_scale;

    /* position = ui_scale_to_virtual_screenspace( state, position ); */
    /* size = ui_scale_to_virtual_screenspace( state, size ); */

    render_command top_left = render_command_spritesheet_quad( position.x, position.y,
                                                               corner_size, corner_size,
                                                               color.r, color.g, color.b, color.a,
                                                               state->ui_atlas,
                                                               base_offset + SCALABLE_UI_TOP_LEFT_OFFSET,
                                                               &state->assets );

    render_command top_right = render_command_spritesheet_quad( position.x + size.x, position.y,
                                                                corner_size, corner_size,
                                                                color.r, color.g, color.b, color.a,
                                                                state->ui_atlas,
                                                                base_offset + SCALABLE_UI_TOP_RIGHT_OFFSET,
                                                                &state->assets );

    render_command bottom_left = render_command_spritesheet_quad( position.x, position.y + size.y,
                                                                  corner_size, corner_size,
                                                                  color.r, color.g, color.b, color.a,
                                                                  state->ui_atlas,
                                                                  base_offset + SCALABLE_UI_BOTTOM_LEFT_OFFSET,
                                                                  &state->assets );

    render_command bottom_right = render_command_spritesheet_quad( position.x + size.x, position.y + size.y,
                                                                   corner_size, corner_size,
                                                                   color.r, color.g, color.b, color.a,
                                                                   state->ui_atlas,
                                                                   base_offset + SCALABLE_UI_BOTTOM_RIGHT_OFFSET,
                                                                   &state->assets );

    {
        render_command top_middle = render_command_spritesheet_quad( position.x + (corner_size), position.y,
                                                                     size.x, corner_size,
                                                                     color.r, color.g, color.b, color.a,
                                                                     state->ui_atlas,
                                                                     base_offset + SCALABLE_UI_TOP_MIDDLE_OFFSET,
                                                                     &state->assets );

        render_layer_push_command( layer, top_middle );
    }

    {
        render_command right_border = render_command_spritesheet_quad( position.x + size.x, position.y + (corner_size),
                                                                       corner_size, size.y,
                                                                       color.r, color.g, color.b, color.a,
                                                                       state->ui_atlas,
                                                                       base_offset + SCALABLE_UI_RIGHT_OFFSET,
                                                                       &state->assets );

        render_layer_push_command( layer, right_border );
    }

    {
        render_command left_border = render_command_spritesheet_quad( position.x, position.y + (corner_size),
                                                                      corner_size, size.y,
                                                                      color.r, color.g, color.b, color.a,
                                                                      state->ui_atlas,
                                                                      base_offset + SCALABLE_UI_LEFT_OFFSET,
                                                                      &state->assets );

        render_layer_push_command( layer, left_border );
    }

    {
        render_command bottom_middle = render_command_spritesheet_quad( position.x + (corner_size), position.y + size.y,
                                                                        size.x, corner_size,
                                                                        color.r, color.g, color.b, color.a,
                                                                        state->ui_atlas,
                                                                        base_offset + SCALABLE_UI_BOTTOM_MIDDLE_OFFSET,
                                                                        &state->assets );

        render_layer_push_command( layer, bottom_middle );
    }

    {
        render_command middle = render_command_spritesheet_quad( position.x + (corner_size), position.y + (corner_size),
                                                                 size.x - corner_size, size.y - corner_size,
                                                                 color.r, color.g, color.b, color.a,
                                                                 state->ui_atlas,
                                                                 base_offset + SCALABLE_UI_MIDDLE_OFFSET,
                                                                 &state->assets );

        render_layer_push_command( layer, middle );
    }

    render_layer_push_command( layer, top_left );
    render_layer_push_command( layer, top_right );
    render_layer_push_command( layer, bottom_left );
    render_layer_push_command( layer, bottom_right );
}

void game_ui_update_render( game_state* state, 
                            renderer* renderer, 
                            game_input* input,
                            f32 delta_time ){
    state->ui_info.touching_ui = false;
    game_ui_screen* current_screen = &state->ui_info.ui_screens[state->ui_info.mode];
    game_ui_screen_update_render( state,
                                  current_screen,
                                  renderer,
                                  input,
                                  delta_time );
    /* fprintf(stderr, "%d\n", state->ui_info.touching_ui); */
}

void game_ui_finish( game_state* state ){
    // free.
    memory_pool_finish( &state->ui_info.ui_memory );
}

vec2 game_ui_widget_calculate_position_relative_to_parent( game_ui_widget* child ){
    if( child->parent == NULL ){
        return child->position;
    }else{
        vec2 parent_position = game_ui_widget_calculate_position_relative_to_parent(child->parent);

        return v2(child->position.x + parent_position.x,
                  child->position.y + parent_position.y);
    }
}

static void draw_item_tooltip(game_state* state,
                              game_input* input,
                              renderer* renderer,
                              item_slot* slot) {
    char* tooltip_text = memory_pool_allocate(&state->strings_scratch, 1024);

    snprintf(tooltip_text, 1024, "%s\n\n%s", slot->item_ptr->name, slot->item_ptr->description);

    vec2 tooltip_position = v2(input_get_mouse_x(input), input_get_mouse_y(input));

    f32 text_width;
    f32 text_height;
    f32 text_size;

    renderer_get_wrapped_text_size(renderer,
                                   state->gothic_font,
                                   tooltip_text,
                                   400, /*this doesn't matter but okay*/300,
                                   &text_width,
                                   &text_height,
                                   &text_size);

    render_command tooltip_text_command = 
        render_command_text_wrapped(tooltip_text,
                                    state->gothic_font,
                                    1, 
                                    0, 0, 0, 1,
                                    tooltip_position.x + 15, tooltip_position.y + 15, 
                                    text_width, text_height);
    game_ui_draw_ui_window_with_skin(state,
                                     SCALABLE_PAPER_UI_SKIN,
                                     tooltip_position,
                                     v2(text_width, text_height),
                                     (colorf){1, 1, 1, 1});

    render_layer_push_command(&state->ui_layer, tooltip_text_command);
}

static void draw_item_slot_icon(game_state* state,
                                renderer* renderer,
                                item_slot* slot,
                                rectangle square,
                                colorf square_color)  {
    if (!item_slot_is_empty(slot)) {
        char* amount_text = memory_pool_allocate(&state->strings_scratch, 64);
        snprintf(amount_text, 64, "x%d", slot->item_count);
        render_layer_push_command(&state->ui_layer,
                render_command_spritesheet_quad(square.position.x,
                                                square.position.y,
                                                square.size.w,
                                                square.size.h,
                                                square_color.r,
                                                square_color.g,
                                                square_color.b,
                                                square_color.a,
                                                state->ui_icon_atlas,
                                                slot->item_ptr->atlas_icon,
                                                &state->assets));
        render_layer_push_command(&state->ui_layer,
                                  render_command_text(amount_text, 
                                                      state->gothic_font,
                                                      square.position.x,
                                                      square.position.y, 1,
                                                      1, 1, 1, 1));
    } else {
        render_layer_push_command(&state->ui_layer,
                render_command_spritesheet_quad(square.position.x,
                                                square.position.y,
                                                square.size.w,
                                                square.size.h,
                                                square_color.r,
                                                square_color.g,
                                                square_color.b,
                                                square_color.a,
                                                state->ui_icon_atlas,
                                                0,
                                                &state->assets));
    }
}

static b32 is_dragging_item(game_state* state) {
    return !item_slot_is_empty(&state->ui_info.dragged_item);
}

static void start_dragging_item(game_state* state, item_slot* slot) {
    if (!is_dragging_item(state)) {
        state->ui_info.dragged_item = *slot;
        slot->item_count = 0;
    }
}

static void try_to_place_stack_item(game_state* state, item_slot* slot) {
    item_slot_add_to(slot, &state->ui_info.dragged_item);
}

static void place_dragged_item_on_slot(game_state* state, item_slot* slot) {
    if (item_slot_is_empty(slot)) {
        *slot  = state->ui_info.dragged_item;
        state->ui_info.dragged_item.item_count = 0;
    } else {
        // handle stacking and swapping
        if (!item_slot_equal(slot, &state->ui_info.dragged_item)) {
            item_slot aux = *slot;
            *slot = state->ui_info.dragged_item;
            state->ui_info.dragged_item = aux;
        } else {
            if (item_slot_can_fit_more(slot) > 0) {
                try_to_place_stack_item(state, slot);
            } else {
                item_slot aux = *slot;
                *slot = state->ui_info.dragged_item;
                state->ui_info.dragged_item = aux;
            }
        }
    }
}

static void drop_dragged_item(game_state* state) {
    if (!item_slot_is_empty(&state->ui_info.dragged_item)) {
        actor* player = game_get_player_actor(state);
        tilemap* map = player->current_map;

        map_item_pickup pickup =
            (map_item_pickup){
                .contains_item = true,
                .x = player->position.x,
                .y = player->position.y,
                .w = 1,
                .h = 1,

                .item_to_pickup = state->ui_info.dragged_item
            };

        state->ui_info.dragged_item.item_count = 0;
        // NOTE(jerry): should have more complicated handling.
        // this isn't particularly great..... Eh.
        tilemap_push_item_pickup(map, pickup);
    }
}

void game_ui_show_combat_initial_elements(game_state* state) {
    stub_less_important("COMBAT BEGIN WIDGET SHOW!");
}

void game_ui_show_combat_finished_elements(game_state* state) {
    stub_less_important("COMBAT END WIDGET SHOW!");
}

static inline b32 ui_mouse_rectangle_intersection(game_ui_state* ui_state,
                                                  game_input* input,
                                                  rectangle rect) {
    bool result = point_intersects_rectangle(v2(input_get_mouse_x(input),
                                                input_get_mouse_y(input)),
                                                rect);
    ui_state->touching_ui |= result;
    return result;
}

// lots of clean up needed...
static void game_ui_widget_update_render( game_ui_widget* widget,
                                          game_state* state,
                                          renderer* renderer,
                                          game_input* input,
                                          f32 delta_time ){
    game_ui_widget* current_widget = widget;
    vec2 position = game_ui_widget_calculate_position_relative_to_parent(current_widget);

    static f32 tooltip_timer = 0;
    const static f32 tooltip_threshold = 0.45;

    if (!widget->show) {
        return;
    }

    if (widget->update) {
        widget->update(widget, state, input, delta_time);
    }

    switch( current_widget->type ){
        case GAME_UI_WIDGET_TYPE_PLACEHOLDER_LOOT_INVENTORY:
        {
            vec2 parent_size = current_widget->parent->size;
            parent_size = ui_scale_to_virtual_screenspace( state, parent_size );
            position = ui_scale_to_virtual_screenspace( state, position );

            actor* player = game_get_player_actor(state);

            const unsigned inventory_rows = ACTOR_MAX_INVENTORY_ITEMS / 10;
            const unsigned inventory_cols = ACTOR_MAX_INVENTORY_ITEMS / inventory_rows;

            unsigned target_inventory_rows;
            unsigned target_inventory_cols;

            item_slot* target_inventory = NULL;

            if (state->ui_info.loot_inventory.target_type == INVENTORY_LOOTING_TARGET_CONTAINER) {
                target_inventory_rows = CONTAINER_MAX_INVENTORY_ITEMS / 10;
                target_inventory_cols = CONTAINER_MAX_INVENTORY_ITEMS / target_inventory_rows;

                // I should probably stop assuming we target the player.
                // That makes no sense... Again we should be targetting
                // inventories and not indicies like these......
                tilemap* map = player->current_map;
                target_inventory = tilemap_get_container(map, state->ui_info.loot_inventory.target_index)->inventory; 
            } else {
                target_inventory_rows = ACTOR_MAX_INVENTORY_ITEMS / 10;
                target_inventory_cols = ACTOR_MAX_INVENTORY_ITEMS / target_inventory_rows;

                actor* actor_target = actors_list_get_actor(&state->actors, state->ui_info.loot_inventory.target_index);
                target_inventory = actor_target->inventory;
            }

            const float inventory_square_size = 64;

            bool any_non_empty_touched = false;
            bool any_touched = false;

            item_slot* slot_to_show_tooltip_for = NULL;

            for (unsigned row = 0; row < inventory_rows; ++row) {
                for (unsigned col = 0; col < inventory_cols; ++col) {
                    item_slot* current_slot = &player->inventory[(row * inventory_cols) + col];
                    rectangle square = (rectangle){
                        .x = (col * inventory_square_size) + position.x,
                        .y = (row * inventory_square_size) + position.y,
                        inventory_square_size,
                        inventory_square_size
                    };

                    bool is_moused_over = ui_mouse_rectangle_intersection(&state->ui_info, input, square);

                    colorf square_color = (colorf){1, 1, 1, 1};

                    if (is_moused_over) {
                        square_color.g = 0;
                        square_color.b = 0;
                        tooltip_timer += delta_time;

                        if (input_is_mouse_left_click(input)) {
                            if (!is_dragging_item(state) && !item_slot_is_empty(current_slot)) {
                                start_dragging_item(state, current_slot);
                            } else {
                                if (is_dragging_item(state)) {
                                    place_dragged_item_on_slot(state, current_slot);
                                }
                            }
                        }

                        if (!item_slot_is_empty(current_slot)) {
                            if (tooltip_timer >= tooltip_threshold) {
                                slot_to_show_tooltip_for = current_slot;
                            }
                            any_non_empty_touched = true;
                        }

                        any_touched = true;
                    }

                    draw_item_slot_icon(state, renderer, current_slot, square, square_color);
                }
            }

            for (unsigned row = 0; row < target_inventory_rows; ++row) {
                for (unsigned col = 0; col < target_inventory_cols; ++col) {
                    item_slot* current_slot = &target_inventory[(row * target_inventory_cols) + col];
                    rectangle square = (rectangle){
                        .x = (col * inventory_square_size) + position.x,
                        .y = (row * inventory_square_size) + position.y + 300,
                        inventory_square_size,
                        inventory_square_size
                    };

                    bool is_moused_over = ui_mouse_rectangle_intersection(&state->ui_info, input, square);

                    colorf square_color = (colorf){1, 1, 1, 1};

                    if (is_moused_over) {
                        square_color.g = 0;
                        square_color.b = 0;
                        tooltip_timer += delta_time;

                        if (input_is_mouse_left_click(input)) {
                            if (!is_dragging_item(state) && !item_slot_is_empty(current_slot)) {
                                start_dragging_item(state, current_slot);
                            } else {
                                if (is_dragging_item(state)) {
                                    place_dragged_item_on_slot(state, current_slot);
                                }
                            }
                        }

                        if (!item_slot_is_empty(current_slot)) {
                            if (tooltip_timer >= tooltip_threshold) {
                                slot_to_show_tooltip_for = current_slot;
                            }
                            any_non_empty_touched = true;
                        }

                        any_touched = true;
                    }

                    draw_item_slot_icon(state, renderer, current_slot, square, square_color);
                }
            }

            if (is_dragging_item(state)) {
                rectangle square = (rectangle){
                    .x = input_get_mouse_x(input),
                    .y = input_get_mouse_y(input),
                    inventory_square_size,
                    inventory_square_size
                };

                hide_game_cursor(state);
                draw_item_slot_icon(state, renderer, &state->ui_info.dragged_item, square, (colorf){1, 1, 1, 1});
            } else {
                show_game_cursor(state);
            }

            if (slot_to_show_tooltip_for) {
                draw_item_tooltip(state, input, renderer, slot_to_show_tooltip_for);
            }

            if (!any_touched) {
                tooltip_timer = 0;
                if (input_is_mouse_left_click(input)) {
                    drop_dragged_item(state);
                }
            }
        }
        break;
        case GAME_UI_WIDGET_TYPE_PLACEHOLDER_PLAYER_INVENTORY:
        {
            vec2 parent_size = current_widget->parent->size;
            parent_size = ui_scale_to_virtual_screenspace( state, parent_size );
            position = ui_scale_to_virtual_screenspace( state, position );

            actor* player = game_get_player_actor(state);

            const unsigned inventory_rows = ACTOR_MAX_INVENTORY_ITEMS / 10;
            const unsigned inventory_cols = ACTOR_MAX_INVENTORY_ITEMS / inventory_rows;
            const float inventory_square_size = 64;

            bool any_non_empty_touched = false;
            bool any_touched = false;

            item_slot* slot_to_show_tooltip_for = NULL;

            for (unsigned row = 0; row < inventory_rows; ++row) {
                for (unsigned col = 0; col < inventory_cols; ++col) {
                    item_slot* current_slot = &player->inventory[(row * inventory_cols) + col];
                    rectangle square = (rectangle){
                        .x = (col * inventory_square_size) + position.x,
                        .y = (row * inventory_square_size) + position.y,
                        inventory_square_size,
                        inventory_square_size
                    };

                    bool is_moused_over = ui_mouse_rectangle_intersection(&state->ui_info, input, square);

                    colorf square_color = (colorf){1, 1, 1, 1};

                    if (is_moused_over) {
                        square_color.g = 0;
                        square_color.b = 0;
                        tooltip_timer += delta_time;

                        if (input_is_mouse_left_click(input)) {
                            if (!is_dragging_item(state) && !item_slot_is_empty(current_slot)) {
                                start_dragging_item(state, current_slot);
                            } else {
                                if (is_dragging_item(state)) {
                                    place_dragged_item_on_slot(state, current_slot);
                                }
                            }
                        }

                        if (!item_slot_is_empty(current_slot)) {
                            if (tooltip_timer >= tooltip_threshold) {
                                slot_to_show_tooltip_for = current_slot;
                            }
                            any_non_empty_touched = true;
                        }

                        any_touched = true;
                    }

                    draw_item_slot_icon(state, renderer, current_slot, square, square_color);
                }
            }

            if (is_dragging_item(state)) {
                rectangle square = (rectangle){
                    .x = input_get_mouse_x(input),
                    .y = input_get_mouse_y(input),
                    inventory_square_size,
                    inventory_square_size
                };

                hide_game_cursor(state);
                draw_item_slot_icon(state, renderer, &state->ui_info.dragged_item, square, (colorf){1, 1, 1, 1});
            } else {
                show_game_cursor(state);
            }

            if (slot_to_show_tooltip_for) {
                draw_item_tooltip(state, input, renderer, slot_to_show_tooltip_for);
            }

            if (!any_touched) {
                tooltip_timer = 0;
                if (input_is_mouse_left_click(input)) {
                    drop_dragged_item(state);
                }
            }
        }
        break;
        case GAME_UI_WIDGET_TYPE_PLACEHOLDER_JOURNAL:
        {
            vec2 parent_size = current_widget->parent->size;
            parent_size = ui_scale_to_virtual_screenspace( state, parent_size );
            position = ui_scale_to_virtual_screenspace( state, position );

            f32 journal_layout_y_start = position.y;
            f32 journal_layout_y = journal_layout_y_start;

            u32 page_count = 1;
            u32 current_page = state->ui_info.journal.current_page;
            u32 drawing_page = 1;

            f32 page_width =  parent_size.w;
            f32 page_height = parent_size.h;

            // calculate page count.
            {
                f32 journal_layout_y = journal_layout_y_start;
                for( u64 journal_entry_index = 0;
                     journal_entry_index < state->journal.entry_count;
                     ++journal_entry_index ){
                    // Calculate layout movements.
                    f32 width_of_pages;
                    f32 move_y;
                    f32 size_y;
                    {
                        renderer_get_wrapped_text_size(
                            renderer,
                            state->gothic_font,
                            state->journal.entries[journal_entry_index].entry_text,
                            page_width,
                            page_height,
                            &width_of_pages, &move_y, &size_y 
                        );
                    }

                    journal_layout_y += move_y + size_y*1.5 + 48;

                    if( journal_layout_y >= page_height ){
                        journal_layout_y = journal_layout_y_start;
                        if( journal_entry_index + 1 < state->journal.entry_count ){
                            page_count++;
                        }
                    }
                }
            }

            for( u64 journal_entry_index = 0;
                 journal_entry_index < state->journal.entry_count;
                 ++journal_entry_index ){
                if( current_page == drawing_page ){
                    f32 journal_text_x = position.x;

                    render_command journal_entry_title = render_command_text_justified(
                        state->journal.entries[journal_entry_index].entry_title, 
                        state->gothic_font,
                        1.0, 0.0, 0.0, 0.0, 1.0,
                        journal_text_x, journal_layout_y,
                        page_width, page_height,
                        TEXT_COMMAND_JUSTIFICATION_LEFT
                    );

                    journal_layout_y += 48;

                    render_command journal_entry_text = render_command_text_wrapped(
                        state->journal.entries[journal_entry_index].entry_text,
                        state->gothic_font,
                        1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                        journal_text_x, journal_layout_y,
                        page_width, page_height
                    );

                    journal_layout_y += 48;

                    render_layer_push_command( &state->ui_layer, journal_entry_text );
                    render_layer_push_command( &state->ui_layer, journal_entry_title );
                }

                // Calculate layout movements.
                f32 width_of_pages;
                f32 move_y;
                f32 size_y;
                {
                    renderer_get_wrapped_text_size(
                        renderer,
                        state->font_texture,
                        state->journal.entries[journal_entry_index].entry_text,
                        page_width,
                        page_height,
                        &width_of_pages, &move_y, &size_y 
                    );
                }

                journal_layout_y += move_y + size_y*1.5 + 48;

                if( journal_layout_y >= page_height ){
                    journal_layout_y = journal_layout_y_start;
                    drawing_page++;
                }
            }
        }
        break;
        case GAME_UI_WIDGET_TYPE_PLACEHOLDER_DIALOGUE:
        {
            if( current_widget->parent ){
                // for word wrapping
                vec2 parent_size = current_widget->parent->size;
                parent_size = ui_scale_to_virtual_screenspace( state, parent_size );
                position = ui_scale_to_virtual_screenspace( state, position );

                if( state->active_dialogue && state->active_dialogue->initiated ){
                    dialogue_info_try_to_select_node(state->active_dialogue, state, &state->event_stack);
                    dialogue_node* current_node = &state->active_dialogue->nodes[ state->active_dialogue->current_node ];

                    char* speaker_name; 
                    char* text;
                    if( current_node->type == DIALOGUE_NODE_SELECTOR ){
                        speaker_name = "ERRNO";
                        text = "This shouldn't happen!";
                    }else{
                        /* speaker_name = current_node->dialogue.speaker->name; */
                        text = current_node->dialogue.text;
                    }


                    f32 width_used;
                    f32 move_y;
                    f32 size_y;
                    {
                        renderer_get_wrapped_text_size( renderer,
                                                        state->gothic_font,
                                                        text,
                                                        parent_size.x,
                                                        parent_size.y,
                                                        &width_used, &move_y, &size_y );
                    }

                    render_layer_push_text_wrapped( &state->ui_layer,
                                                    text, state->gothic_font,
                                                    0, 0, 0, 1,
                                                    position.x, position.y,
                                                    parent_size.x, parent_size.y );

                    f32 layout_x = position.x;
                    f32 layout_y = position.y + move_y + size_y * 1;

                    for( u64 choice_index = 0;
                         choice_index < current_node->dialogue.choice_count;
                         ++choice_index ){
                        f32 text_width;
                        f32 text_height;
                        f32 text_size;

                        colorf text_color = (colorf){
                            .r = 0,
                            .g = 0,
                            .b = 0,
                            .a = 1
                        };

                        char* choice_text = current_node->dialogue.choices[choice_index].text; 
                        renderer_get_text_size( renderer,
                                                state->gothic_font,
                                                choice_text,
                                                &text_width,
                                                &text_height,
                                                &text_size );
                        // determine collision
                        {
                            rectangle text_rect = (rectangle){
                                .position = v2(layout_x, layout_y),
                                .size = v2(text_width, text_height)
                            };

                            
                            if (ui_mouse_rectangle_intersection(&state->ui_info, input, text_rect)) {
                                text_color.r = 0.65;

                                if( input_is_mouse_left_click(input) ){
                                    dialogue_info_follow_choice( state->active_dialogue,
                                                                 choice_index,
                                                                 state, &state->event_stack );
                                }
                            }
                        }

                        render_layer_push_text( &state->ui_layer,
                                                choice_text,
                                                state->gothic_font,
                                                layout_x, layout_y,
                                                text_color.r,
                                                text_color.g,
                                                text_color.b,
                                                text_color.a );
                        layout_y += size_y;
                    }
                }
            }else{
                // error...
            }
        }
        break;
        case GAME_UI_WIDGET_TYPE_IMAGE:
        {
            colorf color = current_widget->image.color;
            vec2 size = ui_scale_to_virtual_screenspace( state, current_widget->size );
            position = ui_scale_to_virtual_screenspace( state, position );

            render_command cmd = render_command_spritesheet_quad( position.x, position.y,
                                                                  size.x, size.y,
                                                                  color.r, color.g, color.b, color.a,
                                                                  state->ui_atlas,
                                                                  current_widget->image.atlas_index,
                                                                  &state->assets );

            render_layer_push_command( &state->ui_layer, cmd );
        }
        break;
        case GAME_UI_WIDGET_TYPE_RECTANGLE:
        {
            colorf color = current_widget->rectangle.color;
            vec2 size = ui_scale_to_virtual_screenspace( state, current_widget->size );
            position = ui_scale_to_virtual_screenspace( state, position );


            render_command cmd = render_command_quad( position.x,
                                                      position.y,
                                                      size.w,
                                                      size.h,
                                                      color.r, color.g, color.b, color.a );
            render_layer_push_command( &state->ui_layer, cmd );
            ui_mouse_rectangle_intersection(&state->ui_info, input, (rectangle){position.x, position.y, size.w, size.h});
        }
        break;
        case GAME_UI_WIDGET_TYPE_WINDOW:
        {
            position = ui_scale_to_virtual_screenspace( state, position );
            vec2 size = ui_scale_to_virtual_screenspace( state, current_widget->size );
            game_ui_draw_ui_window_with_skin( state,
                                              current_widget->window.skin,
                                              position,
                                              size,
                                              current_widget->window.color ); 
            // dumb hack to adjust scale
            // I did the 9 slice wrong. Have to fix later.
            size.x += (32 * ui_scale);
            size.y += (32 * ui_scale);
            ui_mouse_rectangle_intersection(&state->ui_info, input, (rectangle){position.x, position.y, size.w, size.h});
        }
        break;
        case GAME_UI_WIDGET_TYPE_BUTTON:
        {
            position = ui_scale_to_virtual_screenspace( state, position );
            vec2 size = ui_scale_to_virtual_screenspace( state, current_widget->size );

            rectangle button_rectangle = (rectangle) {
                position.x,
                position.y,
                size.x,
                size.y
            };

            if (ui_mouse_rectangle_intersection(&state->ui_info, input, button_rectangle)) {
                if( input_is_mouse_left_click(input) ){
                    if( current_widget->button.on_click ){
                        current_widget->button.on_click( current_widget, state );
                    }
                }else{
                    // TODO(jerry): HIGHLIGHT
                }
            }

            render_command cmd = render_command_spritesheet_quad( position.x,
                                                                  position.y,
                                                                  size.w,
                                                                  size.h,
                                                                  1.0, 1.0, 1.0, 1.0,
                                                                  state->ui_atlas,
                                                                  current_widget->button.atlas_index,
                                                                  &state->assets );
            render_layer_push_command( &state->ui_layer, cmd );
        }
        break;
        case GAME_UI_WIDGET_TYPE_TEXT:
        {
            vec2 text_pos = position;
            text_pos = ui_scale_to_virtual_screenspace( state, text_pos );
            render_layer_push_command( &state->ui_layer,
                                       render_command_text( current_widget->text.text, 
                                                            state->gothic_font, 
                                                            text_pos.x,
                                                            text_pos.y,
                                                            1.0f, 
                                                            current_widget->text.color.r,
                                                            current_widget->text.color.g,
                                                            current_widget->text.color.b,
                                                            current_widget->text.color.a ) );
        }
        break;
        case GAME_UI_WIDGET_TYPE_NONE:
        default:
        {
        }
        break;
    }

    for( u64 widget_index = 0; widget_index < widget->children_count; ++widget_index ){
        game_ui_widget_update_render( &widget->children[widget_index], state, renderer, input, delta_time );
    }
}

static void game_ui_screen_update_render( game_state* state,
                                          game_ui_screen* screen,
                                          renderer* renderer,
                                          game_input* input,
                                          f32 delta_time ){
    game_ui_widget_update_render( &screen->root, state, renderer, input, delta_time );
}

static game_ui_widget* game_ui_widget_push_child( game_ui_widget* parent,
                                                  game_ui_widget target ){
    parent->children_count++;
    target.parent = parent;
    parent->children[parent->children_count - 1] = target;
    return &parent->children[parent->children_count - 1];
}

static game_ui_widget game_widget_window( u8 skin,
                                          vec2 position,
                                          vec2 size ){
    return (game_ui_widget) {
        .type = GAME_UI_WIDGET_TYPE_WINDOW,
        .show = true,
        .position = position,
        .size = size,
        .window = (game_ui_widget_window){
            .skin = skin,
            .color = (colorf){1, 1, 1, 1}
        }
    };
}

static game_ui_widget game_widget_rectangle( vec2 position,
                                             vec2 size,
                                             colorf color ){
    return (game_ui_widget) {
        .type = GAME_UI_WIDGET_TYPE_RECTANGLE,
        .show = true,
        .position = position,
        .size = size,
        .rectangle = (game_ui_widget_rectangle){
            .color.r = color.r,
            .color.g = color.g,
            .color.b = color.b,
            .color.a = color.a
        }
    };
}

static game_ui_widget game_widget_text( char* text,
                                        game_asset_handle font,
                                        vec2 position,
                                        colorf color ){
    return (game_ui_widget) {
        .type = GAME_UI_WIDGET_TYPE_TEXT,
        .show = true,
        .position = position,
        .text = (game_ui_widget_text) {
            .text = text,
            .font = font,
            .color = color
        }
    };
}

static game_ui_widget game_widget_button( u32 atlas_index,
                                          vec2 position,
                                          vec2 size,
                                          game_ui_widget_button_callback callback ){
    return (game_ui_widget) {
        .type = GAME_UI_WIDGET_TYPE_BUTTON,
        .show = true,
        .position = position,
        .size = size,
        .button = (game_ui_widget_button) {
            .atlas_index = atlas_index,
            .on_click = callback
        }
    };
}

static game_ui_widget game_widget_dialogue_placeholder( vec2 position ){
    return (game_ui_widget) {
        .type = GAME_UI_WIDGET_TYPE_PLACEHOLDER_DIALOGUE,
        .position = position,
        .show = true
    };
}

static game_ui_widget game_widget_image(u32 atlas_index, vec2 position, vec2 size, colorf color){
    return (game_ui_widget) {
        .type = GAME_UI_WIDGET_TYPE_IMAGE,
        .position = position,
        .size = size,
        .show = true,
        .image.atlas_index = atlas_index,
        .image.color = color
    };
}

static game_ui_widget game_widget_loot_inventory(vec2 position) {
    return (game_ui_widget) {
        .type = GAME_UI_WIDGET_TYPE_PLACEHOLDER_LOOT_INVENTORY,
        .position = position,
        .show = true
    };
}

static game_ui_widget game_widget_player_inventory(vec2 position) {
    return (game_ui_widget) {
        .type = GAME_UI_WIDGET_TYPE_PLACEHOLDER_PLAYER_INVENTORY,
        .position = position,
        .show = true
    };
}

static game_ui_widget game_widget_journal_placeholder(vec2 position){
    return (game_ui_widget) {
        .type = GAME_UI_WIDGET_TYPE_PLACEHOLDER_JOURNAL,
        .position = position,
        .show = true
    };
}

// button callback forward declarations
static void internal_game_ui_on_pause_resume_button_click(game_ui_widget* self, void* userdata);
static void internal_dialogue_override_button(game_ui_widget* self, void* userdata);
static void internal_attack_override_button(game_ui_widget* self, void* userdata);
static void internal_game_ui_on_pause_load_game_button_click(game_ui_widget* self, void* userdata);
static void internal_game_ui_on_pause_quit_game_button_click(game_ui_widget* self, void* userdata);
static void internal_game_ui_on_start_button_click(game_ui_widget* self, void* userdata);
static void internal_game_ui_on_load_game_button_click(game_ui_widget* self, void* userdata);
static void internal_game_ui_on_quit_game_button_click(game_ui_widget* self, void* userdata);
static void internal_game_ui_on_open_model_editor_game_button_click(game_ui_widget* self, void* userdata);
static void internal_game_ui_dummy_button(game_ui_widget* self, void* userdata);

// special update callback forward declarations
static void internal_ingame_screen_widget_update(game_ui_widget* self, game_state* state, game_input* input, f32 delta_time);

void game_ui_initialize( game_state* state ){
    state->ui_info.virtual_width = 800;
    state->ui_info.virtual_height = 600;
    state->ui_info.journal.current_page = 1;

    memory_pool_init( &state->ui_info.ui_memory, KB(8) );

    state->ui_info.ui_screens[GAME_UI_STATE_NONE].userdata = state;
    state->ui_info.ui_screens[GAME_UI_STATE_MAINMENU].userdata = state;
    state->ui_info.ui_screens[GAME_UI_STATE_INGAME].userdata = state;
    state->ui_info.ui_screens[GAME_UI_STATE_DIALOGUE].userdata = state;
    state->ui_info.ui_screens[GAME_UI_STATE_JOURNAL].userdata = state;
    state->ui_info.ui_screens[GAME_UI_STATE_INVENTORY_PLAYER] .userdata = state;
    state->ui_info.ui_screens[GAME_UI_STATE_INVENTORY_LOOTING].userdata = state;
    state->ui_info.ui_screens[GAME_UI_STATE_INVENTORY_TRADING].userdata = state;
    state->ui_info.ui_screens[GAME_UI_STATE_CHARACTER_RECORD].userdata = state;
    state->ui_info.ui_screens[GAME_UI_STATE_PAUSE_MENU].userdata = state;
    state->ui_info.ui_screens[GAME_UI_STATE_CUTSCENE].userdata = state;

    // MAIN MENU UI
    {
        game_ui_screen* main_menu_screen = &state->ui_info.ui_screens[GAME_UI_STATE_MAINMENU];
        main_menu_screen->root.show = true;
        main_menu_screen->root.children =
            memory_pool_allocate(&state->ui_info.ui_memory, sizeof(game_ui_widget) * 2);

        // assuming everything comes from the atlas.
        game_ui_widget* paper_window = game_ui_widget_push_child(&main_menu_screen->root, game_widget_window(SCALABLE_PAPER_UI_SKIN, v2(200, 0), v2(400, 545)));
        paper_window->children = memory_pool_allocate(&state->ui_info.ui_memory, sizeof(game_ui_widget) * 5);
        game_ui_widget* stone_window = game_ui_widget_push_child(paper_window, game_widget_window(SCALABLE_DEFAULT_UI_SKIN, v2(110, 100), v2(175, 380)));
        stone_window->children = memory_pool_allocate(&state->ui_info.ui_memory, sizeof(game_ui_widget) * 5);

        game_ui_widget_push_child(paper_window, game_widget_text("WANDERER", state->gothic_font, v2(150, 50), (colorf){0, 0, 0, 1}));

        game_ui_widget_push_child(stone_window, game_widget_button(MAIN_MENU_NEW_GAME_BUTTON, v2(40, 80), v2(96 * 1.3, 32 * 1.3), internal_game_ui_on_start_button_click));
        game_ui_widget_push_child(stone_window, game_widget_button(MAIN_MENU_LOAD_GAME_BUTTON, v2(40, 80 + 30 * 2), v2(96 * 1.3, 32 * 1.3), internal_game_ui_on_load_game_button_click));
        game_ui_widget_push_child(stone_window, game_widget_button(MAIN_MENU_QUIT_GAME_BUTTON, v2(40, 80 + 30 * 4), v2(96 * 1.3, 32 * 1.3), internal_game_ui_on_quit_game_button_click));
        game_ui_widget_push_child(stone_window, game_widget_button(MAIN_MENU_LOAD_GAME_BUTTON, v2(40, 80 + 30 * 6), v2(96 * 1.3, 32 * 1.3), internal_game_ui_on_open_model_editor_game_button_click));
    }

    // PAUSE UI
    {
        game_ui_screen* pause_screen = &state->ui_info.ui_screens[GAME_UI_STATE_PAUSE_MENU];
        pause_screen->root.show = true;
        pause_screen->root.children =
            memory_pool_allocate(&state->ui_info.ui_memory, sizeof(game_ui_widget) * 3);

        colorf darkening_color = { 0.0, 0.0, 0.0, 0.6 };
        game_ui_widget* darken_rect = game_ui_widget_push_child(&pause_screen->root, game_widget_rectangle(v2(0, 0), v2(800, 600), darkening_color));
        game_ui_widget* paper_window = game_ui_widget_push_child(&pause_screen->root, game_widget_window(SCALABLE_PAPER_UI_SKIN, v2(200, 0), v2(400, 545)));
        paper_window->children = memory_pool_allocate(&state->ui_info.ui_memory, sizeof(game_ui_widget) * 4);
        game_ui_widget_push_child(paper_window, game_widget_text("WANDERER", state->gothic_font, v2(150, 50), (colorf){0, 0, 0, 1}));
        game_ui_widget_push_child(paper_window, game_widget_button(MAIN_MENU_NEW_GAME_BUTTON, v2(150, 120), v2(96 * 1.3, 32 * 1.3), internal_game_ui_on_pause_resume_button_click));
        game_ui_widget_push_child(paper_window, game_widget_button(MAIN_MENU_LOAD_GAME_BUTTON, v2(150, 120 + 30 * 2), v2(96 * 1.3, 32 * 1.3), internal_game_ui_on_pause_load_game_button_click));
        game_ui_widget_push_child(paper_window, game_widget_button(MAIN_MENU_QUIT_GAME_BUTTON, v2(150, 120 + 30 * 4), v2(96 * 1.3, 32 * 1.3), internal_game_ui_on_pause_quit_game_button_click));
    }

    // INGAME UI
    {
        game_ui_screen* ingame_screen = &state->ui_info.ui_screens[GAME_UI_STATE_INGAME];
        ingame_screen->root.show = true;
        ingame_screen->root.children = memory_pool_allocate(&state->ui_info.ui_memory, sizeof(game_ui_widget) * 2);

        ingame_screen->root.update = internal_ingame_screen_widget_update;

        game_ui_widget* paper_window = game_ui_widget_push_child(&ingame_screen->root, game_widget_window(SCALABLE_PAPER_UI_SKIN, v2(50, 480), v2(650, 55)));
        paper_window->children = memory_pool_allocate(&state->ui_info.ui_memory, sizeof(game_ui_widget) * 12);

        // UI Buttons
        {
            f32 layout_x = 400;
            f32 layout_y = 23;
            {
                game_ui_widget_push_child(paper_window, game_widget_button(DIALOGUE_OVERRIDE_BUTTON, v2(layout_x, layout_y), v2(32, 32), internal_dialogue_override_button));
                layout_y += 33;
                game_ui_widget_push_child(paper_window, game_widget_button(ATTACK_OVERRIDE_BUTTON, v2(layout_x, layout_y), v2(32, 32), internal_attack_override_button));
            }
            layout_x += 33;
            layout_y = 23;
            {
                game_ui_widget_push_child(paper_window, game_widget_button(USE_SKILL_OVERRIDE_BUTTON, v2(layout_x, layout_y), v2(32, 32), internal_game_ui_dummy_button));
                layout_y += 33;
                game_ui_widget_push_child(paper_window, game_widget_button(OPEN_JOURNAL_BUTTON, v2(layout_x, layout_y), v2(32, 32), internal_game_ui_dummy_button));
            }
            layout_x += 33;
            layout_y = 23;
            {
                game_ui_widget_push_child(paper_window, game_widget_button(OPEN_INVENTORY_BUTTON, v2(layout_x, layout_y), v2(32, 32), internal_game_ui_dummy_button));
                layout_y += 33;
                game_ui_widget_push_child(paper_window, game_widget_button(OPEN_CHARACTER_RECORD, v2(layout_x, layout_y), v2(32, 32), internal_game_ui_dummy_button));
            }
            layout_x += 33;
            layout_y = 23;
            {
                game_ui_widget_push_child(paper_window, game_widget_button(REST_BUTTON, v2(layout_x, layout_y), v2(32, 32), internal_game_ui_dummy_button));
            }
        }

        // UI Combat Info
        {
            game_ui_widget_push_child(paper_window, game_widget_text("hey you shouldn't be able to see me >:(", state->gothic_font, v2(10, 10), (colorf){0, 0, 0, 1}));
        }
    }

    // DIALOGUE SCREEN UI
    // Because of the way the Dialogue must work, it will use a placeholder
    // "widget" that is quite similar to the IMGUI, but just more themed after the game.
    // Also allows keyboard option selection
    {
        game_ui_screen* dialogue_screen = &state->ui_info.ui_screens[GAME_UI_STATE_DIALOGUE];
        dialogue_screen->root.show = true;
        dialogue_screen->root.children = memory_pool_allocate(&state->ui_info.ui_memory, sizeof(game_ui_widget) * 2);
        game_ui_widget* paper_window = game_ui_widget_push_child(&dialogue_screen->root, game_widget_window(SCALABLE_PAPER_UI_SKIN, v2(50, 340), v2(650, 185)));
        paper_window->children = memory_pool_allocate(&state->ui_info.ui_memory, sizeof(game_ui_widget) * 1);
        game_ui_widget_push_child(paper_window, game_widget_dialogue_placeholder(v2(25, 40)));
    }

    // INVENTORY SCREEN UI
    // There are 3 separate inventory screens intended for different
    // purposes.
    {
        // GAME_UI_STATE_INVENTORY_PLAYER
        // default player inventory. Drag, drop, make stacks
        {
            game_ui_screen* player_inventory_screen = &state->ui_info.ui_screens[GAME_UI_STATE_INVENTORY_PLAYER];
            player_inventory_screen->root.show = true;
            player_inventory_screen->root.children = memory_pool_allocate(&state->ui_info.ui_memory, sizeof(game_ui_widget) * 2);
            game_ui_widget* player_inventory_widget = game_ui_widget_push_child(&player_inventory_screen->root, game_widget_player_inventory(v2(50, 50)));
        }

        // GAME_UI_STATE_INVENTORY_LOOTING
        // Same as default player inventory, but it also
        // allows you to move to and from another inventory,
        // which is basically anything but the player.
        {
            game_ui_screen* looting_inventory_screen = &state->ui_info.ui_screens[GAME_UI_STATE_INVENTORY_LOOTING];
            looting_inventory_screen->root.show = true;
            looting_inventory_screen->root.children = memory_pool_allocate(&state->ui_info.ui_memory, sizeof(game_ui_widget) * 2);
            game_ui_widget* looting_inventory_widget = game_ui_widget_push_child(&looting_inventory_screen->root, game_widget_loot_inventory(v2(50, 50)));
        }

        // GAME_UI_STATE_INVENTORY_TRADING
        // This is merchant shop view.
        // Effectively the same as looting, but it looks very different
        // and also everything requires payment now.
        {
        }
    }

    // JOURNAL SCREEN UI
    // Again this is also spoofed because it's dynamically rendered.
    // There are a few "real" buttons for page moving and that's about it.
    {
        game_ui_screen* journal_screen = &state->ui_info.ui_screens[GAME_UI_STATE_JOURNAL];
        journal_screen->root.show = true;
        journal_screen->root.children = memory_pool_allocate(&state->ui_info.ui_memory, sizeof(game_ui_widget) * 5);
        game_ui_widget* journal_paper = game_ui_widget_push_child(&journal_screen->root, game_widget_image(20, v2(200, 0), v2(158*2.5, 600), (colorf){1, 1, 1, 1}));
        journal_paper->children = memory_pool_allocate(&state->ui_info.ui_memory, sizeof(game_ui_widget) * 5);
        game_ui_widget_push_child(journal_paper, game_widget_journal_placeholder(v2(25, 40)));
        game_ui_widget_push_child(journal_paper, game_widget_button(26, v2(15, 600 - 40), v2(32, 32), internal_game_ui_dummy_button));
        game_ui_widget_push_child(journal_paper, game_widget_button(26, v2(15+48, 600 - 40), v2(32, 32), internal_game_ui_dummy_button));
    }

    // Character Record UI
    // Buttons and another spoofed widget.
    // has a paper doll and stuff
    {
        game_ui_screen* record_screen = &state->ui_info.ui_screens[GAME_UI_STATE_CHARACTER_RECORD];
        record_screen->root.show = true;
        record_screen->root.children = memory_pool_allocate(&state->ui_info.ui_memory, sizeof(game_ui_widget) * 2);
        game_ui_widget* paper_window = game_ui_widget_push_child(&record_screen->root, game_widget_window(SCALABLE_PAPER_UI_SKIN, v2(200, 0), v2(400, 545)));
    }
}

// define all special update callbacks here.
static void internal_ingame_screen_widget_update(game_ui_widget* self,
                                                 game_state* state,
                                                 game_input* input,
                                                 f32 delta_time) {
    /*
      Hmmmm maybe don't do something like this. Not entirely anyways...
    */
    game_ui_widget* the_paper_window = &self->children[0]; 
    game_ui_widget* combat_info_text = &the_paper_window->children[7];
    if (state->player_is_in_combat) {
        combat_info_text->show = true;
        char* combat_info_temp_text = memory_pool_allocate(&state->strings_scratch, 128);
        actor* player = game_get_player_actor(state);

        if (player->finished_turn) {
            actor* participating_actor =
                combat_participant_lookup_actor(&state->actors, game_combat_round_get_current_participant(&state->combat_round));

            snprintf(combat_info_temp_text, 128, "Waiting for %s to finish", participating_actor->name);
            the_paper_window->window.color = (colorf) {0.4, 0.4, 0.4, 1.0};
        } else {
            snprintf(combat_info_temp_text, 128, "Player Action Points %d out of %d", player->action_points, player->max_action_points);
            the_paper_window->window.color = (colorf) {1.0, 1.0, 1.0, 1.0};
        }

        combat_info_text->text.text = combat_info_temp_text;
    } else {
        combat_info_text->show = false;
    }
}

// define all button callbacks here.

static void internal_game_ui_on_pause_load_game_button_click(game_ui_widget* self, void* userdata) {
    fprintf(stderr, "ingame load!\n");
    game_state* state = userdata;
}

static void internal_game_ui_on_pause_quit_game_button_click(game_ui_widget* self, void* userdata) {
    game_state* state = userdata;
    game_ui_toggle_pause( state );
    state->mode = GAME_STATE_MAIN_MENU;
    state->ui_info.mode = GAME_UI_STATE_NONE;
}

static void internal_game_ui_on_start_button_click(game_ui_widget* self, void* userdata) {
    game_state* state = userdata;
    state->mode = GAME_STATE_GAME_PLAY;
    state->ui_info.mode = GAME_UI_STATE_INGAME;
}

static void internal_game_ui_on_load_game_button_click(game_ui_widget* self, void* userdata) {
    game_state* state = userdata;
    fprintf(stderr, "load\n");
}

static void internal_game_ui_on_quit_game_button_click(game_ui_widget* self, void* userdata) {
    game_state* state = userdata;
    state->mode = GAME_STATE_QUIT;
}

static void internal_game_ui_on_open_model_editor_game_button_click(game_ui_widget* self, void* userdata) {
    game_state* state = userdata;
    state->mode = GAME_STATE_EDITOR_MODEL_EDITOR;
    state->ui_info.mode = GAME_UI_STATE_NONE;
}


static void internal_game_ui_dummy_button(game_ui_widget* self, void* user_data) {
    fprintf(stderr, "This button doesn't do anything yet!");
}

static void internal_dialogue_override_button(game_ui_widget* self, void* user_data) {
    game_state* state = user_data;
    state->ui_info.cursor.override_state = CURSOR_OVERRIDE_STATE_DIALOGUE;
    /* internal_game_ui_dummy_button(self, user_data); */
}

static void internal_attack_override_button(game_ui_widget* self, void* user_data) {
    game_state* state = user_data;
    state->ui_info.cursor.override_state = CURSOR_OVERRIDE_STATE_ATTACK;
}

static void internal_game_ui_on_pause_resume_button_click(game_ui_widget* self, void* userdata) {
    game_state* state = userdata;
    game_ui_toggle_pause( state );
    state->mode = GAME_STATE_GAME_PLAY;
    state->ui_info.mode = GAME_UI_STATE_INGAME;
}

