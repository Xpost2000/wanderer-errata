// Also contains containers for some reason.
#ifndef WANDERER_ACTOR_H
#define WANDERER_ACTOR_H

#include "common.h"
#include "wanderer_types_forward_declarations.h"

#include "wanderer_assets.h"

#include "color.h"
#include "vec2.h"
#include "rect.h"

#include "wanderer_item.h"
#include "wanderer_magic.h"

#define ACTOR_HIDDEN_CURRENCY_SLOT_ITEMS 1
#define ACTOR_MAX_INVENTORY_ITEMS 30
#define ACTOR_MAX_ACTIVE_SPELL_EFFECTS 96
#define CONTAINER_MAX_INVENTORY_ITEMS 50

typedef struct container{
    char* name;

    vec2 position;
    vec2 size;

    b32 requires_key_item;
    b32 locked;

    union{
        u32 lock_level;
        u32 key_id;
    };

    item_slot inventory[CONTAINER_MAX_INVENTORY_ITEMS];
}container;

void container_add_item(container* self, item_slot item);

static const i32 flash_cycles_per_hit = 8;
static const f32 time_per_flash = 0.052f;
static const f32 health_bar_max_show_time = 2;
static const f32 action_points_bar_max_show_time = 2;

enum attack_animation_stage {
    ATTACK_ANIMATION_NOT_STARTED,
    ATTACK_ANIMATION_FORWARD_MOVEMENT,
    ATTACK_ANIMATION_BACKWARD_MOVEMENT,
    ATTACK_ANIMATION_END
};

typedef struct actor_visual_info{
    u16 animation;
    game_asset_handle model_id;

    u32 animation_finish_iterations;
    f32 current_frame_timer;
    u32 current_frame;
    u16 selected_set;

    vec2 looking_direction;

    vec2 position;
    vec2 scale;

    colorf rgba;

    f32 health_bar_show_time;
    f32 action_points_bar_show_time;

    // for certain animations.
    // These are not part of the model files.
    // Readded for temporary reasons
    i32 flashes_left;
    f32 hurt_flash_timer;
    b32 flash_cycle;
    b32 healing_flash; // hurt and heal flash are the same. Just different color.

    f32 attack_animation_duration;
    f32 attack_animation_timer;
    i32 attack_animation_stage;
    vec2 attack_animation_backward_start_position;
}actor_visual_info;

#define ACTOR_QUICK_WEAPON_SLOTS 3
#define ACTOR_QUICK_ITEM_BAR_SLOTS 4

enum ability_score_id{
    ABILITY_SCORE_STRENGTH,
    ABILITY_SCORE_DEXTERITY,
    ABILITY_SCORE_WILLPOWER,
    ABILITY_SCORE_INTELLIGENCE,
    ABILITY_SCORE_CONSTITUTION,
    ABILITY_SCORE_CHARISMA,

    ABILITY_SCORE_TYPES
};

typedef struct ability_score_info{
    union{
        struct{
            i8 strength;
            i8 dexterity;
            i8 willpower;
            i8 intelligence;
            i8 constitution;
            i8 charisma;
        };

        i8 scores[ABILITY_SCORE_TYPES];
    };
}ability_score_info;

enum actor_command_action {
    ACTOR_COMMAND_NONE,
    ACTOR_COMMAND_MOVE_TO,
    ACTOR_COMMAND_ATTACK,
    ACTOR_COMMAND_CAST_SPELL,
    ACTOR_COMMAND_COUNT
};

static char* actor_command_action_strings[ACTOR_COMMAND_COUNT] = {
    STRINGIFY(ACTOR_COMMAND_NONE),
    STRINGIFY(ACTOR_COMMAND_MOVE_TO),
    STRINGIFY(ACTOR_COMMAND_ATTACK),
    STRINGIFY(ACTOR_COMMAND_CAST_SPELL)
};

// TODO(jerry): How do I handle projectiles now?
// cause I still need to wait until it's finished
// also yeah figure this one out later...
// need to distinguish between scripted casting "forced cast" and
// normal game casting...

enum cast_spell_origin {
    CAST_SPELL_ORIGIN_DICTIONARY,
    CAST_SPELL_ORIGIN_SPELLBOOK
};

typedef struct cast_spell_info {
    u8 origin;

    union {
        u16 index;
        char* id;
    };
} cast_spell_info;

typedef struct actor_command_cast_spell {
    b32 was_forced;
    cast_spell_info spell_info;

    spell_target target_info;
    i32 cast_timer;
} actor_command_cast_spell;

typedef struct actor_command_attacking_other {
    actor* target;
} actor_command_attacking_other;

typedef struct actor_command_movement {
    vec2 start_position;
    vec2 position;
} actor_command_movement;

typedef struct actor_command {
    u8 type;
    b32 finished;

    union {
        actor_command_movement movement;
        actor_command_attacking_other attacking_other;
        actor_command_cast_spell cast_spell;
    };
}actor_command;

#define ACTOR_NAME_STRING_LENGTH 64
#define ACTOR_DIALOGUE_REFERENCE_NAME_LENGTH 64
#define ACTOR_MINIMUM_ACCEPTABLE_ACTION_POINTS 20

typedef struct damage_resist{
    i32 value;
}damage_resist;

typedef struct damage_resistance_info{
    i8 enchantment;
    damage_resist resistances[DAMAGE_TYPES];
}damage_resistance_info;

void actor_set_enchantment_resist(actor* self, i8 level);
void actor_set_resistance(actor* self, u8 type, i32 value);
damage_resist actor_get_resistance(actor* self, u8 type);

enum actor_think_type {
    ACTOR_THINK_TYPE_BLANK,
    ACTOR_THINK_TYPE_PLAYER,
    ACTOR_THINK_TYPE_DUMMY,
    ACTOR_THINK_TYPE_COUNT
};

// maybe move more stuff here.
typedef struct actor_think_info {
    b32 started_wait_time;
    f32 think_timer;
} actor_think_info;

typedef struct actor{
    u8 think_type;
    actor_think_info think_info;

    actor_command current_command;
    actor* current_target; // combat information for ai.
    b32 finished_first_turn; // for specific effects to apply. Or prethink information?
    b32 finished_turn; // used in turn-based combat. Manually set flag.

    damage_resistance_info resistance_info;
    ability_score_info ability_scores;

    actor_visual_info visual_info;

    u64 ref_id;
    char name[ACTOR_NAME_STRING_LENGTH];

    i32 max_health_points;
    i32 health_points;

    u32 action_points;
    u32 max_action_points;

    u32 armor_class;

    vec2 position;
    vec2 size;

    // ACTOR_MAX_INVENTORY_ITEMS + active_selected_weapon_index <=
    // ACTOR_MAX_INVENTORY_ITEMS + ACTOR_QUICK_ITEM_BAR_SLOTS
    // This will be a relative index.
    u8 active_selected_weapon_index;

    item_slot inventory[ACTOR_MAX_INVENTORY_ITEMS  + 
                        ACTOR_QUICK_WEAPON_SLOTS +
                        ACTOR_HIDDEN_CURRENCY_SLOT_ITEMS];

    u16 active_spell_effects;
    magic_effect effects[ACTOR_MAX_ACTIVE_SPELL_EFFECTS];

    b32 has_dialogue;
    char dialogue_file_reference[ACTOR_DIALOGUE_REFERENCE_NAME_LENGTH];

    tilemap* current_map;
}actor;

i32 actor_calculate_estimated_cost_of_current_command(actor* self, game_state* state);

// for more convenient command state querying
b32 actor_is_currently_walking(actor* self);

i32 actor_get_effective_armor_class(actor* self);
b32 actor_is_dead( actor* target );

void actor_start_showing_info_bars(actor* self);
void actor_start_showing_health_bar(actor* self);
void actor_start_showing_action_points_bar(actor* self);

/* This will lookup from a global spell list for the game */
void actor_force_cast_spell(actor* self, game_state* state, char* spell_name, spell_target target);
/* This will lookup from the spellbook index of the character */
void actor_force_cast_spell_from_spellbook(actor* self, game_state* state, u16 spell_index, spell_target target);
void actor_cast_spell(actor* self, game_state* state, u16 spell_index, spell_target target);

void actor_remove_effect(actor* self, u16 index);
void actor_push_effect(actor* self, game_state* state, magic_effect effect);

void actor_apply_spell(actor* self, game_state* state, spell* to_apply);

void actor_end_turn(actor* self);
void actor_on_turn_start_update(actor* self, game_state* state);
void actor_on_turn_end_update(actor* self, game_state* state);
void actor_turn_update(actor* self, game_state* state);

void actor_think(actor* self, game_state* state, f32 delta_time);
void actor_hurt(actor* self, game_state* state, damage_info damage);
void actor_heal(actor* self, game_state* state, i32 health);

b32 actor_is_aggressive_to(actor* self, actor* target);
b32 actor_finished_combat_turn_actions(actor* self);

actor actor_create_default(char* name, i32 health_points, u32 action_points);

b32 actor_add_item(actor* self, item_slot item);
void actor_pickup_item_from_map(actor* self, u32 item_index);

// These require game state to make sure if it's okay to override a command.
b32 actor_can_issue_new_command(actor* self, game_state* state);

void actor_command_attack(actor* self, game_state* state, actor* target);
void actor_command_move_to(actor* target, game_state* state, vec2 position);

enum run_command_result {
    RUN_COMMAND_NOT_FINISHED,
    RUN_COMMAND_FINISHED,
    RUN_COMMAND_NO_ACTION_POINTS_TO_EXECUTE,
    RUN_COMMAND_COUNT
};

i32 actor_run_command( actor* target, game_state* state, f32 delta_time );

void actor_update_visual_info( actor* target, game_assets* assets, f32 delta_time );

void actor_remove_dialogue_file_reference( actor* target );
void actor_set_dialogue_file_reference( actor* target, char* dialogue_file_name );

#endif
