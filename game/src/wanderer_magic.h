#ifndef WANDERER_MAGIC_H
#define WANDERER_MAGIC_H

#include "common.h"
#include "wanderer_types_forward_declarations.h"
#include "wanderer_item.h"

#include "vec2.h"
#include "color.h"

/*
  Might call something else but for now
  these things just do certain things.
  
  They are part of spells and when processed by entities
  will do different things.
  
  effects are applied by registering callbacks, although following
  the style for the rest of the engine / game, I'll use an enum for
  spell effects.
  
  I'll keep adding to this throughout the week as I see what spells I want
  and can make at the moment.

  Since I want poison first...
 */
enum magic_effect_type {
    MAGIC_EFFECT_DAMAGE_HEALTH,
    MAGIC_EFFECT_RESTORE_HEALTH,

    /*
      This effect might need special handling...
      Since my logic restores all action points on next turn,
      which means I might have to delay these effects.
    */
    MAGIC_EFFECT_DAMAGE_ACTION_POINTS,
    MAGIC_EFFECT_RESTORE_ACTION_POINTS,

    MAGIC_EFFECT_KILL_TARGET,
    MAGIC_EFFECT_VISUAL_FLASH,

    MAGIC_EFFECT_COUNT
};

// This effects the types of things an effect will try to hit.
// Particularly it applies to it's projectiles... or area.
enum magic_effect_target_flags {
    MAGIC_EFFECT_TARGET_FLAGS_NONE = 0, // something is wrong if this happens.
    MAGIC_EFFECT_TARGET_FLAGS_CASTER = BIT(0),
    MAGIC_EFFECT_TARGET_FLAGS_TARGET = BIT(2),
    MAGIC_EFFECT_TARGET_FLAGS_CASTER_AND_TARGET = BIT(0) | BIT(2),
    MAGIC_EFFECT_TARGET_FLAG_COUNT
};

enum magic_effect_resist_flags {
    MAGIC_EFFECT_RESIST_FLAG_NONE = 0,
    /*
      if this effect is indeed resistable this modifier is a percent
      that is applied relative to the current resistance.

      if the resist flags state it is absolute it will override the current
      resistance.

      EG: RESISTANCE_ABSOLUTE
      resist_chance = 35%, 35% roll to face effects.
      EG: NO RESISTANCE_ABSOLUTE
      resist_chance = 35%, 
      current_resistance + resist_chance

      This will probably be noted in descriptions as

      "This spell has 35% additional chance atop the current resistance
      to be resisted."

      "This spell has 35% chance of being resisted."
    */
    MAGIC_EFFECT_RESIST_FLAG_RESISTABLE = BIT(0),
    MAGIC_EFFECT_RESIST_FLAG_RESISTANCE_ABSOLUTE = BIT(1),
    MAGIC_EFFECT_RESIST_FLAG_COUNT = 2
};

// this is okay I think for reserving possible future stuff.
typedef struct generic_magic_effect {
    int magnitude;
    int params[5];
} generic_magic_effect;

typedef struct damaging_magic_effect {
    damage_range damage;
    u8 damage_type;
    u8 enchantment_level;
} damaging_magic_effect;

// TODO(jerry): Track caster.
typedef struct magic_effect {
    u16 effect_type;
    u8 target_flags;
    u8 resist_flags;
    /*
      Damage type that the effect will fall under.
      Will use that damage's specific resistance percent. (not armor class)
    */
    u8 resist_type;  
    f32 resist_modifier;

    // the time units are in turns.
    // A turn will be 3 seconds IRL outside of combat.

    // 0 = no delay before effect applies
    // >= 1 = time to wait before allowing the effect to apply.
    u8 delay_duration;
    // 0 = INSTANT 
    // >= 1 = Amount of turns to apply consecutively.
    u8 duration;

    // union provided for readability purposes.
    union {
        generic_magic_effect generic;
        damaging_magic_effect damaging;
        rolled_range roll_range;
    };
} magic_effect;

enum spell_target_type {
    SPELL_TARGET_ACTOR,
    SPELL_TARGET_AREA, // Position with radius.
    SPELL_TARGET_CONTAINER,
    SPELL_TARGET_ITEM_PICKUP,

    SPELL_TARGET_TYPE_COUNT
};
static const char* spell_target_type_strings[] = {
    STRINGIFY(SPELL_TARGET_ACTOR),
    STRINGIFY(SPELL_TARGET_AREA),
    STRINGIFY(SPELL_TARGET_CONTAINER),
    STRINGIFY(SPELL_TARGET_ITEM_PICKUP)
};

typedef struct spell_target_area {
    f32 radius;
    vec2 position;
} spell_target_area;

typedef struct spell_target {
    enum spell_target_type type;
    union {
        void* pointer;
        spell_target_area area;
    };
} spell_target;

// inconsistent name.
spell_target make_spell_target_area(vec2 position, f32 radius);
spell_target make_spell_target_actor(actor* pointer);

typedef struct spell {
    char* name;
    char* description;

    u16 effect_count;
    magic_effect* effects;

    u8 level;
    u8 target_flags;

    i32 action_points_cost;
} spell;

typedef struct spell_dictionary_key_value_pair {
    char* key;
    spell value;
} spell_dictionary_key_value_pair;

typedef struct spell_dictionary {
    u64 count;
    spell_dictionary_key_value_pair* spells;
} spell_dictionary;

typedef struct actor actor;
void actor_apply_effect(actor* self, game_state* state, magic_effect effect);

void spell_dictionary_init(spell_dictionary* self, u64 max_capacity);
void spell_dictionary_add_spell(spell_dictionary* self, spell to_add, const char* lookup_name);
spell* spell_dictionary_find_spell(spell_dictionary* self, const char* id);
void spell_dictionary_finish(spell_dictionary* self);

void spell_set_name(spell* self, const char* name);
void spell_set_description(spell* self, const char* description);
void spell_push_effect(spell* self, magic_effect effect);

magic_effect test_delayed_poison_spell_effect(void);
magic_effect test_poison_spell_effect(void);

magic_effect test_heal_spell_effect(void);
magic_effect test_flash_visual_effect(colorf rgba, i32 flashes, f32 time_between_flashes, f32 flash_linger_time);

#endif
