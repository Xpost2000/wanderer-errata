#include "wanderer_assets.h"

#include "wanderer_actor.h"
#include "wanderer_tilemap.h"

#include "wanderer.h"

#include "easing_functions.h"

#define FLAVOR_TEXT_BUFFER_LENGTH 512

damage_info pure_physical_damage(i32 damage, u8 enchantment_level) {
    damage_info damage_result = {
        .type = DAMAGE_TYPE_PHYSICAL,
        .enchantment_level = enchantment_level,
        .damage = damage
    };
    
    return damage_result;
}

damage_info slash_damage(i32 damage, u8 enchantment_level) {
    damage_info damage_result = {
        .type = DAMAGE_TYPE_PHYSICAL_SLASHING,
        .enchantment_level = enchantment_level,
        .damage = damage
    };
    
    return damage_result;
}

damage_info thrust_damage(i32 damage, u8 enchantment_level) {
    damage_info damage_result = {
        .type = DAMAGE_TYPE_PHYSICAL_THRUSTING,
        .enchantment_level = enchantment_level,
        .damage = damage
    };
    
    return damage_result;
}

damage_info blunt_damage(i32 damage, u8 enchantment_level) {
    damage_info damage_result = {
        .type = DAMAGE_TYPE_PHYSICAL_BLUNT,
        .enchantment_level = enchantment_level,
        .damage = damage
    };
    
    return damage_result;
}

damage_info magic_damage(i32 damage) {
    damage_info damage_result = {
        .type = DAMAGE_TYPE_MAGIC,
        .enchantment_level = NON_PHYSICAL_ENCHANTMENT_LEVEL,
        .damage = damage
    };
    
    return damage_result;
}

static ability_score_info create_default_ability_scores(void) {
    ability_score_info default_scores = {};

    default_scores.strength = 9;
    default_scores.dexterity = 9;
    default_scores.willpower = 9;
    default_scores.intelligence = 9;
    default_scores.constitution = 9;
    default_scores.charisma = 9;

    return default_scores;
}

#define DEFAULT_ACTOR_ARMOR_CLASS 6

actor actor_create_default(char* name,
                           i32 health_points,
                           u32 action_points) {
    if (health_points <= 0) {
        health_points = 1;
    }

    if (action_points < ACTOR_MINIMUM_ACCEPTABLE_ACTION_POINTS) {
        action_points = ACTOR_MINIMUM_ACCEPTABLE_ACTION_POINTS;
    }

    actor new_actor = {};

    new_actor.armor_class = DEFAULT_ACTOR_ARMOR_CLASS;

    strncpy(new_actor.name, name, ACTOR_NAME_STRING_LENGTH);
    new_actor.health_points = health_points;
    new_actor.max_health_points = new_actor.health_points;

    new_actor.action_points = action_points;
    new_actor.max_action_points = new_actor.action_points;

    new_actor.size.w = 1.0f;
    new_actor.size.h = 1.0f;

    new_actor.ability_scores = create_default_ability_scores();

    return new_actor;
}

void container_add_item(container* self, item_slot item) {
    for (unsigned item_slot_index = 0;
         item_slot_index < CONTAINER_MAX_INVENTORY_ITEMS;
         ++item_slot_index) {
        item_slot* current_slot = &self->inventory[item_slot_index];

        b32 placement_result = item_slot_add_to(current_slot, &item);

        if (placement_result) {
            return true;
        } else {
            continue;
        }
    }

    return false;
}

void item_dictionary_initialize(item_dictionary* self, u64 capacity) {
    self->item_count = 0;
    self->items = memory_allocate(sizeof(item) * capacity);
}

void item_dictionary_finish(item_dictionary* self) {
    memory_deallocate(self->items);
}

void item_dictionary_add_item( item_dictionary* dictionary, item to_add ){
    assert( dictionary != NULL );
    assert( to_add.type != ITEM_TYPE_NONE );

    item* dictionary_item = &dictionary->items[ dictionary->item_count ];

    *dictionary_item = to_add;
    dictionary_item->id = dictionary->item_count++;
}

item_slot item_dictionary_make_instance_of( item_dictionary* dictionary, char* name ){
    u64 copy_index = 0;

    for( copy_index; copy_index < dictionary->item_count; ++copy_index ){
        if( strcmp( name, dictionary->items[copy_index].name ) == 0 ){
            item_slot returned_slot = {};

            returned_slot.item_ptr = &dictionary->items[copy_index];
            returned_slot.item_count = 1;

            return returned_slot;
        }
    }

    item_slot none = {};
    none.item_ptr = NULL;
    none.item_count = 0;
    return none;
}

b32 item_slot_equal(item_slot* a, item_slot* b) {
    return (a->item_ptr == b->item_ptr);
}

i32 item_slot_can_fit_more(item_slot* slot) {
    if (slot) {
        if (!item_slot_is_empty(slot)) {
            item* actual_item = slot->item_ptr;

            i32 as_i32_stack_max = (i32)actual_item->stack_max;
            i32 as_i32_item_count = (i32)slot->item_count;

            i32 diff = (as_i32_stack_max - as_i32_item_count);
            return diff;
        } else {
            return true; // don't know what to return.
        }
    }

    return 0;
}

b32 item_slot_is_empty(item_slot* slot) {
    if (slot->item_count == 0) {
        return true;
    } else {
        item* actual_item = slot->item_ptr;

        if (actual_item) {
            if (actual_item->type == ITEM_TYPE_NONE) {
                return true;
            }
        } else {
            return false;
        }
    }

    return false;
}

u8 map_string_to_item_class_id(const char* string) {
    for (unsigned item_class_id_index = 0;
         item_class_id_index < ITEM_CLASS_TYPES;
         ++item_class_id_index) {
        if (string_compare(string, item_class_id_strings[item_class_id_index]) == 0) {
            return item_class_id_index;
        }
    }

    return ITEM_CLASS_MISC;
}

u8 map_string_to_item_type(const char* string) {
    for (unsigned item_type_index = 0;
         item_type_index < ITEM_TYPE_COUNT;
         ++item_type_index) {
        if (string_compare(string, item_type_strings[item_type_index]) == 0) {
            return item_type_index;
        }
    }

    return ITEM_TYPE_NONE;
}

void item_set_name(item* self, const char* name) {
    strncpy(self->name, name, ITEM_NAME_STR_LENGTH);
}

void item_set_description(item* self, const char* description) {
    strncpy(self->description, description, ITEM_DESCRIPTION_STR_LENGTH);
}

static actor_model_animation_frame* actor_visual_info_find_current_frame( actor_visual_info* visual, game_assets* assets ){
    game_asset* model_asset = game_asset_get_from_handle( assets, visual->model_id );
    actor_model* model = &model_asset->actor_model;

    actor_model_animation_group* animation_group =
        &model->animation_groups[visual->animation];

    // technically this shouldn't be here.
    visual->selected_set = actor_model_animation_group_find_animation_set_for_direction( animation_group, visual->looking_direction );

    if( animation_group->animation_set_count ){
        actor_model_animation_set* animation_set =
            &animation_group->animation_sets[visual->selected_set];

        if( animation_set->animation_frame_count ){
            return &animation_set->frames[visual->current_frame];
        }
    }

    return NULL;
} 

static b32 animation_frame_overflows_animation_set( actor_visual_info* visual, game_assets* assets ){
    game_asset* model_asset = game_asset_get_from_handle( assets, visual->model_id );
    actor_model* model = &model_asset->actor_model;

    actor_model_animation_group* animation_group =
        &model->animation_groups[visual->animation];

    if( animation_group->animation_set_count ){
        actor_model_animation_set* animation_set =
            &animation_group->animation_sets[visual->selected_set];

        if( (visual->current_frame+1) >= animation_set->animation_frame_count ){
            return true;
        } else {
            return false;
        }
    }

    return true;
}

static void handle_animation_flashes(actor_visual_info* visual, f32 delta_time) {
    if (visual->flashes_left) {
        if (visual->flash_cycle) {
            if (visual->healing_flash) {
                visual->rgba.r = 0;
                visual->rgba.g = 1;
                visual->rgba.b = 0;
            } else {
                visual->rgba.r = 1;
                visual->rgba.g = 0;
                visual->rgba.b = 0;
            }
        } else {
            visual->rgba.r = 1;
            visual->rgba.g = 1;
            visual->rgba.b = 1;
        }

        visual->hurt_flash_timer -= delta_time;

        if (visual->hurt_flash_timer <= 0) {
            visual->hurt_flash_timer = time_per_flash; 
            visual->flashes_left--;
            visual->flash_cycle ^= 1;
        }
    }
}

static void handle_visual_bar_time(f32* timer, f32 delta_time) {
    if (*timer > 0.0f) {
        *timer -= delta_time;
    }
}

static void actor_visual_info_handle_animation( actor_visual_info* visual,
                                                game_assets* assets,
                                                f32 delta_time ){
    if( visual->animation >= ACTOR_ANIMATION_GROUP_COUNT ) {
        visual->animation = ACTOR_ANIMATION_GROUP_IDLE;
        visual->current_frame_timer = 0;
        visual->selected_set = 0;
        visual->current_frame = 0;
        visual->animation_finish_iterations = 0;

        fprintf(stderr, "bad anim\n");
        return;
    }

    actor_model_animation_frame* animation_frame =
        actor_visual_info_find_current_frame( visual, assets );

    if (animation_frame) {
        visual->current_frame_timer += delta_time;

        if( visual->current_frame_timer >= animation_frame->time_to_next_frame ){
            if( animation_frame_overflows_animation_set(visual, assets) ){
                visual->animation_finish_iterations++;
                visual->current_frame = 0;
            } else {
                visual->current_frame++;
            }
            visual->current_frame_timer = 0;
        }
    }

    // handle some special stuff....
    {
        handle_animation_flashes(visual, delta_time);
        handle_visual_bar_time(&visual->health_bar_show_time, delta_time);
        handle_visual_bar_time(&visual->action_points_bar_show_time, delta_time);
    }
}

void actor_update_visual_info( actor* target,
                               game_assets* assets,
                               f32 delta_time ){
    actor_visual_info* visual = &target->visual_info;

    {
        // odd coupling....
        if (actor_is_dead(target)) {
            visual->animation = ACTOR_ANIMATION_GROUP_DEAD;
        } else {
            if (visual->animation_finish_iterations > 0) {
                // check if repeating...
                visual->animation = ACTOR_ANIMATION_GROUP_IDLE;
            }
        }

        visual->position = target->position;
        visual->scale = (vec2){.w = 1, .h = 1};
        visual->rgba = (colorf){1.0, 1.0, 1.0, 1.0};
    }

    actor_visual_info_handle_animation( visual, assets, delta_time );
}

b32 actor_is_dead( actor* target ){
    return target->health_points <= 0;
}

b32 actor_can_issue_new_command(actor* self, game_state* state) {
    return !(state->player_is_in_combat && !self->current_command.finished && !self->finished_turn);
}

void actor_command_attack(actor* self, game_state* state, actor* target) {
    actor_command* command = &self->current_command;

    if (actor_can_issue_new_command(self, state)) {
        command->type = ACTOR_COMMAND_ATTACK;
        command->finished = false;
        command->attacking_other.target = target;

        fprintf(stderr, "issued attack command\n");
    }
}

void actor_command_move_to(actor* self, game_state* state, vec2 position) {
    actor_command* command = &self->current_command;

    if (actor_can_issue_new_command(self, state)) {
        command->type = ACTOR_COMMAND_MOVE_TO;
        command->finished = false;
        command->movement.position = position;
        command->movement.start_position = self->position;

        fprintf(stderr, "issued movement command\n");
    }
}


void actor_force_cast_spell_from_spellbook(actor* self, game_state* state, u16 spell_index, spell_target target) {
    stub_important_code("force_cast_spell_from_spellbook");
}

void actor_cast_spell(actor* self, game_state* state, u16 spell_index, spell_target target) {
    stub_important_code("cast_spell");
}

void actor_force_cast_spell(actor* self, game_state* state, char* spell_name, spell_target target) {
    stub_less_important("force_cast_spell_from_spellbook");
    actor_command* command = &self->current_command;

    if (actor_can_issue_new_command(self, state)) {
        command->type = ACTOR_COMMAND_CAST_SPELL;
        command->finished = false;
        command->cast_spell.was_forced = true;
        command->cast_spell.spell_info.origin = CAST_SPELL_ORIGIN_DICTIONARY;
        command->cast_spell.spell_info.id = spell_name;
        command->cast_spell.cast_timer = 0;
        command->cast_spell.target_info = target;
    }
}

void actor_remove_dialogue_file_reference( actor* target ) {
    if (target->has_dialogue) {
        target->has_dialogue = false;
    }
}

void actor_set_dialogue_file_reference( actor* target, char* dialogue_file_name ) {
    if (!target->has_dialogue) {
        target->has_dialogue = true;
        strncpy(target->dialogue_file_reference,
                dialogue_file_name,
                ACTOR_DIALOGUE_REFERENCE_NAME_LENGTH);
        fprintf(stderr, "Set actor dialogue ref: %s\n", target->dialogue_file_reference);
    }
}

static spell* lookup_spell(actor* self, game_state* state, cast_spell_info spell_info) {
    spell* result = NULL;

    if (spell_info.origin == CAST_SPELL_ORIGIN_DICTIONARY) {
        result = spell_dictionary_find_spell(&state->spells, spell_info.id);
    } else if (spell_info.origin == CAST_SPELL_ORIGIN_SPELLBOOK) {
    }

    return result;
}

static i32 actor_command_calculate_estimated_cost(actor* self, game_state* state, actor_command command) {
    switch (command.type) {
        case ACTOR_COMMAND_MOVE_TO:
        {
            vec2 start = command.movement.start_position;
            vec2 end = command.movement.position;
            f32 x_delta = end.x - start.x;
            f32 y_delta = end.y - start.y;
            i32 distance = ceilf(sqrtf((x_delta * x_delta) + (y_delta * y_delta)));
            return i32_max(1, distance);
        }
        break;
        case ACTOR_COMMAND_ATTACK:
        {
            // get weapon attributes and calculate
            // based on that anyways.
            // for now just do fixed five.
            return 5;
        }
        break;
        case ACTOR_COMMAND_CAST_SPELL:
        {
            spell* to_cast = lookup_spell(self, state, command.cast_spell.spell_info);

            if (to_cast) {
                return to_cast->action_points_cost;
            } else {
                return 0;
            }
        }
        break;
    }

    return 0;
}

i32 actor_calculate_estimated_cost_of_current_command(actor* self, game_state* state) {
    return actor_command_calculate_estimated_cost(self, state, self->current_command);
}

static void actor_handle_movement_command( actor* target, game_state* state, f32 delta_time );
static void actor_handle_attack_other_command(actor* self, game_state* state, f32 delta_time);
static void actor_handle_cast_spell_command(actor* self, game_state* state, f32 delta_time);

i32 actor_run_command( actor* target, game_state* state, f32 delta_time ){
    if (actor_is_dead(target)) {
        return RUN_COMMAND_FINISHED;
    } else {
        actor_command* command = &target->current_command;

        i32 action_point_cost = actor_calculate_estimated_cost_of_current_command(target, state);

        if (state->player_is_in_combat) {
            if (target->action_points < action_point_cost) {
                command->finished = true;
                command->type = ACTOR_COMMAND_NONE;
                return RUN_COMMAND_NO_ACTION_POINTS_TO_EXECUTE;
            }
        }

        if (!command->finished) {
            switch( command->type ){
                case ACTOR_COMMAND_MOVE_TO:
                {
                    actor_handle_movement_command(target, state, delta_time);
                }
                break;
                case ACTOR_COMMAND_ATTACK:
                {
                    actor_handle_attack_other_command(target, state, delta_time);
                }
                break;
                case ACTOR_COMMAND_CAST_SPELL:
                {
                    actor_handle_cast_spell_command(target, state, delta_time);
                }
                break;
            }
        } else if (command->finished &&
                   command->type != ACTOR_COMMAND_NONE) {
            if (state->player_is_in_combat) {
                target->action_points -= action_point_cost;
            }

            command->type = ACTOR_COMMAND_NONE;
        }

        return command->finished;
    }
}

static b32 actor_movement_intersects_tilemap( actor* target, vec2 velocity, f32 delta_time ){
    rectangle player_rect = {
        .position = {
            target->position.x + velocity.x * delta_time,
            target->position.y + velocity.y * delta_time
        },
        .size = target->size
    };

    tilemap* map = target->current_map;

    for( u32 y = 0; y < map->height; ++y ){
        for( u32 x = 0; x < map->width; ++x ){
            if( map->tiles[y][x].solid ){
                rectangle wall_rect = {
                    .position = v2(x, y),
                    .size = v2(1, 1)
                };

                b32 intersection = rectangle_intersects( wall_rect, player_rect );

                if( intersection ){
                    return true;
                }
            }
        }
    }

    return false;
}

static void actor_handle_movement_command( actor* target, game_state* state, f32 delta_time ){
    const f32 move_speed = 3;
    const f32 good_enough_cutoff = 0.02855f;

    actor_command* command = &target->current_command;

    vec2 direction_to_walk = v2(command->movement.position.x - target->position.x,
                                command->movement.position.y - target->position.y);

    target->visual_info.looking_direction = direction_to_walk;
    direction_to_walk = vec2_normalize(direction_to_walk);

    vec2 velocity = v2(direction_to_walk.x * move_speed,
                       direction_to_walk.y * move_speed);

    b32 finished_x = fabs(target->position.x - (command->movement.position.x)) < good_enough_cutoff;
    b32 finished_y = fabs(target->position.y - (command->movement.position.y)) < good_enough_cutoff;

    b32 hit_tilemap =
        actor_movement_intersects_tilemap( target,
                                           velocity,
                                           delta_time );

    if( (finished_x || finished_y) || (hit_tilemap) ){
        command->finished = true;
    }else if( !hit_tilemap ){
        target->position.x += (velocity.x * delta_time);
        target->position.y += (velocity.y * delta_time);
    }
}

static void actor_handle_cast_spell_command(actor* self, game_state* state, f32 delta_time) {
    actor_command* command = &self->current_command;
    spell* to_cast = lookup_spell(self, state, command->cast_spell.spell_info);

    if (to_cast) {
        fprintf(stderr, "\"%s\" Casting : %s\n", self->name, to_cast->name);

        switch (command->cast_spell.target_info.type) {
            case SPELL_TARGET_AREA:
            {
            }
            break;
            case SPELL_TARGET_ACTOR:
            {
                actor_apply_spell(command->cast_spell.target_info.pointer, state, to_cast);
            }
            break;
            case SPELL_TARGET_CONTAINER:
            {
            }
            break;
            case SPELL_TARGET_ITEM_PICKUP:
            {
            }
            break;
        }

        command->finished = true;
        /* 
           I forgot I didn't think about how I would handle casting time.
           command->cast_spell.cast_timer++; 
        */
    } else {
        fprintf(stderr, "Spell lookup failed\n");
        command->finished = true;
    }
}

enum hit_miss_constant {
    MISS,
    HIT,
    CRITICAL_HIT,
    CRITICAL_MISS
};

// current_target should be an index...
static void actor_handle_attack_other_command(actor* self, game_state* state, f32 delta_time) {
    actor_command* command = &self->current_command;
    actor* target = command->attacking_other.target;
    
    if (actor_is_dead(target)) {
        command->finished = true;
        self->current_target = NULL;
        fprintf(stderr, "target is dead. Not attacking and removing attack\n");
    } else {
        /*
         * I'd be waiting for the
         * correct attack frame, but I don't have such
         * animations so we'll get back to shuffle hit.
         */
        self->current_target = target;
        self->visual_info.animation = ACTOR_ANIMATION_GROUP_ATTACK;
        i32 attack_roll = random_integer_ranged(1, 20);

        i32 hit = (attack_roll > actor_get_effective_armor_class(self->current_target));
        damage_info rolled_damage = pure_physical_damage(random_integer_ranged(4, 18), 10);

        char attack_info_text[FLAVOR_TEXT_BUFFER_LENGTH];

        // simple combat rules for now
        if (attack_roll == 20) {
            rolled_damage.damage *= 2;
            hit = HIT;
            snprintf(attack_info_text, FLAVOR_TEXT_BUFFER_LENGTH, "Critical Hit!");
        } else if (attack_roll == 1) {
            hit = CRITICAL_MISS;
            snprintf(attack_info_text, FLAVOR_TEXT_BUFFER_LENGTH, "Critical Miss!");
        }

        if (hit == HIT || hit == CRITICAL_HIT) {
            actor_hurt(self->current_target, state, rolled_damage);
            if (hit == CRITICAL_HIT) {
                floating_messages_on_actor(&state->floating_messages, self, (colorf){0, 1, 0, 1}, attack_info_text);
            }
        } else if (hit == MISS) {
            snprintf(attack_info_text, FLAVOR_TEXT_BUFFER_LENGTH, "Miss!");
            floating_messages_on_actor(&state->floating_messages, self, (colorf){1, 0, 0, 1}, attack_info_text);
        } else if (hit == CRITICAL_MISS) {
            floating_messages_on_actor(&state->floating_messages, self, (colorf){1, 0, 0, 1}, attack_info_text);
        }

        command->finished = true;
        
    }
}

b32 actor_is_currently_walking(actor* self) {
    if (!self->finished_turn) {
        if (self->current_command.finished) {
            return false;
        } else if (!self->current_command.finished &&
                   self->current_command.type == ACTOR_COMMAND_MOVE_TO) {
            return true;
        }
    }

    return false;
}

b32 actor_finished_combat_turn_actions(actor* self) {
    return self->finished_turn;
}
// not totally complete.
// This is only for active aggressiveness though.
b32 actor_is_aggressive_to(actor* self, actor* target) {
    if (actor_is_dead(self) || actor_is_dead(target)) {
        return false;
    }

    if (self == target) {
        return false;
    } else {
        if (self->current_target == target) {
            return true;
        }

        if (target->current_target == self) {
            return true;
        }
    }

    return false;
}

// only returns true when the other stack can be completely emptied.
// will return false if the stack cannot fit or has any remainder.
// remainder stuff is handled by caller.
b32 item_slot_add_to(item_slot* slot, item_slot* other) {
    if (item_slot_is_empty(slot)) {
        *slot = *other;
        return true;
    } else {
        if (item_slot_equal(slot, other)) {
            i32 can_fit = item_slot_can_fit_more(slot);
#if 0
            fprintf(stderr, "can_fit : %d vs other->item_count : %d\n", can_fit, other->item_count);
#endif

            if (can_fit >= other->item_count) {
                slot->item_ptr = other->item_ptr;
                slot->item_count += other->item_count;
                other->item_count = 0;
                return true;
            } else if ((can_fit) && (can_fit <= other->item_count)) {
                other->item_count -= can_fit;
                slot->item_ptr = other->item_ptr;
                slot->item_count += can_fit;
                return false;
            } else if (can_fit <= 0) {
                return false;
            }
        } else {
            return false;
        }
    }

    // to suppress warning, I think the above covers all paths.
    return false;
}

b32 actor_add_item(actor* self, item_slot item) {
    item.owner_index = self->ref_id;

    for (unsigned item_slot_index = 0;
         item_slot_index < ACTOR_MAX_INVENTORY_ITEMS;
         ++item_slot_index) {
        item_slot* current_slot = &self->inventory[item_slot_index];

        b32 placement_result = item_slot_add_to(current_slot, &item);

        if (placement_result) {
            return true;
        } else {
            continue;
        }
    }

    return false;
}

void actor_pickup_item_from_map(actor* self, u32 item_index) {
    tilemap* map = self->current_map;

    map_item_pickup* pickup = tilemap_get_pickup(map, item_index);
    pickup->contains_item = false;

    /*
     * This implementation is incomplete in so many ways...
     * If we cannot fit the entire item in the inventory we'd have an issue.
     * Thankfully the floor can only fit one stack which makes things much easier.
     */
    actor_add_item(self, pickup->item_to_pickup);
}

void actor_start_showing_info_bars(actor* self) {
    actor_start_showing_action_points_bar(self);
    actor_start_showing_health_bar(self);
}

void actor_set_enchantment_resist(actor* self, i8 level) {
    self->resistance_info.enchantment = level;
}

void actor_set_resistance(actor* self, u8 type, i32 value) {
    self->resistance_info.resistances[type].value = value;
}

void actor_start_showing_health_bar(actor* self) {
    self->visual_info.health_bar_show_time = health_bar_max_show_time;
}

void actor_start_showing_action_points_bar(actor* self) {
    self->visual_info.action_points_bar_show_time = action_points_bar_max_show_time;
}

i32 actor_get_effective_armor_class(actor* self) { 
    return self->armor_class;
}

damage_resist actor_get_resistance(actor* self, u8 type) {
    return self->resistance_info.resistances[type];
}

void actor_hurt(actor* self, game_state* state, damage_info damage) {
    // begin actor_hurt_animation
    {
        self->visual_info.flashes_left = flash_cycles_per_hit;
        self->visual_info.hurt_flash_timer = time_per_flash;
        self->visual_info.flash_cycle = true;
        self->visual_info.healing_flash = false;
    }
    actor_start_showing_info_bars(self);

    char damage_text[FLAVOR_TEXT_BUFFER_LENGTH];

    if (self->resistance_info.enchantment > damage.enchantment_level) {
        snprintf(damage_text, FLAVOR_TEXT_BUFFER_LENGTH, "Ineffective %s Damage!", damage_type_names[damage.type]);
        floating_messages_on_actor(&state->floating_messages, self, (colorf){1, 1, 0, 1}, damage_text);
    } else {
        i32 actual_damage = damage.damage;
        i32 armor_class = actor_get_effective_armor_class(self);

        // damage calculations
        // figure out something better.
        {
            // flat resists.
            damage_resist resist_value = actor_get_resistance(self, damage.type);

            if (damage.type == DAMAGE_TYPE_PHYSICAL_BLUNT ||
                damage.type == DAMAGE_TYPE_PHYSICAL_THRUSTING ||
                damage.type == DAMAGE_TYPE_PHYSICAL_SLASHING) {
                damage_resist main_physical_resistance = actor_get_resistance(self, damage.type);
                resist_value.value += (main_physical_resistance.value / 2); 
            }

            actual_damage -= resist_value.value;
            actual_damage -= ceilf((f32)(armor_class * 0.17));
        }

        actual_damage = i32_max(actual_damage, 0);

        if (actual_damage) {
            snprintf(damage_text, FLAVOR_TEXT_BUFFER_LENGTH, "%d %s Damage!", actual_damage, damage_type_names[damage.type]);
            floating_messages_on_actor(&state->floating_messages, self, (colorf){1, 0, 0, 1}, damage_text);
        } else if (actual_damage == 0) {
            snprintf(damage_text, FLAVOR_TEXT_BUFFER_LENGTH, "Absorbed attack entirely!");
            floating_messages_on_actor(&state->floating_messages, self, (colorf){1, 0, 0, 1}, damage_text);
        }

        self->health_points -= actual_damage;
    }
}

void actor_heal(actor* self, game_state* state, i32 health) {
    self->health_points += health;
    self->health_points = i32_clamp(self->health_points, 0, self->max_health_points);
    // begin actor_heal_animation
    {
        self->visual_info.flashes_left = flash_cycles_per_hit;
        self->visual_info.hurt_flash_timer = time_per_flash;
        self->visual_info.flash_cycle = true;
        self->visual_info.healing_flash = true;
    }
    actor_start_showing_health_bar(self);

    char info_text[FLAVOR_TEXT_BUFFER_LENGTH];
    snprintf(info_text, FLAVOR_TEXT_BUFFER_LENGTH, "+%d Health", health);
    floating_messages_on_actor(&state->floating_messages,
                               self,
                               (colorf){0, 1, 0, 1},
                               info_text);
}

// adjusted until it looks okay in and out of turn-based mode.
void actor_think(actor* self, game_state* state, f32 delta_time) {
    if (self->think_type == ACTOR_THINK_TYPE_PLAYER) {
        return;
    }

    // artifical delay for turn-based mode.
    if (!self->think_info.started_wait_time) {
        self->think_info.started_wait_time = true;
        self->think_info.think_timer = 2;
    } else {
        self->think_info.think_timer -= delta_time;
    }

    b32 should_think_now =
        (!state->player_is_in_combat) || (self->think_info.think_timer <= 0.0f);

    if (should_think_now) {
        switch (self->think_type) {
            case ACTOR_THINK_TYPE_BLANK:
            {
                actor_end_turn(self);
                self->current_target = NULL;
                self->current_command.type = ACTOR_COMMAND_NONE;
                self->current_command.finished = true;
            }
            break;
            case ACTOR_THINK_TYPE_DUMMY:
            {
                // nothing yet
            }
            break;
            default:
            {
            }
            break;
        }

        self->think_info.started_wait_time = false;
    }
}

void actor_apply_spell(actor* self, game_state* state, spell* to_apply) {
    for (u64 spell_effect_index = 0; spell_effect_index < to_apply->effect_count; ++spell_effect_index) {
        actor_push_effect(self, state, to_apply->effects[spell_effect_index]);
    }
}

void actor_push_effect(actor* self, game_state* state, magic_effect effect) {
    if (self->active_spell_effects >= ACTOR_MAX_ACTIVE_SPELL_EFFECTS) {
        return;
    }

    const b32 is_instant_effect = (effect.delay_duration == 0 && effect.duration == 0);

    if (is_instant_effect) {
        actor_apply_effect(self, state, effect);
    } else {
        magic_effect* next_effect = &self->effects[self->active_spell_effects++];
        *next_effect = effect;
    }
}

void actor_remove_effect(actor* self, u16 index) {
    if (index >= self->active_spell_effects) {
        return;
    }

    self->effects[index] = self->effects[self->active_spell_effects];
    self->active_spell_effects--;
}

void actor_end_turn(actor* self) {
    if (!self->finished_turn) {
        self->finished_first_turn = false;
        self->finished_turn = true;
    }
}

void actor_on_turn_start_update(actor* self, game_state* state) {
    if (!self->finished_first_turn) {
        self->finished_first_turn = true;
    }
}

void actor_on_turn_end_update(actor* self, game_state* state) {
    self->finished_first_turn = false;
    self->finished_turn = false;
}

void actor_turn_update(actor* self, game_state* state) {
    // This list has to be filtered for "appliable" effects
    // this list contains any spell that has either a duration or delay.
    for (u32 effect_index = 0; effect_index < self->active_spell_effects; ++effect_index) {
        if (self->effects[effect_index].delay_duration == 0) {
            actor_apply_effect(self, state, self->effects[effect_index]);

            if (self->effects[effect_index].duration != 0) {
                self->effects[effect_index].duration--;
            }
        } else {
            self->effects[effect_index].delay_duration--;
        }
    }

    u32 end_of_list = self->active_spell_effects;
    for (u32 effect_index = 0; effect_index < end_of_list; ++effect_index) {
        if (self->effects[effect_index].delay_duration == 0 &&
            self->effects[effect_index].duration == 0) {
            actor_remove_effect(self, effect_index);
        }
    }
}
